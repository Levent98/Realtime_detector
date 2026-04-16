#include "uart.h"
#include "stm32f410rx.h"
#include "delay.h"
#include <string.h>
#define UART_TX_QUEUE_SIZE 8
volatile uint16_t baud_rate;
/*============================ STATIC DEGISKENLER =========================*/
typedef struct {
    uint8_t buf[UART_RX_BUF_SIZE];
    volatile uint16_t head;  /* DMA yazdigi son konum */
    volatile uint16_t tail;  /* Okunan konum */
} ring_t;
typedef enum
{
    RTU_IDLE = 0,
    RTU_RECEIVING
} rtu_state_t;
typedef struct {
    uint8_t data[UART_TX_BUF_SIZE];
    uint16_t len;
} tx_item_t;

static struct {
    tx_item_t items[UART_TX_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} tx_queue;
static rtu_state_t rtu_state = RTU_IDLE;

static uint16_t rtu_last_head = 0;
static uint16_t rtu_frame_head = 0;

static uint32_t rtu_deadline = 0;
//static uint8_t  rtu_frame_ready = 0;
static ring_t rx_ring;
static uint8_t tx_buf[UART_TX_BUF_SIZE];
volatile uint8_t uart_tx_busy = 0;

static volatile uint32_t rtu_gap_us = 0U;
static volatile uint32_t rtu_deadline_us = 0U;
//volatile uint8_t rtu_frame_ready = 0U;
static volatile uint16_t rtu_frame_head_snapshot = 0;
volatile uint8_t  rtu_frame_ready = 0;
/* Komut kuyrugu */
static cmd_queue_t cmd_queue;
// Baud hizina göre 3.5 karakter süresinin kaç "50us" adimina denk geldigi
// Örn: 115200 baud -> 1750us -> 1750/50 = 35 adim
volatile uint16_t modbus_t35_steps = 35; 
volatile uint16_t modbus_timer_counter = 0;
volatile uint8_t  modbus_timer_running = 0;
//volatile uint8_t  rtu_frame_ready = 0;
//volatile uint16_t rtu_frame_head = 0;
//static void UART_StartDMA(uint8_t *data, uint16_t len);
/*============================ STATIC FONKSIYONLAR =========================*/

/**
 * @brief DMA head pointer oku
 * @return Güncel head indeksi
 */
static inline uint16_t dma_head(void)
{
    return UART_RX_BUF_SIZE - DMA2_Stream2->NDTR;
}

/*============================ RING BUFFER FONKSIYONLARI ===================*/

/**
 * @brief Ring buffer'da okunabilir byte sayisi
 * @return Okunabilir byte sayisi
 */
uint16_t UART_RxAvailable(void)
{
    uint16_t head, tail;
    
    /* Tutarli okuma için double-check */
    do {
        rx_ring.head = dma_head();
        tail = rx_ring.tail;
        head = dma_head();
    } while (rx_ring.head != head);

    if(head >= tail)
        return head - tail;
    else
        return UART_RX_BUF_SIZE - tail + head;
}

/**
 * @brief Ring buffer'dan veri oku
 * @param dst Okunan verinin yazilacagi buffer
 * @param len Okunacak maksimum byte sayisi
 * @return Gerçekte okunan byte sayisi
 */
uint16_t UART_RxRead(uint8_t *dst, uint16_t len)
{
    uint16_t avail = UART_RxAvailable();
    if(len > avail) len = avail;

    for(uint16_t i = 0; i < len; i++)
    {
        dst[i] = rx_ring.buf[rx_ring.tail++];
        if(rx_ring.tail >= UART_RX_BUF_SIZE)
            rx_ring.tail = 0;
    }
    return len;
}

/*============================ KOMUT KUYRUGU FONKSIYONLARI =================*/

/**
 * @brief Komut kuyruguna yeni komut ekle
 * @param data Komut verisi
 * @param len Komut uzunlugu
 * @return 1: basarili, 0: kuyruk dolu
 */
uint8_t UART_QueueCommand(uint8_t *data, uint16_t len)
{
    uint8_t success = 0;
    
    if(len > 256) len = 256;  /* Güvenlik kontrolü */
    
    /* Kritik bölge baslangici */
    __disable_irq();
    
    if(cmd_queue.count < UART_CMD_QUEUE_SIZE)
    {
        /* Komutu kuyruga kopyala */
        memcpy(cmd_queue.cmds[cmd_queue.head].data, data, len);
        cmd_queue.cmds[cmd_queue.head].len = len;
        cmd_queue.cmds[cmd_queue.head].timestamp = GetTime_us();
        
        /* head indeksini güncelle */
        cmd_queue.head = (cmd_queue.head + 1) % UART_CMD_QUEUE_SIZE;
        cmd_queue.count++;
        success = 1;
    }
    
    /* Kritik bölge sonu */
    __enable_irq();
    
    return success;
}

/**
 * @brief Kuyruktan komut al
 * @param cmd Alinan komutun depolanacagi yapi
 * @return 1: basarili, 0: kuyruk bos
 */
uint8_t UART_DequeueCommand(cmd_item_t *cmd)
{
    uint8_t success = 0;
    
    if(cmd == NULL) return 0;
    
    /* Kritik bölge baslangici */
    __disable_irq();
    
    if(cmd_queue.count > 0)
    {
        /* Komutu kuyruktan al */
        memcpy(cmd->data, cmd_queue.cmds[cmd_queue.tail].data, 
               cmd_queue.cmds[cmd_queue.tail].len);
        cmd->len = cmd_queue.cmds[cmd_queue.tail].len;
        cmd->timestamp = cmd_queue.cmds[cmd_queue.tail].timestamp;
        
        /* tail indeksini güncelle */
        cmd_queue.tail = (cmd_queue.tail + 1) % UART_CMD_QUEUE_SIZE;
        cmd_queue.count--;
        success = 1;
    }
    
    /* Kritik bölge sonu */
    __enable_irq();
    
    return success;
}

/**
 * @brief Kuyruktaki komut sayisini döndür
 * @return Kuyruktaki komut sayisi
 */
uint8_t UART_GetQueueCount(void)
{
    uint8_t count;
    
    __disable_irq();
    count = cmd_queue.count;
    __enable_irq();
    
    return count;
}

/**
 * @brief Kuyruk dolu mu kontrol et
 * @return 1: dolu, 0: dolu degil
 */
uint8_t UART_IsQueueFull(void)
{
    uint8_t full;
    
    __disable_irq();
    full = (cmd_queue.count >= UART_CMD_QUEUE_SIZE);
    __enable_irq();
    
    return full;
}

/**
 * @brief Kuyruk bos mu kontrol et
 * @return 1: bos, 0: bos degil
 */
uint8_t UART_IsQueueEmpty(void)
{
    uint8_t empty;
    
    __disable_irq();
    empty = (cmd_queue.count == 0);
    __enable_irq();
    
    return empty;
}

/**
 * @brief Kuyrugu temizle
 */
void UART_ClearQueue(void)
{
    __disable_irq();
    cmd_queue.head = 0;
    cmd_queue.tail = 0;
    cmd_queue.count = 0;
    __enable_irq();
}

/*============================ UART BASLATMA FONKSIYONU ====================*/

/**
 * @brief UART ve DMA baslat
 * @param baud Baud rate
 */
void UART_Init(uint32_t baud)
{
    /* Ring buffer'i temizle */
    memset((void*)rx_ring.buf, 0, UART_RX_BUF_SIZE);
    rx_ring.head = 0;
    rx_ring.tail = 0;
    baud_rate=baud;
    /* Komut kuyrugunu temizle */
    cmd_queue.head = 0;
    cmd_queue.tail = 0;
    cmd_queue.count = 0;
    memset((void*)cmd_queue.cmds, 0, sizeof(cmd_queue.cmds));

    /* 1. Saatleri etkinlestir */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    /* 2. GPIO yapilandirma (PA9 TX, PA10 RX) */
    GPIOA->MODER &= ~((3U << (9*2)) | (3U << (10*2)));
    GPIOA->MODER |=  ((2U << (9*2)) | (2U << (10*2)));  /* Alternate function */
    GPIOA->OSPEEDR |= ((3U << (9*2)) | (3U << (10*2))); /* High speed */
    GPIOA->AFR[1] |= (7U << 4) | (7U << 8);  /* AF7 for USART1 */

    /* RS485 DE (PA8) - Data Enable pin */
    GPIOA->MODER &= ~(3U << (8*2));
    GPIOA->MODER |= (1U << (8*2));  /* Output */
    GPIOA->BSRR = GPIO_BSRR_BR8;    /* Receive mode (DE low) */

    /* 3. USART yapilandirma */
    USART1->BRR = 24000000UL / baud;  /* 24 MHz sistem clock */
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_IDLEIE;
    USART1->CR1 |= USART_CR1_UE;  /* USART enable */
    tx_queue.head = 0;
    tx_queue.tail = 0;
    tx_queue.count = 0;
    /* 4. RX DMA yapilandirma (Stream2 Channel4) */
    DMA2_Stream2->CR &= ~DMA_SxCR_EN;
    while(DMA2_Stream2->CR & DMA_SxCR_EN);

    DMA2_Stream2->PAR = (uint32_t)&USART1->DR;
    DMA2_Stream2->M0AR = (uint32_t)rx_ring.buf;
    DMA2_Stream2->NDTR = UART_RX_BUF_SIZE;
    DMA2_Stream2->CR = (4U << 25) | (2U << 16) | DMA_SxCR_MINC | DMA_SxCR_CIRC;
    DMA2->LIFCR = 0x3DU << 16;
    DMA2_Stream2->CR |= DMA_SxCR_EN;

    USART1->CR3 |= USART_CR3_DMAR;  /* DMA enable for RX */

    /* 5. NVIC yapilandirma */
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);
    
		/* Istege bagli: öncelik ayarla */
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_SetPriority(DMA2_Stream7_IRQn, 2);
    /* 3.5 karakter timeout hesapla */
    //RTU_GapInit(baud);
}

uint8_t UART_Send(const uint8_t *data, uint16_t len)
{
    if(len == 0 || len > UART_TX_BUF_SIZE)
        return 0;

    if(uart_tx_busy)
        return 0;   // UART mesgul

    uart_tx_busy = 1;

    memcpy(tx_buf, data, len);

    /* RS485 transmit mode */
    GPIOA->BSRR = GPIO_BSRR_BS8;

    DMA2_Stream7->CR &= ~DMA_SxCR_EN;
    while(DMA2_Stream7->CR & DMA_SxCR_EN);

    DMA2->HIFCR = DMA_HIFCR_CTCIF7 |
                  DMA_HIFCR_CHTIF7 |
                  DMA_HIFCR_CTEIF7 |
                  DMA_HIFCR_CDMEIF7 |
                  DMA_HIFCR_CFEIF7;

    DMA2_Stream7->PAR  = (uint32_t)&USART1->DR;
    DMA2_Stream7->M0AR = (uint32_t)tx_buf;
    DMA2_Stream7->NDTR = len;

    DMA2_Stream7->CR =
        (4U << 25) |
        DMA_SxCR_MINC |
        DMA_SxCR_DIR_0 |
        DMA_SxCR_TCIE;

    USART1->CR3 |= USART_CR3_DMAT;
    DMA2_Stream7->CR |= DMA_SxCR_EN;

    return 1;
}

void USART1_IRQHandler(void)
{
	
// 1. Yeni karakter geldiginde (RXNE)
    if(USART1->SR & USART_SR_RXNE)
    {
        (void)USART1->DR; // Veriyi oku (DMA zaten aliyor ama bayrak temizlensin)
        
        // ÖNEMLI: Paket akarken sessizlik süresini SIFIRLA
        modbus_timer_running = 0; 
        modbus_timer_counter = 0;
    }

    // 2. Hat bosaldiginda (IDLE)
    if(USART1->SR & USART_SR_IDLE)
    {
        (void)USART1->SR; // IDLE bayragini temizleme sekansi
        (void)USART1->DR;

        // DMA'nin o ana kadar kaç byte yazdigini snapshot al
        rtu_frame_head = dma_head();

        // Sessizlik süresini (T3.5) ölçmeye BASLA
        modbus_timer_counter = 0;
        modbus_timer_running = 1; 

        // Baudrate 19200'den büyükse standart 1.75ms (35 adim)
        // Küçükse formül: (3.5 * 11 bit * 1.000.000 / baud) / 50us
        if(baud_rate > 19200) {
            modbus_t35_steps = 18;
        } else {
            modbus_t35_steps = (uint16_t)((38500000UL / baud_rate) / 50);
        }
    }
		 if(USART1->SR & USART_SR_TC)
    {
        /* TC flag temizle */
        (void)USART1->SR;
        USART1->DR; // bazi MCU’larda gerekebilir

        /* TC interrupt disable */
        USART1->CR1 &= ~USART_CR1_TCIE;

        /* DE LOW */
        GPIOA->BSRR = GPIO_BSRR_BR8;

        uart_tx_busy = 0;
    }
}

uint16_t UART_GetFrame(uint8_t *dst, uint16_t max_len)
{
    uint16_t head, tail, len;

    // Snapshot alinan kafa noktasi (IDLE anindaki NDTR degeri)
    head = rtu_frame_head;
    tail = rx_ring.tail;

    // Uzunluk hesabi (Circular Buffer)
    if(head >= tail)
        len = head - tail;
    else
        len = UART_RX_BUF_SIZE - tail + head;

    if(len == 0) return 0;
    if(len > max_len) len = max_len;

    // Veriyi dst buffer'ina kopyala ve tail'i ilerlet
    for(uint16_t i = 0; i < len; i++)
    {
        dst[i] = rx_ring.buf[rx_ring.tail++];
        if(rx_ring.tail >= UART_RX_BUF_SIZE)
            rx_ring.tail = 0;
    }

    return len;
}
/**
 * @brief DMA2 Stream7 kesme isleyicisi (TX tamam)
 */
void DMA2_Stream7_IRQHandler(void)
{
    if(DMA2->HISR & DMA_HISR_TCIF7)
    {
        /* DMA Transfer complete bayragini temizle */
        DMA2->HIFCR = DMA_HIFCR_CTCIF7;
     /* USART Transmission Complete interrupt enable */
        USART1->CR1 |= USART_CR1_TCIE;

    }
}

/*============================ ZAMAN FONKSIYONLARI =========================*/

/**
 * @brief Mikrosaniye cinsinden zaman al
 * @return Mikrosaniye cinsinden geçen süre
 */
uint32_t GetTime_us(void)
{
    uint32_t ms1, ms2;
    uint16_t us;

    do
    {
        ms1 = g_tick;
        us  = TIM6->CNT;
        ms2 = g_tick;
    }
    while(ms1 != ms2);

    return (ms1 * 1000U) + (us % 1000U);
}
void UART_RTU_Poll(void)
{
    uint16_t head = dma_head();
    uint32_t now = g_tick; // Mikrosaniye yerine milisaniye!

    switch(rtu_state)
    {
        case RTU_IDLE:
					  
				    if(rtu_frame_ready){
                break;}
            if(head != rx_ring.tail)
            {
                rtu_state = RTU_RECEIVING;
//							  rtu_frame_ready = 1;
                rtu_last_head = head;
                rtu_deadline = now + 3; // YORUMDAN ÇIKARILDI
            }
            break;

        case RTU_RECEIVING:
            if(head != rtu_last_head)
            {
                rtu_last_head = head;
                rtu_deadline = now + 3; // YORUMDAN ÇIKARILDI - Süreyi tazele
            }
            else
            {
                // Sessizlik süresi doldu mu?
                if((int32_t)(now - rtu_deadline) >= 0)
                {
                    rtu_frame_head = head;
                    rtu_frame_ready = 1; // Paketin bittigini haber ver
                    rtu_state = RTU_IDLE;
                }
            }
            break;
    }
}
/*============================ FRAME KONTROL FONKSIYONLARI =================*/

/**
 * @brief Yeni frame hazir mi kontrol et
 * @return 1: frame hazir, 0: frame yok
 */
uint8_t UART_FrameReady(void)
{
    return rtu_frame_ready;
}
/**
 * @brief Frame hazir flag'ini temizle
 */
void UART_ClearFrameFlag(void)
{
    rtu_frame_ready = 0;
}
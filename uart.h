#ifndef UART_H
#define UART_H

#include <stdint.h>

/*============================ MACROS ======================================*/
#define UART_RX_BUF_SIZE    256
#define UART_TX_BUF_SIZE    256
#define UART_CMD_QUEUE_SIZE 10   /* Komut kuyrugu boyutu */

/*============================ TYPEDEFS ====================================*/
typedef struct {
    uint8_t data[256];           /* Komut verisi */
    uint16_t len;                 /* Komut uzunlugu */
    uint32_t timestamp;           /* Zaman damgasi */
} cmd_item_t;

typedef struct {
    cmd_item_t cmds[UART_CMD_QUEUE_SIZE];  /* Komut depolama */
    volatile uint8_t head;                    /* Yazma indeksi */
    volatile uint8_t tail;                    /* Okuma indeksi */
    volatile uint8_t count;                    /* Kuyruktaki eleman sayisi */
} cmd_queue_t;

/*============================ FONKSIYON PROTOTIPLERI =====================*/
void UART_Init(uint32_t baud);
uint8_t UART_Send(const uint8_t *data, uint16_t len);
uint16_t UART_RxAvailable(void);
uint16_t UART_RxRead(uint8_t *dst, uint16_t len);
uint8_t UART_FrameReady(void);
void UART_ClearFrameFlag(void);
uint32_t GetTime_us(void);
uint16_t UART_GetFrame(uint8_t *dst, uint16_t max_len);
/* Komut kuyrugu fonksiyonlari */
uint8_t UART_QueueCommand(uint8_t *data, uint16_t len);
uint8_t UART_DequeueCommand(cmd_item_t *cmd);
uint8_t UART_GetQueueCount(void);
uint8_t UART_IsQueueFull(void);
uint8_t UART_IsQueueEmpty(void);
void UART_ClearQueue(void);
void UART_RTU_Poll(void);
/*============================ DIS DEGISKENLER ============================*/
extern volatile uint32_t g_tick;  /* delay.h'dan gelir */
extern volatile uint16_t modbus_t35_steps ; 
extern volatile uint16_t modbus_counter ;
extern volatile uint8_t  modbus_timer_running ;
extern volatile uint8_t  rtu_frame_ready ;
#endif /* UART_H */
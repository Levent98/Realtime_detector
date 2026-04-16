#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdint.h>

/*============================ MACROS ======================================*/
#define MODBUS_SLAVE_ID         0x01U
#define HOLDING_REG_SIZE         128U
#define FC_READ_HOLDING          0x03U
#define FC_READ_INPUT            0x04U
#define FC_WRITE_SINGLE_REG      0x06U
#define FC_WRITE_MULTIPLE_REGS   0x10U

/* Frame kuyrugu boyutu - ayni anda en fazla 10 frame saklanabilir */
#define FRAME_QUEUE_SIZE         10U

/* Hata kodlari */
#define MODBUS_ERR_ILLEGAL_FUNCTION     0x01U
#define MODBUS_ERR_ILLEGAL_DATA_ADDR    0x02U
#define MODBUS_ERR_ILLEGAL_DATA_VALUE   0x03U
#define MODBUS_ERR_SLAVE_DEVICE_FAILURE 0x04U
#define MODBUS_ERR_QUEUE_FULL            0x05U
///* ARMCC için __disable_irq ve __enable_irq tanimlari */
//#ifdef __ARMCC_VERSION
//    #define __disable_irq() __disable_irq()
//    #define __enable_irq()  __enable_irq()
//#else
//    /* CMSIS core_CM4.h zaten tanimliyor */
//    #include "cmsis_armcc.h"
//#endif
/*============================ TYPEDEFS ====================================*/
typedef struct {
    uint8_t data[256];           /* Frame verisi */
    uint16_t len;                 /* Frame uzunlugu */
    uint32_t timestamp;           /* Zaman damgasi (mikrosaniye) */
} frame_item_t;

typedef struct {
    frame_item_t frames[FRAME_QUEUE_SIZE];  /* Frame depolama alani */
    volatile uint8_t head;                    /* Yazma indeksi (producer) */
    volatile uint8_t tail;                    /* Okuma indeksi (consumer) */
    volatile uint8_t count;                    /* Kuyruktaki eleman sayisi */
    volatile uint32_t overflow_count;          /* Tasma sayaci (debug için) */
} frame_queue_t;
typedef enum
{
    /* ============================================================
     *  Table-A Detector Registers  (Dec 0 – 107 / 0x00 – 0x6B)
     * ============================================================ */

    /* 0x00 */ REG_TYPE_REG             = 0x00,  /* Device Model veya Gas Type                     */
    /* 0x01 */ REG_VALUE_REG            = 0x01,  /* Gas Concentration Value (see 0x06)             */
    /* 0x02 */ REG_STATUS_REG           = 0x02,  /* Fault Status (0=Fault, 1=Normal)               */
    /* 0x03 */ REG_MAX_VALUE            = 0x03,  /* Maximum Measurement Range (see 0x23)           */
    /* 0x04 */ REG_UNIT                 = 0x04,  /* Measurement Unit (see 0x22)                    */
    /* 0x05 */ REG_SENSOR_STATUS        = 0x05,  /* Device Status                                  */
    /* 0x06 */ REG_GAS_CONCT_VALUE      = 0x06,  /* Gas Concentration Value                        */
    /* 0x07 */ REG_GAS_CONCT_PERCENT    = 0x07,  /* Gas Concentration Percent                      */
    /* 0x08 */ REG_TWA_VALUE            = 0x08,  /* TWA Value - 15 min time weighted avg value     */
    /* 0x09 */ REG_STWA_VALUE           = 0x09,  /* STWA Value - 1 min time weighted avg value     */
    /* 0x0A */ REG_READ_ADC_VALUE       = 0x0A,  /* Read ADC Value                                 */
    /* 0x0B */ REG_TEMP_VALUE           = 0x0B,  /* Sensor Temperature                             */
    /* 0x0C */ REG_IN_VOL_VALUE         = 0x0C,  /* Input Voltage  (read / 10)                     */
    /* 0x0D */ REG_CURRENT_OUTPUT       = 0x0D,  /* Current Output (mA) (read / 100)               */
    /* 0x0E */ REG_CUR_LINE_RESISTOR    = 0x0E,  /* Current Line Resistor                          */
    /* 0x0F */ REG_DATE_YEAR_MON        = 0x0F,  /* Date - Year (last two digits) & Month          */
    /* 0x10 */ REG_DATE_DAY             = 0x10,  /* Date - Day                                     */
    /* 0x11 */ REG_TIME_HOUR_MIN        = 0x11,  /* Time - Hour and Minute                         */
    /* 0x12 */ REG_TIME_SEC             = 0x12,  /* Time - Second                                  */
    /* 0x13 */ REG_TEST_STATUS          = 0x13,  /* Self Test Register                             */
    /* 0x14 */ REG_INTERNAL_USE_0x14    = 0x14,  /* Internal Use                                   */
    /* 0x15 */ REG_UNUSED_0x15          = 0x15,  /* UNUSED REG - Span Set Time (= / 2)             */
    /* 0x16 */ REG_CALIBRATION_TIME     = 0x16,  /* Calibration Time (= / 2)                       */
    /* 0x17 */ REG_SENSOR_VOL_VALUE     = 0x17,  /* Sensor Voltage Level (read / 1000)             */
    /* 0x18 */ REG_LAMP_VOL_VALUE       = 0x18,  /* Lamp Voltage Level  (Vls / 1000)               */
    /* 0x19 */ REG_ALARM_FLAGS          = 0x19,  /* Fault Status Code                              */

    /* 0x1A */ REG_ALARM1_CONFIG        = 0x1A,  /* Alarm1 Config Reg - Off Delay                  */
    /* 0x1B */ REG_ALARM2_CONFIG        = 0x1B,  /* Alarm2 Config Reg - Off Delay                  */
    /* 0x1C */ REG_ALARM3_CONFIG        = 0x1C,  /* Alarm3 Config Reg - Off Delay                  */

    /* 0x1D */ REG_ANALOG_OUT_LEV1      = 0x1D,  /* Analog Out Level1 Fault State (read / 10)      */
    /* 0x1E */ REG_ANALOG_OUT_LEV2      = 0x1E,  /* Analog Out Level2 Calibration (read / 10)      */
    /* 0x1F */ REG_ANALOG_OUT_LEV3      = 0x1F,  /* Analog Out Level3 Overrange State              */
    /* 0x20 */ REG_SENSOR_TYPE          = 0x20,  /* Sensor Type Register                           */

    /* 0x21 */ REG_GAS_TYPE             = 0x21,  /* Gas Type (see Table-D)                         */
    /* 0x22 */ REG_MEAS_UNIT            = 0x22,  /* Measurement Unit (decimal)                     */
    /* 0x23 */ REG_GAS_CONCT_RANGE      = 0x23,  /* Maximum Measurement Range                      */
    /* 0x24 */ REG_GAS_LEL_VALUE        = 0x24,  /* Maximum Measurement Volume (read / 10)         */
    /* 0x25 */ REG_CAL_GAS_TYPE         = 0x25,  /* Relative Sensitivity                           */
    /* 0x26 */ REG_TEMP_HIGH_ALARM      = 0x26,  /* Alarm1 Level (read / ScalingFactor)            */
    /* 0x27 */ REG_TEMP_LOW_ALARM       = 0x27,  /* Alarm2 Level (read / ScalingFactor)            */
    /* 0x28 */ REG_HUM_HIGH_ALARM       = 0x28,  /* Alarm3 Level (read / ScalingFactor)            */
    /* 0x29 */ REG_SCALING_FACTOR       = 0x29,  /* Scaling Factor                                 */
    /* 0x2A */ REG_IR_PEAK_RD_DELAY     = 0x2A,  /* Ir Peak Read Delay                             */
    /* 0x2B */ REG_IR_DEEP_RD_DELAY     = 0x2B,  /* Ir Deep Read Delay                             */
    /* 0x2C */ REG_IR_LAMP_PERIOD       = 0x2C,  /* Ir Lamp Period                                 */
    /* 0x2D */ REG_FW_VERSION           = 0x2D,  /* Firmware version detail (xx.xx)                */
    /* 0x2E */ REG_FW_VERSION_2         = 0x2E,  /* Firmware version detail (xx.xx.read)           */
    /* 0x2F */ REG_FW_DATE              = 0x2F,  /* Firmware date (Year)                           */
    /* 0x30 */ REG_FW_DATE_2            = 0x30,  /* Firmware date (Month)                          */
    /* 0x31 */ REG_DISPLAY_LANGUAGE     = 0x31,  /* Display Language (decimal)                     */
    /* 0x32 */ REG_BAUD_RATE            = 0x32,  /* Baud Rate                                      */
    /* 0x33 */ REG_DET_MODEL            = 0x33,  /* Device Model                                   */

    /* 0x34 */ REG_MAN_ID_NUMB1         = 0x34,  /* Manufacturing ID char[1,2]                     */
    /* 0x35 */ REG_MAN_ID_NUMB2         = 0x35,  /* Manufacturing ID char[3,4]                     */
    /* 0x36 */ REG_MAN_ID_NUMB3         = 0x36,  /* Manufacturing ID char[5,6]                     */
    /* 0x37 */ REG_MAN_ID_NUMB4         = 0x37,  /* Manufacturing ID char[7,8]                     */

    /* 0x38 */ REG_BASE_SERIAL_NO       = 0x38,  /* Base Serial No char[1,2]                       */
    /* 0x39 */ REG_BASE_SER_NUMB2       = 0x39,  /* Base Serial No char[3,4]                       */
    /* 0x3A */ REG_BASE_SER_NUMB3       = 0x3A,  /* Base Serial No char[5,6]                       */
    /* 0x3B */ REG_BASE_SER_NUMB4       = 0x3B,  /* Base Serial No char[7,8]                       */

    /* 0x3C */ REG_SERIAL_NO            = 0x3C,  /* Serial Number char[1,2]                        */
    /* 0x3D */ REG_SERIAL_NUMB2         = 0x3D,  /* Serial Number char[3,4]                        */
    /* 0x3E */ REG_SERIAL_NUMB3         = 0x3E,  /* Serial Number char[5,6]                        */
    /* 0x3F */ REG_SERIAL_NUMB4         = 0x3F,  /* Serial Number char[7,8]                        */

    /* 0x40 */ REG_LOC_STRING_START     = 0x40,  /* Location Text - Start (0x40'tan itibaren)      */
    /* 0x4C */ REG_LOC_STRING_END       = 0x4C,  /* Location Text - End                            */

    /* 0x4D */ REG_BASE_CONFIG          = 0x4D,  /* Base Configuration Details                     */
    /* 0x4E */ REG_TEST_DATE_YEAR_MON   = 0x4E,  /* Test Date (Year/Month)                         */
    /* 0x4F */ REG_TEST_DATE_DAY_CYCLE  = 0x4F,  /* Test Date (Day)                                */

    /* 0x50 */ REG_ZERO_VAL             = 0x50,  /* Zero Value                                     */
    /* 0x51 */ REG_SPAN_VAL             = 0x51,  /* Span Value                                     */
    /* 0x52 */ REG_SPAN_TEMP            = 0x52,  /* Span Temperature (read / 10)                   */
    /* 0x53 */ REG_CALB_CONC_VAL        = 0x53,  /* Calibration Gas Concentration Level            */
    /* 0x54 */ REG_CALB_DATE_YEAR_MON   = 0x54,  /* Calibration Date (Year/Month)                  */
    /* 0x55 */ REG_CALB_DATE_DAY        = 0x55,  /* Calibration Date (Day of year)                 */
    /* 0x56 */ REG_CALB_CYCLE           = 0x56,  /* Calibration Cycle period moon                  */
    /* 0x57 */ REG_SECOND_SPAN_VAL      = 0x57,  /* Second Span Value                              */
      	       IR_THERMISTOR_RES_REG	= 0x58,
	             IR_THERMISTOR_BETA_REG	= 0x59,
	             IR_CO_A_HI_REG			= 0x5A,
	             IR_CO_A_LOW_REG			= 0x5B,
	             IR_CO_N_HI_REG			= 0x5C,
	             IR_CO_N_LOW_REG			= 0x5D,
	             IR_CO_AL_POS_HI_REG		= 0x5E,
	             IR_CO_AL_POS_LOW_REG	= 0x5F,
	             IR_CO_AL_NEG_HI_REG		= 0x60,
	             IR_CO_AL_NEG_LOW_REG	= 0x61,
	             IR_CO_BET_POS_HI_REG	= 0x62,
	             IR_CO_BET_POS_LOW_REG	= 0x63,
	             IR_CO_BET_NEG_HI_REG	= 0x64,
	             IR_CO_BET_NEG_LOW_REG	= 0x65,
	/*************************************************************************/
	             SENSOR_MAN_DATE_REG		= 0x66,
	             XTRM_MAN_DATE_REG		= 0x67,
    /* 0x58–0x67 tabloda tanimsiz, register adresleri atlanmis */

    /* 0x68 */ REG_ALRM_HYSTERESIS_1    = 0x68,  /* A1 / A2 Hysteresis                             */
    /* 0x69 */ REG_ALRM_HYSTERESIS_2    = 0x69,  /* A3 Hysteresis                                  */
    /* 0x6A */ REG_PID_GAS_INDEX        = 0x6A,  /* PID Gas Index (register = read + 1)            */
    /* 0x6B */ REG_SEN_SUPPLY_VOLTAGE   = 0x6B,  /* Sensor Voltage (Vf - read / 10)                */
    	/*************************************************************************/
	             WARM_UP_TIME_REG		= 0x6C,
	/*************************************************************************/
	             PASWORD_HIGH_REG		= 0x6D,
	             PASWORD_LOW_REG			= 0x6E,
	             HART_CONFIG_REG			= 0x6F,
	             OPERATING_TEMP_REG		= 0x70,
	             CALL_SPAN_TIME_REG		= 0x71,
	             CALL_ZERO_TIME_REG		= 0x72,
	             IR_READ_DELAY_REG		= 0x73,
    /* ============================================================
     *  Tabloda karsiligi olmayan kullanici register'lari (RESERVED)
     * ============================================================ */
    REG_HUM_VALUE         = 0x2000,  /* Humidity - tabloda karsiligi yok           */
    REG_HUM_LOW_ALARM     = 0x2001,  /* Humidity Low Alarm - tabloda karsiligi yok */
    REG_RESERVED_1        = 0x2002,
    REG_RESERVED_2        = 0x2003,
    REG_RESERVED_3        = 0x2004,
    REG_RESERVED_4        = 0x2005,
    REG_RESERVED_5        = 0x2006,
    REG_RESERVED_6        = 0x2007,
    REG_RESERVED_7        = 0x2008,
    REG_RESERVED_8        = 0x2009,

} Reg_Table;

enum eHoldingRegisterId
{
	PASSWORD_REG	= 0x00,
	COMMAND_CODE	= 0x01,
	COMMAND_ARG1	= 0x02,
	COMMAND_ARG2	= 0x03,
	COMMAND_ARG3	= 0x04,
	COMMAND_ARG4	= 0x05,
	COMMAND_ARG5	= 0x06,
	COMMAND_ARG6	= 0x07,
	LOG_BUFF_COPY	= 0x10,
};

enum eModbusCmd{
	DISPLAY_MODULE_DISABLE	= 0x9153,
	SET_IR_ADC_SAMPLER	= 0x5338,
	SET_CONCETRATION_VALUE	= 0x5583,
	SET_SENSOR_ZERO_DSP	= 0x6675,
	SET_SENSOR_ZERO		= 0x7566,
	SET_SENSOR_SPAN		= 0x4927,
	SET_SENSOR_SPAN_DSP	= 0x4827,
	CANCEL_CALIBRATION	= 0x4257,
	SET_CALIBRATION_GAS	= 0x4368,
	SET_CLBRATION_CYCLE	= 0x4715,
	SET_SENSOR_DATA		= 0x7327,
	SET_TYPE_WR_PRTCT	= 0x6216,
	SET_DEVICE_ID		= 0x1298,
	SET_ALARM_CONFIG	= 0x153A,
	SET_PASWORD			= 0x2A83,
	SET_TIME_DATE		= 0x3607,
	SET_LANGUAGE		= 0x6370,
	READ_LOG_BUFFER		= 0x5191,
	RESET_DETECTOR		= 0x3482,
	SET_OPERATIONAL_MODE= 0x1073,
	RUN_DEVICE_TEST		= 0x2317,
	SET_TEST_INTERVAL	= 0x0945,
	SET_TEST_INHIBIT	= 0x1892,
	SET_ZERO_SUPPR		= 0x6022,
	SET_DSP_MODEL		= 0x6977,
	SET_CURRENT_LEVELS	= 0x5962,
	SET_DEVICE_LABEL	= 0x3126,
	SET_DETECTOR_DATA	= 0x7372,
};
//enum eGasType
//{
//	GAS_INDX_OFFSET = 30,  // LPG
//	LPG	=	30,
//	CH4	=	31,
//	C3H8 = 34, 			//PROPANE
//	C5H12 = 37, 		//N-pentane
//	C2H2 = 60, 			// ASETILEN
//	O2 = 65,
//	CO2 = 71,
//	FORMALDEHYDE = 74,
//	ACETALDEHYDE = 78,
//	HYDROGEN_CHLORINE = 79,
//	TVOC = 80,
//	VOC = 81,
//	OZONE = 82,
//	HF = 83,
//	PHOSPHINE = 84,
//	ISOBUTYLENE = 85,
//	A2L			= 91,
//	LAST_GAS_INDX = 255,
//};
/*============================ FONKSIYON PROTOTIPLERI =====================*/
void Modbus_Init(void);
void Modbus_Task(void);
void Modbus_SyncAll(void);
const frame_item_t *Modbus_PeekFrame(void);
void Modbus_PopFrame(void);
uint8_t Modbus_QueueFrame(uint8_t *data, uint16_t len);
uint8_t Modbus_DequeueFrame(frame_item_t *frame);
uint8_t Modbus_ProcessFrame(const frame_item_t *frame);
void Modbus_SendErrorResponse(uint8_t function, uint8_t error_code);
uint8_t Modbus_Is_Writable(uint16_t addr);
uint16_t Modbus_Internal_Read(uint16_t addr);
uint16_t Modbus_ReadRegister(uint16_t addr);
/* Queue durum fonksiyonlari */
uint8_t Modbus_GetQueueCount(void);
uint8_t Modbus_IsQueueFull(void);
uint8_t Modbus_IsQueueEmpty(void);
void Modbus_ClearQueue(void);
uint32_t Modbus_GetOverflowCount(void);
void Modbus_SetInputRegister(uint16_t reg, uint16_t value);
/* Register erisim fonksiyonlari */
void Modbus_SetRegister(uint16_t reg, uint16_t value);
uint16_t Modbus_GetRegister(uint16_t reg);
void Modbus_UpdateRegisters(uint16_t start_reg, uint16_t *data, uint16_t count);

#endif /* MODBUS_RTU_H */
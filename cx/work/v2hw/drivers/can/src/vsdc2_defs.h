#ifndef __VSDC2_DEFS_H
#define __VSDC2_DEFS_H


/* Map of registers */
enum
{
    /* General-purpose registers */
    VSDC2_R_ID               = 0x00,
    VSDC2_R_GCSR             = 0x01,
    VSDC2_R_CALIB_OFFSET_NUM = 0x02,
    VSDC2_R_UART0_BAUDRATE   = 0x03,
    VSDC2_R_UART0_DATA       = 0x04,
    VSDC2_R_UART0_NUM        = 0x05,
    VSDC2_R_UART1_BAUDRATE   = 0x06,
    VSDC2_R_UART1_DATA       = 0x07,
    VSDC2_R_UART1_NUM        = 0x08,
    VSDC2_R_CAN_CSR          = 0x09,
    VSDC2_R_GPIO             = 0x0A,
    VSDC2_R_IRQ_MASK         = 0x0B,
    VSDC2_R_IRQ_STATUS       = 0x0C,
    VSDC2_R_ADC0_CSR         = 0x0D,
    VSDC2_R_ADC0_TIMER       = 0x0E,
    VSDC2_R_ADC0_AVG_NUM     = 0x0F,
    VSDC2_R_ADC0_WRITE_ADDR  = 0x10,
    VSDC2_R_ADC0_READ_ADDR   = 0x11,
    VSDC2_R_ADC0_DATA        = 0x12,
    VSDC2_R_ADC0_INTEGRAL    = 0x13,
    VSDC2_R_ADC0_INTEGRAL_C  = 0x14,
    VSDC2_R_ADC1_CSR         = 0x15,
    VSDC2_R_ADC1_TIMER       = 0x16,
    VSDC2_R_ADC1_AVG_NUM     = 0x17,
    VSDC2_R_ADC1_WRITE_ADDR  = 0x18,
    VSDC2_R_ADC1_READ_ADDR   = 0x19,
    VSDC2_R_ADC1_DATA        = 0x1A,
    VSDC2_R_ADC1_INTEGRAL    = 0x1B,
    VSDC2_R_ADC1_INTEGRAL_C  = 0x1C,
    VSDC2_R_ADC0_BP_MUX      = 0x20,
    VSDC2_R_ADC1_BP_MUX      = 0x28,

    /* Non-general-purpose registers */
    VSDC2_R_REF_VOLTAGE_LOW  = 0x30,
    VSDC2_R_REF_VOLTAGE_HIGH = 0x31,
    VSDC2_R_BF_TIMER0        = 0x32,
    VSDC2_R_BF_TIMER1        = 0x33,
    VSDC2_R_ADC0_OFFSET_AMP  = 0x34,
    VSDC2_R_ADC1_OFFSET_AMP  = 0x35,
    VSDC2_R_ADC0_OFFSET_ADC  = 0x36,
    VSDC2_R_ADC1_OFFSET_ADC  = 0x37,
};

/* Register-specific bits */
enum
{
    VSDC2_GCSR_GPIO           = 1 << 0,
    VSDC2_GCSR_UART0          = 1 << 1,
    VSDC2_GCSR_UART1          = 1 << 2,
    VSDC2_GCSR_CAN            = 1 << 3,
    VSDC2_GCSR_MUX_MASK       = 3 << 4,
        VSDC2_GCSR_MUX_U_INP  = 0 << 4,
        VSDC2_GCSR_MUX_U_GND  = 1 << 4,
        VSDC2_GCSR_MUX_U_1851 = 2 << 4,
        VSDC2_GCSR_MUX_U_168  = 3 << 4,
    VSDC2_GCSR_CALIB          = 1 << 6,
    VSDC2_GCSR_AUTOCAL        = 1 << 7,
    
    VSDC2_GCSR_PSTART_ALL     = 1 << 14,
    VSDC2_GCSR_PSTOP_ALL      = 1 << 15,
};

enum
{
    VSDC2_ADCn_CSR_PSTART              = 1 << 0,
    VSDC2_ADCn_CSR_PSTOP               = 1 << 1,
    VSDC2_ADCn_CSR_START_MASK          = 7 << 2,
        VSDC2_ADCn_CSR_START_PSTART    = 0 << 2,
        VSDC2_ADCn_CSR_START_SYNC_A    = 5 << 2,
        VSDC2_ADCn_CSR_START_SYNC_B    = 6 << 2,
        VSDC2_ADCn_CSR_START_BACKPLANE = 7 << 2,
    VSDC2_ADCn_CSR_STOP_MASK           = 7 << 5,
        VSDC2_ADCn_CSR_STOP_PSTOP      = 0 << 5,
        VSDC2_ADCn_CSR_STOP_TIMER      = 1 << 5,
        VSDC2_ADCn_CSR_STOP_SYNC_A     = 5 << 5,
        VSDC2_ADCn_CSR_STOP_SYNC_B     = 6 << 5,
        VSDC2_ADCn_CSR_STOP_BACKPLANE  = 7 << 5,
    VSDC2_ADCn_CSR_GAIN_MASK           = 7 << 8,
        VSDC2_ADCn_CSR_GAIN_ERROR      = 0 << 8,
        VSDC2_ADCn_CSR_GAIN_02V        = 1 << 8,
        VSDC2_ADCn_CSR_GAIN_05V        = 2 << 8,
        VSDC2_ADCn_CSR_GAIN_1V         = 3 << 8,
        VSDC2_ADCn_CSR_GAIN_2V         = 4 << 8,
        VSDC2_ADCn_CSR_GAIN_5V         = 5 << 8,
        VSDC2_ADCn_CSR_GAIN_10V        = 6 << 8,
        VSDC2_ADCn_CSR_GAIN_MISSING7   = 7 << 8,
    VSDC2_ADCn_CSR_OVRNG               = 1 << 11,
    VSDC2_ADCn_CSR_STATUS_MASK         = 3 << 12,
        VSDC2_ADCn_CSR_STATUS_STOP     = 0 << 12,
        VSDC2_ADCn_CSR_STATUS_MISSING1 = 1 << 12,
        VSDC2_ADCn_CSR_STATUS_INTE     = 2 << 12,
        VSDC2_ADCn_CSR_STATUS_POSTINT  = 3 << 12,
    VSDC2_ADCn_CSR_MOVF                = 1 << 14,
    VSDC2_ADCn_CSR_SINGLE              = 1 << 15,
    
    VSDC2_ADCn_CSR_IRQ_M_START         = 1 << 16,
    VSDC2_ADCn_CSR_IRQ_M_STOP          = 1 << 17,
    VSDC2_ADCn_CSR_IRQ_M_INTSTOP       = 1 << 18,
    VSDC2_ADCn_CSR_IRQ_M_OVRNG         = 1 << 19,
    VSDC2_ADCn_CSR_IRQ_M_MOVF          = 1 << 20,

    VSDC2_ADCn_CSR_IRQ_C_START         = 1 << 24,
    VSDC2_ADCn_CSR_IRQ_C_STOP          = 1 << 25,
    VSDC2_ADCn_CSR_IRQ_C_INTSTOP       = 1 << 26,
    VSDC2_ADCn_CSR_IRQ_C_OVRNG         = 1 << 27,
    VSDC2_ADCn_CSR_IRQ_C_MOVF          = 1 << 28,
};

enum
{
    VSDC2_ADCn_BP_MUX_START0 = 0,
    VSDC2_ADCn_BP_MUX_START1 = 1,
    VSDC2_ADCn_BP_MUX_STOP0  = 2,
    VSDC2_ADCn_BP_MUX_STOP1  = 3,
};

#endif /* __VSDC2_DEFS_H */

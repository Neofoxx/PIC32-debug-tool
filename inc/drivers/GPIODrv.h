#ifndef GPIODRV_H_573f2ca8b43e44209acc90f0f9b7de5d
#define GPIODRV_H_573f2ca8b43e44209acc90f0f9b7de5d

#include <inttypes.h>

#define GPIO_mode_output_low	0
#define GPIO_mode_output_high	1
#define GPIO_mode_input			2

#define GPIO_LOW				0
#define GPIO_HIGH				1

////////
// MX270
////////
#if defined(__32MX270F256D__)
// LED
#define LEDUSER_TRISbits		TRISCbits
#define LEDUSER_TRISPIN			TRISC4
#define LEDUSER_LATbits			LATCbits
#define LEDUSER_LATPIN			LATC4
#define LEDUSER_LATINV			LATCINV
#define LEDUSER_MASK			(1<<4)

// Button
#define BTNUSER_TRISbits		TRISCbits
#define BTNUSER_TRISPIN			TRISC5
#define BTNUSER_PORTbits		PORTCbits
#define BTNUSER_PORTPIN			RC5
#define BTNUSER_PULLREG			CNPUC	// Call also pull down with CNPDx
#define BTNUSER_PULLBIT			(1<<5)

// UART
#define UART_TX_TRISbits		TRISCbits
#define UART_TX_TRISPIN			TRISC8
#define UART_TX_LATbits			LATCbits
#define UART_TX_LATPIN			LATC8
#define UART_TX_RP_REG			RPC9R
#define UART_TX_RP_VAL			0b0010	// U2TX

#define UART_RX_TRISbits		TRISCbits
#define UART_RX_TRISPIN			TRISC9
#define UART_RX_PULLREG			CNPUC	// UARTs are idle high, so pull-up
#define UART_RX_PULLBIT			(1<<9)
#define UART_RX_REMAP_VAL		0b0110

#define UART_MODE_bits			U2MODEbits
#define UART_STA_bits			U2STAbits
#define UART_BRG_reg			U2BRG
#define UART_TX_reg				U2TXREG
#define UART_RX_reg				U2RXREG

#define UART_INT_IPC_bits			IPC9bits
#define UART_INT_IPC_PRIORITY		U2IP
#define UART_INT_IPC_SUBPRIORITY	U2IS
#define UART_INT_IEC_bits			IEC1bits
#define UART_INT_IEC_RXIE			U2RXIE
#define UART_INT_IFS_bits			IFS1bits
#define UART_INT_IFS_RXIF				U2RXIF


////////
// MX440
////////
#elif defined(__32MX440F256H__)
// LED
#define LEDUSER_TRISbits		TRISDbits
#define LEDUSER_TRISPIN			TRISD1
#define LEDUSER_LATbits			LATDbits
#define LEDUSER_LATPIN			LATD1
#define LEDUSER_LATINV			LATDINV
#define LEDUSER_MASK			(1<<1)

// LED USB_INTERRUPT - CON 1-15
#define LEDUSBINT_TRISbits		TRISDbits
#define LEDUSBINT_TRISPIN		TRISD7
#define LEDUSBINT_LATbits		LATDbits
#define LEDUSBINT_LATPIN		LATD7
#define LEDUSBINT_LATINV		LATDINV
#define LEDUSBINT_MASK			(1<<7)

// LED USB_EP_UART_OUT (PC to us) - CON 1-14
#define LEDUSBUART_OUT_TRISbits		TRISDbits
#define LEDUSBUART_OUT_TRISPIN		TRISD8
#define LEDUSBUART_OUT_LATbits		LATDbits
#define LEDUSBUART_OUT_LATPIN		LATD8
#define LEDUSBUART_OUT_LATINV		LATDINV
#define LEDUSBUART_OUT_MASK			(1<<8)

// LED USB_EP_UART_IN (us to PC) - CON 1-13
#define LEDUSBUART_IN_TRISbits		TRISDbits
#define LEDUSBUART_IN_TRISPIN		TRISD11
#define LEDUSBUART_IN_LATbits		LATDbits
#define LEDUSBUART_IN_LATPIN		LATD11
#define LEDUSBUART_IN_LATINV		LATDINV
#define LEDUSBUART_IN_MASK			(1<<11)

// LED USB_EP_PROG_OUT (PC to us) - CON 1-11
#define LEDUSBPROG_OUT_TRISbits		TRISFbits
#define LEDUSBPROG_OUT_TRISPIN		TRISF1
#define LEDUSBPROG_OUT_LATbits		LATFbits
#define LEDUSBPROG_OUT_LATPIN		LATF1
#define LEDUSBPROG_OUT_LATINV		LATFINV
#define LEDUSBPROG_OUT_MASK			(1<<1)

// LED USB_EP_PROG_IN (us to PC) - CON 1-10
#define LEDUSBPROG_IN_TRISbits		TRISEbits
#define LEDUSBPROG_IN_TRISPIN		TRISE7
#define LEDUSBPROG_IN_LATbits		LATEbits
#define LEDUSBPROG_IN_LATPIN		LATE7
#define LEDUSBPROG_IN_LATINV		LATEINV
#define LEDUSBPROG_IN_MASK			(1<<7)


// Button
#define BTNUSER_TRISbits		TRISDbits
#define BTNUSER_TRISPIN			TRISD0
#define BTNUSER_PORTbits		PORTDbits
#define BTNUSER_PORTPIN			RD0
// No pull-up/down available on this pin

// UART
#define UART_TX_TRISbits		TRISDbits
#define UART_TX_TRISPIN			TRISD3
#define UART_TX_LATbits			LATDbits
#define UART_TX_LATPIN			LATD3
// No remapping available

#define UART_RX_TRISbits		TRISDbits
#define UART_RX_TRISPIN			TRISD2

#define UART_MODE_bits			U1MODEbits
#define UART_STA_bits			U1STAbits
#define UART_BRG_reg			U1BRG
#define UART_TX_reg				U1TXREG
#define UART_RX_reg				U1RXREG

#define UART_INT_IPC_bits			IPC6bits
#define UART_INT_IPC_PRIORITY		U1IP
#define UART_INT_IPC_SUBPRIORITY	U1IS
#define UART_INT_IEC_bits			IEC0bits
#define UART_INT_IEC_RXIE			U1RXIE
#define UART_INT_IFS_bits			IFS0bits
#define UART_INT_IFS_RXIF			U1RXIF

// Pin definitions for programming
// TMS bit on RE0
#define PROG_TMS_TRISbits		TRISEbits
#define PROG_TMS_TRISPIN		TRISE0
#define PROG_TMS_PORTbits		PORTEbits
#define PROG_TMS_PORTPIN		RE0
#define PROG_TMS_LATbits		LATEbits
#define PROG_TMS_LATPIN			LATE0
#define PROG_TMS_LATINV			LATEINV
#define PROG_TMS_LATSET			LATESET
#define PROG_TMS_LATCLR			LATECLR
#define PROG_TMS_MASK			(1<<0)

// TCK bit on RE1
#define PROG_TCK_TRISbits		TRISEbits
#define PROG_TCK_TRISPIN		TRISE1
#define PROG_TCK_PORTbits		PORTEbits
#define PROG_TCK_PORTPIN		RE1
#define PROG_TCK_LATbits		LATEbits
#define PROG_TCK_LATPIN			LATE1
#define PROG_TCK_LATINV			LATEINV
#define PROG_TCK_LATSET			LATESET
#define PROG_TCK_LATCLR			LATECLR
#define PROG_TCK_MASK			(1<<1)

// TDO bit on RE2
#define PROG_TDO_TRISbits		TRISEbits
#define PROG_TDO_TRISPIN		TRISE2
#define PROG_TDO_PORTbits		PORTEbits
#define PROG_TDO_PORTPIN		RE2
#define PROG_TDO_LATbits		LATEbits
#define PROG_TDO_LATPIN			LATE2
#define PROG_TDO_LATINV			LATEINV
#define PROG_TDO_LATSET			LATESET
#define PROG_TDO_LATCLR			LATECLR
#define PROG_TDO_MASK			(1<<2)

// TDI bit on RE3
#define PROG_TDI_TRISbits		TRISEbits
#define PROG_TDI_TRISPIN		TRISE3
#define PROG_TDI_PORTbits		PORTEbits
#define PROG_TDI_PORTPIN		RE3
#define PROG_TDI_LATbits		LATEbits
#define PROG_TDI_LATPIN			LATE3
#define PROG_TDI_LATINV			LATEINV
#define PROG_TDI_LATSET			LATESET
#define PROG_TDI_LATCLR			LATECLR
#define PROG_TDI_MASK			(1<<3)

// MCLR bit on RE4
#define PROG_MCLR_TRISbits		TRISEbits
#define PROG_MCLR_TRISPIN		TRISE4
#define PROG_MCLR_PORTbits		PORTEbits
#define PROG_MCLR_PORTPIN		RE4
#define PROG_MCLR_LATbits		LATEbits
#define PROG_MCLR_LATPIN		LATE4
#define PROG_MCLR_LATINV		LATEINV
#define PROG_MCLR_LATSET		LATESET
#define PROG_MCLR_LATCLR		LATECLR
#define PROG_MCLR_MASK			(1<<4)

#endif

// Function definitons
void GPIODrv_init();

void GPIODrv_setupPinTMS(uint32_t mode);
void GPIODrv_setupPinTCK(uint32_t mode);
void GPIODrv_setupPinTDI(uint32_t mode);
void GPIODrv_setupPinTDO(uint32_t mode);
void GPIODrv_setupPinMCLR(uint32_t mode);

void GPIODrv_setStateTMS(uint32_t state);
// Optimize later. Static inline works just fine (and too fast :p)
void GPIODrv_setStateTCK(uint32_t state);
/*
static inline void GPIODrv_setStateTCK(uint32_t state){
	if (state == GPIO_HIGH){
		PROG_TCK_LATSET = PROG_TCK_MASK;
	}
	else{
		PROG_TCK_LATCLR = PROG_TCK_MASK;
	}
}
*/
void GPIODrv_setStateTDI(uint32_t state);
void GPIODrv_setStateTDO(uint32_t state);
void GPIODrv_setStateMCLR(uint32_t state);

uint32_t GPIODrv_getStateTMS();
uint32_t GPIODrv_getStateTCK();
uint32_t GPIODrv_getStateTDI();
uint32_t GPIODrv_getStateTDO();
uint32_t GPIODrv_getStateMCLR();



#endif

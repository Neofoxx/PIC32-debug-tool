#include <p32xxxx.h>
#include <UARTDrv.h>
#include <GPIODrv.h>
#include <system.h>
#include <inttypes.h>
#include <interrupt.h>
#include <LED.h>

// Circular buffer for receiving elements
volatile uint8_t head = 0;
volatile uint8_t tail = 0;
volatile uint8_t receiveArray[256];
uint8_t temp;

#if defined(__32MX270F256D__)
INTERRUPT(UART2Interrupt){
#elif defined(__32MX440F256H__)
INTERRUPT(UART1Interrupt){
#endif

	// Should check if TX or RX interrupt
	if ((uint8_t)(head+1) == tail){
		// Buffer full
		temp = UART_RX_reg;	// Readout data, otherwise we'll be stuck here
	}
	else{
		// If we have space, save into buffer
		receiveArray[head++] = UART_RX_reg;
	}

	LED_toggle();

	UART_INT_IFS_bits.UART_INT_IFS_RXIF = 0;
}


void UARTDrv_Init(uint32_t baud){
	UART_MODE_bits.ON = 0;

	UART_TX_TRISbits.UART_TX_TRISPIN = 0;	// 0 == output
	UART_TX_LATbits.UART_TX_LATPIN = 1;		// Set high, as UART is Idle High
	#ifdef UART_TX_RP_REG
		UART_TX_RP_REG = UART_TX_RP_VAL;		// Remap to proper pin
	#endif

	UART_RX_TRISbits.UART_TX_TRISPIN = 1;						// 1 == input
	#ifdef UART_RX_PULLREG
		UART_RX_PULLREG = UART_RX_PULLREG | UART_RX_PULLBIT;	// Enable pull-up
	#endif

	#ifdef UART_RX_REMAP_VAL
		U2RXR = UART_RX_REMAP_VAL;									// Set to which pin
	#endif

	UART_MODE_bits.SIDL = 0;	// Stop when in IDLE mode
	UART_MODE_bits.IREN	= 0;	// Disable IrDA
	UART_MODE_bits.RTSMD = 0;	// Don't care, RTS not used
	UART_MODE_bits.UEN = 0;		// TX & RX controlled by UART peripheral, RTC & CTS are not.
	UART_MODE_bits.WAKE = 0;	// Don't wake up from sleep
	UART_MODE_bits.LPBACK = 0;	// Loopback mode disabled
	UART_MODE_bits.ABAUD = 0;	// No autobauding
	UART_MODE_bits.RXINV = 0;	// Idle HIGH
	UART_MODE_bits.BRGH = 0;	// Standard speed mode - 16x baud clock
	UART_MODE_bits.PDSEL = 0;	// 8 bits, no parity
	UART_MODE_bits.STSEL = 0;	// 1 stop bit

	UART_STA_bits.ADM_EN = 0;	// Don't care for auto address detection, unused
	UART_STA_bits.ADDR = 0;		// Don't care for auto address mark
	UART_STA_bits.UTXISEL = 00;	// Generate interrupt, when at least one space available (unused)
	UART_STA_bits.UTXINV = 0;	// Idle HIGH
	UART_STA_bits.URXEN = 1;	// UART receiver pin enabled
	UART_STA_bits.UTXBRK = 0;	// Don't send breaks.
	UART_STA_bits.UTXEN = 1;	// Uart transmitter pin enabled
	UART_STA_bits.URXISEL = 0;	// Interrupt what receiver buffer not empty
	UART_STA_bits.ADDEN = 0;	// Address detect mode disabled (unused)
	UART_STA_bits.OERR = 0;		// Clear RX Overrun bit - not important at this point

	// (PBCLK/BRGH?4:16)/BAUD - 1
	UART_BRG_reg = (GetPeripheralClock() / (U2MODEbits.BRGH ? 4 : 16)) / baud - 1;

	// Setup interrupt - Split into new function fer easier ifdef-ing?
	UART_INT_IPC_bits.UART_INT_IPC_PRIORITY		= 1;	// Priority = 1
	UART_INT_IPC_bits.UART_INT_IPC_SUBPRIORITY 	= 0;	// Subpriority = 0;
	UART_INT_IEC_bits.UART_INT_IEC_RXIE			= 1;	// Enable interrupt.

	UART_MODE_bits.ON = 1;
}

void UARTDrv_SendBlocking(uint8_t * buffer, uint32_t length){

	uint32_t counter = 0;

	for (counter = 0; counter<length; counter++){
		while(UART_STA_bits.UTXBF){ asm("nop"); }
		UART_TX_reg = buffer[counter];
		asm("nop");
	}

	// Wait until sent
	// Fucking errata.
	/*
	while(UART_STA_bits.TRMT){
		_nop();
		asm("nop");
	}
	*/
}

uint32_t UARTDrv_GetCount(){
	uint32_t tempHead = head;
	uint32_t tempTail = tail;
	if (tempHead == tempTail){
		return 0;
	}
	else if (tempHead > tempTail){
		return (tempHead-tempTail);
	}
	else{
		return (sizeof(receiveArray) - tempTail + tempHead);
	}

}

uint32_t UARTDrv_GetReceiveData(uint8_t *copyTo, uint8_t maxSize){
	// Copy a max of maxSize into copyTo array.
	// Return number of Bytes received from the array
	uint8_t tempHead = head;
	uint8_t tempTail = tail;
	uint8_t counter = 0;
	for (counter = 0; counter<maxSize && tempTail != tempHead; ){
		copyTo[counter] = receiveArray[tempTail];

		counter++;
		tempTail++;	// Relying on uint8_t overflow
	}
	tail = tempTail;
	return counter;
}


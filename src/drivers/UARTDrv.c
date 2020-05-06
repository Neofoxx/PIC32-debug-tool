#include <p32xxxx.h>
#include <UARTDrv.h>
#include <GPIODrv.h>
#include <system.h>
#include <inttypes.h>
#include <interrupt.h>
#include <LED.h>
#include <COMMS.h>
#include <kmem.h>
#include <usb.h>		// If usb.h isn't present, usb_cdc.h goes ballistic with errors.
#include <usb_cdc.h>

uint32_t sizeToSendTx = 0;	// Size to increment the circular buffer after transfer is done
#if defined (__32MX440F256H__)
#define DMA_MAX_SIZE 256
#else
_Static_assert(0, "Please add the proper define for DMA");
#endif



#if defined(__32MX270F256D__)
INTERRUPT(UART2Interrupt){
#elif defined(__32MX440F256H__)
INTERRUPT(UART1Interrupt){
#endif

	// Just an RX interrupt here.
	// This is fine for slower speeds like 115200.

	// Get data and save to buffer
	uint8_t temp = UART_RX_reg;
	COMMS_helper_addToBuf(&uartRXstruct, &temp, 1);

	// Clear RX interrupt
	UART_INT_IFS_bits.UART_INT_IFS_RXIF = 0;
}

// DMA 0 for TX-ing, DMA1 for RXint (when added)
INTERRUPT(DMA0Interrupt){

	// Currently, when finished, just update the buffer position
	// and clear interrupt flag
	uartTXstruct.tail = (uartTXstruct.tail + sizeToSendTx) & cyclicBufferSizeMask;

	DCH0INTCLR = _DCH0INT_CHBCIF_MASK;	// Clear the DMA channel interrupt flag (Block Transfer Done)
	IFS1bits.DMA0IF = 0;				// Clear the DMA0 interrup flag ><

}


void UARTDrv_InitDMA(){
	// Setup parts of DMA, like priorities etc.

	DCH0INTbits.CHBCIE = 1;	// Enable Block Transfer Done

#if defined(__32MX440F256H__)
	IPC9bits.DMA0IP = 5;	// Priority 5 (higher than USB, so we update the size at the right time)
	IPC9bits.DMA0IS = 0;	// Subpriority 0
#else
	_Static_assert(0, "Not implemented yet.");
#endif

	//DCH0CON = 0;				// Clear everything.
	DCH0CONCLR = 0xFFFFFFFF;
	// No CHBUSY bit, so can't check for end of transaction.
	//DCH0CONbits.CHPRI = 0x03;	// Higher priority
	//DCH0CONbits.CHAEN = 0;		// Auto Disable on block transfer
	//DCH0CONbits.CHCHN = 0;		// No chaining
	DCH0CONSET = 0x3 << _DCH0CON_CHPRI_POSITION;

	//IEC1bits.DMA0IE = 1;		// Enable DMA0 interrupt
	IEC1SET = _IEC1_DMA0IE_MASK;

	// DMACON gets enabled at the end of setup in main.
}

void UARTDrv_Init(struct cdc_line_coding* coding){
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
	if (coding->bCharFormat == CDC_CHAR_FORMAT_2_STOP_BITS){
		UART_MODE_bits.STSEL = 1;	// 2 stop bits
	}
	else{
		UART_MODE_bits.STSEL = 0;	// 1 stop bit
	}

	UART_STA_bits.ADM_EN = 0;	// Don't care for auto address detection, unused
	UART_STA_bits.ADDR = 0;		// Don't care for auto address mark
	UART_STA_bits.UTXISEL = 0b10;	//TODO
	UART_STA_bits.UTXINV = 0;	// Idle HIGH
	UART_STA_bits.URXEN = 1;	// UART receiver pin enabled
	UART_STA_bits.UTXBRK = 0;	// Don't send breaks.
	UART_STA_bits.UTXEN = 1;	// Uart transmitter pin enabled
	UART_STA_bits.URXISEL = 0;	// Interrupt what receiver buffer not empty
	UART_STA_bits.ADDEN = 0;	// Address detect mode disabled (unused)
	UART_STA_bits.OERR = 0;		// Clear RX Overrun bit - not important at this point

	// (PBCLK/BRGH?4:16)/BAUD - 1
	UART_BRG_reg = (GetPeripheralClock() / (U2MODEbits.BRGH ? 4 : 16)) / coding->dwDTERate - 1;

	// Setup interrupt - Split into new function fer easier ifdef-ing?
	UART_INT_IPC_bits.UART_INT_IPC_PRIORITY		= 6;	// Priority = 6, highest, above USB.
														// Once receiving is put into DMA, it can be lower.
	UART_INT_IPC_bits.UART_INT_IPC_SUBPRIORITY 	= 0;	// Subpriority = 0;
	UART_INT_IEC_bits.UART_INT_IEC_RXIE			= 1;	// Enable interrupt.

	UARTDrv_InitDMA();	// Perform initializations of DMA.

	UART_MODE_bits.ON = 1;
}

void UARTDrv_SendBlocking(uint8_t * buffer, uint32_t length){
	// TODO remove.
	uint32_t counter = 0;

	for (counter = 0; counter<length; counter++){
		while(UART_STA_bits.UTXBF){ asm("nop"); }
		UART_TX_reg = buffer[counter];
		asm("nop");
	}
}



// Can expand for modularity, if multiple DMAs
uint32_t UARTDrv_IsTxDmaRunning(){
	return (DCH0CONbits.CHEN);	// Don't have a CHBUSY bit. Check if DMA disabled.
}

void UARTDrv_RunDmaTx(){
	// So, here's the thing. For small transfers DMA is going to have a bit of overhead.
	// Also, if we start near the end of the buffer, there will be some performance penalty. Oh well.

	// Which struct
	comStruct* whichStruct = &uartTXstruct;

	// Get size to send
	sizeToSendTx = COMMS_helper_dataLen(whichStruct);

	// Take care of end of buffer edge case. TODO DOUBLE CHECK >=
	if ((whichStruct->tail + sizeToSendTx) > cyclicBufferSizeMask){
		sizeToSendTx = cyclicBufferSizeMask - whichStruct->tail + 1;
	}

	// TODO, save this differently, and update in DMA via DCH0CPTR or whichever.
	if (sizeToSendTx > DMA_MAX_SIZE){
		sizeToSendTx = DMA_MAX_SIZE;	// FUCKING BLANKET DATASHEETS AND SPECIFIC "some bits are not available on all devices" BULLSHIT.
	}

	UART_INT_IFS_bits.UART_INT_IFS_TXIF = 0;	// Clear TX done of last transfer.

	//DCH0CONbits.CHAEN = 0;						// Disable channel
	DCH0CONCLR = _DCH0CON_CHAEN_MASK;
	asm("nop");


	//DCH0ECON = 0;
	DCH0ECONCLR = 0xFFFFFFFF;
	asm("nop");
	//DCH0ECONbits.CHSIRQ = _UART1_TX_IRQ;		// Which interrupts enable the transfer. Interrupts from 0 to 255. U1TX = 28 on 440F
	DCH0ECONSET = (_UART1_TX_IRQ << _DCH0ECON_CHSIRQ_POSITION) | _DCH0ECON_SIRQEN_MASK;	// Run on irq set by CHSIRQ
	asm("nop");


	DCH0SSA = KVA_TO_PA(&(whichStruct->data[whichStruct->tail]));	// Source address
	DCH0DSA = KVA_TO_PA(&U1TXREG);									// Destination address
	DCH0SSIZ = (sizeToSendTx >= DMA_MAX_SIZE) ? 0 : sizeToSendTx;	// Source is of size sizeToSendTX. 0 == 2^numBits.
	DCH0DSIZ = 1;													// UART register is 1B large
	DCH0CSIZ = 1;													// 1B per UART transfer

	DCH0INTCLR = 0x000000FF;										// Clear all interrupt flags
	// IE flags are already set in DMA_init


	//DCH0CONbits.CHEN = 1;		// Enable channel
	DCH0CONSET = _DCH0CON_CHEN_MASK;
	asm("nop");
	//DCH0ECONbits.CFORCE = 1;	// Force start first transfer.
	DCH0ECONSET = _DCH0ECON_CFORCE_MASK;
	asm("nop");
}

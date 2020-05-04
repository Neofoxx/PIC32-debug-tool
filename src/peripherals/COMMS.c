#include <p32xxxx.h>
#include <inttypes.h>
#include <COMMS.h>
#include <usb.h>
#include <string.h>
#include <stdio.h>
#include <LED.h>
#include <UARTDrv.h>
#include <transport.h>
#include <GPIODrv.h>
#include <pic32_prog_definitions.h>

// This file handles communication
// - USB-UART will be one type.
// - USB to programmer will be the other type

// Circular buffers, all same size.
// uartRX is fed with UART RX DMA. We have to make sure
// that we send data fast enough, so we don't get buffer
// overrun errors.
// uartTX is transmitted via UART TX DMA,
// at a more leisurely pace.
// progOUT is our data/commands, that we must execute.
// progRET are our responses.
comStruct uartRXstruct;	// Target to PC
comStruct uartTXstruct;	// PC to target
comStruct progOUTstruct;	// PC to target
comStruct progRETstruct;	// Us to PC (Return values)


// FUNCTIONS UART
// Setup UART RX DMA - function in UART Driver - TODO!!

void COMMS_USB_uartRX_transmitBuf(){
	// Called from USB routine, to send data we received from target
	while (usb_in_endpoint_busy(EP_UART_NUM)){
	}

	comStruct* whichStruct = &uartRXstruct;	// Copy&paste error protection
	uint32_t sizeToSend = COMMS_helper_dataLen(whichStruct);
	uint8_t *buf;
	if (sizeToSend>512){
		sizeToSend=8*64;	// Limit to number of packets
	}

	while (sizeToSend>0){
		LED_USBUART_IN_toggle();	// Toggle on each send, for nicer debugging

		buf = usb_get_in_buffer(EP_UART_NUM);
		if (sizeToSend>=EP_UART_NUM_LEN){
			COMMS_helper_getData(whichStruct, EP_UART_NUM_LEN, buf);
			usb_send_in_buffer(EP_UART_NUM, EP_UART_NUM_LEN);	// Send on endpoint EP_UART_NUM, of length EP_UART_NUM_LEN

			while (usb_in_endpoint_busy(EP_UART_NUM)){
				asm("nop");
			}

			if (sizeToSend==EP_UART_NUM_LEN){
				// If we land on boundary, send a zero-length packet
				LED_USBPROG_IN_toggle();
				usb_send_in_buffer(EP_UART_NUM, 0);
				// ><, forgot
				while (usb_in_endpoint_busy(EP_UART_NUM)){
					asm("nop");
				}
			}
			sizeToSend = sizeToSend - EP_UART_NUM_LEN;
		}
		else{
			COMMS_helper_getData(whichStruct, sizeToSend, buf);
			usb_send_in_buffer(EP_UART_NUM, sizeToSend);
			while (usb_in_endpoint_busy(EP_UART_NUM)){
				asm("nop");
			}
			sizeToSend = sizeToSend - sizeToSend;
		}
	}

	whichStruct->timeStamp = _CP0_GET_COUNT();
}

// UART TX - add data from USB to UART TX
uint32_t COMMS_uartTX_addToBuf(){
	// Called from USB, to gives us data from PC
	comStruct* whichStruct = &uartTXstruct;	// Copy&paste error protection
	const unsigned char *out_buf;
	volatile size_t out_buf_len;
	//uint16_t counter = 0;

	LED_USBUART_OUT_toggle();	// Debug toggle

	// Check for an empty transaction.
	out_buf_len = usb_get_out_buffer(EP_UART_NUM, &out_buf);
	if ( (out_buf_len <= 0)){
		return 1;	// Rearms in interrupt handler.
	}
	else{
		LED_USBUART_OUT_toggle();	// Debug toggle

		if (COMMS_helper_addToBuf(whichStruct, (uint8_t *)out_buf, out_buf_len)){
			return 1;	// If no space, signal, that it shouldn't rearm the endpoint.
		}
	}

	whichStruct->timeStamp = _CP0_GET_COUNT();
	return 0;	// 0 on success, else otherwise (no more space, PC will buffer for us :3)

}

void COMMS_uartTX_transmitBuf(){

	// Either self-call on DMA DONE interrupt,
	// or call first time/periodically from USB interrupt
	// -> in that case, check if DMA active and then restart DMA
	// Possible race condition if DMA starts reactivating, and USB interferes
	// Don't fuck this up -.-
}

// FUNCTIONS PROGRAMMER
uint32_t COMMS_progOUT_addToBuf(){
	// Called from USB, to gives us data from PC
	comStruct* whichStruct = &progOUTstruct;	// Copy&paste error protection
	const unsigned char *out_buf;
	volatile size_t out_buf_len;
	//uint16_t counter = 0;

	LED_USBPROG_OUT_toggle();	// Debug toggle

	// Check for an empty transaction.
	out_buf_len = usb_get_out_buffer(EP_PROG_NUM, &out_buf);
	if ( (out_buf_len <= 0)){
		return 1;	// Rearms in interrupt handler.
	}
	else{
		LED_USBPROG_OUT_toggle();	// Debug toggle

		if (COMMS_helper_addToBuf(whichStruct, (uint8_t *)out_buf, out_buf_len)){
			return 1;	// If no space, signal, that it shouldn't rearm the endpoint.
		}
	}

	whichStruct->timeStamp = _CP0_GET_COUNT();
	return 0;	// 0 on success, else otherwise (no more space, PC will buffer for us :3)
}


void COMMS_USB_progRET_transmitBuf(){
	// Called from USB routine, to send data from the programmer to PC
	while (usb_in_endpoint_busy(EP_PROG_NUM)){
	}

	comStruct* whichStruct = &progRETstruct;	// Copy&paste error protection
	uint32_t sizeToSend = COMMS_helper_dataLen(whichStruct);
	uint8_t *buf;
	if (sizeToSend>512){
		sizeToSend=8*64;	// Limit to number of packets
	}

	while (sizeToSend>0){
		LED_USBPROG_IN_toggle();	// Toggle on each send, for nicer debugging

		buf = usb_get_in_buffer(EP_PROG_NUM);
		if (sizeToSend>=EP_PROG_NUM_LEN){
			COMMS_helper_getData(whichStruct, EP_PROG_NUM_LEN, buf);
			usb_send_in_buffer(EP_PROG_NUM, EP_PROG_NUM_LEN);	// Send on endpoint EP_PROG_NUM, of length EP_PROG_NUM_LEN

			while (usb_in_endpoint_busy(EP_PROG_NUM)){
				asm("nop");
			}

			if (sizeToSend==EP_PROG_NUM_LEN){
				// If we land on boundary, send a zero-length packet
				LED_USBPROG_IN_toggle();
				usb_send_in_buffer(EP_PROG_NUM, 0);
				// ><, forgot
				while (usb_in_endpoint_busy(EP_PROG_NUM)){
					asm("nop");
				}
			}
			sizeToSend = sizeToSend - EP_PROG_NUM_LEN;
		}
		else{
			COMMS_helper_getData(whichStruct, sizeToSend, buf);
			usb_send_in_buffer(EP_PROG_NUM, sizeToSend);
			while (usb_in_endpoint_busy(EP_PROG_NUM)){
				asm("nop");
			}
			sizeToSend = sizeToSend - sizeToSend;
		}
	}

	whichStruct->timeStamp = _CP0_GET_COUNT();
}


// HELPER FUNCTION
// helper function to add to buffer, so don't have to deal with hard coded things etc.
uint32_t COMMS_helper_addToBuf(comStruct* st, uint8_t* data, uint16_t length){
	if (COMMS_helper_spaceLeft(st) < length){
		return 1;	// Fail
	}

	uint32_t i = 0;
	for (i=0; i < length; i++){
		st->data[st->head] = data[i];
		st->head = (st->head + 1) & cyclicBufferSizeMask;
	}

	return 0; // 0 on success, else otherwise (no space available)
}

// Returns how much data is in the struct
inline uint32_t COMMS_helper_dataLen(comStruct* st){
	return (st->head - st->tail) & cyclicBufferSizeMask;
}

// Returns how much space is left in the struct
inline uint32_t COMMS_helper_spaceLeft(comStruct* st){
	return (st->tail - st->head - 1) & cyclicBufferSizeMask;
}



inline void COMMS_helper_getData(comStruct* st, uint32_t length, uint8_t *buf){
	if (COMMS_helper_dataLen(st) < length){
		// Don't do this, check beforehand
	}
	uint32_t i = 0;
	for (i=0; i< length; i++){
		buf[i] = st->data[st->tail];
		st->tail = (st->tail + 1) & cyclicBufferSizeMask;
	}
}

// Returns how much time has passed, since data last sent
uint32_t COMMS_helper_timeSinceSent(comStruct* st){

	return _CP0_GET_COUNT() - st->timeStamp;	// Read2 - read1, should wrap nicely.
}

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

// Structures of data. Init with defaults.
commStruct incomingData = {
		{0}, 0, 0, 0, 0, 0, 0
};

commStruct outgoingData= {
		{0}, 0, 0, 0, 0, 0, 0
};

uint8_t tempBuffer[2048];
uint8_t uartBuffer[2048];

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

void COMMS_reinitStruct(commStruct *st, uint32_t cleanAll){
	//memset(st->data, 0, sizeof(st->data));
	if (cleanAll){
		st->head = 0;
		st->tail = 0;
	}
	st->currentPos		= 0;
	st->type			= 0;
	st->expectedLength	= 0;
	st->crc				= 0;
	st->status			= 0;
}

// FUNCTIONS UART
// Setup UART RX DMA - function in UART Driver - TODO!!


uint32_t COMMS_USB_uartRX_transmitBuf(){
	// Called from USB routine, to send data we received from target
	while (usb_in_endpoint_busy(EP_UART_NUM)){
	}
	uint32_t i = PROG_EP_OUT_LEN;
	uint8_t *buf;

	while (i>0){
		buf = usb_get_in_buffer(EP_UART_NUM);
		if (i>=EP_UART_NUM_LEN){
			memcpy(buf, &(PROG_EP_OUT[PROG_EP_OUT_LEN - i]), EP_UART_NUM_LEN);
			usb_send_in_buffer(EP_UART_NUM, EP_UART_NUM_LEN);	// Send on endpoint 4, of length i

			if (i==EP_UART_NUM_LEN){
				// If we land on boundary, send a zero-length packet
				while (usb_in_endpoint_busy(4)){
				}
				usb_send_in_buffer(4, 0);
			}
			i = i - EP_UART_NUM_LEN;
			while (usb_in_endpoint_busy(4)){
			}
		}
		else{
			memcpy(buf, &(PROG_EP_OUT[PROG_EP_OUT_LEN - i]), i);
			usb_send_in_buffer(4, i);
			while (usb_in_endpoint_busy(4)){
			}
			i = i - i;
		}
	}
	PROG_EP_OUT_LEN = 0;
	// NOTE - don't forget to mark size_last_sent, 'cos if EP_x_LEN, and no more data, <i>must</i> send packet of 0!
}

// UART TX - add data from USB to UART TX
uint32_t COMMS_uartTX_addToBuf(uint8_t* buffer, uint16_t size){
	// Called from USB, to gives us data from PC

	const unsigned char *out_buf;
	volatile size_t out_buf_len;
	uint16_t counter = 0;

	// Check for an empty transaction.
	out_buf_len = usb_get_out_buffer(4, &out_buf);
	if (out_buf_len <= 0){
		// Let's avoid gotos
		usb_arm_out_endpoint(4);
	}
	else{
		LED_toggle();

		for (counter = 0; counter < out_buf_len; counter++){
			UART_EP_OUT[counter] = out_buf[counter];
		}
		UART_EP_OUT_LEN = out_buf_len;

		usb_arm_out_endpoint(4);
	}

	return 0;	// 0 on success, else otherwise (no more space, PC will buffer for us :3)

	// TODO IMPLEMENT
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
	const unsigned char *out_buf;
	volatile size_t out_buf_len;
	uint16_t counter = 0;

	// Check for an empty transaction.
	out_buf_len = usb_get_out_buffer(2, &out_buf);
	if (out_buf_len <= 0){
		// Let's avoid gotos
		usb_arm_out_endpoint(2);
	}
	else{
		LED_toggle();

		for (counter = 0; counter < out_buf_len; counter++){
			PROG_EP_OUT[counter] = out_buf[counter];
		}
		PROG_EP_OUT_LEN = out_buf_len;

		usb_arm_out_endpoint(2);
	}

	return 0;	// 0 on success, else otherwise (no more space, PC will buffer for us :3)
}


void COMMS_USB_progRET_transmitBuf(){
	// Called from USB routine, to send data we received from target
	if (!usb_in_endpoint_halted(2)){
		while (usb_in_endpoint_busy(2)){
		}
		uint32_t i = UART_EP_OUT_LEN;
		uint8_t *buf;

		while (i>0){
			buf = usb_get_in_buffer(2);
			if (i>=EP_2_LEN){
				memcpy(buf, &(UART_EP_OUT[UART_EP_OUT_LEN - i]), EP_2_LEN);
				usb_send_in_buffer(2, EP_2_LEN);	// Send on endpoint 2, of length i

				if (i==EP_2_LEN){
					// If we land on boundary, send a zero-length packet
					while (usb_in_endpoint_busy(2)){
					}
					usb_send_in_buffer(2, 0);
				}
				i = i - EP_2_LEN;
				while (usb_in_endpoint_busy(2)){
				}
			}
			else{
				memcpy(buf, &(UART_EP_OUT[UART_EP_OUT_LEN - i]), i);
				usb_send_in_buffer(2, i);
				while (usb_in_endpoint_busy(2)){
				}
				i = i - i;
			}
		}
		UART_EP_OUT_LEN = 0;

	}

	// NOTE - don't forget to mark size_last_sent, 'cos if EP_x_LEN, and no more data, <i>must</i> send packet of 0!
}


// HELPER FUNCTION
// helper function to add to buffer, so don't have to deal with hard coded things etc.
uint32_t COMMS_helper_addToBuf(comStruct* st, uint8_t data, uint16_t size){

	return 0; // 0 on success, else otherwise (no space available)
}

// Returns how much data is in the struct
uint32_t COMMS_helper_dataLen(comStruct* st){

	return 0;
}

// Returns how much time has passed, since data last sent
uint32_t COMMS_helper_timeSinceSent(comStruct* st){

	return 0;
}

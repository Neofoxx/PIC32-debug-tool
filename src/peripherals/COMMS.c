#include <p32xxxx.h>
#include <inttypes.h>
#include <COMMS.h>
#include <usb.h>
#include <string.h>
#include <stdio.h>
#include <LED.h>
#include <UARTDrv.h>
#include <transport.h>

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

uint8_t tempBuffer[256];
uint8_t uartBuffer[256];

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

// Called from the USB interrupt, so it needs to be fast...
void COMMS_addToInputBuffer(void){
	if (usb_is_configured() && !usb_out_endpoint_halted(2) && usb_out_endpoint_has_data(2)) {
		const unsigned char *out_buf;
		volatile size_t out_buf_len;
		uint16_t counter = 0;

		// Check for an empty transaction.
		out_buf_len = usb_get_out_buffer(2, &out_buf);
		if (out_buf_len <= 0){
			// Let's avoid gotos
			usb_arm_out_endpoint(2);
			return;
		}

		LED_toggle();

		// If we can't do anything - buffer full
		if (incomingData.status == status_overflow){
			usb_arm_out_endpoint(2);	// Rearm endpoint.
			return;
		}

		for (counter = 0; counter < out_buf_len; counter++){
			if (((incomingData.head+1) % sizeof(incomingData.data)) == incomingData.tail){
				incomingData.status = status_overflow;
				break;
			}

			incomingData.data[incomingData.head] = out_buf[counter];
			incomingData.head = (incomingData.head + 1) % sizeof(incomingData.data);
		}

		usb_arm_out_endpoint(2);
	}

}

void COMMS_sendStruct(commStruct *st){
	// Function sends raw data from commStruct

	if (usb_is_configured() && !usb_in_endpoint_halted(2)){
		while (usb_in_endpoint_busy(2)){
		}
		uint32_t i = st->currentPos;
		uint8_t *buf;

		while (i>0){
			buf = usb_get_in_buffer(2);
			if (i>=EP_2_LEN){
				memcpy(buf, &(st->data[st->currentPos - i]), EP_2_LEN);
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
				memcpy(buf, &(st->data[st->currentPos - i]), i);
				usb_send_in_buffer(2, i);
				while (usb_in_endpoint_busy(2)){
				}
				i = i - i;
			}
		}

	}
}

void COMMS_handleIncomingProg(void){
	uint32_t startPos = incomingData.tail;
	uint32_t endPos = incomingData.head;

	if (type_empty == incomingData.type){
		while (startPos != endPos){
			if ('i' == incomingData.data[startPos]){
				incomingData.type = type_info;
				startPos = (startPos+1) % sizeof(incomingData.data);
				break;
			}
			else if ('p' == incomingData.data[startPos]){
				incomingData.type = type_packet;
				startPos = (startPos+1) % sizeof(incomingData.data);
				break;
			}
		}
	}
	if(type_info == incomingData.type){
		// So we got an info request.

		// SEND DATA HERE.
		COMMS_addInfoToOutput();
		COMMS_sendStruct(&outgoingData);

		// RESET STRUCTURE(s) HERE
		COMMS_reinitStruct(&outgoingData, 0);

	}
	else if (type_packet == incomingData.type){
		// Concatenate until full, check crc, execute (or send error)
		while (startPos != endPos){

			// Save into tempBuffer, not feelign like more right now.
			tempBuffer[incomingData.currentPos++] = incomingData.data[startPos];
			startPos = (startPos+1) % sizeof(incomingData.data);

			if (0 == incomingData.expectedLength && 2 == incomingData.currentPos){
				incomingData.expectedLength = ((uint16_t)tempBuffer[1] << 8) | (uint16_t)tempBuffer[0];
				incomingData.currentPos = 0;
				if (incomingData.expectedLength > 10){
					asm("nop");
				}
				uint16_t temp = snprintf(	(char *)uartBuffer, sizeof(uartBuffer),
											"Calculated length: %d\n", incomingData.expectedLength);
				UARTDrv_SendBlocking(uartBuffer, temp);
				continue;
			}
			else if (0 == incomingData.expectedLength){
				continue;
			}

			if (incomingData.currentPos == incomingData.expectedLength){
				// Calculate crc
				uint8_t tempCrc = 0;
				uint16_t tempCounter = 0;
				for (tempCounter = 0; tempCounter < incomingData.currentPos-1; tempCounter++){
					tempCrc = tempCrc+tempBuffer[tempCounter];
				}
				if(tempCrc == tempBuffer[incomingData.currentPos-1]){
					// Valid CRC!
					incomingData.status = status_parsed;

					// Send
					uint16_t temp = snprintf(	(char *)uartBuffer, sizeof(uartBuffer),
												"Got a packet with a valid CRC\n");
					UARTDrv_SendBlocking(uartBuffer, temp);

					// At this point, go and do something about all of the commands.
					COMMS_commandExecutor();

					// Clear structures
					COMMS_reinitStruct(&incomingData, 0);
					COMMS_reinitStruct(&outgoingData, 0);
					break;
				}
				else{
					// Error in decoding
					incomingData.status = status_error;

					// Do something about it?
					uint16_t temp = snprintf(	(char *)uartBuffer, sizeof(uartBuffer),
												"ERROR in decoding\n");
					UARTDrv_SendBlocking(uartBuffer, temp);

					// Clear structures
					COMMS_reinitStruct(&incomingData, 0);
					COMMS_reinitStruct(&outgoingData, 0);
					break;
				}
			}

		}
	}

	incomingData.tail = startPos;
	//incomingData.status = status_ok;	// Assumed.
}

void COMMS_addInfoToOutput(){
	uint16_t otherTemp = 0;
	uint16_t startPos = outgoingData.currentPos;
	uint16_t temp = snprintf(	(char *)&outgoingData.data[outgoingData.currentPos],
								sizeof(outgoingData.data) - outgoingData.currentPos,
								"INFO packet from device\n");
	otherTemp = otherTemp + temp;
	outgoingData.currentPos = outgoingData.currentPos + temp;

	temp = snprintf(	(char *)&outgoingData.data[outgoingData.currentPos],
						sizeof(outgoingData.data) - outgoingData.currentPos,
						"MCU: 32MX440F256H\nMODE: Bitbang\nNAME: Neofoxx PIC32-debug-tool\n\n");
	otherTemp = otherTemp + temp;
	outgoingData.currentPos = outgoingData.currentPos + temp;

	if (otherTemp > 128){
		otherTemp = 128;
	}
	// Set rest of data to \0
	memset(&outgoingData.data[outgoingData.currentPos], 0, 128 - otherTemp);
	outgoingData.currentPos = startPos + 128;
}

void COMMS_commandExecutor(){
	uint32_t counter;

	if (incomingData.status != status_parsed){
		return;
	}

	// Start at position 0 in the packet (reset earlier ><).
	for (counter = 0; counter < (incomingData.currentPos-1); ){
		if (COMMAND_GET_INFO == tempBuffer[counter]){
			// Similar as in INFO. Fixed at 128 bytes.
			COMMS_addInfoToOutput();
			COMMS_sendStruct(&outgoingData);
			COMMS_reinitStruct(&outgoingData, 1);
			counter = counter + 1;	// These could be defines (lengths)
		}
		else{
			asm("nop");
		}

	}
}

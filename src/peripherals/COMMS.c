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

					// At this point, go and do something about ALL of the commands.
					COMMS_commandExecutor();

					// Clear structures
					COMMS_reinitStruct(&incomingData, 0);
					COMMS_reinitStruct(&outgoingData, 0);
					temp = snprintf(	(char *)uartBuffer, sizeof(uartBuffer),
												"Reinited structures\n");
										UARTDrv_SendBlocking(uartBuffer, temp);
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

// Make different later?
void COMMS_addDataToOutput_64b(uint64_t data){
	memcpy (&outgoingData.data[outgoingData.currentPos], &data, sizeof(data));
	memcpy (&outgoingData.data[0], &data, sizeof(data));
	outgoingData.currentPos = outgoingData.currentPos + sizeof(data);
}

void COMMS_addDataToOutput_32b(uint32_t data){
	memcpy (&outgoingData.data[outgoingData.currentPos], &data, sizeof(data));
	memcpy (&outgoingData.data[0], &data, sizeof(data));
	outgoingData.currentPos = outgoingData.currentPos + sizeof(data);
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
		else if (COMMAND_SET_SPEED == tempBuffer[counter]){
			// Currently not supported, so skip
			// Speed will be in 2 bytes (kHz)
			counter = counter + 3;
		}
		else if (COMMAND_SET_PROG_MODE == tempBuffer[counter]){
			// The mode is in the next byte.
			transportSetup(tempBuffer[counter+1]);
			counter = counter + 2;
		}
		else if (COMMAND_SET_PIN_IO_MODE == tempBuffer[counter]){
			// First byte is pin
			// Second byte is mode (input, output /w high, output / low)
			if (PIN_TMS == tempBuffer[counter+1]){
				GPIODrv_setupPinTMS(tempBuffer[counter+2]);
			}
			else if (PIN_TCK == tempBuffer[counter+1]){
				GPIODrv_setupPinTCK(tempBuffer[counter+2]);
			}
			else if (PIN_TDI == tempBuffer[counter+1]){
				GPIODrv_setupPinTDI(tempBuffer[counter+2]);
			}
			else if (PIN_TDO == tempBuffer[counter+1]){
				GPIODrv_setupPinTDO(tempBuffer[counter+2]);
			}
			else if (PIN_MCLR == tempBuffer[counter+1]){
				GPIODrv_setupPinMCLR(tempBuffer[counter+2]);
			}

			counter = counter + 3;
		}
		else if (COMMAND_SET_PIN_WRITE == tempBuffer[counter]){
			// First byte is pin
			// Second byte is value (high, low).
			if (PIN_TMS == tempBuffer[counter+1]){
				GPIODrv_setStateTMS(tempBuffer[counter+2]);
			}
			else if (PIN_TCK == tempBuffer[counter+1]){
				GPIODrv_setStateTCK(tempBuffer[counter+2]);
			}
			else if (PIN_TDI == tempBuffer[counter+1]){
				GPIODrv_setStateTDI(tempBuffer[counter+2]);
			}
			else if (PIN_TDO == tempBuffer[counter+1]){
				GPIODrv_setStateTDO(tempBuffer[counter+2]);
			}
			else if (PIN_MCLR == tempBuffer[counter+1]){
				GPIODrv_setStateMCLR(tempBuffer[counter+2]);
			}

			counter = counter + 3;
		}
		else if (COMMAND_SET_PIN_READ == tempBuffer[counter]){
			// First byte is pin
			uint32_t returnVal = 0;
			if (PIN_TMS == tempBuffer[counter+1]){
				returnVal = GPIODrv_getStateTMS();
			}
			else if (PIN_TCK == tempBuffer[counter+1]){
				returnVal = GPIODrv_getStateTCK();
			}
			else if (PIN_TDI == tempBuffer[counter+1]){
				returnVal = GPIODrv_getStateTDI();
			}
			else if (PIN_TDO == tempBuffer[counter+1]){
				returnVal = GPIODrv_getStateTDO();
			}
			else if (PIN_MCLR == tempBuffer[counter+1]){
				returnVal = GPIODrv_getStateMCLR();
			}

			// TODO implement return.
			COMMS_addDataToOutput_32b(returnVal);

			counter = counter + 2;
		}
		else if (COMMAND_SEND == tempBuffer[counter]){
			// 4B of TMS_prolog nbits, 4B of TMS_prolog value
			// 4B of TDI nbits, 8B of TDI value
			// 4B of TMS_epilog nbits, 4B of TMS_epilog value
			// 4B of read_flag
			// TODO optimize later.

			uint32_t tms_prolog_nbits, tms_prolog;
			uint32_t tdi_nbits;
			uint64_t tdi;
			uint32_t tms_epilog_nbits, tms_epilog;
			uint32_t read_flag;

			uint32_t location = counter + 1;

			volatile uint64_t returnVal = 0;

			// TMS prolog
			memcpy(&tms_prolog_nbits, &tempBuffer[location], sizeof(tms_prolog_nbits));
			location = location + sizeof(tms_prolog_nbits);
			memcpy(&tms_prolog, &tempBuffer[location], sizeof(tms_prolog));
			location = location + sizeof(tms_prolog);

			// TDI
			memcpy(&tdi_nbits, &tempBuffer[location], sizeof(tdi_nbits));
			location = location + sizeof(tdi_nbits);
			memcpy(&tdi, &tempBuffer[location], sizeof(tdi));
			location = location + sizeof(tdi);

			// TMS epilog
			memcpy(&tms_epilog_nbits, &tempBuffer[location], sizeof(tms_epilog_nbits));
			location = location + sizeof(tms_epilog_nbits);
			memcpy(&tms_epilog, &tempBuffer[location], sizeof(tms_epilog));
			location = location + sizeof(tms_epilog);

			// read flag
			memcpy(&read_flag, &tempBuffer[location], sizeof(read_flag));
			location = location + sizeof(read_flag);

			counter = counter + 1;
			counter = counter + sizeof(tms_prolog_nbits) + sizeof(tms_prolog);
			counter = counter + sizeof(tdi_nbits) + sizeof(tdi);
			counter = counter + sizeof(tms_epilog_nbits) + sizeof(tms_epilog);
			counter = counter + sizeof(read_flag);

			if (PROG_MODE_TRISTATE == transport_currentMode){
				// Do nothing?
			}
			else if(PROG_MODE_JTAG == transport_currentMode){
				returnVal = transportSendJTAG(tms_prolog_nbits, tms_prolog, tdi_nbits, tdi, tms_epilog_nbits, tms_epilog);
			}
			else if(PROG_MODE_ICSP == transport_currentMode){
				returnVal = transportSendICSP(tms_prolog_nbits, tms_prolog, tdi_nbits, tdi, tms_epilog_nbits, tms_epilog);
			}

			if (read_flag > 0){
				COMMS_addDataToOutput_64b(returnVal);
				// Sending shall be done when parsing the packet is done, or when a special command is parsed.
			}
		}
		else if (COMMAND_XFER_INSTRUCTION == tempBuffer[counter]){

			uint32_t maxCounter = 0;
			uint32_t maxLimit = 40;	// Just a define

			uint32_t controlVal = 0;
			uint32_t instruction = 0;

			// instruction is at counter+1
			memcpy(&instruction, &tempBuffer[counter+1], sizeof(instruction));
			counter = counter + 1 + sizeof(instruction);


			COMMS_pic32SendCommand(ETAP_CONTROL);

			// Wait until CPU is ready
			// Check if Processor Access bit (bit 18) is set
			do {
				controlVal = (uint32_t)COMMS_pic32XferData(32, (CONTROL_PRACC | CONTROL_PROBEN | CONTROL_PROBTRAP | CONTROL_EJTAGBRK), 1);    // Send data, read
				if (!(controlVal & CONTROL_PROBEN)){
					// Note - xfer instruction, ctl was %08x\n
					maxCounter++;
					if (maxCounter >= maxLimit){
						// Processor still not ready :/, give up.
						break;
					}
				}
			} while ( !(controlVal & CONTROL_PROBEN));

			if (maxCounter < maxLimit){
				// Select Data Register
				COMMS_pic32SendCommand(ETAP_DATA);    // ETAP_DATA

				/* Send the instruction */
				COMMS_pic32XferData(32, instruction, 0);  // Send instruction, don't read

				// Tell CPU to execute instruction
				COMMS_pic32SendCommand(ETAP_CONTROL); // ETAP_CONTROL

				/* Send data. */
				COMMS_pic32XferData(32, (CONTROL_PROBEN | CONTROL_PROBTRAP), 0);   // Send data, don't read

				// Return status, so application knows what's happening
				COMMS_addDataToOutput_64b(0);
			}
			else{
				COMMS_addDataToOutput_64b(0x80000000);	// -1 in case of FAIL.
			}

		}
		else{
			asm("nop");
		}

	}

	// At the END, send everything we have in the outgoing buffer.
	// If there'll be a "send everything you've got right now" command, then _also_ do it then.
	COMMS_sendStruct(&outgoingData);
	COMMS_reinitStruct(&outgoingData, 1);
}


void COMMS_pic32SendCommand(uint32_t command){

	// WTf?
	// Bi mogoce morali biti == command || ... ???
	// ... Ja, sam je ista cifra, pa zato dela. ><
	// fuckit, popravi kasneje.

	uint32_t returnVal;

	if (MTAP_COMMAND != command && TAP_SW_MTAP != command
		&& TAP_SW_ETAP != command && MTAP_IDCODE != command){   // MTAP commands
		if (PROG_MODE_TRISTATE == transport_currentMode){
			// Do nothing?
		}
		else if(PROG_MODE_JTAG == transport_currentMode){
			returnVal = transportSendJTAG(TMS_HEADER_COMMAND_NBITS, TMS_HEADER_COMMAND_VAL,
											MTAP_COMMAND_NBITS, command,
											TMS_FOOTER_COMMAND_NBITS, TMS_FOOTER_COMMAND_VAL);
		}
		else if(PROG_MODE_ICSP == transport_currentMode){
			returnVal = transportSendICSP(TMS_HEADER_COMMAND_NBITS, TMS_HEADER_COMMAND_VAL,
											MTAP_COMMAND_NBITS, command,
											TMS_FOOTER_COMMAND_NBITS, TMS_FOOTER_COMMAND_VAL);
		}

	}
	else if (ETAP_ADDRESS != command && ETAP_DATA != command    // ETAP commands
			&& ETAP_CONTROL != command && ETAP_EJTAGBOOT != command
			&& ETAP_FASTDATA != command && ETAP_NORMALBOOT != command){
		if (PROG_MODE_TRISTATE == transport_currentMode){
			// Do nothing?
		}
		else if(PROG_MODE_JTAG == transport_currentMode){
			returnVal = transportSendJTAG(TMS_HEADER_COMMAND_NBITS, TMS_HEADER_COMMAND_VAL,
											ETAP_COMMAND_NBITS, command,
											TMS_FOOTER_COMMAND_NBITS, TMS_FOOTER_COMMAND_VAL);
		}
		else if(PROG_MODE_ICSP == transport_currentMode){
			returnVal = transportSendICSP(TMS_HEADER_COMMAND_NBITS, TMS_HEADER_COMMAND_VAL,
											ETAP_COMMAND_NBITS, command,
											TMS_FOOTER_COMMAND_NBITS, TMS_FOOTER_COMMAND_VAL);
		}
	}
}

uint64_t COMMS_pic32XferData(uint32_t nBits, uint32_t data, uint32_t readFlag){
	volatile uint64_t returnVal = 0;

	if (PROG_MODE_TRISTATE == transport_currentMode){
		// Do nothing?
	}
	else if(PROG_MODE_JTAG == transport_currentMode){
		returnVal = transportSendJTAG(TMS_HEADER_XFERDATA_NBITS, TMS_HEADER_XFERDATA_VAL,
										nBits, (uint64_t)data,
										TMS_FOOTER_XFERDATA_NBITS, TMS_FOOTER_XFERDATA_VAL);
	}
	else if(PROG_MODE_ICSP == transport_currentMode){
		returnVal = transportSendICSP(TMS_HEADER_XFERDATA_NBITS, TMS_HEADER_XFERDATA_VAL,
										nBits, (uint64_t)data,
										TMS_FOOTER_XFERDATA_NBITS, TMS_FOOTER_XFERDATA_VAL);
	}

	if (readFlag > 0){
		return returnVal;
	}

	return 0;
}






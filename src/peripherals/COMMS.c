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
#include <configBits.h>

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

// TODO - initialize all to 0, don't trust the compiler.
comStruct uartRXstruct;	// Target to PC
comStruct uartTXstruct;	// PC to target
comStruct progOUTstruct;	// PC to target
comStruct progRETstruct;	// Us to PC (Return values)

dataDecoder packetHelper;

uint32_t clkDelay = 20;		// Delay for timing things right (not too fast)
							// Delay is in * 100ns and at 1/2 CLK FREQ!
							// Remember, there is a delay per edge = 2 per clk

// FUNCTIONS UART
// Setup UART RX DMA - function in UART Driver - TODO!!

uint32_t COMMS_USB_uartRX_transmitBuf(){
	// Called from USB routine, to send data we received from target

	comStruct* whichStruct = &uartRXstruct;	// Copy&paste error protection
	uint32_t sizeToSend = COMMS_helper_dataLen(whichStruct);
	uint8_t *buf;

	if (sizeToSend > EP_UART_NUM_LEN){
		sizeToSend = EP_UART_NUM_LEN;	// Limit to size of endpoint
	}

	whichStruct->timeStamp = _CP0_GET_COUNT();			// Save current time.

	if (sizeToSend == 0){
		if (whichStruct->sizeofLastSent == EP_UART_NUM_LEN){
			// If we landed on a boundary last time, send a zero-length packet
			usb_send_in_buffer(EP_UART_NUM, 0);
			LED_USBUART_IN_toggle();	// Toggle on each send, for nicer debugging
			whichStruct->sizeofLastSent = 0;
			return 0;	// Return - packet used up
		}
		else{
			// Do nothing. Nothing to send, no transaction to complete.
			return 1;	// Return - nothing to be done
		}
	}
	else{
		buf = usb_get_in_buffer(EP_UART_NUM);			// Get buffer from endpoint
		COMMS_helper_getData(whichStruct, sizeToSend, buf);	// Get sizeToSend data and copy into buf
		usb_send_in_buffer(EP_UART_NUM, sizeToSend);	// Send on endpoint EP_UART_NUM, of length sizeToSend
		LED_USBUART_IN_toggle();	// Toggle on each send, for nicer debugging
		whichStruct->sizeofLastSent = sizeToSend;		// Save data size, so we can finish transaction if needed
		return 0;		// Return - packet used up
	}
}

// UART TX - add data from USB to UART TX
uint32_t COMMS_uartTX_addToBuf(){
	// Called from USB, to gives us data from PC
	comStruct* whichStruct = &uartTXstruct;	// Copy&paste error protection
	const unsigned char *out_buf;
	volatile size_t out_buf_len;
	//uint16_t counter = 0;

	// Check for an empty transaction.
	out_buf_len = usb_get_out_buffer(EP_UART_NUM, &out_buf);
	if ( (out_buf_len <= 0)){
		usb_arm_out_endpoint(EP_UART_NUM);
		LED_USBUART_OUT_toggle();	// Debug toggle
		return 0;	// Return - packet used up
	}
	else{
		if (COMMS_helper_addToBuf(whichStruct, (uint8_t *)out_buf, out_buf_len)){
			return 1;	// Return - nothing to be done
		}
	}

	usb_arm_out_endpoint(EP_UART_NUM);
	LED_USBUART_OUT_toggle();	// Debug toggle
	whichStruct->timeStamp = _CP0_GET_COUNT();
	return 0;	// Return - packet used up

}

void COMMS_uartTX_transmitBuf(){
	// Wholly implemented in the UARTDrv. Remove later, or intergrate differently.
}

// FUNCTIONS PROGRAMMER
uint32_t COMMS_progOUT_addToBuf(){
	// Called from USB, to gives us data from PC
	comStruct* whichStruct = &progOUTstruct;	// Copy&paste error protection
	const unsigned char *out_buf;
	volatile size_t out_buf_len;

	// Check for an empty transaction.
	out_buf_len = usb_get_out_buffer(EP_PROG_NUM, &out_buf);
	if ( (out_buf_len <= 0)){
		// This should happen on edge boundaries. Nothing wrong here.
		usb_arm_out_endpoint(EP_PROG_NUM);
		LED_USBPROG_OUT_toggle();	// Debug toggle
		return 0;	// Signal packet used
	}
	else{
		if (COMMS_helper_addToBuf(whichStruct, (uint8_t *)out_buf, out_buf_len)){
			return 1;	// If no space, signal, that we didn't rearm endpoint.
		}
	}

	usb_arm_out_endpoint(EP_PROG_NUM);
	LED_USBPROG_OUT_toggle();	// Debug toggle
	whichStruct->timeStamp = _CP0_GET_COUNT();
	return 0;	// 0 on success, else otherwise (no more space, PC will buffer for us :3)
}


uint32_t COMMS_USB_progRET_transmitBuf(){
	// Called from USB routine, to send data from the programmer to PC

	comStruct* whichStruct = &progRETstruct;	// Copy&paste error protection
	uint32_t sizeToSend = COMMS_helper_dataLen(whichStruct);
	uint8_t *buf;


	if (sizeToSend>EP_PROG_NUM_LEN){
		sizeToSend=EP_PROG_NUM_LEN;	// Limit to number of packets
	}

	whichStruct->timeStamp = _CP0_GET_COUNT();			// Save current time.


	if (sizeToSend == 0){
		if (whichStruct->sizeofLastSent == EP_PROG_NUM_LEN){
			// If we landed on a boundary last time, send a zero-length packet
			usb_send_in_buffer(EP_PROG_NUM, 0);
			LED_USBPROG_IN_toggle();	// Toggle on each send, for nicer debugging
			whichStruct->sizeofLastSent = 0;
			return 0;	// Return - packet used up
		}
		else{
			// Do nothing. Nothing to send, no transaction to complete.
			return 1;	// Return - nothing to be done
		}
	}
	else{
		buf = usb_get_in_buffer(EP_PROG_NUM);			// Get buffer from endpoint
		COMMS_helper_getData(whichStruct, sizeToSend, buf);	// Get sizeToSend data and copy into buf
		usb_send_in_buffer(EP_PROG_NUM, sizeToSend);	// Send on endpoint EP_UART_NUM, of length sizeToSend
		LED_USBPROG_IN_toggle();	// Toggle on each send, for nicer debugging
		whichStruct->sizeofLastSent = sizeToSend;		// Save data size, so we can finish transaction if needed
		return 0;		// Return - packet used up
	}

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

// Copy data from start to position+start-1 into *buf
inline void COMMS_helper_peekData(const uint8_t *inData, uint32_t start, uint32_t length, uint8_t * buf){
	uint32_t counter = 0;
	for (counter = 0; counter < length; counter++){
		buf[counter] = inData[(start + counter) & cyclicBufferSizeMask];	// Also takes care of if start is already out of bounds.
	}
}

// Returns how much time has passed, since data last sent
uint32_t COMMS_helper_timeSinceSent(comStruct* st){

	return _CP0_GET_COUNT() - st->timeStamp;	// Read2 - read1, should wrap nicely.
}

///////////////////////////////////////////////////////////////////
// Function to setup how many cycles to delay when doing CLKs
// Note this is important for BITBANG mode.
// Ideally, we'd use something like SPI that has nice even edges,
// but in bitbang mode we have to make do.
// TODO - move to transport layer or something.
void COMMS_helper_setClkDelay(uint32_t frequency){
	// Check some upper limit
	if (frequency > 5000000){
		frequency = 5000000;
	}

	clkDelay = (10000000 / frequency) / 2;	// Get 100ns worth of ticks per clock edge.
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// OLD COMMS CODE just modified


// Our main function here.
void COMMS_handleIncomingProg(void){
	uint32_t startPos = progOUTstruct.tail;
	uint32_t endPos = progOUTstruct.head;

	// At this point, also check for timeout
	// -> If that hasn't been clocked in for a while, clear buffers and send error or something.

	if (type_empty == packetHelper.type){
		while (startPos != endPos){
			if ('i' == progOUTstruct.data[startPos]){
				packetHelper.type = type_info;
				startPos = (startPos+1) & cyclicBufferSizeMask;
				break;
			}
			else if ('p' == progOUTstruct.data[startPos]){
				packetHelper.type = type_packet;
				startPos = (startPos+1) & cyclicBufferSizeMask;
				break;
			}
			startPos = (startPos+1) & cyclicBufferSizeMask;
		}

		// If successful/parsed, update space position in buffer
		progOUTstruct.tail = startPos;
	}
	if(type_info == packetHelper.type){
		// So we got an info request.

		// SEND DATA HERE.
		COMMS_addInfoToOutput();

		// RESET STRUCTURE(s) HERE
		COMMS_reinitPacketHelper(&packetHelper);

	}
	else if (type_packet == packetHelper.type){
		// Concatenate until full, check crc, execute (or send error)
		//while (startPos != endPos){

			// Updated version, that does everything in-situ
			if (0 == packetHelper.expectedLength){
					if (COMMS_helper_dataLen(&progOUTstruct) >= 2){
					// If enough data in buffer, calculate length of packet
					//packetHelper.expectedLength = ((uint16_t)tempBuffer[1] << 8) | (uint16_t)tempBuffer[0];
					packetHelper.expectedLength = (uint16_t)progOUTstruct.data[startPos];
					startPos = (startPos+1) & cyclicBufferSizeMask;
					packetHelper.expectedLength = packetHelper.expectedLength | (uint16_t)progOUTstruct.data[startPos] << 8;
					startPos = (startPos+1) & cyclicBufferSizeMask;
					packetHelper.currentPos = 0;

					// If successful/parsed, update space position in buffer
					progOUTstruct.tail = startPos;
				}
				else{
					// Not enough data available to infer length.
					//break;
				}

			}


			if ((COMMS_helper_dataLen(&progOUTstruct) >= packetHelper.expectedLength) && (0 != packetHelper.expectedLength)){
				// Calculate crc
				uint8_t tempCrc = 0;
				uint16_t tempCounter = 0;
				uint32_t tempTail = startPos;
				// Add all bytes from start to end-1 (where crc is)
				for (tempCounter = 0; tempCounter < packetHelper.expectedLength-1; tempCounter++){
					tempCrc = tempCrc + progOUTstruct.data[tempTail];
					tempTail = (tempTail+1) & cyclicBufferSizeMask;
				}

				// tempTail is already at proper position
				if(tempCrc == progOUTstruct.data[tempTail]){
					// Valid CRC!
					packetHelper.status = status_parsed;

					// At this point, go and do something about ALL of the commands.
					COMMS_commandExecutor();

					// If successful/parsed, update space position in buffer
					progOUTstruct.tail = (startPos + packetHelper.expectedLength) & cyclicBufferSizeMask;

					// Clear structures
					COMMS_reinitPacketHelper(&packetHelper);
					//break;
				}
				else{
					// Error in decoding
					packetHelper.status = status_error;

					// Do something about it? Yes, send "error".
					// TODO

					// If successful/parsed, update space position in buffer
					progOUTstruct.tail = (startPos + packetHelper.expectedLength) & cyclicBufferSizeMask;

					// Clear structures
					COMMS_reinitPacketHelper(&packetHelper);
					//break;
				}

			}


		//}
	}

}

void COMMS_addInfoToOutput(){
	// Send information about the device to PC
	// A long time ago info output got hardcoded to 128B.
	uint8_t emptyFluff [128 - strlen(info.name) - strlen(info.mcu) - strlen(info.mode) - strlen(info.revision)];
	COMMS_helper_addToBuf(&progRETstruct, (uint8_t *)info.name, strlen(info.name));
	COMMS_helper_addToBuf(&progRETstruct, (uint8_t *)info.mcu, strlen(info.mcu));
	COMMS_helper_addToBuf(&progRETstruct, (uint8_t *)info.mode, strlen(info.mode));
	COMMS_helper_addToBuf(&progRETstruct, (uint8_t *)info.revision, strlen(info.revision));

	// Pad to 128 with \0
	memset(emptyFluff, 0, sizeof(emptyFluff));
	COMMS_helper_addToBuf(&progRETstruct, emptyFluff, sizeof(emptyFluff));
}

// Make different later?
void COMMS_addDataToOutput_64b(uint64_t data){
	// Possibly check endianness
	COMMS_helper_addToBuf(&progRETstruct, (uint8_t *)&data, sizeof(data));
}

void COMMS_addDataToOutput_32b(uint32_t data){
	// Possibly check endianness
	COMMS_helper_addToBuf(&progRETstruct, (uint8_t *)&data, sizeof(data));
}

void COMMS_commandExecutor(){
	uint32_t counter;
	uint32_t startPos = progOUTstruct.tail;
	uint32_t stopPos = (startPos + packetHelper.expectedLength) & cyclicBufferSizeMask; // I'm sure there's an off-by-1 error somewhere.

	if (packetHelper.status != status_parsed){
		return;
	}

	// Start at position 0 in the packet (reset earlier ><).
	for (counter = 0; counter < (packetHelper.expectedLength - 1); ){	// CRC ><
		uint8_t dataAtCounter;
		COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + counter, 1, &dataAtCounter);
		//uint8_t dataAtCounter = progOUTstruct.data[(progOUTstruct.tail + counter) & cyclicBufferSizeMask]; // I have regrets

		if (COMMAND_GET_INFO == dataAtCounter){
			// Similar as in INFO. Fixed at 128 bytes.
			COMMS_addInfoToOutput();
			counter = counter + 1;	// These _should_ be defines (lengths)
		}
		else if (COMMAND_SET_SPEED == dataAtCounter){
			// Currently not supported, so skip
			// Speed will be in 2 bytes (kHz)
			// TODO - functionality is here, just implement when everything else is back working.
			counter = counter + 3;
		}
		else if (COMMAND_SET_PROG_MODE == dataAtCounter){
			// The mode is in the next byte.
			uint8_t tempData[1];
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + counter + 1, 1, tempData);	// +1 ><
			transportSetup(tempData[0]);
			counter = counter + 2;
		}
		else if (COMMAND_SET_PIN_IO_MODE == dataAtCounter){
			// First byte is pin
			// Second byte is mode (input, output /w high, output / low)
			uint8_t tempData[2];
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + counter + 1, 2, tempData);	// +1 ><

			if (PIN_TMS == tempData[0]){
				GPIODrv_setupPinTMS(tempData[1]);
			}
			else if (PIN_TCK == tempData[0]){
				GPIODrv_setupPinTCK(tempData[1]);
			}
			else if (PIN_TDI == tempData[0]){
				GPIODrv_setupPinTDI(tempData[1]);
			}
			else if (PIN_TDO == tempData[0]){
				GPIODrv_setupPinTDO(tempData[1]);
			}
			else if (PIN_MCLR == tempData[0]){
				GPIODrv_setupPinMCLR(tempData[1]);
			}

			counter = counter + 3;
		}
		else if (COMMAND_SET_PIN_WRITE == dataAtCounter){
			// First byte is pin
			// Second byte is value (high, low).
			uint8_t tempData[2];
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + counter + 1, 2, tempData);	// +1 ><

			if (PIN_TMS == tempData[0]){
				GPIODrv_setStateTMS(tempData[1]);
			}
			else if (PIN_TCK == tempData[0]){
				GPIODrv_setStateTCK(tempData[1]);
			}
			else if (PIN_TDI == tempData[0]){
				GPIODrv_setStateTDI(tempData[1]);
			}
			else if (PIN_TDO == tempData[0]){
				GPIODrv_setStateTDO(tempData[1]);
			}
			else if (PIN_MCLR == tempData[0]){
				GPIODrv_setStateMCLR(tempData[1]);
			}

			counter = counter + 3;
		}
		else if (COMMAND_SET_PIN_READ == dataAtCounter){
			// First byte is pin
			uint8_t tempData[1];
			uint32_t returnVal = 0;
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + counter + 1, 1, tempData);	// +1 ><
			if (PIN_TMS == tempData[0]){
				returnVal = GPIODrv_getStateTMS();
			}
			else if (PIN_TCK == tempData[0]){
				returnVal = GPIODrv_getStateTCK();
			}
			else if (PIN_TDI == tempData[0]){
				returnVal = GPIODrv_getStateTDI();
			}
			else if (PIN_TDO == tempData[0]){
				returnVal = GPIODrv_getStateTDO();
			}
			else if (PIN_MCLR == tempData[0]){
				returnVal = GPIODrv_getStateMCLR();
			}

			// TODO implement return.
			COMMS_addDataToOutput_32b(returnVal);

			counter = counter + 2;
		}
		else if (COMMAND_SEND == dataAtCounter){
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
			//memcpy(&tms_prolog_nbits, &tempBuffer[location], sizeof(tms_prolog_nbits));
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + location, sizeof(tms_prolog_nbits), (uint8_t *)&tms_prolog_nbits);
			location = location + sizeof(tms_prolog_nbits);
			//memcpy(&tms_prolog, &tempBuffer[location], sizeof(tms_prolog));
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + location, sizeof(tms_prolog), (uint8_t *)&tms_prolog);
			location = location + sizeof(tms_prolog);

			// TDI
			//memcpy(&tdi_nbits, &tempBuffer[location], sizeof(tdi_nbits));
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + location, sizeof(tdi_nbits), (uint8_t *)&tdi_nbits);
			location = location + sizeof(tdi_nbits);
			//memcpy(&tdi, &tempBuffer[location], sizeof(tdi));
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + location, sizeof(tdi), (uint8_t *)&tdi);
			location = location + sizeof(tdi);

			// TMS epilog
			//memcpy(&tms_epilog_nbits, &tempBuffer[location], sizeof(tms_epilog_nbits));
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + location, sizeof(tms_epilog_nbits), (uint8_t *)&tms_epilog_nbits);
			location = location + sizeof(tms_epilog_nbits);
			//memcpy(&tms_epilog, &tempBuffer[location], sizeof(tms_epilog));
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + location, sizeof(tms_epilog), (uint8_t *)&tms_epilog);
			location = location + sizeof(tms_epilog);

			// read flag
			//memcpy(&read_flag, &tempBuffer[location], sizeof(read_flag));
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + location, sizeof(read_flag), (uint8_t *)&read_flag);
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
			}
		}
		else if (COMMAND_XFER_INSTRUCTION == dataAtCounter){

			uint32_t maxCounter = 0;
			uint32_t maxLimit = 40;	// Just a define

			uint32_t controlVal = 0;
			uint32_t instruction = 0;

			// instruction is at counter+1
			//memcpy(&instruction, &tempBuffer[counter+1], sizeof(instruction));
			COMMS_helper_peekData(progOUTstruct.data, progOUTstruct.tail + counter + 1, sizeof(instruction), (uint8_t *)&instruction);
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
				COMMS_addDataToOutput_64b(0x8000000000000000);	// -1 in case of FAIL.
			}

		}
		else if (COMMAND_GET_PE_RESPONSE == dataAtCounter){

			uint32_t maxCounter = 0;
			uint32_t maxLimit = 40;	// Just a define

			uint32_t response = 0;
			uint32_t controlVal = 0;

			uint32_t tempStart;
			uint32_t delayTime = 20 * 40;	// in us

			counter = counter + 1;

			COMMS_pic32SendCommand(ETAP_CONTROL);

			do {
				tempStart = _CP0_GET_COUNT();
				while((_CP0_GET_COUNT() - tempStart) < delayTime);
				// There needs to be a minimum _7us_ delay between ETAP_CONTROL and here.
				// You know, actually give the PE time to respond.
				// (The number is probably a bit bigger, overhead and things)

			    controlVal = (uint32_t)COMMS_pic32XferData(32, (CONTROL_PRACC | CONTROL_PROBEN | CONTROL_PROBTRAP | CONTROL_EJTAGBRK), 1);
			    if (!(controlVal & CONTROL_PRACC)){
					// Note - xfer instruction, ctl was %08x\n
					maxCounter++;
					if (maxCounter >= maxLimit){
						// Processor still not ready :/, give up.
						break;
					}
				}
			} while (! (controlVal & CONTROL_PRACC));


			if (maxCounter < maxLimit){
				COMMS_pic32SendCommand(ETAP_DATA);    // ETAP_DATA

				response = COMMS_pic32XferData(32, 0, 1);	// Send 32 zeroes, read response
				COMMS_addDataToOutput_64b(response);

				COMMS_pic32SendCommand(ETAP_CONTROL); // ETAP_CONTROL
				COMMS_pic32XferData(32, (CONTROL_PROBEN | CONTROL_PROBTRAP), 0);   // Send data, don't read
			}
			else{
				COMMS_addDataToOutput_64b(0x8000000000000000);	// -1 in case of FAIL.
			}

		}

		else{
			asm("nop");
		}

	}

	// Update where we are...
	progOUTstruct.tail = stopPos;

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


void COMMS_reinitPacketHelper(dataDecoder * st){
	st->currentPos = 0;
	st->expectedLength = 0;
	st->type = 0;
	st->crc = 0;
	st->status = 0;
}




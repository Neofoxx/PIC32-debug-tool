#include <p32xxxx.h>
#include <transport.h>
#include <inttypes.h>
#include <GPIODrv.h>

// The functions that we need to implement
/*	- setup_protocol (JTAG, ICSP, ?)
 * 	- teardown (go to high impedance, disconnect, ?)
 * 	- send_jtag
 * 	- send_icsp
 */

#if defined(__32MX440F256H__)

uint32_t transport_currentMode = 0;

void transportSetup(uint32_t mode){
	if (TRANSPORT_MODE_JTAG == mode){
		// All pinout output except TDO
		GPIODrv_setupPinTMS(GPIO_mode_output_low);
		GPIODrv_setupPinTCK(GPIO_mode_output_low);
		GPIODrv_setupPinTDI(GPIO_mode_output_low);
		GPIODrv_setupPinTDO(GPIO_mode_input);
		GPIODrv_setupPinMCLR(GPIO_mode_output_high);
		transport_currentMode = mode;
	}
	else if(TRANSPORT_MODE_ICSP == mode){
		// ICSP works on TMS and TCK pins. But still init TDO and TDI.
		GPIODrv_setupPinTMS(GPIO_mode_output_low);	// PGED
		GPIODrv_setupPinTCK(GPIO_mode_output_low);	// PGEC
		GPIODrv_setupPinTDI(GPIO_mode_input);
		GPIODrv_setupPinTDO(GPIO_mode_input);
		GPIODrv_setupPinMCLR(GPIO_mode_output_high);
		transport_currentMode = mode;
	}
	else if(TRANSPORT_MODE_TRISTATE == mode){
		// Tri-state all pins, disconnect from bus
		GPIODrv_setupPinTMS(GPIO_mode_input);	// PGED
		GPIODrv_setupPinTCK(GPIO_mode_input);	// PGEC
		GPIODrv_setupPinTDI(GPIO_mode_input);
		GPIODrv_setupPinTDO(GPIO_mode_input);
		GPIODrv_setupPinMCLR(GPIO_mode_input);
		transport_currentMode = mode;
	}

}

/////////////////////////////////////////////////////////
// Send commands. Modeled after mpsse_send in pic32prog
// Can return a value. Max 33 bits, because of PRACC bit.
/////////////////////////////////////////////////////////


uint64_t transportSendJTAG(	uint32_t tms_header_nbits, uint32_t tms_header_val,
							uint32_t data_nbits, uint64_t data_val,
							uint32_t tms_epilogue_nbits, uint32_t tms_epilogue_val){

	// All data is output (and received!) LSB first

	volatile uint64_t returnVal = 0;
	volatile uint32_t counterOfBits = 0;

	// TMS header
	for (;tms_header_nbits>0; tms_header_nbits--){
		GPIODrv_setStateTMS((tms_header_val & 0x01) ? GPIO_HIGH : GPIO_LOW);
		GPIODrv_setStateTDI(GPIO_LOW);
		GPIODrv_setStateTCK(GPIO_HIGH);
		// Nops?
		GPIODrv_setStateTCK(GPIO_LOW);
		tms_header_val = tms_header_val >> 1;	// Remove the sent bit
	}

	GPIODrv_setStateTMS(GPIO_LOW);

	// Data. Special case for last bit - shares with TMS = 1!
	for (;data_nbits>0; data_nbits--){
		GPIODrv_setStateTDI((data_val & 0x01) ? GPIO_HIGH : GPIO_LOW);
		if (data_nbits == 1){
			GPIODrv_setStateTMS(GPIO_HIGH);
		}
		GPIODrv_setStateTCK(GPIO_HIGH);
		// Nops?
		returnVal = (returnVal >> 1) | (GPIODrv_getStateTDO() ? ((uint64_t)1<<63): 0);
		counterOfBits++;
		GPIODrv_setStateTCK(GPIO_LOW);
		data_val = data_val >> 1;				// Remove the sent bit
	}

	// TMS epilogue. Has one bit less, because of TMS bit in code above!
	for (;tms_epilogue_nbits; tms_epilogue_nbits--){
		GPIODrv_setStateTDI(GPIO_LOW);
		GPIODrv_setStateTMS((tms_epilogue_val & 0x01) ? GPIO_HIGH : GPIO_LOW);
		GPIODrv_setStateTCK(GPIO_HIGH);
		tms_epilogue_val = tms_epilogue_val >> 1;				// Remove the sent bit
		GPIODrv_setStateTCK(GPIO_LOW);
	}

	// Align LSB to 0 bit. For PrACC bit, shift >> 1 to remove it.
	returnVal = returnVal >> (64 - counterOfBits);

	return returnVal;
}

uint64_t transportSendICSP(	uint32_t tms_header_nbits, uint32_t tms_header_val,
							uint32_t data_nbits, uint64_t data_val,
							uint32_t tms_epilogue_nbits, uint32_t tms_epilogue_val){

	// All data is output (and received!) LSB first
	// Remember - communication on TMS and TCK only

	volatile uint64_t returnVal = 0;
	volatile uint32_t counterOfBits = 0;

	if (tms_header_val == 0x1F){
		asm("nop");
	}

	// TMS header
	for (;tms_header_nbits>0; tms_header_nbits--){

		// Send TDI
		GPIODrv_setStateTCK(GPIO_HIGH);
		GPIODrv_setStateTMS(GPIO_LOW);	// Send TDI bit = 0
		GPIODrv_setStateTCK(GPIO_LOW);

		// Send TMS
		GPIODrv_setStateTCK(GPIO_HIGH);
		GPIODrv_setStateTMS((tms_header_val & 0x01) ? GPIO_HIGH : GPIO_LOW);
		GPIODrv_setStateTCK(GPIO_LOW);

		// Change output to  input
		GPIODrv_setStateTCK(GPIO_HIGH);
		GPIODrv_setupPinTMS(GPIO_mode_input);
		GPIODrv_setStateTCK(GPIO_LOW);

		// Read TDO bit
		GPIODrv_setStateTCK(GPIO_HIGH);
		if (tms_header_nbits == 1){
			returnVal = (returnVal >> 1) | (GPIODrv_getStateTMS() ? ((uint64_t)1<<63): 0);
			counterOfBits++;
		}
		GPIODrv_setStateTCK(GPIO_LOW);

		// Reset TMS direction
		GPIODrv_setupPinTMS(GPIO_mode_output_low);

		// Remove the sent bit
		tms_header_val = tms_header_val >> 1;	// Remove the sent bit
	}

	// Data. Special case for last bit - shares with TMS = 1!
	for (;data_nbits>0; data_nbits--){

		// Send TDI
		GPIODrv_setStateTCK(GPIO_HIGH);
		GPIODrv_setStateTMS((data_val & 0x01) ? GPIO_HIGH : GPIO_LOW);	// Send TDI bit
		GPIODrv_setStateTCK(GPIO_LOW);

		// Send TMS
		GPIODrv_setStateTCK(GPIO_HIGH);
		if (data_nbits == 1){
			GPIODrv_setStateTMS(GPIO_HIGH); // Special case, send 1
		}
		else{
			GPIODrv_setStateTMS(GPIO_LOW);	// Else, send 0
		}
		GPIODrv_setStateTCK(GPIO_LOW);

		// Change output to  input
		GPIODrv_setStateTCK(GPIO_HIGH);
		GPIODrv_setupPinTMS(GPIO_mode_input);
		GPIODrv_setStateTCK(GPIO_LOW);


		// Read TDO bit. Don't read on last output bit, special case
		GPIODrv_setStateTCK(GPIO_HIGH);
		if (data_nbits > 1){
			returnVal = (returnVal >> 1) | (GPIODrv_getStateTMS() ? ((uint64_t)1<<63): 0);
			counterOfBits++;
		}
		GPIODrv_setStateTCK(GPIO_LOW);

		// Reset TMS direction
		GPIODrv_setupPinTMS(GPIO_mode_output_low);

		// Remove the sent bit
		data_val = data_val >> 1;	// Remove the sent bit
	}

	// TMS epilogue. Has one bit less, because of TMS bit in code above!
	for (;tms_epilogue_nbits>0; tms_epilogue_nbits--){

		// Send TDI
		GPIODrv_setStateTCK(GPIO_HIGH);
		GPIODrv_setStateTMS(GPIO_LOW);	// Send TDI bit = 0
		GPIODrv_setStateTCK(GPIO_LOW);

		// Send TMS
		GPIODrv_setStateTCK(GPIO_HIGH);
		GPIODrv_setStateTMS((tms_epilogue_val & 0x01) ? GPIO_HIGH : GPIO_LOW);
		GPIODrv_setStateTCK(GPIO_LOW);

		// Change output to  input
		GPIODrv_setStateTCK(GPIO_HIGH);
		GPIODrv_setupPinTMS(GPIO_mode_input);
		GPIODrv_setStateTCK(GPIO_LOW);

		GPIODrv_setStateTCK(GPIO_HIGH);
		tms_epilogue_val = tms_epilogue_val >> 1;				// Remove the sent bit
		GPIODrv_setStateTCK(GPIO_LOW);

		// Reset TMS direction
		GPIODrv_setupPinTMS(GPIO_mode_output_low);	// Grr.
	}


	returnVal = returnVal >> (64 - counterOfBits);

	return returnVal;
}




#endif

#include <p32xxxx.h>
#include <GPIODrv.h>
#include <inttypes.h>
#include <forNewlib.h>

void GPIODrv_init(){
	GPIODrv_setupPinTMS(GPIO_mode_input);
	GPIODrv_setupPinTCK(GPIO_mode_input);
	GPIODrv_setupPinTDI(GPIO_mode_input);
	GPIODrv_setupPinTDO(GPIO_mode_input);
	GPIODrv_setupPinMCLR(GPIO_mode_input);



}

////////////
// Pin setup
////////////

void GPIODrv_setupPinTMS(uint32_t mode){
	if (mode == GPIO_mode_input){
		PROG_TMS_TRISbits.PROG_TMS_TRISPIN = 1;	// 1 is input
	}
	else if (mode == GPIO_mode_output_high){
		GPIODrv_setStateTMS(GPIO_HIGH);			// Preset state
		PROG_TMS_TRISbits.PROG_TMS_TRISPIN = 0;	// 0 is output
	}
	else if (mode == GPIO_mode_output_low){
		GPIODrv_setStateTMS(GPIO_LOW);			// Preset state
		PROG_TMS_TRISbits.PROG_TMS_TRISPIN = 0;	// 0 is output
	}
}

void GPIODrv_setupPinTCK(uint32_t mode){
	if (mode == GPIO_mode_input){
		PROG_TCK_TRISbits.PROG_TCK_TRISPIN = 1;	// 1 is input
	}
	else if (mode == GPIO_mode_output_high){
		PROG_TCK_LATSET = PROG_TCK_MASK;		// Preset state
		PROG_TCK_TRISbits.PROG_TCK_TRISPIN = 0;	// 0 is output
	}
	else if (mode == GPIO_mode_output_low){
		PROG_TCK_LATCLR = PROG_TCK_MASK;		// Preset state
		PROG_TCK_TRISbits.PROG_TCK_TRISPIN = 0;	// 0 is output
	}
}

void GPIODrv_setupPinTDI(uint32_t mode){
	if (mode == GPIO_mode_input){
		PROG_TDI_TRISbits.PROG_TDI_TRISPIN = 1;	// 1 is input
	}
	else if (mode == GPIO_mode_output_high){
		PROG_TDI_LATSET = PROG_TDI_MASK;		// Preset state
		PROG_TDI_TRISbits.PROG_TDI_TRISPIN = 0;	// 0 is output
	}
	else if (mode == GPIO_mode_output_low){
		PROG_TDI_LATCLR = PROG_TDI_MASK;		// Preset state
		PROG_TDI_TRISbits.PROG_TDI_TRISPIN = 0;	// 0 is output
	}
}

void GPIODrv_setupPinTDO(uint32_t mode){
	if (mode == GPIO_mode_input){
		PROG_TDO_TRISbits.PROG_TDO_TRISPIN = 1;	// 1 is input
	}
	else if (mode == GPIO_mode_output_high){
		PROG_TDO_LATSET = PROG_TDO_MASK;		// Preset state
		PROG_TDO_TRISbits.PROG_TDO_TRISPIN = 0;	// 0 is output
	}
	else if (mode == GPIO_mode_output_low){
		PROG_TDO_LATCLR = PROG_TDO_MASK;		// Preset state
		PROG_TDO_TRISbits.PROG_TDO_TRISPIN = 0;	// 0 is output
	}
}

void GPIODrv_setupPinMCLR(uint32_t mode){
	if (mode == GPIO_mode_input){
		PROG_MCLR_TRISbits.PROG_MCLR_TRISPIN = 1;	// 1 is input
	}
	else if (mode == GPIO_mode_output_high){
		PROG_MCLR_LATSET = PROG_MCLR_MASK;			// Preset state
		PROG_MCLR_TRISbits.PROG_MCLR_TRISPIN = 0;	// 0 is output
	}
	else if (mode == GPIO_mode_output_low){
		PROG_MCLR_LATCLR = PROG_MCLR_MASK;			// Preset state
		PROG_MCLR_TRISbits.PROG_MCLR_TRISPIN = 0;	// 0 is output
	}
}

//////////////////////////////////////////////////////////////////////
// Setting states of pins.
// Inefficient, but not a problem for now. Let the complier handle it.
//////////////////////////////////////////////////////////////////////

void GPIODrv_setStateTMS(uint32_t state){
	if (state == GPIO_HIGH){
		PROG_TMS_LATSET = PROG_TMS_MASK;
	}
	else{
		PROG_TMS_LATCLR = PROG_TMS_MASK;
	}
}


void GPIODrv_setStateTCK(uint32_t state){
	if (state == GPIO_HIGH){
		PROG_TCK_LATSET = PROG_TCK_MASK;
	}
	else{
		PROG_TCK_LATCLR = PROG_TCK_MASK;
	}
}


void GPIODrv_setStateTDI(uint32_t state){
	if (state == GPIO_HIGH){
		PROG_TDI_LATSET = PROG_TDI_MASK;
	}
	else{
		PROG_TDI_LATCLR = PROG_TDI_MASK;
	}
}

void GPIODrv_setStateTDO(uint32_t state){
	if (state == GPIO_HIGH){
		PROG_TDO_LATSET = PROG_TDO_MASK;
	}
	else{
		PROG_TDO_LATCLR = PROG_TDO_MASK;
	}
}

void GPIODrv_setStateMCLR(uint32_t state){
	// For MCLR, maybe add an inverse option?
	if (state == GPIO_HIGH){
		PROG_MCLR_LATSET = PROG_MCLR_MASK;
	}
	else{
		PROG_MCLR_LATCLR = PROG_MCLR_MASK;
	}
}

///////////////////
// Read input state
///////////////////
uint32_t GPIODrv_getStateTMS(){
	if (PROG_TMS_PORTbits.PROG_TMS_PORTPIN){
		return 1;
	}
	return 0;
}

uint32_t GPIODrv_getStateTCK(){
	if (PROG_TCK_PORTbits.PROG_TCK_PORTPIN){
		return 1;
	}
	return 0;
}

uint32_t GPIODrv_getStateTDI(){
	if (PROG_TDI_PORTbits.PROG_TDI_PORTPIN){
		return 1;
	}
	return 0;
}

uint32_t GPIODrv_getStateTDO(){
	if (PROG_TDO_PORTbits.PROG_TDO_PORTPIN){
		return 1;
	}
	return 0;
}

uint32_t GPIODrv_getStateMCLR(){
	if (PROG_MCLR_PORTbits.PROG_MCLR_PORTPIN){
		return 1;
	}
	return 0;
}

//////////////////////////////////////////////////////////


// UART things
void GPIODrv_setRTS(uint32_t state){
	if (state == GPIO_HIGH){
		UART_RTS_LATSET = UART_RTS_MASK;
	}
	else{
		UART_RTS_LATCLR = UART_RTS_MASK;
	}
}

void GPIODrv_setDTR(uint32_t state){
	if (state == GPIO_HIGH){
		UART_DTR_LATSET = UART_DTR_MASK;
	}
	else{
		UART_DTR_LATCLR = UART_DTR_MASK;
	}
}

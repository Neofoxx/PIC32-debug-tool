#ifndef TRANSPORT_H_1e6ef42163ef4b848869ee2079a96a87
#define TRANSPORT_H_1e6ef42163ef4b848869ee2079a96a87

#include <inttypes.h>

// Generic header for the transport layer.
// Selects the appropriate one, depending on the ifdefs

#define TRANSPORT_MODE_TRISTATE 0
#define TRANSPORT_MODE_JTAG 1
#define TRANSPORT_MODE_ICSP 2



// Copied from pic32prog
#define COMMAND_GET_INFO		0
#define COMMAND_SET_SPEED		1
#define COMMAND_SET_PROG_MODE	2
#define COMMAND_SET_PIN_IO_MODE	3	// Input/output. These three are useful for custom stuff
#define COMMAND_SET_PIN_WRITE	4	// Write to pin
#define COMMAND_SET_PIN_READ	5	// Read pin value
#define COMMAND_SEND			6	// Send command
#define COMMAND_XFER_INSTRUCTION	7	// xferInstruction, but performed on MCU
#define COMMAND_GET_PE_RESPONSE		8	// get_pe_response, but performed on MCU


#define PROG_MODE_TRISTATE	0
#define PROG_MODE_JTAG		1
#define PROG_MODE_ICSP		2

#define PIN_TMS		0
#define PIN_TCK		1
#define PIN_TDI		2
#define PIN_TDO		3
#define PIN_MCLR	4

#define PIN_DIR_OUTPUT	0
#define PIN_DIR_INPUT	1
#define PIN_VAL_LOW		0
#define PIN_VAL_HIGH	1

// The above are condensed into these.
#define PIN_SET_OUTPUT_LOW	0
#define PIN_SET_OUTPUT_HIGH	1
#define PIN_SET_INPUT		2
// End.

extern uint32_t transport_currentMode;

#if defined(__32MX270F256D__)
#include <transport_dual_spi.h>
#elif defined(__32MX440F256H__)
#include <transport_bitbang_gpio.h>
#endif



#endif

#include <p32xxxx.h>
#include <inttypes.h>
#include <configBits.h>

#if defined (__32MX270F256D__)
	const uint32_t __attribute__((section (".SECTION_DEVCFG3"))) temp3 =
		0xFFF0000
		| (0b0 << _DEVCFG3_FVBUSONIO_POSITION)	// USBVBUSON controlled by PORT function
		| (0b0 << _DEVCFG3_FUSBIDIO_POSITION)	// USBID controlled by PORT function
		| (0b0 << _DEVCFG3_IOL1WAY_POSITION)		// Allow multiple reconfigurations of Peripheral Pins
		| (0b0 << _DEVCFG3_PMDL1WAY_POSITION)	// Allow multiple reconfigurations of Peripheral Module Disable
		| (0xF0C5 << _DEVCFG3_USERID_POSITION);	// UserID is F0C5

	const uint32_t __attribute__((section (".SECTION_DEVCFG2"))) temp2 =
		0xFFF87888
		| (0b001 << _DEVCFG2_FPLLODIV_POSITION)	// PLL output divided by 2 (96MHz/2 = 48Mhz)
		| (0b0 << _DEVCFG2_UPLLEN_POSITION)		// USB PLL Enabled
		| (0b001 << _DEVCFG2_UPLLIDIV_POSITION)	// USB PLL input divided by 2 (8MHz/2 = 4MHz)
		| (0b111 << _DEVCFG2_FPLLMUL_POSITION)	// PLL Multiplier is 24 (4MHz*24 = 96MHz)
		| (0b001 << _DEVCFG2_FPLLIDIV_POSITION);	// PLL input divided by 2 (8MHz/2 = 4MHz)

	const uint32_t __attribute__((section (".SECTION_DEVCFG1"))) temp1 =
		0xFC200858
		| (0b00 << _DEVCFG1_FWDTWINSZ_POSITION)	// Watchdog timer window size is 75%
		| (0b0 << _DEVCFG1_FWDTEN_POSITION)		// Watchdog timer disabled, can be enabled in software
		| (0b00000 << _DEVCFG1_WDTPS_POSITION)	// Watchdog timer postscale is 1
		| (0b01 << _DEVCFG1_FCKSM_POSITION)		// Clock switching enabled, Fail-Safe Clock Monitoring DISABLED
		| (0b00 << _DEVCFG1_FPBDIV_POSITION)	// PBCLK is SYSCLK / 1
		| (0b1 << _DEVCFG1_OSCIOFNC_POSITION)	// CLOCK output disabled
		| (0b01 << _DEVCFG1_POSCMOD_POSITION)	// XT oscillator mode
		| (0b0 << _DEVCFG1_IESO_POSITION)		// Internal-External switchover disabled (Two-speed start-up disabled)
		| (0b1 << _DEVCFG1_FSOSCEN_POSITION)		// Enable secondary oscillator (WARNING; CHECK IF PORTING)
		| (0b011 << _DEVCFG1_FNOSC_POSITION);		// POSC (XT) + PLL selected

	const uint32_t __attribute__((section (".SECTION_DEVCFG0"))) temp0 =
		0x6EF803E0								// Don't forget about that one 0
		| (0b1 << _DEVCFG0_CP_POSITION)			// Code Protection disabled
		| (0b1 << _DEVCFG0_BWP_POSITION)			// Boot Flash is Writeable during code execution
		| (0b1111111111 << _DEVCFG0_PWP_POSITION)	// Memory is NOT write-protected
		| (0b00 << _DEVCFG0_ICESEL_POSITION)		// PGEC4/PGED4 is used
		| (0b1 << _DEVCFG0_JTAGEN_POSITION)		// JTAG is enabled
	#ifdef DEBUG_BUILD							// Defined with Makefile
		| (0b11<<_DEVCFG0_DEBUG_POSITION);		// Debugger is DISABLED. Apparently the MX1/MX2 family need this disabled, for JTAG to work
												// Note, application will run automatically, might want to add a delay at the beginning.
	#else
		| (0b11<<_DEVCFG0_DEBUG_POSITION);		// Debugger is DISABLED (DEBUG bit) - DEBUG NEEDS TO BE DISABLED, IF NO DEBUGGER PRESENT! Otherwise code doesn't run.
	#endif

#elif defined (__32MX440F256H__)

	const uint32_t __attribute__((section (".SECTION_DEVCFG3"))) temp3 = 0xFFFFBEEF;   // DEVCFG3
	const uint32_t __attribute__((section (".SECTION_DEVCFG2"))) temp2 =
		0xFFF87888
		| (0b000<<_DEVCFG2_FPLLODIV_POSITION)
		| (0b0<<_DEVCFG2_UPLLEN_POSITION)
		| (0b001<<_DEVCFG2_UPLLIDIV_POSITION)
		| (0b101<<_DEVCFG2_FPLLMUL_POSITION)
		| (0b001<<_DEVCFG2_FPLLIDIV_POSITION);   // DEVCFG2
	const uint32_t __attribute__((section (".SECTION_DEVCFG1"))) temp1 =
		0xFF600858
		| (0b10100<<_DEVCFG1_WDTPS_POSITION)
		| (0b01<<_DEVCFG1_FPBDIV_POSITION)
		| (0b0<<_DEVCFG1_OSCIOFNC_POSITION)
		| (0b10<<_DEVCFG1_POSCMOD_POSITION)
		| (0b0<<_DEVCFG1_IESO_POSITION)	//?
		| (0b0<<_DEVCFG1_FSOSCEN_POSITION)
		| (0b011<<_DEVCFG1_FNOSC_POSITION);   // DEVCFG1
	const uint32_t __attribute__((section (".SECTION_DEVCFG0"))) temp0 =
		0x6EF00FF4 // There's that _one_ bit that has to be 0
		| (0b1<<_DEVCFG0_CP_POSITION)
		| (0b1<<_DEVCFG0_BWP_POSITION)
		| (0b11111111<<_DEVCFG0_PWP_POSITION)
		| (0b1<<_DEVCFG0_ICESEL_POSITION)	// PGEC2/PGED2
	#ifdef DEBUG_BUILD	// Defined with Makefile
		| (0b10<<_DEVCFG0_DEBUG_POSITION);   // DEVCFG0
	#else
		| (0b11<<_DEVCFG0_DEBUG_POSITION);   // DEVCFG0	- DEBUG NEEDS TO BE DISABLED, IF NO DEBUGGER PRESENT! Otherwise code doesn't run.
	#endif

#endif




// Information about the device
const infoStruct info = {
		"MCU: MX440F256H\n",
		"MODE: Bitbang\n",
		"NAME: Neofoxx PIC32-debug-tool\n",
		"REV: 1.1\n"	// MAJOR number . MINOR number.
						// MAJOR represents the API,
						// MINOR represents internal changes not visible to outside
};

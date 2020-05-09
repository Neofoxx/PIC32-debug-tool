/*	----------------------------------------------------------------------------
    FILE:			system.c
    PROJECT:		pinguino
    PURPOSE:		
    PROGRAMER:		Régis Blanchot <rblanchot@gmail.com>
    FIRST RELEASE:	16 nov. 2010
    LAST RELEASE:	06 feb. 2012
    ----------------------------------------------------------------------------
    CHANGELOG:
    [23-02-11][Marcus Fazzi][Removed  asm("di")/asm("ei") from GetCP0Count/SetCP0Count]
    [30-01-12][Régis Blanchot][Added P32MX220F032D support]
    [06-02-12][Roland Haag][Added new clock settings]
    [13-05-12][Jean-Pierre Mandon][added P32MX250F128B and P32MX220F032B support]
    ----------------------------------------------------------------------------
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    --------------------------------------------------------------------------*/



#include <system.h>
// Already in system.h
//#include <p32xxxx.h>
//#include <const.h>              // MIPS32
//#include <inttypes.h>

// Hm... Do defines by boards first (since you know which chips are there specifically)
// Then by chip.
#if defined(PIC32_PINGUINO_220)||defined(PINGUINO32MX250)||defined(PINGUINO32MX220)
    #define CPUCOREMAXFREQUENCY	40000000L	// 40 MHz
#elif defined(PINGUINO32MX270)
    #define CPUCOREMAXFREQUENCY	64000000L	// 64 MHz (overclock)
#elif defined(__32MX250F128B__)
    #define CPUCOREMAXFREQUENCY 50000000L   // 50 MHz
#else
    #define CPUCOREMAXFREQUENCY	80000000L	// 80 MHz
#endif

#ifndef CRYSTALFREQUENCY
#define CRYSTALFREQUENCY	8000000L	// 8 MHz    -> Support an outside parameter.
#endif
#define FLASHACCESSTIME		50			// 50 ns
#define PLL_Fin_MIN			4000000L	// 4MHz	
#define PLL_Fin_MAX			5000000L	// 5MHz PLL input MUST BE between 4 and 5 MHz!

// COSC<2:0>: Current Oscillator Selection bits
// NOSC<2:0>: New Oscillator Selection bits
#define FIRCOSCDIV		0b111 // Fast Internal RC Oscillator divided by OSCCON<FRCDIV> bits
#define FIRCOSCDIV16	0b110 // Fast Internal RC Oscillator divided by 16
#define LPIRCOSC		0b101 // Low-Power Internal RC Oscillator (LPRC)
#define SOSC			0b100 // Secondary Oscillator (SOSC)
#define POSCPLL			0b011 // Primary Oscillator with PLL module (XTPLL, HSPLL or ECPLL)
#define POSC			0b010 // Primary Oscillator (XT, HS or EC)
#define FRCOSCPLL		0b001 // Fast RC Oscillator with PLL module via Postscaler (FRCPLL)
#define FRCOSC			0b000 // Fast RC Oscillator (FRC)

// PLLODIV<2:0>: Output Divider for PLL
#define PLLODIV256		0b111 // PLL output divided by 256
#define PLLODIV64		0b110 // PLL output divided by 64
#define PLLODIV32		0b101 // PLL output divided by 32
#define PLLODIV16		0b100 // PLL output divided by 16
#define PLLODIV8		0b011 // PLL output divided by 8
#define PLLODIV4		0b010 // PLL output divided by 4
#define PLLODIV2		0b001 // PLL output divided by 2
#define PLLODIV1		0b000 // PLL output divided by 1

// FRCDIV<2:0>: Fast Internal RC Clock Divider bits
#define FRCDIV256		0b111 // FRC divided by 256
#define FRCDIV64		0b110 // FRC divided by 64
#define FRCDIV32		0b101 // FRC divided by 32
#define FRCDIV16		0b100 // FRC divided by 16
#define FRCDIV8			0b011 // FRC divided by 8
#define FRCDIV4			0b010 // FRC divided by 4
#define FRCDIV2			0b001 // FRC divided by 2 (default setting)
#define FRCDIV1			0b000 // FRC divided by 1

// PBDIV<1:0>: Peripheral Bus Clock Divisor
#define PBDIV8			0b11 // PBCLK is SYSCLK divided by 8 (default)
#define PBDIV4			0b10 // PBCLK is SYSCLK divided by 4
#define PBDIV2			0b01 // PBCLK is SYSCLK divided by 2
#define PBDIV1			0b00 // PBCLK is SYSCLK divided by 1

// PLLMULT<2:0>: PLL Multiplier bits
#define PLLMULT24		0b111 // Clock is multiplied by 24
#define PLLMULT21		0b110 // Clock is multiplied by 21
#define PLLMULT20		0b101 // Clock is multiplied by 20
#define PLLMULT19		0b100 // Clock is multiplied by 19
#define PLLMULT18		0b011 // Clock is multiplied by 18
#define PLLMULT17		0b010 // Clock is multiplied by 17
#define PLLMULT16		0b001 // Clock is multiplied by 16
#define PLLMULT15		0b000 // Clock is multiplied by 15


uint32_t ticksPerUs = 0;		// How many ticks of CP0 per 1us
uint32_t ticksPer100ns = 0;		// How many ticks of CP0 per 100ns



/**
 * Implementation notes
 *
 * This implementation only handles the PLL setup with "PLL input
 * divider", "PLL output divider", "PLL multiplier", and "Peripheral
 * clock divider". Other clocking schemes are not supported.
 *
 * See @page 186, PIC32MX Family Reference Manual, PIC32MX Family
 * Clock Diagram
 */

/** The indices are valid values for PLLIDIV */
const uint32_t pllInputDivs[]  = {  1,  2,  3,  4,  5,  6, 10,  12 };

/** The indices are valid values for PLLODIV */
const uint32_t pllOutputDivs[] = {  1,  2,  4,  8, 16, 32, 64, 256 };

/** The indices are valid values for PLLMULT */
const uint32_t pllMuls[]       = { 15, 16, 17, 18, 19, 20, 21,  24 };

/** The indices are valid values for PBDIV */
const uint32_t pbDivs[]        = { 1, 2, 4, 8 };

const uint8_t pllInputDivsCount = 
  sizeof(pllInputDivs) / sizeof(pllInputDivs[0]);

const uint8_t pllOutputDivsCount = 
  sizeof(pllOutputDivs) / sizeof(pllOutputDivs[0]);

const uint8_t pllMulsCount = 
  sizeof(pllMuls) / sizeof(pllMuls[0]);

const uint8_t pbDivsCount = 
  sizeof(pbDivs) / sizeof(pbDivs[0]);

/*	----------------------------------------------------------------------------
    SystemUnlock() perform a system unlock sequence
    --------------------------------------------------------------------------*/

void SystemUnlock()
{
    SYSKEY = 0;				// ensure OSCCON is locked
    SYSKEY = 0xAA996655;	// Write Key1 to SYSKEY
    SYSKEY = 0x556699AA;	// Write Key2 to SYSKEY
}

/*	----------------------------------------------------------------------------
    SystemLock() relock OSCCON by relocking the SYSKEY
    --------------------------------------------------------------------------*/

void SystemLock()
{
    SYSKEY = 0x12345678;	// Write any value other than Key1 or Key2
}

/*	----------------------------------------------------------------------------
    Software Reset
    ----------------------------------------------------------------------------
    assume interrupts are disabled
    assume the DMA controller is suspended
    assume the device is locked
    --------------------------------------------------------------------------*/

void SystemReset()
{
    uint16_t dummy;

    SystemUnlock();
    // set SWRST bit to arm reset
    RSWRSTSET = 1;
    // read RSWRST register to trigger reset
    dummy = RSWRST;
    // prevent any unwanted code execution until reset occurs
    while(1);
}


/*	----------------------------------------------------------------------------
    Sleep mode
    --------------------------------------------------------------------------*/


void SystemSleep()
{
    //uint16_t dummy;

    OSCCONSET = 1<<4; // Enable sleep mode after wait instruction

    SystemUnlock();
    asm volatile ("wait");
    SystemLock();
}


/*	----------------------------------------------------------------------------
    Idle mode
    --------------------------------------------------------------------------*/

void SystemIdle()
{
    //uint16_t dummy;

    OSCCONCLR = 1<<4; // Enable idle mode after wait instruction

    SystemUnlock();
    asm volatile ("wait");
    SystemLock();
}



/**
 * Read in all relevant clock settings
 *
 * @see system.c::GetSystemClock()
 *
 * @return Nothing (the result is put into the struct)
 */
void SystemClocksReadSettings(SystemClocksSettings *s) 
{
  s->PLLIDIV = DEVCFG2bits.FPLLIDIV;
  
  s->PLLODIV = OSCCONbits.PLLODIV;
  s->PLLMULT = OSCCONbits.PLLMULT;

  s->PBDIV   = OSCCONbits.PBDIV;
}

/**
 * Calculates the CPU frequency based on the values which are passed in.
 * This normally requires a call to SystemClocksReadSettings() before.
 *
 * @see system.c::GetSystemClock()
 *
 * @return The CPU frequency
 */
uint32_t SystemClocksGetCpuFrequency(const SystemClocksSettings *s) 
{
  return 
    CRYSTALFREQUENCY 
    / pllInputDivs [s->PLLIDIV]
    / pllOutputDivs[s->PLLODIV] 
    * pllMuls      [s->PLLMULT];
}

/**
 * Calculates the peripheral clock frequency based on the values which
 * are passed in. This normally requires a call to
 * SystemClocksReadSettings() before.
 *
 * @see system.c::GetPeripheralClock()
 *
 * @return The peripheral clock frequency
 */
uint32_t SystemClocksGetPeripheralFrequency(const SystemClocksSettings *s) 
{
  return SystemClocksGetCpuFrequency(s) / pbDivs[s->PBDIV];
}

/**
 * Calculate the necessary values in order to have the CPU running at
 * the desired frequency.
 *
 * @see system.c::SystemConfig()
 *
 * @return Nothing (the result is put into the struct)
 */
void SystemClocksCalcCpuClockSettings(SystemClocksSettings *s, 
                      uint32_t cpuFrequency) 
{
    if (cpuFrequency <= CPUCOREMAXFREQUENCY) 
    {
      #ifdef EXTENDED_TEST_CASE
        s->error = Error_None;
      #endif
    } 
    else 
    {
      cpuFrequency = CPUCOREMAXFREQUENCY;

      #ifdef EXTENDED_TEST_CASE
        s->error = Error_RequestedFrequencyTooHigh;
      #endif
    }

	uint8_t pllInputDivIndex;
	for (pllInputDivIndex = 0; pllInputDivIndex < pllInputDivsCount; pllInputDivIndex++){
		if (	((CRYSTALFREQUENCY / pllInputDivs[pllInputDivIndex]) >= PLL_Fin_MIN)
			&&	((CRYSTALFREQUENCY / pllInputDivs[pllInputDivIndex]) <= PLL_Fin_MAX)){
			break;
		}
	}
	if (pllInputDivIndex >= pllInputDivsCount){
		// Error, Crystal unsuitable
		return;	// Extend with pass/fail later.
	}
	s->PLLIDIV = pllInputDivIndex;	// Proper division, to satisfy PLL requirements.

    uint8_t pllOutputDivIndex;
    for (pllOutputDivIndex = 0; pllOutputDivIndex < pllOutputDivsCount; pllOutputDivIndex++){
		uint8_t pllMulIndex;
		for (pllMulIndex = 0; pllMulIndex < pllMulsCount; pllMulIndex++){
			if ( (cpuFrequency * pllInputDivs[s->PLLIDIV] * pllOutputDivs[pllOutputDivIndex])
			   == (CRYSTALFREQUENCY * pllMuls[pllMulIndex]) ) {
			  s->PLLODIV = pllOutputDivIndex;
			  s->PLLMULT = pllMulIndex;
			  // Match found
			  return;
			}
		}
    }
	return;	// If no combination, just return
    /* 
    * No combination found -> try to get max CPU frequency. This
    * depends from pllInputDiv.
    *
    * The selected values will result in the max CPU frequency, if
    * the pllInputDiv is 1 or 2.
    *
    * With pllInputDiv > 2 we cannot reach the max frequency.
    */

	// This needs a different implementation	

/*    if (pllInputDivs[s->PLLIDIV] >= 2) 
    {
      s->PLLODIV = PLLODIV1; // /1

      #ifdef EXTENDED_TEST_CASE
        s->error = Error_NoCombinationFound;
      #endif
    }
    else 
    {
      s->PLLODIV = PLLODIV2; // /2

      #ifdef EXTENDED_TEST_CASE
        s->error = Error_NoCombinationFound;
      #endif
    }

    s->PLLMULT = PLLMULT20; // x20
*/
}

/**
 * Calculate the necessary values in order to have the peripheral
 * clock running at the desired frequency. As this value is only a
 * divider, this normally requires a call to
 * SystemClocksCalcCpuClockSettings() before, because the CPU
 * frequency is needed to find the divider value.
 *
 * @see system.c::SystemConfig()
 *
 * @return Nothing (the result is put into the struct)
 */
void SystemClocksCalcPeripheralClockSettings(SystemClocksSettings *s, 
                         uint32_t peripheralFrequency) 
{
    #ifdef EXTENDED_TEST_CASE
    s->error = Error_None;
    #endif

    if (peripheralFrequency > CPUCOREMAXFREQUENCY) 
    {
      peripheralFrequency = CPUCOREMAXFREQUENCY;

      #ifdef EXTENDED_TEST_CASE
        s->error = Error_RequestedFrequencyTooHigh;
      #endif
    }

    const uint32_t cpuFrequency = SystemClocksGetCpuFrequency(s);

    uint8_t i;
    for (i = 0; i < pbDivsCount; i++) 
    {
      if (cpuFrequency == peripheralFrequency * pbDivs[i]) 
        {
          s->PBDIV = i;
          // Match found
          return;
        }
    }

    // No match: Use default value
    s->PBDIV = PBDIV1; // /1
    #ifdef EXTENDED_TEST_CASE
    s->error = Error_NoCombinationFound;
    #endif
}

/**
 * Write the clock settings into the registers; this effectively applies the
 * new settings, and will change the CPU frequency, peripheral
 * frequency, and the number of flash wait states.
 *
 * This normally requires a call to
 * SystemClocksCalcCpuClockSettings(), and
 * SystemClocksCalcPeripheralClockSettings() before.
 *
 * @see system.c::SystemConfig()
 *
 * @return Nothing
 */
void SystemClocksWriteSettings(const SystemClocksSettings *s) 
{
    SystemUnlock();

    /**
    * @page 186 
    *
    * PIC32MX Family Clock Diagram
    *
    * @page 189
    *
    * OSCCON: Oscillator Control Register
    */

    /* 
    * bit 10-8 NOSC<2:0>: New Oscillator Selection bits
    *     011 = Primary Oscillator with PLL module (XTPLL, HSPLL or ECPLL)
    */
    OSCCONbits.NOSC = POSCPLL;

    /*
    * bit 29-27 PLLODIV<2:0>: Output Divider for PLL
    */
    OSCCONbits.PLLODIV = s->PLLODIV;

    /*
    * bit 18-16 PLLMULT<2:0>: PLL Multiplier bits
    */
    OSCCONbits.PLLMULT = s->PLLMULT;

    /*
    * bit 0 OSWEN: Oscillator Switch Enable bit
    *   1 = Initiate an osc switch to selection specified by NOSC2:NOSC0 bits
    *   0 = Oscillator switch is complete
    */
    OSCCONbits.OSWEN = 1;
    // Busy wait until osc switch has been completed
    while (OSCCONbits.OSWEN == 1) 
    {
      asm("nop");
    }

    /*
    * bit 20-19 PBDIV<1:0>: Peripheral Bus Clock Divisor
    */
    OSCCONbits.PBDIV = s->PBDIV;

    // Set wait states
    #if !defined(PIC32_PINGUINO_220)    && \
        !defined(PINGUINO32MX220)       && \
        !defined(PINGUINO32MX250)       && \
        !defined(PINGUINO32MX270)       && \
		!defined(__32MX270F256D__)      && \
        !defined(__32MX250F128B__)      // Should do just by CPU here, not by board.

    CHECON = (SystemClocksGetCpuFrequency(s) / 20) - 1;		// FlashWaitStates

    #endif
  
    //
    SystemLock();
}

/*	----------------------------------------------------------------------------
    GetSystemClock
    --------------------------------------------------------------------------*/

uint32_t GetSystemClock(void)
{
    SystemClocksSettings s;
    SystemClocksReadSettings(&s);
    return SystemClocksGetCpuFrequency(&s);
}

/*	----------------------------------------------------------------------------
    GetPeripheralClock()
    --------------------------------------------------------------------------*/

uint32_t GetPeripheralClock()
{
    SystemClocksSettings s;
    SystemClocksReadSettings(&s);
    return SystemClocksGetPeripheralFrequency(&s);
}


/*	----------------------------------------------------------------------------
    SystemSetSystemClock
    --------------------------------------------------------------------------*/

void SystemSetSystemClock(uint32_t cpuCoreFrequency)
{
    SystemClocksSettings s;
    SystemClocksReadSettings(&s);
    SystemClocksCalcCpuClockSettings(&s, cpuCoreFrequency);
    SystemClocksWriteSettings(&s);
}

/*	----------------------------------------------------------------------------
    SystemSetPeripheralClock()
    --------------------------------------------------------------------------*/

void SystemSetPeripheralClock(uint32_t peripheralFrequency)
{
    SystemClocksSettings s;
    SystemClocksReadSettings(&s);
    SystemClocksCalcPeripheralClockSettings(&s, peripheralFrequency);
    SystemClocksWriteSettings(&s);
}

/*	----------------------------------------------------------------------------
    SystemConfig()
    --------------------------------------------------------------------------*/

void SystemConfig(uint32_t cpuCoreFrequency, uint32_t peripheralFreqDiv)
{
    SystemClocksSettings s;
    SystemClocksReadSettings(&s);
  	SystemClocksCalcCpuClockSettings(&s, cpuCoreFrequency);
    SystemClocksCalcPeripheralClockSettings(&s, cpuCoreFrequency / peripheralFreqDiv);
    SystemClocksWriteSettings(&s);

    System_DelaySetTicksPer();

    //RB2014 : already defined in io.c / IOsetDigital()
    //DDPCONbits.JTAGEN=0;  // PORTA is used as digital instead of JTAG
    //CFGCONbits.JTAGEN=0;  // Disable the JTAG port
}

/*	----------------------------------------------------------------------------
    GetCP0Count()
    --------------------------------------------------------------------------*/

uint32_t MIPS32 GetCP0Count() {
    //uint32_t count;
    //asm("di"); // Disable all interrupts
    //count = _CP0_GET_COUNT();
    //asm("ei"); // Enable all interrupts
    //return count;
    return _CP0_GET_COUNT();
}

/*	----------------------------------------------------------------------------
    SetCP0Count()
    --------------------------------------------------------------------------*/

void MIPS32 SetCP0Count(uint32_t count)
{
    //asm("di"); // Disable all interrupts
    _CP0_SET_COUNT(count);
    //asm("ei"); // Enable all interrupts
}


// Also http://www.astralis.fr/wiki/index.php/Pic32-Int
void MIPS32 INTEnableSystemMultiVectoredInt(void)
{
    uint32_t val;

    // set the CP0 cause IV bit high
    asm volatile("mfc0   %0,$13" : "=r"(val));
    val |= 0x00800000;
    asm volatile("mtc0   %0,$13" : "+r"(val));

    INTCONSET = _INTCON_MVEC_MASK;

    // set the CP0 status IE bit high to turn on interrupts
    //INTEnableInterrupts();
	asm("ei");
}


// Some Delay things.
static void System_DelaySetTicksPerUs(){
	// Calculate how many ticks to wait for 1us
	// CP0 runs at FSYS/2. Divide by 1000000 to get ticks/us
	ticksPerUs = (GetSystemClock() / 2) / 1000000;
}

static void System_DelaySetTicksPer100ns(){
	// Calculate how many ticks to wait for 1us
	// CP0 runs at FSYS/2. Divide by 1000000 to get ticks/us
	ticksPer100ns = ticksPerUs / 10;
}

void System_DelaySetTicksPer(){
	System_DelaySetTicksPerUs();
	System_DelaySetTicksPer100ns();
}



// Note, this should really be inlined (PROPERLY), so it would be faster
void System_DelayUs(uint32_t us){
	// Delay for some us. BLOCKING.
	// Prerequisite - call System_DelaySetTicksPer every time after changing CPU frequency
	uint32_t start = _CP0_GET_COUNT();
	while ((_CP0_GET_COUNT() - start) < (ticksPerUs * us)){
	}
}

void System_Delay100ns(uint32_t nss){
	// Delay for some us. BLOCKING.
	// Prerequisite - call System_DelaySetTicksPer every time after changing CPU frequency
	uint32_t start = _CP0_GET_COUNT();
	while ((_CP0_GET_COUNT() - start) < (ticksPer100ns * nss)){
	}
}

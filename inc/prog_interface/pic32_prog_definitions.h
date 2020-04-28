#ifndef PIC32_PROG_DEFINITIONS_H_44614ab793594561968ef594e1e52b71
#define PIC32_PROG_DEFINITIONS_H_44614ab793594561968ef594e1e52b71

// Definitions for the low-level (JTAG & ICSP)

// TMS header and footer defines
#define TMS_HEADER_COMMAND_NBITS        4
#define TMS_HEADER_COMMAND_VAL          0b0011
#define TMS_HEADER_XFERDATA_NBITS       3
#define TMS_HEADER_XFERDATA_VAL         0b001
#define TMS_HEADER_XFERDATAFAST_NBITS   3
#define TMS_HEADER_XFERDATAFAST_VAL     0b001
#define TMS_HEADER_RESET_TAP_NBITS      6
#define TMS_HEADER_RESET_TAP_VAL        0b011111

#define TMS_FOOTER_COMMAND_NBITS        2
#define TMS_FOOTER_COMMAND_VAL          0b01
#define TMS_FOOTER_XFERDATA_NBITS       2
#define TMS_FOOTER_XFERDATA_VAL         0b01
#define TMS_FOOTER_XFERDATAFAST_NBITS   2
#define TMS_FOOTER_XFERDATAFAST_VAL     0b01





// Definitions for communication (one layer up):

// TAP instructions (5-bit).

#define TAP_SW_MTAP     4       // Switch to MCHP TAP controller
#define TAP_SW_ETAP     5       // Switch to EJTAG TAP controller

// MTAP-specific instructions.
#define MTAP_IDCODE     1       // Select chip identification register
#define MTAP_COMMAND    7       // Connect to MCHP command register

// ETAP-specific instructions.
#define ETAP_IDCODE     1       // Device identification
#define ETAP_IMPCODE    3       // Implementation register
#define ETAP_ADDRESS    8       // Select Address register
#define ETAP_DATA       9       // Select Data register
#define ETAP_CONTROL    10      // Select EJTAG Control register
#define ETAP_ALL        11      // Select Address, Data and Control registers
#define ETAP_EJTAGBOOT  12      // On reset, take debug exception
#define ETAP_NORMALBOOT 13      // On reset, enter reset handler
#define ETAP_FASTDATA   14      // Select FastData register

//Length of TAP/MTAP and ETAP commands
#define ETAP_COMMAND_NBITS		5
#define MTAP_COMMAND_NBITS		5
#define MTAP_COMMAND_DR_NBITS	8


//Microchip DR commands (32-bit).
#define MCHP_STATUS        0x00 // Return Status
#define MCHP_ASSERT_RST    0xD1 // Assert device reset
#define MCHP_DEASSERT_RST  0xD0 // Remove device reset
#define MCHP_ERASE         0xFC // Flash chip erase
#define MCHP_FLASH_ENABLE  0xFE // Enable access from CPU to flash
#define MCHP_FLASH_DISABLE 0xFD // Disable access from CPU to flash

//MCHP status value.
#define MCHP_STATUS_CPS    0x80 // Device is NOT code-protected
#define MCHP_STATUS_NVMERR 0x20 // Error occured during NVM operation
#define MCHP_STATUS_CFGRDY 0x08 // Configuration has been read and
                                // Code-Protect State bit is valid
#define MCHP_STATUS_FCBUSY 0x04 // Flash Controller is Busy (erase is in progress)
#define MCHP_STATUS_FAEN   0x02 // Flash access is enabled
#define MCHP_STATUS_DEVRST 0x01 // Device reset is active

//EJTAG Control register.
#define CONTROL_ROCC            (1 << 31)   // Reset occurred
#define CONTROL_PSZ_MASK        (3 << 29)   // Size of pending access
#define CONTROL_PSZ_BYTE        (0 << 29)   // Byte
#define CONTROL_PSZ_HALFWORD    (1 << 29)   // Half-word
#define CONTROL_PSZ_WORD        (2 << 29)   // Word
#define CONTROL_PSZ_TRIPLE      (3 << 29)   // Triple, double-word
#define CONTROL_VPED            (1 << 23)   // VPE disabled
#define CONTROL_DOZE            (1 << 22)   // Processor in low-power mode
#define CONTROL_HALT            (1 << 21)   // System bus clock stopped
#define CONTROL_PERRST          (1 << 20)   // Peripheral reset applied
#define CONTROL_PRNW            (1 << 19)   // Store access
#define CONTROL_PRACC           (1 << 18)   // Pending processor access
#define CONTROL_RDVEC           (1 << 17)   // Relocatable debug exception vector
#define CONTROL_PRRST           (1 << 16)   // Processor reset applied
#define CONTROL_PROBEN          (1 << 15)   // Probe will service processor accesses
#define CONTROL_PROBTRAP        (1 << 14)   // Debug vector at ff200200
#define CONTROL_EJTAGBRK        (1 << 12)   // Debug interrupt exception
#define CONTROL_DM              (1 << 3)    // Debug mode


#endif /* PIC32_PROG_DEFINITIONS_H_ */

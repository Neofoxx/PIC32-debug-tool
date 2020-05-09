#ifndef COMMS_H_96582fe1b9194228b1c0475bf8856698
#define COMMS_H_96582fe1b9194228b1c0475bf8856698


#define type_empty			0
#define type_info			1
#define type_packet			2
#define type_response_raw	3

#define status_ok			0
#define status_parsed		1
#define status_error		2
#define status_overflow		3


#define cyclicBufferSize		1024
#define cyclicBufferSizeMask	(cyclicBufferSize-1)
// Check to enforce buffer being of size 2^n
_Static_assert( ((cyclicBufferSize & cyclicBufferSizeMask) == 0), "Buffer size not equal to 2^n"); // TODO get rid of Eclipse warning.


// To replace struct above!
typedef struct comStruct{
	uint8_t data[cyclicBufferSize];
	uint16_t head;
	uint16_t tail;
	uint32_t timeStamp;
	uint32_t sizeofLastSent;
} comStruct;

extern comStruct uartRXstruct;	// Target to PC
extern comStruct uartTXstruct;	// PC to target
extern comStruct progOUTstruct;	// PC to target
extern comStruct progRETstruct;	// Us to PC (Return values)

// A struct that holds the information about information about packets,
// positions, status, whatever.
typedef struct dataDecoder{
	uint16_t currentPos;
	uint16_t expectedLength;
	uint8_t type;
	uint8_t crc;
	uint8_t status;

} dataDecoder;

/*
void COMMS_reinitStruct(commStruct *st, uint32_t cleanAll);
void COMMS_sendStruct(commStruct *st);
void COMMS_handleIncomingProg(void);
void COMMS_commandExecutor(void);
void COMMS_addInfoToOutput(void);
void COMMS_addToInputBuffer(void);


void COMMS_pic32SendCommand(uint32_t command);
uint64_t COMMS_pic32XferData(uint32_t nBits, uint32_t data, uint32_t readFlag);
*/

// Newer functions
uint32_t COMMS_USB_uartRX_transmitBuf();
uint32_t COMMS_uartTX_addToBuf();
void COMMS_uartTX_transmitBuf();
uint32_t COMMS_progOUT_addToBuf();
uint32_t COMMS_USB_progRET_transmitBuf();
uint32_t COMMS_helper_addToBuf(comStruct* st, uint8_t* data, uint16_t length);
uint32_t COMMS_helper_dataLen(comStruct* st);
uint32_t COMMS_helper_timeSinceSent(comStruct* st);

uint32_t COMMS_helper_spaceLeft(comStruct* st);
void COMMS_helper_getData(comStruct* st, uint32_t length, uint8_t *buf);

void COMMS_reinitPacketHelper(dataDecoder * st);
void COMMS_helper_peekData(const uint8_t *inData, uint32_t start, uint32_t length, uint8_t * buf);
void COMMS_addInfoToOutput();		// TODO sort functions in the same order they are in the .c file.
void COMMS_commandExecutor();
void COMMS_pic32SendCommand(uint32_t command);
uint64_t COMMS_pic32XferData(uint32_t nBits, uint32_t data, uint32_t readFlag);
void COMMS_handleIncomingProg(void);

#endif

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

typedef struct commStruct{
	uint8_t data[2048];			// Data received
	uint16_t head;				// Circular buffer
	uint16_t tail;				// Circular buffer
	uint16_t currentPos;		// Current position in data buffer, with crc
	uint8_t type;				// Type of request
	uint16_t expectedLength;	// Length of data altogether, with crc
	uint8_t crc;				// The crc.
	uint8_t status;				// Status - ok, error, ?

} commStruct;


void COMMS_reinitStruct(commStruct *st, uint32_t cleanAll);
void COMMS_sendStruct(commStruct *st);
void COMMS_handleIncomingProg(void);
void COMMS_commandExecutor(void);
void COMMS_addInfoToOutput(void);
void COMMS_addToInputBuffer(void);


void COMMS_pic32SendCommand(uint32_t command);
uint64_t COMMS_pic32XferData(uint32_t nBits, uint32_t data, uint32_t readFlag);


#endif

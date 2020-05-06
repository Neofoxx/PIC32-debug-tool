#ifndef UARTDRV_H_a424a95491db41c6ab0c1d6e8070a876
#define UARTDRV_H_a424a95491db41c6ab0c1d6e8070a876

#include <inttypes.h>

void UARTDrv_InitDMA();
void UARTDrv_Init(uint32_t baud);
void UARTDrv_SendBlocking(uint8_t * buffer, uint32_t length);
uint32_t UARTDrv_GetCount();
uint32_t UARTDrv_GetReceiveData(uint8_t *copyTo, uint8_t maxSize);

uint32_t UARTDrv_IsTxDmaRunning();
void UARTDrv_RunDmaTx();


#if defined(__32MX270F256D__)
	
#elif defined(__32MX440F256H__)

#endif

#endif

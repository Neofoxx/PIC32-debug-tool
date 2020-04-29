#ifndef LED_H_1bbf1140eed64c8eb493ff52fbe25fc3
#define LED_H_1bbf1140eed64c8eb493ff52fbe25fc3

void LED_init();
void LED_setState(uint8_t setState);
void LED_toggle();

void LED_USBINT_toggle();
void LED_USBUART_OUT_toggle();
void LED_USBUART_IN_toggle();
void LED_USBPROG_OUT_toggle();
void LED_USBPROG_IN_toggle();



#endif

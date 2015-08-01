#include "HAL.h"

inline void InitAll(void)
{

//InitUSART
UBRR0L = LO(bauddivider);
UBRR0H = HI(bauddivider);
UCSR0A = 0;
UCSR0B = 1<<RXEN|1<<TXEN|0<<RXCIE|0<<TXCIE;
UCSR0C = 1<<UCSZ0|1<<UCSZ1;

//InitPort
LED_DDR |= 1<<LED1|1<<LED2|1<<LED3|1<<I_L|1<<I_C;


}




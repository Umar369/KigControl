#ifndef HAL_H
#define HAL_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include "avrlibdefs.h"
#include "avrlibtypes.h"
#include <avr/pgmspace.h>
#include <avr/wdt.h>

//Clock Config
#define F_CPU 7372800L

//System Timer Config
#define Prescaler	  		64
#define	TimerDivider  		(F_CPU/Prescaler/1000)		// 1 mS

//USART Config
#define baudrate 9600L
#define bauddivider (F_CPU/(16*baudrate)-1)
#define HI(x) ((x)>>8)
#define LO(x) ((x)& 0xFF)

//PORT Defines
#define LED1 		4
#define LED2		5
#define	LED3		7

#define I_C			3
#define I_L			6
#define LED_PORT 	PORTC
#define LED_DDR		DDRC


extern void InitAll(void);



#endif

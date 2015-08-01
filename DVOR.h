#ifndef __DVORPROTOCOL__
#define __DVORPROTOCOL__

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/crc16.h>
#include "avrlibtypes.h"
#include "avrlibdefs.h"
#include "string.h"
#include "stdlib.h"
#define BAUNDRATE 9600L
#define BAUND (7372800/(16*BAUNDRATE)-1) 

#define RE_DE_PIN PB7
#define RE_DE_PORT PORTB

#define STATUS_REG 			SREG
#define Interrupt_Flag		SREG_I
#define Disable_Interrupt	cli();
#define Enable_Interrupt	sei();

volatile struct
{
	u08 HAA:1;
	u08 H55:1;
	u08 start:1;
	u08 complete:1;
	u08 error:4;
	u08 buffer[64];
} RXbuffer;

volatile struct {
	u08 length;
	u08 buffer[64];
	u08 status;
}TXbuffer;

extern void DVOR_Init(u16);
extern void USART_Transmit(u08);
extern unsigned char USART_Receive();
//extern void DVOR_Init(u16);
extern void RX(u08);
extern void TX(u08,char*,u08);
extern void TX_C(u08,u08,char*,u08);
extern void parcepacket(u08,u08);

//буффер приема
extern u08  rxbufgetchar(u08 *);
extern void rxbufgotostart(u08 *,u08);
extern void rxbufputchar(u08 *,u08);

//буффер передачи
extern u08  txbuffergetchar();
extern void txbufferputchar(u08);

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

#endif
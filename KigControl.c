#include "DVOR.h"
#include <util/delay.h>
#include <avr/interrupt.h>

#define SETBIT(port,pin)  port |=(1<<pin)
#define CLRBIT(port,pin)  port &=~(1<<pin)
#define delay(ms) _delay_ms(ms)
#define DD_SS PB0
#define DD_SCK PB1 //D0
#define DD_MOSI PB2 //D1
#define DDR_SPI DDRB
#define PORT_SPI PORTB
#define RE PE2
#define DE PE3

unsigned int tik = 0;
int flag = 2; // flag for start or stop blink (2- stop)
int Metka = 0; // position in display menu
unsigned int event = 0; // presence of interrupt
unsigned int position = 7; // position for blink (7- first, 6- second..., 1 and 0- not displayed on LED)
unsigned int display[8] = {0,0,0,0,0,0,0,0};
const static NumsHex[10] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};
unsigned long int NewFrequency, PrevFrequency;
unsigned int ConvertFrequency[6] = {0,0,0,0,0,0};

void Esc()
{
	Metka = 0;
	flag = 2;
	ConvertingFrequency(PrevFrequency);
	CalcNewFrequency();
	PrintFrequency();
}
void MenuOk()
{
	if (Metka == 0) // Menu - режим установки частоты
	{
		Metka = 11;
		flag = 0;
		position = 7;
	}
	else if (Metka!=0)			// OK - установить и отправить частоту
	{
		Metka = 0;
		flag = 2;
		CalcNewFrequency();
		ConvertingFrequency(NewFrequency);
		PrintFrequency();
		TX_C(0x02,0xA1,(char*)&NewFrequency, 4);
	}
}
void Right()
{
	if (Metka >= 15)
	{
		Metka = 11;
		flag = 0;
		PrintFrequency();
		position = 7;
		
	}
	else
	{
		Metka++;
		flag = 0;
		PrintFrequency();
		position--;
	}
}
void Down()
{
	if (Metka == 11)
	{
		ConvertFrequency[0]--;
		if (ConvertFrequency[0] < 1)
		{
			ConvertFrequency[0] = 4;
			ConvertFrequency[1] = 0;
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
	}
	else if (Metka == 12)
	{
		if ((ConvertFrequency[0] == 1)&&(ConvertFrequency[1] == 0))
		{
			ConvertFrequency[1] = 5;
		}
		else ConvertFrequency[1]--;
		if (ConvertFrequency[0] == 2)
		{
			ConvertFrequency[1] = 2;
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
			if (ConvertFrequency[1] < 2)
			{
				ConvertFrequency[1] = 9;
			}
		}
		else if (ConvertFrequency[0] == 3)
		{
			ConvertFrequency[1]--;
			if (ConvertFrequency[1] < 0)
			{
				ConvertFrequency[1] = 9;
			}
		}
		else if (ConvertFrequency[0] == 4)
		{
			ConvertFrequency[1] = 0;
		}
	}
	else if (Metka == 13)
	{
		ConvertFrequency[2]--;
		if (ConvertFrequency[2] < 0)
		{
			ConvertFrequency[2] = 9;
		}
		else if ((ConvertFrequency[0] == 1) && (ConvertFrequency[1] == 5))
		{
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[0] == 4)
		{
			ConvertFrequency[2] = 0;
		}
	}
	else if (Metka == 14)
	{
		ConvertFrequency[3]--;
		if (ConvertFrequency[3] < 0)
		{
			ConvertFrequency[3] = 9;
		}
		else if ((ConvertFrequency[0] == 1) && (ConvertFrequency[1] == 5))
		{
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[0] == 4)
		{
			ConvertFrequency[3] = 0;
		}
	}
	else if (Metka == 15)
	{
		if (ConvertFrequency[4] == 0)
		{
			ConvertFrequency[4] = 7;
			ConvertFrequency[5] = 5;
		}
		else if (ConvertFrequency[4] == 7)
		{
			ConvertFrequency[4] = 5;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[4] == 5)
		{
			ConvertFrequency[4] = 2;
			ConvertFrequency[5] = 5;
		}
		else if ((ConvertFrequency[4] == 2)|| ((ConvertFrequency[0] == 1) && (ConvertFrequency[1] == 5)) || (ConvertFrequency[0] == 4))
		{
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
	}
	PrintFrequency();
}
void Left()
{
	if (Metka > 11)
	{
		Metka--;
		flag = 0;
		PrintFrequency();
		position++;
		TX_C(0x02,0xA1,(char*)&NewFrequency, 4);
	}
	else if(Metka == 11)
	{
		Metka = 15;
		flag = 0;
		PrintFrequency();
		position = 3;
		TX_C(0x02,0xA1,(char*)&NewFrequency, 4);
	}
}
void Up()
{
	if (Metka == 11)
	{
		ConvertFrequency[0]++;
		if (ConvertFrequency[0] == 4)
		{
			ConvertFrequency[1] = 0;
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[0] > 4)
		{
			ConvertFrequency[0] = 1;
			ConvertFrequency[1] = 0;
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[0] == 2)
		{
			ConvertFrequency[1] = 2;
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
	}
	else if (Metka == 12)
	{
		if ((ConvertFrequency[0] == 1) && (ConvertFrequency[1] == 4))
		{
			ConvertFrequency[1]++;
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[0] == 1)
		{
			ConvertFrequency[1]++;
			if (ConvertFrequency[1] > 5)
			{
				ConvertFrequency[1] = 0;
			}
		}
		else if (ConvertFrequency[0] == 2)
		{
			ConvertFrequency[1]++;
			if (ConvertFrequency[1] > 9)
			{
				ConvertFrequency[1] = 2;
			}
		}
		else if (ConvertFrequency[0] == 3)
		{
			ConvertFrequency[1]++;
			if (ConvertFrequency[1] > 9)
			{
				ConvertFrequency[1] = 0;
			}
		}
		else if (ConvertFrequency[0] == 4)
		{
			ConvertFrequency[1] = 0;
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
	}
	else if (Metka == 13)
	{
		ConvertFrequency[2]++;
		if ((ConvertFrequency[0] == 1) && (ConvertFrequency[1] == 5))
		{
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[2] > 9)
		{
			ConvertFrequency[2] = 0;
		}
		else if (ConvertFrequency[0] == 4)
		{
			ConvertFrequency[2] = 0;
		}
	}
	else if (Metka == 14)
	{
		ConvertFrequency[3]++;
		if ((ConvertFrequency[0] == 1) && (ConvertFrequency[1] == 5))
		{
			ConvertFrequency[2] = 0;
			ConvertFrequency[3] = 0;
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[3] > 9)
		{
			ConvertFrequency[3] = 0;
		}
		else if (ConvertFrequency[0] == 4)
		ConvertFrequency[3] = 0;
	}
	else if (Metka == 15)
	{
		if ((ConvertFrequency[4] == 7) || ((ConvertFrequency[0] == 1) && (ConvertFrequency[1] == 5)) || (ConvertFrequency[0] == 4))
		{
			ConvertFrequency[4] = 0;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[4] == 0)
		{
			ConvertFrequency[4] = 2;
			ConvertFrequency[5] = 5;
		}
		else if (ConvertFrequency[4] == 2)
		{
			ConvertFrequency[4] = 5;
			ConvertFrequency[5] = 0;
		}
		else if (ConvertFrequency[4] == 5)
		{
			ConvertFrequency[4] = 7;
			ConvertFrequency[5] = 5;
		}
	}
	PrintFrequency();	
}
void InitTimer0()
{
	OCR0=255; // 0.5c при F_CPU 7372800L
	TCCR0|=(1<<CS02)|(1<<CS01)|(1<<CS00)|(1<<WGM01)|(0<<WGM00); // Делить частоту на 1024.
	TIMSK |= (1<<OCIE0);
}
void SPI_MasterInit(void)
{
	/* Set MOSI and SCK output, all others input */
	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_SS);
	//DDR_CONFIG_PORT=(1<<OLED_DC)|(1<<OLED_RES);
	/* Enable SPI, Master, set clock rate fck/128 */
	//SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0);
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA)|(1<<SPR1)|(1<<SPR0)|(1<<SPIE);
	
}
void SPI_MasterTransmit(unsigned char cData)
{
	/* Start transmission */
	SPDR = cData;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)));

}
void InitADM489()	// Config ADM489 for transmit
{
	DDRE = (1<<RE)|(1<<DE);
	PORTE = (1<<RE)|(1<<DE);
}
void InitTimer3() // Timer for blink
{
	OCR3AH=14; OCR3AL=16; // 0.5c при F_CPU 7372800L
	TCCR3B|=(1<<CS30)|(1<<CS32)|(1<<WGM32); // Делить частоту на 1024. 
	SREG|=(1<<7); // Разрешить прерывания.
	ETIMSK|=(1<<OCIE3A); // Разрешить прерывание по совпадению.
	
}
void InitButtons()
{
	/* The falling edge of INTn generates asynchronously an interrupt request. */
	EICRA = (1<<ISC11)|(0<<ISC10)|(1<<ISC01)|(0<<ISC00);
	EICRB = (1<<ISC71)|(0<<ISC70)|(1<<ISC61)|(0<<ISC60)|(1<<ISC51)|(0<<ISC50)|(1<<ISC41)|(0<<ISC40);
	/* External Interrupt Request 7,6,5,4,1,0 Enable */
	EIMSK = (1<<INT7)|(1<<INT6)|(1<<INT5)|(1<<INT4)|(1<<INT1)|(1<<INT0);
}
void TurnOffDisp()
{
	display[7]=0xFF;
	display[6]=0xFF;
	display[5]=0xFF;
	display[4]=0xFF;
	display[3]=0xFF;
	display[2]=0xFF;
	display[1]=0xFF;
	display[0]=0xFF;
	SPDR = display[0];
}
void ConvertingFrequency(unsigned long int freq)
{
	unsigned long int BufFreq = freq/1000;
	PrevFrequency = freq;
	ConvertFrequency[5] = BufFreq % 10;
	ConvertFrequency[4] = BufFreq / 10 % 10;
	ConvertFrequency[3] = BufFreq / 100 % 10;
	ConvertFrequency[2] = BufFreq / 1000 % 10;
	ConvertFrequency[1] = BufFreq / 10000 % 10;
	ConvertFrequency[0] = BufFreq / 100000 % 10;
}
void CalcNewFrequency()
{
	NewFrequency=   ConvertFrequency[0]*100000 +
					ConvertFrequency[1]*10000 + 
					ConvertFrequency[2]*1000 + 
					ConvertFrequency[3]*100 + 
					ConvertFrequency[4]*10 + 
					ConvertFrequency[5];
	NewFrequency = NewFrequency*1000;
	return NewFrequency; 
}
void PrintFrequency()
{
	display[7] = NumsHex[ConvertFrequency[0]];
	display[6] = NumsHex[ConvertFrequency[1]];
	display[5] = NumsHex[ConvertFrequency[2]] & 0x7F;
	display[4] = NumsHex[ConvertFrequency[3]];
	display[3] = NumsHex[ConvertFrequency[4]];
	display[2] = NumsHex[ConvertFrequency[5]];
	display[1] = 0xFF;
	display[0] = 0xFF;
	SPDR = display[0];
}
ISR(SPI_STC_vect)
{
	static char counter = 1;
	
	if (counter == 8)
	{
		counter = 1;
		PORTB = 1 << PB0;
		PORTB = 0 << PB0;
	}else
	{
		SPDR = display[counter++];
	}
}
ISR(TIMER3_COMPA_vect) //Обработчик прерывания по совпадению с OCR3A.
{
	if (flag == 0) // Погасить курсор в позиции position
	{
		display[position] = 0xff;
		SPDR = display [0];
		flag = 1;
	}
	else if(flag == 1) // Зажечь курсор и затем погасить
	{	
		flag = 0;
		display[position] = NumsHex[ConvertFrequency[7-position]]; // [7-position] - for conform 7 to 0, 6 to 1, 5 to 2...
		SPDR = display [0];
	}
	else if (flag == 2) // Зажечь и не гасить
	{
		CalcNewFrequency();
		ConvertingFrequency(NewFrequency);
		PrintFrequency();
	}		
}
ISR(TIMER0_COMP_vect)
{
	tik++;	
}
ISR(INT4_vect) // Left
{
	static unsigned int tik2 = 0;
	if ((tik - tik2) > 10)
	{
		Left();
		tik2 = tik;
	}	
}
ISR(INT5_vect) // Up
{
	static unsigned int tik2 = 0;
	if ((tik - tik2) > 10)
	{
		Up();
		tik2 = tik;
	}
}
ISR(INT6_vect) // Down
{
	static unsigned int tik2 = 0;
	if ((tik - tik2) > 10)
	{
		Down();
		tik2 = tik;
	}
}
ISR(INT7_vect) // Right
{
	static unsigned int tik2 = 0;
	if ((tik - tik2) > 10)
	{
		Right();
		tik2 = tik;
	}
}
ISR(INT0_vect) // Menu, Ok
{
	static unsigned int tik2 = 0;
	if ((tik - tik2) > 10)
	{
		MenuOk();
		tik2 = tik;
	}	
}
ISR(INT1_vect) // Esc
{
	static unsigned int tik2 = 0;
	if ((tik - tik2) > 10)
	{
		Esc();
		tik2 = tik;
	}
}
int main(void)
{
	SPI_MasterInit();
	InitADM489();
	DVOR_Init(BAUND);
	InitButtons();
	InitTimer3();
	InitTimer0();
	TurnOffDisp();
	NewFrequency = 100000000;
	ConvertingFrequency(NewFrequency);
	PrintFrequency();
	
	while(1)
	{

	}
	//return 0;
}

#include "DVOR.h"
#include "util/crc16.h"
#define ADDRESS 0x01
#define SETBIT(port,pin)  port |=(1<<pin)
#define CLRBIT(port,pin)  port &=~(1<<pin)
void SendComplete();
ISR(USART0_RX_vect)
{	
	RX(UDR0);//прием пакета 
}

ISR(USART0_TX_vect){
	//CLRBIT(RE_DE_PORT,RE_DE_PIN);
}

ISR(USART0_UDRE_vect){
	if (TXbuffer.length)
	{
		//SETBIT(RE_DE_PORT,RE_DE_PIN); //передача
		UDR0 = txbuffergetchar();
	} else {
		TXbuffer.status=0;
		CLRBIT(UCSR0B,UDRE0);
	}
}
enum {
	NO_ERROR=0,
	ERR_NOT_FOR_ME,
	ERR_CRC,
	ERR_UNKNOWN,
	ERR_TIMEOUT
};

void Timeout()
{
	RXbuffer.error = ERR_TIMEOUT;
	return;
}


void DVOR_Init(u16 ubrr){
	DDRE |=(1<<PE0)|(1<<PE1);
	PORTE &=~((1<<PE0)|(1<<PE1));
	UBRR0H = (u08)(ubrr>>8);
	UBRR0L = (u08) ubrr;
	/* Enable receiver and transmitter*/
	UCSR0B = (1<<RXCIE0)|(1<<TXCIE0)|(0<<UDRE0)|(1<<RXEN)|(1<<TXEN);
	/* Set frame format: 8data, 2stop bit*/
	UCSR0C = (1<<USBS)|(3<<UCSZ0);
}

void USART_Transmit( unsigned char data )
{

	/* Wait for empty transmit buffer*/
	while( !( UCSR0A & (1<<UDRE)) )
	;
	/* Put data into buffer, sends the data*/
	UDR0 = data;
}

unsigned char USART_Receive( void)
{
	/* Wait for data to be received*/
	while( !(UCSR0A & (1<<RXC)) )
	;
	/* Get and return received data from buffer*/
	return UDR0;
}

void TX(u08 address,char* data,u08 len){

	u08 *buffer;
	u16 crccalc=0;
	Disable_Interrupt;
	buffer = malloc((6+len)*sizeof(u08));//выделяем память для пакета
	//txbufferputchar(0xFF);
	txbufferputchar(0xAA);
	txbufferputchar(0x55);
	buffer[0]=ADDRESS;
	buffer[1]=address;
	buffer[2]=0;
	buffer[3]=len;
	memcpy(buffer+4,data,len);
	//подсчет CRC кода
	for(u08 i=0;i<4+len;i++){
		crccalc = _crc_xmodem_update(crccalc,buffer[i]);
	}
	buffer[len+4] = (u08)(crccalc&0xff);
	buffer[len+5]   = (u08)( (crccalc>>8) & 0xff );
	for (u08 i=0;i<6+len;i++)
	{
		txbufferputchar(buffer[i]);
		if(buffer[i]==0xaa)
			txbufferputchar(0x00);
	}
	free(buffer);
	sbi(UCSR0B,UDRE0);
	TXbuffer.status=1;	
	Enable_Interrupt;	
}

unsigned short CRC16_Byte(unsigned char *msg, unsigned int msg_len)
{
	unsigned short crccalc = 0;
	while (msg_len-- > 0)
	crccalc = _crc_xmodem_update(crccalc,*msg++);
	return (crccalc);
}
void TX_C(u08 address, u08 cmd, char* data, u08 len){

	u08 *buffer;
	u16 crccalc=0;
	Disable_Interrupt;
	buffer = malloc((7+len)*sizeof(u08));//выделяем память для пакета
	txbufferputchar(0xFF);
	txbufferputchar(0xAA);
	txbufferputchar(0x55);
	buffer[0]=ADDRESS;
	buffer[1]=address;
	buffer[2]=0;
	buffer[3]=len+1;
	buffer[4]=cmd;
	memcpy(buffer+5,data,5);
	//подсчет CRC кода
// 	for(u08 i=0;i<4+len;i++){
// 		crccalc = _crc_xmodem_update(crccalc,buffer[i]);
// 	}
crccalc = CRC16_Byte((u08*)buffer, (unsigned int)(len+5));
// 	buffer[len+5] = (u08)(crccalc&0xff);
// 	buffer[len+6]   = (u08)( (crccalc>>8) & 0xff );
	buffer[len+5] = (u08)(crccalc);
	buffer[len+6]   = (u08)(crccalc>>8);
	for (u08 i=0;i<7+len;i++)
	{
		txbufferputchar(buffer[i]);
		if(buffer[i]==0xaa)
		txbufferputchar(0x00);
	}
	free(buffer);
	sbi(UCSR0B,UDRE0);
	TXbuffer.status=1;	
	Enable_Interrupt;	
}



//кольцевой буффер для отправки сообщения функция заполнения

void txbufferputchar(u08 data){
	static u08 pos=0;
	if (pos>=sizeof(TXbuffer.buffer))
		pos=0;
	
	TXbuffer.buffer[pos++]=data;
	TXbuffer.length++;
}
//кольцевой буффер для получения элементов
u08 txbuffergetchar(){
	static u08 pos=0;
	if (pos>=sizeof(TXbuffer.buffer))
		pos=0;
	TXbuffer.length--;
	return TXbuffer.buffer[pos++];
}

void RX(u08 byte)
{
	static u08 buf_pos=0;//позиция в буфере
	static u08 cur_pos=0;//текушая позиция
	static u08 length=0;//длинна пакета
	static u08 dvrlen=0;//длинна пакета
	//SetTimerTask(Timeout,10);//таймаут пакета 
	switch(byte)
	{
		case 0xAA://header 1
		{
			RXbuffer.HAA=1;
		}			
		case 0x55://header 2
		{
			if(RXbuffer.HAA && byte==0x55)
			{
				RXbuffer.HAA=0;
				RXbuffer.H55=1;
				RXbuffer.start=1;
				cur_pos=buf_pos;
				length=0;
				dvrlen = 0;
				RXbuffer.error=0;
				//PORTA=buf_pos;
				break; //начало приема пакета )) 
			} else if(!RXbuffer.start && byte==0xAA){
				break;
			}
		}			
		case 0x00://ignore
		{
			if(RXbuffer.HAA==1 && byte == 0x00){
				RXbuffer.HAA=0;
				break;
			}
		}					
		default:
		{
			if(RXbuffer.error) break;
			if(length==1){
				if(byte!=ADDRESS) if(byte!=0xFF){
					RXbuffer.error=ERR_NOT_FOR_ME;
					RXbuffer.start=0;
					break;
				}
			}
			if(!RXbuffer.start){	
				break;
			}
			rxbufputchar(&buf_pos,byte);//принимаем пакет в кольцевой буфер		
			if(length==3 ){
				dvrlen = byte+6;
			}
			length++;					
			if(length && length==dvrlen){
				RXbuffer.start=0;
				RXbuffer.complete=1;	
				parcepacket(cur_pos,length);
			}
		}
	}
 }
//обработка пакета и выполнение заданий
void parcepacket(u08 pos, u08 length)
{
	u08 sender,receiver,flag,len,cmd;
	u16 crccalc=0;
	//порверка CRC кода
	for(u08 j=0;j<length-2;j++)
	{
		crccalc = _crc_xmodem_update(crccalc,rxbufgetchar(&pos));
	}
	//сравнение CRC
	if((u08)(crccalc&0xff)!=(u08)rxbufgetchar(&pos) || (u08)( (crccalc>>8) & 0xff )!=rxbufgetchar(&pos))
	{
		TX(0xFF,"\xEECrc error",10);//Неправильный CRC поставить оповешение в дальнейшем
	}
	rxbufgotostart(&pos,length);
	
	sender= rxbufgetchar(&pos);
	receiver = rxbufgetchar(&pos);
	flag = rxbufgetchar(&pos);
	len = rxbufgetchar(&pos);
	cmd = rxbufgetchar(&pos);
	if(len==0 && receiver !=0xff)
	{
		TX(sender, "\xEELen error",10);
	}
	
	switch(cmd)
	{
// 		case 0x00:
// 			break;
// 		case 0x04:
// 			break;
// 		case 0x05:
// 			break;
// 		case 0x06:
// 			break;
// 		case 0x07:
// 			break;
	}
	return ;
}
//получение данных из кольцевого буфера
u08 rxbufgetchar(u08 *pos){
	if( *pos >=sizeof(RXbuffer.buffer))
		*pos=0;
	return RXbuffer.buffer[(*pos)++];
}

//переход на начало пакета
void rxbufgotostart(u08 *pos,u08 length){
	//s08 spos = *pos;
	if((s08)(*pos)-(s08)length<0){
		(*pos) = sizeof(RXbuffer.buffer) - (length - (*pos));
	}else{
		(*pos) -= length;
	}

}

//запись байта в кольцевой буффер
void rxbufputchar(u08 *pos,u08 byte){
	if(*pos>=sizeof(RXbuffer.buffer))
		*pos=0;
	RXbuffer.buffer[(*pos)++]=byte;	

}
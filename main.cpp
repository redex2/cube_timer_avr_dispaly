#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

/*
//      a
//    -----
//   |     |
// f |     | b
//   |     |
//    -----
//   |  g  |
// e |     | c
//   |     |
//    -----     .h
//      d
//
*/

#define LED_A (1<<6)
#define LED_B (1<<3)
#define LED_C (1<<2)
#define LED_D (1<<4)
#define LED_E (1<<0)
#define LED_F (1<<1)
#define LED_G (1<<5)
#define LED_H (1<<7)

#define LED_0 (LED_A|LED_B|LED_C|LED_D|LED_E|LED_F)
#define LED_1 (LED_B|LED_C)
#define LED_2 (LED_A|LED_B|LED_G|LED_E|LED_D)
#define LED_3 (LED_B|LED_C|LED_A|LED_G|LED_D)
#define LED_4 (LED_C|LED_B|LED_G|LED_F)
#define LED_5 (LED_A|LED_F|LED_G|LED_C|LED_D)
#define LED_6 (LED_C|LED_D|LED_E|LED_G|LED_F|LED_A)
#define LED_7 (LED_B|LED_C|LED_A)
#define LED_8 (LED_A|LED_B|LED_C|LED_D|LED_E|LED_F|LED_G)
#define LED_9 (LED_A|LED_B|LED_D|LED_C|LED_F|LED_G)

uint8_t display_data[6]		= {LED_G|LED_H, LED_G, LED_G|LED_H, LED_G, LED_G, LED_G};
const uint8_t digits[10]	= {LED_0, LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7, LED_8, LED_9};

#define		rcv_data_size 11
#define		rcv_timeout_threshold 0x1FF

uint8_t		rcv_data[rcv_data_size];

uint16_t	rcv_timeout		= 0xFFFF;
uint8_t		display_index	= 0;

int main(void)
{
    asm("wdr");
    WDTCR |= (1<<WDCE) | (1<<WDE);
    WDTCR = (1<<WDE) | (1<<WDP1) | (1<<WDP0);
    asm("wdr");
    
	DDRB |= 255;//set PB as out
	DDRD |= 0b01111110;//set PD as out and PD0(rx) as input
	PORTD |= 1; //PD0 pull-up
	
	PRR = (1<<PRTIM0)|(1<<PRUSI); //turn off timer0 and USI
	ACSR |= (1<<ACD); //turn off analog comparator
	
	//clear timer
	TCCR1A=0x00;
	TCCR1B=0x00;
	TCNT1H=0x00;
	TCNT1L=0x00;
	
	// OCR1A=3999 - 4MHz
	OCR1AH=0x0F;
	OCR1AL=0x9F;
	
	//unused
	OCR1BH=0xff;
	OCR1BL=0xff;

	//interrupts flag
	TIFR=TIFR&(~( (1<<ICF1)|(1<<OCF1A)|(1<<OCF1B)|(1<<TOV1)));

	//interrupts flag
	TIMSK=(TIMSK&(~((1<<ICIE1)|(1<<OCIE1A)|(1<<OCIE1B)|(1<<TOIE1))))|(1<<OCIE1A);
	
	//timer 1 control reg 
	TCCR1A=0x00;
	TCCR1B=0x09;
	
	
	//USART
	UCSRA=0;
	UCSRB=(1<<RXCIE)|(1<<RXEN);
	UCSRC=(1<<UCSZ1);
	//1200bps@4MHz
	UBRRH=0x00;
	UBRRL=0xCF;
	
	asm("wdr");
	asm("sei");
	asm("wdr");
	
    while (1) asm("nop");
}

SIGNAL(TIMER1_COMPA_vect)
{
	asm("wdr");
	PORTD|=0b01111110;
	PORTB=display_data[display_index];
	PORTD=((~(1<<(display_index+1)))&0b01111110)|(PORTD&(~0b01111110));
	
	display_index++;
	if(display_index>5)
	{
		display_index=0;
	}
	
	if(rcv_timeout<0xFFFF) rcv_timeout++;
	
	if(rcv_timeout > rcv_timeout_threshold)
	{
		display_data[0]=LED_G|LED_H;
		display_data[1]=LED_G;
		display_data[2]=LED_G|LED_H;
		display_data[3]=LED_G;
		display_data[4]=LED_G;
		display_data[5]=LED_G;
	}
}

SIGNAL(USART_RX_vect)
{
	for(uint8_t i = (rcv_data_size-1); i > 0; i--) rcv_data[i] = rcv_data[i-1];
	rcv_data[0] = UDR;
	UDR = 0;
	
	if(rcv_data[0]=='\n' && rcv_data[1]>=64)
	{
		uint8_t n	= 0;
		uint8_t sum	= 64;
		for(uint8_t i = 2; i < rcv_data_size; i++)
		{
			if(rcv_data[i] >= '0' && rcv_data[i] <= '9') 
			{
				n++;
				sum+=(rcv_data[i]-48);
			}
			else 
			{
				break;
			}
		}
		uint8_t tmp = rcv_data[2+n];
		if((n == 5 || n == 6) && rcv_data[1] == sum && (tmp == ' ' || (tmp >= 'A' && tmp <= 'Z')))
		{
			rcv_timeout = 0;
			if((rcv_data[7]-48))
			{
				display_data[0] = digits[(rcv_data[7]-48)]|LED_H;
				display_data[1] = digits[(rcv_data[6]-48)];
			}
			else
			{
				display_data[0] = 0;
				if((rcv_data[6]-48))
				{
					display_data[1] = digits[(rcv_data[6]-48)];
				}
				else
				{
					display_data[1] = 0;
				}
			}
			display_data[2] = digits[(rcv_data[5]-48)]|LED_H;
			display_data[3] = digits[(rcv_data[4]-48)];
			display_data[4] = digits[(rcv_data[3]-48)];
			if(n==6) display_data[5] = digits[(rcv_data[2]-48)];
			else display_data[5] = 0; //older timers not send thousandths of a second
		}
	}
}
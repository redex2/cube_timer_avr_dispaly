#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#define F_CPU 11059200
#include <util/delay.h>



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

#define LED_A 1<<6
#define LED_B 1<<3
#define LED_C 1<<2
#define LED_D 1<<4
#define LED_E 1<<0
#define LED_F 1<<1
#define LED_G 1<<5
#define LED_H 1<<7

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

uint8_t display_data[6]={LED_G|LED_H, LED_G, LED_G|LED_H, LED_G, LED_G, LED_G};
const uint8_t digits[10] = {LED_0, LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7, LED_8, LED_9};

int main(void)
{
    asm("wdr");
    WDTCR |= (1<<WDCE) | (1<<WDE);
    WDTCR = (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);
    asm("wdr");
    //uint8_t sreg_tmp;
	DDRB |= 255;//set PB as out
	DDRD |= 0b01111110;//set PD as out
	
	//clear timer
	TCCR1A=0x00;
	TCCR1B=0x00;
	TCNT1H=0x00;
	TCNT1L=0x00;
	
	// OCR1A=7999 - 8MHz
	OCR1AH=0x23;
	OCR1AL=0xff;

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

	
	asm("wdr");
	asm("sei");
	asm("wdr");
	
    while (1) 
    {
		asm("wdr");
    }
}

uint8_t display_index = 0;

SIGNAL(TIMER1_COMPA_vect)
{
	PORTD|=0b01111110;
	PORTB=display_data[display_index];
	PORTD=((~(1<<(display_index+1)))&0b01111110)|(PORTD&(~0b01111110));
	display_index++;
	if(display_index>5)
	{
		display_index=0;
	}
}
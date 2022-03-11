// Copyright (c) 2018 nikitapnn1@gmail.com

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <myavrlib/uart.h>
#include <myavrlib/os/messages.h>
#include <string.h>
#include "ir.h"
#include "os.h"

#define FRAME_LEN 4

static uint8_t buffer[FRAME_LEN];
static uint8_t size;
static uint8_t bit_n;
static int8_t state;

#if (F_CPU == 11059200UL)
// divisor 1024
#	define PULSE_1 90
#	define PULSE_2 40
// divisor 256
#	define us600 20
#	define us1600 60
#elif (F_CPU == 16000000UL)
// divisor 1024
#	define PULSE_1 131
#	define PULSE_2 58
// divisor 256
#	define us600 29
#	define us1600 87
#else
#	error "F_CPU is not defined."
#endif

ISR(TIMER2_OVF_vect) {
	GICR &= ~(1 << INT1);
	TCCR2 = 0;
	sei();
	if ((size == 4) && ((uint8_t)buffer[2] == (uint8_t)~buffer[3])) {
	//	uart_send_c(buffer[2]);
		message_send(MSG_REMOTE_BUTTON_DOWN, buffer[2]);
	} 
	state = -3;
	GICR |= (1 << INT1);
}

ISR(INT1_vect) {
	uint8_t tcnt = TCNT2; 
	TCNT2 = 0;
	switch( state )
	{
		case -3:
			memset(buffer, 0, FRAME_LEN);
			bit_n = 0;
			size = 0;
			TCCR2 = (1 << CS20) | (1 << CS21) | (1 << CS22); // 1024
			state = -2;
			break;
		case -2:
			// if first pulse width more then 9000 us
			if (tcnt >= PULSE_1) {
				state = -1;
			} else {
				state = -3;
			}
			break;
		case -1:
			 // if second pulse more then 4500 us
			 if (tcnt >= PULSE_2) {
				 TCCR2 = (1 << CS21) | (1 << CS22); // 256
			 } else {
				 state = -3;
				 break;
			 }
		case 0:
			state = 1;
			break;
		case 1: // start
			if (tcnt >= us600) {
				state = 2;
			} else {
				state = -3;
			}
			break;
		case 2: // get bit
			if (tcnt >= us1600) {
				buffer[size] |= (1 << bit_n); 
			} 
			if (++bit_n == 8) {
				bit_n = 0;
				size++;
				if (size > FRAME_LEN) {
					state = -3;
					break;
				}
			}
			state = 1;
			break;
		default:
			break;
	}
}

void ir_init(void) {
	TIMSK |= 1 << TOIE2;
		
	DDRD &= ~(1 << PD3);
	PORTD |= (1 << PD3);
		
	MCUCR |= (1 << ISC10);
	
	state = -3;
	GICR |= (1 << INT1);
};
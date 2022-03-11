// Copyright (c) 2018 nikitapnn1@gmail.com

#include <avr/io.h>
#include <myavrlib/os/timers.h>
#include "os.h"
#include "analog.h"

#define N_MAX 9

uint16_t ai[AI_MAX];
static uint16_t buf[AI_MAX][N_MAX];


void selection_sort(uint16_t* ptr) {
	for (unsigned char i = 0; i < N_MAX - 1; i++) {
		unsigned char min = i;
		for (unsigned char j = i + 1; j < N_MAX; j++) {
			if (ptr[j] < ptr[min]) {
				min = j;
			}
		}
		uint16_t tmp = ptr[i];
		ptr[i] = ptr[min];
		ptr[min] = tmp;
	}
}


void analog_init(void) {
	ADCSRA = (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2) | (1 << ADEN);
}

void analog_proc(void) {
	static uint8_t state;
	static uint8_t sel;
	static uint8_t n;
	
	switch(state) {
		case 0:
			ADMUX = sel;
			ADCSRA |= (1 << ADSC);
			n = 0;
			state = 1;
			break;
		case 1:
			if (ADCSRA & (1 << ADIF))  {
				ADCSRA |= (1 << ADIF);
				buf[sel][n] = ADCW;
				n++;
				if (n >= N_MAX) {
					timer_reset(TMR_ANALOG);
					state = 2;
				} else {
					ADCSRA |= (1 << ADSC);
				}
				break;
			}
			break;
		case 2:
			if (timer_expired(TMR_ANALOG, 150)) {
			//	uint16_t sum = 0;
			//	for (uint8_t i = 0 ; i < N_MAX; ++i) {
			//		sum += buf[sel][i];
			//	}
			//	ai[sel] = sum / N_MAX;
				selection_sort(buf[sel]);
				ai[sel] = buf[sel][5];
				
				if (++sel > (AI_MAX - 1)) {
					sel = 0;
				}
				state = 0;
			}
			break;
	};
}


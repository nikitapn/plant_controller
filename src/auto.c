// Copyright (c) 2018 nikitapnn1@gmail.com

#include "avr/io.h"
#include "auto.h"
#include "data.h"
#include "os.h"
#include <myavrlib/ds1307.h>
#include <myavrlib/fixed_point.h>
#include <myavrlib/os/timers.h>

extern FixedPoint hum;

void auto_init(void) {
	// hydro 1 h level
	DDRD &= ~(1 << PD7);
	PORTD |= (1 << PD7);
	// outputs
	DDRC = 0b11111100;
}

void auto_proc(void) {
	uint8_t tick = 0;
	if (timer_get_count(TMR_MINUTES_COUNTER) > 1000) {
		timer_reset(TMR_MINUTES_COUNTER);
		tick = 1;
		
		// light
		if ((daytime == DAY) && (config.sunset == *(uint16_t*)&ds1307_time.minute)) {
			daytime = NIGHT;
			outputs.triac_1 = 0;
		} else if ((daytime == NIGHT) && (config.dawn == *(uint16_t*)&ds1307_time.minute)) {
			daytime = DAY;
			outputs.triac_1 = 1;
		}
		if (hum.i > config.hum_h) {
			outputs.triac_2 = 0;
		}
		if (hum.i < config.hum_l) {
			outputs.triac_2 = 1;
		}
	}
	
	for(uint8_t ix = 0; ix < 1; ++ix) {
		hydro_t* h = &hydro[ix];
		
		if (tick && h->fad_sec_untill_next_state) {
			h->fad_sec_untill_next_state--;
		}
		
		if (h->manual_override) continue;
		
		switch(h->mode) {
			case HM_EAF: 
			{
				if (h->fad_state == FAD_FLOW) {
					if (h->fad_sec_untill_next_state == 0) {
						h->water_pump_state = 0;	
						h->fad_sec_untill_next_state = (uint32_t)h->fad_ebb_time * 60;
						h->fad_state = FAD_EBB;
					}
				} else if (h->fad_state == FAD_EBB) {
					if (h->fad_sec_untill_next_state == 0) {
						h->water_pump_state = 1;
						h->fad_sec_untill_next_state = (uint32_t)h->fad_flow_time * 60;
						h->fad_state = FAD_FLOW;
					}
				} else {
					// error state do nothing
				}
				break;
			}
			case HM_NFT:
			case HM_DWC:
			break;
		}
	}
}

void input_proc(void) {
	if (timer_get_count(TMR_INPUT) < 10) return;
	// hydro_1 level
	inputs.alarm = ((PIND & (1 << PD7)) ? 0 : 1);  
	timer_reset(TMR_INPUT);
}

void output_proc(void) {
	if (inputs.alarm) {
		OCR1AL = 0;
		OCR1AH = 0;
		return;
	}
	PORTC = outputs.triacs << 2;
	if (hydro[0].water_pump_state) {
		OCR1AL = hydro[0].water_pump_flow_rate;
	} else {
		OCR1AL = 0;
	}
}
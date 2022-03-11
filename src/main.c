// Copyright (c) 2018 nikitapnn1@gmail.com

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <stdio.h>
#include <util/delay.h>
#include <myavrlib/uart.h>
#include <myavrlib/dht22.h>
#include <myavrlib/lcd_12864zw.h>
#include <myavrlib/ds18b20.h>
#include <myavrlib/ds1307.h>
#include <myavrlib/os/messages.h>
#include <myavrlib/os/timers.h>
//#include <myavrlib/modbus.h>
#include <myavrlib/i2c.h>
#include "os.h"
#include "ir.h"
#include "lcd_menu.h"
#include "data.h"
#include "auto.h"
#include "analog.h"

FixedPoint hum, temp, ds_temp;

static uint8_t encoder_state;

static void sensors_proc(void) {
	if (timer_expired(TMR_SENSORS, 1000)) {
		dht22_get(&hum, &temp);
//		ds18b20_get(&ds_temp);
//		ds18b20_begin_conversion();
	}
}

static void encoder_proc(void)
{
	if (timer_get_count(TMR_ENCODER_TASK) < 1) return;
	// 0 - 
	// 0xFF - 
	static uint8_t table[16] = {
		0,					// 0000
		ENCODER_R_LEFT,		// 0001
		ENCODER_R_RIGHT,	// 0010
		0xFF,				// 0011
		ENCODER_R_RIGHT,	// 0100
		0,					// 0101
		0xFF,				// 0110
		ENCODER_R_LEFT,		// 0111
		ENCODER_R_LEFT,		// 1000
		0xFF,				// 1001
		0,					// 1010
		ENCODER_R_RIGHT,	// 1011
		0xFF,				// 1100
		ENCODER_R_RIGHT,	// 1101
		ENCODER_R_LEFT,		// 1110
		0					// 1111
	};
	
	encoder_state |= (PINB & 0x03);
	uint8_t dir = table[encoder_state];
	encoder_state = ((encoder_state & 0x03) << 2);
	
	if ((dir == ENCODER_R_RIGHT) || (dir == ENCODER_R_LEFT)) {
		message_send(MSG_REMOTE_BUTTON_DOWN, dir);
	}
	
	timer_reset(TMR_ENCODER_TASK);
}


void recalc_daytime(void) {
	uint16_t time =  *(uint16_t*)&ds1307_time.minute;
	if ((time >= config.dawn) && (time < config.sunset)) {
		daytime = DAY;
		outputs.triac_1 = 1;
	} else {
		daytime = NIGHT;
		outputs.triac_1 = 0;
	}
}

void prepare_ds1307_i2c(void) {
	twi_req_t r;
	r.dev_addr = 0xD0;
	r.addr = 0x00;
	r.len = sizeof(ds1307_t);
	twi_read_bytes(&r);	
}

void ds1307_proc(void) {
	static uint8_t task_id = 0xFF;
	if (task_id != 0xFF) {
		if (twi_get_data(task_id, (void*)&ds1307_time, sizeof(ds1307_t)) >= TWI_DONE) {
			task_id = 0xFF;
		}
	} else {
		if (timer_expired(TMR_DS1307, 1000)) {
			task_id = twi_add_task(&prepare_ds1307_i2c);
		}
	}
}

int main(void)
{
	load_data();
	
	twi_init();
	
//	sei();
//	uint8_t task_id = twi_add_task(&prepare_ds1307_i2c);
//	twi_proc();
//	while (twi_get_data(task_id, (void*)&ds1307_time, sizeof(ds1307_t)) != TWI_DONE);
//	recalc_daytime();
//	cli();
	
	timers_init();
	messages_init();
	dht22_get(&hum, &temp);
//	ds18b20_begin_conversion();
	// logic
	analog_init();
	auto_init();
	ir_init();
	menu_init();
//	modbus_init();
	// PWM pumps
	// phase correct PWM 8-bits 31.25 kHz
	DDRD |= (1 << PD5) | (1 << PD4);
	TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);
	TCCR1B = (1 << CS10);
	OCR1AL = hydro[0].water_pump_flow_rate;
	OCR1BL = 0;
	// encoder init
	DDRB &= ~((1 << PB0) | (1 << PB1));
	PORTB |= (1 << PB0) | (1 << PB1);
	encoder_state = ((PINB & 0x03) << 2);
	sei();
	wdt_enable(WDTO_2S);
	while (1) {
		timers_update();
		input_proc();
		analog_proc();
		sensors_proc();
		encoder_proc();
		menu_proc();
		auto_proc();
//		ds1307_proc();
		output_proc();
		twi_proc();
//		modbus_proc();
		messages_proc();
		wdt_reset();
	}
}
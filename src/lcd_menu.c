// Copyright (c) 2018 nikitapnn1@gmail.com

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <myavrlib/fixed_point.h>
#include <myavrlib/lcd_12864zw.h>
#include <myavrlib/macro.h>
#include <myavrlib/ds1307.h>
#include <myavrlib/os/messages.h>
#include <myavrlib/os/timers.h>
#include "os.h"
#include "lcd_menu.h"
#include "ir.h"
#include "data.h"
#include "analog.h"

#define LCD_BACKLIGHT_DDR			DDRB
#define LCD_BACKLIGHT_PORT		PORTB
#define LCD_BACKLIGHT_PIN			PB2


FixedPoint solution_1_ph, solution_1_ph_la, solution_1_ph_ha;
extern FixedPoint hum, temp, ds_temp;
extern uint8_t buzzer;

extern void recalc_daytime(void);
extern void adcvoltagetoa(uint16_t value, char* dest);

static const char ps_on[] PROGMEM = "ON";
static const char ps_off[] PROGMEM = "OFF";
static const char ps_true[] PROGMEM = "T";
static const char ps_false[] PROGMEM = "F";

static char buf[17];
static uint8_t cell_pos;
static menu_node_t const* current_node;

void dht22_humidity(void) {
	fixtoa(hum, buf, 17);
}

void dht22_temperature(void) {
	fixtoa(temp, buf, 17);
}

// date

void show_time(void) {
	buf[0] = ((ds1307_time.hour >> 4) & 0xf) + '0';
	buf[1] = (ds1307_time.hour & 0xf) + '0';
	buf[2] = ':';
	buf[3] = ((ds1307_time.minute >> 4) & 0xf) + '0';
	buf[4] = (ds1307_time.minute & 0xf) + '0';
	buf[5] = ':';
	buf[6] = ((ds1307_time.second >> 4) & 0xf) + '0';
	buf[7] = (ds1307_time.second & 0xf) + '0';
	buf[8] = '\0';
}

void show_date(void) {
	buf[0] = ((ds1307_time.date >> 4) & 0xf) + '0';
	buf[1] = (ds1307_time.date & 0xf) + '0';
	buf[2] = '.';
	buf[3] = ((ds1307_time.month >> 4) & 0xf) + '0';
	buf[4] = (ds1307_time.month & 0xf) + '0';
	buf[5] = '.';
	buf[6] = ((ds1307_time.year >> 4) & 0xf) + '0';
	buf[7] = (ds1307_time.year & 0xf) + '0';
	buf[8] = '\0';
}

void show_date_full(void) {
	buf[0] = (ds1307_time.day & 0xf) + '0';
	buf[1] = '.';
	buf[2] = ((ds1307_time.date >> 4) & 0xf) + '0';
	buf[3] = (ds1307_time.date & 0xf) + '0';
	buf[4] = '.';
	buf[5] = ((ds1307_time.month >> 4) & 0xf) + '0';
	buf[6] = (ds1307_time.month & 0xf) + '0';
	buf[7] = '.';
	buf[8] = ((ds1307_time.year >> 4) & 0xf) + '0';
	buf[9] = (ds1307_time.year & 0xf) + '0';
	buf[10] = '\0';
}

uint8_t find_firts_whitespace_or_null(void) {
	uint8_t ix = 0;
	for (char* p = buf; (*p != ' ') && *p; ++p) {
		if (++ix == 17) return 0xFF;
	}
	return ix;
}

uint8_t set_null(void) {
	uint8_t pos = find_firts_whitespace_or_null();
	if (pos != 0xFF) {
		buf[pos] = '\0';
		return 1;
	} else {
		buf[16] = '\0';
		return 0;
	}
}


uint8_t menu_validate_date(void) {
	return set_null() ? !ds1307_set_date(buf) : 0;
}
uint8_t menu_validate_time(void) {
	return set_null() ? !ds1307_set_time(buf) : 0;
}

void solution_1_tb(void) {
	fixtoa(ds_temp, buf, 17);
}

//
void solution_1_air_pump_state(void) {
	if (PORTA & 0x02) memcpy(buf, ps_on, sizeof(ps_on));
	else  memcpy(buf, ps_off, sizeof(ps_off));
}
void solution_1_air_pump_on(void) { }
void solution_1_air_pump_off(void) {  }

// solution 1 Ph
void solution_1_ph_f(void) {
	fixtoa(solution_1_ph, buf, 17);
}
void solution_1_ph_la_f(void) {
	fixtoa(solution_1_ph_la, buf, 17);
}
void solution_1_ph_ha_f(void) {
	fixtoa(solution_1_ph_ha, buf, 17);
}
uint8_t solution_1_validate_ph_la_f(void) {
	return atofp(buf, &solution_1_ph_la);
}
uint8_t solution_1_validate_ph_ha_f(void) {
	return atofp(buf, &solution_1_ph_ha);
}


void solution_1_ec_f(void) {
	uint16_t tmp = ai[AI_S1EC] - 6;
	adcvoltagetoa(tmp, buf);
}


// water pump state
void solution_1_water_pump_state(void) {
	if (hydro[0].water_pump_state) {
		memcpy(buf, ps_on, sizeof(ps_on));
	} else {
		memcpy(buf, ps_off, sizeof(ps_off));
	}
}
void solution_1_water_pump_on(void) { 
	hydro[0].water_pump_state = 1;
}
void solution_1_water_pump_off(void) { 
	hydro[0].water_pump_state = 0;  
}
// solution water pump flow rate
void solution_1_water_pump_flow_rate_f(void) {
	utoa(hydro[0].water_pump_flow_rate, buf, 10);
}
uint8_t validate_water_pump_flow_rate(void) {
	hydro[0].water_pump_flow_rate = atoi(buf);
	eeprom_write_byte(&e_hydro[0].water_pump_flow_rate, hydro[0].water_pump_flow_rate);
	return 1;
}

// ovveride 
void solution_1_ovveride_state(void) {
	if (hydro[0].manual_override) memcpy(buf, ps_true, sizeof(ps_true));
	else memcpy(buf, ps_false, sizeof(ps_false));
}
void solution_1_ovveride_yes(void) {
	hydro[0].manual_override = 1;
	eeprom_write_byte(&e_hydro[0].manual_override, hydro[0].manual_override);
}
void solution_1_ovveride_no(void) {
	hydro[0].manual_override = 0;
	eeprom_write_byte(&e_hydro[0].manual_override, hydro[0].manual_override);
}
	
void water_pump_1_encoder_input(uint8_t dir) {
	if (dir == ENCODER_R_RIGHT) {
		if (hydro[0].water_pump_flow_rate < 255) hydro[0].water_pump_flow_rate++;
	} else {
		if (hydro[0].water_pump_flow_rate > 0) hydro[0].water_pump_flow_rate--;
	}
	solution_1_water_pump_flow_rate_f();
	if (hydro[0].water_pump_state) {
		OCR1AL = hydro[0].water_pump_flow_rate;
	}
}
// high level
void config_error_code(void) {
	if (inputs.alarm) {
		memcpy(buf, ps_true, sizeof(ps_true));
	} else {
		memcpy(buf, ps_false, sizeof(ps_false));
	}
}
// outputs
void config_outputs(void) {
	uint8_t o = outputs.triacs;
	for (uint8_t i = 0; i < 6; ++i) {
		buf[i] = (o & 0x1) + '0';
		o >>= 1;
	}
	buf[6] = '\0';
}
// fad state
void solution_1_fad_state(void) {
	if (hydro[0].fad_state == FAD_EBB)			buf[0] = 'E';
	else if (hydro[0].fad_state == FAD_FLOW)	buf[0] = 'F';
	else										buf[0] = ' ';
	buf[1] = '\0';
}

void solution_1_fad_next_state_in(void) {
	if (hydro[0].mode == HM_EAF) {
		utoa(hydro[0].fad_sec_untill_next_state, buf, 10);
	} else {
		buf[0] = '\0';
	}
}
// ebb time
void solution_1_fad_ebb_time(void) {
	utoa(hydro[0].fad_ebb_time, buf, 10);
}

uint8_t validate_solution_1_fad_ebb_time(void) {
	hydro[0].fad_ebb_time = atoi(buf);
	eeprom_write_byte(&e_hydro[0].fad_ebb_time, hydro[0].fad_ebb_time);
	return 1;
}

// flow time
void solution_1_fad_flow_time(void) {
	utoa(hydro[0].fad_flow_time, buf, 10);
}

uint8_t validate_solution_1_fad_flow_time(void) {
	hydro[0].fad_flow_time = atoi(buf);
	eeprom_write_byte(&e_hydro[0].fad_flow_time, hydro[0].fad_flow_time);
	return 1;
}

// stub
void no_encoder_input(uint8_t unused) {}


// config dawn 
void show_dawn(void) {
	buf[0] = ((config.dawn_hours >> 4) & 0xf) + '0';
	buf[1] = (config.dawn_hours & 0xf) + '0';
	buf[2] = ':';
	buf[3] = ((config.dawn_minutes >> 4) & 0xf) + '0';
	buf[4] = (config.dawn_minutes & 0xf) + '0';
	buf[5] = '\0';
}

uint8_t validate_config_dawn(void) {
	if (!set_null()) return 0;
	uint8_t tmp[2] = {};
	if (parse_string(buf, 2, ':', tmp)) return 0;
	config.dawn_hours = tmp[0];
	config.dawn_minutes = tmp[1];
	eeprom_write_word(&e_config.dawn, config.dawn);
	recalc_daytime();
	return 1;
}

// config sunset
void show_sunset(void) {
	buf[0] = ((config.sunset_hours >> 4) & 0xf) + '0';
	buf[1] = (config.sunset_hours & 0xf) + '0';
	buf[2] = ':';
	buf[3] = ((config.sunset_minutes >> 4) & 0xf) + '0';
	buf[4] = (config.sunset_minutes & 0xf) + '0';
	buf[5] = '\0';
}

uint8_t validate_config_sunset(void) {
	if (!set_null()) return 0;
	uint8_t tmp[2] = {};
	if (parse_string(buf, 2, ':', tmp)) return 0;
	config.sunset_hours = tmp[0];
	config.sunset_minutes = tmp[1];
	eeprom_write_word(&e_config.sunset, config.sunset);
	recalc_daytime();
	return 1;
}

uint8_t validate_i_number(uint8_t* dest, uint8_t ll, uint8_t hl) {
	uint8_t num = atoi(buf);
	if (num >= ll && num <= hl) {
		*dest = num;
		return 1;
	}
	return 0;
}

// config humidity
void show_hum_h(void) {
	utoa(config.hum_h, buf, 10);
}

uint8_t validate_hum_h(void) {
	uint8_t ok = validate_i_number(&config.hum_h, 0, 100);
	if (ok) {
		eeprom_write_byte(&e_config.hum_h, config.hum_h);
	}
	return ok;
}

void show_hum_l(void) {
	utoa(config.hum_l, buf, 10);
}

uint8_t validate_hum_l(void) {
	uint8_t ok =validate_i_number(&config.hum_l, 0, 100);
	if (ok) {
		eeprom_write_byte(&e_config.hum_l, config.hum_l);
	}
	return ok;
}


extern const menu_node_t mn_top;
extern const menu_node_t mn_root;
extern const menu_node_t mn_top_footer;
extern const menu_node_t mn_dh22_humidity;
extern const menu_node_t mn_dh22_temp;
extern const menu_node_t mn_solution_1;
extern const menu_node_t mn_solution_1_tb;
extern const menu_node_t mn_solution_1_ph;
extern const menu_node_2_t mn_solution_1_ph_la;
extern const menu_node_2_t mn_solution_1_ph_ha;
extern const menu_node_2_t mn_solution_1_water_pump_flow_rate;
extern const menu_node_2_t mn_set_dawn;
extern const menu_node_2_t mn_set_sunset;

extern const menu_node_t mn_solution_1_ec;
extern const menu_node_t mn_solution_1_water_pump_head;
extern const menu_node_t mn_solution_1_water_pump;
extern const menu_node_t mn_solution_1_water_pump_on;
extern const menu_node_t mn_solution_1_water_pump_off;
extern const menu_node_t mn_solution_1_ovveride;
extern const menu_node_t mn_solution_1_ovveride_yes;
extern const menu_node_t mn_solution_1_ovveride_no;

extern const menu_node_t mn_solution_1_air_pump;
extern const menu_node_t mn_solution_1_air_pump_on;
extern const menu_node_t mn_solution_1_air_pump_off;

extern const menu_node_t mn_solution_1_fad_config;
	extern const menu_node_t	mn_solution_1_fad_state;
	extern const menu_node_2_t	mn_solution_1_fad_flow_time;
	extern const menu_node_2_t	mn_solution_1_fad_ebb_time;
	extern const menu_node_t	mn_solution_1_fad_next_state_in;

extern const menu_node_t mn_solution_2;
extern const menu_node_t mn_solution_3;
extern const menu_node_t mn_config;
	extern const menu_node_t mn_config_outputs;
	extern const menu_node_t mn_config_alarm_code;
	extern const menu_node_2_t mn_set_date;
	extern const menu_node_2_t mn_set_time;
	extern const menu_node_2_t mn_set_hum_h;
	extern const menu_node_2_t mn_set_hum_l;
extern const menu_node_t mn_about;
extern const menu_node_t mn_time;
extern const menu_node_t mn_date;




static const char ps_top[] PROGMEM = "################";
static const char ps_menu[] PROGMEM = "0. MENU:";
static const char ps_solution_1[] PROGMEM = "1. Solution 1";
static const char ps_solution_2[] PROGMEM = "2. Solution 2";
static const char ps_config[] PROGMEM = "3. Config";
static const char ps_alarm_code[] PROGMEM = "1. ALARM: ";
static const char ps_outputs[] PROGMEM =	"2. OUT: ";
static const char ps_date[] PROGMEM =		"3. D: ";
static const char ps_time[] PROGMEM =		"4. T: ";
static const char ps_dawn[] PROGMEM =		"5. DAWN:   ";
static const char ps_sunset[] PROGMEM =		"6. SUNSET: ";
static const char ps_hum_h[] PROGMEM =		"7. HUM H:  ";
static const char ps_hum_l[] PROGMEM =		"8. HUM L:  ";

static const char ps_about[] PROGMEM = "4. About";
static const char ps_t1[] PROGMEM =					"1. Tb: ";
static const char ps_ph[] PROGMEM =					"2. Ph: ";
static const char ps_ec[] PROGMEM =					"3. EC: ";
static const char ps_water_pump_static[] PROGMEM =	"4. WATER PUMP";
static const char ps_air_pump[] PROGMEM =			"5. AIR PUMP: ";
static const char ps_override[] PROGMEM =			"6. MANUAL: ";
static const char ps_fad_config[] PROGMEM =			"7. CONFIG E&F";

static const char ps_water_pump[] PROGMEM =			  "1. STATE:  ";
static const char ps_water_pump_flow_rate[] PROGMEM = "2. F RATE: ";

static const char ps_state[] PROGMEM =				"1. STATE:  ";
static const char ps_fad_flow_time[] PROGMEM =		"2. FLOW:   ";
static const char ps_fad_ebb_time[] PROGMEM =		"3. EBB:    ";
static const char ps_fad_flooding_in[] PROGMEM =	"4. CH. IN: ";
static const char ps_on2[] PROGMEM = "1. ON";
static const char ps_off2[] PROGMEM = "2. OFF";
static const char ps_yes[] PROGMEM = "1. YES";
static const char ps_no[] PROGMEM = "2. NO";
static const char ps_ph_la[] PROGMEM = "Ph LA: ";
static const char ps_ph_ha[] PROGMEM = "Ph HA: ";
static const char ps_about_1[] PROGMEM = "Hydroponics    ";
static const char ps_about_2[] PROGMEM = "     controller";
static const char ps_about_3[] PROGMEM = "made by        ";
static const char ps_about_4[] PROGMEM = "barbarian from ";
static const char ps_about_5[] PROGMEM = "east 2018 A.D. ";
static const char ps_dht22_humidity[] PROGMEM =		"HUM:    ";
static const char ps_dht22_temperature[] PROGMEM =	"TEMP:   ";
static const char ps_top_scren_time[] PROGMEM =		"TIME:  ";
static const char ps_top_scren_date[] PROGMEM =		"DATE:  ";

// top screen
MAKE_MENU_NODE(MENU_TOP_SCREEN, mn_top, ps_top, NULL, NULL, &mn_dh22_humidity, &mn_dh22_temp, &mn_time, &mn_date);
MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_dh22_humidity, ps_dht22_humidity, &dht22_humidity, NULL);
MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_dh22_temp, ps_dht22_temperature, &dht22_temperature, NULL);
MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_time, ps_top_scren_time, &show_time, NULL);
MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_date, ps_top_scren_date, &show_date, NULL);
// menu tree
MAKE_MENU_NODE(MENU_STATIC, mn_root, ps_menu, NULL, &mn_top, &mn_solution_1, &mn_solution_2, &mn_config, &mn_about); 

// Solution 1
	MAKE_MENU_NODE(MENU_STATIC, mn_solution_1, ps_solution_1, NULL, &mn_root,
		&mn_solution_1_tb, &mn_solution_1_ph, &mn_solution_1_ec, &mn_solution_1_water_pump_head, &mn_solution_1_air_pump, &mn_solution_1_ovveride, &mn_solution_1_fad_config);
	// Solution 1 items
		MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_solution_1_tb, ps_t1, &solution_1_tb, &mn_solution_1);
		MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_solution_1_ph, ps_ph, &solution_1_ph_f, &mn_solution_1, &mn_solution_1_ph_la, &mn_solution_1_ph_ha);
			MAKE_MENU_NODE_STATIC_VALUE(mn_solution_1_ph_la, ps_ph_la, &solution_1_ph_la_f, &solution_1_validate_ph_la_f, &no_encoder_input, &mn_solution_1_ph);
			MAKE_MENU_NODE_STATIC_VALUE(mn_solution_1_ph_ha, ps_ph_ha, &solution_1_ph_ha_f, &solution_1_validate_ph_ha_f, &no_encoder_input, &mn_solution_1_ph);
		MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_solution_1_ec, ps_ec, &solution_1_ec_f, &mn_solution_1);
		// water pump solution 1
		
		MAKE_MENU_NODE(MENU_STATIC, mn_solution_1_water_pump_head, ps_water_pump_static, NULL, &mn_solution_1, &mn_solution_1_water_pump, &mn_solution_1_water_pump_flow_rate);
			MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_solution_1_water_pump, ps_water_pump,  &solution_1_water_pump_state, &mn_solution_1_water_pump_head, 
				&mn_solution_1_water_pump_on, &mn_solution_1_water_pump_off);
				MAKE_MENU_NODE(MENU_DYNAMIC_CONTROL, mn_solution_1_water_pump_on, ps_on2, &solution_1_water_pump_on, NULL);
				MAKE_MENU_NODE(MENU_DYNAMIC_CONTROL, mn_solution_1_water_pump_off, ps_off2, &solution_1_water_pump_off, NULL);
			MAKE_MENU_NODE_STATIC_VALUE(mn_solution_1_water_pump_flow_rate, ps_water_pump_flow_rate,
				&solution_1_water_pump_flow_rate_f, &validate_water_pump_flow_rate, &water_pump_1_encoder_input, &mn_solution_1_water_pump_head);
		// air pump solution 1
		MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_solution_1_air_pump, ps_air_pump,  &solution_1_air_pump_state, &mn_solution_1, 
			&mn_solution_1_air_pump_on, &mn_solution_1_air_pump_off);
		MAKE_MENU_NODE(MENU_DYNAMIC_CONTROL, mn_solution_1_air_pump_on, ps_on2, &solution_1_air_pump_on, NULL);
		MAKE_MENU_NODE(MENU_DYNAMIC_CONTROL, mn_solution_1_air_pump_off, ps_off2, &solution_1_air_pump_off, NULL);

		// ovveride
		MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_solution_1_ovveride, ps_override,  &solution_1_ovveride_state, &mn_solution_1,
			&mn_solution_1_ovveride_yes, &mn_solution_1_ovveride_no);
			MAKE_MENU_NODE(MENU_DYNAMIC_CONTROL, mn_solution_1_ovveride_yes, ps_yes, &solution_1_ovveride_yes, NULL);
			MAKE_MENU_NODE(MENU_DYNAMIC_CONTROL, mn_solution_1_ovveride_no, ps_no, &solution_1_ovveride_no, NULL);
		MAKE_MENU_NODE(MENU_STATIC, mn_solution_1_fad_config, ps_fad_config, NULL, &mn_solution_1, 
				&mn_solution_1_fad_state,  &mn_solution_1_fad_flow_time, &mn_solution_1_fad_ebb_time, &mn_solution_1_fad_next_state_in);
			MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_solution_1_fad_state, ps_state, &solution_1_fad_state, &mn_solution_1_fad_config);
			MAKE_MENU_NODE_STATIC_VALUE(mn_solution_1_fad_flow_time, ps_fad_flow_time, &solution_1_fad_flow_time, &validate_solution_1_fad_flow_time, &no_encoder_input, &mn_solution_1_fad_config);
			MAKE_MENU_NODE_STATIC_VALUE(mn_solution_1_fad_ebb_time, ps_fad_ebb_time, &solution_1_fad_ebb_time, &validate_solution_1_fad_ebb_time, &no_encoder_input, &mn_solution_1_fad_config);
			MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_solution_1_fad_next_state_in, ps_fad_flooding_in, &solution_1_fad_next_state_in, &mn_solution_1_fad_config);
// Solution 2
MAKE_MENU_NODE(MENU_STATIC, mn_solution_2, ps_solution_2, NULL, &mn_root);
// config
MAKE_MENU_NODE(MENU_STATIC, mn_config, ps_config, NULL, &mn_root, &mn_config_alarm_code, &mn_config_outputs, &mn_set_date, &mn_set_time, &mn_set_dawn, &mn_set_sunset, &mn_set_hum_h, &mn_set_hum_l);
	// alarm
	MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_config_alarm_code, ps_alarm_code, &config_error_code, &mn_config);
	MAKE_MENU_NODE(MENU_DYNAMIC_VALUE, mn_config_outputs, ps_outputs, &config_outputs, &mn_config);
	MAKE_MENU_NODE_STATIC_VALUE(mn_set_date, ps_date, &show_date_full, &menu_validate_date, &no_encoder_input, &mn_config);
	MAKE_MENU_NODE_STATIC_VALUE(mn_set_time, ps_time, &show_time, &menu_validate_time, &no_encoder_input, &mn_config);
	MAKE_MENU_NODE_STATIC_VALUE(mn_set_dawn, ps_dawn, &show_dawn, &validate_config_dawn, &no_encoder_input, &mn_config);
	MAKE_MENU_NODE_STATIC_VALUE(mn_set_sunset, ps_sunset, &show_sunset, &validate_config_sunset, &no_encoder_input, &mn_config);
	MAKE_MENU_NODE_STATIC_VALUE(mn_set_hum_h, ps_hum_h, &show_hum_h, &validate_hum_h, &no_encoder_input, &mn_config);
	MAKE_MENU_NODE_STATIC_VALUE(mn_set_hum_l, ps_hum_l, &show_hum_l, &validate_hum_l, &no_encoder_input, &mn_config);
// about
MAKE_MENU_NODE(MENU_STATIC, mn_about_1, ps_about_1, NULL, &mn_about);
MAKE_MENU_NODE(MENU_STATIC, mn_about_2, ps_about_2, NULL, &mn_about);
MAKE_MENU_NODE(MENU_STATIC, mn_about_3, ps_about_3, NULL, &mn_about);
MAKE_MENU_NODE(MENU_STATIC, mn_about_4, ps_about_4, NULL, &mn_about);
MAKE_MENU_NODE(MENU_STATIC, mn_about_5, ps_about_5, NULL, &mn_about);
MAKE_MENU_NODE(MENU_STATIC, mn_about, ps_about, NULL, &mn_root, 
	&mn_about_1, &mn_about_2, &mn_about_3, &mn_about_4, &mn_about_5);

static uint8_t screen_y_offset, screen_y_offset_prev;
static uint8_t cursor_y = 1, cursor_y_prev = 1, cursor_x;
static uint8_t dynamic_screen;

void print_static_value_and_set_cursor(uint8_t padding)
{
	lcd_12864zw_set_cursor(0, 1);
	const custom_function_t fn = (void*)pgm_read_word(&current_node->fn);
	if (fn != NULL) {
		fn();
		if (padding == 1) {
			uint8_t len = strlen(buf);
			memset(buf + len, 0x20, 17 - len);
			buf[16] = '\0';
		}
		lcd_12864zw_print(buf);
	}
	lcd_12864zw_set_cursor(0, 1);
	cell_pos = cursor_x = 0;
}

void show_menu(void)  {
	uint8_t menu_type = pgm_read_byte(&current_node->menu_type);
	if (menu_type == MENU_STATIC_VALUE) return;
	
	dynamic_screen = 0;
	uint8_t childs_count = pgm_read_byte(&current_node->childs_count);
	
	uint8_t row, i;
	
	lcd_12864zw_clear();
	if (screen_y_offset == 0) {
		lcd_12864zw_set_cursor(0, 0);
		const char* text = (void*)pgm_read_word(&current_node->text) + 1; // ignore #
		lcd_12864zw_print_p(text);
		i = 0;
		row = 1;
	} else {
		i = screen_y_offset - 1;
		row = 0;
	}
	
	if (childs_count) {
		for (;(i < childs_count) && (row < 4); ++i, ++row) {
			lcd_12864zw_set_cursor(0, row);
			menu_node_t* child = (menu_node_t*)pgm_read_word(&current_node->childs[i]);
			const char* text = (void*)pgm_read_word(&child->text);
			lcd_12864zw_print_p(text);
			const custom_function_t fn = (void*)pgm_read_word(&child->fn);
			uint8_t child_type = pgm_read_byte(&child->menu_type);
			if ((child_type != MENU_DYNAMIC_CONTROL) && (fn != NULL))  {
				fn();
				lcd_12864zw_print(buf);
				dynamic_screen = 1;
			}
		}
	}
	lcd_12864zw_set_cursor(0, cursor_y);
}

void menu_init(void) {
	lcd_12864zw_init();
	lcd_12864zw_set_mode(LCD_12864_DISPLAY_ON | LCD_12864_CURSOR_ON);
	current_node = &mn_top;
	show_menu();
	
	LCD_BACKLIGHT_DDR |= (1 << LCD_BACKLIGHT_PIN);
	LCD_BACKLIGHT_PORT |= (1 << LCD_BACKLIGHT_PIN);
}

static void setup_new_screen(void const* child) {
	current_node = child;
	cursor_y_prev = cursor_y;
	screen_y_offset_prev = screen_y_offset;
	cursor_y = 1;
	screen_y_offset = 0;
}

static void return_to_parent(void) {
	lcd_12864zw_set_mode(LCD_12864_DISPLAY_ON | LCD_12864_CURSOR_ON);
	menu_node_t* parent = (menu_node_t*)pgm_read_word(&current_node->parent);
	if (parent != NULL) { 
		current_node = parent;
		cursor_y = cursor_y_prev;
		screen_y_offset = screen_y_offset_prev;
		cursor_y_prev = 1;
		screen_y_offset_prev = 0;
	}
}

static void move_cursor_left(void) {
	cell_pos = 0;
	if (cursor_x == 0) return;
	cursor_x--;
	lcd_12864zw_set_cursor(cursor_x, 1);
}

static void move_cursor_right(void) {
	cell_pos = 0;
	if (cursor_x == 7) return;
	cursor_x++;
	lcd_12864zw_set_cursor(cursor_x, 1);
}

static void enter_char(char c) {
	buf[cursor_x * 2 + cell_pos] = c;
	lcd_12864zw_print_cell(buf + cursor_x * 2);
	lcd_12864zw_set_cursor(cursor_x, 1);
	cell_pos++;
	if (cell_pos == 2) {
		move_cursor_right();
	}
}
static void handle_button(uint8_t code)
{
	uint8_t childs_count = pgm_read_byte(&current_node->childs_count);
	uint8_t menu_type = pgm_read_byte(&current_node->menu_type);
	
	switch(code)
	{
		case VK_UP:
		{
			if (childs_count) {
				if (screen_y_offset == 0 && cursor_y == 1) {
				
				} else if (cursor_y == 1 && screen_y_offset != 0) {
					screen_y_offset--;
				} else {
					cursor_y--;
					lcd_12864zw_set_cursor(0, cursor_y);
					return;
				}
			}
			break;
		}
		case VK_DOWN:
		{
			if (childs_count) {
				if (cursor_y == 3) {
					if (screen_y_offset + 3 < childs_count) screen_y_offset++;
				} else {
					if (screen_y_offset + cursor_y < childs_count) {
						cursor_y++;
						lcd_12864zw_set_cursor(0, cursor_y);
					}
					return;
				}
			}
			break;
		}
		case VK_BACK: 
		{
			return_to_parent();
			break;
		}
		case VK_ENTER:
		{
			if (menu_type == MENU_TOP_SCREEN) {
				setup_new_screen(&mn_root);
			} else if (menu_type == MENU_STATIC_VALUE) { 
				const validate_function_t validate_fn = (void*)pgm_read_word(&((menu_node_2_t*)current_node)->fn_validate);
				if (validate_fn()) {
					return_to_parent();
				}
			} else if (childs_count) {
				uint8_t ix = screen_y_offset + cursor_y - 1;
				if (ix == 255) return;
				if (ix < childs_count) {
					menu_node_t* child = (menu_node_t*)pgm_read_word(&current_node->childs[ix]);
					uint8_t child_menu_type = pgm_read_byte(&child->menu_type);					
					if (child_menu_type == MENU_STATIC_VALUE) {
						setup_new_screen(child);
						lcd_12864zw_set_mode(LCD_12864_DISPLAY_ON | LCD_12864_CURSOR_ON | LCD_12864_BLINK_ON);
						lcd_12864zw_clear();
						lcd_12864zw_set_cursor(0, 0);
						const char* text = (void*)pgm_read_word(&current_node->text);
						lcd_12864zw_print_p(text);
						print_static_value_and_set_cursor(0);
						return;
					}
					uint8_t child_childs_count = pgm_read_byte(&child->childs_count);
					uint8_t child_type = pgm_read_byte(&child->menu_type);
					if (child_childs_count) {
						setup_new_screen(child);
					} else if (child_type == MENU_DYNAMIC_CONTROL) {
						const custom_function_t fn = (void*)pgm_read_word(&child->fn);
						if (fn != NULL) {
							fn();
							return_to_parent();
						}					
					} else {
						// do nothing
					}
				}
			} else {
				
			}
			break;
		}
		case VK_LEFT: 
		{
			if (menu_type == MENU_STATIC_VALUE) move_cursor_left();
			return;
		}
		case VK_RIGHT:
		{
			if (menu_type == MENU_STATIC_VALUE) move_cursor_right();
			return;
		}
		case  VK_RECALL:
		{
			if (menu_type == MENU_STATIC_VALUE) {
				cell_pos = cursor_x = 0;
				memset(buf,  0x20, 16);
				buf[16] = '\0';
				lcd_12864zw_set_cursor(0, 1);
				lcd_12864zw_print(buf);
				lcd_12864zw_set_cursor(0, 1);
			}
			return;
		}
		case  VK_FAV:
		{
			if (menu_type == MENU_STATIC_VALUE) {
				enter_char('.');
			}
			return;
		}
		case ENCODER_R_LEFT:
		case ENCODER_R_RIGHT:
		{
			if (menu_type == MENU_STATIC_VALUE) {
				const encoder_function_t encoder_fn = (void*)pgm_read_word(&((menu_node_2_t*)current_node)->fn_encoder);
				encoder_fn(code);
				print_static_value_and_set_cursor(1);
			}
			break;
		}
		case VK_PLAY:
		{
			enter_char(':');
			break;
		}
		default: 
		{
			if ((menu_type == MENU_STATIC_VALUE) && ((code >= VK_0) && (code <= VK_9))) {
				enter_char('0' + (code - VK_0));
			}
			return;
		}
	};
	show_menu();
}

void menu_proc(void) {
	uint8_t code;
	static uint8_t display_on =1;
	if (message_get(MSG_REMOTE_BUTTON_DOWN, &code)) {
		LCD_BACKLIGHT_PORT |= (1 << LCD_BACKLIGHT_PIN);
		display_on = 1;
		timer_reset(TMR_LCD_BACKLIGHT);
		handle_button(code);
	} else if (display_on && dynamic_screen) {
		if (timer_get_count(TMR_SCREEN_UPDATE) > 1000) {
			timer_reset(TMR_SCREEN_UPDATE);
			show_menu();
		}
	}
	if ((LCD_BACKLIGHT_PORT & (1 << LCD_BACKLIGHT_PIN)) && 
		timer_get_count(TMR_LCD_BACKLIGHT) > 60000) {
		display_on = 0;
		LCD_BACKLIGHT_PORT &= ~(1 << LCD_BACKLIGHT_PIN);
	}
}

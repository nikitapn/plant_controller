// Copyright (c) 2018 nikitapnn1@gmail.com

#ifndef DATA_H_
#define DATA_H_

#include <avr/eeprom.h>

#define HM_DWC			0
#define HM_EAF			1
#define HM_NFT			2

#define FAD_EBB			1
#define FAD_FLOW		2


typedef struct {
	uint8_t water_pump_state; // 0 - off, 1 - on
	uint8_t water_pump_flow_rate; // 0 - 255
	uint8_t fad_flow_time; // minutes
	uint8_t fad_ebb_time; // minutes
	uint8_t fad_state;
	uint8_t manual_override;
	uint8_t mode; // HM_DWC, HM_FAD, HM_NFT
	// eeprom zeros
	uint32_t fad_sec_untill_next_state;
} hydro_t;

typedef struct {
	uint8_t alarm;
} inputs_t;

typedef struct inputs_s {
	union {
		struct {
			uint8_t triac_1 : 1;
			uint8_t triac_2 : 1;
			uint8_t triac_3 : 1;
			uint8_t triac_4 : 1;
			uint8_t triac_5 : 1;
			uint8_t triac_6 : 1;	
		};
		uint8_t triacs;
	};
} outputs_t;


typedef struct {
	union {
		struct {
			uint8_t dawn_minutes;
			uint8_t dawn_hours;
		};
		uint16_t dawn;
	};
	union {
		struct {
			uint8_t sunset_minutes;
			uint8_t sunset_hours;
		};
		uint16_t sunset;
	};
	uint8_t hum_h;
	uint8_t hum_l;
} config_t;

void load_data(void);


extern hydro_t hydro[2];
extern hydro_t EEMEM e_hydro[2];
extern inputs_t inputs;
extern outputs_t outputs;
extern config_t EEMEM e_config;
extern config_t config;

#define DAY		0	
#define NIGHT	1

extern uint8_t daytime;


#endif // DATA_H_
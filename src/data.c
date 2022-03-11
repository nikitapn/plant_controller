// Copyright (c) 2018 nikitapnn1@gmail.com

#include "data.h"

config_t config;
hydro_t hydro[2];
inputs_t inputs;
outputs_t outputs;
uint8_t daytime;

config_t EEMEM e_config = {
	.dawn_hours = 0x07,
	.dawn_minutes = 0x00,
	.sunset_hours = 0x23,
	.sunset_minutes = 0x00,
	.hum_h = 65,
	.hum_l = 55
};

hydro_t EEMEM e_hydro[2] = { 
{
	.fad_flow_time = 10,
	.fad_ebb_time = 30,
	.water_pump_flow_rate = 80, 
	.water_pump_state = 0,
	.manual_override = 0,
	.mode = HM_EAF,
	.fad_state = FAD_EBB,
	.fad_sec_untill_next_state = 0
},
{
	
}
};


void load_data(void) {
	eeprom_read_block(&hydro[0], &e_hydro[0], sizeof(hydro_t));
	eeprom_read_block(&config, &e_config, sizeof(config_t));
}
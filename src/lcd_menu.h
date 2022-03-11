// Copyright (c) 2018 nikitapnn1@gmail.com

#ifndef _MENU_H_
#define _MENU_H_

#include <myavrlib/macro.h>

#define MENU_TOP_SCREEN					0
#define MENU_STATIC							1
#define MENU_DYNAMIC_VALUE			2
#define MENU_DYNAMIC_CONTROL		3
#define MENU_STATIC_VALUE				4

typedef void(*custom_function_t)(void);
typedef uint8_t(*validate_function_t)(void);
typedef void(*encoder_function_t)(uint8_t);

typedef struct {
	const char* text;
	const custom_function_t fn;
	const struct menu_node_t* parent;
	const uint8_t menu_type;
	const uint8_t childs_count;
	const void* childs[];
} menu_node_t;

typedef struct {
	const menu_node_t base;
	const validate_function_t fn_validate;
	const encoder_function_t fn_encoder;
} menu_node_2_t;

#define MAKE_MENU_NODE(menu_type, name, text, fn, parent, ...) \
	const menu_node_t name PROGMEM = {text, fn, (void*)parent, menu_type, GET_ARG_COUNT(__VA_ARGS__), {__VA_ARGS__}};

#define MAKE_MENU_NODE_STATIC_VALUE(name, text, fn, fn_validate, fn_encoder, parent) \
	const menu_node_2_t name PROGMEM = { {text, fn, (void*)parent, MENU_STATIC_VALUE, 0 }, fn_validate, fn_encoder };

void menu_init(void);
void menu_proc(void);

#endif /* _MENU_H_ */
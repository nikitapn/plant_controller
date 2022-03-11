// Copyright (c) 2018 nikitapnn1@gmail.com

#ifndef ANALOG_H_
#define ANALOG_H_

#define AI_MAX	2
#define AI_S1EC 0


void analog_init(void);
void analog_proc(void);

extern uint16_t ai[AI_MAX];


#endif /* ANALOG_H_ */
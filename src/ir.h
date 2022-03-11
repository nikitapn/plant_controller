// Copyright (c) 2018 nikitapnn1@gmail.com

#ifndef IR_H_
#define IR_H_

#define VK_ENTER			0x1f
#define VK_LEFT				0x03	  
#define VK_RIGHT			0x02
#define VK_DOWN				0x01
#define VK_UP					0x00
#define VK_0					0x10
#define VK_1					0x11
#define VK_2					0x12
#define VK_3					0x13
#define VK_4					0x14
#define VK_5					0x15
#define VK_6					0x16
#define VK_7					0x17
#define VK_8					0x18
#define VK_9					0x19
#define VK_BACK				0x1d
#define VK_POWER			0x0a
#define VK_SOUND			0x0c
#define VK_FAV				0x1b
#define VK_RECALL			0x1a
#define VK_LIFE				0x05
#define VK_PLAY				0x07
#define VK_MENU				0x1c
#define VK_CHANNEL_PLUS		0x08
#define VK_CHANNEL_MINUS	0x06
#define VK_VOLUME_PLUS		0x09
#define VK_VOLUME_MINUS		0x0f

#define ENCODER_R_LEFT		0xf0
#define ENCODER_R_RIGHT		0xf1

void ir_init(void);

#endif // IR_H_
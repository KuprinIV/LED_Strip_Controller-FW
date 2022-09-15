#ifndef __WS2812B_H
#define __WS2812B_H

#include "stm32f0xx_hal.h"

#define LEDS_COUNT 			60
#define EFFECTS_COUNT		7
#define BIT0				0xC0
#define BIT1				0xF8

// some colors in GRB format
#define COLOR_BLACK 		0x000000
#define COLOR_WHITE 		0xFFFFFF
#define COLOR_RED 			0x00FF00
#define COLOR_LIME 			0xFF0000
#define COLOR_BLUE 			0x0000FF
#define COLOR_YELLOW 		0xFFFF00
#define COLOR_CYAN 			0xFF00FF
#define COLOR_MAGENTA 		0x00FFFF
#define COLOR_SILVER 		0xC0C0C0
#define COLOR_GRAY 			0x808080
#define COLOR_MAROON 		0x008000
#define COLOR_OLIVE 		0x808000
#define COLOR_GREEN 		0x800000
#define COLOR_PURPLE 		0x008080
#define COLOR_TEAL 			0x800080
#define COLOR_NAVY 			0x000080
#define COLOR_ORANGE 		0xA5FF00

typedef struct
{
	uint8_t effectNum;
	uint32_t colors[4];
	uint8_t saturation;
	uint8_t value;
	uint8_t sectorSize;
}effectParamsData;

typedef struct
{
	void (*setStripColor)(uint32_t grb);
	void (*setBlinkingStrip)(uint32_t grb);
	void (*setHsvStripColor)(uint8_t h, uint8_t s, uint8_t v);
	void (*setStripPartColor)(uint32_t grb, uint8_t part_length);
	void (*setStripFourColor)(uint32_t* colors4);
	void (*setStripTwoColor)(uint32_t* colors2, uint8_t part_size);
	void (*setHsvSequence)(uint8_t value);
	void (*updateFramebuffer)(uint8_t index_offset);
}LED_StripDriver;

extern LED_StripDriver* led_strip_drv;

#endif

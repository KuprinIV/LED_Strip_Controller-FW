#ifndef __WS2812B_H
#define __WS2812B_H

#include "stm32f0xx_hal.h"

#define LEDS_COUNT 60

// some colors in GRB format
#define COLOR_BLACK 0x000000
#define COLOR_WHITE 0xFFFFFF
#define COLOR_RED 0x00FF00
#define COLOR_LIME 0xFF0000
#define COLOR_BLUE 0x0000FF
#define COLOR_YELLOW 0xFFFF00
#define COLOR_CYAN 0xFF00FF
#define COLOR_MAGENTA 0x00FFFF
#define COLOR_SILVER 0xC0C0C0
#define COLOR_GRAY 0x808080
#define COLOR_MAROON 0x008000
#define COLOR_OLIVE 0x808000
#define COLOR_GREEN 0x800000
#define COLOR_PURPLE 0x008080
#define COLOR_TEAL 0x800080
#define COLOR_NAVY 0x000080
#define COLOR_ORANGE 0xA5FF00

void setStripColor(uint32_t grb);
void setBlinkingStrip(uint32_t grb);
void setHSV_StripColor(uint8_t h, uint8_t s, uint8_t v);
void setStripPartColor(uint32_t grb, uint8_t offset, uint8_t part_length);
void setStripFourColor(uint32_t* colors4, uint8_t state);
void setStripTwoColor(uint32_t* colors2, uint8_t state, uint8_t part_size);
void setHSV_Sequence(uint8_t value);

#endif

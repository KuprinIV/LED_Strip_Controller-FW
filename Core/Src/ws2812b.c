#include "ws2812b.h"
#include <math.h>

extern SPI_HandleTypeDef hspi1;

static uint32_t HsvToRgb(uint8_t h, uint8_t s, uint8_t v);
static uint32_t RgbToHsv(uint32_t grb);

void setStripColor(uint32_t grb)
{
	static uint32_t prev_color;
	static uint16_t colorData[24] = {0};
	uint8_t temp = 0;

	// fill color data array
	if(prev_color != grb){
		prev_color = grb;
		for(uint8_t i = 0; i < 24; i++)
		{
			temp = (uint8_t)((grb >> (23 - i))&0x01);
			if(temp){
				colorData[i] = 0xFF80; // 1 code
			}else{
				colorData[i] = 0x7000; // 0 code
			}
		}
	}

	// send data to strip LED controllers
	for(uint8_t j = 0; j < LEDS_COUNT; j++){
		for(uint8_t i = 0; i < 24; i++){
			SPI1->DR = colorData[i];
			while(!(SPI1->SR & SPI_SR_TXE)){}
		}
	}
	// add delay >= 50 uS for RET code
	for(uint8_t i = 0; i < 50; i++){
		SPI1->DR = 0;
		while(!(SPI1->SR & SPI_SR_TXE)){}
	}
}

void setBlinkingStrip(uint32_t grb)
{
	static uint8_t index, dir;
	static uint32_t prev_color;
	static uint32_t hsv;

	if(prev_color != grb){
		prev_color = grb;
		hsv = RgbToHsv(grb);
	}
	setStripColor(HsvToRgb((hsv & 0xFF0000)>>16, (hsv & 0xFF00)>>8, index));
	if(!dir){
		if(index == 255){
			dir = 1;
		}else{
			index+=5;
		}
	}else{
		if(index == 0){
			dir = 0;
		}else{
			index-=5;
		}
	}
}

void setHSV_StripColor(uint8_t h, uint8_t s, uint8_t v) {
	  setStripColor(HsvToRgb(h, s, v));
}

void setHSV_Sequence(uint8_t value)
{
	static uint32_t colors[LEDS_COUNT] = {0};
	static uint8_t prev_value;
	uint16_t colorData[24] = {0};
	uint8_t temp = 0;
	// calc LEDs colors
	if(prev_value != value){
		prev_value = value;
		for(uint8_t i = 0; i < LEDS_COUNT; i++){
			colors[i] = HsvToRgb((229 + (uint8_t)255.0f/LEDS_COUNT*i) & 0xFF, 255, value);
		}
	}

	// send data to strip LED controllers
	for(uint8_t j = 0; j < LEDS_COUNT; j++){
		// fill color data array
		for(uint8_t i = 0; i < 24; i++)
		{
			temp = (uint8_t)((colors[j] >> (23 - i))&0x01);
			if(temp){
				colorData[i] = 0xFF80; // 1 code
			}else{
				colorData[i] = 0x7000; // 0 code
			}
			// send data
			SPI1->DR = colorData[i];
			while(!(SPI1->SR & SPI_SR_TXE)){}
		}
	}
	// add delay >= 50 uS for RET code
	for(uint8_t i = 0; i < 50; i++){
		SPI1->DR = 0;
		while(!(SPI1->SR & SPI_SR_TXE)){}
	}
}

void setStripPartColor(uint32_t grb, uint8_t offset, uint8_t part_length)
{
	static uint32_t prev_color;
	static uint32_t hsv_color;
	static uint8_t delta;
	static uint32_t color_grades[30] = {0}; // max part length is 30 diodes
	static uint8_t h = 0, s = 0, v = 0, part_length_prev = 0;

	volatile uint32_t temp_color = 0;
	uint8_t temp = 0;

	if(prev_color != grb || part_length != part_length_prev){
		prev_color = grb;
		part_length_prev = part_length;
		// init value difference and convert rgb to hsv
		hsv_color = RgbToHsv(grb);
		h = (hsv_color>>16) & 0xFF;
		s = (hsv_color>>8) & 0xFF;
		v = hsv_color & 0xFF;
		delta = (uint8_t)(v/part_length);

		// fill color grades array
		for(uint8_t i = 0; i < part_length; i++){
			color_grades[i] = HsvToRgb(h, s, v - (part_length - 1 - i)*delta);
		}
	}

	// send data to strip LED controllers
	for(uint8_t j = 0; j < LEDS_COUNT; j++){
		if((offset + part_length > LEDS_COUNT) && (j >= 0 && j < offset + part_length - LEDS_COUNT)){
			temp_color = color_grades[j + LEDS_COUNT - offset];
			for(uint8_t i = 0; i < 24; i++){
				temp = (uint8_t)((temp_color >> (23 - i))&0x01);
				if(temp){
					SPI1->DR = 0xFF80; // 1 code
				}else{
					SPI1->DR = 0x7000; // 0 code
				}
				while(!(SPI1->SR & SPI_SR_TXE)){}
			}
		}else if(j >= offset && j < offset + part_length){
			temp_color = color_grades[j-offset];
			for(uint8_t i = 0; i < 24; i++){
				temp = (uint8_t)((temp_color >> (23 - i))&0x01);
				if(temp){
					SPI1->DR = 0xFF80; // 1 code
				}else{
					SPI1->DR = 0x7000; // 0 code
				}
				while(!(SPI1->SR & SPI_SR_TXE)){}
			}
		}else{
			for(uint8_t i = 0; i < 24; i++){
				SPI1->DR = 0x7000; // set "black" color
				while(!(SPI1->SR & SPI_SR_TXE)){}
			}
		}
	}
	// add delay >= 50 uS for RET code
	for(uint8_t i = 0; i < 50; i++){
		SPI1->DR = 0;
		while(!(SPI1->SR & SPI_SR_TXE)){}
	}
}

void setStripFourColor(uint32_t* colors4, uint8_t state)
{
	uint16_t colorData[4][24] = {0};
	uint8_t temp = 0;

	// fill color data array
	for(uint8_t i = 0; i < 24; i++)
	{
		temp = (uint8_t)((colors4[state & 0x03] >> (23 - i))&0x01);
		if(temp){
			colorData[0][i] = 0xFF80; // 1 code
		}else{
			colorData[0][i] = 0x7000; // 0 code
		}

		temp = (uint8_t)((colors4[(state+1) & 0x03] >> (23 - i))&0x01);
		if(temp){
			colorData[1][i] = 0xFF80; // 1 code
		}else{
			colorData[1][i] = 0x7000; // 0 code
		}

		temp = (uint8_t)((colors4[(state+2) & 0x03] >> (23 - i))&0x01);
		if(temp){
			colorData[2][i] = 0xFF80; // 1 code
		}else{
			colorData[2][i] = 0x7000; // 0 code
		}

		temp = (uint8_t)((colors4[(state+3) & 0x03] >> (23 - i))&0x01);
		if(temp){
			colorData[3][i] = 0xFF80; // 1 code
		}else{
			colorData[3][i] = 0x7000; // 0 code
		}
	}

	// send data to strip LED controllers
	for(uint8_t j = 0; j < LEDS_COUNT; j++){
			for(uint8_t i = 0; i < 24; i++){
				SPI1->DR = colorData[j%4][i];
				while(!(SPI1->SR & SPI_SR_TXE)){}
			}
	}
	// add delay >= 50 uS for RET code
	for(uint8_t i = 0; i < 50; i++){
		SPI1->DR = 0;
		while(!(SPI1->SR & SPI_SR_TXE)){}
	}
}

void setStripTwoColor(uint32_t* colors2, uint8_t state, uint8_t part_size)
{
	uint16_t colorData[2][24] = {0};
	uint8_t temp = 0;
	uint8_t arrayIndex = 0;

	// fill color data array
	for(uint8_t i = 0; i < 24; i++)
	{
		temp = (uint8_t)((colors2[state & 0x01] >> (23 - i))&0x01);
		if(temp){
			colorData[0][i] = 0xFF80; // 1 code
		}else{
			colorData[0][i] = 0x7000; // 0 code
		}

		temp = (uint8_t)((colors2[(state+1) & 0x01] >> (23 - i))&0x01);
		if(temp){
			colorData[1][i] = 0xFF80; // 1 code
		}else{
			colorData[1][i] = 0x7000; // 0 code
		}
	}

	// send data to strip LED controllers
	for(uint8_t j = 0; j < LEDS_COUNT; j++){
		if(j % part_size == 0 && j > 0){
			arrayIndex ^= 0x01;
		}
		for(uint8_t i = 0; i < 24; i++){
			SPI1->DR = colorData[arrayIndex][i];
			while(!(SPI1->SR & SPI_SR_TXE)){}
		}
	}
	// add delay >= 50 uS for RET code
	for(uint8_t i = 0; i < 50; i++){
		SPI1->DR = 0;
		while(!(SPI1->SR & SPI_SR_TXE)){}
	}
}

static uint32_t HsvToRgb(uint8_t h, uint8_t s, uint8_t v)
{
    uint32_t grb;
    uint8_t region, remainder, p, q, t;

    if (s == 0)
    {
        grb = (v<<16)|(v<<8)|v;
        return grb;
    }

    region = h/43;
    remainder = (h - (region * 43)) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            grb = (t<<16)|(v<<8)|p;
            break;
        case 1:
            grb = (v<<16)|(q<<8)|p;
            break;
        case 2:
            grb = (v<<16)|(p<<8)|t;
            break;
        case 3:
            grb = (q<<16)|(p<<8)|v;
            break;
        case 4:
            grb = (p<<16)|(t<<8)|v;
            break;
        default:
            grb = (p<<16)|(v<<8)|q;
            break;
    }

    return grb;
}

static uint32_t RgbToHsv(uint32_t grb)
{
    uint8_t grbMin, grbMax;
    uint8_t r = (grb>>8)&0xFF, g = (grb>>16)&0xFF, b = grb & 0xFF;
    uint8_t h = 0, s = 0, v = 0;

    grbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
    grbMax = r > g ? (r > b ? r : b) : (g > b ? g : b);

    v = grbMax;
    if (v == 0)
    {
        h = 0;
        s = 0;
        return (h<<16)|(s<<8)|v;
    }

    s = 255 * (long)(grbMax - grbMin) / v;
    if (s == 0)
    {
        h = 0;
        return (h<<16)|(s<<8)|v;
    }

    if (grbMax == r)
        h = 0 + 43 * (g - b) / (grbMax - grbMin);
    else if (grbMax == g)
        h = 85 + 43 * (b - r) / (grbMax - grbMin);
    else
        h = 171 + 43 * (r - g) / (grbMax - grbMin);

    return (h<<16)|(s<<8)|v;
}

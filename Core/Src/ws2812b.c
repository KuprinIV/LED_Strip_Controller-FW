#include "ws2812b.h"

extern SPI_HandleTypeDef hspi1;

// driver functions
static void setStripColor(uint32_t grb);
static void setBlinkingStrip(uint32_t grb);
static void setHSV_StripColor(uint8_t h, uint8_t s, uint8_t v);
static void setStripPartColor(uint32_t grb, uint8_t part_length);
static void setStripFourColor(uint32_t* colors4);
static void setStripTwoColor(uint32_t* colors2, uint8_t part_size);
static void setHSV_Sequence(uint8_t value);
static void stripPowerOff(void);
static void updateFramebuffer(uint8_t index_offset);

// inner functions
static uint32_t HsvToRgb(uint8_t h, uint8_t s, uint8_t v);
static uint32_t RgbToHsv(uint32_t grb);
static void setLedColorInFrameBuffer(uint8_t led_pos, uint32_t grb_color);

// init driver
LED_StripDriver led_drv = {
		setStripColor,
		setBlinkingStrip,
		setHSV_StripColor,
		setStripPartColor,
		setStripFourColor,
		setStripTwoColor,
		setHSV_Sequence,
		stripPowerOff,
		updateFramebuffer,
};
LED_StripDriver* led_strip_drv = &led_drv;

// init LEDs framebuffer
uint8_t leds_framebuffer[LEDS_COUNT][24] = {0};

static void setStripColor(uint32_t grb)
{
	static uint32_t prev_color;

	// fill LED framebuffer
	if(prev_color != grb)
	{
		prev_color = grb;
		for(uint8_t i = 0; i < LEDS_COUNT; i++)
		{
			setLedColorInFrameBuffer(i, grb);
		}
	}
}

static void setBlinkingStrip(uint32_t grb)
{
	static uint8_t index, dir;
	static uint32_t prev_color;
	static uint32_t hsv;

	if(prev_color != grb)
	{
		prev_color = grb;
		hsv = RgbToHsv(grb);
	}
	setStripColor(HsvToRgb((hsv & 0xFF0000)>>16, (hsv & 0xFF00)>>8, index));
	if(!dir)
	{
		if(index == 255)
		{
			dir = 1;
		}
		else
		{
			index+=5;
		}
	}
	else
	{
		if(index == 0)
		{
			dir = 0;
		}
		else
		{
			index-=5;
		}
	}
}

static void setHSV_StripColor(uint8_t h, uint8_t s, uint8_t v)
{
	  setStripColor(HsvToRgb(h, s, v));
}

static void setHSV_Sequence(uint8_t value)
{
	static uint8_t prev_value;
	uint32_t temp = 0;
	// fill LED framebuffer
	if(prev_value != value)
	{
		prev_value = value;
		for(uint8_t i = 0; i < LEDS_COUNT; i++)
		{
			temp = HsvToRgb((229 + (uint8_t)255.0f/LEDS_COUNT*i) & 0xFF, 255, value);
			setLedColorInFrameBuffer(i, temp);
		}
	}
}

static void stripPowerOff(void)
{
	setStripColor(COLOR_BLACK);
}

static void setStripPartColor(uint32_t grb, uint8_t part_length)
{
	static uint32_t prev_color;
	static uint32_t hsv_color;
	static uint8_t delta;
	static uint8_t h = 0, s = 0, v = 0, part_length_prev = 0;

	uint32_t temp_color = 0;

	if(prev_color != grb || part_length != part_length_prev)
	{
		prev_color = grb;
		part_length_prev = part_length;
		// init value difference and convert rgb to hsv
		hsv_color = RgbToHsv(grb);
		h = (hsv_color>>16) & 0xFF;
		s = (hsv_color>>8) & 0xFF;
		v = hsv_color & 0xFF;
		delta = (uint8_t)(v/part_length);

		// fill LED framebuffer
		for(uint8_t i = 0; i < LEDS_COUNT; i++)
		{
			if(i < part_length)
			{
				temp_color = HsvToRgb(h, s, v - (part_length - 1 - i)*delta);
				setLedColorInFrameBuffer(i, temp_color);
			}
			else
			{
				setLedColorInFrameBuffer(i, COLOR_BLACK);
			}
		}
	}
}

static void setStripFourColor(uint32_t* colors4)
{
	// fill LED framebuffer
	for(uint8_t i = 0; i < LEDS_COUNT; i++)
	{
		setLedColorInFrameBuffer(i, colors4[i%4]);
	}
}

static void setStripTwoColor(uint32_t* colors2, uint8_t part_size)
{
	uint8_t arrayIndex = 0;

	// fill LED framebuffer
	for(uint8_t i = 0; i < LEDS_COUNT; i++)
	{
		if(i > 0 && (i % part_size) == 0)
		{
			arrayIndex = (~arrayIndex) & 0x01;
		}
		setLedColorInFrameBuffer(i, colors2[arrayIndex]);
	}
}

static void updateFramebuffer(uint8_t index_offset)
{
	// send data to strip LED controllers
	for(uint8_t j = index_offset; j < LEDS_COUNT; j++)
	{
		for(uint8_t i = 0; i < 24; i+=2)
		{
			SPI1->DR = (uint16_t)((leds_framebuffer[j][i]<<8)|leds_framebuffer[j][i+1]);
			while(!(SPI1->SR & SPI_SR_TXE)){}
		}
	}
	for(uint8_t j = 0; j < index_offset; j++)
	{
		for(uint8_t i = 0; i < 24; i+=2)
		{
			SPI1->DR = (uint16_t)((leds_framebuffer[j][i]<<8)|leds_framebuffer[j][i+1]);
			while(!(SPI1->SR & SPI_SR_TXE)){}
		}
	}
	// add delay >= 50 uS for RET code
	for(uint8_t i = 0; i < 50; i++)
	{
		SPI1->DR = 0;
		while(!(SPI1->SR & SPI_SR_TXE)){}
	}
}

static uint32_t HsvToRgb(uint8_t h, uint8_t s, uint8_t v)
{
    uint32_t grb;
    uint8_t region, remainder, p, q, t;
    uint16_t rgb_sum = 0;

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

    // add LEDs brightness smoothing
//    rgb_sum = (uint16_t)(p + q + t);
//    p = (uint8_t)((uint16_t)(p<<8)/rgb_sum);
//    q = (uint8_t)((uint16_t)(q<<8)/rgb_sum);
//    t = (uint8_t)((uint16_t)(t<<8)/rgb_sum);

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

static void setLedColorInFrameBuffer(uint8_t led_pos, uint32_t grb_color)
{
	uint8_t temp = 0;

	// fill color data array
	for(uint8_t i = 0; i < 24; i++)
	{
		temp = (uint8_t)((grb_color >> (23 - i))&0x01);
		if(temp)
		{
			leds_framebuffer[led_pos][i] = BIT1; // 1 code
		}
		else
		{
			leds_framebuffer[led_pos][i] = BIT0; // 0 code
		}
	}
}

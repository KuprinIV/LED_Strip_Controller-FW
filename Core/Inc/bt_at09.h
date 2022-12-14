#ifndef __BT_AT09_H
#define __BT_AT09_H

#include "stm32f0xx_hal.h"

typedef struct
{
	void (*Init)(void);
	void (*PowerCtrl)(uint8_t is_enable);
	uint8_t (*IsConnected)(void);
	void (*SendCommand)(const char* command);
}BluetoothDriver;

extern BluetoothDriver* bt05_drv;

#endif

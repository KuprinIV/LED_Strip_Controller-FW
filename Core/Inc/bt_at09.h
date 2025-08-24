#ifndef __BT_AT09_H
#define __BT_AT09_H

#include "stm32f0xx_hal.h"

#define CMD_BUF_LEN			19

typedef struct
{
	void (*Init)(void);
	void (*PowerCtrl)(uint8_t is_enable);
	uint8_t (*IsConnected)(void);
	void (*SendResponse)(const char* command);
	void (*SendData)(uint8_t* data, uint8_t len);
}BluetoothDriver;

extern BluetoothDriver* bt05_drv;

#endif

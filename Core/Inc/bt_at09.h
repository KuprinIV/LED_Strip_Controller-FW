#ifndef __BT_AT09_H
#define __BT_AT09_H

#include "stm32f0xx_hal.h"

#define IS_DEBUG 1
#define DATA_SIZE 50
#define OUEUE_DEPTH 20

typedef struct{
	uint8_t effectNum;
	uint32_t colors[4];
	uint8_t saturation;
	uint8_t value;
	uint8_t sectorSize;
}effectParamsData;

void bt_init();
void enableConnection();
void disableConnection();
uint8_t isConnected();
void sendCommand(const char* command);
uint8_t* getReply();

// queue functions
uint8_t* peek();
uint8_t isEmpty();
uint8_t isFull();
uint8_t size();
void insert(uint8_t* data);
uint8_t* poll();

#endif

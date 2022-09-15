#include "bt_at09.h"
#include <string.h>

extern UART_HandleTypeDef huart1;

__IO ITStatus isUartReady = RESET;

uint8_t is_Connected = 0, isLineOK = 0;
uint8_t dataBuffer[DATA_SIZE] = {0};
uint8_t* atCmdsReplyBuffer;
uint8_t dataReceiveMode = 0;

__IO uint8_t isInputDataReady = 0;

// queue variables
uint8_t commandsArray[OUEUE_DEPTH][DATA_SIZE];
uint8_t front = 0;
uint8_t rear = -1;
uint8_t itemCount = 0;

void enableConnection()
{
	GPIOA->BSRR = GPIO_PIN_1; // grant connection
}

void disableConnection()
{
	GPIOA->BSRR = GPIO_PIN_1<<16; // break connection
	is_Connected = 0;
	isLineOK = 0;
}

uint8_t isConnected()
{
	is_Connected = ((GPIOA->IDR & GPIO_PIN_4) != 0);
	return is_Connected;
}

void bt_init()
{
//	enableConnection();
	// send AT command
	sendCommand("AT\r\n");

	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dataBuffer, DATA_SIZE); // start receive data
	atCmdsReplyBuffer = getReply();

	if(strcmp((const char*)atCmdsReplyBuffer, "OK\r\n") == 0){
		isLineOK = 1;
	}else{
		isLineOK = 0;
	}
	// init module
	if(isLineOK){
		sendCommand("AT+ROLE0\r\n");
		atCmdsReplyBuffer = getReply();

		sendCommand("AT+NAMELED_Strip_BT\r\n");
		atCmdsReplyBuffer = getReply();

		sendCommand("AT+RESET\r\n");
		atCmdsReplyBuffer = getReply();

		dataReceiveMode = 1;
	}
}

void sendCommand(const char* command)
{
	HAL_UART_Transmit_DMA(&huart1, (uint8_t*)command, strlen(command));
	while(isUartReady != SET){}
	isUartReady = RESET;
}

uint8_t* getReply()
{
	volatile uint32_t start_time = HAL_GetTick();
	volatile uint32_t time = start_time;
#ifdef IS_DEBUG
	HAL_Delay(100);
	return NULL;
#endif
	while(!isInputDataReady || (time - start_time < 500)){ // wait reply or timeout elapse
		time = HAL_GetTick();
	}
	if(!isEmpty()){
		return poll();
	}
	return NULL;
}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of DMA Rx transfer, and
  *         you can add your own implementation.
  * @retval None
  */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(!dataReceiveMode || Size == 19){
		isInputDataReady = 1;
		insert(dataBuffer);
		memset(dataBuffer, 0, sizeof(dataBuffer));
	}
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dataBuffer, dataReceiveMode ? 19 : DATA_SIZE);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /* Set transmission flag: trasfer complete*/
  isUartReady = SET;
}

/*
 * command queue realization
 */
uint8_t* peek() {
   return commandsArray[front];
}

uint8_t isEmpty() {
	uint8_t res = (itemCount == 0) ? 1 : 0;
	return res;
}

uint8_t isFull() {
	uint8_t res = (itemCount == OUEUE_DEPTH) ? 1 : 0;
   return res;
}

uint8_t size() {
   return itemCount;
}

void insert(uint8_t* data) {

   if(!isFull()) {

      if(rear == OUEUE_DEPTH-1) {
         rear = -1;
      }

      memcpy(commandsArray[++rear], data, DATA_SIZE);
      itemCount++;
   }
}

uint8_t* poll() {
   uint8_t* data = commandsArray[front++];

   if(front == OUEUE_DEPTH) {
      front = 0;
   }

   itemCount--;

   isInputDataReady = 0;

   return data;
}

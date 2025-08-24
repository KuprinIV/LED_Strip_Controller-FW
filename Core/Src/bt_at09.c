#include "bt_at09.h"
#include "queue.h"
#include "main.h"
#include "ws2812b.h"
#include <string.h>

extern UART_HandleTypeDef huart1;

// driver functions
static void bt_init();
static void powerCtrl(uint8_t is_enable);
static uint8_t isConnected();
static void sendResponse(const char* command);
static void sendData(uint8_t* data, uint8_t len);

// init driver
BluetoothDriver bt_drv = {
		bt_init,
		powerCtrl,
		isConnected,
		sendResponse,
		sendData,
};

BluetoothDriver* bt05_drv = &bt_drv;

uint8_t is_Connected = 0;
uint8_t dataBuffer[50] = {0};
uint8_t dataReceiveMode = 0;
uint8_t isHardwareCheck = 0;
uint8_t isConfigurationCheck = 0;
uint8_t configurationStep = 0;

/**
  * @brief  Bluetooth module power control
  * @param  is_enable: 0 - disable module, 1 - enable bluetooth
  * @retval None
  */
static void powerCtrl(uint8_t is_enable)
{
	if(is_enable)
	{
		GPIOA->BSRR = GPIO_PIN_1; // grant connection
	}
	else
	{
		GPIOA->BSRR = GPIO_PIN_1<<16; // break connection
		is_Connected = 0;
	}
}

/**
  * @brief  Get Bluetooth connection state
  * @param  None
  * @retval 0 - not connected, 1 - connected
  */
static uint8_t isConnected()
{
	is_Connected = ((GPIOA->IDR & GPIO_PIN_4) != 0);
	return is_Connected;
}

/**
  * @brief  Bluetooth module initialization
  * @param  None
  * @retval None
  */
static void bt_init()
{
	// reset BT
	powerCtrl(0);
	HAL_Delay(10);
	powerCtrl(1);
	HAL_Delay(1000);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dataBuffer, sizeof(dataBuffer)); // start receive data
	// check UART communication
	isHardwareCheck = 1;
	sendResponse("AT\r\n");
}

/**
  * @brief  Send AT-command to Bluetooth module
  * @param  command: AT-command string
  * @retval None
  */
static void sendResponse(const char* command)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)command, strlen(command), 1000);
}

/**
  * @brief  Send data buffer to Bluetooth module
  * @param  data: data buffer
  * @param  len: data buffer length
  * @retval None
  */
static void sendData(uint8_t* data, uint8_t len)
{
	HAL_UART_Transmit(&huart1, data, len, 1000);
}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @retval None
  */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(Size > 0)
	{
		if(isHardwareCheck)
		{
			if(strstr((char*)dataBuffer, "OK")) // hardware is ok
			{
				isHardwareCheck = 0;
				isConfigurationCheck = 1;
				configurationStep = 0;
				sendResponse("AT+ROLE\r\n");
			}
			else
			{
				sendResponse("AT\r\n");
			}
		}
		else if(isConfigurationCheck)
		{
			switch(configurationStep)
			{
				case 0:
					if(strstr((char*)dataBuffer, "+ROLE=0"))
					{
						sendResponse("AT+NAME\r\n");
						configurationStep += 2;
					}
					else
					{
						sendResponse("AT+ROLE0\r\n");
						configurationStep++;
					}
					break;

				case 1:
					if(strstr((char*)dataBuffer, "+ROLE=0"))
					{
						sendResponse("AT+NAME\r\n");
						configurationStep++;
					}
					else
					{
						isConfigurationCheck = 0;
						Error_Handler();
					}
					break;

				case 2:
					if(strstr((char*)dataBuffer, "+NAME=LED_Strip_BT"))
					{
						sendResponse("AT+PIN\r\n");
						configurationStep += 2;
					}
					else
					{
						sendResponse("AT+NAMELED_Strip_BT\r\n");
						configurationStep++;
					}
					break;

				case 3:
					if(strstr((char*)dataBuffer, "+NAME=LED_Strip_BT"))
					{
						sendResponse("AT+PIN\r\n");
						configurationStep++;
					}
					else
					{
						isConfigurationCheck = 0;
						Error_Handler();
					}
					break;

				case 4:
					if(strstr((char*)dataBuffer, "+PIN=123456"))
					{
						configurationStep += 2;
					}
					else
					{
						sendResponse("AT+PIN123456\r\n");
						configurationStep++;
					}
					break;

				case 5:
					if(strstr((char*)dataBuffer, "+PIN=123456"))
					{
						configurationStep++;
					}
					else
					{
						isConfigurationCheck = 0;
						Error_Handler();
					}
					break;

				case 6:
					dataReceiveMode = 1;
					isConfigurationCheck = 0;
					break;

				default:
					Error_Handler();
					break;
			}
		}
		else if(dataReceiveMode && Size == CMD_BUF_LEN)
		{
			commands_queue->Insert(dataBuffer);
		}
	}
	if(isHardwareCheck || isConfigurationCheck)
	{
		memset(dataBuffer, 0, sizeof(dataBuffer));
	}

	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dataBuffer, dataReceiveMode ? CMD_BUF_LEN : sizeof(dataBuffer));
}

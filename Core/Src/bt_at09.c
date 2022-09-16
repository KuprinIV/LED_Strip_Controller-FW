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
static void sendCommand(const char* command);

// init driver
BluetoothDriver bt_drv = {
		bt_init,
		powerCtrl,
		isConnected,
		sendCommand,
};

BluetoothDriver* bt05_drv = &bt_drv;

uint8_t is_Connected = 0;
uint8_t dataBuffer[50] = {0};
uint8_t dataReceiveMode = 0;
uint8_t isHardwareCheck = 0;
uint8_t isConfigurationCheck = 0;
uint8_t configurationStep = 0;

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

static uint8_t isConnected()
{
	is_Connected = ((GPIOA->IDR & GPIO_PIN_4) != 0);
	return is_Connected;
}

static void bt_init()
{
	// reset BT
	powerCtrl(0);
	HAL_Delay(10);
	powerCtrl(1);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dataBuffer, sizeof(dataBuffer)); // start receive data
	// check UART communication
	isHardwareCheck = 1;
	sendCommand("AT\r\n");
}

static void sendCommand(const char* command)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)command, strlen(command), 1000);
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
	if(Size > 0)
	{
		if(isHardwareCheck)
		{
			if(strstr((char*)dataBuffer, "OK")) // hardware is ok
			{
				isHardwareCheck = 0;
				isConfigurationCheck = 1;
				configurationStep = 0;
				sendCommand("AT+ROLE\r\n");
			}
			else
			{
				sendCommand("AT\r\n");
			}
		}
		else if(isConfigurationCheck)
		{
			switch(configurationStep)
			{
				case 0:
					if(strstr((char*)dataBuffer, "+ROLE=0"))
					{
						sendCommand("AT+NAME\r\n");
						configurationStep += 2;
					}
					else
					{
						sendCommand("AT+ROLE0\r\n");
						configurationStep++;
					}
					break;

				case 1:
					if(strstr((char*)dataBuffer, "+ROLE=0"))
					{
						sendCommand("AT+NAME\r\n");
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
						sendCommand("AT+PIN\r\n");
						configurationStep += 2;
					}
					else
					{
						sendCommand("AT+NAMELED_Strip_BT\r\n");
						configurationStep++;
					}
					break;

				case 3:
					if(strstr((char*)dataBuffer, "+NAME=LED_Strip_BT"))
					{
						sendCommand("AT+PIN\r\n");
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
						sendCommand("AT+RESET\r\n");
						configurationStep += 2;
					}
					else
					{
						sendCommand("AT+PIN123456\r\n");
						configurationStep++;
					}
					break;

				case 5:
					if(strstr((char*)dataBuffer, "+PIN=123456"))
					{
						sendCommand("AT+RESET\r\n");
						configurationStep++;
					}
					else
					{
						isConfigurationCheck = 0;
						Error_Handler();
					}
					break;

				case 6:
					if(strstr((char*)dataBuffer, "OK"))
					{
						dataReceiveMode = 1;
						isConfigurationCheck = 0;
					}
					else
					{
						Error_Handler();
					}
					break;

				default:
					Error_Handler();
					break;
			}
		}
		else if(dataReceiveMode && Size == 19)
		{
			commands_queue->Insert(dataBuffer);
		}
	}
	if(isHardwareCheck || isConfigurationCheck)
	{
		memset(dataBuffer, 0, sizeof(dataBuffer));
	}

	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dataBuffer, dataReceiveMode ? 19 : sizeof(dataBuffer));
}

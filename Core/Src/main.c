/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ws2812b.h"
#include "bt_at09.h"
#include "queue.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim14;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */
volatile uint8_t isUpdate = 0;
volatile uint8_t updateClockDivider = 1;
effectParamsData effectsArray[8] = {
		{0, {0, 0, 0, 0}, 0, 0, 0}, 										// strip power off
		{1, {COLOR_RED, COLOR_YELLOW, COLOR_LIME, COLOR_BLUE}, 0, 0, 0}, 	// four colors effect
		{2, {COLOR_RED, COLOR_YELLOW, 0, 0}, 0, 0, 1}, 						// two colors effect
		{3, {0, 0, 0, 0}, 255, 150, 0}, 									// smooth color change effect
		{4, {COLOR_WHITE, 0, 0, 0}, 0, 0, 0}, 								// constant color effect
		{5, {COLOR_WHITE, 0, 0, 0}, 0, 0, 0}, 								// blinking color effect
		{6, {COLOR_WHITE, 0, 0, 0}, 0, 0, 15},								// shift color part effect
		{7, {0, 0, 0, 0}, 255, 150, 0}										// rainbow effect
};

effectParamsData currentEffect;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM14_Init(void);
/* USER CODE BEGIN PFP */
static void updateCurrentEffect(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint8_t hue = 0;
  uint8_t needToUpdate = 0, isButtonPrevPressed = 0, colorCounter = 0, longPressEffectCounter = 0, shiftOffset = 0,
	      clockCounter = 0, isLongPress = 0, isLongPressDetected = 0, isDeviceBusy = 0;
  uint32_t pressTime = 0;
  uint32_t hard_colors[8] = {COLOR_WHITE, COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_PURPLE};
  uint32_t hard_two_colors[2][2] = {{COLOR_RED, COLOR_YELLOW}, {COLOR_GREEN, COLOR_BLUE}};
  uint8_t* commandData;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM14_Init();
  /* USER CODE BEGIN 2 */
  //init BT module
  bt05_drv->Init();

  // init current effect
  currentEffect = effectsArray[0];
  updateCurrentEffect();
  needToUpdate = 1;

  // enable SPI
  SPI1->CR1 |= SPI_CR1_SPE;

  // start update timer
  HAL_TIM_Base_Start_IT(&htim14);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if(isUpdate)
	  {
		  isUpdate = 0;
		  //check button
		  if(!isButtonPrevPressed && ((GPIOA->IDR & GPIO_PIN_6) == 0)) // detect button press event
		  {
			  isButtonPrevPressed = 1;
			  pressTime = HAL_GetTick();
		  }
		  if(isButtonPrevPressed && ((GPIOA->IDR & GPIO_PIN_6) == 0)) // count button press duration
		  {
			  if((HAL_GetTick() - pressTime > 500) && !isLongPressDetected)
			  {
				  isLongPress = 1;
				  isLongPressDetected = 1;
			  }
		  }
		  // handle long button press
		  if(isLongPress)
		  {
			  isLongPress = 0;

			  longPressEffectCounter++;
			  if(longPressEffectCounter < 4)  colorCounter = 0; // reset color counter
			  if(longPressEffectCounter == 3) hue = 0; // reset hue value
			  if(longPressEffectCounter == 6) shiftOffset = 0; // reset shiftOffset value
			  if(longPressEffectCounter == EFFECTS_COUNT)
			  {
				  longPressEffectCounter = 0;
			  }
			  currentEffect = effectsArray[longPressEffectCounter];
			  updateCurrentEffect();
			  needToUpdate = 1;
		  }
		  if(isButtonPrevPressed && (GPIOA->IDR & GPIO_PIN_6)) // detect button release event
		  {
			  isButtonPrevPressed = 0;
			  if(isLongPressDetected) // was long press
			  {
				  isLongPressDetected = 0;
			  }
			  else // was short press
			  {
				  colorCounter++;
				  if(longPressEffectCounter > 3) // don't change color counter for 4 first effects
				  {
					  colorCounter &= 0x07;
					  currentEffect.colors[0] = hard_colors[colorCounter];
				  }
				  else if(longPressEffectCounter == 2)
				  {
					  colorCounter &= 0x01;
					  currentEffect.colors[0] = hard_two_colors[colorCounter][0];
					  currentEffect.colors[1] = hard_two_colors[colorCounter][1];
				  }
				  updateCurrentEffect();
				  needToUpdate = 1;
			  }
		  }
		  // check bluetooth BLE input data
		  if(!commands_queue->IsEmpty() && !isDeviceBusy)
		  {
			  commandData = commands_queue->Poll(); // get data from commands queue
			  if(commandData[0] == 0xAA && commandData[17] == 0x0D && commandData[18] == 0x0A) // effect data report received
			  {
				  isDeviceBusy = 1;
				  longPressEffectCounter = commandData[1];
				  currentEffect = effectsArray[longPressEffectCounter];
				  // fill effects arrays
				  currentEffect.effectNum = commandData[1];
				  currentEffect.colors[0] = (commandData[2]<<16)|(commandData[3]<<8)|commandData[4];
				  currentEffect.colors[1] = (commandData[5]<<16)|(commandData[6]<<8)|commandData[7];
				  currentEffect.colors[2] = (commandData[8]<<16)|(commandData[9]<<8)|commandData[10];
				  currentEffect.colors[3] = (commandData[11]<<16)|(commandData[12]<<8)|commandData[13];
				  currentEffect.saturation = commandData[14];
				  currentEffect.value = commandData[15];
				  currentEffect.sectorSize = commandData[16];
				  memset(commandData, 0, DATA_SIZE);

				  if(longPressEffectCounter == 2) hue = 0; // reset hue value
				  if(longPressEffectCounter == 5) shiftOffset = 0; // reset shiftOffset value

				  updateCurrentEffect();
				  needToUpdate = 1;
			  }
		  }
		  // check update
		  if(!isDeviceBusy)
		  {
			  if(clockCounter < updateClockDivider)
			  {
				  clockCounter++;
			  }
			  else
			  {
				  clockCounter = 0;
				  needToUpdate = 1;
			  }
		  }

		  // update strip color state
		  if(needToUpdate)
		  {
			  needToUpdate = 0;
			  // select effect
			  switch(longPressEffectCounter)
			  {
				  case 1: // four color blinking
					  shiftOffset = (shiftOffset + 1) & 0x03;
					  break;

				  case 2: // two color blinking
					  shiftOffset = ((shiftOffset + 1) & 0x01)*currentEffect.sectorSize;
					  break;

				  case 3: // smooth color changing
					  led_strip_drv->setHsvStripColor(hue, currentEffect.saturation, currentEffect.value);
					  hue++;
					  break;

				  case 0: // power off
				  case 4: // constant color
				  case 7: // rainbow
				  default:
					  shiftOffset = 0;
					  break;

				  case 5: // blinking constant color
					  led_strip_drv->setBlinkingStrip(currentEffect.colors[0]);
					  break;

				  case 6: // shifting colored part
					  if(!isDeviceBusy)
					  {
						  if(shiftOffset > 0)
						  {
							  shiftOffset--;
						  }
						  else
						  {
							  shiftOffset = LEDS_COUNT-1;
						  }
					  }
					  break;

			  }
			  // update framebuffer data
			  led_strip_drv->updateFramebuffer(shiftOffset);

			  // reset busy state
			  if(isDeviceBusy)
			  {
				  isDeviceBusy = 0;
				  clockCounter = 0;
				  bt05_drv->SendCommand("Effect set");
			  }
		  }
	  }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM14 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM14_Init(void)
{

  /* USER CODE BEGIN TIM14_Init 0 */

  /* USER CODE END TIM14_Init 0 */

  /* USER CODE BEGIN TIM14_Init 1 */

  /* USER CODE END TIM14_Init 1 */
  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 1999;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 999;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM14_Init 2 */

  /* USER CODE END TIM14_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	isUpdate = 1;
}

static void updateCurrentEffect(void)
{
	  switch(currentEffect.effectNum)
	  {
		  case 0: // strip power off
			  led_strip_drv->stripPowerOff();
			  break;

		  case 1: // four color blinking
			  updateClockDivider = 19;
			  led_strip_drv->setStripFourColor(currentEffect.colors);
			  break;

		  case 2: // two color blinking
			  updateClockDivider = 19;
			  led_strip_drv->setStripTwoColor(currentEffect.colors, currentEffect.sectorSize);
			  break;

		  case 3: // smooth color changing
			  updateClockDivider = 2;
			  led_strip_drv->setHsvStripColor(0, currentEffect.saturation, currentEffect.value);
			  break;

		  case 4: // constant color
			  updateClockDivider = 19;
			  led_strip_drv->setStripColor(currentEffect.colors[0]);
			  break;

		  case 5: // blinking constant color
			  updateClockDivider = 0;
			  led_strip_drv->setBlinkingStrip(currentEffect.colors[0]);
			  break;

		  case 6: // shifting colored part
			  updateClockDivider = 1;
			  led_strip_drv->setStripPartColor(currentEffect.colors[0], currentEffect.sectorSize);
			  break;

		  case 7: // rainbow
			  updateClockDivider = 19;
			  led_strip_drv->setHsvSequence(currentEffect.value);
			  break;
	  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */


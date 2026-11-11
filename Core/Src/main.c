/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
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
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//USART 통신 변수
uint8_t rx_data;
char rx_buffer[50];
uint8_t rx_index = 0;
char rx_flag = 0;

//cds 변수
uint16_t cds_value =0;
uint32_t sensing_time=0;
uint32_t cds_sum =0;
uint16_t cds_average= 0;

//가속도 변수
uint16_t x_value = 0;
uint32_t pre_x_ADC_value = 0;
int x_diff =0;
char first_x_value =1;
uint32_t x_sum = 0;
uint16_t x_average= 0;

//led 변수
uint32_t LED_time =0;
char holding_LED_state =0;

//알고리즘 변수
char moter_mode = 0;
char previous_moter_mode =0;
volatile uint16_t ADC_value[2] = {0};
char holding_flag =0;
uint8_t sensing_count= 0;
char end_flag =0;
char start_flag =0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void __io_putchar(int ch)
{
	if(ch == '\n')
	{
		HAL_UART_Transmit (&huart2, (uint8_t*) "\r", 1, 0xffff);
	}

	HAL_UART_Transmit (&huart2, (uint8_t*) &ch, 1, 0xffff);
}

void holding_function(void)
{
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0); //정지 2개핀 추가
	HAL_GPIO_WritePin(motor_control1_GPIO_Port,motor_control1_Pin,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(motor_control2_GPIO_Port,motor_control2_Pin,GPIO_PIN_RESET);
	printf("시스템 홀딩\n");
	printf("PLAY 나 END을 입력하세요.\n\n");

	while(holding_flag == 1)
	{
  		if (strcmp(rx_buffer,"END")== 0)
  		{
  			start_flag = 0;
  			holding_flag = 0;
  			first_x_value = 1;
			__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
			__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
			__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
  		}
  		else if (strcmp(rx_buffer,"PLAY")== 0)
  		{
  			  end_flag =0;
  			  holding_flag = 0;
  			  previous_moter_mode = 0;
			  first_x_value = 1;
			  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
			  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
			  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
	          strcpy(rx_buffer, "START");
	          printf("프로젝트 시스템 재시작\n");
	          break;
  		}

		if (HAL_GetTick() - LED_time >= 300)
		{
			if (holding_LED_state == 0)
			{
				__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 1000);
				__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 1000);
				__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 1000);
				holding_LED_state = 1;
			}
			else
			{
				__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
				__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
				__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
				holding_LED_state = 0;
			}
			LED_time = HAL_GetTick();
		}
	}
}
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
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  printf("80-4.EVB_USART_holdingfuntcion_test\n\n");

   HAL_ADCEx_Calibration_Start(&hadc1);//ADC1의 CDS와 X값 calibration
   HAL_ADC_Start_DMA(&hadc1,(uint32_t*)&ADC_value,2);

   HAL_UART_Receive_IT(&huart2,&rx_data,1);//universal analog recieve transmit

   HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);//LED 3색 PWM
   HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
   HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

   HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);//모터제어 PWM
   HAL_GPIO_WritePin(motor_control1_GPIO_Port,motor_control1_Pin,GPIO_PIN_RESET);
   HAL_GPIO_WritePin(motor_control2_GPIO_Port,motor_control2_Pin,GPIO_PIN_RESET);
   printf("start을 입력해주세요\n\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  cds_sum += ADC_value[0];
	  sensing_count++;

	  if (sensing_count == 10)
	  {
		  sensing_count = 0;
		  cds_average = cds_sum / 10;
		  cds_sum = 0;
	  }

	  if (rx_flag == 1)
	  {
		  if(strcmp(rx_buffer,"START")== 0)
		  {
			  if(start_flag == 0)
			  {
				  start_flag =1;
				  printf("프로젝트 시스템 시작\n\n");
			  }
			  end_flag = 0;

			  if ((0 < cds_average) && (cds_average <= 820)) //밝음,파랑
			  {
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 1000);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
				  moter_mode = 1;
			  }
			  else if ((820 < cds_average) && (cds_average <= 1640))//높구,초록
			  {
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 1000);
				  moter_mode = 2;
			  }
			  else if ((1640 < cds_average) && (cds_average <= 2460))// 먹구름,노랑 빨100+초50
			  {
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 800);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 200);
				  moter_mode = 3;
			  }
			  else if ((2460 < cds_average) && (cds_average <= 3280))// 소나기,주황
			  {
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 780);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 30);
				  moter_mode = 4;
			  }
			  else if ((3280 < cds_average) && (cds_average <= 4095))//폭우,빨강
			  {
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 1000);
				  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
				  moter_mode = 5;
			  }

			  if (first_x_value == 1)
			  {
					first_x_value = 0;
					pre_x_ADC_value = ADC_value[1];
			  }
			  else
			  {
				  if (ADC_value[1] >= pre_x_ADC_value)
				  {
					x_diff = ADC_value[1] - pre_x_ADC_value;
				  }
				  else
				  {
					x_diff = pre_x_ADC_value - ADC_value[1];
				  }

				  if (x_diff >= 500)
				  {
					holding_flag = 1;
					holding_function();
				  }
				  pre_x_ADC_value = ADC_value[1];
			  }
		  }
		  else if (strcmp(rx_buffer,"END")== 0)
		  {
			  if(end_flag == 0)
			  {
				  printf("프로젝트 시스템 종료\n\n");
				  end_flag = 1;
			  }
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
				HAL_GPIO_WritePin(motor_control1_GPIO_Port,motor_control1_Pin,GPIO_PIN_RESET);
				HAL_GPIO_WritePin(motor_control2_GPIO_Port,motor_control2_Pin,GPIO_PIN_RESET);

			  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
			  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
			  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
			  start_flag = 0;
			  moter_mode = 0;
			  previous_moter_mode = 0;
		  }
	  }

	  if (moter_mode != previous_moter_mode)
	  {
		  previous_moter_mode = moter_mode;

		  if (moter_mode == 1)
		  {
			  HAL_GPIO_WritePin(motor_control1_GPIO_Port,motor_control1_Pin,GPIO_PIN_SET);
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000);
			  printf("주행 안전, 조도: 밝음, LED:파랑색 입니다.\n\n");
		  }
		  else if (moter_mode == 2)
		  {
			  HAL_GPIO_WritePin(motor_control1_GPIO_Port,motor_control1_Pin,GPIO_PIN_SET);
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 800);
			  printf("주행 보통, 조도: 높은구름, LED :초록색 입니다.\n\n");
		  }
		  else if (moter_mode == 3)
		  {
			  HAL_GPIO_WritePin(motor_control1_GPIO_Port,motor_control1_Pin,GPIO_PIN_SET);
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 600);
			  printf("주행 경계, 조도: 먹구름, LED: 노란색입니다.\n\n");
		  }
		  else if (moter_mode == 4)
		  {
			  HAL_GPIO_WritePin(motor_control1_GPIO_Port,motor_control1_Pin,GPIO_PIN_SET);
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 400);
			  printf("주행 주의, 조도: 소나기, LED: 주황색입니다.\n\n");
		  }
		  else if (moter_mode == 5)
		  {
			  HAL_GPIO_WritePin(motor_control1_GPIO_Port,motor_control1_Pin,GPIO_PIN_SET);
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 200);
			  printf("서행 필수, 조도: 폭우, LED: 빨간색입니다.\n\n");
		  }
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV8;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 63;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 63;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, motor_control1_Pin|motor_control2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : motor_control1_Pin motor_control2_Pin */
  GPIO_InitStruct.Pin = motor_control1_Pin|motor_control2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback (UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
    	rx_flag = 1;
    	if((rx_data=='\r')||(rx_data=='\n'))
    	{
        	if(rx_index>0)
        	{
				rx_buffer[rx_index] = '\0'; //rx_index을 늘릴 필요x , 문자열의 끝 널을 abc'0'로 만들었음
				rx_index = 0;	//rx_index 이제 또 받아야하니 초기화
        	}
    	}
    	else
    	{
    		rx_buffer[rx_index++] = (char)rx_data; //rx_index++ = index++
    	}
    	HAL_UART_Receive_IT(&huart2, &rx_data, 1);
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
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

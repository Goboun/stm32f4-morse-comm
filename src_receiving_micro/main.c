/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2025 STMicroelectronics.
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
#include "stdbool.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>
#include "stdarg.h"
#include <inttypes.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */



//uint32_t signalStartTime = 0;
//uint32_t signalDuration = 0;
//uint32_t lastSignalTime = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;


/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
void safe_printf(const char *fmt, ...);



#define DOT_DURATION 200   // 点的持续时间（毫秒）
#define DASH_DURATION 600  // 划的持续时间（毫秒）
#define SPACE 400          // 点和点、点和划、划和划之间的间隔
#define LETTER_SPACE 1400  // 字符之间的间隔
#define WORD_SPACE 2000    // 单词之间的间隔
#define THRESHOLD 1000     // 小于阈值说明有声音
#define DEBOUNCE_TIME 50      // 去抖动时间，防止短时间内的多次波动被误判
#define SAMPLING_INTERVAL 10  // 采样间隔，单位为毫秒，设置每次采样的时间间隔
#define INTERVAL_THRESHOLD 1000  // 字符之间的间隔时间阈值

//uint32_t adcValue = 0;
//uint32_t currentTime = 0;
//uint32_t signalStartTime = 0;
//uint32_t lastSignalEndTime = 0;
//bool signalDetected = false;

typedef enum {
    IDLE,
    SIGNAL_LOW,
	DEBOUNCE_HIGH_END
} SignalState;

// 全局变量
SignalState currentState = IDLE;

uint32_t signalStartTime = 0;
uint32_t signalEndTime = 0;
uint32_t debounceEndStartTime = 0;
uint32_t duration = 0;
uint32_t lastEventEndTime = 0;  // 上次事件结束时间


// ADC初始化句柄
extern ADC_HandleTypeDef hadc1;

// 获取ADC值
uint32_t getADCValue(void) {
    // 启动ADC转换
    HAL_ADC_Start(&hadc1);

    // 等待ADC转换完成
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        // 获取转换后的ADC值
        return HAL_ADC_GetValue(&hadc1);
    }

    // 如果读取失败，则返回0
    return 0;
}


void havePoint(uint32_t ADCValue) {
	if(ADCValue < THRESHOLD){
		printf("have point\n\n");
	}
}


// 莫尔斯电码映射表（部分常用字母示例）
typedef struct {
    const char* code;
    char symbol;
} MorseCode;

MorseCode morseTable[] = {
    {".-",   'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..",  'D'},
    {".",    'E'}, {"..-.", 'F'}, {"--.",  'G'}, {"....", 'H'},
    {"..",   'I'}, {".---", 'J'}, {"-.-",  'K'}, {".-..", 'L'},
    {"--",   'M'}, {"-.",   'N'}, {"---",  'O'}, {".--.", 'P'},
    {"--.-", 'Q'}, {".-.",  'R'}, {"...",  'S'}, {"-",    'T'},
    {"..-",  'U'}, {"...-", 'V'}, {".--",  'W'}, {"-..-", 'X'},
    {"-.--", 'Y'}, {"--..", 'Z'}, {"-----",'0'}, {".----",'1'},
    {"..---",'2'}, {"...--",'3'}, {"....-",'4'}, {".....",'5'},
    {"-....",'6'}, {"--...",'7'}, {"---..",'8'}, {"----.",'9'},
    {NULL,   '\0'} // 结束标记
};

// 全局变量
#define CHAR_TIMEOUT 1100    // 字符间隔时间（ms）
#define WORD_TIMEOUT 1800   // 单词间隔时间（ms）

char currentCode[8] = {0};  // 当前字符的莫尔斯码缓存
uint32_t lastSymbolTime = 0; // 最后收到符号的时间

// 莫尔斯码解码函数
char decodeMorse(const char* code) {
    for(int i=0; morseTable[i].code != NULL; i++) {
        if(strcmp(code, morseTable[i].code) == 0) {
            return morseTable[i].symbol;
        }
    }
    return '?'; // 未识别的编码
}



void morse() {
    uint32_t adcValue = getADCValue();
    uint32_t currentTime = HAL_GetTick();

    if (strlen(currentCode) > 0 && (currentTime - lastEventEndTime) > CHAR_TIMEOUT) {
    	char symbol = decodeMorse(currentCode);
    	safe_printf("%c", symbol);
    	memset(currentCode, 0, sizeof(currentCode)); // 正确清空缓存


    	if ((currentTime - lastEventEndTime) > WORD_TIMEOUT) {
    	     safe_printf(" ");
    	}

    }

    switch (currentState) {
        case IDLE:
            if (adcValue < THRESHOLD) {
                signalStartTime = currentTime;
                currentState = SIGNAL_LOW;
            }
            break;

        case SIGNAL_LOW:
            if (adcValue >= THRESHOLD) {
                signalEndTime = currentTime;
                debounceEndStartTime = currentTime;
                currentState = DEBOUNCE_HIGH_END;
            }
            break;

        case DEBOUNCE_HIGH_END:
            if (adcValue < THRESHOLD) {
                currentState = SIGNAL_LOW;
            } else {
                if (currentTime - debounceEndStartTime >= DEBOUNCE_TIME) {
                    duration = signalEndTime - signalStartTime;

                    if(duration > 100){
                    	// 输出持续时间和符号
//                    	safe_printf("Fluctuation Duration = %lums", duration);
//                    	safe_printf(duration < 500 ? "." : "-");
                    	if(duration < 400){
                    		safe_printf(".");
                    		strcat(currentCode, ".");
                    	}else{
                    		safe_printf("-");
                    		strcat(currentCode, "-");
                    	}

                    }


                    // 更新事件结束时间
                    lastEventEndTime = currentTime;
                    currentState = IDLE;
                }
            }
            break;
    }
}



//成功打印摩斯电码点和划
//void calculateFluctuationDuration() {
//    uint32_t adcValue = getADCValue();
//    uint32_t currentTime = HAL_GetTick();
//
//    switch (currentState) {
//        case IDLE:
//            if (adcValue < THRESHOLD) {
//                signalStartTime = currentTime;
//                currentState = SIGNAL_LOW;
//            }else{
//            	if (lastEventEndTime != 0 && (currentTime - lastEventEndTime) > WORD_TIMEOUT) {
//            	   safe_printf(" ");  // 打印间隔空格
//            	   lastEventEndTime = 0; //避免重复打印空格
//            	}
//            }
//            break;
//
//        case SIGNAL_LOW:
//            if (adcValue >= THRESHOLD) {
//                signalEndTime = currentTime;
//                debounceEndStartTime = currentTime;
//                currentState = DEBOUNCE_HIGH_END;
//            }
//            break;
//
//        case DEBOUNCE_HIGH_END:
//            if (adcValue < THRESHOLD) {
//                currentState = SIGNAL_LOW;
//            } else {
//                if (currentTime - debounceEndStartTime >= DEBOUNCE_TIME) {
//                    duration = signalEndTime - signalStartTime;
//
//                    if(duration > 100){
//                    	// 输出持续时间和符号
////                    	safe_printf("Fluctuation Duration = %lums", duration);
//                    	safe_printf(duration < 500 ? "." : "-");
//                    }
//
//
//                    // 更新事件结束时间
//                    lastEventEndTime = currentTime;
//                    currentState = IDLE;
//                }
//            }
//            break;
//    }
//}



//成功计算持续时长
//uint32_t signalStartTime = 0;     // 波动开始时间
//uint32_t signalEndTime = 0;       // 首次超过阈值时间
//uint32_t debounceEndStartTime = 0;// 结束确认开始时间
//uint32_t duration = 0;            // 有效持续时间
//
void getDuration() {
    uint32_t adcValue = getADCValue();
    uint32_t currentTime = HAL_GetTick();

    switch (currentState) {
        case IDLE:
            if (adcValue < THRESHOLD) {
                signalStartTime = currentTime;
                currentState = SIGNAL_LOW;
            }
            break;

        case SIGNAL_LOW:
            // 首次检测到超过阈值时进入确认状态
            if (adcValue >= THRESHOLD) {
                signalEndTime = currentTime;       // 记录首次超过阈值的时间
                debounceEndStartTime = currentTime; // 记录确认期开始时间
                currentState = DEBOUNCE_HIGH_END;
            }
            break;

        case DEBOUNCE_HIGH_END:
            if (adcValue < THRESHOLD) {
                // 确认期内再次低于阈值，回到SIGNAL_LOW状态继续计时
                currentState = SIGNAL_LOW;
            } else {
                // 持续检查确认期是否结束
                if (currentTime - debounceEndStartTime >= DEBOUNCE_TIME) {
                    // 确认波动有效，计算实际低电平持续时间
                    duration = signalEndTime - signalStartTime;
                    printf("Fluctuation Duration = %lu ms\n", duration);
                    currentState = IDLE;
                }
            }
            break;
    }
}






// 自定义高效输出函数
void safe_printf(const char *fmt, ...) {
    char buffer[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    HAL_UART_Transmit_DMA(&huart2, (uint8_t*)buffer, strlen(buffer));
}


// printf
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
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
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  printf("Ready...\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//	  uint32_t ADCValue = getADCValue();
//	  printf("ADC Value = %lu \n",ADCValue);
//	  havePoint(ADCValue);
//	  getDuration();
//	  calculateFluctuationDuration();
	  morse();
	  HAL_Delay(10); //每秒100次采样
//	  getDuration();  // 获取并打印持续时间

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

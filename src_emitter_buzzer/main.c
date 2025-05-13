/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "usart.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"  // HAL库，包含 UART、Delay 等 // Bibliothèque HAL, contient UART, Delay, etc.
#include <stdio.h>          // sprintf()
#include <string.h>         // strlen()

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Poll_UART_Receive(void);

#include "stm32f4xx_hal.h"

#define BUZZER_PIN GPIO_PIN_5
#define BUZZER_PORT GPIOA

//#define DOT_DURATION 200   // 点的持续时间（毫秒） // Durée du point (millisecondes)
//#define DASH_DURATION 600  // 划的持续时间（毫秒） // Durée du trait (millisecondes)
//#define LETTER_SPACE 800   // 字符之间的间隔（毫秒） // Intervalle entre les lettres (millisecondes)
//#define WORD_SPACE 1600    // 单词之间的间隔（毫秒） // Intervalle entre les mots (millisecondes)

#define DOT_DURATION 200   // 点的持续时间（毫秒） (适当增加点的时间，使点的持续时间更清晰)
#define DASH_DURATION 600  // 划的持续时间（毫秒） (增加划的时间，使划更容易分辨)
#define SPACE 400          // 点和点、点和划、划和划之间的间隔（增大间隔，以降低误判）
#define LETTER_SPACE 1400  // 字符之间的间隔（增大字符间隔，避免误判）
#define WORD_SPACE 2000    // 单词之间的间隔（增大单词间隔，保证间断性）


#define RX_BUFFER_SIZE 50  // 接收缓冲区大小 // Taille du buffer de réception
char rxBuffer[RX_BUFFER_SIZE];  // UART 接收缓冲区 // Buffer de réception UART
int rx_index = 0;

// 摩尔斯码编码表（字母A-Z） // Table de codage Morse (lettres A-Z)
const char* morseCode[] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
    "..-", "...-", ".--", "-..-", "-.--", "--.."};

void sendMorseCode(char ch) {
    // 如果是小写字母，转换为大写 // Convertir les lettres minuscules en majuscules
    if (ch >= 'a' && ch <= 'z') {
        ch = ch - 'a' + 'A';  // 'a' 转 'A'，'b' 转 'B'，依此类推 // 'a' devient 'A', 'b' devient 'B', etc.
    }

    if (ch >= 'A' && ch <= 'Z') {
        // 第一个是A，ch-A就是在morseCode中的索引 // Le premier est 'A', ch - 'A' donne l'index dans morseCode
        const char* code = morseCode[(int)(ch-'A')];

        // 发送莫尔斯码 // Envoyer le code Morse
        while (*code) {
            if (*code == '.') {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // 使LD2亮起 // Allumer LD2
                HAL_Delay(DOT_DURATION);   // 点持续时间 // Durée du point
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // 使LD2熄灭 // Éteindre LD2
                HAL_Delay(SPACE);   // 点与点之间的间隔 // Intervalle entre les points
            } else if (*code == '-') {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // 使LD2亮起 // Allumer LD2
                HAL_Delay(DASH_DURATION);  // 划持续时间 // Durée du trait
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // 使LD2熄灭 // Éteindre LD2
                HAL_Delay(SPACE);   // 划与划之间的间隔 // Intervalle entre les traits
            }
            code++;
        }

        // 每个字母之间有间隔 // Intervalle entre chaque lettre
        HAL_Delay(LETTER_SPACE);
    }
}

// 手动接收UART数据 // Réception manuelle des données UART
void Poll_UART_Receive(void){
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE))
    {
        char receivedChar = (char)(huart2.Instance->DR & 0xFF);

        if (receivedChar == '\r' || receivedChar == '\n')
        {
            rxBuffer[rx_index] = '\0';  // 终止接收的字符串 // Terminer la réception de la chaîne
            rx_index = 0;

            HAL_UART_Transmit(&huart2, (uint8_t*)rxBuffer, strlen(rxBuffer), HAL_MAX_DELAY);  // 回显接收到的字符串 // Répéter la chaîne reçue

            // 发送接收到的字符串的莫尔斯码 // Envoyer la chaîne reçue en code Morse
            for (int i = 0; rxBuffer[i] != '\0'; i++) {
                sendMorseCode(rxBuffer[i]);
                HAL_Delay(WORD_SPACE);  // 每个单词之间的间隔 // Intervalle entre chaque mot
            }
        }
        else
        {
            if (rx_index < RX_BUFFER_SIZE - 1)  // 防止溢出 // Éviter le débordement
            {
                rxBuffer[rx_index++] = receivedChar;  // 存储接收到的字符 // Stocker le caractère reçu
            }
        }
    }
}

// 主函数 // Fonction principale
int main(void) {
    // HAL初始化 // Initialisation de HAL
    HAL_Init();

    // 初始化蜂鸣器GPIO // Initialisation du GPIO du buzzer
    __HAL_RCC_GPIOA_CLK_ENABLE();  // 开启GPIOA时钟 // Activer l'horloge GPIOA
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);

    // 初始化UART2 // Initialisation de l'UART2
    MX_USART2_UART_Init();

    // 程序进入死循环 // Boucle infinie du programme
    while (1) {
        Poll_UART_Receive();  // 不断轮询接收数据 // Polling en continu pour recevoir des données
    }
}



//void Poll_UART_Receive(void){
//    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE))
//    {
//        char receivedChar = (char)(huart2.Instance->DR & 0xFF);
//
//        if (receivedChar == '\r' || receivedChar == '\n')
//        {
//            rx_buffer[rx_index] = '\0';
//            rx_index = 0;
//
//
//            HAL_UART_Transmit(&huart2, (uint8_t*)rx_buffer, strlen(rx_buffer), HAL_MAX_DELAY);
//
//
////            sendMorseMessage(rx_buffer);
//        }
//        else
//        {
//
//            if (rx_index < RX_BUFFER_SIZE - 1)
//            {
//                rx_buffer[rx_index++] = receivedChar;
//            }
//        }
//    }
//}
//
//
//
//int main(void) {
//    /* HAL 初始化 */
//    HAL_Init();
//    SystemClock_Config();
//
//    /* 初始化外设 */
//    MX_GPIO_Init();
//    MX_USART2_UART_Init();
//
//    /* 发送初始化消息 */
//    char initMessage[] = "STM32 Ready. Send a message:\r\n";
//    HAL_UART_Transmit(&huart2, (uint8_t*)initMessage, strlen(initMessage), HAL_MAX_DELAY);
//
//    /* 定义接收缓冲区 */
////    char receivedMessage[100];  // 存储接收到的字符串
//
//    /* 无限循环 */
//    while (1) {
////        /* 清空缓冲区 */
////        memset(receivedMessage, 0, sizeof(receivedMessage));
////
////        /* 等待接收数据（阻塞模式） */
////        HAL_UART_Receive(&huart2, (uint8_t*)receivedMessage, sizeof(receivedMessage) - 1, HAL_MAX_DELAY);
////
////        /* 打印接收到的信息 */
////        HAL_UART_Transmit(&huart2, (uint8_t*)"Received: ", 10, HAL_MAX_DELAY);
////        HAL_UART_Transmit(&huart2, (uint8_t*)receivedMessage, strlen(receivedMessage), HAL_MAX_DELAY);
////        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
//
//    	Poll_UART_Receive();
//    }
//}


///* 定义全局变量 */
//char rx_buffer[100];  // 用于存储接收的字符串
//volatile int rx_index = 0;  // 接收索引
//
//int main(void) {
//    // HAL 初始化
//    HAL_Init();
//    SystemClock_Config();
//
//    // 初始化蜂鸣器 GPIO
//    __HAL_RCC_GPIOA_CLK_ENABLE();  // 开启 GPIOA 时钟
//    GPIO_InitTypeDef GPIO_InitStruct = {0};
//    GPIO_InitStruct.Pin = BUZZER_PIN;
//    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);
//
//    // 初始化 UART
//    MX_USART2_UART_Init();
//
//    // 发送欢迎信息
//    uint8_t initMessage[] = "STM32 Ready. Send Morse Code Message:\r\n";
//    HAL_UART_Transmit(&huart2, initMessage, sizeof(initMessage) - 1, HAL_MAX_DELAY);
//
//    // 无限循环
//    while (1) {
//        Poll_UART_Receive();  // 轮询 UART 接收数据
//
//        // 如果接收到完整的一句话（回车换行），则转换为摩尔斯码
//        if (rx_index == 0 && rx_buffer[0] != '\0') {
//            HAL_UART_Transmit(&huart2, (uint8_t*)"Processing Morse Code:\r\n", 24, HAL_MAX_DELAY);
//
//            // 播放摩尔斯码
//            for (int i = 0; rx_buffer[i] != '\0'; i++) {
//                sendMorseCode(rx_buffer[i]);  // 逐个字符转换为摩尔斯码
//            }
//
//            // 清空缓冲区，准备接收下一条消息
//            memset(rx_buffer, 0, sizeof(rx_buffer));
//        }
//    }
//}
//
///* UART 轮询接收函数 */
//void Poll_UART_Receive(void) {
//    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
//        char receivedChar = (char)(huart2.Instance->DR & 0xFF);
//
//        if (receivedChar == '\r' || receivedChar == '\n') {
//            rx_buffer[rx_index] = '\0';  // 添加字符串结束符
//            rx_index = 0;  // 复位索引
//
//            // 回显收到的数据
//            HAL_UART_Transmit(&huart2, (uint8_t*)"Received: ", 10, HAL_MAX_DELAY);
//            HAL_UART_Transmit(&huart2, (uint8_t*)rx_buffer, strlen(rx_buffer), HAL_MAX_DELAY);
//            HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
//        } else {
//            if (rx_index < sizeof(rx_buffer) - 1) {
//                rx_buffer[rx_index++] = receivedChar;
//            }
//        }
//    }
//}
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

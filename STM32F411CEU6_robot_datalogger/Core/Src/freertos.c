/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SDcard.h"
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
/* USER CODE BEGIN Variables */
static const data_type_t one_1[] = {CORRENTE, A_CTRL_TRACAO};
static const data_type_t one_10[] = {VEL_ANG_MOTOR, A_CTRL_VEL, S_POINT_VEL};
static const data_type_t one_100[] = {A_CTRL_POSI};
static const data_type_t three_1[] = {G_CTRL_TRACAO};
static const data_type_t three_10[] = {ACEL_LIN, VEL_ANG_GIRO, CAMPO_MAG, G_CTRL_VEL, ANG_ROT_BASE};
static const data_type_t three_100[] = {GPS, G_CTRL_POSI};
static const data_type_t three_ap[] = {G_CTRL_TRACAO_RX, G_CTRL_VEL_RX, G_CTRL_POSI_RX};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 8,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void send_data(void* arg);
static bool start_send_data_tasks(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */

  vTaskDelay(3000);
  if(!SDcard_start()){
	  USB_PRINT("error starting sd card\n");
	  return;
  }
  if(!start_send_data_tasks()){
	  USB_PRINT("error starting send data tasks\n");
  }


  /* Infinite loop */
  for(;;)
  {
    vTaskDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * Função que gera e envia dados periodicamente para a fila da tarefa SD card.
 *
 * A função `send_data` é uma tarefa FreeRTOS que gera dados simulados com base em
 * configurações fornecidas como argumento. Os dados gerados são enviados para a fila
 * que a tarefa de gerenciamento da SD card utiliza para gravar esses dados no cartão.
 * A função executa um número limitado de iterações e, em seguida, exclui a tarefa.
 *
 * @param arg Parâmetro de entrada, convertido para um valor de 32 bits, que contém as configurações
 * de período, tipo de dado, e tamanho do array.
 */
void send_data(void* arg) {
    uint32_t settings_in_bits = (uint32_t)arg; // Configurações passadas como argumento
    QueueHandle_t SDcard_queue = SDcard_get_queue_handle(); // Obtém o handle da fila SD card
    uint16_t period_ms = 0;  // Período de envio em milissegundos
    uint8_t array_size = 0;  // Tamanho do array de valores (1 ou 3)
    data_type_t type = 0;    // Tipo de dado a ser enviado
    char buf[35];            // Buffer para mensagens de erro
    int n = 0;               // Contador de iterações

    // Determina o tamanho do array com base no bit mais significativo (31º bit) de settings_in_bits
    if (settings_in_bits >> 31) {
        array_size = 3;
    } else {
        array_size = 1;
    }

    // Extrai o período em milissegundos dos bits 16 a 30 de settings_in_bits
    period_ms = (settings_in_bits << 1) >> 16;

    // Extrai o tipo de dado dos bits 0 a 15 de settings_in_bits
    type = (settings_in_bits << 16) >> 16;

    // Inicializa o gerador de números aleatórios com base no tipo de dado
    srand(type);

    // Armazena o tempo atual para uso em delays periódicos
    TickType_t LastWakeTime = xTaskGetTickCount();

    for (;;) {
        if (n == 3) break; // Limita a execução a 3 iterações

        // Atraso periódico baseado no período em milissegundos
        vTaskDelayUntil(&LastWakeTime, pdMS_TO_TICKS(period_ms));

        // Cria a estrutura de dados a ser enviada
        SD_data_t data = {
            .value = {rand() % 100, rand() % 100, rand() % 100}, // Valores aleatórios
            .data_type = type, // Tipo de dado
            .timestamp = LastWakeTime, // Timestamp da última execução
            .array_size = array_size, // Tamanho do array de valores
        };

        // Envia os dados para a fila da tarefa SD card
        if (xQueueSend(SDcard_queue, (void*)&data, pdMS_TO_TICKS(period_ms) / 2) != pdTRUE) {
            // Em caso de falha, imprime uma mensagem de erro
            sprintf(buf, "fail send data to queue - %d\n", data.data_type);
            USB_PRINT(buf);
        }

        n++; // Incrementa o contador de iterações
    }

    // Exclui a tarefa após 3 iterações
    vTaskDelete(NULL);
}


/**
 * Função para iniciar várias tarefas de envio de dados.
 *
 * Esta função cria múltiplas tarefas FreeRTOS, cada uma configurada para enviar dados
 * periodicamente, conforme definido pelos parâmetros fornecidos. As tarefas são configuradas
 * para enviar dados com diferentes tamanhos de array e períodos.
 *
 * @return Retorna `true` se todas as tarefas forem criadas com sucesso, caso contrário, retorna `false`.
 */
static bool start_send_data_tasks(void) {
    BaseType_t ret = pdTRUE;
    uint32_t settings_in_bits = 0UL;

    // Configuração para tarefas com array de 1 elemento e período de 1 ms
    settings_in_bits = 0 << 31; // 1 elemento no array
    settings_in_bits += 1 << 15; // Período de 1 ms
    for (int i = 0; i < sizeof(one_1)/sizeof(one_1[0]); i++) {
        settings_in_bits += one_1[i]; // Tipo de dado
        ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
        if (ret != pdPASS) return false; // Falha na criação da tarefa
        settings_in_bits = (settings_in_bits >> 15) << 15; // Reset dos bits menos significativos
    }

    // Configuração para tarefas com array de 1 elemento e período de 10 ms
    settings_in_bits = 0 << 31; // 1 elemento no array
    settings_in_bits += 10 << 15; // Período de 10 ms
    for (int i = 0; i < sizeof(one_10)/sizeof(one_10[0]); i++) {
        settings_in_bits += one_10[i]; // Tipo de dado
        ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
        if (ret != pdPASS) return false; // Falha na criação da tarefa
        settings_in_bits = (settings_in_bits >> 15) << 15; // Reset dos bits menos significativos
    }

    // Configuração para tarefas com array de 1 elemento e período de 100 ms
    settings_in_bits = 0 << 31; // 1 elemento no array
    settings_in_bits += 100 << 15; // Período de 100 ms
    for (int i = 0; i < sizeof(one_100)/sizeof(one_100[0]); i++) {
        settings_in_bits += one_100[i]; // Tipo de dado
        ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
        if (ret != pdPASS) return false; // Falha na criação da tarefa
        settings_in_bits = (settings_in_bits >> 15) << 15; // Reset dos bits menos significativos
    }

    // Configuração para tarefas com array de 3 elementos e período de 1 ms
    settings_in_bits = 1 << 31; // 3 elementos no array
    settings_in_bits += 1 << 15; // Período de 1 ms
    for (int i = 0; i < sizeof(three_1)/sizeof(three_1[0]); i++) {
        settings_in_bits += three_1[i]; // Tipo de dado
        ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
        if (ret != pdPASS) return false; // Falha na criação da tarefa
        settings_in_bits = (settings_in_bits >> 15) << 15; // Reset dos bits menos significativos
    }

    // Configuração para tarefas com array de 3 elementos e período de 10 ms
    settings_in_bits = 1 << 31; // 3 elementos no array
    settings_in_bits += 10 << 15; // Período de 10 ms
    for (int i = 0; i < sizeof(three_10)/sizeof(three_10[0]); i++) {
        settings_in_bits += three_10[i]; // Tipo de dado
        ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
        if (ret != pdPASS) return false; // Falha na criação da tarefa
        settings_in_bits = (settings_in_bits >> 15) << 15; // Reset dos bits menos significativos
    }

    // Configuração para tarefas com array de 3 elementos e período de 100 ms
    settings_in_bits = 1 << 31; // 3 elementos no array
    settings_in_bits += 100 << 15; // Período de 100 ms
    for (int i = 0; i < sizeof(three_100)/sizeof(three_100[0]); i++) {
        settings_in_bits += three_100[i]; // Tipo de dado
        ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
        if (ret != pdPASS) return false; // Falha na criação da tarefa
        settings_in_bits = (settings_in_bits >> 15) << 15; // Reset dos bits menos significativos
    }

    // Configuração para tarefas com array de 3 elementos e período aleatório (até 1000 ms)
    settings_in_bits = 1 << 31; // 3 elementos no array
    settings_in_bits += rand() % 1000 << 15; // Período aleatório em ms
    for (int i = 0; i < sizeof(three_ap)/sizeof(three_ap[0]); i++) {
        settings_in_bits += three_ap[i]; // Tipo de dado
        ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
        if (ret != pdPASS) return false; // Falha na criação da tarefa
        settings_in_bits = (settings_in_bits >> 15) << 15; // Reset dos bits menos significativos
    }

    return true; // Todas as tarefas foram criadas com sucesso
}

/* USER CODE END Application */


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
DataParameter data_parameters[] = {
    {CORRENTE, 1, 3, 1},
    {VEL_ANG_MOTOR, 1, 3, 10},
    {ACEL_LIN, 3, 3, 10},
    {VEL_ANG_GIRO, 3, 3, 10},
    {CAMPO_MAG, 3, 3, 10},
    {A_CTRL_TRACAO, 1, 3, 1},
    {G_CTRL_TRACAO, 3, 9, 1},
    {G_CTRL_TRACAO_RX, 3, 9, 0},
    {A_CTRL_VEL, 1, 3, 10},
    {S_POINT_VEL, 1, 3, 10},
    {G_CTRL_VEL, 3, 9, 10},
    {G_CTRL_VEL_RX, 3, 9, 0},
    {ANG_ROT_BASE, 3, 3, 10},
    {GPS, 3, 3, 100},
    {A_CTRL_POSI, 1, 3, 100},
    {G_CTRL_POSI, 3, 3, 100},
    {G_CTRL_POSI_RX, 3, 3, 0}
};
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
void send_data(void* arg){
	DataParameter *params = (DataParameter *)arg;
	QueueHandle_t SDcard_queue = SDcard_get_queue_handle();
	uint16_t period_ms = params->period_ms;
	uint8_t array_size = (params->dimensions == 3) ? 3 : 1;
	uint8_t data_quantity = params->data_quantity;
	data_type_t type = params->type;
	char buf[35];
	int n = 0;
	int i;

	srand(type);
	TickType_t LastWakeTime = xTaskGetTickCount();

	for (;;) {
		if (n == 3) break;
		if (period_ms > 0) {
		            vTaskDelayUntil(&LastWakeTime, pdMS_TO_TICKS(period_ms));
		} else {
			// Delay aleatório entre 1 e 1000 ms para dados aperiódicos
			vTaskDelayUntil(&LastWakeTime, pdMS_TO_TICKS(rand() % 1000 + 1));
		}
		SD_data_t data = {
			.value = {rand() % 100, rand() % 100, rand() % 100},
			.data_type = type,
			.timestamp = LastWakeTime,
			.array_size = array_size,
		};
		for (i = 0; i < data_quantity; i++) {
			if (xQueueSend(SDcard_queue, (void*)&data, pdMS_TO_TICKS(period_ms) / 2) != pdTRUE) {
				sprintf(buf, "fail send data to queue - %d\n", data.data_type);
				USB_PRINT(buf);
			}
		}
		n++;
	}
	vTaskDelete(NULL);
}


static bool start_send_data_tasks(void){
	BaseType_t ret = pdTRUE;
	for (int i = 0; i < sizeof(data_parameters) / sizeof(DataParameter); i++) {
	    ret = xTaskCreate(send_data,
				"send_data",
				128 * 2,
				(void *) &data_parameters[i],
				25,
				NULL);
	    if (ret != pdPASS) return false;
	}

	return true;
}

/* USER CODE END Application */

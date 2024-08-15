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
void send_data(void* arg){
	uint32_t settings_in_bits = (uint32_t)arg;
	QueueHandle_t SDcard_queue = SDcard_get_queue_handle();
	uint16_t period_ms = 0;
	uint8_t array_size = 0;
	data_type_t type = 0;
	char buf [35];
	int n = 0;

	if(settings_in_bits>>31){
		array_size = 3;
	} else {
		array_size = 1;
	}

	period_ms = (settings_in_bits<<1)>>16;
	type = (settings_in_bits<<16)>>16;
	srand(type);
	TickType_t LastWakeTime = xTaskGetTickCount();
	for(;;){
		if(n == 3) break;
		vTaskDelayUntil(&LastWakeTime, pdMS_TO_TICKS(period_ms));
		SD_data_t data = {
			.value = {rand()%100,rand()%100,rand()%100},
			.data_type = type,
			.timestamp = LastWakeTime,
			.array_size = array_size,
		};

		if(xQueueSend(SDcard_queue, (void*)&data, pdMS_TO_TICKS(period_ms)/2) != pdTRUE){
			sprintf(buf, "fail send data to queue - %d\n", data.data_type);
			USB_PRINT(buf);
		}
		n++;
	}
	vTaskDelete(NULL);
}


static bool start_send_data_tasks(void){
	BaseType_t ret = pdTRUE;
	uint32_t settings_in_bits = 0UL;

	settings_in_bits = 0<<31; //1 element array
	settings_in_bits += 1<<15; // time in ms
	for(int i = 0; i < sizeof(one_1)/sizeof(one_1[0]); i++){
	  settings_in_bits += one_1[i]; //type of data
	  ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
	  if(ret != pdPASS) return false;
	  settings_in_bits = (settings_in_bits>>15)<<15;
	}

	settings_in_bits = 0<<31; //1 element array
	settings_in_bits += 10<<15; // time in ms
	for(int i = 0; i < sizeof(one_10)/sizeof(one_10[0]); i++){
	  settings_in_bits += one_10[i]; //type of data
	  ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
	  if(ret != pdPASS) return false;
	  settings_in_bits = (settings_in_bits>>15)<<15;
	}

	settings_in_bits = 0<<31; //1 element array
	settings_in_bits += 100<<15; // time in ms
	for(int i = 0; i < sizeof(one_100)/sizeof(one_100[0]); i++){
	  settings_in_bits += one_100[i]; //type of data
	  ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
	  if(ret != pdPASS) return false;
	  settings_in_bits = (settings_in_bits>>15)<<15;
	}

	settings_in_bits = 1<<31; //3 element array
	settings_in_bits += 1<<15; // time in ms
	for(int i = 0; i < sizeof(three_1)/sizeof(three_1[0]); i++){
	  settings_in_bits += three_1[i]; //type of data
	  ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
	  if(ret != pdPASS) return false;
	  settings_in_bits = (settings_in_bits>>15)<<15;
	}
	settings_in_bits = 1<<31; //3 element array
	settings_in_bits += 10<<15; // time in ms
	for(int i = 0; i < sizeof(three_10)/sizeof(three_10[0]); i++){
	  settings_in_bits += three_10[i]; //type of data
	  ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
	  if(ret != pdPASS) return false;
	  settings_in_bits = (settings_in_bits>>15)<<15;
	}

	settings_in_bits = 1<<31; //3 element array
	settings_in_bits += 100<<15; // time in ms
	for(int i = 0; i < sizeof(three_100)/sizeof(three_100[0]); i++){
	  settings_in_bits += three_100[i]; //type of data
	  ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
	  if(ret != pdPASS) return false;
	  settings_in_bits = (settings_in_bits>>15)<<15;
	}

	settings_in_bits = 1<<31; //3 element array
	settings_in_bits += rand()%1000<<15; // time in ms
	for(int i = 0; i < sizeof(three_ap)/sizeof(three_ap[0]); i++){
	  settings_in_bits += three_ap[i]; //type of data
	  ret = xTaskCreate(send_data, "send_data", 128*2, (void*)settings_in_bits, 25, NULL);
	  if(ret != pdPASS) return false;
	  settings_in_bits = (settings_in_bits>>15)<<15;
	}
	return true;
}

/* USER CODE END Application */

/*
 * sd_card.c
 *
 *  Created on: Aug 10, 2024
 *      Author: Bianca, Pedro Hugo e Fredson
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "queue.h"
#include "fatfs.h"
#include <stdbool.h>
#include "sd_card.h"

static FATFS fs;
static FIL fil;

SemaphoreHandle_t print_smpr_handle;
static TaskHandle_t sd_card_task_handle = NULL;
static QueueHandle_t sd_card_queue_handle = NULL;

TaskHandle_t sd_Card_get_task_handle(void){
	return sd_card_task_handle;
}

QueueHandle_t sd_card_get_queue_handle(void){
	return sd_card_queue_handle;
}

static bool path_select(char* path, size_t buff_size, data_type_t data_type){
	if(buff_size < 50*sizeof(char)){
		USB_PRINT("path buffer size too small\n");
		return false;
	}
	*path = '\0';
	switch(data_type){
				case CORRENTE:
					strcpy(path, "CORRENTE.csv");
					break;
				case VEL_ANG_MOTOR:
					strcpy(path, "VEL_ANG_MOTOR.csv");
					break;
				case ACEL_LIN:
					strcpy(path, "ACEL_LIN.csv");
					break;
				case VEL_ANG_GIRO:
					strcpy(path, "VEL_ANG_GIRO.csv");
					break;
				case CAMPO_MAG:
					strcpy(path, "CAMPO_MAG.csv");
					break;
				case A_CTRL_TRACAO:
					strcpy(path, "A_CTRL_TRACAO.csv");
					break;
				case G_CTRL_TRACAO:
					strcpy(path, "G_CTRL_TRACAO.csv");
					break;
				case G_CTRL_TRACAO_RX:
					strcpy(path, "G_CTRL_TRACAO_RX.csv");
					break;
				case A_CTRL_VEL:
					strcpy(path, "A_CTRL_VEL.csv");
					break;
				case S_POINT_VEL:
					strcpy(path, "S_POINT_VEL.csv");
					break;
				case G_CTRL_VEL:
					strcpy(path, "G_CTRL_VEL.csv");
					break;
				case G_CTRL_VEL_RX:
					strcpy(path, "G_CTRL_VEL_RX.csv");
					break;
				case ANG_ROT_BASE:
					strcpy(path, "ANG_ROT_BASE.csv");
					break;
				case GPS:
					strcpy(path, "GPS.csv");
					break;
				case A_CTRL_POSI:
					strcpy(path, "A_CTRL_POSI.csv");
					break;
				case G_CTRL_POSI:
					strcpy(path, "G_CTRL_POSI.csv");
					break;
				case G_CTRL_POSI_RX:
					strcpy(path, "G_CTRL_POSI_RX.csv");
					break;
				default:
					USB_PRINT("type not supported\n");
					return false;
			}
	return true;
}

static bool reset_files(void){
	int er = 1;
	char path[50];
	char buffer[100];

	for(int i = 0; i < NumberOfTypes; i++){
		er = path_select(path, sizeof(path), i);
		if(!er){
			USB_PRINT("error selecting path\n");
			return false;
		}

		er = f_stat(path, NULL);
		if(er != FR_OK){
			if(er == FR_NO_FILE) continue;
			sprintf(buffer, "error: %d\n", er);
			USB_PRINT(buffer);
			return false;
		}

		er = f_unlink(path);
		if(er != FR_OK){
			sprintf(buffer, "error: %d\n", er);
			USB_PRINT(buffer);
			return false;
		}
	}
	return true;
}

static void sd_card_task(void* arg){
	sd_data_t data = {0};
	char buffer[100];
	char path[50];
	int er = 0;
	for(;;){
		if(xQueueReceive(sd_card_queue_handle, &data, portMAX_DELAY) != pdTRUE){
			USB_PRINT("error receive from queue\n");
			continue;
		}

		er = path_select(path, sizeof(path), data.data_type);
		if(!er){
			USB_PRINT("error selecting path\n");
			continue;
		}

		er = f_stat(path, NULL);
		if(er == FR_NO_FILE){
			f_open(&fil, path, FA_CREATE_ALWAYS | FA_WRITE);
			f_printf(&fil, "%s\n", path);
			if(data.array_size == 1){
				f_puts("timestamp,value\n", &fil);
			}
			else if(data.array_size == 3){
				f_puts("timestamp,value1,value2,value3\n", &fil);
			}
			else{
				USB_PRINT("incompatible array size\n");
				continue;
			}
			f_close(&fil);
		}

		er = f_open(&fil, path, FA_OPEN_APPEND | FA_WRITE | FA_READ);
		if(er != FR_OK){
			sprintf(buffer, "error: %d\n", er);
			USB_PRINT(buffer);
			continue;
		}

		if(data.array_size == 1){
			f_printf(&fil, "%u,%d\n", data.timestamp, data.value[0]);
		}
		else if(data.array_size == 3){
			f_printf(&fil, "%u,%d,%d,%d\n", data.timestamp, data.value[0], data.value[1], data.value[2]);
		}
		else {
			USB_PRINT("incompatible array size\n");
			continue;
		}


		f_lseek(&fil, 0);
		while(f_gets(buffer, sizeof(buffer), &fil))
		{
			USB_PRINT(buffer);
			memset(buffer,0,sizeof(buffer));
		}
		USB_PRINT("\n");
		f_close(&fil);

	}
}

int sd_card_start(void){
	print_smpr_handle = xSemaphoreCreateBinary();
	sd_card_queue_handle = xQueueCreate(20, sizeof(sd_data_t));
	if(f_mount(&fs, "", 0) != FR_OK){
		USB_PRINT("fail mount sd card\n");
		return false;
	}
	if(reset_files() != true){
		USB_PRINT("fail reseting files\n");
		return false;
	}
	if(xTaskCreate(sd_card_task, "sd_card_task", 128*5, NULL, 26, NULL) != pdPASS) return false;
	return true;
}


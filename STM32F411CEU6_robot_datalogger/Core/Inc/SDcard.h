/*
 * SDcard.h
 *
 *  Created on: Ago 10, 2024
 *      Author: Bianca, Fredson e Pedro Hugo
 */

#ifndef SRC_SDCARD_H_
#define SRC_SDCARD_H_
#include "queue.h"
#include "string.h"
#include "usbd_cdc_if.h"
#include <stdbool.h>
#include "semphr.h"

extern SemaphoreHandle_t print_smpr_handle;

typedef enum{
	CORRENTE,
	VEL_ANG_MOTOR,
	ACEL_LIN,
	VEL_ANG_GIRO,
	CAMPO_MAG,
	A_CTRL_TRACAO,
	G_CTRL_TRACAO,
	G_CTRL_TRACAO_RX,
	A_CTRL_VEL,
	S_POINT_VEL,
	G_CTRL_VEL,
	G_CTRL_VEL_RX,
	ANG_ROT_BASE,
	GPS,
	A_CTRL_POSI,
	G_CTRL_POSI,
	G_CTRL_POSI_RX,
	NumberOfTypes,
} data_type_t;

typedef struct{
	int32_t value[3];
	data_type_t data_type;
	TickType_t timestamp;
	uint8_t array_size;
} SD_data_t;

/**
 * @brief Inicia a task e a queue de escrever no SDcard.
 *
 * @return
 * 		True - se a criação da task e da queue forem bem-sucedidas.
 * 		False - se a criação da task ou da queue for mal-sucedida.
 */
int SDcard_start(void);

/**
 * @return
 * 		Handler da task do SDcard
 */
TaskHandle_t SDcard_get_task_handle(void);

/**
 * @return
 * 		Handler da queue do SDcard
 */
QueueHandle_t SDcard_get_queue_handle(void);


#define USB_PRINT(data) do { \
		if(xSemaphoreTake(print_smpr_handle, 1)) \
			CDC_Transmit_FS((uint8_t*)data, strlen(data)); \
			xSemaphoreGive(print_smpr_handle); \
    } while(0)
#endif /* SRC_SDCARD_H_ */

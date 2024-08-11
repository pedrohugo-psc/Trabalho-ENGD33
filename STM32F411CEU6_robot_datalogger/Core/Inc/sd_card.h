/*
 * sd_card.h
 *
 *  Created on: Aug 10, 2024
 *      Author: Bianca, Pedro Hugo e Fredson
 */

#ifndef INC_SD_CARD_H_
#define INC_SD_CARD_H_

#include "queue.h"
#include "string.h"
#include "usbd_cdc_if.h"
#include <stdbool.h>
#include "semphr.h"

extern SemaphoreHandle_t print_smpr_handle;

typedef enum data_type{
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
}data_type_t;

typedef struct sd_data{
	int32_t value[3];
	data_type_t data_type;
	TickType_t timestamp;
	uint8_t array_size;
}sd_data_t;

/**
 * @brief Inicia a task e a queue de escrever no sd_card.
 *
 * @return
 * 		True - se a criação da task e da queue forem bem-sucedidas.
 * 		False - se a criação da task ou da queue for mal-sucedida.
 */
int sd_card_start(void);

/**
 * @return
 * 		Handler da task do sd_card
 */
TaskHandle_t sd_card_get_task_handle(void);

/**
 * @return
 * 		Handler da queue do sd_card
 */
QueueHandle_t sd_card_get_queue_handle(void);


#define USB_PRINT(data) do { \
		if(xSemaphoreTake(print_smpr_handle, 1)) \
			CDC_Transmit_FS((uint8_t*)data, strlen(data)); \
			xSemaphoreGive(print_smpr_handle); \
    } while(0)

#endif /* INC_SD_CARD_H_ */

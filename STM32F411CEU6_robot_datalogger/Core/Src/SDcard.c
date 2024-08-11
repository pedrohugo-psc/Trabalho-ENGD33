/*
 * SDcard.h
 *
 *  Created on: Ago 10, 2024
 *      Author: Bianca, Fredson e Pedro Hugo
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "queue.h"
#include "fatfs.h"
#include "fatfs_sd.h"
#include <stdbool.h>
#include "SDcard.h"

static FATFS fs;
static FIL fil;

SemaphoreHandle_t print_smpr_handle;
static TaskHandle_t SDcard_task_handle = NULL;
static QueueHandle_t SDcard_queue_handle = NULL;

TaskHandle_t SDCard_get_task_handle(void){
	return SDcard_task_handle;
}

QueueHandle_t SDcard_get_queue_handle(void){
	return SDcard_queue_handle;
}

/**
 * Seleciona o caminho do arquivo correspondente a um tipo de dado específico.
 *
 * A função `path_select` verifica se o buffer de caminho fornecido é grande o suficiente e, em seguida,
 * copia o nome do arquivo apropriado, baseado no tipo de dado (`data_type`), para o buffer `path`.
 * Cada tipo de dado está associado a um arquivo CSV específico.
 *
 * @param path Ponteiro para o buffer de string onde o caminho do arquivo será armazenado.
 * @param buff_size Tamanho do buffer de caminho em bytes.
 * @param data_type Tipo de dado para o qual o caminho do arquivo deve ser selecionado.
 * @return `true` se o caminho foi selecionado com sucesso, `false` caso contrário.
 */
static bool path_select(char* path, size_t buff_size, data_type_t data_type) {
    // Verifica se o tamanho do buffer é grande o suficiente para armazenar o nome do arquivo.
    if(buff_size < 50 * sizeof(char)) {
        USB_PRINT("path buffer size too small\n");
        return false;  // Retorna falso se o buffer for pequeno demais.
    }

    *path = '\0';  // Inicializa o buffer de caminho com uma string vazia.

    // Seleciona o caminho do arquivo com base no tipo de dado fornecido.
    switch(data_type) {
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
            return false;  // Retorna falso se o tipo de dado não for suportado.
    }

    return true;  // Retorna verdadeiro se o caminho foi selecionado com sucesso.
}

/**
 * Reseta (deleta) arquivos associados a diferentes tipos de dados, se existirem.
 *
 * A função `reset_files` percorre uma lista de tipos de dados e tenta deletar os arquivos
 * correspondentes, caso eles existam. Se algum arquivo não puder ser deletado ou se ocorrer
 * algum erro durante o processo, a função retorna `false`.
 *
 * @return `true` se todos os arquivos foram deletados com sucesso ou se eles não existiam;
 *         `false` se houve algum erro na seleção do caminho ou durante a deleção dos arquivos.
 */
static bool reset_files(void) {
    int er = 1;                // Variável de erro inicializada com um valor não nulo.
    char path[50];             // Buffer para armazenar o caminho do arquivo.
    char buffer[100];          // Buffer para mensagens de erro.

    // Itera sobre todos os tipos de dados definidos.
    for(int i = 0; i < NumberOfTypes; i++) {
        // Seleciona o caminho do arquivo correspondente ao tipo de dado.
        er = path_select(path, sizeof(path), i);
        if(!er) {
            USB_PRINT("error selecting path\n");
            return false;  // Retorna falso se a seleção do caminho falhar.
        }

        // Verifica se o arquivo existe utilizando f_stat.
        er = f_stat(path, NULL);
        if(er != FR_OK) {
            if(er == FR_NO_FILE) continue;  // Se o arquivo não existe, continua para o próximo tipo de dado.
            sprintf(buffer, "error: %d\n", er);
            USB_PRINT(buffer);  // Imprime a mensagem de erro correspondente.
            return false;  // Retorna falso se houver algum outro erro.
        }

        // Deleta o arquivo existente.
        er = f_unlink(path);
        if(er != FR_OK) {
            sprintf(buffer, "error: %d\n", er);
            USB_PRINT(buffer);  // Imprime a mensagem de erro correspondente.
            return false;  // Retorna falso se a deleção do arquivo falhar.
        }
    }

    return true;  // Retorna verdadeiro se todos os arquivos foram deletados com sucesso ou se não existiam.
}

/**
 * Tarefa responsável por gravar dados na SD card em arquivos CSV.
 *
 * A função `SDcard_task` é uma tarefa FreeRTOS que fica em loop infinito, aguardando
 * dados de uma fila (`SDcard_queue_handle`). Quando recebe os dados, ela grava as
 * informações correspondentes em um arquivo CSV na SD card. Se o arquivo não existir,
 * ele é criado e um cabeçalho apropriado é escrito.
 *
 * @param arg Parâmetro passado para a tarefa (não utilizado neste contexto).
 */
static void SDcard_task(void* arg) {
    SD_data_t data = {0};  // Estrutura para armazenar os dados recebidos da fila.
    char buffer[100];      // Buffer para leitura de linhas do arquivo.
    char path[50];         // Buffer para armazenar o caminho do arquivo.
    int er = 0;            // Variável para armazenar códigos de erro.

    for(;;) {
        // Aguarda até que dados sejam recebidos da fila SDcard_queue_handle.
        if(xQueueReceive(SDcard_queue_handle, &data, portMAX_DELAY) != pdTRUE) {
            USB_PRINT("error receive from queue\n");
            continue;  // Se ocorrer erro ao receber da fila, imprime mensagem e continua.
        }

        // Seleciona o caminho do arquivo com base no tipo de dado.
        er = path_select(path, sizeof(path), data.data_type);
        if(!er) {
            USB_PRINT("error selecting path\n");
            continue;  // Se a seleção do caminho falhar, imprime mensagem e continua.
        }

        // Verifica se o arquivo existe.
        er = f_stat(path, NULL);
        if(er == FR_NO_FILE) {
            // Se o arquivo não existir, cria um novo arquivo e adiciona um cabeçalho.
            f_open(&fil, path, FA_CREATE_ALWAYS | FA_WRITE);
            f_printf(&fil, "%s\n", path);  // Escreve o nome do arquivo no início.

            // Adiciona o cabeçalho com base no tamanho do array de valores.
            if(data.array_size == 1) {
                f_puts("timestamp,value\n", &fil);
            } else if(data.array_size == 3) {
                f_puts("timestamp,value1,value2,value3\n", &fil);
            } else {
                USB_PRINT("incompatible array size\n");
                f_close(&fil);  // Fecha o arquivo antes de continuar.
                continue;  // Se o tamanho do array for incompatível, continua.
            }

            f_close(&fil);  // Fecha o arquivo após adicionar o cabeçalho.
        }

        // Abre o arquivo em modo de adição e leitura.
        er = f_open(&fil, path, FA_OPEN_APPEND | FA_WRITE | FA_READ);
        if(er != FR_OK) {
            sprintf(buffer, "error: %d\n", er);
            USB_PRINT(buffer);  // Imprime o código de erro se a abertura do arquivo falhar.
            continue;
        }

        // Escreve os dados no arquivo, dependendo do tamanho do array.
        if(data.array_size == 1) {
            f_printf(&fil, "%u,%d\n", data.timestamp, data.value[0]);
        } else if(data.array_size == 3) {
            f_printf(&fil, "%u,%d,%d,%d\n", data.timestamp, data.value[0], data.value[1], data.value[2]);
        } else {
            USB_PRINT("incompatible array size\n");
            f_close(&fil);  // Fecha o arquivo antes de continuar.
            continue;
        }

        // Reposiciona o ponteiro do arquivo no início e lê o conteúdo para imprimir via USB.
        f_lseek(&fil, 0);
        while(f_gets(buffer, sizeof(buffer), &fil)) {
            USB_PRINT(buffer);  // Imprime cada linha do arquivo.
            memset(buffer, 0, sizeof(buffer));  // Limpa o buffer para a próxima leitura.
        }
        USB_PRINT("\n");
        f_close(&fil);  // Fecha o arquivo após o processamento.

    }
}

/**
 * Inicializa o sistema de armazenamento em SD card e inicia a tarefa de gerenciamento.
 *
 * A função `SDcard_start` realiza a configuração inicial necessária para o funcionamento
 * do sistema de armazenamento em SD card. Ela cria um semáforo e uma fila para comunicação
 * entre tarefas, monta o sistema de arquivos, reseta arquivos antigos, e inicia a tarefa
 * responsável por gravar dados na SD card.
 *
 * @return `true` se a inicialização foi bem-sucedida; `false` caso contrário.
 */
int SDcard_start(void) {
    // Cria um semáforo binário para sincronização (por exemplo, para impressão ou outras operações críticas).
    print_smpr_handle = xSemaphoreCreateBinary();

    // Cria uma fila para armazenar até 20 elementos do tipo SD_data_t, usada para comunicação entre tarefas.
    SDcard_queue_handle = xQueueCreate(20, sizeof(SD_data_t));

    // Monta o sistema de arquivos na SD card.
    if(f_mount(&fs, "", 0) != FR_OK) {
        USB_PRINT("fail mount sd card\n");
        return false;  // Retorna falso se o sistema de arquivos não puder ser montado.
    }

    // Reseta (deleta) arquivos antigos se existirem.
    if(reset_files() != true) {
        USB_PRINT("fail reseting files\n");
        return false;  // Retorna falso se ocorrer um erro ao resetar os arquivos.
    }

    // Cria e inicia a tarefa de gerenciamento da SD card.
    if(xTaskCreate(SDcard_task, "SDcard_task", 128*5, NULL, 26, NULL) != pdPASS) {
        return false;  // Retorna falso se a tarefa não puder ser criada.
    }

    return true;  // Retorna verdadeiro se tudo for inicializado com sucesso.
}


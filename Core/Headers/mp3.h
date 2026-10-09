#ifndef MP3_H
#define MP3_H

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
#include "Driver_USART.h"
#include "stdbool.h"
#include "string.h"
#include <stdint.h>

//reducir para ahorrar memoria
#define MP3_TX_STACK_SIZE   2048U   // pila del hilo TX (ajusta si crash)
#define MP3_RX_STACK_SIZE   2048U   // pila del hilo RX (un poco m?s por Receive/parse)

#define MP3_MSG_QUEUE_DEPTH 4U     // cola de comandos: pocos comandos en buffer
#define MP3_EVENT_QUEUE_DEPTH 6U   // cola de eventos parseados

//Comandos definidos que vamos a usar (CMD) que corresponden con el byte 4
#define NEXT_SONG 0x01
#define PREVIOUS_SONG 0x02
#define PLAY_WITH_INDEX 0x03
#define VOLUME_UP 0x04
#define VOLUME_DOWN 0x05
#define SET_VOLUME 0x06 
#define SELECT_DEVICE 0x09 
#define SLEEP_MODE 0x0A 
#define WAKE_UP 0x0B
#define RESET 0x0C 
#define PLAY 0x0D 
#define PAUSE 0x0E 
#define PLAY_WITH_FOLDER_AND_FILENAME 0x0F
#define STOP_PLAY 0x16
#define SHUFFLE 0x16
#define PLAY_VOLUME 0x22
#define PLAY_LOOP 0x11
#define END LOOP 0xAA

#define NUMERO_CANCIONES_ENCONTRADAS 0x48
#define CANCION_ACTUAL 0x4C
#define NUMERO_CARPETAS 0x4F
#define VOLUMEN_ACTUAL 0x43
#define FILE_NOT_FOUND 0x40
#define FINSIHED_PLAYING_FILE 0x3D

#define MP3_FLAG_TX_DONE 0x77
#define MP3_FLAG_RX_DONE 0x66

typedef struct{
	uint8_t comando[8]; //la trama se forma por 8 Bytes, segun el datasheet
	
}MSGQUEUE_MP3_TX_t; //Mensajes que enviamos al MP3

typedef struct{
	uint8_t comando[10]; //la trama se forma por 10 Bytes, segun el el mensaje de julian
	
}MSGQUEUE_MP3_RX_t; //Mensajes que recibimos al MP3


typedef enum {
    MP3_EVENT_UNKNOWN = 0,
    MP3_EVENT_DATA_OK,
    MP3_EVENT_TF_INSERTED,
    MP3_EVENT_TF_REMOVED,
    MP3_EVENT_FILE_NOT_FOUND,
    MP3_EVENT_FINISHED_PLAYING,
    MP3_EVENT_PLAYER_STOPPED,
    MP3_EVENT_PLAYER_PLAY,
    MP3_EVENT_PLAYER_PAUSED,
    MP3_EVENT_FILES_FOUND,
    MP3_EVENT_CURRENTLY_PLAYING,
    MP3_EVENT_FOLDERS_FOUND,
    MP3_EVENT_VOLUME_LEVEL
} MP3_EventType_t;

typedef struct {
    MP3_EventType_t type; /* tipo interpretado */
    uint8_t code;         /* buf[3] c?digo bruto */
    uint8_t sub;          /* buf[5] subc?digo / MSB par?metro */
    uint8_t value;        /* buf[6] LSB par?metro (?ndice, cantidad, volumen...) */
} MP3_Event_t;

/* Valores simples para que el hilo principal solo lea esta variable. */
typedef enum {
    MP3_SD_UNKNOWN = 0,
    MP3_SD_REMOVED = 1,
    MP3_SD_INSERTED = 2
} MP3_SD_State_t;

//int Init_Thread_mp3_test(void);
int Init_Thread_mp3(void);
void mp3_send_cmd(uint8_t cmd_macro, uint8_t param_msb, uint8_t param_lsb);

extern osMessageQueueId_t mp3_MsgQueue;
extern osMessageQueueId_t mp3_EventQueue;

extern MSGQUEUE_MP3_TX_t tx_msg;          // mensaje que enviamos al mp3
extern MSGQUEUE_MP3_RX_t rx_msg;          // mensaje que recibimos del mp3

extern volatile MP3_SD_State_t estado_sd;       // nos dice el estado de la tarjeta SD
extern volatile MP3_SD_State_t last_sd_state;   // Último estado conocido de SD
extern volatile MP3_Event_t ev;                 //dato ya parseado

#endif

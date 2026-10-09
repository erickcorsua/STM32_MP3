#ifndef COM_H
#define COM_H

#include "cmsis_os2.h"    // CMSIS RTOS2
#include "Driver_USART.h"  // CMSIS Driver para USART

// Definición de la cola
#define COM_PC_QUEUE_SIZE 10  // Número máximo de mensajes en la cola
extern osMessageQueueId_t com_pc_queue;  // Cola de mensajes
extern osThreadId_t com_pc_thread_id; //Definicion del thread.

// Estructura para los mensajes de comunicación
typedef struct {
    uint8_t* data;      // Puntero a los datos
    uint16_t length;    // Longitud de los datos
} com_pc_message_t;

void myUSART_callback(uint32_t event);
// Función de inicialización
void COM_PC_Init(void);

void COM_PC_SendData(uint8_t* data, uint16_t length);
// Función para enviar un mensaje al PC
void COM_PC_QueueMessage(uint8_t* data, uint16_t length);

#endif /* COM_H */
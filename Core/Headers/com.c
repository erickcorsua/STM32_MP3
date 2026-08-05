//Alberto Cortés y Erickson Correa

//CODIGO PROBADO Y FUNCIONAL

//COMO UTILIZAR ESTA FUNCION:
/*  COM_PC_QueueMessage((uint8_t *)"Comando enviado al MP3", 22);
    COM_PC_QueueMessage((uint8_t *)"Pista 05", 8);


SALIDA DEL TERA TERM

    Comando enviado al MP3
    Pista 05

*/

#include "com.h"
#include "string.h"
#include "stm32f4xx_hal.h"

// Definición de la cola global
osMessageQueueId_t com_pc_queue;  // Cola para almacenar los mensajes

// Buffer para datos USART
uint8_t uart_buffer[64];  // Tamaño de buffer de acuerdo con el USART

// Configuración del Driver USART
extern ARM_DRIVER_USART Driver_USART3;  // Asumiendo que estamos usando USART3

//Identificación del Thread (lo necesito para la flag)
osThreadId_t com_pc_thread_id;

//Flag para la ISR
#define COM_PC_FLAG_TX_COMPLETE (1<<0)

// LLAMO A ESTA CALLBACK AL TERMINAR LA TRANSMISION
void myUSART_callback(uint32_t event) {

  if(event & ARM_USART_EVENT_SEND_COMPLETE){
    //Despertar el hilo
    osThreadFlagsSet(com_pc_thread_id, COM_PC_FLAG_TX_COMPLETE);
  }
}
static void COM_PC_Task(void *argument){
    com_pc_message_t msg;
    osStatus_t status;

    while(1) {
        // Esperar un mensaje de la cola
        status = osMessageQueueGet(com_pc_queue, &msg, NULL, osWaitForever);

        if (status == osOK) {
            // Enviar los datos al PC
            COM_PC_SendData(msg.data, msg.length);
        }
    }
}

// Función de inicialización del módulo COM-PC
void COM_PC_Init(void) {
    // Crear la cola para almacenar los mensajes a enviar al PC
    com_pc_queue = osMessageQueueNew(COM_PC_QUEUE_SIZE, sizeof(com_pc_message_t), NULL); //Creamos una cola

    // Inicializar el driver USART (asumiendo que estamos usando USART3)
    Driver_USART3.Initialize(myUSART_callback);

    // Encender el periférico USART
    Driver_USART3.PowerControl(ARM_POWER_FULL);

    // Configurar el USART para modo asincrónico, con 8 bits de datos, sin paridad, 1 bit de parada, sin control de flujo
    Driver_USART3.Control(ARM_USART_MODE_ASYNCHRONOUS |
                           ARM_USART_DATA_BITS_8 |
                           ARM_USART_PARITY_NONE |
                           ARM_USART_STOP_BITS_1 |
                           ARM_USART_FLOW_CONTROL_NONE, 9600);

    // Habilitar líneas TX y RX
    Driver_USART3.Control(ARM_USART_CONTROL_TX, 1);
    Driver_USART3.Control(ARM_USART_CONTROL_RX, 1);

    // Crear la tarea de comunicación
    com_pc_thread_id = osThreadNew(COM_PC_Task, NULL, NULL);
}

// Función para enviar datos al PC a través de USART
void COM_PC_SendData(uint8_t* data, uint16_t length) {
  
    // Enviar los datos utilizando el driver USART
    Driver_USART3.Send(data, length);
  
    //Esperamos a que termine la transicion
    osThreadFlagsWait(COM_PC_FLAG_TX_COMPLETE, osFlagsWaitAny, osWaitForever);
}

// Tarea que lee de la cola y transmite los datos al PC

// Función para poner un mensaje en la cola
void COM_PC_QueueMessage(uint8_t* data, uint16_t length) {
  
    com_pc_message_t msg;
    msg.data = data;
    msg.length = length;

    // Poner el mensaje en la cola
    osMessageQueuePut(com_pc_queue, &msg, 0, osWaitForever);
}


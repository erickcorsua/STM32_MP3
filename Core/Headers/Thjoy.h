#ifndef THJOY_H
#define THJOY_H

#include "cmsis_os2.h"
#include "stdbool.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

// --- Pines Joystick --- (ajusta si cambias los pines o el micro)
#define THJOY_PIN_UP      GPIO_PIN_10
#define THJOY_PIN_RIGHT   GPIO_PIN_11
#define THJOY_PIN_DOWN    GPIO_PIN_12
#define THJOY_PIN_LEFT    GPIO_PIN_14
#define THJOY_PIN_CENTER  GPIO_PIN_15

#define THJOY_GPIO_PORT_UP_RIGHT          GPIOB
#define THJOY_GPIO_PORT_DOWN_LEFT_CENTER  GPIOE

#define THJOY_NVIC_LINE EXTI15_10_IRQn

// --- Se�ales internas ----
#define THJOY_FLAG_ISR 0x01

// -------- Evento de joystick (enum ampliable) --------
/*
typedef enum {
    THJOY_EVENT_NONE = 0,
    THJOY_EVENT_RIGHT,
    THJOY_EVENT_LEFT,
    THJOY_EVENT_UP,
    THJOY_EVENT_DOWN,
    THJOY_EVENT_CENTER
} Thjoy_Event_t;
*/



typedef uint8_t Thjoy_Mask_t; //--> mejora la legibilidad 

#define THJOY_MASK_UP     0x01u  // 0000 0001
#define THJOY_MASK_RIGHT  0x02u  // 0000 0010
#define THJOY_MASK_DOWN   0x04u  // 0000 0100
#define THJOY_MASK_LEFT   0x08u  // 0000 1000
#define THJOY_MASK_CENTER 0x10u  // 0001 0000

#define THJOY_MASK_UP_LONG     0x21u  // 0010 0001
#define THJOY_MASK_RIGHT_LONG  0x22u  // 0010 0010
#define THJOY_MASK_DOWN_LONG   0x24u  // 0010 0100
#define THJOY_MASK_LEFT_LONG   0x28u  // 0010 1000
#define THJOY_MASK_CENTER_LONG 0x30u  // 0011 0000

#define THJOY_QUEUE_SIZE 8



extern bool gb_joystick_init;
extern osMessageQueueId_t thjoy_queue_id;

// ---------- API para aplicaci�n ----------
void Thjoy_Init(void);                        // Inicializa driver y arranca hilo/timer
void Thjoy_ISR_Callback(void);
void EXTI15_10_IRQHandler(void);



#endif //THJOY_H

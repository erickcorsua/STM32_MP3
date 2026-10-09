#ifndef PWM_H
#define PWM_H

#include "cmsis_os2.h"

int Init_ThPwm (void);
//void MX_TIM1_Init(void);

typedef struct {

  uint16_t duracion_ms;
  uint8_t cantidad_bips;
  
}bipsMsg_t;

extern osThreadId_t tid_ThPwmConsumer;  //el hilo lo hacemos externo
extern osMessageQueueId_t tid_pwmQueue; //hago la cola externa para encolar en ella

#endif
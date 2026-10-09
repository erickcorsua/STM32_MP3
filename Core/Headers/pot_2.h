#ifndef POT_2_H
#define POT_2_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>

// ---------------- ADC ----------------
void ADC1_pins_F429ZI_config(void);
int ADC_Init_Single_Conversion(ADC_HandleTypeDef *, ADC_TypeDef *);
uint8_t ADC_getLevel30(ADC_HandleTypeDef *, uint32_t); // devuelve 0..29

// ---------------- Threads y cola ----------------
extern osThreadId_t tid_ThPot_2;        // thread productor
//extern osThreadId_t tid_ThConsumer;     // thread consumidor
extern osMessageQueueId_t tid_potQueue; // cola de niveles 0..29

extern float VOLUMEN;                   // valor normalizado 0.0–1.0

extern void ThPot_2(void *argument);    // función del thread productor
extern void ThConsumer(void *argument); // función del thread consumidor
extern int Init_ThPot_2(void);          // inicializa threads y cola

#endif

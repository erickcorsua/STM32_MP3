#ifndef PRINCIPAL_H
#define PRINCIPAL_H

#include "cmsis_os2.h"
#include "Thjoy.h"
#include "clock.h"
#include "LCD_SPI.h"
#include "leds_stm_v2_1.h"
#include "mp3.h"
#include "pot_2.h"
#include "pwm.h"
#include "temperatura.h"
#include "rgb.h"
#include "Driver_I2C.h"
#include "com.h"

 
typedef enum{
  REPOSO_MODE,
  REPRODUCCION_MODE,
  PROGRAMACION_HORA_MODE
}MODOS_t;

typedef enum {

  SEL_HORAS = 0,
  SEL_MINUTOS,
  SEL_SEGUNDOS
}seleccion_hora_t;

extern  seleccion_hora_t seleccionHora;

//Declaracion de funciones
void Init_Principal(void);
void Principal_Thread(void *argument);

#endif
#ifndef CLOCK_H
#define CLOCK_H

#include "cmsis_os2.h"
#include <stdint.h>

#define PERIODO_1S 1000U


/* Variables globales (salida del módulo) */
extern volatile uint8_t horas;    /* 0..23 */
extern volatile uint8_t minutos;  /* 0..59 */
extern volatile uint8_t segundos; /* 0..59 */

/* Inicializa y arranca un timer periódico.
   period_ms: periodo del timer en ms (usa 1000 si pasas 0).
   Devuelve 0 OK, <0 error. */
int Clock_Init(void);

/* Para y libera recursos del módulo */
void Clock_Deinit(void);

/* Ajusta la hora (hh 0..23, mm 0..59, ss 0..59). Devuelve 0 OK, <0 error. */
int Clock_SetTime(uint8_t hh, uint8_t mm, uint8_t ss);

extern osTimerId_t clock_timer;
//==================HILO DE TEST=====================
int Clock_CreateInitThread(void);
//===================================================

#endif /* CLOCK_H */

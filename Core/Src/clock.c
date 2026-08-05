//intro 
//titulo: clock.c
//autor: erick cs
//descripcion: Módulo reloj sencillo con timer CMSIS-RTOS2.
//              Añade además un hilo de init simple para arrancar el timer
//              después de osKernelStart (útil para depuración).


//-------------headers-------------
#include "clock.h"

//------------variables------------

//==================HILO DE TEST=====================
static osThreadId_t clock_init_thread;
static void Clock_InitThread(void *arg);
int Clock_CreateInitThread(void);
//===================================================

osTimerId_t clock_timer;

//publicas

volatile uint8_t horas    = 0;
volatile uint8_t minutos  = 0;
volatile uint8_t segundos = 0;


//funtions prototype

//privadas
static void Clock_TimerCallback(void *arg);

//publicas
int Clock_Init(void);
void Clock_Deinit(void);
int Clock_SetTime(uint8_t hh, uint8_t mm, uint8_t ss);


//funtions
/* Callback del timer: ejecuta cada periodo (por defecto 1000 ms)
   Incrementa segundos y gestiona el rollover a minutos/horas */
static void Clock_TimerCallback(void *arg){
  (void)arg;
	
  /* actualización simple y rápida */
  segundos++;
  
	if (segundos >= 60U) {
    segundos = 0U;
    minutos++;
    if (minutos >= 60U) {
      minutos = 0U;
      horas++;
      if (horas >= 24U) {
        horas = 0U;
      }
    }
  }
}
//-----------------------------------------
int Clock_Init(void){
  if (clock_timer != NULL) {
    return 0; /* ya inicializado */
  }
  /* Crear timer periódico */
  clock_timer = osTimerNew(Clock_TimerCallback, osTimerPeriodic, NULL, NULL);
  if (clock_timer == NULL) {
    return -1;  //sale y devuelve un -1
  }
	
	/* inicializar hora a 00:00:00 */
  horas = 0;
  minutos = 0;
  segundos = 0;
  /* Arrancar timer */
	
  if (osTimerStart(clock_timer, PERIODO_1S) != osOK) {
		
    osTimerDelete(clock_timer);
    clock_timer = NULL;
    return -2; //sale y devuelve un -2
  }
  
  return 0;//todo bien devuelve un 0
}
//-----------------------------------------
void Clock_Deinit(void){
	
  if (clock_timer != NULL) {
    osTimerStop(clock_timer);
    osTimerDelete(clock_timer);
    clock_timer = NULL;
  }
  horas = 0;
  minutos = 0;
  segundos = 0;
	
}
//-----------------------------------------
int Clock_SetTime(uint8_t hh, uint8_t mm, uint8_t ss){
  if (hh > 23U || mm > 59U || ss > 59U) return -1;
  /* Asignación directa a variables globales (sin sincronización) */
  horas = hh;
  minutos = mm;
  segundos = ss;
  return 0;
}
//-----------------------------------------

//==================HILO DE TEST=====================

/*
// Hilo de init sencillo: se ejecuta tras osKernelStart si se creó antes.
  // Llama a Clock_Init() y luego termina (retorna). 
static void Clock_InitThread(void *arg) {
  (void)arg;

  // Intentamos inicializar reloj. Si falla, nos quedamos en loop para facilitar debug. 
  Clock_Init();
  
  // Opcional: fijar una hora inicial para pruebas 
  //Clock_SetTime(12, 0, 0);

  // Este hilo ya no necesita ejecutar nada más: termina retornando. 
  return;
}

// Crea el hilo de inicialización. Debes llamar a esta función ANTES de osKernelStart().
  // Cuando el scheduler arranque, el hilo ejecutará Clock_Init() y se auto-terminará. 
int Clock_CreateInitThread(void) {
	
  clock_init_thread = osThreadNew(Clock_InitThread, NULL, NULL);
	
  if (clock_init_thread == NULL) {
    return -1; // fallo al crear hilo 
  }

  return 0;
}
*/
//===================================================




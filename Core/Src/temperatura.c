//intro 
//titulo: temperatura
//autor: erick cs
//autor: alberto cp
//descripcion: configura un sensor de temperatura I2C, usando el CMSIS driver
//
/*
                                                
 _/_                             _/_            
 /  _  _ _ _    ,_   _  _   __,  /  , , _   __, 
(__(/_/ / / /__/|_)_(/_/ (_(_/(_(__(_/_/ (_(_/(_
               /|                               
              (/                                
*/
#include "temperatura.h"

/* -------------------------- Variables estáticas -------------------------- */

extern ARM_DRIVER_I2C Driver_I2C1;
static ARM_DRIVER_I2C *I2Cdrv = &Driver_I2C1;

/* hilo y cola */
osThreadId_t tid_thread_temperatura = NULL;
osMessageQueueId_t queue_temp = NULL;

static uint8_t sensor_addr = TEMP_SENSOR_ADDR;          // dirección 7-bit 
static uint8_t rxbuf[2];                                // buffer rx 

volatile bool i2c_error = false;
//====================TEST======================
/* Variable global visible desde el depurador (actualizada con la última lectura en °C) */
volatile float dbg_temperature = NAN;
static osThreadId_t tid_hilotest = NULL;
volatile float hilotest_value;
//==============================================
static volatile float last_temperature = NAN;           // última temperatura (getter) 


/* ---------- Prototipos (declaraciones) de funciones ---------- */
//---------privadas------------- 
static void I2C_EventCallback(uint32_t event);
static float convert_11bit_0125(uint8_t msb, uint8_t lsb);
static int start_read_sequence_and_publish(void);
static void temp_thread(void *arg);

 // Prototipos de las funciones públicas
int Temp_Init(void);

/* -------------------------- Implementación -------------------------- */
/* Callback simple que despierta el hilo (driver I2C llamará a esta función) */

static void I2C_EventCallback(uint32_t event) {

  if (tid_thread_temperatura != NULL) {
		if (event & ARM_I2C_EVENT_TRANSFER_DONE) {
		 osThreadFlagsSet(tid_thread_temperatura, THREAD_FLAG_I2C);
    }
  }
}

/* Conversión 11-bit LSB=0.125 °C */
static float convert_11bit_0125(uint8_t msb, uint8_t lsb){
  uint16_t raw11 = ((uint16_t)msb << 3) | ((lsb >> 5) & 0x07);
  if (raw11 & (1 << 10)) {
    int16_t s = (int16_t)(raw11 | 0xF800);
    return (float)s * 0.125f;
  } else {
    return (float)raw11 * 0.125f;
  }
}

/* Inicia la secuencia: write pointer (no STOP) -> read 2 bytes -> convertir y publicar */
static int start_read_sequence_and_publish(void) {
	
  uint8_t reg = 0x00;         //segun la ficha tecnica sirve para temperature register
	uint32_t flags = false;
	
  /* 1) write pointer register (xfer_pending = true -> no STOP) */
	osThreadFlagsClear(THREAD_FLAG_I2C);
  if (I2Cdrv->MasterTransmit(sensor_addr, &reg, 1, true) != ARM_DRIVER_OK) {
		i2c_error = true;
    return -1;
  }
  /* esperar fin de transmisión (callback) */
	flags = osThreadFlagsWait(THREAD_FLAG_I2C, osFlagsWaitAny, I2C_STAGE_TIMEOUT_MS);
  if ((flags & THREAD_FLAG_I2C) != THREAD_FLAG_I2C) {
    /* no llegó la flag esperada (timeout u otro motivo) */
    i2c_error = true;
    return -1;
  }
  i2c_error = false;

	
  /* 2) read 2 bytes (xfer_pending = false -> genera STOP) */
	osThreadFlagsClear(THREAD_FLAG_I2C);
  if (I2Cdrv->MasterReceive(sensor_addr, rxbuf, 2, false) != ARM_DRIVER_OK) {
    i2c_error = true;
    return -1;
  }
	flags = osThreadFlagsWait(THREAD_FLAG_I2C, osFlagsWaitAny, I2C_STAGE_TIMEOUT_MS);
  if ((flags & THREAD_FLAG_I2C) != THREAD_FLAG_I2C) {
    i2c_error = true;
    return -1;
  }
  i2c_error = false;
	
  /* 3) convertir */
  float t = convert_11bit_0125(rxbuf[0], rxbuf[1]);
  last_temperature = t;      //variable que usamos para encolar

  /* 4) actualizar variable visible para el profesor y publicar en cola */
  dbg_temperature = t;       //con esta podemos observar la lectura de la temperatura, util para Watch
  if (queue_temp != NULL) {  //verifica si la cola existe y encolamos
    (void)osMessageQueuePut(queue_temp, &t, 0U, 0U); /* no bloqueante */
  }
  return 0;
}

/* Funcion del hilo que inicializa I2C y hace lecturas periódicas (1 s) */
static void temp_thread(void *arg) {
  (void)arg;

  /* Inicializar driver con callback */
  I2Cdrv->Initialize(I2C_EventCallback);
  I2Cdrv->PowerControl(ARM_POWER_FULL);
  I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
  I2Cdrv->Control(ARM_I2C_BUS_CLEAR, 0);

  osDelay(10);

  /* intento inicial para debuggear*/
  (void)start_read_sequence_and_publish();

  for (;;) {
		//cada segundo voy haciendo lecturas
    osDelay(1000);
    (void)start_read_sequence_and_publish();
  }
}

/* API pública --------------------------------------------------------------- */
int Temp_Init(void) {
  
	//creamos la cola
	if (queue_temp == NULL) {
    queue_temp = osMessageQueueNew(6, sizeof(float), NULL);
    if (queue_temp == NULL) return -1;
  }
	
	
  if (tid_thread_temperatura != NULL) return 0;

  tid_thread_temperatura = osThreadNew(temp_thread, NULL, NULL);
  return (tid_thread_temperatura == NULL) ? -1 : 0;
}
//-----------------------------------------



//========================hilo  test=============

/* Hilo test: simplemente coge elementos de la cola de temperatura y los guarda en hilotest_value */
static void hilotest_thread(void *arg) {
  (void)arg;
  float v;
  for (;;) {
    if (queue_temp != NULL) {
      /* Espera indefinidamente hasta que haya un mensaje en la cola */
      if (osMessageQueueGet(queue_temp, &v, NULL, osWaitForever) == osOK) {
        hilotest_value = v;
      }
    } else {
      /* Si la cola no existe aún, esperamos un poco */
      osDelay(100);
    }
  }
}

/* Inicializa el hilo test minimalista que lee la cola y carga una variable */
int HiloTest_Init(void) {

  tid_hilotest = osThreadNew(hilotest_thread, NULL, NULL);
  return (tid_hilotest == NULL) ? -1 : 0;
	
}

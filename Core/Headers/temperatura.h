#ifndef TEMPERATURA_H
#define TEMPERATURA_H

#include "Driver_I2C.h"
#include "cmsis_os2.h"
#include <math.h>    /* NAN */
#include <stdint.h>

#define TEMP_SENSOR_ADDR 0x48
#define THREAD_FLAG_I2C 0x0001U       // flag para sincronizar callback <-> hilo 
#define I2C_STAGE_TIMEOUT_MS 200     // timeout por etapa en ms 

extern volatile bool i2c_error;

int Temp_Init(void);
int HiloTest_Init(void);

extern volatile float dbg_temperature;
extern volatile float hilotest_value;

extern osMessageQueueId_t queue_temp;
extern osThreadId_t tid_thread_temperatura;

#endif /* TEMPERATURA_H */
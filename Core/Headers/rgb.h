#ifndef RGB_H
#define RGB_H


#include "RTE_Components.h"
#include "stm32f4xx_hal.h"
#include "stdbool.h"
#include "cmsis_os2.h"
#include <stdint.h>

//--------struct-------------
typedef struct {
char color;
//bool on;
uint8_t freq;
}led_t;

//--------variables-----------

extern volatile bool gb_rgb_green_on;
extern volatile bool gb_rgb_blue_on;
extern volatile bool gb_rgb_red_on;
//----------------------------
//-----funtions prototype-----

void rgb_reproduciendo(void );
void rgb_pausado(void );
void rgb_no_hay_sd(void);
void rgb_apagarlo(void);

void rgb_green_on(void);
void rgb_blue_on(void);
void rgb_red_on(void);

void rgb_green_off(void);
void rgb_blue_off(void);
void rgb_red_off(void);

void rgb_green_alt(void);
void rgb_blue_alt(void);
void rgb_red_alt(void);

void rgb_mk_white(void);
void rgb_mk_dark(void);
void rgb_mk_yellow(void);
void rgb_mk_cyan(void);
void rgb_mk_magenta(void);


//----------------------------
//---------THREAD-------------
extern osThreadId_t tid_ThreadRGB;
extern osTimerId_t  ledTimer;
extern osMessageQueueId_t tid_QueueRGB;

extern int Init_ThreadRGB (void);


#endif //RGB_H
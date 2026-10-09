#ifndef LEDS_STM_V2_1_H
#define LEDS_STM_V2_1_H


#include "RTE_Components.h"
#include "stm32f4xx_hal.h"
#include "stdbool.h"


//--------variables-----------

extern volatile bool gb_leds_stm_green_on;
extern volatile bool gb_leds_stm_blue_on;
extern volatile bool gb_leds_stm_red_on;



//----------------------------
//-----funtions prototype-----

void leds_stm_init(void);

void leds_stm_encender_all(void);
void leds_stm_apagar_all(void);

void leds_stm_encender_green(void);
void leds_stm_encender_blue(void);
void leds_stm_encender_red(void);

void leds_stm_apagar_green(void);
void leds_stm_apagar_blue(void);
void leds_stm_apagar_red(void);

void leds_stm_alternar_green(void);
void leds_stm_alternar_blue(void);
void leds_stm_alternar_red(void);



//----------------------------


#endif //LEDS_STM_V2_1_H
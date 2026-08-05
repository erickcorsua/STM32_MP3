// intro
// titulo: leds_stm version 2.1
// autor: erick cs
// descripcion: gestion de leds LD1(verde), LD2(azul), LD3(rojo) en PB0, PB7, PB14

#include "leds_stm_v2_1.h"
#include "stm32f4xx_hal.h"
#include "stdbool.h"



// -------- macros --------
#define LED_GREEN_PIN GPIO_PIN_0
#define LED_BLUE_PIN  GPIO_PIN_7
#define LED_RED_PIN   GPIO_PIN_14
#define LEDS_PORT     GPIOB
// ------------------------
//--------variables-----------

static GPIO_InitTypeDef GPIO_InitStruct;
static bool gb_leds_stm_init = false;

volatile bool gb_leds_stm_green_on = false;
volatile bool gb_leds_stm_blue_on = false;
volatile bool gb_leds_stm_red_on = false;


// ------------------------
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

void leds_stm_Test(void);

// ------------------------
// ---------------- funciones ----------------
void leds_stm_init(void) {
    if (!gb_leds_stm_init) {
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull  = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Pin   = LED_GREEN_PIN | LED_BLUE_PIN | LED_RED_PIN;
			
        HAL_GPIO_Init(LEDS_PORT, &GPIO_InitStruct);
			
			  gb_leds_stm_green_on = false;
        gb_leds_stm_blue_on = false;
			  gb_leds_stm_red_on = false;

        gb_leds_stm_init = true;
    }
}

void leds_stm_encender_all(void) {
    HAL_GPIO_WritePin(LEDS_PORT, LED_GREEN_PIN | LED_BLUE_PIN | LED_RED_PIN, GPIO_PIN_SET);
}

void leds_stm_apagar_all(void) {
    HAL_GPIO_WritePin(LEDS_PORT, LED_GREEN_PIN | LED_BLUE_PIN | LED_RED_PIN, GPIO_PIN_RESET);
}

void leds_stm_encender_green(void){ HAL_GPIO_WritePin(LEDS_PORT, LED_GREEN_PIN, GPIO_PIN_SET); gb_leds_stm_green_on = true;}
void leds_stm_encender_blue(void){  HAL_GPIO_WritePin(LEDS_PORT, LED_BLUE_PIN, GPIO_PIN_SET);  gb_leds_stm_blue_on  = true;}
void leds_stm_encender_red(void){   HAL_GPIO_WritePin(LEDS_PORT, LED_RED_PIN, GPIO_PIN_SET);   gb_leds_stm_red_on   = true;}

void leds_stm_apagar_green(void){ HAL_GPIO_WritePin(LEDS_PORT, LED_GREEN_PIN, GPIO_PIN_RESET); gb_leds_stm_green_on = false;}
void leds_stm_apagar_blue(void){  HAL_GPIO_WritePin(LEDS_PORT, LED_BLUE_PIN, GPIO_PIN_RESET);  gb_leds_stm_blue_on  = false;}
void leds_stm_apagar_red(void){   HAL_GPIO_WritePin(LEDS_PORT, LED_RED_PIN, GPIO_PIN_RESET);   gb_leds_stm_red_on   = false; }

void leds_stm_alternar_green(void){ HAL_GPIO_TogglePin(LEDS_PORT, LED_GREEN_PIN); gb_leds_stm_green_on=(HAL_GPIO_ReadPin(LEDS_PORT, LED_GREEN_PIN)==GPIO_PIN_SET)?true:false;}
void leds_stm_alternar_blue(void){ HAL_GPIO_TogglePin(LEDS_PORT, LED_BLUE_PIN);   gb_leds_stm_blue_on =(HAL_GPIO_ReadPin(LEDS_PORT, LED_BLUE_PIN)==GPIO_PIN_SET)?true:false;}
void leds_stm_alternar_red(void){ HAL_GPIO_TogglePin(LEDS_PORT, LED_RED_PIN);     gb_leds_stm_red_on  =(HAL_GPIO_ReadPin(LEDS_PORT, LED_RED_PIN)==GPIO_PIN_SET)?true:false;}



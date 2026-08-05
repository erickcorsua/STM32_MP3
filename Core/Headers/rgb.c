//intro 
//titulo: rgb.c
//autor: Alberto Cortés y Erick Corsua
//descripcion: inicia los gpio necesarios, para usar el rgb de la mbed board
//-----------------------------------------
/*
                    d8b      
                    ?88      
                     88b     
  88bd88b d888b8b    888888b 
  88P'  `d8P' ?88    88P `?8b
 d88     88b  ,88b  d88,  d88
d88'     `?88P'`88bd88'`?88P'
                )88          
               ,88P          
           `?8888P           

*/
#include "rgb.h"
#include "stm32f4xx_hal.h"
#include "stdbool.h"
#include "cmsis_os2.h"
#include <stdint.h>

// -------- macros --------
#define LED_GREEN_PIN GPIO_PIN_12
#define LED_BLUE_PIN  GPIO_PIN_11
#define LED_RED_PIN   GPIO_PIN_13
#define LEDS_PORT     GPIOD
// ------------------------
//--------variables-----------

//Timer para la frecuencia de parpadeo

osTimerId_t  ledTimer;


//Thread del RGB

//para el hilo
osThreadId_t tid_ThreadRGB;
void ThreadRGB (void *argument);
//cola
osMessageQueueId_t tid_QueueRGB;
//incia el hilo 
int Init_ThreadRGB (void);

static led_t led;

static GPIO_InitTypeDef GPIO_InitStruct;
static bool gb_rgb_init = false;

volatile bool gb_rgb_green_on = false;
volatile bool gb_rgb_blue_on = false;
volatile bool gb_rgb_red_on = false;

// ------------------------
//-----funtions prototype-----
static void rgb_init(void);
static void LedTimerCallback(void *argument);

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


// ------------------------
// ---------------- funciones ----------------
static void rgb_init(void) {
    if (!gb_rgb_init) {
        __HAL_RCC_GPIOD_CLK_ENABLE();

        GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull  = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Pin   = LED_GREEN_PIN | LED_BLUE_PIN | LED_RED_PIN;
			
        HAL_GPIO_Init(LEDS_PORT, &GPIO_InitStruct);
			
			  rgb_mk_dark();

        gb_rgb_init = true;
    }
}
//--------------------------------------------
void rgb_green_on(void){ HAL_GPIO_WritePin(LEDS_PORT, LED_GREEN_PIN, GPIO_PIN_RESET); gb_rgb_green_on = true; }
void rgb_blue_on(void){  HAL_GPIO_WritePin(LEDS_PORT, LED_BLUE_PIN, GPIO_PIN_RESET);  gb_rgb_blue_on  = true; }
void rgb_red_on(void){   HAL_GPIO_WritePin(LEDS_PORT, LED_RED_PIN, GPIO_PIN_RESET); gb_rgb_red_on  = true; }
	
void rgb_green_off(void){ HAL_GPIO_WritePin(LEDS_PORT, LED_GREEN_PIN, GPIO_PIN_SET); gb_rgb_green_on = false; }
void rgb_blue_off(void){  HAL_GPIO_WritePin(LEDS_PORT, LED_BLUE_PIN, GPIO_PIN_SET);  gb_rgb_blue_on  = false; }
void rgb_red_off(void){   HAL_GPIO_WritePin(LEDS_PORT, LED_RED_PIN, GPIO_PIN_SET); gb_rgb_red_on  = false; }
		
void rgb_green_alt(void){ HAL_GPIO_TogglePin(LEDS_PORT, LED_GREEN_PIN); gb_rgb_green_on = (HAL_GPIO_ReadPin(LEDS_PORT,LED_GREEN_PIN) == GPIO_PIN_RESET)?true:false;}
void rgb_blue_alt(void){  HAL_GPIO_TogglePin(LEDS_PORT, LED_BLUE_PIN);  gb_rgb_blue_on  = (HAL_GPIO_ReadPin(LEDS_PORT,LED_BLUE_PIN) == GPIO_PIN_RESET) ?true:false; }
void rgb_red_alt(void){   HAL_GPIO_TogglePin(LEDS_PORT, LED_RED_PIN); gb_rgb_red_on  = (HAL_GPIO_ReadPin(LEDS_PORT,LED_RED_PIN) == GPIO_PIN_RESET)  ?true:false; }
		
void rgb_mk_white(void){ 

	rgb_green_on();
	rgb_blue_on();
	rgb_red_on();

}
void rgb_mk_dark(void){

	rgb_green_off();
	rgb_blue_off();
	rgb_red_off();

}
void rgb_mk_yellow(void){

	rgb_green_on();
	rgb_red_on();

}
void rgb_mk_cyan(void){

	rgb_green_on();
	rgb_blue_on();

}
void rgb_mk_magenta(void){

	rgb_red_on();
	rgb_blue_on();

}


//funciones para el hilo
int Init_ThreadRGB (void) {
  //incia un timer periodico
  ledTimer = osTimerNew(LedTimerCallback, osTimerPeriodic, NULL, NULL);
 
  if (ledTimer == NULL){return -1;}

  tid_QueueRGB = osMessageQueueNew(6, sizeof(led_t), NULL);  //cola
  tid_ThreadRGB = osThreadNew(ThreadRGB, NULL, NULL);        //hilo
  
  if (tid_ThreadRGB == NULL) {
    return(-1);
  }
 
  return(0);
}

void ThreadRGB (void *argument) {
  
  rgb_init();//inicia el Hardware
 
  while (1) {
    
    osMessageQueueGet(tid_QueueRGB, &led, NULL, osWaitForever); //esperamos a que haya algo en la cola
    
		//------configuramos el timer---------
    osTimerStop(ledTimer);
    
    if (led.freq == 4 && led.color == 'G')
        {
					  rgb_blue_off();
					  rgb_red_off();
            osTimerStart(ledTimer, 250); // 4 Hz
        }
        else if (led.freq == 1 && led.color == 'B')
        {   
					  rgb_green_off();
					  rgb_red_off();  					
            osTimerStart(ledTimer, 1000); // 1 Hz
        }
				else if (led.freq == 5 && led.color == 'R')
        {   
					  rgb_green_off();
					  rgb_blue_off();  					
            osTimerStart(ledTimer, 200); // 1 Hz
        }
        else
        {
            rgb_blue_off();
            rgb_green_off();
            rgb_red_off();
        }
		//-----------------------------------
     
    }
  }

  //callback del timer
  
static void LedTimerCallback(void *argument){
    if (led.color == 'G')
    {
        rgb_green_alt();
    }
    else if (led.color == 'B')
    {
        rgb_blue_alt();
    }else{
		    rgb_red_alt();
		}
}
  
void rgb_reproduciendo(void){

        led.color = 'G';
        led.freq = 4;
        osMessageQueuePut(tid_QueueRGB, &led, NULL, NULL);
}

void rgb_pausado(void){

        led.color = 'B';
        led.freq = 1;
        osMessageQueuePut(tid_QueueRGB, &led, NULL, NULL);
}
void rgb_no_hay_sd(void){

        led.color = 'R';
        led.freq = 5;
        osMessageQueuePut(tid_QueueRGB, &led, NULL, NULL);
}
void rgb_apagarlo(void){
	
  rgb_blue_off();rgb_green_off();rgb_red_off();
	osTimerStop(ledTimer);
	
}

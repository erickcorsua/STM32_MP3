#ifndef LCD_SPI_H
#define LCD_SPI_H

#include "Driver_SPI.h"           
#include "RTE_Components.h"           
#include "RTE_Device.h"           
#include "stdbool.h"
#include "stm32f4xx_hal.h" 
#include "string.h"
#include "stdio.h"
#include "cmsis_os2.h"

// -------- macros --------

//---------------pines y puertos---------------
#define LCD_RESET_PIN GPIO_PIN_6
#define LCD_A0_PIN    GPIO_PIN_13
#define LCD_CS_N_PIN  GPIO_PIN_14

#define LCD_RESET_PORT GPIOA
#define LCD_A0_PORT    GPIOF
#define LCD_CS_N_PORT  GPIOD

// -------- Comandos LCD --------
#define LCD_DISPLAY_OFF        0xAE
#define LCD_DISPLAY_ON         0xAF
#define LCD_SET_BIAS_1_9       0xA2
#define LCD_RAM_NORMAL         0xA0
#define LCD_COM_SCAN_NORMAL    0xC8
#define LCD_INTERNAL_RES_2     0x22
#define LCD_POWER_ON           0x2F
#define LCD_START_LINE_0       0x40
#define LCD_SET_CONTRAST       0x81
#define LCD_CONTRAST_VALUE     0x18  // normal
#define LCD_ALL_POINTS_NORMAL  0xA4
#define LCD_DISPLAY_NORMAL     0xA6


//flag

#define FLAG_MY_CALLBACK 0x01

/* API de la cola / tarea */

#define LCD_QUEUE_SIZE 8

 extern osMessageQueueId_t lcdQueue; //habra que poner la cola que consuma el LCD, de momento esta esta
 extern osThreadId_t lcdThreadId;


typedef struct {
	
  uint8_t line;               // 1 o 2
  const char *text;
//  uint8_t freq; //Hz
	
} LCD_Message_t;

extern ARM_DRIVER_SPI Driver_SPI1;

extern volatile bool error_envio;

extern unsigned char buffer[512];
extern uint16_t positionL1;
extern uint16_t positionL2;

void LCD_Init(void);

void LCD_symbolToLocalBuffer(uint8_t line, const char *text);

void LCD_write_ERROR(uint8_t line);
void LCD_clean(uint8_t line);

void LCD_print_prueba_valores(int valor1, float valor2);


#endif //LCD_SPI_H
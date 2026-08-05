/**********************************************************

autor       :  erick cs y Alberto.c
titulo      :  LCD_SPI.c
descripcion :  se usa el CMSIS para usar un Driver_SPI. Para este caso se usa el SPI1, con los pines
               pa7 -> mosi
               pa5 -> clk
               pa6 -> rst
               pf13 -> ao (comando / dato)
               pd14 -> chip select
fecha:

*/

#include "LCD_SPI.h"    
#include "Arial12x12.h"    

//variables
extern ARM_DRIVER_SPI Driver_SPI1;

static ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;

static GPIO_InitTypeDef GPIO_InitStruct;

volatile bool error_envio = false;

/* RTOS  */
 osMessageQueueId_t lcdQueue; //habra que poner la cola que consuma el LCD, de momento esta esta
 osThreadId_t lcdThreadId;

static bool volatile spi_terminado = false;
static bool volatile b_lcd_init = false;

unsigned char buffer[512] = {0};

uint16_t positionL1 = 0;
uint16_t positionL2 = 0;

//prototype funtions
/* Prototipos internos */
static void gpio_init(void);
static void SPI_Init(void);
static void LCD_Reset(void);
static void LCD_wr_data(unsigned char data);
static void LCD_wr_cmd(unsigned char cmd);
static void LCD_update(void);
static void LCD_symbolToLocalBuffer_L1(const char *str);
static void LCD_symbolToLocalBuffer_L2(const char *str);
static void lcd_thread(void *arg);


void LCD_Init(void);
void symbolToLocalBuffer_L1(uint8_t symbol);
void LCD_symbolToLocalBuffer(uint8_t line, const char *text);

void LCD_write_ERROR(uint8_t line);
void LCD_clean(uint8_t line);
void LCD_print_prueba_valores(int valor1, float valor2);

//static void mySPI_callback(uint32_t event);
static void mySPI_callback(uint32_t event);


//funtions

/*        __          __  .__               
  _______/  |______ _/  |_|__| ____   ______
 /  ___/\   __\__  \\   __\  |/ ___\ /  ___/
 \___ \  |  |  / __ \|  | |  \  \___ \___ \ 
/____  > |__| (____  /__| |__|\___  >____  >
     \/            \/             \/     \/ 
*/
static void gpio_init(void){

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  
  GPIO_InitStruct.Pin   = LCD_RESET_PIN;
  HAL_GPIO_Init(LCD_RESET_PORT, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin   = LCD_A0_PIN;
  HAL_GPIO_Init(LCD_A0_PORT, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin   = LCD_CS_N_PIN;
  HAL_GPIO_Init(LCD_CS_N_PORT, &GPIO_InitStruct);
 
  //por defecto, para comprobar si estan bien
 
  HAL_GPIO_WritePin(LCD_RESET_PORT, LCD_RESET_PIN, GPIO_PIN_SET);  // Estado alto por defecto (sin reset)
  HAL_GPIO_WritePin(LCD_A0_PORT, LCD_A0_PIN, GPIO_PIN_SET);        // A0 en alto por defecto
  HAL_GPIO_WritePin(LCD_CS_N_PORT, LCD_CS_N_PIN, GPIO_PIN_SET);    // CS inactivo 
  
}

static void mySPI_callback(uint32_t event){
	if(lcdThreadId != NULL){
		if (event & ARM_SPI_EVENT_TRANSFER_COMPLETE){   
      osThreadFlagsSet(lcdThreadId, FLAG_MY_CALLBACK);    
		}
	}
}

static void SPI_Init(void){

  //SPIdrv->Initialize(mySPI_callback);
	SPIdrv->Initialize(mySPI_callback);
  SPIdrv->PowerControl(ARM_POWER_FULL);
  SPIdrv->Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_MSB_LSB |
	ARM_SPI_DATA_BITS(8), 20000000);//poner a 1MHz para ver mejor todo en el analizador logico 1000000 el correcto es de: 20000000
  SPIdrv->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);

  gpio_init();
  
}
//--------------------------------------------------------------

static void LCD_symbolToLocalBuffer_L1(const char *str){
  //como una pagina mide 8 pixeles de alto y queremos letras mas grandes, tenemos que dibujar en dos paginas.
  // El orden de pintar es: pantalla 0 columna 0, pantalla 1 columna 0, pantalla 0 columna 1, pantalla 1 columna 1...
  uint8_t j,i, value1, value2,width;
  uint16_t offset = 0;
  positionL1 = 0;

    
	for(j=0;j < strlen(str);j++){
      
    offset = 25*(str[j] - ' ');
		   
	  width = Arial12x12[offset];
    
	  if (positionL1 + width > 128){
      LCD_write_ERROR(1);
      return;
    }
		 
    for(i = 0; i < 12; i++){
    
      value1 = Arial12x12[offset + i*2 + 1]; //cogemos datos alternos con value1 y value2. 
      value2 = Arial12x12[offset + i*2 + 2];

      buffer[i + positionL1] = value1;
      buffer[i + 128 + positionL1] = value2;
    } 
    positionL1 = positionL1 + Arial12x12[offset];//si ves que se pegan o se solapan poner un +1
      
  }
   
	LCD_update();  
			 
}
  
 static void LCD_symbolToLocalBuffer_L2(const char *str){
  //como una pagina mide 8 pixeles de alto y queremos letras mas grandes, tenemos que dibujar en dos paginas.
  // El orden de pintar es: pantalla 0 columna 0, pantalla 1 columna 0, pantalla 0 columna 1, pantalla 1 columna 1...
    
	uint8_t j,i, value1, value2,width;
  uint16_t offset = 0;
  positionL2 = 0;

    
	for(j=0;j < strlen(str);j++){
      
    offset = 25*(str[j] - ' ');
		   
	  width = Arial12x12[offset];
    
	  if (positionL2 + width > 128){
      LCD_write_ERROR(2);
      return;
    }
		 
    for(i = 0; i < 12; i++){
    
      value1 = Arial12x12[offset + i*2 + 1]; //cogemos datos alternos con value1 y value2. 
      value2 = Arial12x12[offset + i*2 + 2];

      buffer[i + 256 + positionL2] = value1;
      buffer[i + 384 + positionL2] = value2;
    } 
    positionL2 = positionL2 + Arial12x12[offset]; //si ves que se pegan o se solapan poner un +1
      
  }
   
	LCD_update();  
			 
}
//------------------------------------------------- 

static void LCD_wr_data(unsigned char data){

	spi_terminado = false;

  osThreadFlagsClear(FLAG_MY_CALLBACK);
	
  // Seleccionar CS = 0;
  HAL_GPIO_WritePin(LCD_CS_N_PORT, LCD_CS_N_PIN, GPIO_PIN_RESET); 
  // Seleccionar A0 = 1; 
  HAL_GPIO_WritePin(LCD_A0_PORT, LCD_A0_PIN, GPIO_PIN_SET); 
	// Escribir un dato (data) usando la función SPIDrv->Send(…);
	SPIdrv->Send(&data, sizeof(data)); 
	uint32_t flags = osThreadFlagsWait(FLAG_MY_CALLBACK,osFlagsWaitAny,200);
	if((flags & FLAG_MY_CALLBACK) != FLAG_MY_CALLBACK) {
		error_envio = true; 
  }else{error_envio = false;}
	HAL_GPIO_WritePin(LCD_CS_N_PORT, LCD_CS_N_PIN, GPIO_PIN_SET); 

}
//--------------------------------------------------------------
static void LCD_wr_cmd(unsigned char cmd){
	
	spi_terminado = false;
	
	osThreadFlagsClear(FLAG_MY_CALLBACK);
	
 // Seleccionar CS = 0;
 HAL_GPIO_WritePin(LCD_CS_N_PORT, LCD_CS_N_PIN, GPIO_PIN_RESET); 
 // Seleccionar A0 = 0;
 HAL_GPIO_WritePin(LCD_A0_PORT, LCD_A0_PIN, GPIO_PIN_RESET); 
 // Escribir un comando (cmd) usando la función SPIDrv->Send(…);
 
	SPIdrv->Send(&cmd, sizeof(cmd)); 
	uint32_t flags = osThreadFlagsWait(FLAG_MY_CALLBACK,osFlagsWaitAny,200);
	if((flags & FLAG_MY_CALLBACK) != FLAG_MY_CALLBACK) {
		error_envio = true; 
  }else{error_envio = false;}
 
 HAL_GPIO_WritePin(LCD_CS_N_PORT, LCD_CS_N_PIN, GPIO_PIN_SET); 
}

//--------------------------------------------------------------
static void LCD_update(void){
	// con esto dibujas el cuadro
  /*
  for (int i = 0; i < 8; i++) {
    buffer[i] = 0xFF;  // 11111111 ? 8 píxeles encendidos en vertical
  } 
	
	//----------------------
	*/
  /*
  
	 unsigned char letraA[8] = {0x00, 0x30, 0x46, 0x40, 0x40, 0x46, 0x30, 0x00};
	
	for (int i = 0; i < 8; i++) {

   	buffer[i] = letraA[i];  // 

  }*/
 int i;
 LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
 LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
 LCD_wr_cmd(0xB0); // Página 0
 for(i=0;i<128;i++){LCD_wr_data(buffer[i]);}

 LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
 LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
 LCD_wr_cmd(0xB1); // Página 1
 for(i=128;i<256;i++){LCD_wr_data(buffer[i]);}

 LCD_wr_cmd(0x00);
 LCD_wr_cmd(0x10);
 LCD_wr_cmd(0xB2); //Página 2
 for(i=256;i<384;i++){LCD_wr_data(buffer[i]);}

 LCD_wr_cmd(0x00);
 LCD_wr_cmd(0x10);
 LCD_wr_cmd(0xB3); // Pagina 3
 for(i=384;i<512;i++){ LCD_wr_data(buffer[i]);}
}

//--------------------------------------------------------------
static void LCD_Reset(void){
  HAL_GPIO_WritePin(LCD_RESET_PORT, LCD_RESET_PIN, GPIO_PIN_RESET); // reset activo (bajo)

  // Si el kernel está corriendo usar osDelay para ceder CPU
  if (osKernelGetState() == osKernelRunning) {
    osDelay(10);
  } else {
    HAL_Delay(10);
  }

  HAL_GPIO_WritePin(LCD_RESET_PORT, LCD_RESET_PIN, GPIO_PIN_SET);   // reset inactivo (alto)

  if (osKernelGetState() == osKernelRunning) {
    osDelay(100);
  } else {
    HAL_Delay(100);
  }
}
//--------------------------------------------------------------
/*               __                              
  ____ ___  ____/  |_  ___________  ____   ______
_/ __ \\  \/  /\   __\/ __ \_  __ \/    \ /  ___/
\  ___/ >    <  |  | \  ___/|  | \/   |  \\___ \ 
 \___  >__/\_ \ |__|  \___  >__|  |___|  /____  >
     \/      \/           \/           \/     \/ 
*/


void LCD_Init(void){
  if(!b_lcd_init){
		
		b_lcd_init=true;
		
	  SPI_Init();      // Inicializa SPI y GPIOs
    LCD_Reset();     // Pulso de reset

	
	  LCD_wr_cmd(LCD_DISPLAY_OFF);
    LCD_wr_cmd(LCD_SET_BIAS_1_9);
	
    LCD_wr_cmd(LCD_RAM_NORMAL);
    LCD_wr_cmd(LCD_COM_SCAN_NORMAL);
 	
    LCD_wr_cmd(LCD_INTERNAL_RES_2);
    LCD_wr_cmd(LCD_POWER_ON);
	
    LCD_wr_cmd(LCD_START_LINE_0);
    LCD_wr_cmd(LCD_DISPLAY_ON);
	
    LCD_wr_cmd(LCD_SET_CONTRAST);
    LCD_wr_cmd(LCD_CONTRAST_VALUE);
	
    LCD_wr_cmd(LCD_ALL_POINTS_NORMAL);
    LCD_wr_cmd(LCD_DISPLAY_NORMAL);
		
		lcdThreadId = osThreadNew(lcd_thread, NULL, NULL);
		
		//Hilo de test (descomentar para usar)
//		Init_Thread_productor_test();
		lcdQueue = osMessageQueueNew(LCD_QUEUE_SIZE, sizeof(LCD_Message_t), NULL);
		
  }
}

//--------------------------------------------------------------

void LCD_symbolToLocalBuffer(uint8_t line, const char *text){
  
  if(line == 1){LCD_symbolToLocalBuffer_L1(text);}
  if(line == 2){LCD_symbolToLocalBuffer_L2(text);}

}

//-------------------------------------------------
void LCD_write_ERROR(uint8_t line){

  LCD_clean(line);
  
  LCD_symbolToLocalBuffer(line,"ERROR");   

  LCD_update(); // función que envía el buffer completo al LCD
}
//-------------------------------------------------
void LCD_clean(uint8_t line){
  
    // Limpiar el buffer antes de escribir (opcional)
  if(line == 1){
    for (int i = 0; i < 256; i++){ 
        buffer[i] = 0x00;
		}
		positionL1 = 0;
		
  }
	else{
    for (int i = 256; i < 512; i++){ 
      buffer[i] = 0x00;
		}
		positionL2 = 0;	
  }
	
	LCD_update();
	
}
//-------------------------------------------------  
void LCD_print_prueba_valores(int valor1, float valor2){
    char line1[32];
    char line2[32];

    // Formatea la primera línea: entero
    sprintf(line1, "Prueba valor1: %d", valor1);
    // Formatea la segunda línea: flotante con 5 decimales
    sprintf(line2, "Prueba valor2: %.5f", (double)valor2);

    // Imprime en la línea superior (1) y en la inferior (2)
    LCD_symbolToLocalBuffer(1, line1);
    LCD_symbolToLocalBuffer(2, line2);
}


/* ------------------ RTOS: hilo y cola minimalistas ------------------ */

/* Hilo que inicializa el LCD y consume la cola */
static void lcd_thread(void *arg){
  (void)arg;

  LCD_Message_t msg;
	
	LCD_clean(1);
	LCD_clean(2);
	
  while(1) {
		
    if (osMessageQueueGet(lcdQueue, &msg, NULL, osWaitForever) == osOK) {
      /* Procesa un solo evento: limpiar la línea y escribir */
      uint8_t line = (msg.line == 2) ? 2 : 1;
			
      LCD_clean(line);
      LCD_symbolToLocalBuffer(line, msg.text);
			
      /* las funciones de conversión llaman a LCD_update() */
    }
  }
}
//---------------------------------------------------------------

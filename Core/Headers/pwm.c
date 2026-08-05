#include "pwm.h"
#include "stm32f4xx_hal.h"
 
/*----------------------------------------------------------------------------
  _______          ____  __ 
 |  __ \ \        / /  \/  |
 | |__) \ \  /\  / /| \  / |
 |  ___/ \ \/  \/ / | |\/| |
 | |      \  /\  /  | |  | |
 |_|       \/  \/   |_|  |_|
                                                       
 *---------------------------------------------------------------------------*/
 
//variables para la inicializacion del timer 
static TIM_HandleTypeDef htim1;
static TIM_OC_InitTypeDef sConfigOC;


//Declaracion de la cola y los hilos productor y consumidor.
osMessageQueueId_t tid_pwmQueue;
osThreadId_t tid_ThPwmConsumer;
//osThreadId_t tid_ThPwmProducer;

//prototype funtions
//privadas
static void MX_TIM1_Init(void);           //sirve para inicializar el hardware
static void PWM_Start(void);              //funcion que da start al pwm
static void PWM_Stop(void);               //funcion que da stop al pwm
static void ThPwmConsumer(void *argument);//funcion de hilo

//publicas
int Init_ThPwm (void);
//void ThPwmProducer (void *argument);


//funtions
//Funcion de inicializacion del GPIO y del htim1
static void MX_TIM1_Init(void){
	
	GPIO_InitTypeDef GPIO_InitStruct;
  
  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
    
  GPIO_InitStruct.Pin = GPIO_PIN_9;      // PE9 TIM1_CH1
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 84-1;      //Preescaler bajo afina, preescaler alto desafina.
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 2274-1;       // nota LA
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  HAL_TIM_PWM_Init(&htim1);
  
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  
  HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);
   
}

//funcion que inicia el timer
static void PWM_Start(void){
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

//funcion que pausa el timer
static void PWM_Stop(void){
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}




//externas
//Inicializacion de los hilos Producer y Consumer
int Init_ThPwm (void) {
 
  tid_pwmQueue = osMessageQueueNew(6, sizeof(bipsMsg_t), NULL);
	if(tid_pwmQueue == NULL){return -1;}
	
  tid_ThPwmConsumer = osThreadNew(ThPwmConsumer, NULL, NULL);
  if(tid_ThPwmConsumer == NULL){return(-1);}
 
  //----------------creacion del hilo producer, que es de test------------
  // tid_ThPwmProducer = osThreadNew(ThPwmProducer, NULL, NULL);
  //if (tid_ThPwmProducer == NULL) {
    //return(-1);
  //}
	//----------------------------------------------------------------------
  return(0);
}
 
//declaracion hilo Consumer----> este nos interesa para el principal
void ThPwmConsumer (void *argument){
  
	(void)argument;
  bipsMsg_t mensaje;
	
	MX_TIM1_Init();
	PWM_Stop();
  
  while (1) {
    
    if(osMessageQueueGet(tid_pwmQueue, &mensaje, NULL, osWaitForever) == osOK){
    
      for(uint8_t i = 0; i < mensaje.cantidad_bips; i++){
          PWM_Start();
          osDelay(mensaje.duracion_ms);
        
          PWM_Stop();
          osDelay(150);
      }
    }
  }
}

/*
//declaracion hilo Producer
void ThPwmProducer (void *argument) {
  
  bipsMsg_t mensaje;
 
  while (1) {
    
    mensaje.duracion_ms = 100; //un bip de 100ms
    mensaje.cantidad_bips = 2; //dos bips
      
    osMessageQueuePut(tid_pwmQueue, &mensaje, 0, 0);
    osDelay(5000); //5 segundos entre cada ciclo
  }
}
*/


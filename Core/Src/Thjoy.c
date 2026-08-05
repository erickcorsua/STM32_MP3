//intro 
//titulo: Thjoy.c
//autor: erick cs
//descripcion: driver de un joystick, que utiliza un timer virtual periodico para distinguir pulsaciones cortas y largas, ademas
//             de quitar rebotes.

/*      
   __     ______     __  __     ______     ______   __     ______     __  __    
  /\ \   /\  __ \   /\ \_\ \   /\  ___\   /\__  _\ /\ \   /\  ___\   /\ \/ /    
 _\_\ \  \ \ \/\ \  \ \____ \  \ \___  \  \/_/\ \/ \ \ \  \ \ \____  \ \  _"-.  
/\_____\  \ \_____\  \/\_____\  \/\_____\    \ \_\  \ \_\  \ \_____\  \ \_\ \_\ 
\/_____/   \/_____/   \/_____/   \/_____/     \/_/   \/_/   \/_____/   \/_/\/_/ 
                                                                                
*/
//cabeceras

#include "Thjoy.h"
#include "leds_stm_v2_1.h"

#define TMP_ANTI_RBTES 50  
#define TMP_PULS_LARGA 1000

#define TMP_FLAG_LONG ((Thjoy_Mask_t)0x20U)

// --- Variables privadas ---
static osThreadId_t thjoy_thread_id = NULL;                   // hilo del joystick, se encarga de manejar el timer periodico
static osTimerId_t tim__joystick_id;                          // timer periodico
static GPIO_InitTypeDef GPIO_InitStruct;                      // estructura para inicializar el GPIO

// bandera que indica que ya hay una ISR pendiente de ser procesada (debounce en curso)
// volatile porque se accede desde contextos de interrupción y de hilo/callback
static volatile bool thjoy_isr_pending = false;

static volatile uint16_t count_ms;                            // contador ms desde que se inició el muestreo
static volatile bool rbtes_eliminados;                        // true si ya pasó anti-rebotes y hay una pulsación estable
static volatile Thjoy_Mask_t press_mask;                      // máscara de botones confirmada tras debounce
static volatile Thjoy_Mask_t mask;

// --- Variables publicas ---
bool gb_joystick_init = false;  // bool que protege ante segundas, terceras ... incializaciones
osMessageQueueId_t thjoy_queue_id = NULL;              // cola que produce el timer one-shoot, y que consume el hilo principal

// --- Forward declarations ---
static void Thjoy_Thread(void *argument);                     //contiene el programa del hilo
static void Thjoy_GPIO_Init(void);                            //contiene lo necesario para los GPIOS involucrados
static Thjoy_Mask_t Thjoy_ReadAllPins(void);                  //lectura de pines
static void Timer_periodico_Callback(void const *arg);        //Callback del timer periodico

void Thjoy_ISR_Callback(void);
void EXTI15_10_IRQHandler(void);



//===================HILO DE TEST======================
static osThreadId_t tid_Thread_consumidor_test;                      

static void Thread_consumidor_test(void *argument);                  
int Init_Thread_consumidor_test(void);

//=====================================================


// === Internas/private ===
/*static void Thjoy_GPIO_Init(void) 

  inicia los gpios, en estado pulldown y detecta los flancos de subida
  una vez queden inicializados pone a true un booleano de inicio

*/
//--------------------------------------------
static void Thjoy_GPIO_Init(void) {
	
		//habilita relojes
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

		//detectamos el nivel alto
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;

    GPIO_InitStruct.Pin = THJOY_PIN_UP | THJOY_PIN_RIGHT;
    HAL_GPIO_Init(THJOY_GPIO_PORT_UP_RIGHT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = THJOY_PIN_DOWN | THJOY_PIN_LEFT | THJOY_PIN_CENTER;
    HAL_GPIO_Init(THJOY_GPIO_PORT_DOWN_LEFT_CENTER, &GPIO_InitStruct);
	
    // inicializamos estado
    count_ms = 0;
    rbtes_eliminados = false;
    press_mask = 0;

}
/*static void Thjoy_GPIO_Init(void) 

 lectura atomica de pines y construccion de máscara en una variable local

*/
//--------------------------------------------
static Thjoy_Mask_t Thjoy_ReadAllPins(void){
    Thjoy_Mask_t mask = 0;
    if(HAL_GPIO_ReadPin(THJOY_GPIO_PORT_UP_RIGHT, THJOY_PIN_UP) == GPIO_PIN_SET)
        mask |= THJOY_MASK_UP;
    if(HAL_GPIO_ReadPin(THJOY_GPIO_PORT_UP_RIGHT, THJOY_PIN_RIGHT) == GPIO_PIN_SET)
        mask |= THJOY_MASK_RIGHT;
    if(HAL_GPIO_ReadPin(THJOY_GPIO_PORT_DOWN_LEFT_CENTER, THJOY_PIN_DOWN) == GPIO_PIN_SET)
        mask |= THJOY_MASK_DOWN;
    if(HAL_GPIO_ReadPin(THJOY_GPIO_PORT_DOWN_LEFT_CENTER, THJOY_PIN_LEFT) == GPIO_PIN_SET)
        mask |= THJOY_MASK_LEFT;
    if(HAL_GPIO_ReadPin(THJOY_GPIO_PORT_DOWN_LEFT_CENTER, THJOY_PIN_CENTER) == GPIO_PIN_SET)
        mask |= THJOY_MASK_CENTER;
    return mask;
}
/*static void Timer_periodico_Callback(void const *arg){

  funcion callback que ejecuta el timer periodico (1ms), evaluamos los valores
  de los pines, elimina rebotes y distingue entre una pulsacion corta y una larga
  
*/
//--------------------------------------------
static void Timer_periodico_Callback(void const *arg){
  // add user code here
  (void)arg;
  
	if(rbtes_eliminados == false){
		
		mask = Thjoy_ReadAllPins(); //leemos el estado de los pines, y lo cargamos en la mascara
		
		if(mask == 0){ //no se ha pulsado nada, es solo ruido
		
			//para el timer y restaura su estado
			osTimerStop(tim__joystick_id);
			thjoy_isr_pending = false;
			HAL_NVIC_EnableIRQ(THJOY_NVIC_LINE);
			//reset
			count_ms = 0;
			rbtes_eliminados = false;
			press_mask = 0;
			return;
		}
		else{
			rbtes_eliminados = true;
			press_mask = mask;
			count_ms = 50;
		  // Arranca/reinicia timer para filtrar rebotes (50 ms)
      osTimerStop(tim__joystick_id);
      osTimerStart(tim__joystick_id, 475U); //primer lanzamiento para saber si es pulsacion valida
			return;
		}
		
	}else{//rebotes eliminados, situacion de pulsacion valida
		
		// incrementamos con protección contra overflow
    if (count_ms <= (uint16_t)(0xFFFFU - 475U)) {
      count_ms = (uint16_t)(count_ms + 475U);
    } else {
      count_ms = 0xFFFFU;
    }
		
		mask = Thjoy_ReadAllPins();

		if(mask != press_mask){//se ha soltado, porque hay un cambio
		
			if(count_ms >= TMP_PULS_LARGA){//hay pulsacion larga
				
				Thjoy_Mask_t event_mask = press_mask | TMP_FLAG_LONG;
				osMessageQueuePut(thjoy_queue_id, &event_mask, 0, 0);
				
			}
			else{//hay pulsacion corta
				
				Thjoy_Mask_t event_mask = press_mask;
				osMessageQueuePut(thjoy_queue_id, &event_mask, 0, 0);
				
			}
			
			//para el timer y restaura su estado
			osTimerStop(tim__joystick_id);
			thjoy_isr_pending = false;
			HAL_NVIC_EnableIRQ(THJOY_NVIC_LINE);
			//reset
			count_ms = 0;
			rbtes_eliminados = false;
			press_mask = 0;
			
		}
	}
}
//--------------------------------------------
/*static void Thjoy_Thread(void *argument){

  Funcion que contiene las tareas del hilo, básicamente controla
  el timer virtual. 
  Espera a que le llegue una flag, que proviene de una interrupción, del joystick

*/
//--------------------------------------------
static void Thjoy_Thread(void *argument){
	
    (void)argument;
    while(1) {
        // Espera una pulsación sucia (“ISR”)
        osThreadFlagsWait(THJOY_FLAG_ISR, osFlagsWaitAny, osWaitForever);

        
        // Reiniciamos estado y arrancamos el timer periódico 1ms para contar/debounce
        // Protegemos variables antes de arrancar
        count_ms = 0;
        rbtes_eliminados = false;
        press_mask = 0;
		
        // Arranca/reinicia timer para filtrar rebotes (50 ms)
        osTimerStop(tim__joystick_id);
        osTimerStart(tim__joystick_id, 50U); //primer lanzamiento para saber si es pulsacion valida
    }
}

// === API pública ===
/*void Thjoy_Init(void){

  Inicializa todo, los gpios, la cola, el timer, el hilo
*/
//--------------------------------------------

void Thjoy_Init(void){
	if(!gb_joystick_init){
    // Inicialización de GPIOs / interrupciones joystick
    Thjoy_GPIO_Init();

    // Cola de eventos limpios
	  // ahora la cola transporta Thjoy_Mask_t (1 byte con 5 bits útiles)
    thjoy_queue_id = osMessageQueueNew(THJOY_QUEUE_SIZE, sizeof(Thjoy_Mask_t), NULL);

    // Timer virtual periodico;
    tim__joystick_id = osTimerNew((osTimerFunc_t)&Timer_periodico_Callback, osTimerPeriodic, NULL, NULL);  

    // Hilo para gestionar flags y timer debounce
    thjoy_thread_id = osThreadNew(Thjoy_Thread, NULL, NULL);
	
	  //Hilo de test (descomentar para usar)
//    Init_Thread_consumidor_test();	
	
	  //Una vez todo este construido habilitamos interrupciones
	  thjoy_isr_pending = false;
	  HAL_NVIC_EnableIRQ(THJOY_NVIC_LINE);
		
		gb_joystick_init = true;
	}
}

//--------------------------------------------
/*void Thjoy_ISR_Callback(void){

  cuando se produzca una interrupción, esta función de callback será
  llamada desde el fichero stm32f4xx_it.c

*/
//--------------------------------------------

void Thjoy_ISR_Callback(void){
	
	if(!thjoy_isr_pending){
		
		thjoy_isr_pending= true;			
		
    // Primer rebote/flanco detectado: deshabilitar interrupciones
    HAL_NVIC_DisableIRQ(THJOY_NVIC_LINE);
    // Sólo notificamos si el hilo ya existe (protección ante inicialización tardía)
    if (thjoy_thread_id != NULL){
      osThreadFlagsSet(thjoy_thread_id, THJOY_FLAG_ISR);
    }
		// Si thjoy_isr_pending == true, no hacemos nada: ya hay un debounce en curso.
	}
}


//--------------------------------------------
/*void EXTI15_10_IRQHandler(void){

  Sobreescribe la función week, del IRQHandler, para poder manejar las ISR

*/
//--------------------------------------------
void EXTI15_10_IRQHandler(void){
	
    HAL_GPIO_EXTI_IRQHandler(THJOY_PIN_UP);
    HAL_GPIO_EXTI_IRQHandler(THJOY_PIN_DOWN);
    HAL_GPIO_EXTI_IRQHandler(THJOY_PIN_RIGHT);
    HAL_GPIO_EXTI_IRQHandler(THJOY_PIN_LEFT);
    HAL_GPIO_EXTI_IRQHandler(THJOY_PIN_CENTER);
	
}


//--------------------------------------------



//===================HILO DE TEST======================
int Init_Thread_consumidor_test(void) {
 
  
  tid_Thread_consumidor_test = osThreadNew(Thread_consumidor_test, NULL, NULL);
  if (tid_Thread_consumidor_test == NULL) {
    return(-1);
  }
 
  return(0);
}
//--------------------------------------------- 
void Thread_consumidor_test(void *argument){
	
	(void)argument;
 
	leds_stm_init();
  Thjoy_Mask_t mask_recived;

  while (1) {
        if (osMessageQueueGet(thjoy_queue_id, &mask_recived, NULL, osWaitForever) == osOK) {

            switch (mask_recived) {
							//------------nada--------------
                case 0x00: // NONE
                
                break;
              //------------center--------------
                case THJOY_MASK_CENTER:
                    
                   leds_stm_encender_blue();
								
                break;
						  //------------up--------------
                case THJOY_MASK_UP:
									
								   leds_stm_encender_green();
									
                break;
							//------------right--------------
                case THJOY_MASK_RIGHT:
									
					         leds_stm_encender_red();
                    
                break;
              //------------largas--------------
                case (THJOY_MASK_UP_LONG): 
									
                  leds_stm_apagar_green();
								
                break;
							  case (THJOY_MASK_CENTER_LONG): 
									
                  leds_stm_apagar_blue();
								
                break;
                case (THJOY_MASK_RIGHT_LONG): 
									
                  leds_stm_apagar_red();
								
                break;
              
                default:
                    
                break;
            }

        osThreadYield();                            // suspend thread
        }
  }
}
//=====================================================

/*

añade esto en el stm32f4xx_it.c

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	
	(void)GPIO_Pin;
  Thjoy_ISR_Callback();
	    
}

*/

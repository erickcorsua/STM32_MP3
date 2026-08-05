//----------------------------HILO PRINCIPAL----------------------------

/*
   __   _ __                _          _           __
  / /  (_) /__    ___  ____(_)__  ____(_)__  ___ _/ /
 / _ \/ / / _ \  / _ \/ __/ / _ \/ __/ / _ \/ _ `/ / 
/_//_/_/_/\___/ / .__/_/ /_/_//_/\__/_/ .__/\_,_/_/  
               /_/
							 
*/

#include "principal.h"

//VARIABLES

static osThreadId_t thprincipal_id;

//static osMessageQueueId_t JoystickMask_id;
static MODOS_t modoActual;
static LCD_Message_t msg_enviar;
static bipsMsg_t bip_msg_common;

static uint8_t ya_has_llamado_al_rojo = 0; //super util para el rgb modo no hay sd

//=================================variables de eventos=============================
static float temperatura = 0.0f;
static Thjoy_Mask_t mask_recived;        /* recibe eventos joystick (ahora en file-scope) */
static MP3_Event_t  evento_recibido_mp3; /* info del MP3 (reservado)                      */
//===============================================================================

//=================================variables de LCD=============================
static char line1[32];
static char line2[32];
//===============================================================================
//=================================COM=============================
static char msg[64];
static uint8_t trama[8] = {0x7E, 0xFF, 0x06, 0x0C, 0x00, 0x00, 0x00, 0xEF};
static int len;
//============================variables auxiliares horas ===========================

static uint8_t horas_aux = 0;
static uint8_t minutos_aux = 0;
static uint8_t segundos_aux = 0;

static uint8_t modo_configuracion_hora = 0;
static uint8_t prm_vz_programcion_hora = 1;
//===============================================================================
//============ Variables que representarán los datos del modo REPRODUCCIÓN===========
static uint8_t carpeta = 01;         // Número actual de carpeta
static uint8_t cancion = 01;         // Número actual de canción

// Número total de archivos en cada carpeta (definido manualmente)
static const uint8_t archivos_por_carpeta[] = {2, 3, 2}; // Ej: folder01: 2 archivos, folder02: 3 archivos...


static uint8_t volumen = 68;         // Volumen actual
static uint8_t ultimo_volumen = 0;//variable que compara si el volumen ha cambiado muy util para no enviar un comando de set volumen en mano

static uint8_t tiempo_minutos = 0;   // Minutos de reproducción (actualizado en tiempo real) 
static uint8_t tiempo_segundos = 0; // Segundos de reproducción (actualizado en tiempo real)

static uint8_t esta_play_o_no = 0;   // Nos dice si esta play o no 0 no 1 si
static uint8_t prmr_play_dsps_cmb_d_md = 0;

static osTimerId_t tim_tiempo_de_reproduccion;
static void TimerCallback(void *argument) {
    (void)argument;

    /* Incrementar tiempo de reproducción */
    tiempo_segundos++;
    if (tiempo_segundos > 59) {
        tiempo_segundos = 0;
        tiempo_minutos++;
    }
}
//===============================================================================
/* DECLARACION DE FUNCIONES */
void Init_Principal(void);
void Principal_Thread(void *argument);

/* DEFINICION DE FUNCIONES */
void Init_Principal(void){
  const osThreadAttr_t principal_attr = {
		.name = "principal",
    .stack_size = 2048U,
  };
	tim_tiempo_de_reproduccion = osTimerNew((osTimerFunc_t)&TimerCallback, osTimerPeriodic, NULL, NULL);  
  thprincipal_id = osThreadNew(Principal_Thread, NULL, &principal_attr);
}

void Principal_Thread(void *argument){
  (void)argument;

  //Inicializacion de todos los hilos periféricos
  Thjoy_Init(); 
  Init_Thread_mp3(); 
	LCD_Init();
	Clock_Init();
	Temp_Init();
  Init_ThPwm();
	Init_ThreadRGB();
	Init_ThPot_2();
	COM_PC_Init();
  //--------------------------------------------
  /* estado inicial */
  modoActual = REPOSO_MODE;         //nuestro estado inicial, despues de un reset
  mp3_send_cmd(SLEEP_MODE, 0x00, 0x00);  // Enviar comando para dormir al mp3
	  len = snprintf(msg, sizeof(msg),
    "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
    horas, minutos, segundos,
    SLEEP_MODE, 0x00, 0x00);
		
	COM_PC_QueueMessage((uint8_t *)msg, len);
		
  for (;;){
    osStatus_t st;

    /* Si la SD está ausente forzamos REPOSO y dejamos de intentar entrar en reproducción */
    if (estado_sd == MP3_SD_REMOVED || estado_sd == MP3_SD_UNKNOWN){ //variable global que se actualiza con el estado de la sd
			if(ya_has_llamado_al_rojo == 0){
				ya_has_llamado_al_rojo = 1;
				rgb_no_hay_sd();			
				bip_msg_common.cantidad_bips = 10;
			  bip_msg_common.duracion_ms   = 250;
				osMessageQueuePut(tid_pwmQueue, &bip_msg_common, 0U, 0U);
			}
      if (modoActual != REPOSO_MODE){
        modoActual = REPOSO_MODE; //forzamos ir a modo reposos      			  
				mp3_send_cmd(SLEEP_MODE, 0x00, 0x00);  // Enviar comando para dormir al mp3
				len = snprintf(msg, sizeof(msg),
        "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
        horas, minutos, segundos,
        SLEEP_MODE, 0x00, 0x00);
		
	      COM_PC_QueueMessage((uint8_t *)msg, len);
				esta_play_o_no = 0;			
				//activar los hilos
				osThreadResume(tid_thread_temperatura);        // Activar hilo de temperatura
				//suspender los hilos				
				osTimerStop(tim_tiempo_de_reproduccion); // Pausar contador de la musica
				osThreadSuspend(tid_ThPot_2); // Suspender hilo de pot
				
        
      }
      /* seguimos leyendo joystick, pero no permitimos entrar en REPRODUCCION */
    }else{
			 if(ya_has_llamado_al_rojo == 1){rgb_apagarlo();}
			 ya_has_llamado_al_rojo = 0;
		}

    /* Espera ligera por evento joystick (el hilo duerme si no hay eventos) */
    st = osMessageQueueGet(thjoy_queue_id, &mask_recived, NULL, 100U); //timeout 100 ms
    if(st != osOK){mask_recived = 0U;}
		//en caso de recibir un gesto del joystick valido, lo que haremos sera hacer un "bip"
	  if (mask_recived != 0U) {
      bip_msg_common.cantidad_bips = 1;
			bip_msg_common.duracion_ms   = 100;
			
      osMessageQueuePut(tid_pwmQueue, &bip_msg_common, 0U, 0U);
    }
		
      /* Procesar se4gún modo */
      switch (modoActual) {
        //-----------------------------------------------------
        //----------------------modo reposo--------------------
				//-----------------------------------------------------
        case REPOSO_MODE:
					 
					prmr_play_dsps_cmb_d_md = 1;
				  osMessageQueueGet(queue_temp, &temperatura, NULL, 0U);
				
				  //--------------construccion de lo que se ve en el lcd----------------------------
				  // Construir el segundo mensaje: línea 1
          msg_enviar.line = 1; // Línea 1
					snprintf(line1, sizeof(line1), "SBM 2025  T= %.1f C", (double)temperatura);
					msg_enviar.text = line1; // Asignar el texto construido
					osMessageQueuePut(lcdQueue, &msg_enviar, 0U, 0U); // Encolar el mensaje

					// Construir el segundo mensaje: línea 2 (hora)
					msg_enviar.line = 2; // Línea 2
					snprintf(line2, sizeof(line2), "      %02u:%02u:%02u   ", horas, minutos, segundos);
					msg_enviar.text = line2; // Asignar el texto construido
					osMessageQueuePut(lcdQueue, &msg_enviar, 0U, 0U); // Encolar el mensaje
				  
				  //---------------------------------------------------------------------------------
          
          if (mask_recived & THJOY_MASK_CENTER_LONG){
            modoActual = REPRODUCCION_MODE;
						mp3_send_cmd(WAKE_UP, 0x00, 0x00);  // Enviar comando para despertar al mp3
						len = snprintf(msg, sizeof(msg),
            "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
            horas, minutos, segundos,
            WAKE_UP, 0x00, 0x00);
		
	          COM_PC_QueueMessage((uint8_t *)msg, len);
						//Activar hilo del siguiente modulo
					  osThreadResume(tid_ThPot_2); // Suspender hilo de pot
						// Suspender hilos innecesarios
            osThreadSuspend(tid_thread_temperatura); // Suspender hilo de temperatura
						if(estado_sd == MP3_SD_INSERTED){rgb_pausado();}
        
          }
          break;
        //-----------------------------------------------------------
        //----------------------modo reproduccion--------------------
			  //-----------------------------------------------------------
        case REPRODUCCION_MODE:
		
					osMessageQueueGet(mp3_EventQueue, &evento_recibido_mp3, NULL, 0U);
				
				  
				  if(evento_recibido_mp3.type==MP3_EVENT_FINISHED_PLAYING){tiempo_segundos=0;tiempo_minutos=0;}
					
          // Obtener el volumen actual desde la cola del potenciómetro
          osMessageQueueGet(tid_potQueue, &volumen, NULL, 0U);

          // Solo enviar el comando si el valor del volumen ha cambiado
          if (volumen != ultimo_volumen){
            mp3_send_cmd(SET_VOLUME, 0x00, volumen);  // Enviar comando para ajustar volumen
						len = snprintf(msg, sizeof(msg),
            "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
            horas, minutos, segundos,
            SET_VOLUME, 0x00, volumen);
		
	          COM_PC_QueueMessage((uint8_t *)msg, len);
            ultimo_volumen = volumen;                 // Actualizar último volumen enviado
          }
					
					
				  //--------------construccion de lo que se ve en el lcd----------------------------
				  // Construir el segundo mensaje: línea 1 
          msg_enviar.line = 1; // Línea 1
				  snprintf(line1, sizeof(line1), "F:%02d C:%02d   Vol:%02d",carpeta,cancion,volumen);
					msg_enviar.text = line1; // Asignar el texto construido
					osMessageQueuePut(lcdQueue, &msg_enviar, 0U, 0U); // Encolar el mensaje

					// Construir el segundo mensaje: línea 2 
					msg_enviar.line = 2; // Línea 2
				  snprintf(line2, sizeof(line2), "    T: %02d:%02d    ",tiempo_minutos, tiempo_segundos);
					msg_enviar.text = line2; // Asignar el texto construido
					osMessageQueuePut(lcdQueue, &msg_enviar, 0U, 0U); // Encolar el mensaje
					//---------------------------------------------------------------------------------
          
          //gestionamos lso eventos del joystick
          switch (mask_recived) {
            case THJOY_MASK_RIGHT:
              /* siguiente canción */
						  if(cancion < archivos_por_carpeta[carpeta - 1]){
                cancion++; // Incrementar archivo dentro de la carpeta
              }else{
                cancion = 1; // Regresar al primer archivo de la carpeta
              }

              // Enviar comando al MP3 para reproducir la nueva canción
              mp3_send_cmd(PLAY_WITH_FOLDER_AND_FILENAME, carpeta, cancion);
							len = snprintf(msg, sizeof(msg),
              "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
              horas, minutos, segundos,
              PLAY_WITH_FOLDER_AND_FILENAME, carpeta, cancion);
		
	            COM_PC_QueueMessage((uint8_t *)msg, len);
							
						  //COM_PC_QueueMessage(,);
						  rgb_reproduciendo();
						  osTimerStop(tim_tiempo_de_reproduccion); // Pausar contador
						  tiempo_segundos = 0; 
						  tiempo_minutos = 0;
						  
						  osTimerStart(tim_tiempo_de_reproduccion, 1000U); // Reanudar contador
							
              break;

            case THJOY_MASK_LEFT:
              /* canción anterior */
						  if(cancion > 1){
                cancion--; // Incrementar archivo dentro de la carpeta
              }else{
                cancion = archivos_por_carpeta[carpeta - 1];
              }

              // Enviar comando al MP3 para reproducir la nueva canción
              mp3_send_cmd(PLAY_WITH_FOLDER_AND_FILENAME, carpeta, cancion);
						  len = snprintf(msg, sizeof(msg),
              "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
              horas, minutos, segundos,
              PLAY_WITH_FOLDER_AND_FILENAME, carpeta, cancion);
		
	            COM_PC_QueueMessage((uint8_t *)msg, len);
						  rgb_reproduciendo();
						  osTimerStop(tim_tiempo_de_reproduccion); // Pausar contador
						  tiempo_segundos = 0; 
						  tiempo_minutos = 0;
						  
						  osTimerStart(tim_tiempo_de_reproduccion, 1000U); // Reanudar contador
							
              break;

            case THJOY_MASK_UP:
             if(carpeta < sizeof(archivos_por_carpeta) / sizeof(archivos_por_carpeta[0])){
               carpeta++; // Incrementar carpeta
             }else{
               carpeta = 1; // Regresar a la primera carpeta
             }
             cancion = 1; // Reiniciar al primer archivo de la nueva carpeta

             //Enviar comando al MP3 para reproducir desde la nueva carpeta
             mp3_send_cmd(PLAY_WITH_FOLDER_AND_FILENAME, carpeta, cancion);
						 len = snprintf(msg, sizeof(msg),
             "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
             horas, minutos, segundos,
             PLAY_WITH_FOLDER_AND_FILENAME, carpeta, cancion);
		
	           COM_PC_QueueMessage((uint8_t *)msg, len);
						 rgb_reproduciendo();
						 osTimerStop(tim_tiempo_de_reproduccion); // Pausar contador
						   tiempo_segundos = 0; 
						   tiempo_minutos = 0;
						  
						 osTimerStart(tim_tiempo_de_reproduccion, 1000U); // Reanudar contador
						
            break;              

            case THJOY_MASK_DOWN:
              /* bajar carpeta y reproducir primera canción */
						if(carpeta > 1){
               carpeta--; // Incrementar carpeta
             }else{
               carpeta = sizeof(archivos_por_carpeta) / sizeof(archivos_por_carpeta[0]);
             }
             cancion = 1; // Reiniciar al primer archivo de la nueva carpeta

             //Enviar comando al MP3 para reproducir desde la nueva carpeta
             mp3_send_cmd(PLAY_WITH_FOLDER_AND_FILENAME, carpeta, cancion);
						 len = snprintf(msg, sizeof(msg),
             "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
             horas, minutos, segundos,
             PLAY_WITH_FOLDER_AND_FILENAME, carpeta, cancion);
		
	           COM_PC_QueueMessage((uint8_t *)msg, len);
						 rgb_reproduciendo();
						 osTimerStop(tim_tiempo_de_reproduccion); // Pausar contador
						   tiempo_segundos = 0; 
						   tiempo_minutos = 0;
						  
						 osTimerStart(tim_tiempo_de_reproduccion, 1000U); // Reanudar contador
						
           break;

            case THJOY_MASK_CENTER:
              
						if(prmr_play_dsps_cmb_d_md == 1){
						  mp3_send_cmd(PLAY_WITH_FOLDER_AND_FILENAME,0x01,0x01);
							len = snprintf(msg, sizeof(msg),
              "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
              horas, minutos, segundos,
              PLAY_WITH_FOLDER_AND_FILENAME, 0x01, 0x01);
		
	            COM_PC_QueueMessage((uint8_t *)msg, len);
							prmr_play_dsps_cmb_d_md = 0;
							rgb_reproduciendo();
							osTimerStart(tim_tiempo_de_reproduccion, 1000U); // Reanudar contador
							esta_play_o_no = 1;
							
							break;
						}
						
						if(esta_play_o_no == 0){
						  mp3_send_cmd(PLAY,0x00,0x00);
							len = snprintf(msg, sizeof(msg),
              "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
              horas, minutos, segundos,
              PLAY,0x00,0x00);
		
	            COM_PC_QueueMessage((uint8_t *)msg, len);
							osTimerStart(tim_tiempo_de_reproduccion, 1000U); // Reanudar contador
							rgb_reproduciendo();
							esta_play_o_no = 1;
							
						}else{
						  mp3_send_cmd(PAUSE,0x00,0x00);
							len = snprintf(msg, sizeof(msg),
              "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
              horas, minutos, segundos,
              PAUSE,0x00,0x00);
		
	            COM_PC_QueueMessage((uint8_t *)msg, len);
							osTimerStop(tim_tiempo_de_reproduccion); // Pausar contador
							rgb_pausado();
							esta_play_o_no = 0;
						}
						
              break;

            case THJOY_MASK_CENTER_LONG:
              /* entrar en programación de hora */
              modoActual = PROGRAMACION_HORA_MODE;
						  mp3_send_cmd(PAUSE,0x00,0x00);
		   				len = snprintf(msg, sizeof(msg),
              "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
              horas, minutos, segundos,
              PAUSE,0x00,0x00);
		
	            COM_PC_QueueMessage((uint8_t *)msg, len);
		
						  mp3_send_cmd(SLEEP_MODE, 0x00, 0x00);  // Enviar comando para dormir al mp3
							
							len = snprintf(msg, sizeof(msg),
              "%02d:%02d:%02d ---> 0x7E 0xFF 0x06 %02X 0x00 %02X %02X 0xEF\r\n",
              horas, minutos, segundos,
              SLEEP_MODE, 0x00, 0x00);
		
	            COM_PC_QueueMessage((uint8_t *)msg, len);
							
						  rgb_apagarlo();
						  rgb_mk_dark();
						  modo_configuracion_hora = 0;
						  prm_vz_programcion_hora == 1;
           
						  //activar hilo de temperatura
				      osThreadResume(tid_thread_temperatura);        // Activar hilo de temperatura		   
						
						  //suspender los hilos
						  
              osThreadSuspend(tid_ThPot_2); // Suspender hilo de pot
						  osTimerStop(tim_tiempo_de_reproduccion); // Pausar contador de la musica
              break;

            default:
              break;
          }
          

          break;
        //-----------------------------------------------------------
        //----------------------modo programacion de hora------------
			  //-----------------------------------------------------------
        case PROGRAMACION_HORA_MODE:
				if(prm_vz_programcion_hora == 1){
				  horas_aux = horas;
				  minutos_aux = minutos;
          segundos_aux = segundos;	
          prm_vz_programcion_hora = 0;					
				}
         osMessageQueueGet(queue_temp, &temperatura, NULL, 0U);
				
				  //--------------construccion de lo que se ve en el lcd----------------------------
				  // Construir el segundo mensaje: línea 1 
          msg_enviar.line = 1; // Línea 1
          snprintf(line1, sizeof(line1), "HORA  T= %.1f C", (double)temperatura);
					msg_enviar.text = line1; // Asignar el texto construido
					osMessageQueuePut(lcdQueue, &msg_enviar, 0U, 0U); // Encolar el mensaje

					// Construir el segundo mensaje: línea 2 
					msg_enviar.line = 2; // Línea 2
          snprintf(line2, sizeof(line2), "      %02u:%02u:%02u   ", horas_aux, minutos_aux, segundos_aux);				  
					msg_enviar.text = line2; // Asignar el texto construido
					osMessageQueuePut(lcdQueue, &msg_enviar, 0U, 0U); // Encolar el mensaje
					//---------------------------------------------------------------------------------
  					
				//gestionamos lso eventos del joystick
          switch (mask_recived) {
            case THJOY_MASK_RIGHT:
            	
						  if(modo_configuracion_hora == 0){								
								modo_configuracion_hora++;
							}
						  else if(modo_configuracion_hora == 1){
							  modo_configuracion_hora++;
							}
							else{
							  modo_configuracion_hora = 0;
							}
              break;

            case THJOY_MASK_LEFT:
							if(modo_configuracion_hora == 2){								
								modo_configuracion_hora--;
							}
						  else if(modo_configuracion_hora == 1){
							  modo_configuracion_hora--;
							}
							else{
							  modo_configuracion_hora = 2;
							}
            	
              break;

            case THJOY_MASK_UP:
							
							if(modo_configuracion_hora == 0){																
								if(horas_aux == 23){
								  horas_aux = 0;
								}else{
								  horas_aux++;
								}								
							}
						  if(modo_configuracion_hora == 1){
								
								if(minutos_aux == 59){									
									minutos_aux = 0;								
								}else{
								  minutos_aux++;
								}
							  
							}
							if(modo_configuracion_hora == 2){
							  if(segundos_aux==59){
								  segundos_aux = 0;
								}else{
								  segundos_aux++;
								}
							}
            
            break;              

            case THJOY_MASK_DOWN:
							
						  if(modo_configuracion_hora == 0){																
								if(horas_aux == 0){
								  horas_aux = 23;
								}else{
								  horas_aux--;
								}								
							}
						  if(modo_configuracion_hora == 1){
								
								if(minutos_aux == 0){									
									minutos = 59;								
								}else{
								  minutos--;
								}
							  
							}
							if(modo_configuracion_hora == 2){
							  if(segundos_aux==0){
								  segundos_aux = 59;
								}else{
								  segundos_aux--;
								}
							}
            
           break;

            case THJOY_MASK_CENTER:
              
						  horas = horas_aux;
						  minutos = minutos_aux;
						  segundos = segundos_aux; 
						
             break;

            case THJOY_MASK_CENTER_LONG:
							modoActual = REPOSO_MODE;
						  
               						
            break;
          }
          
          break;
      } // switch modo
    
  } // for
} /* HILO */


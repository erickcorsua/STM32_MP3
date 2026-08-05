//intro 
//titulo: mp3.c
//autor: erick cs
//descripcion: fichero que usa el CMSIS para programar el usart6 (pg14 y pg9) para 
//             comunicarse con el mp3, tambien recibe el feedback del mp3
/*
 _____ ______   ________  ________     
|\   _ \  _   \|\   __  \|\_____  \    
\ \  \\\__\ \  \ \  \|\  \|____|\ /_   
 \ \  \\|__| \  \ \   ____\    \|\  \  
  \ \  \    \ \  \ \  \___|   __\_\  \ 
   \ \__\    \ \__\ \__\     |\_______\
    \|__|     \|__|\|__|     \|_______|
                                       
*/
#include "mp3.h" 

/* -------------------------- Variables-------------------------- */
extern ARM_DRIVER_USART Driver_USART6;
static ARM_DRIVER_USART *USARTdrv = &Driver_USART6;

static osThreadId_t tid_Thread_mp3_tx;      // hilo TX (envío)
static osThreadId_t tid_Thread_mp3_rx;      // hilo RX (recepción)

osMessageQueueId_t mp3_MsgQueue;             // primera cola, para enviar comandos
osMessageQueueId_t mp3_EventQueue;           // segunda cola, para el feedback que nos envia el mp3

MSGQUEUE_MP3_TX_t tx_msg;          // mensaje que enviamos al mp3
MSGQUEUE_MP3_RX_t rx_msg;          // mensaje que recibimos del mp3

volatile MP3_SD_State_t estado_sd;       // estado de la sd, muy util para saber si esta isertada o no
volatile MP3_SD_State_t last_sd_state;   //situada aqui para depurar
volatile MP3_Event_t ev;                 // tipo de dato ya parseado

//static volatile uint8_t rx_byte;

/* ---------- Prototipos (declaraciones) de funciones ---------- */
static void My_USART_Callback(uint32_t event);
static inline void mp3_parse_reply(const uint8_t *buf, MP3_Event_t *ev);

static void Thread_mp3_tx(void *argument);     // TX thread
static void Thread_mp3_rx(void *argument);     // RX thread

int Init_Thread_mp3(void);
void mp3_send_cmd(uint8_t cmd_macro, uint8_t param_msb, uint8_t param_lsb);


/* -------------------------- Implementación -------------------------- */
/**
 * Callback del driver USART. Señala a los hilos cuando termina TX y RX usando flags.
 */
static void My_USART_Callback(uint32_t event) {
    if (event & ARM_USART_EVENT_SEND_COMPLETE) {
        // Señala al hilo TX que terminó la transmisión        
            osThreadFlagsSet(tid_Thread_mp3_tx, MP3_FLAG_TX_DONE);        
    }
    if (event & ARM_USART_EVENT_RECEIVE_COMPLETE) {
        // Señala al hilo RX que terminó la recepción
            osThreadFlagsSet(tid_Thread_mp3_rx, MP3_FLAG_RX_DONE);        
    }
}

/**
 * funcion que nos carga en una estrucuta informacion ya parseada de la trama recibia del mp3
 */
static inline void mp3_parse_reply(const uint8_t *buf, MP3_Event_t *ev){
    if (!ev || !buf) return;
    ev->type = MP3_EVENT_UNKNOWN;
    ev->code = buf[3];
    ev->sub  = buf[5];
    ev->value= buf[6];

    switch (ev->code) {
        case 0x41: ev->type = MP3_EVENT_DATA_OK; break;
        case 0x3A: ev->type = MP3_EVENT_TF_INSERTED; break;
        case 0x3B: ev->type = MP3_EVENT_TF_REMOVED; break;
        case 0x40: ev->type = MP3_EVENT_FILE_NOT_FOUND; break;
        case 0x3D: ev->type = MP3_EVENT_FINISHED_PLAYING; break;
        case 0x42:
            if (ev->sub == 0x00) ev->type = MP3_EVENT_PLAYER_STOPPED;
            else if (ev->sub == 0x01) ev->type = MP3_EVENT_PLAYER_PLAY;
            else if (ev->sub == 0x02) ev->type = MP3_EVENT_PLAYER_PAUSED;
            break;
        case 0x48: ev->type = MP3_EVENT_FILES_FOUND; break;
        case 0x4C: ev->type = MP3_EVENT_CURRENTLY_PLAYING; break;
        case 0x4F: ev->type = MP3_EVENT_FOLDERS_FOUND; break;
        case 0x43: ev->type = MP3_EVENT_VOLUME_LEVEL; break;
        default: break;
    }
}
/**
 * Hilo TX: gestiona solo la transmisión de comandos al MP3.
 * Espera mensajes en la cola mp3_MsgQueue, los envía por USART y espera TX_DONE.
 */
static void Thread_mp3_tx(void *argument){
  (void)argument;
	uint32_t flags;
  while (1){
        // Espera comando
        if (osMessageQueueGet(mp3_MsgQueue, &tx_msg, NULL, osWaitForever) == osOK) {

            // Transmisión no bloqueante con flags
            osThreadFlagsClear(MP3_FLAG_TX_DONE);
            USARTdrv->Send(tx_msg.comando, sizeof(tx_msg.comando));
            flags = osThreadFlagsWait(MP3_FLAG_TX_DONE, osFlagsWaitAny, 200);

            // identificamos que flag llego
            if ((flags & MP3_FLAG_TX_DONE) != MP3_FLAG_TX_DONE) { 
                // Timeout TX: depuración, error, etc.              
                continue;							
						}   
        }
  }
}

/**
 * Hilo RX: queda en escucha continua. Cada vez que recibe una trama la encola en mp3_Rx_MsgQueue.
 */
static void Thread_mp3_rx(void *argument)
{
    (void)argument;

    char     byte;          // Byte temporal para recepción
    uint8_t  i = 0;         // Índice para acumulación
    uint8_t  state_mp3 = 0; // Estado recepción (0 = inicio, 1 = acumulando)
    uint32_t flags;         // Flags de recepción

    while (1) {

        /* Recepción byte a byte */
        osThreadFlagsClear(MP3_FLAG_RX_DONE);
        USARTdrv->Receive(&byte, 1);
        flags = osThreadFlagsWait(MP3_FLAG_RX_DONE,
                                  osFlagsWaitAny,
                                  osWaitForever);

        if (flags == MP3_FLAG_RX_DONE) {

            /* ---- ESTADO 0: buscar inicio de trama ---- */
            if (state_mp3 == 0) {

                if (byte == 0x7E) {
                    i = 0;
                    state_mp3 = 1;
                    rx_msg.comando[i++] = byte;
                }
            }

            /* ---- ESTADO 1: acumular trama ---- */
            else if (state_mp3 == 1) {

                if (i < sizeof(rx_msg.comando)) {
                    rx_msg.comando[i++] = byte;
                }
                else {
                    /* Overflow de trama → reset FSM */
                    state_mp3 = 0;
                    i = 0;
                    continue;
                }

                if (byte == 0xEF) {

                    /* Procesar trama completa */
                    mp3_parse_reply(rx_msg.comando, &ev);

                    /* Manejo de eventos SD */
                    if (ev.type == MP3_EVENT_TF_INSERTED ||
                        ev.type == MP3_EVENT_TF_REMOVED) {

                        MP3_SD_State_t new_sd_state =
                            (ev.type == MP3_EVENT_TF_INSERTED) ?
                            MP3_SD_INSERTED : MP3_SD_REMOVED;

                        if (new_sd_state != last_sd_state) {
                            last_sd_state = new_sd_state;
                            estado_sd     = new_sd_state;
                        }
                    }
                    else {
                        osMessageQueuePut(mp3_EventQueue, &ev, 0, 0);
                    }

                    /* Reset FSM tras trama completa */
                    state_mp3 = 0;
                    i = 0;
                }
            }
        }
    }
}


//creacion del hilo
int Init_Thread_mp3(void){
  // Inicialización de USART6 a 9600 bps (hacerlo antes de crear hilos)
  USARTdrv->Initialize(My_USART_Callback);  // aprovechamos el driver CMSIS
  USARTdrv->PowerControl(ARM_POWER_FULL);
  USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                    ARM_USART_DATA_BITS_8 |
                    ARM_USART_PARITY_NONE |
                    ARM_USART_STOP_BITS_1 |
                    ARM_USART_FLOW_CONTROL_NONE, 9600);//segun el datasheet es lo que usa el mp3
	
  USARTdrv->Control(ARM_USART_CONTROL_TX, 1);
  USARTdrv->Control(ARM_USART_CONTROL_RX, 1);

  // creacion de las dos colas
  mp3_MsgQueue    = osMessageQueueNew(MP3_MSG_QUEUE_DEPTH, sizeof(MSGQUEUE_MP3_TX_t), NULL);
  mp3_EventQueue = osMessageQueueNew(MP3_EVENT_QUEUE_DEPTH, sizeof(MP3_Event_t), NULL);

	/* atributos para hilos (stack reducido) */
  const osThreadAttr_t tx_attr = {
		.name = "mp3_tx",
    .stack_size = MP3_TX_STACK_SIZE,
  };
  const osThreadAttr_t rx_attr = {
		.name = "mp3_rx",
		.priority = osPriorityHigh,
    .stack_size = MP3_RX_STACK_SIZE,
  };
	
  // creacion del hilo TX
  tid_Thread_mp3_tx = osThreadNew(Thread_mp3_tx, NULL, &tx_attr);
  if (tid_Thread_mp3_tx == NULL) {
    return (-1);
  }

  // creacion del hilo RX
  tid_Thread_mp3_rx = osThreadNew(Thread_mp3_rx, NULL, &rx_attr);
  if (tid_Thread_mp3_rx == NULL) {
    return (-1);
  }

  return 0;
}

// Construye una trama de 8 bytes para el MP3, y la encola para que sea enviada
void mp3_send_cmd(uint8_t cmd_macro, uint8_t param_msb, uint8_t param_lsb){
    MSGQUEUE_MP3_TX_t mensaje = {0};
    mensaje.comando[0] = 0x7E;
    mensaje.comando[1] = 0xFF;
    mensaje.comando[2] = 0x06;
    mensaje.comando[3] = cmd_macro;
    mensaje.comando[4] = 0x00; // feedback activo
    mensaje.comando[5] = param_msb;
    mensaje.comando[6] = param_lsb;
    mensaje.comando[7] = 0xEF;
    osMessageQueuePut(mp3_MsgQueue, &mensaje, 0, 0);
}



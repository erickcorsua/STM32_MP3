#include "stm32f4xx_hal.h"
#include "pot_2.h"
#include "cmsis_os2.h"
#include <stdint.h>

#define VREF 3.3f
#define LEVELS 30  // 0..29

/*
                                  /'
                              --/'--
            ____     ____     /'    
          /'    )--/'    )--/'      
        /'    /' /'    /' /'        
      /(___,/'  (___,/'  (__        
    /'                              
  /'                                
/'                                  
*/

float VOLUMEN;

osThreadId_t tid_ThPot_2;        // Thread productor
//osThreadId_t tid_ThConsumer;     // Thread consumidor
osMessageQueueId_t tid_potQueue; // Cola de niveles (0..29)

// ---------------------- ADC GPIO Config ----------------------
void ADC1_pins_F429ZI_config(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PC0 -> ADC1_IN10, PC3 -> ADC1_IN13 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

// ---------------------- ADC Init ----------------------
int ADC_Init_Single_Conversion(ADC_HandleTypeDef *hadc, ADC_TypeDef *ADC_Instance) {
    hadc->Instance = ADC_Instance;
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.ScanConvMode = DISABLE;
    hadc->Init.ContinuousConvMode = DISABLE;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.NbrOfConversion = 1;
    hadc->Init.DMAContinuousRequests = DISABLE;
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if (HAL_ADC_Init(hadc) != HAL_OK) {
        return -1;
    }

    return 0;
}

// ---------------------- ADC 5-bit y mapeo a 0..29 ----------------------
uint8_t ADC_getLevel30(ADC_HandleTypeDef *hadc, uint32_t Channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    HAL_StatusTypeDef status;
    uint32_t raw12;

    sConfig.Channel = Channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
        return 0;

    HAL_ADC_Start(hadc);

    do {
        status = HAL_ADC_PollForConversion(hadc, 0);
    } while (status != HAL_OK);

    raw12 = HAL_ADC_GetValue(hadc);

    // Mapear 12 bits (0..4095) a 0..29
    uint32_t level = (raw12 * LEVELS + 2048) / 4096; // redondeo
    if (level >= LEVELS) level = LEVELS - 1;

    return (uint8_t)level; // 0..29
}

// ---------------------- Thread Init ----------------------
int Init_ThPot_2(void) {
    // Cola de niveles 0..29
    tid_potQueue = osMessageQueueNew(8, sizeof(uint8_t), NULL);

    // Threads
    tid_ThPot_2  = osThreadNew(ThPot_2, NULL, NULL);
    //tid_ThConsumer = osThreadNew(ThConsumer, NULL, NULL);
/*
    if (tid_ThPot_2 == NULL || tid_ThConsumer == NULL) {
        return -1;
    }
*/
    return 0;
}

// ---------------------- Thread Productor ----------------------
void ThPot_2(void *argument) {
    ADC_HandleTypeDef hadc;
    uint8_t lastLevel = 0xFF;

    ADC1_pins_F429ZI_config();
    ADC_Init_Single_Conversion(&hadc, ADC1);

    while (1) {
        uint8_t level = ADC_getLevel30(&hadc, 10); // canal 10

        if (level != lastLevel) {
            osMessageQueuePut(tid_potQueue, &level, 0, 0);
            lastLevel = level;
        }

        osDelay(300);
    }
}
/*
// ---------------------- Thread Consumidor ----------------------
void ThConsumer(void *argument) {
    uint8_t level;

    while (1) {
        if (osMessageQueueGet(tid_potQueue, &level, NULL, osWaitForever) == osOK) {
            VOLUMEN = (float)level / (LEVELS - 1); // Normalizado 0.0 – 1.0
        }
    }
}


*/
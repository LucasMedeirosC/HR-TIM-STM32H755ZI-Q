/*
 * LI_Measures.c
 *
 *  Created on: May 5, 2026
 *      Author: lucas
 */

#include "LI_Measures.h"

#include <stdint.h>
#include "adc.h"
#include "dma.h"
#include "stm32h7xx_hal_adc.h"

static volatile uint16_t g_adc_buffer[1] = {0U};
static volatile uint16_t g_last_adc_sample = 0U;
static li_measures_status_t g_measures_status =
    {
        .last_error = STATUS_NOT_READY,
        .error_count = 0U,
        .is_ready = 0U,
        .is_running = 0U,
};

/**
 * Callback de conversão completa do ADC, chamada pela HAL.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)LI_measures_on_adc_conv_cplt_isr(hadc);
}
//----------------------------------------------------------//

static void LI_measures_set_error(status_t error)
{
    __disable_irq();
    g_measures_status.last_error = error;
    g_measures_status.error_count++;
    __enable_irq();
}

status_t LI_start_adc_dma(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK)
    {
        LI_measures_set_error(STATUS_HW_ERROR);
        return STATUS_HW_ERROR;
    }

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)g_adc_buffer, 1U) != HAL_OK)
    {
        LI_measures_set_error(STATUS_HW_ERROR);
        return STATUS_HW_ERROR;
    }

    __disable_irq();
    g_measures_status.last_error = STATUS_OK;
    g_measures_status.is_ready = 1U;
    g_measures_status.is_running = 1U;
    __enable_irq();

    return STATUS_OK;
}

status_t LI_measures_on_adc_conv_cplt_isr(ADC_HandleTypeDef *hadc)
{
    if ((hadc == NULL) || (hadc->Instance != ADC1))
    {
        LI_measures_set_error(STATUS_INVALID_ARG);
        return STATUS_INVALID_ARG;
    }

    g_last_adc_sample = g_adc_buffer[0];

    /*
        Colocar aqui a parte de CALL_Back para APP fazer os caculos e atualizar os Timers, ou seja, a parte de controle. 
        O ideal é que o controle seja feito na APP e não aqui, para manter essa camada de medidas o mais simples possível, 
        apenas lendo o ADC e expondo o valor para a APP.
    */

    __disable_irq();
    g_measures_status.last_error = STATUS_OK;
    __enable_irq();

    return STATUS_OK;
}

status_t LI_measures_get_status(li_measures_status_t *out_status)
{
    if (out_status == NULL)
    {
        return STATUS_INVALID_ARG;
    }

    __disable_irq();
    *out_status = g_measures_status;
    __enable_irq();

    return STATUS_OK;
}

status_t LI_measures_clear_errors(void)
{
    __disable_irq();
    g_measures_status.last_error = STATUS_OK;
    g_measures_status.error_count = 0U;
    __enable_irq();

    return STATUS_OK;
}

status_t LI_measures_get_latest_sample(uint16_t *out_sample)
{
    if (out_sample == NULL)
    {
        return STATUS_INVALID_ARG;
    }

    if (g_measures_status.is_ready == 0U)
    {
        LI_measures_set_error(STATUS_NOT_READY);
        return STATUS_NOT_READY;
    }

    __disable_irq();
    *out_sample = g_last_adc_sample;
    __enable_irq();

    return STATUS_OK;
}
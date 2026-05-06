/*
 * LI_Measures.h
 *
 *  Created on: May 5, 2026
 *      Author: lucas
 */

#ifndef INC_LI_MEASURES_H_
#define INC_LI_MEASURES_H_

#include <stdint.h>

#include "stm32h7xx_hal.h"

/**
 * @brief Status codes for the measurement module.
 */
typedef enum
{
    LI_MEASURES_STATUS_OK = 0,
    LI_MEASURES_STATUS_INVALID_ARG,
    LI_MEASURES_STATUS_NOT_READY,
    LI_MEASURES_STATUS_BUSY,
    LI_MEASURES_STATUS_TIMEOUT,
    LI_MEASURES_STATUS_HW_ERROR,
    LI_MEASURES_STATUS_OVERFLOW,
    LI_MEASURES_STATUS_COMM_ERROR,
    LI_MEASURES_STATUS_FAULT_LATCHED
} status_measures_t;

/**
 * @brief Snapshot of the measurement module state.
 */
typedef struct
{
    status_measures_t last_error;   /**< Last error reported by the module. */
    uint32_t error_count;  /**< Number of errors reported since startup. */
    uint8_t is_ready;      /**< Non-zero when the module has been initialized. */
    uint8_t is_running;    /**< Non-zero when DMA acquisition is active. */
} li_measures_status_t;

/**
 * @brief Initialize the ADC+DMA measurement module.
 * @return STATUS_OK on success, STATUS_HW_ERROR if calibration or DMA start fails.
 */
status_measures_t LI_start_adc_dma(void);

/**
 * @brief ISR hook for ADC conversion complete.
 *
 * Intended to be called from HAL_ADC_ConvCpltCallback(). Keep it short because it runs in ISR context.
 *
 * @param[in] hadc Pointer to HAL ADC handle.
 * @return STATUS_OK on success, STATUS_INVALID_ARG if the callback is not for ADC1 or hadc is NULL.
 */
status_measures_t LI_measures_on_adc_conv_cplt_isr(ADC_HandleTypeDef *hadc);

/**
 * @brief Get the measurement module status snapshot.
 * @param[out] out_status Destination buffer.
 * @return STATUS_OK on success, STATUS_INVALID_ARG if out_status is NULL.
 */
status_measures_t LI_measures_get_status(li_measures_status_t *out_status);

/**
 * @brief Clear the measurement module error state.
 * @return STATUS_OK on success.
 */
status_measures_t LI_measures_clear_errors(void);

/**
 * @brief Read the last ADC sample captured by DMA.
 * @param[out] out_sample Destination buffer.
 * @return STATUS_OK on success, STATUS_INVALID_ARG if out_sample is NULL, STATUS_NOT_READY if no sample has been captured yet.
 */
status_measures_t LI_measures_get_latest_sample(uint16_t *out_sample);

#endif /* INC_LI_MEASURES_H_ */

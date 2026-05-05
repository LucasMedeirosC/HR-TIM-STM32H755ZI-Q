/*
 * LI_Actuators.h
 *
 *  Created on: May 5, 2026
 *      Author: lucas
 */

#ifndef INC_LI_ACTUATORS_H_
#define INC_LI_ACTUATORS_H_

#include "stdint.h"

#include "LI_Measures.h"

typedef enum
{
    TIMER_A = 'A',
    TIMER_B = 'B',
    TIMER_C = 'C',
    TIMER_D = 'D',
    TIMER_E = 'E'
} timer_identifier_t;

typedef struct
{
    status_t last_error;
    uint32_t error_count;
    uint8_t is_ready;
    uint8_t is_running;
} li_actuators_status_t;

/**
 * @brief Initialize the HRTIM actuator module.
 * @return STATUS_OK on success, STATUS_HW_ERROR if HAL startup fails.
 */
status_t LI_initialize_timers(void);

/**
 * @brief Start one HRTIM timer and its outputs.
 * @param[in] timer Timer identifier.
 * @return STATUS_OK on success, STATUS_INVALID_ARG for an invalid timer, STATUS_NOT_READY if module is not initialized, or STATUS_HW_ERROR on HAL failure.
 */
status_t LI_start_timer(timer_identifier_t timer);

/**
 * @brief Stop one HRTIM timer and its outputs.
 * @param[in] timer Timer identifier.
 * @return STATUS_OK on success, STATUS_INVALID_ARG for an invalid timer, STATUS_NOT_READY if module is not initialized, or STATUS_HW_ERROR on HAL failure.
 */
status_t LI_stop_timer(timer_identifier_t timer);

/**
 * @brief Update the duty cycle for one HRTIM timer.
 * @param[in] timer Timer identifier.
 * @param[in] duty_pct Duty cycle in percent (0..100).
 * @return STATUS_OK on success, STATUS_INVALID_ARG for invalid arguments, STATUS_NOT_READY if module is not initialized, or STATUS_HW_ERROR on HAL failure.
 */
status_t LI_hrtim_update_duty_channel(timer_identifier_t timer, uint32_t duty_pct);

/**
 * @brief Get the actuator module status snapshot.
 * @param[out] out_status Destination buffer.
 * @return STATUS_OK on success, STATUS_INVALID_ARG if out_status is NULL.
 */
status_t LI_actuators_get_status(li_actuators_status_t *out_status);

/**
 * @brief Clear the actuator module error state.
 * @return STATUS_OK on success.
 */
status_t LI_actuators_clear_errors(void);

#endif /* INC_LI_ACTUATORS_H_ */

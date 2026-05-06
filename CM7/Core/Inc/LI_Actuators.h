/*
 * LI_Actuators.h
 *
 *  Created on: May 5, 2026
 *      Author: lucas
 */

#ifndef INC_LI_ACTUATORS_H_
#define INC_LI_ACTUATORS_H_

#include "main.h"
#include "stm32h7xx_hal_hrtim.h"

#include <stdint.h>

/**
 * @brief Number of PWM channels used by the system.
 */
#define PWM_CHANNELS 5U

/**
 * @brief Status codes for the actuator module.
 */
typedef enum
{
    LI_ACTUATORS_STATUS_OK = 0,
    LI_ACTUATORS_STATUS_INVALID_ARG,
    LI_ACTUATORS_STATUS_NOT_READY,
    LI_ACTUATORS_STATUS_BUSY,
    LI_ACTUATORS_STATUS_TIMEOUT,
    LI_ACTUATORS_STATUS_HW_ERROR,
    LI_ACTUATORS_STATUS_OVERFLOW,
    LI_ACTUATORS_STATUS_COMM_ERROR,
    LI_ACTUATORS_STATUS_FAULT_LATCHED
} status_actuators_t;

/**
 * @brief Logical PWM channels used by the control layer.
 */
typedef enum
{
    BOOST_L1 = 0, /**< PWM channel associated with boost L1. */
    BOOST_L2,     /**< PWM channel associated with boost L2. */
    BOOST_L3,     /**< PWM channel associated with boost L3. */
    BUCK_L1,      /**< PWM channel associated with buck L1. */
    BUCK_L2,      /**< PWM channel associated with buck L2. */
    TIMER_A = BOOST_L1,
    TIMER_B = BOOST_L2,
    TIMER_C = BOOST_L3,
    TIMER_D = BUCK_L1,
    TIMER_E = BUCK_L2,
} pwm_channel_t;

/**
 * @brief PWM parameters for one channel.
 */
typedef struct
{
    float duty_cycle;             /**< Current duty cycle in percent. */
    uint32_t hrtim_timer_id;      /**< HRTIM timer identifier. */
    uint32_t hrtim_timer_channel; /**< HRTIM output/channel identifier. */
    uint32_t compare_value;       /**< Current compare register value. */
    uint32_t compare_value_2;     /**< Secondary compare register value. */
} pwm_parameters_t;

/**
 * @brief Snapshot of the actuator module state.
 */
typedef struct
{
    status_actuators_t last_error;
    uint32_t error_count;
    uint8_t is_ready;
    uint8_t is_running;
} li_actuators_status_t;

/**
 * @brief Initialize the HRTIM actuator module.
 * @return STATUS_OK on success, STATUS_HW_ERROR if HAL startup fails.
 */
status_actuators_t LI_initialize_timers(void);

/**
 * @brief Start one HRTIM channel and its outputs.
 * @param[in] channel PWM channel identifier.
 * @return STATUS_OK on success, STATUS_INVALID_ARG for an invalid channel, STATUS_NOT_READY if the module is not initialized, or STATUS_HW_ERROR on HAL failure.
 */
status_actuators_t LI_start_timer(pwm_channel_t channel);

/**
 * @brief Stop one HRTIM channel and its outputs.
 * @param[in] channel PWM channel identifier.
 * @return STATUS_OK on success, STATUS_INVALID_ARG for an invalid channel, STATUS_NOT_READY if the module is not initialized, or STATUS_HW_ERROR on HAL failure.
 */
status_actuators_t LI_stop_timer(pwm_channel_t channel);

/**
 * @brief Update the duty cycle for one HRTIM channel.
 * @param[in] channel PWM channel identifier.
 * @param[in] duty_pct Duty cycle in percent, from 0.0f to 100.0f.
 * @return STATUS_OK on success, STATUS_INVALID_ARG for invalid arguments, STATUS_NOT_READY if the module is not initialized, or STATUS_HW_ERROR on HAL failure.
 */
status_actuators_t LI_hrtim_update_duty_channel(pwm_channel_t channel, float duty_pct);

/**
 * @brief Update all PWM channels from the internal parameter array.
 *
 * Walks the internal `g_pwm_data` array and updates the HRTIM compare
 * registers for every configured channel in a single call.
 *
 * @return STATUS_OK on success, STATUS_NOT_READY if the module is not initialized.
 */
status_actuators_t LI_pwm_control(void);

/**
 * @brief Get the PWM parameters snapshot for all channels.
 * @param[out] out_data Destination buffer with PWM_CHANNELS entries.
 * @return STATUS_OK on success, STATUS_INVALID_ARG if out_data is NULL.
 */
status_actuators_t LI_actuators_get_status(pwm_parameters_t *out_data);

/**
 * @brief Clear the actuator module error state.
 * @return STATUS_OK on success.
 */
status_actuators_t LI_actuators_clear_errors(void);

/**
 * @brief Read the PWM parameters array used by the module.
 * @return Pointer to the internal PWM parameter array.
 */
pwm_parameters_t *LI_actuators_data_read(void);

#endif /* INC_LI_ACTUATORS_H_ */

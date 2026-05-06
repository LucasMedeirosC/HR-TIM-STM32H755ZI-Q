/*
 * LI_Actuators.c
 *
 *  Created on: May 5, 2026
 *      Author: lucas
 */

#include "LI_Actuators.h"

#include <stdint.h>

#include "dma.h"
#include "hrtim.h"
#include "stm32h7xx_hal_hrtim.h"

#define PERIOD_TICKS 8333U

static li_actuators_status_t g_actuators_status =
    {
    .last_error = LI_ACTUATORS_STATUS_NOT_READY,
        .error_count = 0U,
        .is_ready = 0U,
        .is_running = 0U,
};

static pwm_parameters_t g_pwm_data[PWM_CHANNELS] =
    {
        {0.0f, HRTIM_TIMERID_TIMER_A, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2, 0U, 0U},
        {0.0f, HRTIM_TIMERID_TIMER_B, HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2, 0U, 0U},
        {0.0f, HRTIM_TIMERID_TIMER_C, HRTIM_OUTPUT_TC1 | HRTIM_OUTPUT_TC2, 0U, 0U},
        {0.0f, HRTIM_TIMERID_TIMER_D, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2, 0U, 0U},
        {0.0f, HRTIM_TIMERID_TIMER_E, HRTIM_OUTPUT_TE1 | HRTIM_OUTPUT_TE2, 0U, 0U},
};

static void LI_actuators_set_error(status_actuators_t error)
{
    __disable_irq();
    g_actuators_status.last_error = error;
    g_actuators_status.error_count++;
    __enable_irq();
}

static status_actuators_t LI_actuators_get_timer_config(pwm_channel_t channel, uint32_t *timer_id, uint32_t *timer_index, uint32_t *output_mask)
{
    if ((timer_id == NULL) || (timer_index == NULL) || (output_mask == NULL))
    {
        return LI_ACTUATORS_STATUS_INVALID_ARG;
    }

    switch (channel)
    {
    case TIMER_A:
        *timer_id = HRTIM_TIMERID_TIMER_A;
        *timer_index = HRTIM_TIMERINDEX_TIMER_A;
        *output_mask = HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2;
        return LI_ACTUATORS_STATUS_OK;
    case TIMER_B:
        *timer_id = HRTIM_TIMERID_TIMER_B;
        *timer_index = HRTIM_TIMERINDEX_TIMER_B;
        *output_mask = HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2;
        return LI_ACTUATORS_STATUS_OK;
    case TIMER_C:
        *timer_id = HRTIM_TIMERID_TIMER_C;
        *timer_index = HRTIM_TIMERINDEX_TIMER_C;
        *output_mask = HRTIM_OUTPUT_TC1 | HRTIM_OUTPUT_TC2;
        return LI_ACTUATORS_STATUS_OK;
    case TIMER_D:
        *timer_id = HRTIM_TIMERID_TIMER_D;
        *timer_index = HRTIM_TIMERINDEX_TIMER_D;
        *output_mask = HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2;
        return LI_ACTUATORS_STATUS_OK;
    case TIMER_E:
        *timer_id = HRTIM_TIMERID_TIMER_E;
        *timer_index = HRTIM_TIMERINDEX_TIMER_E;
        *output_mask = HRTIM_OUTPUT_TE1 | HRTIM_OUTPUT_TE2;
        return LI_ACTUATORS_STATUS_OK;
    default:
        return LI_ACTUATORS_STATUS_INVALID_ARG;
    }
}

static status_actuators_t LI_actuators_update_pwm_snapshot(pwm_channel_t channel, float duty_cycle, uint32_t compare_value, uint32_t compare_value_2)
{
    if (channel >= PWM_CHANNELS)
    {
        return LI_ACTUATORS_STATUS_INVALID_ARG;
    }

    g_pwm_data[channel].duty_cycle = duty_cycle;
    g_pwm_data[channel].compare_value = compare_value;
    g_pwm_data[channel].compare_value_2 = compare_value_2;

    return LI_ACTUATORS_STATUS_OK;
}

status_actuators_t LI_initialize_timers(void)
{
    if (HAL_HRTIM_WaveformCountStart(&hhrtim, HRTIM_TIMERID_MASTER) != HAL_OK)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_HW_ERROR);
        return LI_ACTUATORS_STATUS_HW_ERROR;
    }

    if (HAL_HRTIM_WaveformOutputStart(&hhrtim, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2) != HAL_OK)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_HW_ERROR);
        return LI_ACTUATORS_STATUS_HW_ERROR;
    }

    __disable_irq();
    g_actuators_status.last_error = LI_ACTUATORS_STATUS_OK;
    g_actuators_status.is_ready = 1U;
    g_actuators_status.is_running = 1U;
    __enable_irq();

    for (uint32_t i = 0U; i < PWM_CHANNELS; i++)
    {
        g_pwm_data[i].duty_cycle = 0.0f;
        g_pwm_data[i].compare_value = 0U;
        g_pwm_data[i].compare_value_2 = 0U;
    }

    return LI_ACTUATORS_STATUS_OK;
}

status_actuators_t LI_start_timer(pwm_channel_t channel)
{
    uint32_t timer_id = 0U;
    uint32_t timer_index = 0U;
    uint32_t output_mask = 0U;

    if (g_actuators_status.is_ready == 0U)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_NOT_READY);
        return LI_ACTUATORS_STATUS_NOT_READY;
    }

    if (LI_actuators_get_timer_config(channel, &timer_id, &timer_index, &output_mask) != LI_ACTUATORS_STATUS_OK)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_INVALID_ARG);
        return LI_ACTUATORS_STATUS_INVALID_ARG;
    }

    if (HAL_HRTIM_WaveformCountStart(&hhrtim, timer_id) != HAL_OK)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_HW_ERROR);
        return LI_ACTUATORS_STATUS_HW_ERROR;
    }

    if (HAL_HRTIM_WaveformOutputStart(&hhrtim, output_mask) != HAL_OK)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_HW_ERROR);
        return LI_ACTUATORS_STATUS_HW_ERROR;
    }

    __disable_irq();
    g_actuators_status.last_error = LI_ACTUATORS_STATUS_OK;
    g_actuators_status.is_running = 1U;
    __enable_irq();

    return LI_ACTUATORS_STATUS_OK;
}

status_actuators_t LI_stop_timer(pwm_channel_t channel)
{
    uint32_t timer_id = 0U;
    uint32_t timer_index = 0U;
    uint32_t output_mask = 0U;

    if (g_actuators_status.is_ready == 0U)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_NOT_READY);
        return LI_ACTUATORS_STATUS_NOT_READY;
    }

    if (LI_actuators_get_timer_config(channel, &timer_id, &timer_index, &output_mask) != LI_ACTUATORS_STATUS_OK)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_INVALID_ARG);
        return LI_ACTUATORS_STATUS_INVALID_ARG;
    }

    if (HAL_HRTIM_WaveformCountStop(&hhrtim, timer_id) != HAL_OK)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_HW_ERROR);
        return LI_ACTUATORS_STATUS_HW_ERROR;
    }

    if (HAL_HRTIM_WaveformOutputStop(&hhrtim, output_mask) != HAL_OK)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_HW_ERROR);
        return LI_ACTUATORS_STATUS_HW_ERROR;
    }

    __disable_irq();
    g_actuators_status.last_error = LI_ACTUATORS_STATUS_OK;
    g_actuators_status.is_running = 0U;
    __enable_irq();

    return LI_ACTUATORS_STATUS_OK;
}

status_actuators_t LI_hrtim_update_duty_channel(pwm_channel_t channel, float duty_pct)
{
    uint32_t timer_id = 0U;
    uint32_t timer_index = 0U;
    uint32_t output_mask = 0U;

    if (g_actuators_status.is_ready == 0U)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_NOT_READY);
        return LI_ACTUATORS_STATUS_NOT_READY;
    }

    if ((duty_pct < 0.0f) || (duty_pct > 100.0f))
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_INVALID_ARG);
        return LI_ACTUATORS_STATUS_INVALID_ARG;
    }

    if (LI_actuators_get_timer_config(channel, &timer_id, &timer_index, &output_mask) != LI_ACTUATORS_STATUS_OK)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_INVALID_ARG);
        return LI_ACTUATORS_STATUS_INVALID_ARG;
    }

    /* Use float math for duty calculations, then convert to integer ticks. */
    float inactive_pct_f = 100.0f - duty_pct;
    float value_f = ((float)PERIOD_TICKS * inactive_pct_f) / 100.0f;
    uint32_t value = (uint32_t)(value_f + 0.5f);
    uint32_t comp1 = (uint32_t)(((float)PERIOD_TICKS - (float)value) / 2.0f + 0.5f);
    uint32_t comp2 = comp1 + value;

    // A atualização dos Registradores deve ser feita pela função pwm_control() para evitar problemas de concorrência, então aqui apenas atualizamos o snapshot dos parâmetros PWM.
    // hhrtim.Instance->sTimerxRegs[timer_index].CMP1xR = comp1;
    // hhrtim.Instance->sTimerxRegs[timer_index].CMP2xR = comp2;

    (void)LI_actuators_update_pwm_snapshot(channel, duty_pct, comp1, comp2);

    __disable_irq();
    g_actuators_status.last_error = LI_ACTUATORS_STATUS_OK;
    __enable_irq();

    (void)output_mask;
    return LI_ACTUATORS_STATUS_OK;
}

status_actuators_t LI_pwm_control(void)
{
    if (g_actuators_status.is_ready == 0U)
    {
        LI_actuators_set_error(LI_ACTUATORS_STATUS_NOT_READY);
        return LI_ACTUATORS_STATUS_NOT_READY;
    }

    for (uint32_t ch = 0U; ch < PWM_CHANNELS; ch++)
    {
        pwm_channel_t channel = (pwm_channel_t)ch;
        uint32_t timer_id = 0U;
        uint32_t timer_index = 0U;
        uint32_t output_mask = 0U;

        if (LI_actuators_get_timer_config(channel, &timer_id, &timer_index, &output_mask) != LI_ACTUATORS_STATUS_OK)
        {
            /* skip invalid channel entries */
            continue;
        }

        /* Use precomputed compare values from the parameter snapshot. */
        uint32_t comp1 = g_pwm_data[ch].compare_value;
        uint32_t comp2 = g_pwm_data[ch].compare_value_2;

        hhrtim.Instance->sTimerxRegs[timer_index].CMP1xR = comp1;
        hhrtim.Instance->sTimerxRegs[timer_index].CMP2xR = comp2;
    }

    __disable_irq();
    g_actuators_status.last_error = LI_ACTUATORS_STATUS_OK;
    __enable_irq();

    return LI_ACTUATORS_STATUS_OK;
}

status_actuators_t LI_actuators_get_status(pwm_parameters_t *out_data)
{
    if (out_data == NULL)
    {
        return LI_ACTUATORS_STATUS_INVALID_ARG;
    }

    __disable_irq();
    for (uint32_t i = 0U; i < PWM_CHANNELS; i++)
    {
        out_data[i] = g_pwm_data[i];
    }
    __enable_irq();

    return LI_ACTUATORS_STATUS_OK;
}

status_actuators_t LI_actuators_clear_errors(void)
{
    __disable_irq();
    g_actuators_status.last_error = LI_ACTUATORS_STATUS_OK;
    g_actuators_status.error_count = 0U;
    __enable_irq();

    return LI_ACTUATORS_STATUS_OK;
}

pwm_parameters_t *LI_actuators_data_read(void)
{
    return g_pwm_data;
}
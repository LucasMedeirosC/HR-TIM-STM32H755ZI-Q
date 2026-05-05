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

static li_actuators_status_t g_actuators_status =
    {
        .last_error = STATUS_NOT_READY,
        .error_count = 0U,
        .is_ready = 0U,
        .is_running = 0U,
};

static const uint32_t g_hrtim_period_ticks = 8333U;

static void li_actuators_set_error(status_t error)
{
    __disable_irq();
    g_actuators_status.last_error = error;
    g_actuators_status.error_count++;
    __enable_irq();
}

static status_t LI_actuators_get_timer_config(timer_identifier_t timer, uint32_t *timer_id, uint32_t *timer_index, uint32_t *output_mask)
{
    if ((timer_id == NULL) || (timer_index == NULL) || (output_mask == NULL))
    {
        return STATUS_INVALID_ARG;
    }

    switch (timer)
    {
    case TIMER_A:
        *timer_id = HRTIM_TIMERID_TIMER_A;
        *timer_index = HRTIM_TIMERINDEX_TIMER_A;
        *output_mask = HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2;
        return STATUS_OK;
    case TIMER_B:
        *timer_id = HRTIM_TIMERID_TIMER_B;
        *timer_index = HRTIM_TIMERINDEX_TIMER_B;
        *output_mask = HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2;
        return STATUS_OK;
    case TIMER_C:
        *timer_id = HRTIM_TIMERID_TIMER_C;
        *timer_index = HRTIM_TIMERINDEX_TIMER_C;
        *output_mask = HRTIM_OUTPUT_TC1 | HRTIM_OUTPUT_TC2;
        return STATUS_OK;
    case TIMER_D:
        *timer_id = HRTIM_TIMERID_TIMER_D;
        *timer_index = HRTIM_TIMERINDEX_TIMER_D;
        *output_mask = HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2;
        return STATUS_OK;
    case TIMER_E:
        *timer_id = HRTIM_TIMERID_TIMER_E;
        *timer_index = HRTIM_TIMERINDEX_TIMER_E;
        *output_mask = HRTIM_OUTPUT_TE1 | HRTIM_OUTPUT_TE2;
        return STATUS_OK;
    default:
        return STATUS_INVALID_ARG;
    }
}

status_t LI_initialize_timers(void)
{
    if (HAL_HRTIM_WaveformCountStart(&hhrtim, HRTIM_TIMERID_MASTER) != HAL_OK)
    {
        li_actuators_set_error(STATUS_HW_ERROR);
        return STATUS_HW_ERROR;
    }

    if (HAL_HRTIM_WaveformOutputStart(&hhrtim, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2) != HAL_OK)
    {
        li_actuators_set_error(STATUS_HW_ERROR);
        return STATUS_HW_ERROR;
    }

    __disable_irq();
    g_actuators_status.last_error = STATUS_OK;
    g_actuators_status.is_ready = 1U;
    g_actuators_status.is_running = 1U;
    __enable_irq();

    return STATUS_OK;
}

status_t LI_start_timer(timer_identifier_t timer)
{
    uint32_t timer_id = 0U;
    uint32_t timer_index = 0U;
    uint32_t output_mask = 0U;

    if (g_actuators_status.is_ready == 0U)
    {
        li_actuators_set_error(STATUS_NOT_READY);
        return STATUS_NOT_READY;
    }

    if (LI_actuators_get_timer_config(timer, &timer_id, &timer_index, &output_mask) != STATUS_OK)
    {
        li_actuators_set_error(STATUS_INVALID_ARG);
        return STATUS_INVALID_ARG;
    }

    if (HAL_HRTIM_WaveformCountStart(&hhrtim, timer_id) != HAL_OK)
    {
        li_actuators_set_error(STATUS_HW_ERROR);
        return STATUS_HW_ERROR;
    }

    if (HAL_HRTIM_WaveformOutputStart(&hhrtim, output_mask) != HAL_OK)
    {
        li_actuators_set_error(STATUS_HW_ERROR);
        return STATUS_HW_ERROR;
    }

    __disable_irq();
    g_actuators_status.last_error = STATUS_OK;
    g_actuators_status.is_running = 1U;
    __enable_irq();

    return STATUS_OK;
}

status_t LI_stop_timer(timer_identifier_t timer)
{
    uint32_t timer_id = 0U;
    uint32_t timer_index = 0U;
    uint32_t output_mask = 0U;

    if (g_actuators_status.is_ready == 0U)
    {
        li_actuators_set_error(STATUS_NOT_READY);
        return STATUS_NOT_READY;
    }

    if (LI_actuators_get_timer_config(timer, &timer_id, &timer_index, &output_mask) != STATUS_OK)
    {
        li_actuators_set_error(STATUS_INVALID_ARG);
        return STATUS_INVALID_ARG;
    }

    if (HAL_HRTIM_WaveformCountStop(&hhrtim, timer_id) != HAL_OK)
    {
        li_actuators_set_error(STATUS_HW_ERROR);
        return STATUS_HW_ERROR;
    }

    if (HAL_HRTIM_WaveformOutputStop(&hhrtim, output_mask) != HAL_OK)
    {
        li_actuators_set_error(STATUS_HW_ERROR);
        return STATUS_HW_ERROR;
    }

    __disable_irq();
    g_actuators_status.last_error = STATUS_OK;
    g_actuators_status.is_running = 0U;
    __enable_irq();

    return STATUS_OK;
}

status_t LI_hrtim_update_duty_channel(timer_identifier_t timer, uint32_t duty_pct)
{
    uint32_t timer_id = 0U;
    uint32_t timer_index = 0U;
    uint32_t output_mask = 0U;

    if (g_actuators_status.is_ready == 0U)
    {
        li_actuators_set_error(STATUS_NOT_READY);
        return STATUS_NOT_READY;
    }

    if (duty_pct > 100U)
    {
        li_actuators_set_error(STATUS_INVALID_ARG);
        return STATUS_INVALID_ARG;
    }

    if (LI_actuators_get_timer_config(timer, &timer_id, &timer_index, &output_mask) != STATUS_OK)
    {
        li_actuators_set_error(STATUS_INVALID_ARG);
        return STATUS_INVALID_ARG;
    }

    uint32_t inactive_pct = 100U - duty_pct;
    uint32_t value = (g_hrtim_period_ticks * inactive_pct) / 100U;
    uint32_t comp1 = (g_hrtim_period_ticks - value) / 2U;
    uint32_t comp2 = comp1 + value;

    hhrtim.Instance->sTimerxRegs[timer_index].CMP1xR = comp1;
    hhrtim.Instance->sTimerxRegs[timer_index].CMP2xR = comp2;

    __disable_irq();
    g_actuators_status.last_error = STATUS_OK;
    __enable_irq();

    (void)output_mask;
    return STATUS_OK;
}

status_t LI_actuators_get_status(li_actuators_status_t *out_status)
{
    if (out_status == NULL)
    {
        return STATUS_INVALID_ARG;
    }

    __disable_irq();
    *out_status = g_actuators_status;
    __enable_irq();

    return STATUS_OK;
}

status_t LI_actuators_clear_errors(void)
{
    __disable_irq();
    g_actuators_status.last_error = STATUS_OK;
    g_actuators_status.error_count = 0U;
    __enable_irq();

    return STATUS_OK;
}
/**
 * @file Soc_Task.c
 * @brief FreeRTOS Task implementation for 5ms SOC estimation
 * @module Soc_Task
 *
 * @assumptions & constraints
 *   - FreeRTOS 10.x or later for task notification
 *   - Timer provides 5ms periodic interrupts
 *   - ADC available for current/voltage measurement
 *
 * @safety considerations
 *   - Task blocked state allows other tasks to run
 *   - Watchdog should be fed in task loop
 *   - Error handling for sensor read failures
 *
 * @execution context
 *   - Dedicated task for SOC estimation
 *   - Blocked on task notification (efficient)
 *
 * @version 1.0.0
 * @compliance MISRA C:2012, ISO 26262
 */

#include "Soc_Task.h"
#include "Soc_Cfg.h"
#include "Soc_Algorithm.h"
#include "Soc_Timer_S32K4.h"
#include <string.h>

/*============================================================================*/
/* Compiler Abstraction for FreeRTOS                                          */
/*============================================================================*/

/**
 * @brief Task notification wait forever
 */
#define SOC_TASK_WAIT_FOREVER      (portMAX_DELAY)

/**
 * @brief Task notification clear on entry
 */
#define SOC_TASK_NOTIFY_CLEAR      (pdTRUE)

/*============================================================================*/
/* Task State Variables                                                      */
/*============================================================================*/

/**
 * @brief Task handle
 *
 * @note Used for task control and notification
 */
void* g_SocTaskHandle = NULL;

/**
 * @brief Task notification event group/semaphore
 *
 * @note Using direct-to-task notification for efficiency
 */
static uint32_t s_TaskNotification = 0U;

/**
 * @brief Task running state
 */
static bool s_TaskRunning = false;

/**
 * @brief Task suspended state
 */
static bool s_TaskSuspended = false;

/**
 * @brief Runtime statistics
 */
static uint32_t s_TaskRunTimeUs = 0U;
static uint32_t s_TaskCycleCount = 0U;

/**
 * @brief Last tick value for timing calculation
 */
static uint32_t s_LastTickValue = 0U;

/**
 * @brief Consecutive error counter
 */
static uint8_t s_ConsecutiveErrors = 0U;

/**
 * @brief Maximum consecutive errors before safe state
 */
static uint8_t s_MaxConsecutiveErrors = SOC_CFG_MAX_CONSECUTIVE_ERRORS;

/*============================================================================*/
/* Local Function Prototypes                                                 */
/*============================================================================*/

/**
 * @brief Process SOC update
 *
 * @description Called from task loop to update SOC.
 *
 * @return SOC_STATUS_OK on success
 */
static Soc_StatusType Soc_Task_ProcessUpdate(void);

/**
 * @brief Handle SOC error
 *
 * @param status Error status code
 */
static void Soc_Task_HandleError(Soc_StatusType status);

/**
 * @brief Update task statistics
 *
 * @param startTick Tick when task started
 */
static void Soc_Task_UpdateStats(uint32_t startTick);

/**
 * @brief Feed watchdog
 *
 * @description Feeds system watchdog if enabled.
 */
static void Soc_Task_FeedWatchdog(void);

/**
 * @brief Enter degraded mode
 *
 * @description Called when multiple errors occur.
 */
static void Soc_Task_EnterDegradedMode(void);

/*============================================================================*/
/* Local Functions                                                           */
/*============================================================================*/

/**
 * @brief Process SOC update
 */
static Soc_StatusType Soc_Task_ProcessUpdate(void)
{
    Soc_StatusType status = SOC_STATUS_OK;
    int32_t current_mA = 0L;
    uint32_t voltage_mV = 0U;
    uint32_t currentTick;
    uint32_t deltaTimeUs;

    /* Get current timer tick */
    currentTick = Soc_Timer_S32K4_GetTick();

    /* Calculate delta time from last update */
    if (s_LastTickValue == 0U)
    {
        /* First update, use configured period */
        deltaTimeUs = SOC_CFG_TIMER_PERIOD_US;
    }
    else if (currentTick >= s_LastTickValue)
    {
        deltaTimeUs = ((currentTick - s_LastTickValue) * 1000000U) /
                      Soc_Timer_S32K4_GetFreqHz();
    }
    else
    {
        /* Handle wrap-around */
        deltaTimeUs = (((0xFFFFFFFFU - s_LastTickValue) + currentTick + 1U) *
                       1000000U) / Soc_Timer_S32K4_GetFreqHz();
    }

    /* Read current measurement */
    status = Soc_Task_ReadCurrent(&current_mA);

    if (status == SOC_STATUS_OK)
    {
        /* Optionally read voltage for OCV fusion */
        (void)Soc_Task_ReadVoltage(&voltage_mV);

        /* Update SOC using algorithm */
        #if (SOC_CFG_COULOMB_EFFICIENCY_ENABLED == 1U)
        /* Use OCV fusion when current is low */
        if ((current_mA < (int32_t)SOC_CFG_MIN_CURRENT_MA) &&
            (current_mA > -(int32_t)SOC_CFG_MIN_CURRENT_MA))
        {
            status = Soc_Algorithm_UpdateWithOcvFusion(
                current_mA,
                voltage_mV,
                deltaTimeUs,
                50U  /* 5% OCV weight */
            );
        }
        else
        {
            status = Soc_Algorithm_Update(current_mA, deltaTimeUs);
        }
        #else
        status = Soc_Algorithm_Update(current_mA, deltaTimeUs);
        #endif

        /* Store last tick */
        s_LastTickValue = currentTick;

        /* Reset error counter on success */
        if (status == SOC_STATUS_OK)
        {
            s_ConsecutiveErrors = 0U;
        }
        else
        {
            Soc_Task_HandleError(status);
        }
    }
    else
    {
        Soc_Task_HandleError(status);
    }

    return status;
}

/**
 * @brief Handle SOC errors
 */
static void Soc_Task_HandleError(Soc_StatusType status)
{
    /* Increment consecutive error counter */
    s_ConsecutiveErrors++;

    /* Check for degraded mode transition */
    if (s_ConsecutiveErrors >= s_MaxConsecutiveErrors)
    {
        Soc_Task_EnterDegradedMode();
    }

    /* Log error (in production, use proper logging) */
    /* Note: Avoid logging in ISR context */
}

/**
 * @brief Update task statistics
 */
static void Soc_Task_UpdateStats(uint32_t startTick)
{
    uint32_t currentTick = Soc_Timer_S32K4_GetTick();
    uint32_t execTicks;

    /* Calculate execution time */
    if (currentTick >= startTick)
    {
        execTicks = currentTick - startTick;
    }
    else
    {
        execTicks = (0xFFFFFFFFU - startTick) + currentTick + 1U;
    }

    /* Convert to microseconds */
    s_TaskRunTimeUs = (execTicks * 1000000U) / Soc_Timer_S32K4_GetFreqHz();

    /* Increment cycle count */
    s_TaskCycleCount++;
}

/**
 * @brief Feed watchdog
 */
static void Soc_Task_FeedWatchdog(void)
{
    #if (SOC_CFG_WATCHDOG_FEED_ENABLED == 1U)
    /* In production, call watchdog feed function */
    /* Wdg_Refresh(); */
    #endif
}

/**
 * @brief Enter degraded mode
 */
static void Soc_Task_EnterDegradedMode(void)
{
    /* Update operating mode */
    /* In production: g_SocRuntime.mode = SOC_MODE_DEGRADED; */

    /* Log degradation event */
    s_ConsecutiveErrors = 0U;

    /* Reset algorithm to known state */
    (void)Soc_Algorithm_Reset(SOC_CFG_INIT_SOC_PERMILLE);
}

/*============================================================================*/
/* Current/Voltage Sensor Interface                                          */
/*============================================================================*/

/**
 * @brief Initialize current sensor
 *
 * @description Placeholder for ADC initialization.
 *              In production, initialize ADC for current measurement.
 */
Soc_StatusType Soc_Task_InitCurrentSensor(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /*
     * In production, initialize ADC for current measurement:
     *
     * 1. Configure ADC channel for current sense amplifier
     * 2. Configure ADC sampling time
     * 3. Configure ADC conversion trigger
     * 4. Initialize DMA for continuous measurement
     */

    /* Placeholder: simulate successful initialization */
    (void)memset(&s_TaskNotification, 0, sizeof(s_TaskNotification));

    return status;
}

/**
 * @brief Read current from sensor
 *
 * @description Reads current measurement from ADC.
 *              Returns signed value (positive = charging).
 *
 * @param current_mA Pointer to store current in mA
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Task_ReadCurrent(int32_t* const current_mA)
{
    Soc_StatusType status = SOC_STATUS_OK;

#if (SOC_CFG_PARAM_VALIDATION_ENABLED == 1U)
    if (current_mA == NULL)
    {
        status = SOC_STATUS_NULL_PTR;
    }
    else
#endif
    {
        /*
         * In production, read from ADC:
         *
         * 1. Start ADC conversion
         * 2. Wait for conversion complete
         * 3. Read ADC result
         * 4. Convert to mA using calibration factors
         *
         * Example:
         * uint16_t adcValue = Adc_ReadChannel(ADC_CURRENT_CHANNEL);
         * int32_t rawCurrent = (int32_t)adcValue - ADC_ZERO_OFFSET;
         * *current_mA = (rawCurrent * CURRENT_SCALE) / 1000;
         */

        /* Placeholder: simulate current reading */
        *current_mA = 5000L;  /* 5A discharge for demo */

        /* Validate range */
        if ((*current_mA > (int32_t)SOC_CFG_MAX_CURRENT_MA) ||
            (*current_mA < -(int32_t)SOC_CFG_MAX_CURRENT_MA))
        {
            status = SOC_STATUS_INVALID_STATE;
        }
    }

    return status;
}

/**
 * @brief Read voltage from sensor
 *
 * @description Reads voltage measurement from ADC.
 *
 * @param voltage_mV Pointer to store voltage in mV
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Task_ReadVoltage(uint32_t* const voltage_mV)
{
    Soc_StatusType status = SOC_STATUS_OK;

#if (SOC_CFG_PARAM_VALIDATION_ENABLED == 1U)
    if (voltage_mV == NULL)
    {
        status = SOC_STATUS_NULL_PTR;
    }
    else
#endif
    {
        /*
         * In production, read from ADC:
         *
         * 1. Start ADC conversion
         * 2. Wait for conversion complete
         * 3. Read ADC result
         * 4. Convert to mV using voltage divider ratio
         *
         * Example:
         * uint16_t adcValue = Adc_ReadChannel(ADC_VOLTAGE_CHANNEL);
         * *voltage_mV = (adcValue * VOLTAGE_SCALE) / 1000;
         */

        /* Placeholder: simulate voltage reading */
        *voltage_mV = 37000U;  /* 37V for demo */

        /* Validate range */
        if ((*voltage_mV < SOC_CFG_MIN_VOLTAGE_MV) ||
            (*voltage_mV > SOC_CFG_MAX_VOLTAGE_MV))
        {
            status = SOC_STATUS_INVALID_STATE;
        }
    }

    return status;
}

/*============================================================================*/
/* Public Functions                                                          */
/*============================================================================*/

Soc_StatusType Soc_Task_Create(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /*
     * In production, create FreeRTOS task:
     *
     * BaseType_t ret = xTaskCreate(
     *     Soc_Task_Main,                    // Task function
     *     SOC_CFG_TASK_NAME,                // Task name
     *     SOC_CFG_TASK_STACK_SIZE,          // Stack size (in words)
     *     NULL,                             // Parameters
     *     SOC_CFG_TASK_PRIORITY,            // Priority
     *     &g_SocTaskHandle                  // Task handle
     * );
     *
     * if (ret != pdPASS) {
     *     status = SOC_STATUS_ERROR;
     * }
     */

    /* Placeholder: simulate successful creation */
    g_SocTaskHandle = (void*)0x12345678U;
    s_TaskRunning = true;
    s_TaskSuspended = false;
    s_ConsecutiveErrors = 0U;
    s_LastTickValue = 0U;

    return status;
}

Soc_StatusType Soc_Task_Delete(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /*
     * In production:
     * if (g_SocTaskHandle != NULL) {
     *     vTaskDelete(g_SocTaskHandle);
     *     g_SocTaskHandle = NULL;
     * }
     */

    s_TaskRunning = false;
    g_SocTaskHandle = NULL;

    return status;
}

Soc_StatusType Soc_Task_Suspend(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /*
     * In production:
     * if (g_SocTaskHandle != NULL) {
     *     vTaskSuspend(g_SocTaskHandle);
     * }
     */

    s_TaskSuspended = true;

    return status;
}

Soc_StatusType Soc_Task_Resume(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /*
     * In production:
     * if (g_SocTaskHandle != NULL) {
     *     vTaskResume(g_SocTaskHandle);
     * }
     */

    s_TaskSuspended = false;

    return status;
}

bool Soc_Task_NotifyFromIsr(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    bool retVal = false;

    /*
     * In production:
     * retVal = xTaskNotifyFromISR(
     *     g_SocTaskHandle,
     *     SOC_TASK_NOTIFY_TIMER_BIT,
     *     eSetBits,
     *     &higherPriorityTaskWoken
     * );
     *
     * portYIELD_FROM_ISR(higherPriorityTaskWoken);
     */

    /* Placeholder: set notification bit */
    s_TaskNotification |= SOC_TASK_NOTIFY_TIMER_BIT;
    retVal = true;

    return retVal;
}

Soc_StatusType Soc_Task_GetStats(uint32_t* const runTime, uint32_t* const cycleCount)
{
    Soc_StatusType status = SOC_STATUS_OK;

#if (SOC_CFG_PARAM_VALIDATION_ENABLED == 1U)
    if ((runTime == NULL) || (cycleCount == NULL))
    {
        status = SOC_STATUS_NULL_PTR;
    }
    else
#endif
    {
        *runTime = s_TaskRunTimeUs;
        *cycleCount = s_TaskCycleCount;
    }

    return status;
}

bool Soc_Task_IsRunning(void)
{
    return s_TaskRunning;
}

uint32_t Soc_Task_GetNotificationValue(void)
{
    return s_TaskNotification;
}

uint32_t Soc_Task_ClearNotification(void)
{
    uint32_t prevValue = s_TaskNotification;
    s_TaskNotification = 0U;
    return prevValue;
}

/**
 * @brief Main SOC task function
 *
 * @description Main task loop for SOC estimation.
 *              Waits for timer notification, then updates SOC.
 *
 *              Task Flow:
 *              1. Wait for timer notification (blocked)
 *              2. Clear notification
 *              3. Read current/voltage sensors
 *              4. Update SOC algorithm
 *              5. Update statistics
 *              6. Feed watchdog
 *              7. Repeat
 *
 * @param params Task parameters (unused)
 */
void Soc_Task_Main(void* params)
{
    Soc_StatusType status;
    uint32_t startTick;

    /* Prevent unused parameter warning */
    (void)params;

    /* Initialize current sensor */
    status = Soc_Task_InitCurrentSensor();
    if (status != SOC_STATUS_OK)
    {
        /* Sensor init failed - enter degraded mode */
        Soc_Task_EnterDegradedMode();
    }

    /* Main task loop */
    for (;;)
    {
        /*
         * Wait for timer notification
         * This blocks the task until timer ISR sends notification
         *
         * In production:
         * uint32_t notification = ulTaskNotifyTake(
         *     SOC_TASK_NOTIFY_CLEAR,    // Clear on entry
         *     SOC_TASK_WAIT_FOREVER     // Wait forever
         * );
         */

        /* Placeholder: wait for notification (simulated) */
        while ((s_TaskNotification & SOC_TASK_NOTIFY_TIMER_BIT) == 0U)
        {
            /* Simulate task delay */
            /* vTaskDelay(1); */
        }

        /* Clear notification */
        (void)Soc_Task_ClearNotification();

        /* Record start time for statistics */
        startTick = Soc_Timer_S32K4_GetTick();

        /* Process SOC update */
        status = Soc_Task_ProcessUpdate();

        /* Update statistics */
        Soc_Task_UpdateStats(startTick);

        /* Feed watchdog */
        Soc_Task_FeedWatchdog();

        /* Safety: Check for stack overflow in production
         * if (uxTaskGetStackHighWaterMark(NULL) < threshold) {
         *     // Handle stack overflow
         * }
         */
    }

    /* Task should never reach here */
    /* vTaskDelete(NULL); */
}

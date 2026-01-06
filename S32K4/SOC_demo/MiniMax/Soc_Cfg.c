/**
 * @file Soc_Cfg.c
 * @brief Configuration implementation for SOC estimation module
 * @module Soc_Cfg
 *
 * @assumptions & constraints
 *   - See Soc_Cfg.h for configuration assumptions
 *
 * @safety considerations
 *   - All values are compile-time constants
 *   - No dynamic memory allocation
 *   - Read-only access to configuration
 *
 * @version 1.0.0
 * @compliance MISRA C:2012, ISO 26262
 */

#include "Soc_Cfg.h"
#include <stdbool.h>

/*============================================================================*/
/* Configuration Data                                                         */
/*============================================================================*/

/**
 * @brief SOC estimation configuration
 *
 * @safety-critical
 *   - Values must be calibrated for the specific battery pack
 *   - Incorrect values lead to inaccurate SOC estimation
 */
const Soc_ConfigType g_SocConfig = SOC_CFG_CREATE_CONFIG();

/*============================================================================*/
/* Runtime Data                                                              */
/*============================================================================*/

/**
 * @brief SOC estimation runtime data
 *
 * @note Initialized to safe default values
 *       Will be updated during operation
 *
 * @safety-critical
 *   - Shared between timer ISR and task context
 *   - Protected by critical section when accessed from task
 */
volatile Soc_RuntimeType g_SocRuntime =
{
    .accumulatedCharge_uAs   = 0LL,          /* No accumulated charge at start */
    .socPermille             = SOC_CFG_INIT_SOC_PERMILLE,
    .lastSocPermille         = SOC_CFG_INIT_SOC_PERMILLE,
    .current_mA              = 0L,
    .direction               = SOC_DIRECTION_IDLE,
    .tickCounter             = 0U,
    .initialized             = false,
    .dataValid               = false,
    .mode                    = SOC_MODE_NORMAL
};

/*============================================================================*/
/* History Buffer (For Plausibility Checks)                                  */
/*============================================================================*/

#if (SOC_CFG_PLAUSIBILITY_CHECK_ENABLED == 1U)

/**
 * @brief SOC history for rate-of-change monitoring
 *
 * @note Circular buffer implementation
 */
static uint16_t s_SocHistory[SOC_CFG_HISTORY_SIZE];

/**
 * @brief History buffer write index
 */
static uint8_t s_HistoryIndex = 0U;

/**
 * @brief History buffer count
 */
static uint8_t s_HistoryCount = 0U;

#endif /* SOC_CFG_PLAUSIBILITY_CHECK_ENABLED */

/*============================================================================*/
/* Local Function Prototypes                                                 */
/*============================================================================*/

#if (SOC_CFG_PLAUSIBILITY_CHECK_ENABLED == 1U)
static bool Soc_Cfg_IsPlausible(uint16_t socPermille);
static void Soc_Cfg_UpdateHistory(uint16_t socPermille);
#endif

/*============================================================================*/
/* History Functions                                                         */
/*============================================================================*/

#if (SOC_CFG_PLAUSIBILITY_CHECK_ENABLED == 1U)

/**
 * @brief Update SOC history buffer
 *
 * @param socPermille New SOC value to record
 *
 * @note Thread-safe when called from single context
 */
static void Soc_Cfg_UpdateHistory(uint16_t socPermille)
{
    /* Update circular buffer */
    s_SocHistory[s_HistoryIndex] = socPermille;

    /* Increment and wrap index */
    s_HistoryIndex++;
    if (s_HistoryIndex >= SOC_CFG_HISTORY_SIZE)
    {
        s_HistoryIndex = 0U;
    }

    /* Update count */
    if (s_HistoryCount < SOC_CFG_HISTORY_SIZE)
    {
        s_HistoryCount++;
    }
}

/**
 * @brief Check if SOC value is plausible
 *
 * @description Validates SOC against historical values and rate limits.
 *
 * @param socPermille SOC value to validate
 *
 * @return true if plausible, false if implausible
 *
 * @safety-critical
 *   - Detects sensor faults and calculation errors
 *   - Prevents invalid SOC values from propagating
 */
static bool Soc_Cfg_IsPlausible(uint16_t socPermille)
{
    bool retVal = true;
    uint8_t i;
    int16_t delta;

    /* Check basic range */
    if (socPermille > 1000U)
    {
        retVal = false;
    }

    /* Check rate of change if history is available */
    if ((retVal == true) && (s_HistoryCount > 0U))
    {
        /* Compare with recent history */
        for (i = 0U; i < s_HistoryCount; i++)
        {
            delta = (int16_t)socPermille - (int16_t)s_SocHistory[i];

            /* Absolute value for comparison */
            if (delta < 0)
            {
                delta = -delta;
            }

            /* Check for implausible jump */
            if (delta > (int16_t)SOC_CFG_MAX_SOC_CHANGE_PERMILLE)
            {
                retVal = false;
                break;
            }
        }
    }

    return retVal;
}

#endif /* SOC_CFG_PLAUSIBILITY_CHECK_ENABLED */

/*============================================================================*/
/* Public Functions                                                          */
/*============================================================================*/

/**
 * @brief Validate SOC configuration
 *
 * @description Checks that all configuration values are within valid ranges.
 *
 * @return SOC_STATUS_OK if valid, error code otherwise
 *
 * @safety-critical
 *   - Must be called before module initialization
 *   - Prevents invalid configuration from causing runtime errors
 */
Soc_StatusType Soc_Cfg_ValidateConfig(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /* Validate battery capacity */
    if (SOC_CFG_BATTERY_CAPACITY_AH == 0U)
    {
        status = SOC_STATUS_INVALID_PARAM;
    }

    /* Validate voltage range */
    if (SOC_CFG_MIN_VOLTAGE_MV >= SOC_CFG_MAX_VOLTAGE_MV)
    {
        status = SOC_STATUS_INVALID_PARAM;
    }

    /* Validate coulomb efficiency (must be < 100%) */
    if (SOC_CFG_COULOMB_EFFICIENCY_0P1 >= 1000U)
    {
        status = SOC_STATUS_INVALID_PARAM;
    }

    /* Validate initial SOC */
    if (SOC_CFG_INIT_SOC_PERMILLE > 1000U)
    {
        status = SOC_STATUS_INVALID_PARAM;
    }

    /* Validate timer period (minimum 1us, maximum 1s) */
    if ((SOC_CFG_TIMER_PERIOD_US < 1U) || (SOC_CFG_TIMER_PERIOD_US > 1000000U))
    {
        status = SOC_STATUS_INVALID_PARAM;
    }

    /* Validate timer frequency */
    if (SOC_CFG_TIMER_FREQ_HZ == 0U)
    {
        status = SOC_STATUS_INVALID_PARAM;
    }

    return status;
}

/**
 * @brief Get SOC history statistics
 *
 * @param[out] maxDelta Pointer to store maximum delta
 * @param[out] avgDelta Pointer to store average delta
 *
 * @return SOC_STATUS_OK on success
 *
 * @pre History must have at least 2 entries
 */
Soc_StatusType Soc_Cfg_GetHistoryStats(int16_t* const maxDelta, int16_t* const avgDelta)
{
    Soc_StatusType status = SOC_STATUS_OK;
    uint8_t i;
    int16_t delta;
    int32_t totalDelta = 0;

#if (SOC_CFG_PARAM_VALIDATION_ENABLED == 1U)
    if ((maxDelta == NULL) || (avgDelta == NULL))
    {
        status = SOC_STATUS_NULL_PTR;
    }
    else
#endif
    {
        *maxDelta = 0;

        if (s_HistoryCount > 1U)
        {
            for (i = 1U; i < s_HistoryCount; i++)
            {
                delta = (int16_t)s_SocHistory[i] - (int16_t)s_SocHistory[i-1];
                if (delta < 0)
                {
                    delta = -delta;
                }

                if (delta > *maxDelta)
                {
                    *maxDelta = delta;
                }

                totalDelta += delta;
            }

            *avgDelta = totalDelta / (int16_t)(s_HistoryCount - 1U);
        }
        else
        {
            *avgDelta = 0;
        }
    }

    return status;
}

/**
 * @brief Clear SOC history
 *
 * @note Call when resetting SOC estimation
 */
void Soc_Cfg_ClearHistory(void)
{
    uint8_t i;

    for (i = 0U; i < SOC_CFG_HISTORY_SIZE; i++)
    {
        s_SocHistory[i] = g_SocRuntime.socPermille;
    }

    s_HistoryIndex = 0U;
    s_HistoryCount = SOC_CFG_HISTORY_SIZE;
}

/**
 * @brief Get initialization state
 *
 * @return true if initialized, false otherwise
 */
bool Soc_Cfg_IsInitialized(void)
{
    return g_SocRuntime.initialized;
}

/**
 * @brief Set initialization state
 *
 * @param state New initialization state
 */
void Soc_Cfg_SetInitialized(bool state)
{
    g_SocRuntime.initialized = state;
}

/**
 * @brief Get current SOC
 *
 * @return Current SOC in per-mille
 */
uint16_t Soc_Cfg_GetSocPermille(void)
{
    return g_SocRuntime.socPermille;
}

/**
 * @brief Set current SOC
 *
 * @param socPermille New SOC value
 *
 * @note Updates history and last SOC
 */
void Soc_Cfg_SetSocPermille(uint16_t socPermille)
{
    g_SocRuntime.lastSocPermille = g_SocRuntime.socPermille;
    g_SocRuntime.socPermille = socPermille;

#if (SOC_CFG_PLAUSIBILITY_CHECK_ENABLED == 1U)
    Soc_Cfg_UpdateHistory(socPermille);
#endif
}

/**
 * @brief Get accumulated charge
 *
 * @return Accumulated charge in microampere-seconds
 */
int64_t Soc_Cfg_GetAccumulatedCharge(void)
{
    return g_SocRuntime.accumulatedCharge_uAs;
}

/**
 * @brief Add to accumulated charge
 *
 * @param delta_uAs Charge delta in microampere-seconds
 */
void Soc_Cfg_AddAccumulatedCharge(int64_t delta_uAs)
{
    g_SocRuntime.accumulatedCharge_uAs += delta_uAs;
}

/**
 * @brief Get current direction
 *
 * @return Current flow direction
 */
Soc_DirectionType Soc_Cfg_GetDirection(void)
{
    return g_SocRuntime.direction;
}

/**
 * @brief Set current direction
 *
 * @param direction New direction
 */
void Soc_Cfg_SetDirection(Soc_DirectionType direction)
{
    g_SocRuntime.direction = direction;
}

/**
 * @brief Get operating mode
 *
 * @return Current operating mode
 */
Soc_OperatingModeType Soc_Cfg_GetMode(void)
{
    return g_SocRuntime.mode;
}

/**
 * @brief Set operating mode
 *
 * @param mode New operating mode
 */
void Soc_Cfg_SetMode(Soc_OperatingModeType mode)
{
    g_SocRuntime.mode = mode;
}

/**
 * @brief Get tick counter
 *
 * @return Current tick count
 */
uint32_t Soc_Cfg_GetTickCounter(void)
{
    return g_SocRuntime.tickCounter;
}

/**
 * @brief Increment tick counter
 */
void Soc_Cfg_IncrementTickCounter(void)
{
    g_SocRuntime.tickCounter++;
}

/**
 * @brief Get data validity
 *
 * @return true if data is valid
 */
bool Soc_Cfg_IsDataValid(void)
{
    return g_SocRuntime.dataValid;
}

/**
 * @brief Set data validity
 *
 * @param valid New validity state
 */
void Soc_Cfg_SetDataValid(bool valid)
{
    g_SocRuntime.dataValid = valid;
}

/**
 * @brief Set current measurement
 *
 * @param current_mA Current in milliamperes
 */
void Soc_Cfg_SetCurrent(int32_t current_mA)
{
    g_SocRuntime.current_mA = current_mA;
}

/**
 * @brief Get current measurement
 *
 * @return Current in milliamperes
 */
int32_t Soc_Cfg_GetCurrent(void)
{
    return g_SocRuntime.current_mA;
}

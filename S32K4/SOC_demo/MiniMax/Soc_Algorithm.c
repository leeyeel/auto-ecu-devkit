/**
 * @file Soc_Algorithm.c
 * @brief Coulomb Counting Algorithm implementation
 * @module Soc_Algorithm
 *
 * @assumptions & constraints
 *   - Timer provides accurate 5ms interrupts
 *   - Current measurement resolution: 1mA or better
 *   - Battery capacity in whole Ampere-hours
 *
 * @safety considerations
 *   - Charge accumulation uses 64-bit to prevent overflow
 *   - SOC clamped to [0, 1000] per-mille range
 *   - Error propagation handled through status codes
 *
 * @execution context
 *   - Called from 5ms periodic task
 *   - May be called from ISR (if simple version)
 *
 * @version 1.0.0
 * @compliance MISRA C:2012, ISO 26262
 */

#include "Soc_Algorithm.h"
#include "Soc_Cfg.h"
#include <stdbool.h>
#include <float.h>

/*============================================================================*/
/* Local Variables                                                           */
/*============================================================================*/

/**
 * @brief Current SOC in per-mille (0-1000)
 */
static uint16_t s_SocPermille = SOC_ALG_MAX_SOC_PERMILLE;

/**
 * @brief Last valid SOC in per-mille
 */
static uint16_t s_LastSocPermille = SOC_ALG_MAX_SOC_PERMILLE;

/**
 * @brief Accumulated charge in microampere-seconds (uAs)
 *
 * @note Positive = charge, negative = discharge
 */
static int64_t s_AccumulatedCharge_uAs = 0LL;

/**
 * @brief Total battery capacity in microampere-seconds
 */
static int64_t s_BatteryCapacity_uAs = 0LL;

/**
 * @brief Coulomb efficiency factor (0.0 - 1.0)
 */
static float s_CoulombEfficiency = 0.985F;

/**
 * @brief Current measurement in mA
 */
static int32_t s_Current_mA = 0L;

/**
 * @brief Current direction
 */
static Soc_DirectionType s_Direction = SOC_DIRECTION_IDLE;

/**
 * @brief Last update status
 */
static Soc_StatusType s_LastStatus = SOC_STATUS_OK;

/**
 * @brief Update counter
 */
static uint32_t s_UpdateCounter = 0U;

/**
 * @brief Algorithm initialization state
 */
static bool s_Initialized = false;

/**
 * @brief Plausibility check enabled
 */
#if (SOC_CFG_PLAUSIBILITY_CHECK_ENABLED == 1U)
static bool s_PlausibilityCheckEnabled = true;
#else
static bool s_PlausibilityCheckEnabled = false;
#endif

/*============================================================================*/
/* Local Function Prototypes                                                 */
/*============================================================================*/

/**
 * @brief Saturate SOC to valid range
 *
 * @param soc SOC value to saturate
 *
 * @return Saturated SOC value
 */
static uint16_t Soc_Algorithm_SaturateSoc(uint16_t soc);

/**
 * @brief Convert voltage to SOC using OCV curve
 *
 * @param voltage_mV Voltage in millivolts
 *
 * @return SOC in per-mille (0-1000)
 *
 * @note Simplified linear model - replace with actual curve
 */
static uint16_t Soc_Algorithm_OcvToSoc(uint32_t voltage_mV);

/**
 * @brief Check SOC plausibility
 *
 * @param newSoc New SOC value to validate
 *
 * @return true if plausible, false if implausible
 */
static bool Soc_Algorithm_IsPlausible(uint16_t newSoc);

/**
 * @brief Calculate charge delta
 *
 * @param current_mA Current in mA
 * @param deltaTimeUs Time in microseconds
 *
 * @return Charge delta in uAs
 */
static int64_t Soc_Algorithm_CalcChargeDelta(int32_t current_mA, uint32_t deltaTimeUs);

/*============================================================================*/
/* Local Functions                                                           */
/*============================================================================*/

/**
 * @brief Saturate SOC to [0, 1000] per-mille range
 */
static uint16_t Soc_Algorithm_SaturateSoc(uint16_t soc)
{
    uint16_t retVal = soc;

    if (retVal < SOC_ALG_MIN_SOC_PERMILLE)
    {
        retVal = SOC_ALG_MIN_SOC_PERMILLE;
    }
    else if (retVal > SOC_ALG_MAX_SOC_PERMILLE)
    {
        retVal = SOC_ALG_MAX_SOC_PERMILLE;
    }
    else
    {
        /* Value within range */
    }

    return retVal;
}

/**
 * @brief Simplified OCV to SOC conversion
 *
 * @description Linear approximation of OCV-SOC curve.
 *              Replace with actual battery model in production.
 *
 *              Example for Li-ion battery:
 *              - 3.0V  = 0% SOC
 *              - 3.7V  = ~50% SOC
 *              - 4.2V  = 100% SOC
 *
 * @param voltage_mV Voltage in millivolts
 *
 * @return Estimated SOC in per-mille
 */
static uint16_t Soc_Algorithm_OcvToSoc(uint32_t voltage_mV)
{
    uint32_t socPermille = 0U;
    uint32_t minVoltage = SOC_CFG_MIN_VOLTAGE_MV;
    uint32_t maxVoltage = SOC_CFG_MAX_VOLTAGE_MV;
    uint32_t range = maxVoltage - minVoltage;

    if (range > 0U)
    {
        if (voltage_mV <= minVoltage)
        {
            socPermille = 0U;
        }
        else if (voltage_mV >= maxVoltage)
        {
            socPermille = 1000U;
        }
        else
        {
            /* Linear interpolation */
            socPermille = ((voltage_mV - minVoltage) * 1000U) / range;
        }
    }

    return (uint16_t)socPermille;
}

/**
 * @brief Check if SOC value is plausible
 *
 * @description Checks for unrealistic SOC changes.
 *
 * @param newSoc New SOC value
 *
 * @return true if plausible, false if implausible
 */
static bool Soc_Algorithm_IsPlausible(uint16_t newSoc)
{
    bool retVal = true;
    int16_t delta;

    /* Check basic range */
    if (newSoc > SOC_ALG_MAX_SOC_PERMILLE)
    {
        retVal = false;
    }

    /* Check rate of change against maximum */
    if (retVal == true)
    {
        if (newSoc > s_LastSocPermille)
        {
            delta = (int16_t)(newSoc - s_LastSocPermille);
        }
        else
        {
            delta = (int16_t)(s_LastSocPermille - newSoc);
        }

        /* Check against configured maximum change */
        if ((uint16_t)delta > SOC_CFG_MAX_SOC_CHANGE_PERMILLE)
        {
            retVal = false;
        }
    }

    return retVal;
}

/**
 * @brief Calculate charge delta
 *
 * @description Q = I * t
 *              - Current in mA = I * 1000
 *              - Time in us = t / 1000000
 *              - Q = I * 1000 * t / 1000000 = I * t / 1000
 *
 * @param current_mA Current in mA
 * @param deltaTimeUs Time in microseconds
 *
 * @return Charge delta in uAs
 */
static int64_t Soc_Algorithm_CalcChargeDelta(int32_t current_mA, uint32_t deltaTimeUs)
{
    /* Charge in uAs = current_mA * 1000 * time_us / 1000000
     *               = current_mA * time_us / 1000
     */
    int64_t chargeDelta = ((int64_t)current_mA * (int64_t)deltaTimeUs) / 1000LL;

    return chargeDelta;
}

/*============================================================================*/
/* Public Functions                                                          */
/*============================================================================*/

Soc_StatusType Soc_Algorithm_Init(uint16_t initSocPermille)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /* Validate initial SOC */
    if (initSocPermille > SOC_ALG_MAX_SOC_PERMILLE)
    {
        status = SOC_STATUS_INVALID_PARAM;
    }
    else
    {
        /* Calculate battery capacity in uAs
         * Capacity = Ah * 3600s * 1000mA * 1000uA
         *          = Ah * 3600 * 1000000 uAs
         */
        s_BatteryCapacity_uAs = (int64_t)SOC_CFG_BATTERY_CAPACITY_AH *
                                3600LL *
                                1000000LL;

        /* Initialize coulomb efficiency */
        #if (SOC_CFG_COULOMB_EFFICIENCY_ENABLED == 1U)
        s_CoulombEfficiency = (float)SOC_CFG_COULOMB_EFFICIENCY_0P1 / 1000.0F;
        #else
        s_CoulombEfficiency = 1.0F;
        #endif

        /* Initialize state */
        s_SocPermille = initSocPermille;
        s_LastSocPermille = initSocPermille;
        s_AccumulatedCharge_uAs = 0LL;
        s_Current_mA = 0L;
        s_Direction = SOC_DIRECTION_IDLE;
        s_LastStatus = SOC_STATUS_OK;
        s_UpdateCounter = 0U;
        s_Initialized = true;
    }

    return status;
}

Soc_StatusType Soc_Algorithm_Update(int32_t current_mA, uint32_t deltaTimeUs)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /* Validation */
    if (s_Initialized == false)
    {
        status = SOC_STATUS_NOT_INITIALIZED;
    }
    else if ((current_mA == 0L) || (deltaTimeUs == 0U))
    {
        /* No change, current or time is zero */
        /* This is not an error, just no update needed */
        status = SOC_STATUS_OK;
    }
    else
    {
        /* Store current for later use */
        s_Current_mA = current_mA;

        /* Determine direction */
        if (current_mA > 0)
        {
            s_Direction = SOC_DIRECTION_CHARGE;
        }
        else
        {
            s_Direction = SOC_DIRECTION_DISCHARGE;
        }

        /* Calculate charge delta */
        int64_t chargeDelta = Soc_Algorithm_CalcChargeDelta(current_mA, deltaTimeUs);

        /* Apply coulomb efficiency for charging
         * During charging, not all charge is stored due to losses
         */
        if (chargeDelta > 0)
        {
            /* Charging - apply efficiency */
            float chargeDeltaFloat = (float)chargeDelta * s_CoulombEfficiency;
            chargeDelta = (int64_t)chargeDeltaFloat;
        }
        /* During discharging, efficiency is typically 100%
         * (all current comes from stored charge)
         */

        /* Update accumulated charge */
        s_AccumulatedCharge_uAs += chargeDelta;

        /* Calculate SOC change
         * dSOC = dQ / Q_capacity * 1000
         */
        int64_t socChange = (chargeDelta * 1000LL) / s_BatteryCapacity_uAs;

        /* Update SOC */
        if (socChange > 0)
        {
            /* SOC increases during charging */
            s_SocPermille = (uint16_t)((int32_t)s_SocPermille + (int32_t)socChange);
        }
        else
        {
            /* SOC decreases during discharging */
            s_SocPermille = (uint16_t)((int32_t)s_SocPermille - (int32_t)(-socChange));
        }

        /* Saturate SOC to valid range */
        s_SocPermille = Soc_Algorithm_SaturateSoc(s_SocPermille);

        /* Plausibility check */
        #if (SOC_CFG_PLAUSIBILITY_CHECK_ENABLED == 1U)
        if (s_PlausibilityCheckEnabled == true)
        {
            if (Soc_Algorithm_IsPlausible(s_SocPermille) == false)
            {
                /* Plausibility check failed - revert to last valid value */
                s_SocPermille = s_LastSocPermille;
                status = SOC_STATUS_INVALID_STATE;

                /* Recalculate accumulated charge */
                /* This is approximate - in production, log error */
            }
            else
            {
                s_LastSocPermille = s_SocPermille;
            }
        }
        else
        {
            s_LastSocPermille = s_SocPermille;
        }
        #else
        s_LastSocPermille = s_SocPermille;
        #endif

        /* Update counter */
        s_UpdateCounter++;
        s_LastStatus = status;
    }

    return status;
}

Soc_StatusType Soc_Algorithm_UpdateWithOcvFusion(
    int32_t current_mA,
    uint32_t voltage_mV,
    uint32_t deltaTimeUs,
    uint16_t ocvWeight
)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /* Validate OCV weight */
    if (ocvWeight > 1000U)
    {
        status = SOC_STATUS_INVALID_PARAM;
    }
    else
    {
        /* First update with coulomb counting */
        status = Soc_Algorithm_Update(current_mA, deltaTimeUs);

        if (status == SOC_STATUS_OK)
        {
            /* Calculate OCV-based SOC */
            uint16_t ocvSoc = Soc_Algorithm_OcvToSoc(voltage_mV);

            /* Fuse with coulomb counting SOC
             * Only apply fusion when current is low (near OCV condition)
             */
            if ((current_mA < SOC_CFG_MIN_CURRENT_MA) &&
                (current_mA > -(int32_t)SOC_CFG_MIN_CURRENT_MA))
            {
                /* Weighted average
                 * Final = CC_SOC * (1 - weight) + OCV_SOC * weight
                 */
                uint16_t ccSoc = s_SocPermille;
                uint32_t finalSoc;

                finalSoc = ((uint32_t)ccSoc * (1000U - ocvWeight)) +
                           ((uint32_t)ocvSoc * ocvWeight);

                s_SocPermille = Soc_Algorithm_SaturateSoc((uint16_t)(finalSoc / 1000U));
            }
        }
    }

    return status;
}

uint16_t Soc_Algorithm_GetSoc(void)
{
    return s_SocPermille;
}

int32_t Soc_Algorithm_GetRemainingCapacity_mAh(void)
{
    int32_t remainingCapacity;

    /* Remaining = SOC * Capacity / 1000 */
    if (s_SocPermille <= SOC_ALG_MAX_SOC_PERMILLE)
    {
        remainingCapacity = (int32_t)((int64_t)s_SocPermille *
                            (int64_t)SOC_CFG_BATTERY_CAPACITY_AH /
                            1000LL);
    }
    else
    {
        remainingCapacity = (int32_t)SOC_CFG_BATTERY_CAPACITY_AH;
    }

    return remainingCapacity;
}

int64_t Soc_Algorithm_GetAccumulatedCharge(void)
{
    return s_AccumulatedCharge_uAs;
}

Soc_StatusType Soc_Algorithm_Reset(uint16_t initSocPermille)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /* Validate parameter */
    if (initSocPermille > SOC_ALG_MAX_SOC_PERMILLE)
    {
        status = SOC_STATUS_INVALID_PARAM;
    }
    else
    {
        /* Reset all state variables */
        s_SocPermille = initSocPermille;
        s_LastSocPermille = initSocPermille;
        s_AccumulatedCharge_uAs = 0LL;
        s_Current_mA = 0L;
        s_Direction = SOC_DIRECTION_IDLE;
        s_UpdateCounter = 0U;
        s_Initialized = true;
        s_LastStatus = SOC_STATUS_OK;
    }

    return status;
}

Soc_DirectionType Soc_Algorithm_GetDirection(void)
{
    return s_Direction;
}

Soc_StatusType Soc_Algorithm_GetLastStatus(void)
{
    return s_LastStatus;
}

uint16_t Soc_Algorithm_EstimateSocFromOcv(uint32_t voltage_mV)
{
    return Soc_Algorithm_OcvToSoc(voltage_mV);
}

Soc_StatusType Soc_Algorithm_SelfTest(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /* Check initialization state */
    if (s_Initialized == false)
    {
        status = SOC_STATUS_NOT_INITIALIZED;
    }

    /* Check SOC range */
    if (s_SocPermille > SOC_ALG_MAX_SOC_PERMILLE)
    {
        status = SOC_STATUS_INVALID_STATE;
    }

    /* Check accumulated charge is within capacity */
    if (s_AccumulatedCharge_uAs > s_BatteryCapacity_uAs)
    {
        status = SOC_STATUS_OVERFLOW;
    }

    /* Check battery capacity is reasonable */
    if (s_BatteryCapacity_uAs <= 0)
    {
        status = SOC_STATUS_ERROR;
    }

    return status;
}

/*============================================================================*/
/* Algorithm Interface Implementation                                         */
/*============================================================================*/

/**
 * @brief Algorithm interface for SOC estimation
 */
static const Soc_AlgorithmInterfaceType s_AlgorithmInterface =
{
    .Init               = Soc_Algorithm_Init,
    .Update             = Soc_Algorithm_Update,
    .GetSoc             = Soc_Algorithm_GetSoc,
    .Reset              = Soc_Algorithm_Reset,
    .GetAccumulatedCharge = Soc_Algorithm_GetAccumulatedCharge
};

/**
 * @brief Get algorithm interface pointer
 */
const Soc_AlgorithmInterfaceType* Soc_Algorithm_GetInterface(void)
{
    return &s_AlgorithmInterface;
}

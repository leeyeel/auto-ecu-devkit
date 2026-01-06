/**
 * @file Soc_Cfg.h
 * @brief Configuration for SOC estimation module
 * @module Soc_Cfg
 *
 * @assumptions & constraints
 *   - S32K4 LPIT timer used for 5ms periodic interrupt
 *   - FreeRTOS available for task scheduling
 *   - Battery parameters must be calibrated for specific pack
 *
 * @safety considerations
 *   - Configuration values affect SOC accuracy
 *   - Timer period must be achievable with system clock
 *   - Stack size must accommodate worst-case execution
 *
 * @version 1.0.0
 * @compliance MISRA C:2012, ISO 26262
 */

#ifndef SOC_CFG_H
#define SOC_CFG_H

#include "Soc_Types.h"

/*============================================================================*/
/* Module Configuration                                                       */
/*============================================================================*/

/**
 * @brief Enable/disable SOC module
 *
 * @safety Setting to false completely disables the module
 */
#define SOC_CFG_ENABLED                     (1U)

/**
 * @brief Enable/disable runtime parameter validation
 *
 * @note Disable for production to reduce overhead
 */
#define SOC_CFG_PARAM_VALIDATION_ENABLED    (1U)

/**
 * @brief Enable/disable plausibility checks
 *
 * @note Must be enabled for safety-critical applications
 */
#define SOC_CFG_PLAUSIBILITY_CHECK_ENABLED  (1U)

/**
 * @brief Enable/disable coulomb efficiency compensation
 */
#define SOC_CFG_COULOMB_EFFICIENCY_ENABLED  (1U)

/**
 * @brief Enable/disable data freshness check
 */
#define SOC_CFG_DATA_FRESHNESS_ENABLED      (1U)

/*============================================================================*/
/* Battery Configuration                                                      */
/*============================================================================*/

/**
 * @brief Total battery capacity in Ampere-hours
 *
 * @note Must match actual battery pack capacity
 */
#define SOC_CFG_BATTERY_CAPACITY_AH         (50U)

/**
 * @brief Nominal battery voltage in millivolts
 *
 * @note Example: 37000mV for a 37V nominal pack
 */
#define SOC_CFG_NOMINAL_VOLTAGE_MV          (37000U)

/**
 * @brief Minimum safe voltage in millivolts
 *
 * @note Must be above deep discharge threshold
 */
#define SOC_CFG_MIN_VOLTAGE_MV              (30000U)

/**
 * @brief Maximum safe voltage in millivolts
 *
 * @note Must be below overcharge threshold
 */
#define SOC_CFG_MAX_VOLTAGE_MV              (42000U)

/**
 * @brief Coulomb efficiency in 0.1% units
 *
 * @note 985 = 98.5% efficiency
 *       Must be less than 1000 to prevent SOC drift during cycling
 */
#define SOC_CFG_COULOMB_EFFICIENCY_0P1      (985U)

/**
 * @brief Initial SOC in per-mille (1000 = 100%)
 *
 * @note Can be set to 1000 for fully charged initial state
 */
#define SOC_CFG_INIT_SOC_PERMILLE           (1000U)

/*============================================================================*/
/* Timer Configuration                                                        */
/*============================================================================*/

/**
 * @brief Timer period in microseconds
 *
 * @note 5000 = 5ms period
 *       LPIT at 80MHz with 80 divider = 1MHz = 1us resolution
 */
#define SOC_CFG_TIMER_PERIOD_US             (5000U)

/**
 * @brief Timer instance (LPIT channel)
 *
 * @note S32K4 has 4 LPIT channels (0-3)
 */
#define SOC_CFG_TIMER_INSTANCE              (0U)

/**
 * @brief Timer interrupt priority
 *
 * @note Must be lower than FreeRTOS kernel interrupts
 *       Recommend: 5-10 for S32K4 (lower = higher priority)
 */
#define SOC_CFG_TIMER_IRQ_PRIORITY          (6U)

/**
 * @brief LPIT clock source (0=FXOSC,1=SIRCDIV1,2=SIRCDIV2,3=SIRC)
 *
 * @note Using SIRCDIV2 at 8MHz for LPIT
 */
#define SOC_CFG_TIMER_CLOCK_SOURCE          (2U)

/**
 * @brief LPIT clock divider (1-128)
 *
 * @note LPIT clock = source / divider
 */
#define SOC_CFG_TIMER_CLOCK_DIVIDER         (8U)

/*============================================================================*/
/* Task Configuration                                                         */
/*============================================================================*/

/**
 * @brief SOC task stack size in words
 *
 * @note Must be sufficient for nested interrupts and stack usage
 */
#define SOC_CFG_TASK_STACK_SIZE             (256U)

/**
 * @brief SOC task priority
 *
 * @note Must be lower than timer ISR priority
 *       Higher than idle task
 */
#define SOC_CFG_TASK_PRIORITY               (4U)

/**
 * @brief SOC task name (for debugging)
 */
#define SOC_CFG_TASK_NAME                   ("SOC_Estimation")

/**
 * @brief SOC task tick count history size
 *
 * @note For rate-of-change monitoring
 */
#define SOC_CFG_HISTORY_SIZE                (16U)

/*============================================================================*/
/* Plausibility Check Configuration                                           */
/*============================================================================*/

/**
 * @brief Maximum allowed SOC change per update (per-mille)
 *
 * @note Limits sudden SOC jumps (10 per-mille = 1%)
 *       Adjust based on expected maximum current
 */
#define SOC_CFG_MAX_SOC_CHANGE_PERMILLE     (10U)

/**
 * @brief Maximum allowed current in mA
 *
 * @note 500A = 500,000mA for a 50Ah pack at 10C rate
 */
#define SOC_CFG_MAX_CURRENT_MA              (500000U)

/**
 * @brief Minimum detectable current in mA
 *
 * @note Below this, current is considered zero (noise floor)
 */
#define SOC_CFG_MIN_CURRENT_MA              (10U)

/**
 * @brief Data freshness timeout in ticks
 *
 * @note If no new data within this time, data considered stale
 */
#define SOC_CFG_DATA_FRESHNESS_TIMEOUT_TICK (10U)

/*============================================================================*/
/* Safety Configuration                                                       */
/*============================================================================*/

/**
 * @brief Enable safe state transition on error
 */
#define SOC_CFG_SAFE_STATE_ON_ERROR         (1U)

/**
 * @brief Maximum consecutive errors before safe state
 */
#define SOC_CFG_MAX_CONSECUTIVE_ERRORS      (5U)

/**
 * @brief Enable watchdog feed in task
 */
#define SOC_CFG_WATCHDOG_FEED_ENABLED       (0U)

/*============================================================================*/
/* Derived Constants (Do Not Modify)                                         */
/*============================================================================*/

/**
 * @brief Battery capacity in microampere-seconds
 *
 * @note Q = I * t = Ah * 3600 * 1000 * 1000 (uAs per Ah)
 */
#define SOC_CFG_BATTERY_CAPACITY_UAS        ((uint64_t)SOC_CFG_BATTERY_CAPACITY_AH * \
                                             3600ULL * 1000000ULL)

/**
 * @brief Timer frequency in Hz (derived)
 */
#define SOC_CFG_TIMER_FREQ_HZ               (8000000U / SOC_CFG_TIMER_CLOCK_DIVIDER)

/**
 * @brief Timer period in ticks (derived)
 */
#define SOC_CFG_TIMER_PERIOD_TICK           ((SOC_CFG_TIMER_FREQ_HZ * SOC_CFG_TIMER_PERIOD_US) / \
                                             1000000U)

/*============================================================================*/
/* API Macros                                                                */
/*============================================================================*/

/**
 * @brief Create SOC configuration structure
 *
 * @note Initializes config with compile-time constants
 */
#define SOC_CFG_CREATE_CONFIG() { \
    .batteryCapacityAh       = SOC_CFG_BATTERY_CAPACITY_AH, \
    .nominalVoltage_mV       = SOC_CFG_NOMINAL_VOLTAGE_MV, \
    .minVoltage_mV           = SOC_CFG_MIN_VOLTAGE_MV, \
    .maxVoltage_mV           = SOC_CFG_MAX_VOLTAGE_MV, \
    .coulombEfficiency_0p1   = SOC_CFG_COULOMB_EFFICIENCY_0P1, \
    .timerPeriodUs           = SOC_CFG_TIMER_PERIOD_US, \
    .initSocPermille         = SOC_CFG_INIT_SOC_PERMILLE \
}

/**
 * @brief Check if parameter is within valid range
 */
#define SOC_CFG_IS_VALID_RANGE(value, min, max) \
    ((value) >= (min) && (value) <= (max))

/**
 * @brief Saturate value to range
 */
#define SOC_CFG_SATURATE(value, min, max) \
    ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

/*============================================================================*/
/* External Declarations                                                      */
/*============================================================================*/

/**
 * @brief SOC configuration instance
 */
extern const Soc_ConfigType g_SocConfig;

/**
 * @brief SOC runtime data instance
 */
extern Soc_RuntimeType g_SocRuntime;

#endif /* SOC_CFG_H */

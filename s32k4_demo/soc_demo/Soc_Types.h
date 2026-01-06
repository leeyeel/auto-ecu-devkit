/**
 * @file Soc_Types.h
 * @brief Type definitions for SOC estimation module
 * @module Soc_Types
 *
 * @assumptions & constraints
 *   - Target: NXP S32K4 series MCUs
 *   - FreeRTOS available for task scheduling
 *   - All types follow MISRA C:2012 guidelines
 *
 * @safety considerations
 *   - Fixed-width types ensure predictable sizing
 *   - No implicit signed/unsigned conversions
 *   - All pointers checked for NULL before use
 *
 * @version 1.0.0
 * @compliance MISRA C:2012, ISO 26262
 */

#ifndef SOC_TYPES_H
#define SOC_TYPES_H

#include <stdint.h>

/*============================================================================*/
/* Module Version                                                             */
/*============================================================================*/

#define SOC_MODULE_VERSION_MAJOR    (1U)
#define SOC_MODULE_VERSION_MINOR    (0U)
#define SOC_MODULE_VERSION_PATCH    (0U)

/*============================================================================*/
/* Error Codes                                                                */
/*============================================================================*/

/**
 * @brief SOC estimation status codes
 *
 * @note All error codes are negative values for error detection
 */
typedef enum
{
    SOC_STATUS_OK               = 0,   /**< Operation successful */
    SOC_STATUS_ERROR            = -1,  /**< Generic error */
    SOC_STATUS_NULL_PTR         = -2,  /**< NULL pointer argument */
    SOC_STATUS_INVALID_PARAM    = -3,  /**< Invalid parameter value */
    SOC_STATUS_NOT_INITIALIZED  = -4,  /**< Module not initialized */
    SOC_STATUS_OVERFLOW         = -5,  /**< Calculation overflow */
    SOC_STATUS_UNDERFLOW        = -6,  /**< Calculation underflow */
    SOC_STATUS_SATURATED        = -7,  /**< Value saturated at limit */
    SOC_STATUS_INVALID_STATE    = -8   /**< Invalid state detected */
} Soc_StatusType;

/**
 * @brief Battery charging/discharging state
 */
typedef enum
{
    SOC_DIRECTION_DISCHARGE     = 0,   /**< Battery is discharging */
    SOC_DIRECTION_CHARGE        = 1,   /**< Battery is charging */
    SOC_DIRECTION_IDLE          = 2    /**< No current flow */
} Soc_DirectionType;

/**
 * @brief Battery operational mode
 */
typedef enum
{
    SOC_MODE_NORMAL             = 0,   /**< Normal operation */
    SOC_MODE_DEGRADED           = 1,   /**< Degraded mode (sensor fault) */
    SOC_MODE_SLEEP              = 2,   /**< Low power/sleep mode */
    SOC_MODE_FAULT              = 3    /**< Fault state */
} Soc_OperatingModeType;

/*============================================================================*/
/* Data Structures                                                            */
/*============================================================================*/

/**
 * @brief Configuration parameters for SOC estimation
 *
 * @safety-critical
 *   - Values must be calibrated for specific battery pack
 *   - Incorrect values lead to inaccurate SOC display
 */
typedef struct
{
    uint32_t    batteryCapacityAh;     /**< Total battery capacity in Ampere-hours */
    uint32_t    nominalVoltage_mV;     /**< Nominal battery voltage in millivolts */
    uint32_t    minVoltage_mV;         /**< Minimum safe voltage in millivolts */
    uint32_t    maxVoltage_mV;         /**< Maximum safe voltage in millivolts */
    uint16_t    coulombEfficiency_0p1; /**< Coulomb efficiency in 0.1% units (1000 = 100%) */
    uint32_t    timerPeriodUs;         /**< Timer period in microseconds (5000 for 5ms) */
    uint16_t    initSocPermille;       /**< Initial SOC in per-mille (1000 = 100%) */
} Soc_ConfigType;

/**
 * @brief Runtime data for SOC estimation
 *
 * @safety-critical
 *   - Shared between timer ISR and task context
 *   - Requires atomic access or critical section protection
 */
typedef struct
{
    /** @brief Accumulated charge in microampere-seconds (uAs) */
    int64_t     accumulatedCharge_uAs;

    /** @brief Current SOC in per-mille (0-1000, representing 0-100%) */
    uint16_t    socPermille;

    /** @brief Last valid SOC in per-mille (for plausibility check) */
    uint16_t    lastSocPermille;

    /** @brief Current measurement in milliamperes (signed) */
    int32_t     current_mA;

    /** @brief Current direction (charge/discharge/idle) */
    Soc_DirectionType   direction;

    /** @brief Timer tick counter for rate limiting */
    uint32_t    tickCounter;

    /** @brief Module initialization state */
    bool        initialized;

    /** @brief Data validity flag */
    bool        dataValid;

    /** @brief Operating mode */
    Soc_OperatingModeType   mode;
} Soc_RuntimeType;

/**
 * @brief Input data for SOC update
 *
 * @note All values must be validated before use
 */
typedef struct
{
    int32_t     current_mA;        /**< Measured current in mA (signed) */
    uint16_t    voltage_mV;        /**< Measured voltage in mV */
    uint32_t    timestampTick;     /**< Timestamp in timer ticks */
} Soc_InputDataType;

/**
 * @brief Output data from SOC estimation
 */
typedef struct
{
    uint16_t    socPermille;       /**< Estimated SOC in per-mille (0-1000) */
    int32_t     remainingCapacity_mAh; /**< Remaining capacity in mAh */
    Soc_DirectionType   direction; /**< Current flow direction */
    Soc_OperatingModeType   mode;   /**< Current operating mode */
    bool        dataValid;         /**< Data validity flag */
    Soc_StatusType status;         /**< Last operation status */
} Soc_OutputDataType;

/**
 * @brief Timer driver interface
 *
 * @note Function pointers for timer abstraction
 */
typedef struct
{
    /**
     * @brief Initialize timer
     * @return SOC_STATUS_OK on success
     */
    Soc_StatusType (*Init)(uint32_t periodUs);

    /**
     * @brief Start timer
     * @return SOC_STATUS_OK on success
     */
    Soc_StatusType (*Start)(void);

    /**
     * @brief Stop timer
     * @return SOC_STATUS_OK on success
     */
    Soc_StatusType (*Stop)(void);

    /**
     * @brief Get current tick count
     * @return Current timer tick value
     */
    uint32_t (*GetTick)(void);

    /**
     * @brief Get timer frequency in Hz
     * @return Timer frequency
     */
    uint32_t (*GetFreqHz)(void);
} Soc_TimerInterfaceType;

#endif /* SOC_TYPES_H */

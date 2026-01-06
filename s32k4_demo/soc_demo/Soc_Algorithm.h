/**
 * @file Soc_Algorithm.h
 * @brief Coulomb Counting Algorithm for SOC estimation
 * @module Soc_Algorithm
 *
 * @assumptions & constraints
 *   - Timer provides regular 5ms interrupts
 *   - Current measurement available at each tick
 *   - Battery parameters configured in Soc_Cfg.h
 *
 * @safety considerations
 *   - Coulomb counting accumulates error over time
 *   - Periodic re-calibration may be required
 *   - OCV (Open Circuit Voltage) fusion recommended for long-term accuracy
 *
 * @version 1.0.0
 * @compliance MISRA C:2012, ISO 26262
 */

#ifndef SOC_ALGORITHM_H
#define SOC_ALGORITHM_H

#include "Soc_Types.h"

/*============================================================================*/
/* Algorithm Constants                                                       */
/*============================================================================*/

/**
 * @brief Conversion factor: milliseconds to seconds
 */
#define SOC_ALG_MS_TO_S               (0.001F)

/**
 * @brief Conversion factor: per-mille to percent
 */
#define SOC_ALG_PERMILLE_TO_PERCENT   (0.1F)

/**
 * @brief Per-mille scale factor (1000 = 100%)
 */
#define SOC_ALG_SCALE_PERMILLE        (1000U)

/**
 * @brief Minimum SOC value (0%)
 */
#define SOC_ALG_MIN_SOC_PERMILLE      (0U)

/**
 * @brief Maximum SOC value (100%)
 */
#define SOC_ALG_MAX_SOC_PERMILLE      (1000U)

/*============================================================================*/
/* Algorithm Interface                                                       */
/*============================================================================*/

/**
 * @brief SOC algorithm interface
 */
typedef struct
{
    /**
     * @brief Initialize algorithm
     * @return SOC_STATUS_OK on success
     */
    Soc_StatusType (*Init)(uint16_t initSocPermille);

    /**
     * @brief Update SOC with new measurement
     * @param current_mA Current in mA (positive = charging)
     * @param deltaTimeUs Time elapsed in microseconds
     * @return Updated SOC status
     */
    Soc_StatusType (*Update)(int32_t current_mA, uint32_t deltaTimeUs);

    /**
     * @brief Get current SOC
     * @return Current SOC in per-mille
     */
    uint16_t (*GetSoc)(void);

    /**
     * @brief Reset algorithm
     * @param initSocPermille Initial SOC value
     * @return SOC_STATUS_OK on success
     */
    Soc_StatusType (*Reset)(uint16_t initSocPermille);

    /**
     * @brief Get accumulated charge
     * @return Accumulated charge in uAs
     */
    int64_t (*GetAccumulatedCharge)(void);
} Soc_AlgorithmInterfaceType;

/*============================================================================*/
/* API Functions                                                             */
/*============================================================================*/

/**
 * @brief Initialize Coulomb Counting algorithm
 *
 * @description Initializes internal state for SOC estimation.
 *              Sets initial SOC value and resets accumulated charge.
 *
 * @param initSocPermille Initial SOC in per-mille (0-1000)
 *
 * @return Soc_StatusType
 *         - SOC_STATUS_OK: Initialization successful
 *         - SOC_STATUS_INVALID_PARAM: Invalid initial SOC
 *
 * @pre  g_SocConfig must be valid
 * @post Algorithm ready for updates
 *
 * @safety Initial SOC must be reasonable for current battery state
 */
Soc_StatusType Soc_Algorithm_Init(uint16_t initSocPermille);

/**
 * @brief Update SOC using Coulomb Counting
 *
 * @description Integrates current over time to update SOC.
 *              Uses coulomb efficiency for charge compensation.
 *
 *              Q(t+dt) = Q(t) + I * dt
 *              SOC(t+dt) = SOC(t) + (Q(t+dt) / Q_capacity) * 1000
 *
 *              During charging, efficiency < 100% is applied:
 *              Q_charge_effective = Q_charge * efficiency
 *
 * @param current_mA Measured current in mA
 *                   - Positive: Battery charging
 *                   - Negative: Battery discharging
 * @param deltaTimeUs Time elapsed since last update in microseconds
 *
 * @return Soc_StatusType
 *         - SOC_STATUS_OK: Update successful
 *         - SOC_STATUS_OVERFLOW: Charge accumulator overflow
 *         - SOC_STATUS_INVALID_PARAM: Invalid current or time
 *
 * @pre  Algorithm initialized
 * @post SOC updated, accumulated charge modified
 *
 * @safety-critical
 *   - Current sign determines charge/discharge direction
 *   - Efficiency factor prevents SOC drift during cycling
 */
Soc_StatusType Soc_Algorithm_Update(int32_t current_mA, uint32_t deltaTimeUs);

/**
 * @brief Update SOC with voltage fusion (optional)
 *
 * @description Combines Coulomb Counting with OCV-based SOC estimation.
 *              Uses weighted average to reduce long-term drift.
 *
 * @param current_mA Measured current in mA
 * @param voltage_mV Measured voltage in mV
 * @param deltaTimeUs Time elapsed in microseconds
 * @param ocvWeight OCV weight in per-mille (0-1000)
 *
 * @return Soc_StatusType
 *         - SOC_STATUS_OK: Update successful
 *         - SOC_STATUS_ERROR: OCV-SOC mapping failed
 */
Soc_StatusType Soc_Algorithm_UpdateWithOcvFusion(
    int32_t current_mA,
    uint32_t voltage_mV,
    uint32_t deltaTimeUs,
    uint16_t ocvWeight
);

/**
 * @brief Get current SOC
 *
 * @return Current SOC in per-mille (0-1000)
 *
 * @pre  Algorithm initialized
 */
uint16_t Soc_Algorithm_GetSoc(void);

/**
 * @brief Get remaining capacity
 *
 * @return Remaining capacity in milliampere-hours
 */
int32_t Soc_Algorithm_GetRemainingCapacity_mAh(void);

/**
 * @brief Get accumulated charge
 *
 * @return Accumulated charge in microampere-seconds
 */
int64_t Soc_Algorithm_GetAccumulatedCharge(void);

/**
 * @brief Reset algorithm
 *
 * @description Resets state and sets new initial SOC.
 *
 * @param initSocPermille New initial SOC in per-mille
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Algorithm_Reset(uint16_t initSocPermille);

/**
 * @brief Get algorithm state
 *
 * @return Current direction (charge/discharge/idle)
 */
Soc_DirectionType Soc_Algorithm_GetDirection(void);

/**
 * @brief Get last update status
 *
 * @return Last update status code
 */
Soc_StatusType Soc_Algorithm_GetLastStatus(void);

/**
 * @brief Estimate SOC from voltage (OCV method)
 *
 * @description Simple OCV to SOC lookup.
 *              In production, use full OCV-SOC curve.
 *
 * @param voltage_mV Measured open circuit voltage in mV
 *
 * @return Estimated SOC in per-mille
 *
 * @note Only valid when current is near zero (OCV condition)
 */
uint16_t Soc_Algorithm_EstimateSocFromOcv(uint32_t voltage_mV);

/**
 * @brief Run self-test
 *
 * @description Checks algorithm internal consistency.
 *
 * @return SOC_STATUS_OK if self-test passed
 */
Soc_StatusType Soc_Algorithm_SelfTest(void);

#endif /* SOC_ALGORITHM_H */

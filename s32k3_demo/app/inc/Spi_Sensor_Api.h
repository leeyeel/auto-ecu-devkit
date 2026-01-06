/**
 * @file Spi_Sensor_Api.h
 * @brief High-level API for periodic SPI sensor reading
 * @module Spi_Sensor_Api
 *
 * @ assumptions & constraints
 *   - System timer provides millisecond resolution
 *   - Called from cyclic task (e.g., 1ms or 10ms base cycle)
 *   - Single sensor instance per API handle
 *
 * @ safety considerations
 *   - All operations use timeout protection
 *   - Data validity flags indicate data quality
 *   - Error counters support diagnostic monitoring
 *   - Safe state defined (last valid data with stale flag)
 *
 * @ execution context
 *   - Init/Deinit: Task context, non-reentrant
 *   - Cyclic functions: Task or cyclic ISR context
 *   - Get functions: Any context, reentrant
 *
 * @ verification checklist
 *   - [ ] Task scheduling constraints verified
 *   - [ ] Maximum execution time measured
 *   - [ ] Data consistency validated
 */

#ifndef SPI_SENSOR_API_H
#define SPI_SENSOR_API_H

#include "Spi_Sensor_Types.h"

/*============================================================================*/
/* Constants                                                                  */
/*============================================================================*/

/**
 * @brief Maximum number of periodic read channels
 *
 * @safety Reasoning:
 *   Fixed array size provides deterministic memory usage.
 *   Adjust based on application requirements.
 */
#define SPI_SENSOR_MAX_CHANNELS  (4U)

/**
 * @brief Default cyclic task period (milliseconds)
 */
#define SPI_SENSOR_DEFAULT_PERIOD_MS  (10U)

/**
 * @brief Maximum supported cyclic period
 */
#define SPI_SENSOR_MAX_PERIOD_MS  (1000U)

/*============================================================================*/
/* Channel Configuration                                                      */
/*============================================================================*/

/**
 * @brief Sensor channel configuration
 *
 * @description Defines a periodic read channel for a specific register.
 */
typedef struct {
    uint8_t registerAddress;           /**< Register to read periodically */
    uint32_t periodMs;                 /**< Read period in milliseconds */
    bool enabled;                      /**< Channel enable flag */
} Spi_Sensor_ChannelConfigType;

/**
 * @brief Module configuration structure
 *
 * @description Contains all configuration for the sensor API module.
 */
typedef struct {
    Spi_Sensor_ChannelConfigType channels[SPI_SENSOR_MAX_CHANNELS];
    uint8_t numberOfChannels;          /**< Number of configured channels */
} Spi_Sensor_ApiConfigType;

/*============================================================================*/
/* Initialization and Lifecycle                                               */
/*============================================================================*/

/**
 * @brief Initialize the SPI Sensor API module
 *
 * @description Initializes the sensor API with the provided configuration.
 *             This function must be called once at system startup.
 *
 * @param configPtr Pointer to configuration structure
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Initialization successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: NULL config pointer or invalid config
 *         - SPI_SENSOR_STATUS_ERROR: Initialization failed
 *
 * @pre  Spi_Sensor_Driver_Init must be called first
 * @post  API ready for operation, channels configured
 * @reentrant No
 * @blocking Yes, during driver init check
 *
 * @safety  Configuration validated before use
 * @safety  Initialization state tracked
 */
Spi_Sensor_StatusType Spi_Sensor_Api_Init(
    const Spi_Sensor_ApiConfigType* const configPtr
);

/**
 * @brief Deinitialize the SPI Sensor API module
 *
 * @description Shuts down the API and releases resources.
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Deinit successful
 *         - SPI_SENSOR_STATUS_NOT_INIT: API was not initialized
 *
 * @reentrant No
 */
Spi_Sensor_StatusType Spi_Sensor_Api_Deinit(void);

/*============================================================================*/
/* Cyclic Processing                                                          */
/*============================================================================*/

/**
 * @brief Cyclic processing function
 *
 * @description Main cyclic function that handles periodic sensor reads.
 *             Must be called periodically from the application's cyclic task.
 *
 * @param currentTimeMs Current system time in milliseconds
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Processing complete (or not needed)
 *         - SPI_SENSOR_STATUS_NOT_INIT: API not initialized
 *         - SPI_SENSOR_STATUS_ERROR: Internal error
 *
 * @pre  API initialized
 * @post  Enabled channels checked and triggered if due
 * @reentrant No
 * @blocking No (designed for cyclic context)
 *
 * @execution_time Typical: < 1ms, Max: < SPI_SENSOR_DEFAULT_TIMEOUT_MS
 *
 * @safety  Non-blocking design ensures timing guarantees
 * @safety  Errors logged, don't stop cyclic processing
 */
Spi_Sensor_StatusType Spi_Sensor_Api_Cyclic(uint32_t currentTimeMs);

/*============================================================================*/
/* Data Access API                                                            */
/*============================================================================*/

/**
 * @brief Get the latest reading from a channel
 *
 * @description Returns the most recent sensor reading for the specified channel.
 *
 * @param channelId   Channel identifier (0 to SPI_SENSOR_MAX_CHANNELS-1)
 * @param resultPtr   Pointer to store the read result
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Data retrieved successfully
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid channel or NULL pointer
 *         - SPI_SENSOR_STATUS_NOT_INIT: API not initialized
 *
 * @post  *resultPtr contains latest data and validity flags
 * @reentrant Yes
 * @safety  Result snapshot is atomic
 * @safety  Channel index validated
 *
 * @note Always check resultPtr->validity.dataValid before using data
 */
Spi_Sensor_StatusType Spi_Sensor_Api_GetReading(
    uint8_t channelId,
    Spi_Sensor_ReadResultType* const resultPtr
);

/**
 * @brief Get readings from all channels
 *
 * @description Returns all channel readings in a single call.
 *
 * @param resultArray   Array to store results
 * @param arraySize     Size of result array (must be >= numberOfChannels)
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: All readings retrieved
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid parameters
 *         - SPI_SENSOR_STATUS_NOT_INIT: API not initialized
 *
 * @pre  resultArray size >= numberOfChannels
 * @post  resultArray contains all channel results
 * @reentrant Yes
 * @safety  Array size validated
 */
Spi_Sensor_StatusType Spi_Sensor_Api_GetAllReadings(
    Spi_Sensor_ReadResultType* const resultArray,
    uint8_t arraySize
);

/*============================================================================*/
/* Channel Control                                                            */
/*============================================================================*/

/**
 * @brief Enable a periodic read channel
 *
 * @description Enables the specified channel for periodic reading.
 *
 * @param channelId Channel to enable (0 to SPI_SENSOR_MAX_CHANNELS-1)
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Channel enabled
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid channel ID
 *         - SPI_SENSOR_STATUS_NOT_INIT: API not initialized
 *
 * @reentrant No (per channel)
 * @safety  Channel index validated
 */
Spi_Sensor_StatusType Spi_Sensor_Api_EnableChannel(uint8_t channelId);

/**
 * @brief Disable a periodic read channel
 *
 * @description Disables the specified channel from periodic reading.
 *
 * @param channelId Channel to disable (0 to SPI_SENSOR_MAX_CHANNELS-1)
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Channel disabled
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid channel ID
 *         - SPI_SENSOR_STATUS_NOT_INIT: API not initialized
 *
 * @reentrant No (per channel)
 * @safety  Channel index validated
 */
Spi_Sensor_StatusType Spi_Sensor_Api_DisableChannel(uint8_t channelId);

/**
 * @brief Set channel read period
 *
 * @description Changes the read period for a specific channel.
 *
 * @param channelId Channel to configure
 * @param periodMs  New period in milliseconds
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Period updated
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid channel or period
 *         - SPI_SENSOR_STATUS_NOT_INIT: API not initialized
 *
 * @reentrant No (per channel)
 * @safety  Period range validated
 */
Spi_Sensor_StatusType Spi_Sensor_Api_SetPeriod(uint8_t channelId, uint32_t periodMs);

/*============================================================================*/
/* Diagnostic and Monitoring                                                  */
/*============================================================================*/

/**
 * @brief Get aggregated diagnostic counters
 *
 * @description Returns the sum of all error counters from all channels.
 *
 * @param countersPtr Pointer to store counter values
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Counters retrieved
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: NULL pointer
 *
 * @reentrant Yes
 * @safety  Counters atomically read
 */
Spi_Sensor_StatusType Spi_Sensor_Api_GetDiagCounters(
    Spi_Sensor_DiagCountersType* const countersPtr
);

/**
 * @brief Reset all diagnostic counters
 *
 * @description Clears all error counters in the API and driver.
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Counters reset
 *         - SPI_SENSOR_STATUS_NOT_INIT: API not initialized
 *
 * @reentrant No
 * @safety  Counter reset is atomic
 */
Spi_Sensor_StatusType Spi_Sensor_Api_ResetDiagCounters(void);

/**
 * @brief Get module initialization state
 *
 * @return true if API is initialized, false otherwise
 *
 * @reentrant Yes
 * @safety  Read-only operation
 */
bool Spi_Sensor_Api_IsInitialized(void);

/*============================================================================*/
/* Safe State and Degraded Mode                                               */
/*============================================================================*/

/**
 * @brief Force all channels to degraded mode
 *
 * @description Sets all data validity flags to indicate stale data.
 *             Used when system detects a condition that may affect data quality.
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Degraded mode activated
 *         - SPI_SENSOR_STATUS_NOT_INIT: API not initialized
 *
 * @reentrant No
 *
 * @safety  Prevents use of potentially invalid data
 * @safety  Data remains available but marked stale
 */
Spi_Sensor_StatusType Spi_Sensor_Api_SetDegradedMode(void);

/**
 * @brief Exit degraded mode (normal operation)
 *
 * @description Clears stale flags. Data will be updated on next cyclic read.
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Degraded mode exited
 *         - SPI_SENSOR_STATUS_NOT_INIT: API not initialized
 *
 * @reentrant No
 */
Spi_Sensor_StatusType Spi_Sensor_Api_ExitDegradedMode(void);

#endif /* SPI_SENSOR_API_H */

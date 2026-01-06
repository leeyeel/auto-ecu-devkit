/**
 * @file Spi_Sensor_Driver.h
 * @brief SPI Sensor Driver Layer - provides sensor-specific operations
 * @module Spi_Sensor_Driver
 *
 * @ assumptions & constraints
 *   - Compatible with standard register-based SPI sensors
 *   - 8-bit register addressing
 *   - Single-byte data width per register
 *
 * @ safety considerations
 *   - Input validation prevents invalid register access
 *   - CRC validation when supported
 *   - Plausibility checks on read data
 *   - Timeout protection on all operations
 *
 * @ execution context
 *   - Init/Deinit: Task context only
 *   - Read/Write: Task context (blocking)
 *
 * @ verification checklist
 *   - [ ] All pointers validated before use
 *   - [ ] Register address range checked
 *   - [ ] Data validity flags properly set
 */

#ifndef SPI_SENSOR_DRIVER_H
#define SPI_SENSOR_DRIVER_H

#include "Spi_Sensor_Types.h"

/*============================================================================*/
/* Constants                                                                  */
/*============================================================================*/

/**
 * @brief Default communication timeout (milliseconds)
 *
 * @safety Reasoning:
 *   Timeout prevents indefinite blocking in case of HW failure.
 *   Value chosen based on typical SPI sensor response time plus safety margin.
 */
#define SPI_SENSOR_DEFAULT_TIMEOUT_MS  (10U)

/**
 * @brief Maximum allowed register address
 *
 * @safety Used for range validation
 */
#define SPI_SENSOR_MAX_REGISTER_ADDR   (0x7FU)

/**
 * @brief Dummy value for invalid/unreadable registers
 */
#define SPI_SENSOR_INVALID_REGISTER_VALUE  (0xFFU)

/*============================================================================*/
/* Initialization and Configuration                                           */
/*============================================================================*/

/**
 * @brief Initialize the SPI sensor driver
 *
 * @description Initializes the sensor driver layer and underlying HAL.
 *             Must be called before any sensor operation.
 *
 * @param config Pointer to sensor configuration
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Initialization successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: NULL config pointer
 *         - SPI_SENSOR_STATUS_ERROR: Internal error
 *
 * @pre  Spi_Sensor_Hal_Init must be called first
 * @post  Sensor driver ready for operations
 * @reentrant No
 * @safety  Module tracks initialization state
 */
Spi_Sensor_StatusType Spi_Sensor_Driver_Init(const Spi_Sensor_ConfigType* const config);

/**
 * @brief Deinitialize the SPI sensor driver
 *
 * @description Resets the driver state and deinitializes the HAL.
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Deinit successful
 *         - SPI_SENSOR_STATUS_NOT_INIT: Driver was not initialized
 *
 * @reentrant No
 */
Spi_Sensor_StatusType Spi_Sensor_Driver_Deinit(void);

/*============================================================================*/
/* Register Access API                                                        */
/*============================================================================*/

/**
 * @brief Read a single register from the sensor
 *
 * @description Performs SPI transaction to read one 8-bit register.
 *
 * @param registerAddr Register address to read
 * @param dataPtr      Pointer to store read data
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Read successful, data valid
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: NULL dataPtr or invalid address
 *         - SPI_SENSOR_STATUS_TIMEOUT: Communication timeout
 *         - SPI_SENSOR_STATUS_NOT_INIT: Driver not initialized
 *         - SPI_SENSOR_STATUS_HW_ERROR: Hardware error
 *
 * @pre  Driver initialized
 * @post  *dataPtr contains register value (if OK)
 * @reentrant No
 * @blocking Yes, up to SPI_SENSOR_DEFAULT_TIMEOUT_MS
 *
 * @safety  Pointer checked before dereference
 * @safety  Register address validated
 */
Spi_Sensor_StatusType Spi_Sensor_Driver_ReadRegister(
    uint8_t registerAddr,
    uint8_t* const dataPtr
);

/**
 * @brief Write a single register to the sensor
 *
 * @description Performs SPI transaction to write one 8-bit register.
 *
 * @param registerAddr Register address to write
 * @param data         Data to write
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Write successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid register address
 *         - SPI_SENSOR_STATUS_TIMEOUT: Communication timeout
 *         - SPI_SENSOR_STATUS_NOT_INIT: Driver not initialized
 *         - SPI_SENSOR_STATUS_HW_ERROR: Hardware error
 *
 * @pre  Driver initialized
 * @post  Register updated (if OK)
 * @reentrant No
 * @blocking Yes, up to SPI_SENSOR_DEFAULT_TIMEOUT_MS
 *
 * @safety  Register address validated
 */
Spi_Sensor_StatusType Spi_Sensor_Driver_WriteRegister(
    uint8_t registerAddr,
    uint8_t data
);

/**
 * @brief Read multiple consecutive registers
 *
 * @description Reads a block of registers starting from the specified address.
 *             Many sensors support auto-increment for this operation.
 *
 * @param startAddr Starting register address
 * @param buffer    Buffer to store read data
 * @param length    Number of registers to read
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Read successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid parameters
 *         - SPI_SENSOR_STATUS_TIMEOUT: Communication timeout
 *         - SPI_SENSOR_STATUS_NOT_INIT: Driver not initialized
 *         - SPI_SENSOR_STATUS_HW_ERROR: Hardware error
 *
 * @pre  Driver initialized
 * @pre  buffer has space for at least 'length' bytes
 * @post  buffer contains register values (if OK)
 * @reentrant No
 * @blocking Yes, timeout scales with length
 *
 * @safety  Length checked against maximum transfer size
 * @safety  Buffer pointer validated
 */
Spi_Sensor_StatusType Spi_Sensor_Driver_ReadRegisterBlock(
    uint8_t startAddr,
    uint8_t* const buffer,
    uint16_t length
);

/**
 * @brief Write multiple consecutive registers
 *
 * @description Writes a block of registers starting from the specified address.
 *
 * @param startAddr Starting register address
 * @param buffer    Buffer containing data to write
 * @param length    Number of registers to write
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Write successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid parameters
 *         - SPI_SENSOR_STATUS_TIMEOUT: Communication timeout
 *         - SPI_SENSOR_STATUS_NOT_INIT: Driver not initialized
 *         - SPI_SENSOR_STATUS_HW_ERROR: Hardware error
 *
 * @pre  Driver initialized
 * @post  Registers updated (if OK)
 * @reentrant No
 * @blocking Yes, timeout scales with length
 *
 * @safety  Length checked against maximum transfer size
 * @safety  Buffer pointer validated (read-only access)
 */
Spi_Sensor_StatusType Spi_Sensor_Driver_WriteRegisterBlock(
    uint8_t startAddr,
    const uint8_t* const buffer,
    uint16_t length
);

/*============================================================================*/
/* Device Status and Diagnostics                                              */
/*============================================================================*/

/**
 * @brief Verify sensor communication (ping)
 *
 * @description Performs a simple read operation to verify communication.
 *             Useful for initialization checks.
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Communication OK
 *         - SPI_SENSOR_STATUS_TIMEOUT: No response
 *         - SPI_SENSOR_STATUS_NOT_INIT: Driver not initialized
 *         - SPI_SENSOR_STATUS_ERROR: Other error
 *
 * @reentrant No
 * @safety  Non-destructive read operation
 */
Spi_Sensor_StatusType Spi_Sensor_Driver_VerifyCommunication(void);

/**
 * @brief Get driver initialization state
 *
 * @return true if driver is initialized, false otherwise
 *
 * @reentrant Yes
 * @safety  Read-only operation
 */
bool Spi_Sensor_Driver_IsInitialized(void);

/**
 * @brief Get diagnostic error counters
 *
 * @description Returns a copy of the diagnostic counters for monitoring.
 *
 * @param countersPtr Pointer to store counter values
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Counters retrieved
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: NULL pointer
 *
 * @reentrant Yes
 * @safety  Counters atomically read (snapshot)
 */
Spi_Sensor_StatusType Spi_Sensor_Driver_GetDiagCounters(
    Spi_Sensor_DiagCountersType* const countersPtr
);

/**
 * @brief Reset diagnostic error counters
 *
 * @description Clears all diagnostic counters.
 *
 * @reentrant No
 * @safety  Counters reset atomically
 */
void Spi_Sensor_Driver_ResetDiagCounters(void);

#endif /* SPI_SENSOR_DRIVER_H */

/**
 * @file Spi_Sensor_Hal.h
 * @brief Hardware Abstraction Layer for SPI communication (S32K3)
 * @module Spi_Sensor_Hal
 *
 * @ assumptions & constraints
 *   - S32K3 SDK (RTD or Real-Time Drivers) is available
 *   - Clock configuration already done by system init
 *   - Pins configured via SIUL2 prior to SPI init
 *
 * @ safety considerations
 *   - Hardware register access protected
 *   - Timeout mechanism prevents infinite waits
 *   - Error codes propagate to application layer
 *
 * @ execution context
 *   - Init function: Task context only
 *   - Transfer functions: Task context (blocking) or ISR (non-blocking)
 *
 * @ verification checklist
 *   - [ ] No unbounded wait loops
 *   - [ ] All parameters validated
 *   - [ ] Return values never ignored
 */

#ifndef SPI_SENSOR_HAL_H
#define SPI_SENSOR_HAL_H

#include "Spi_Sensor_Types.h"

/*============================================================================*/
/* API Functions                                                              */
/*============================================================================*/

/**
 * @brief Initialize the SPI HAL layer
 *
 * @description Configures the SPI peripheral based on provided parameters.
 *             Must be called before any transfer operation.
 *
 * @param config Pointer to configuration structure
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Initialization successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: NULL config pointer
 *         - SPI_SENSOR_STATUS_HW_ERROR: Hardware initialization failed
 *
 * @pre  System clocks configured
 * @post  SPI peripheral ready for transfers
 * @reentrant No
 * @safety  Must be called once before use
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_Init(const Spi_Sensor_ConfigType* const config);

/**
 * @brief Deinitialize the SPI HAL layer
 *
 * @description Disables the SPI peripheral and resets hardware state.
 *
 * @param instance SPI instance to deinitialize
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Deinit successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid instance
 *
 * @pre  SPI was initialized
 * @post  SPI peripheral disabled
 * @reentrant No
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_Deinit(Spi_Sensor_InstanceType instance);

/**
 * @brief Perform blocking SPI transfer
 *
 * @description Transmits and receives data on the SPI bus.
 *             Function blocks until transfer completes or timeout expires.
 *
 * @param instance     SPI instance to use
 * @param txBuffer     Transmit data buffer (can be NULL for dummy writes)
 * @param rxBuffer     Receive data buffer (can be NULL if data not needed)
 * @param length       Number of bytes to transfer
 * @param timeoutMs    Timeout in milliseconds
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Transfer successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid parameters
 *         - SPI_SENSOR_STATUS_TIMEOUT: Transfer timed out
 *         - SPI_SENSOR_STATUS_HW_ERROR: Hardware error
 *
 * @pre  SPI initialized with Spi_Sensor_Hal_Init
 * @pre  CS controlled externally (or use auto-CS mode)
 * @post  Data transferred, rxBuffer contains received data
 * @reentrant No (per instance)
 * @safety  Timeout prevents indefinite blocking
 *
 * @note Both buffers must have at least 'length' bytes if non-NULL
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_TransferBlocking(
    Spi_Sensor_InstanceType instance,
    const uint8_t* const txBuffer,
    uint8_t* const rxBuffer,
    uint16_t length,
    uint32_t timeoutMs
);

/**
 * @brief Assert chip select signal
 *
 * @description Activates the CS pin for the specified device.
 *
 * @param instance SPI instance
 * @param csPin    Chip select pin
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: CS asserted
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid parameters
 *
 * @safety  Pin state change is atomic
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_AssertCs(
    Spi_Sensor_InstanceType instance,
    Spi_Sensor_CsType csPin
);

/**
 * @brief Deassert chip select signal
 *
 * @description Deactivates the CS pin for the specified device.
 *
 * @param instance SPI instance
 * @param csPin    Chip select pin
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: CS deasserted
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid parameters
 *
 * @safety  Pin state change is atomic
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_DeassertCs(
    Spi_Sensor_InstanceType instance,
    Spi_Sensor_CsType csPin
);

/**
 * @brief Get HAL initialization state
 *
 * @description Returns whether the specified SPI instance has been initialized.
 *
 * @param instance SPI instance to query
 *
 * @return true if initialized, false otherwise
 *
 * @reentrant Yes
 * @safety  Read-only operation, stateless
 */
bool Spi_Sensor_Hal_IsInitialized(Spi_Sensor_InstanceType instance);

/**
 * @brief Calculate timeout in system ticks
 *
 * @description Converts millisecond timeout to system-specific ticks.
 *             Abstraction for portability across different RTOS configurations.
 *
 * @param timeoutMs Timeout in milliseconds
 *
 * @return Timeout value in system ticks
 *
 * @reentrant Yes
 * @safety  Pure function, no side effects
 */
uint32_t Spi_Sensor_Hal_CalcTimeoutTicks(uint32_t timeoutMs);

#endif /* SPI_SENSOR_HAL_H */

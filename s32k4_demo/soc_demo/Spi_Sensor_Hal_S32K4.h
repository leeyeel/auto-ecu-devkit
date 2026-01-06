/**
 * @file Spi_Sensor_Hal_S32K4.h
 * @brief Hardware Abstraction Layer for SPI communication (S32K4)
 * @module Spi_Sensor_Hal_S32K4
 *
 * @ assumptions & constraints
 *   - S32K4 SDK (RTD) is available
 *   - Clock configuration already done by system init (CCGE - Clock Control)
 *   - Pins configured via SIUL2 prior to SPI init
 *   - Target: S32K4xx series MCUs
 *
 * @ safety considerations
 *   - Hardware register access protected
 *   - Timeout mechanism prevents infinite waits
 *   - Error codes propagate to application layer
 *   - No dynamic memory allocation
 *
 * @ execution context
 *   - Init function: Task context only
 *   - Transfer functions: Task context (blocking)
 *
 * @ verification checklist
 *   - [ ] No unbounded wait loops
 *   - [ ] All parameters validated
 *   - [ ] Return values never ignored
 *   - [ ] Static analysis passes (MISRA C:2012)
 */

#ifndef SPI_SENSOR_HAL_S32K4_H
#define SPI_SENSOR_HAL_S32K4_H

#include "Spi_Sensor_Types.h"

/*============================================================================*/
/* API Functions                                                              */
/*============================================================================*/

/**
 * @brief Initialize the SPI HAL layer for S32K4
 *
 * @description Configures the LPSPI peripheral based on provided parameters.
 *              S32K4 has 4 LPSPI instances (LPSPI0, LPSPI1, LPSPI2, LPSPI3).
 *
 * @param config Pointer to configuration structure
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Initialization successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: NULL config pointer
 *         - SPI_SENSOR_STATUS_HW_ERROR: Hardware initialization failed
 *
 * @pre  System clocks configured (CCGE module)
 * @pre  SIUL2 pin muxing configured
 * @post LPSPI peripheral ready for transfers
 * @reentrant No
 * @safety Must be called once before use
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_S32K4_Init(const Spi_Sensor_ConfigType* const config);

/**
 * @brief Deinitialize the SPI HAL layer for S32K4
 *
 * @description Disables the LPSPI peripheral and resets hardware state.
 *
 * @param instance SPI instance to deinitialize
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Deinit successful
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid instance
 *
 * @pre  SPI was initialized
 * @post LPSPI peripheral disabled
 * @reentrant No
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_S32K4_Deinit(Spi_Sensor_InstanceType instance);

/**
 * @brief Perform blocking SPI transfer on S32K4
 *
 * @description Transmits and receives data on the LPSPI bus.
 *              Function blocks until transfer completes or timeout expires.
 *
 * @param instance     SPI instance to use (0-3 for S32K4)
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
 * @pre  SPI initialized with Spi_Sensor_Hal_S32K4_Init
 * @post Data transferred, rxBuffer contains received data
 * @reentrant No (per instance)
 * @safety Timeout prevents indefinite blocking
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_S32K4_TransferBlocking(
    Spi_Sensor_InstanceType instance,
    const uint8_t* const txBuffer,
    uint8_t* const rxBuffer,
    uint16_t length,
    uint32_t timeoutMs
);

/**
 * @brief Read a register from SPI sensor
 *
 * @description Performs a register read operation following typical SPI sensor
 *              protocol: [CMD + Address] -> [Dummy/Data].
 *
 * @param instance     SPI instance to use
 * @param regAddress   Register address to read
 * @param rxBuffer     Buffer to receive data
 * @param dataLength   Number of data bytes to read
 * @param timeoutMs    Timeout in milliseconds
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Read successful
 *         - SPI_SENSOR_STATUS_TIMEOUT: Transfer timed out
 *         - SPI_SENSOR_STATUS_HW_ERROR: Hardware error
 *
 * @pre  SPI initialized
 * @post rxBuffer contains register value
 * @safety Timeout and error handling ensure safe operation
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_S32K4_ReadRegister(
    Spi_Sensor_InstanceType instance,
    uint8_t regAddress,
    uint8_t* const rxBuffer,
    uint16_t dataLength,
    uint32_t timeoutMs
);

/**
 * @brief Write a register to SPI sensor
 *
 * @description Performs a register write operation following typical SPI sensor
 *              protocol: [CMD + Address + Data].
 *
 * @param instance     SPI instance to use
 * @param regAddress   Register address to write
 * @param txBuffer     Data to transmit
 * @param dataLength   Number of data bytes to write
 * @param timeoutMs    Timeout in milliseconds
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: Write successful
 *         - SPI_SENSOR_STATUS_TIMEOUT: Transfer timed out
 *         - SPI_SENSOR_STATUS_HW_ERROR: Hardware error
 *
 * @pre  SPI initialized
 * @safety Timeout and error handling ensure safe operation
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_S32K4_WriteRegister(
    Spi_Sensor_InstanceType instance,
    uint8_t regAddress,
    const uint8_t* const txBuffer,
    uint16_t dataLength,
    uint32_t timeoutMs
);

/**
 * @brief Assert chip select signal
 *
 * @description Activates the CS pin for the specified device.
 *
 * @param instance SPI instance
 * @param csPin    Chip select pin (0-3 per LPSPI)
 *
 * @return Spi_Sensor_StatusType
 *         - SPI_SENSOR_STATUS_OK: CS asserted
 *         - SPI_SENSOR_STATUS_INVALID_PARAM: Invalid parameters
 *
 * @safety Pin state change is atomic
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_S32K4_AssertCs(
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
 * @safety Pin state change is atomic
 */
Spi_Sensor_StatusType Spi_Sensor_Hal_S32K4_DeassertCs(
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
 * @safety Read-only operation, stateless
 */
bool Spi_Sensor_Hal_S32K4_IsInitialized(Spi_Sensor_InstanceType instance);

#endif /* SPI_SENSOR_HAL_S32K4_H */

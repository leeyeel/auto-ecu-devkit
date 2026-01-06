/**
 * @file Spi_Sensor_Cfg.h
 * @brief Configuration header for SPI Sensor module
 *
 * @description This file contains all compile-time configuration parameters
 *              for the SPI Sensor module. Adjust these values based on your
 *              hardware and application requirements.
 *
 * @ assumptions & constraints
 *   - Configuration values are known at compile time
 *   - Values match target hardware specifications
 */

#ifndef SPI_SENSOR_CFG_H
#define SPI_SENSOR_CFG_H

#include "Spi_Sensor_Types.h"

/*============================================================================*/
/* Hardware Configuration                                                      */
/*============================================================================*/

/**
 * @brief SPI instance to use
 *
 * @note S32K3 has 3 LPSPI instances (LPSPI0, LPSPI1, LPSPI2)
 *       Select based on your hardware design.
 */
#define SPI_SENSOR_CFG_INSTANCE  SPI_SENSOR_INSTANCE_0

/**
 * @brief Chip select pin to use
 *
 * @note Configure based on PCB connection
 */
#define SPI_SENSOR_CFG_CS_PIN    SPI_SENSOR_CS_0

/**
 * @brief SPI clock speed (baudrate)
 *
 * @note Select based on sensor maximum supported frequency
 *       and signal integrity requirements.
 */
#define SPI_SENSOR_CFG_BAUDRATE  SPI_SENSOR_BAUDRATE_1MHZ

/**
 * @brief SPI clock polarity
 *
 * @note CPOL = 0: Clock idle low
 *       CPOL = 1: Clock idle high
 */
#define SPI_SENSOR_CFG_CPOL      SPI_SENSOR_CLOCK_POLARITY_0

/**
 * @brief SPI clock phase
 *
 * @note CPHA = 0: Data captured on first edge
 *       CPHA = 1: Data captured on second edge
 */
#define SPI_SENSOR_CFG_CPHA      SPI_SENSOR_CLOCK_PHASE_0

/**
 * @brief CS setup delay (nanoseconds)
 *
 * @description Delay between CS assertion and first clock edge.
 */
#define SPI_SENSOR_CFG_CS_TO_CLK_DELAY_NS  (1000U)

/**
 * @brief CS hold delay (nanoseconds)
 *
 * @description Delay between last clock edge and CS deassertion.
 */
#define SPI_SENSOR_CFG_CLK_TO_CS_DELAY_NS  (1000U)

/**
 * @brief Inter-transfer delay (nanoseconds)
 *
 * @description Minimum delay between consecutive transfers.
 */
#define SPI_SENSOR_CFG_INTER_TRANSFER_DELAY_NS  (2000U)

/*============================================================================*/
/* Sensor-Specific Configuration                                              */
/*============================================================================*/

/**
 * @brief SPI Read command byte
 *
 * @note Adjust to match your sensor's protocol
 *       Common values: 0x03 (Standard Read), 0x0B (Fast Read)
 */
#define SPI_SENSOR_CFG_CMD_READ  (0x03U)

/**
 * @brief SPI Write command byte
 *
 * @note Adjust to match your sensor's protocol
 *       Common value: 0x02 (Standard Write)
 */
#define SPI_SENSOR_CFG_CMD_WRITE (0x02U)

/**
 * @brief WHO_AM_I register address
 *
 * @description Register containing device ID for communication verification.
 */
#define SPI_SENSOR_CFG_WHO_AM_I_ADDR  (0x0FU)

/**
 * @brief Expected WHO_AM_I value
 *
 * @description Expected device ID. Used for verification during init.
 */
#define SPI_SENSOR_CFG_WHO_AM_I_EXPECTED  (0xC5U)

/*============================================================================*/
/* API Configuration                                                          */
/*============================================================================*/

/**
 * @brief Default number of periodic read channels
 *
 * @note Can be 1 to SPI_SENSOR_MAX_CHANNELS
 */
#define SPI_SENSOR_CFG_NUM_CHANNELS  (3U)

/**
 * @brief Channel 1 configuration
 */
#define SPI_SENSOR_CFG_CH1_REGISTER_ADDR  (0x00U)
#define SPI_SENSOR_CFG_CH1_PERIOD_MS      (10U)
#define SPI_SENSOR_CFG_CH1_ENABLED        (true)

/**
 * @brief Channel 2 configuration
 */
#define SPI_SENSOR_CFG_CH2_REGISTER_ADDR  (0x01U)
#define SPI_SENSOR_CFG_CH2_PERIOD_MS      (20U)
#define SPI_SENSOR_CFG_CH2_ENABLED        (true)

/**
 * @brief Channel 3 configuration
 */
#define SPI_SENSOR_CFG_CH3_REGISTER_ADDR  (0x02U)
#define SPI_SENSOR_CFG_CH3_PERIOD_MS      (50U)
#define SPI_SENSOR_CFG_CH3_ENABLED        (true)

/*============================================================================*/
/* Timing Configuration                                                       */
/*============================================================================*/

/**
 * @brief Default communication timeout (milliseconds)
 *
 * @safety Must be long enough for slowest sensor response,
 *         but short enough for timely error detection.
 */
#define SPI_SENSOR_CFG_DEFAULT_TIMEOUT_MS  (10U)

/**
 * @brief Maximum retry attempts for SPI transfers
 *
 * @safety Limited retry prevents infinite loops on persistent failure.
 */
#define SPI_SENSOR_CFG_MAX_RETRY_ATTEMPTS (3U)

/**
 * @brief Delay between retry attempts (milliseconds)
 */
#define SPI_SENSOR_CFG_RETRY_DELAY_MS     (1U)

/*============================================================================*/
/* Diagnostic Configuration                                                   */
/*============================================================================*/

/**
 * @brief Data staleness threshold (milliseconds)
 *
 * @description Data older than this is marked stale.
 * @safety Must be > maximum channel period.
 */
#define SPI_SENSOR_CFG_STALENESS_THRESHOLD_MS  (1000U)

/**
 * @brief Maximum consecutive errors before degraded mode
 *
 * @safety Enters degraded mode after this many consecutive failures.
 */
#define SPI_SENSOR_CFG_MAX_CONSECUTIVE_ERRORS  (10U)

/*============================================================================*/
/* Safety Configuration                                                       */
/*============================================================================*/

/**
 * @brief Enable CRC checking (if supported by sensor)
 *
 * @note Set to true if your sensor supports CRC on SPI frames.
 */
#define SPI_SENSOR_CFG_CRC_ENABLED  (false)

/**
 * @brief CRC polynomial (if CRC enabled)
 *
 * @note Common values: 0x1021 (CRC-16-CCITT), 0x07 (CRC-8)
 */
#define SPI_SENSOR_CFG_CRC_POLYNOMIAL  (0x1021U)

/**
 * @brief Enable data range validation
 *
 * @note Set to true to validate sensor data against expected ranges.
 */
#define SPI_SENSOR_CFG_RANGE_CHECK_ENABLED  (true)

/**
 * @brief Minimum expected sensor value (for range check)
 *
 * @note Adjust based on your sensor's output range.
 */
#define SPI_SENSOR_CFG_MIN_SENSOR_VALUE  (0U)

/**
 * @brief Maximum expected sensor value (for range check)
 *
 * @note Adjust based on your sensor's output range.
 */
#define SPI_SENSOR_CFG_MAX_SENSOR_VALUE  (255U)

/*============================================================================*/
/* Configuration Macros                                                       */
/*============================================================================*/

/**
 * @brief Create SPI sensor configuration structure
 *
 * @usage
 *   Spi_Sensor_ConfigType config = SPI_SENSOR_CFG_CREATE_CONFIG();
 */
#define SPI_SENSOR_CFG_CREATE_CONFIG() \
    { \
        .instance = SPI_SENSOR_CFG_INSTANCE, \
        .csPin = SPI_SENSOR_CFG_CS_PIN, \
        .baudrate = SPI_SENSOR_CFG_BAUDRATE, \
        .cpol = SPI_SENSOR_CFG_CPOL, \
        .cpha = SPI_SENSOR_CFG_CPHA, \
        .csToClkDelay_ns = SPI_SENSOR_CFG_CS_TO_CLK_DELAY_NS, \
        .clkToCsDelay_ns = SPI_SENSOR_CFG_CLK_TO_CS_DELAY_NS, \
        .interTransferDelay_ns = SPI_SENSOR_CFG_INTER_TRANSFER_DELAY_NS \
    }

/**
 * @brief Create API configuration structure
 *
 * @usage
 *   Spi_Sensor_ApiConfigType apiConfig = SPI_SENSOR_CFG_CREATE_API_CONFIG();
 */
#define SPI_SENSOR_CFG_CREATE_API_CONFIG() \
    { \
        .channels = { \
            { \
                .registerAddress = SPI_SENSOR_CFG_CH1_REGISTER_ADDR, \
                .periodMs = SPI_SENSOR_CFG_CH1_PERIOD_MS, \
                .enabled = SPI_SENSOR_CFG_CH1_ENABLED \
            }, \
            { \
                .registerAddress = SPI_SENSOR_CFG_CH2_REGISTER_ADDR, \
                .periodMs = SPI_SENSOR_CFG_CH2_PERIOD_MS, \
                .enabled = SPI_SENSOR_CFG_CH2_ENABLED \
            }, \
            { \
                .registerAddress = SPI_SENSOR_CFG_CH3_REGISTER_ADDR, \
                .periodMs = SPI_SENSOR_CFG_CH3_PERIOD_MS, \
                .enabled = SPI_SENSOR_CFG_CH3_ENABLED \
            } \
        }, \
        .numberOfChannels = SPI_SENSOR_CFG_NUM_CHANNELS \
    }

#endif /* SPI_SENSOR_CFG_H */

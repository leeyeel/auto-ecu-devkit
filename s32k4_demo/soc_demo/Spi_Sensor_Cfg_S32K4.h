/**
 * @file Spi_Sensor_Cfg_S32K4.h
 * @brief Configuration header for SPI Sensor module (S32K4 specific)
 *
 * @description This file contains all compile-time configuration parameters
 *              for the SPI Sensor module, customized for S32K4 hardware.
 *              Adjust these values based on your hardware and application.
 *
 * @ assumptions & constraints
 *   - Configuration values are known at compile time
 *   - Values match target S32K4 hardware specifications
 *   - S32K4 SDK (RTD) is used for low-level drivers
 *
 * @ safety considerations
 *   - Timeout values chosen for worst-case sensor response time
 *   - Retry limits prevent infinite retry loops
 *   - Range checks validate sensor data bounds
 */

#ifndef SPI_SENSOR_CFG_S32K4_H
#define SPI_SENSOR_CFG_S32K4_H

#include "Spi_Sensor_Types.h"

/*============================================================================*/
/* Hardware Configuration (S32K4 Specific)                                   */
/*============================================================================*/

/**
 * @brief SPI instance to use
 *
 * @note S32K4 has 4 LPSPI instances (LPSPI0, LPSPI1, LPSPI2, LPSPI3)
 *       Select based on your hardware design.
 *       LPSPI0 is typically available on default pins.
 */
#define SPI_SENSOR_CFG_INSTANCE  SPI_SENSOR_INSTANCE_0

/**
 * @brief Chip select pin to use
 *
 * @note S32K4 LPSPI has 4 PCS (Peripheral Chip Select) signals per instance.
 *       Configure based on PCB connection.
 */
#define SPI_SENSOR_CFG_CS_PIN    SPI_SENSOR_CS_0

/**
 * @brief SPI clock speed (baudrate)
 *
 * @note Select based on sensor maximum supported frequency.
 *       Common sensor speeds:
 *       - 125 KHz: Slow sensors, long cables
 *       - 500 KHz - 1 MHz: Most general-purpose sensors
 *       - 4 MHz: High-speed sensors
 */
#define SPI_SENSOR_CFG_BAUDRATE  SPI_SENSOR_BAUDRATE_1MHZ

/**
 * @brief SPI clock polarity
 *
 * @note CPOL = 0: Clock idle low (SCK low when inactive)
 *       CPOL = 1: Clock idle high (SCK high when inactive)
 *       Match to sensor specification.
 */
#define SPI_SENSOR_CFG_CPOL      SPI_SENSOR_CLOCK_POLARITY_0

/**
 * @brief SPI clock phase
 *
 * @note CPHA = 0: Data sampled on leading (first) edge
 *       CPHA = 1: Data sampled on trailing (second) edge
 *       Match to sensor specification.
 */
#define SPI_SENSOR_CFG_CPHA      SPI_SENSOR_CLOCK_PHASE_0

/**
 * @brief CS setup delay (nanoseconds)
 *
 * @description Delay between CS assertion and first SCK edge.
 *              Some sensors require minimum setup time after CS.
 */
#define SPI_SENSOR_CFG_CS_TO_CLK_DELAY_NS  (1000U)

/**
 * @brief CS hold delay (nanoseconds)
 *
 * @description Delay between last SCK edge and CS deassertion.
 *              Some sensors require minimum hold time before CS release.
 */
#define SPI_SENSOR_CFG_CLK_TO_CS_DELAY_NS  (1000U)

/**
 * @brief Inter-transfer delay (nanoseconds)
 *
 * @description Minimum delay between consecutive SPI transfers.
 *              Required for some sensors that process data between commands.
 */
#define SPI_SENSOR_CFG_INTER_TRANSFER_DELAY_NS  (2000U)

/*============================================================================*/
/* Sensor-Specific Protocol Configuration                                     */
/*============================================================================*/

/**
 * @brief SPI Read command byte
 *
 * @note Adjust to match your sensor's protocol.
 *       Common values:
 *       - 0x03: Standard Read (for many sensors)
 *       - 0x0B: Fast Read
 *       - 0x3B: Read with dummy byte (for some sensors)
 */
#define SPI_SENSOR_CFG_CMD_READ  (0x03U)

/**
 * @brief SPI Write command byte
 *
 * @note Adjust to match your sensor's protocol.
 *       Common value: 0x02 (Standard Write)
 */
#define SPI_SENSOR_CFG_CMD_WRITE (0x02U)

/**
 * @brief WHO_AM_I register address
 *
 * @description Register containing device ID for communication verification.
 *              Many sensors have a fixed ID register at specific address.
 */
#define SPI_SENSOR_CFG_WHO_AM_I_ADDR  (0x0FU)

/**
 * @brief Expected WHO_AM_I value
 *
 * @description Expected device ID. Used for verification during init.
 *              This is sensor-specific - replace with your sensor's ID.
 *              Example: 0xC5 for some IMU sensors.
 */
#define SPI_SENSOR_CFG_WHO_AM_I_EXPECTED  (0xC5U)

/*============================================================================*/
/* Timing Configuration                                                       */
/*============================================================================*/

/**
 * @brief Default communication timeout (milliseconds)
 *
 * @safety Must be long enough for slowest sensor response,
 *         but short enough for timely error detection.
 *         10ms is sufficient for most SPI sensors.
 */
#define SPI_SENSOR_CFG_DEFAULT_TIMEOUT_MS  (10U)

/**
 * @brief Maximum retry attempts for SPI transfers
 *
 * @safety Limited retry prevents infinite loops on persistent failure.
 *         3 attempts is typically sufficient for transient errors.
 */
#define SPI_SENSOR_CFG_MAX_RETRY_ATTEMPTS (3U)

/**
 * @brief Delay between retry attempts (milliseconds)
 *
 * @description Allows sensor time to recover between attempts.
 */
#define SPI_SENSOR_CFG_RETRY_DELAY_MS     (1U)

/*============================================================================*/
/* Safety and Diagnostic Configuration                                        */
/*============================================================================*/

/**
 * @brief Data staleness threshold (milliseconds)
 *
 * @description Data older than this is marked stale.
 * @safety Must be greater than maximum channel period.
 */
#define SPI_SENSOR_CFG_STALENESS_THRESHOLD_MS  (1000U)

/**
 * @brief Maximum consecutive errors before degraded mode
 *
 * @safety Enters degraded mode after this many consecutive failures.
 *         Prevents repeated failed access attempts.
 */
#define SPI_SENSOR_CFG_MAX_CONSECUTIVE_ERRORS  (10U)

/**
 * @brief Enable CRC checking (if supported by sensor)
 *
 * @note Set to true if your sensor supports CRC on SPI frames.
 *       Some sensors include CRC in their protocol.
 */
#define SPI_SENSOR_CFG_CRC_ENABLED  (false)

/**
 * @brief Enable data range validation
 *
 * @note Set to true to validate sensor data against expected ranges.
 *       Provides additional safety check for sensor values.
 */
#define SPI_SENSOR_CFG_RANGE_CHECK_ENABLED  (true)

/**
 * @brief Minimum expected sensor value (for range check)
 *
 * @note Adjust based on your sensor's output range.
 *       Used for plausibility checking of sensor data.
 */
#define SPI_SENSOR_CFG_MIN_SENSOR_VALUE  (0U)

/**
 * @brief Maximum expected sensor value (for range check)
 *
 * @note Adjust based on your sensor's output range.
 *       For 8-bit sensor: 255, for 16-bit: 65535.
 */
#define SPI_SENSOR_CFG_MAX_SENSOR_VALUE  (255U)

/*============================================================================*/
/* S32K4 Hardware-Specific Configuration                                      */
/*============================================================================*/

/**
 * @brief Enable LPSPI hardware chip select (Auto-CS mode)
 *
 * @note S32K4 LPSPI can automatically control PCS signals.
 *       If true, hardware manages CS timing.
 *       If false, software must control CS via GPIO.
 */
#define SPI_SENSOR_CFG_HW_AUTO_CS  (false)

/**
 * @brief Enable LPSPI FIFO mode
 *
 * @note S32K4 LPSPI has 4-word TX and RX FIFOs.
 *       Using FIFO improves efficiency for larger transfers.
 */
#define SPI_SENSOR_CFG_USE_FIFO    (true)

/**
 * @brief LPSPI TX FIFO watermark
 *
 * @note Trigger interrupt when TX FIFO has fewer than N words.
 *       Range: 0-3 (for 4-word FIFO)
 */
#define SPI_SENSOR_CFG_TX_FIFO_WATERMARK  (2U)

/**
 * @brief LPSPI RX FIFO watermark
 *
 * @note Trigger interrupt when RX FIFO has N or more words.
 *       Range: 0-3 (for 4-word FIFO)
 */
#define SPI_SENSOR_CFG_RX_FIFO_WATERMARK  (3U)

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

#endif /* SPI_SENSOR_CFG_S32K4_H */

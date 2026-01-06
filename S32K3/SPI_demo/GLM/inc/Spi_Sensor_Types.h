/**
 * @file Spi_Sensor_Types.h
 * @brief Type definitions for SPI sensor module
 * @module Spi_Sensor
 *
 * @ assumptions & constraints
 *   - Target MCU: S32K3xx
 *   - RTOS: FreeRTOS (assumed)
 *   - No dynamic memory allocation
 *   - Fixed-width types required
 *
 * @ safety considerations
 *   - All data structures designed for deterministic behavior
 *   - Status codes enable error propagation
 *   - Initialization state tracking prevents use of uninitialized modules
 *
 * @ verification checklist
 *   - [ ] All structure members initialized
 *   - [ ] No padding issues (compiler warnings)
 *   - [ ] Status codes cover all error cases
 */

#ifndef SPI_SENSOR_TYPES_H
#define SPI_SENSOR_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  /* For NULL definition (MISRA C:2012 compliant) */

/*============================================================================*/
/* Module Version                                                             */
/*============================================================================*/

#define SPI_SENSOR_MODULE_VERSION_MAJOR  (1U)
#define SPI_SENSOR_MODULE_VERSION_MINOR  (0U)
#define SPI_SENSOR_MODULE_VERSION_PATCH  (0U)

/*============================================================================*/
/* Status Codes                                                               */
/*============================================================================*/

/**
 * @brief Standard return type for all module functions
 *
 * @note Following AUTOSAR Std_ReturnType convention
 */
typedef enum {
    SPI_SENSOR_STATUS_OK            = 0U,  /**< Operation successful */
    SPI_SENSOR_STATUS_ERROR         = 1U,  /**< Generic error */
    SPI_SENSOR_STATUS_NOT_INIT      = 2U,  /**< Module not initialized */
    SPI_SENSOR_STATUS_BUSY          = 3U,  /**< Operation in progress */
    SPI_SENSOR_STATUS_TIMEOUT       = 4U,  /**< Communication timeout */
    SPI_SENSOR_STATUS_CRC_ERROR     = 5U,  /**< CRC check failed */
    SPI_SENSOR_STATUS_INVALID_PARAM = 6U,  /**< Invalid parameter */
    SPI_SENSOR_STATUS_HW_ERROR      = 7U,  /**< Hardware error detected */
    SPI_SENSOR_STATUS_NOT_READY     = 8U   /**< Device not ready */
} Spi_Sensor_StatusType;

/*============================================================================*/
/* Configuration Types                                                        */
/*============================================================================*/

/**
 * @brief SPI clock polarity and phase
 */
typedef enum {
    SPI_SENSOR_CLOCK_POLARITY_0 = 0U,  /**< CPOL = 0 */
    SPI_SENSOR_CLOCK_POLARITY_1 = 1U   /**< CPOL = 1 */
} Spi_Sensor_ClockPolarityType;

typedef enum {
    SPI_SENSOR_CLOCK_PHASE_0 = 0U,  /**< CPHA = 0 */
    SPI_SENSOR_CLOCK_PHASE_1 = 1U   /**< CPHA = 1 */
} Spi_Sensor_ClockPhaseType;

/**
 * @brief SPI transfer mode (clock speed)
 */
typedef enum {
    SPI_SENSOR_BAUDRATE_125KHZ  = 0U,
    SPI_SENSOR_BAUDRATE_250KHZ  = 1U,
    SPI_SENSOR_BAUDRATE_500KHZ  = 2U,
    SPI_SENSOR_BAUDRATE_1MHZ    = 3U,
    SPI_SENSOR_BAUDRATE_2MHZ    = 4U,
    SPI_SENSOR_BAUDRATE_4MHZ    = 5U
} Spi_Sensor_BaudrateType;

/*============================================================================*/
/* Hardware Interface Types                                                   */
/*============================================================================*/

/**
 * @brief SPI hardware instance selection
 */
typedef enum {
    SPI_SENSOR_INSTANCE_0 = 0U,
    SPI_SENSOR_INSTANCE_1 = 1U,
    SPI_SENSOR_INSTANCE_2 = 2U,
    SPI_SENSOR_INSTANCE_MAX
} Spi_Sensor_InstanceType;

/**
 * @brief Chip select configuration
 *
 * @note For S32K3, multiple CS options per SPI instance
 */
typedef enum {
    SPI_SENSOR_CS_0 = 0U,
    SPI_SENSOR_CS_1 = 1U,
    SPI_SENSOR_CS_2 = 2U,
    SPI_SENSOR_CS_3 = 3U
} Spi_Sensor_CsType;

/*============================================================================*/
/* Sensor Configuration Types                                                 */
/*============================================================================*/

/**
 * @brief SPI sensor configuration structure
 *
 * @description Contains all hardware and timing parameters needed
 *              to configure communication with an SPI sensor.
 */
typedef struct {
    Spi_Sensor_InstanceType instance;         /**< SPI HW instance */
    Spi_Sensor_CsType csPin;                   /**< Chip select pin */
    Spi_Sensor_BaudrateType baudrate;          /**< Clock speed */
    Spi_Sensor_ClockPolarityType cpol;         /**< Clock polarity */
    Spi_Sensor_ClockPhaseType cpha;            /**< Clock phase */
    uint32_t csToClkDelay_ns;                  /**< CS setup delay (ns) */
    uint32_t clkToCsDelay_ns;                  /**< CS hold delay (ns) */
    uint32_t interTransferDelay_ns;            /**< Delay between transfers */
} Spi_Sensor_ConfigType;

/*============================================================================*/
/* Data Types                                                                 */
/*============================================================================*/

/**
 * @brief Maximum supported SPI transfer size
 *
 * @safety Reasoning:
 *   Fixed buffer size prevents:
 *   - Dynamic memory allocation
 *   - Stack overflow
 *   - Undefined buffer behavior
 */
#define SPI_SENSOR_MAX_TRANSFER_SIZE  (32U)

/**
 * @brief SPI data buffer type
 *
 * @note Fixed size array for deterministic memory usage
 */
typedef struct {
    uint8_t buffer[SPI_SENSOR_MAX_TRANSFER_SIZE];
    uint16_t length;
} Spi_Sensor_BufferType;

/**
 * @brief Sensor register read/write configuration
 */
typedef struct {
    uint8_t deviceId;             /**< Device ID or address */
    uint8_t registerAddress;      /**< Register address */
    uint8_t registerData;         /**< Data to write (write operation) */
    bool autoIncrement;           /**< Auto-increment register address */
} Spi_Sensor_RegConfigType;

/*============================================================================*/
/* Diagnostic and Monitoring Types                                            */
/*============================================================================*/

/**
 * @brief Error counters for diagnostic monitoring
 *
 * @safety These counters support ISO 26262 diagnostic coverage:
 *   - Communication failure detection
 *   - Rate monitoring
 *   - Fault logging
 */
typedef struct {
    uint32_t crcErrorCount;
    uint32_t timeoutCount;
    uint32_t invalidDataCount;
    uint32_t commErrorCount;
} Spi_Sensor_DiagCountersType;

/**
 * @brief Data validity flags
 */
typedef struct {
    bool dataValid;               /**< Main data valid flag */
    bool dataStale;               /**< Data may be outdated */
    bool sensorOk;                /**< Sensor overall health */
    bool commOk;                  /**< Communication status */
} Spi_Sensor_DataValidType;

/*============================================================================*/
/* API Parameter Types                                                        */
/*============================================================================*/

/**
 * @brief Sensor reading result with metadata
 */
typedef struct {
    uint8_t registerAddress;      /**< Source register */
    uint8_t registerValue;        /**< Read value */
    Spi_Sensor_StatusType status; /**< Read operation status */
    uint32_t timestamp;           /**< Read timestamp (system ticks) */
    Spi_Sensor_DataValidType validity; /**< Data validity flags */
} Spi_Sensor_ReadResultType;

/**
 * @brief Periodic read task state
 */
typedef struct {
    bool enabled;
    uint32_t periodMs;            /**< Task period in milliseconds */
    uint32_t lastExecution;       /**< Last execution timestamp */
    uint32_t executionCount;      /**< Total execution counter */
    Spi_Sensor_DiagCountersType diagCounters;
} Spi_Sensor_TaskStateType;

#endif /* SPI_SENSOR_TYPES_H */

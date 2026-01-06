/**
 * @file    spi_demo.h
 * @brief   SPI Demo Application Header for NXP S32K3 Series
 *
 * @details This demo demonstrates SPI communication for reading register status
 *          from an external SPI device (e.g., sensor, memory, or another ECU).
 *
 * @par     Target Device: NXP S32K3 Series (e.g., S32K312, S32K342)
 * @par     SPI Peripheral: LPSPI (Low Power Serial Peripheral Interface)
 *
 * @author  Auto ECU Demo
 * @date    2026-01-07
 *
 * @note    Compliant with MISRA C:2012 and ISO 26262 safety guidelines
 *
 * ============================================================================
 * @par  Assumptions & Constraints:
 *   - Uses S32K3 SDK (RTD) for LPSPI driver
 *   - SPI operates in Master mode
 *   - Blocking transfer mode for simplicity
 *   - Limited RAM footprint (minimal buffering)
 *   - Synchronous operation (non-blocking preferred for production)
 * ============================================================================
 *
 * @par  Safety Considerations:
 *   - All input parameters validated
 *   - Transfer timeout protection
 *   - Status error checking on all SPI operations
 *   - Safe state: SPI pins configured to safe state on error
 */

#ifndef SPI_DEMO_H
#define SPI_DEMO_H

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "spi_lpspi_s32k3.h"

/*******************************************************************************
 * Public Macro Definitions
 ******************************************************************************/

/** @brief SPI Demo Version Major */
#define SPI_DEMO_VERSION_MAJOR    (1U)

/** @brief SPI Demo Version Minor */
#define SPI_DEMO_VERSION_MINOR    (0U)

/** @brief SPI Demo Version Patch */
#define SPI_DEMO_VERSION_PATCH    (0U)

/** @brief Number of registers to read in demo sequence */
#define SPI_DEMO_REG_COUNT        (8U)

/** @brief SPI Transfer timeout in milliseconds */
#define SPI_DEMO_TIMEOUT_MS       (100U)

/** @brief Maximum retry attempts for failed transfers */
#define SPI_DEMO_MAX_RETRIES      (3U)

/** @brief SPI Chip Select pin (example: PTD0 for LPSPI0_PCS0) */
#define SPI_DEMO_CS_PIN           (0U)

/** @brief Data register address for demo (example: 0x00) */
#define SPI_DEMO_REG_ADDR         (0x00U)

/** @brief Read operation bit (1 = read, 0 = write) */
#define SPI_DEMO_OP_READ          (0x80U)

/*******************************************************************************
 * Public Type Definitions
 ******************************************************************************/

/**
 * @brief   SPI Demo Status Return Codes
 *
 * @note    Compliant with AUTOSAR Std_ReturnType convention
 */
typedef enum
{
    SPI_DEMO_E_OK             = 0x00U,   /**< Success */
    SPI_DEMO_E_NOT_OK         = 0x01U,   /**< General error */
    SPI_DEMO_E_NULL_PTR       = 0x02U,   /**< Null pointer error */
    SPI_DEMO_E_TIMEOUT        = 0x03U,   /**< Transfer timeout */
    SPI_DEMO_E_INVALID_PARAM  = 0x04U,   /**< Invalid parameter */
    SPI_DEMO_E_SPI_ERROR      = 0x05U,   /**< SPI hardware error */
    SPI_DEMO_E_CRC_ERROR      = 0x06U,   /**< CRC verification failed */
    SPI_DEMO_E_DEVICE_NOT_READY = 0x07U, /**< Device not responding */
} Spi_DemoReturnType;

/**
 * @brief   SPI Demo Register Data Structure
 *
 * @note    Stores register address and read data for logging
 */
typedef struct
{
    uint8_t  regAddress;     /**< Register address */
    uint8_t  regValue;       /**< Read register value */
    uint8_t  status;         /**< Read status (0=success, non-zero=error) */
} Spi_DemoRegDataType;

/**
 * @brief   SPI Demo Configuration Structure
 *
 * @note    Initializes SPI peripheral and communication parameters
 */
typedef struct
{
    uint32_t       instance;      /**< LPSPI instance (0-3) */
    uint32_t       baudRate;      /**< SPI baud rate in Hz */
    Spi_DemoModeType mode;        /**< SPI mode (CPOL/CPHA) */
    uint8_t        chipSelect;    /**< Chip select pin */
    uint32_t       timeoutMs;     /**< Transfer timeout in ms */
} Spi_DemoConfigType;

/**
 * @brief   SPI Demo Operation Mode
 *
 * @par     Modes for SPI clock polarity and phase (CPOL/CPHA)
 */
typedef enum
{
    SPI_DEMO_MODE_0 = 0U,   /**< CPOL=0, CPHA=0 (Rising edge sample) */
    SPI_DEMO_MODE_1 = 1U,   /**< CPOL=0, CPHA=1 (Falling edge sample) */
    SPI_DEMO_MODE_2 = 2U,   /**< CPOL=1, CPHA=0 (Falling edge sample) */
    SPI_DEMO_MODE_3 = 3U,   /**< CPOL=1, CPHA=1 (Rising edge sample) */
} Spi_DemoModeType;

/**
 * @brief   SPI Demo Statistics Structure
 *
 * @note    Tracks transfer statistics for diagnostics
 */
typedef struct
{
    uint32_t txCount;       /**< Total bytes transmitted */
    uint32_t rxCount;       /**< Total bytes received */
    uint32_t errorCount;    /**< Number of transfer errors */
    uint32_t timeoutCount;  /**< Number of timeouts */
    uint32_t retryCount;    /**< Number of retry operations */
} Spi_DemoStatsType;

/*******************************************************************************
 * Public Function Prototypes
 ******************************************************************************/

/**
 * @brief   Initialize SPI Demo
 *
 * @details Configures and initializes the LPSPI peripheral for register read
 *          operations. Must be called before any other SPI demo functions.
 *
 * @param[in] pConfig  Pointer to configuration structure
 *                     (NULL uses default configuration)
 *
 * @return  SPI_DEMO_E_OK        - Initialization successful
 * @return  SPI_DEMO_E_NULL_PTR  - pConfig is NULL (and default init fails)
 * @return  SPI_DEMO_E_NOT_OK    - Initialization failed
 *
 * @pre     Platform clocks must be configured
 * @pre     LPSPI pins must be configured (PADs)
 * @post    SPI peripheral ready for transfer operations
 *
 * @safety  Reentrant: No | Thread Safe: No
 */
Spi_DemoReturnType Spi_Demo_Init(const Spi_DemoConfigType *pConfig);

/**
 * @brief   Deinitialize SPI Demo
 *
 * @details Resets SPI peripheral to safe state and releases resources.
 *
 * @return  SPI_DEMO_E_OK       - Deinitialization successful
 * @return  SPI_DEMO_E_NOT_OK   - Deinitialization failed
 *
 * @note    After deinit, Spi_Demo_Init must be called to reuse SPI
 *
 * @safety  Reentrant: No | Thread Safe: No
 */
Spi_DemoReturnType Spi_Demo_DeInit(void);

/**
 * @brief   Read Single Register via SPI
 *
 * @details Performs SPI read operation to retrieve register value from
 *          external device. Includes retry logic and timeout protection.
 *
 * @param[in]  regAddr   Register address to read
 * @param[out] pValue    Pointer to store read value (not NULL)
 *
 * @return  SPI_DEMO_E_OK            - Read successful
 * @return  SPI_DEMO_E_NULL_PTR      - pValue is NULL
 * @return  SPI_DEMO_E_INVALID_PARAM - regAddr out of range
 * @return  SPI_DEMO_E_TIMEOUT       - Transfer timeout
 * @return  SPI_DEMO_E_DEVICE_NOT_READY - No response from device
 *
 * @par     Transfer Sequence:
 *         1. Assert chip select (low)
 *         2. Send read command + address byte
 *         3. Receive data byte
 *         4. Deassert chip select (high)
 *
 * @safety  Reentrant: No | Thread Safe: No (requires mutex in RTOS)
 *
 * @note    This function blocks until transfer completes or timeout
 */
Spi_DemoReturnType Spi_Demo_ReadRegister(uint8_t regAddr, uint8_t *pValue);

/**
 * @brief   Read Multiple Registers via SPI
 *
 * @details Reads consecutive register values into provided buffer.
 *          Optimized for bulk register reads.
 *
 * @param[in]  startAddr  Starting register address
 * @param[out] pData      Pointer to data buffer (not NULL)
 * @param[in]  length     Number of registers to read (1-255)
 *
 * @return  SPI_DEMO_E_OK            - Read successful
 * @return  SPI_DEMO_E_NULL_PTR      - pData is NULL
 * @return  SPI_DEMO_E_INVALID_PARAM - length is 0
 * @return  SPI_DEMO_E_NOT_OK        - One or more reads failed
 *
 * @par     Assumptions:
 *         - External device supports auto-increment address
 *         - Buffer size >= length bytes
 *
 * @safety  Reentrant: No | Thread Safe: No
 */
Spi_DemoReturnType Spi_Demo_ReadRegisters(uint8_t  startAddr,
                                           uint8_t *pData,
                                           uint8_t  length);

/**
 * @brief   Run SPI Demo Register Read Sequence
 *
 * @details Demonstrates SPI register reading by reading a predefined set
 *          of registers and storing results.
 *
 * @param[out] pRegData  Pointer to array for register data (not NULL)
 * @param[in]  regCount  Number of registers to read
 *
 * @return  SPI_DEMO_E_OK         - Demo completed successfully
 * @return  SPI_DEMO_E_NULL_PTR   - pRegData is NULL
 * @return  SPI_DEMO_E_NOT_OK     - Demo failed
 *
 * @par     Demo Sequence:
 *         1. Read WHO_AM_I register (0x0F typical)
 *         2. Read status registers
 *         3. Read configuration registers
 *
 * @safety  Reentrant: No | Thread Safe: No
 */
Spi_DemoReturnType Spi_Demo_RunSequence(Spi_DemoRegDataType *pRegData,
                                         uint8_t               regCount);

/**
 * @brief   Save Register Data to Buffer
 *
 * @details Stores register read results in internal buffer for later
 *          retrieval or export.
 *
 * @param[in] pRegData   Pointer to register data (not NULL)
 * @param[in] regCount   Number of registers
 *
 * @return  SPI_DEMO_E_OK           - Data saved successfully
 * @return  SPI_DEMO_E_NULL_PTR     - pRegData is NULL
 * @return  SPI_DEMO_E_INVALID_PARAM - regCount is 0
 *
 * @note    Internal buffer size: SPI_DEMO_REG_COUNT
 * @note    Data can be exported via Spi_Demo_ExportData()
 *
 * @safety  Reentrant: No | Thread Safe: No
 */
Spi_DemoReturnType Spi_Demo_SaveData(const Spi_DemoRegDataType *pRegData,
                                      uint8_t                    regCount);

/**
 * @brief   Export Register Data
 *
 * @details Copies stored register data to provided buffer for external use.
 *
 * @param[out] pBuffer    Pointer to export buffer (not NULL)
 * @param[in]  bufferSize Size of export buffer in bytes
 *
 * @return  SPI_DEMO_E_OK           - Data exported successfully
 * @return  SPI_DEMO_E_NULL_PTR     - pBuffer is NULL
 * @return  SPI_DEMO_E_INVALID_PARAM - bufferSize too small
 * @return  SPI_DEMO_E_NOT_OK       - No data to export
 *
 * @par     Output Format:
 *         [addr1][value1][addr2][value2]...
 *
 * @safety  Reentrant: No | Thread Safe: No
 */
Spi_DemoReturnType Spi_Demo_ExportData(uint8_t *pBuffer,
                                        uint16_t bufferSize);

/**
 * @brief   Get Demo Statistics
 *
 * @details Retrieves current transfer statistics.
 *
 * @param[out] pStats  Pointer to statistics structure (not NULL)
 *
 * @return  SPI_DEMO_E_OK       - Statistics retrieved
 * @return  SPI_DEMO_E_NULL_PTR - pStats is NULL
 *
 * @safety  Reentrant: Yes | Thread Safe: No (atomic read)
 */
Spi_DemoReturnType Spi_Demo_GetStats(Spi_DemoStatsType *pStats);

/**
 * @brief   Print Register Data (for debugging)
 *
 * @details Outputs register data via configured debug interface (UART/printf).
 *
 * @param[in] pRegData  Pointer to register data
 * @param[in] regCount  Number of registers
 *
 * @return  SPI_DEMO_E_OK       - Print successful
 * @return  SPI_DEMO_E_NULL_PTR - pRegData is NULL
 *
 * @note    Requires debug console initialization
 * @note    Output format: "Reg[0x%02X] = 0x%02X (Status: %d)\n"
 *
 * @safety  Reentrant: Yes | Thread Safe: No
 */
Spi_DemoReturnType Spi_Demo_PrintData(const Spi_DemoRegDataType *pRegData,
                                       uint8_t                    regCount);

/**
 * @brief   Get SPI Demo Version
 *
 * @details Returns the current version of SPI Demo library.
 *
 * @return  Version encoded as (major<<16 | minor<<8 | patch)
 *
 * @par     Example: 0x010000 = Version 1.0.0
 *
 * @safety  Reentrant: Yes | Thread Safe: Yes
 */
uint32_t Spi_Demo_GetVersion(void);

#endif /* SPI_DEMO_H */

/*******************************************************************************
 * End of File
 ******************************************************************************/

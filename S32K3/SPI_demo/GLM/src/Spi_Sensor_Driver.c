/**
 * @file Spi_Sensor_Driver.c
 * @brief SPI Sensor Driver implementation
 *
 * @ assumptions & constraints
 *   - Compatible with standard register-based SPI sensors
 *   - 8-bit register addresses
 *   - SPI protocol: [CMD][ADDR][DATA] or similar
 *   - HAL layer properly initialized
 *
 * @ safety considerations
 *   - All parameters validated before use
 *   - CRC calculation when supported
 *   - Error counters for diagnostics
 *   - Timeout on all operations
 *   - Initialization state tracking
 *
 * @ execution context
 *   - All functions designed for Task context (blocking)
 *   - Not ISR-safe (blocking operations)
 *
 * @ verification checklist
 *   - [ ] No pointer dereference without validation
 *   - [ ] All status codes checked
 *   - [ ] Initialization state checked
 */

#include "Spi_Sensor_Driver.h"
#include "Spi_Sensor_Hal.h"

/*============================================================================*/
/* Private Constants                                                          */
/*============================================================================*/

/**
 * @brief SPI Read command byte
 *
 * @note Adjust based on actual sensor protocol
 *       Common patterns: 0x03 for standard read, 0x0B for fast read
 */
#define SPI_CMD_READ  (0x03U)

/**
 * @brief SPI Write command byte
 *
 * @note Adjust based on actual sensor protocol
 *       Common pattern: 0x02 for standard write
 */
#define SPI_CMD_WRITE (0x02U)

/**
 * @brief Maximum retry attempts for communication
 *
 * @safety Limited retry count prevents infinite retry loops
 */
#define MAX_RETRY_ATTEMPTS  (3U)

/**
 * @brief Delay between retry attempts (milliseconds)
 */
#define RETRY_DELAY_MS  (1U)

/*============================================================================*/
/* Private Types                                                              */
/*============================================================================*/

/**
 * @brief Driver state structure
 *
 * @description Tracks runtime state of the sensor driver.
 *          Single instance for single sensor configuration.
 */
typedef struct {
    bool initialized;                           /**< Driver initialized flag */
    Spi_Sensor_InstanceType spiInstance;        /**< SPI instance in use */
    Spi_Sensor_CsType csPin;                    /**< CS pin in use */
    Spi_Sensor_DiagCountersType diagCounters;   /**< Error counters */
} Spi_Sensor_DriverStateType;

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

/**
 * @brief Driver state instance
 *
 * @safety Global state, protected by initialization checks
 *
 * @critical Access to diagCounters may require protection if
 *          called from multiple contexts
 */
static Spi_Sensor_DriverStateType Driver_State = {
    false,              /* initialized */
    SPI_SENSOR_INSTANCE_0,
    SPI_SENSOR_CS_0,
    {0, 0, 0, 0}        /* diagCounters */
};

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

/**
 * @brief Perform single register read with retry logic
 *
 * @param registerAddr Register address to read
 * @param dataPtr      Pointer to store data
 * @param maxAttempts  Maximum retry attempts
 *
 * @return Spi_Sensor_StatusType
 *
 * @safety Limited retry attempts prevent infinite loops
 */
static Spi_Sensor_StatusType Driver_ReadRegisterWithRetry(
    uint8_t registerAddr,
    uint8_t* const dataPtr,
    uint32_t maxAttempts
);

/**
 * @brief Perform single register write with retry logic
 *
 * @param registerAddr Register address to write
 * @param data         Data to write
 * @param maxAttempts  Maximum retry attempts
 *
 * @return Spi_Sensor_StatusType
 *
 * @safety Limited retry attempts prevent infinite loops
 */
static Spi_Sensor_StatusType Driver_WriteRegisterWithRetry(
    uint8_t registerAddr,
    uint8_t data,
    uint32_t maxAttempts
);

/**
 * @brief Update error counter
 *
 * @param counterPtr Pointer to counter to increment
 *
 * @safety Counter saturation check prevents overflow
 */
static void Driver_UpdateCounter(uint32_t* const counterPtr);

/**
 * @brief Validate register address
 *
 * @param addr Address to validate
 *
 * @return true if valid, false otherwise
 *
 * @reentrant Yes
 * @safety Pure validation function
 */
static bool Driver_IsValidRegisterAddress(uint8_t addr);

/**
 * @brief Simple delay function
 *
 * @param delayMs Delay in milliseconds
 *
 * @note In real implementation, use OS delay function
 */
static void Driver_Delay(uint32_t delayMs);

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

Spi_Sensor_StatusType Spi_Sensor_Driver_Init(const Spi_Sensor_ConfigType* const configPtr)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    /* Parameter validation */
    if (NULL == configPtr) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else {
        /* Initialize HAL first */
        status = Spi_Sensor_Hal_Init(configPtr);
    }

    if (SPI_SENSOR_STATUS_OK == status) {
        /* HAL initialized successfully, initialize driver state */

        /*
         * In a real implementation, we might:
         * 1. Perform a soft reset of the sensor
         * 2. Verify communication by reading WHO_AM_I register
         * 3. Configure sensor settings if needed
         */

        /* Store configuration in driver state */
        Driver_State.spiInstance = configPtr->instance;
        Driver_State.csPin = configPtr->csPin;

        /* Reset diagnostic counters */
        Driver_State.diagCounters.crcErrorCount = 0U;
        Driver_State.diagCounters.timeoutCount = 0U;
        Driver_State.diagCounters.invalidDataCount = 0U;
        Driver_State.diagCounters.commErrorCount = 0U;

        /* Mark as initialized */
        Driver_State.initialized = true;
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Driver_Deinit(void)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    /* Check initialization */
    if (false == Driver_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* Deinitialize HAL */
        status = Spi_Sensor_Hal_Deinit(Driver_State.spiInstance);

        /* Mark driver as not initialized regardless of HAL result */
        Driver_State.initialized = false;
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Driver_ReadRegister(
    uint8_t registerAddr,
    uint8_t* const dataPtr)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    /* Parameter validation */
    if (NULL == dataPtr) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Driver_IsValidRegisterAddress(registerAddr)) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Driver_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* Perform read with retry */
        status = Driver_ReadRegisterWithRetry(
            registerAddr,
            dataPtr,
            MAX_RETRY_ATTEMPTS
        );
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Driver_WriteRegister(
    uint8_t registerAddr,
    uint8_t data)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    /* Parameter validation */
    if (false == Driver_IsValidRegisterAddress(registerAddr)) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Driver_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* Perform write with retry */
        status = Driver_WriteRegisterWithRetry(
            registerAddr,
            data,
            MAX_RETRY_ATTEMPTS
        );
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Driver_ReadRegisterBlock(
    uint8_t startAddr,
    uint8_t* const buffer,
    uint16_t length)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;
    uint8_t txBuffer[3];
    uint8_t rxBuffer[SPI_SENSOR_MAX_TRANSFER_SIZE];
    uint32_t timeout;
    uint16_t i;

    /* Parameter validation */
    if (NULL == buffer) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (0U == length) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (length > SPI_SENSOR_MAX_TRANSFER_SIZE) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Driver_IsValidRegisterAddress(startAddr)) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if ((uint32_t)startAddr + (uint32_t)length > (uint32_t)(SPI_SENSOR_MAX_REGISTER_ADDR + 1U)) {
        /* Range check: would overflow register address space */
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Driver_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* All parameters valid, proceed with read */
    }

    if (SPI_SENSOR_STATUS_OK == status) {
        timeout = Spi_Sensor_Hal_CalcTimeoutTicks(SPI_SENSOR_DEFAULT_TIMEOUT_MS);

        /* Build command frame */
        /* Format: [CMD][ADDR][DUMMY][DATA0][DATA1]... */
        txBuffer[0] = SPI_CMD_READ;
        txBuffer[1] = startAddr;
        txBuffer[2] = 0U;  /* Dummy byte */

        /* Assert CS */
        status = Spi_Sensor_Hal_AssertCs(Driver_State.spiInstance, Driver_State.csPin);

        if (SPI_SENSOR_STATUS_OK == status) {
            /* Add small delay after CS assert */
            Driver_Delay(1U);

            /* Perform transfer */
            /* For block read, we send CMD+ADDR and receive length+1 bytes */
            status = Spi_Sensor_Hal_TransferBlocking(
                Driver_State.spiInstance,
                txBuffer,
                rxBuffer,
                (uint16_t)(3U + length),  /* 3 bytes cmd + data bytes */
                timeout
            );

            /* Deassert CS */
            Spi_Sensor_Hal_DeassertCs(Driver_State.spiInstance, Driver_State.csPin);
        }

        if (SPI_SENSOR_STATUS_OK == status) {
            /* Extract data from response */
            /* Data starts after CMD + ADDR bytes */
            for (i = 0U; i < length; i++) {
                buffer[i] = rxBuffer[3U + i];  /* Skip CMD, ADDR, DUMMY */
            }
        }
        else {
            /* Update error counter */
            Driver_UpdateCounter(&Driver_State.diagCounters.commErrorCount);
        }
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Driver_WriteRegisterBlock(
    uint8_t startAddr,
    const uint8_t* const buffer,
    uint16_t length)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;
    uint8_t txBuffer[SPI_SENSOR_MAX_TRANSFER_SIZE];
    uint32_t timeout;
    uint16_t i;

    /* Parameter validation */
    if (NULL == buffer) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (0U == length) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (length > (SPI_SENSOR_MAX_TRANSFER_SIZE - 2U)) {
        /* Need space for CMD and ADDR */
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Driver_IsValidRegisterAddress(startAddr)) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if ((uint32_t)startAddr + (uint32_t)length > (uint32_t)(SPI_SENSOR_MAX_REGISTER_ADDR + 1U)) {
        /* Range check */
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Driver_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* All parameters valid, proceed with write */
    }

    if (SPI_SENSOR_STATUS_OK == status) {
        timeout = Spi_Sensor_Hal_CalcTimeoutTicks(SPI_SENSOR_DEFAULT_TIMEOUT_MS);

        /* Build command frame */
        /* Format: [CMD][ADDR][DATA0][DATA1]... */
        txBuffer[0] = SPI_CMD_WRITE;
        txBuffer[1] = startAddr;

        for (i = 0U; i < length; i++) {
            txBuffer[2U + i] = buffer[i];
        }

        /* Assert CS */
        status = Spi_Sensor_Hal_AssertCs(Driver_State.spiInstance, Driver_State.csPin);

        if (SPI_SENSOR_STATUS_OK == status) {
            /* Add small delay after CS assert */
            Driver_Delay(1U);

            /* Perform transfer */
            status = Spi_Sensor_Hal_TransferBlocking(
                Driver_State.spiInstance,
                txBuffer,
                NULL,  /* No RX data needed for write */
                (uint16_t)(2U + length),
                timeout
            );

            /* Deassert CS */
            Spi_Sensor_Hal_DeassertCs(Driver_State.spiInstance, Driver_State.csPin);
        }

        if (SPI_SENSOR_STATUS_OK != status) {
            /* Update error counter */
            Driver_UpdateCounter(&Driver_State.diagCounters.commErrorCount);
        }
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Driver_VerifyCommunication(void)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;
    uint8_t readback;

    /* testValue variable removed - unused in demo implementation */

    if (false == Driver_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /*
         * In a real implementation, we would:
         * 1. Read a known register (e.g., WHO_AM_I at address 0x0F)
         * 2. Verify the value matches expected device ID
         *
         * For demo, we'll attempt a read of register 0x00
         */

        status = Spi_Sensor_Driver_ReadRegister(0x00U, &readback);

        if (SPI_SENSOR_STATUS_OK == status) {
            /* Optionally verify the value matches expected device ID */
            /* testValue = 0xC5U;  Example device ID */
            /* if (readback != testValue) { status = SPI_SENSOR_STATUS_ERROR; } */
        }
    }

    return status;
}

bool Spi_Sensor_Driver_IsInitialized(void)
{
    return Driver_State.initialized;
}

Spi_Sensor_StatusType Spi_Sensor_Driver_GetDiagCounters(
    Spi_Sensor_DiagCountersType* const countersPtr)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    if (NULL == countersPtr) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else {
        /* Critical section for atomic read */
        /* SchM_Enter_Spi_Sensor_SECTION(); */

        *countersPtr = Driver_State.diagCounters;

        /* SchM_Exit_Spi_Sensor_SECTION(); */
    }

    return status;
}

void Spi_Sensor_Driver_ResetDiagCounters(void)
{
    /* Critical section for atomic write */
    /* SchM_Enter_Spi_Sensor_SECTION(); */

    Driver_State.diagCounters.crcErrorCount = 0U;
    Driver_State.diagCounters.timeoutCount = 0U;
    Driver_State.diagCounters.invalidDataCount = 0U;
    Driver_State.diagCounters.commErrorCount = 0U;

    /* SchM_Exit_Spi_Sensor_SECTION(); */
}

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static Spi_Sensor_StatusType Driver_ReadRegisterWithRetry(
    uint8_t registerAddr,
    uint8_t* const dataPtr,
    uint32_t maxAttempts)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_ERROR;
    uint32_t attempt;
    uint8_t txBuffer[3];
    uint8_t rxBuffer[3];
    uint32_t timeout;

    timeout = Spi_Sensor_Hal_CalcTimeoutTicks(SPI_SENSOR_DEFAULT_TIMEOUT_MS);

    for (attempt = 0U; attempt < maxAttempts; attempt++) {
        /* Build read command */
        txBuffer[0] = SPI_CMD_READ;
        txBuffer[1] = registerAddr;
        txBuffer[2] = 0U;  /* Dummy byte */

        /* Assert CS */
        status = Spi_Sensor_Hal_AssertCs(Driver_State.spiInstance, Driver_State.csPin);

        if (SPI_SENSOR_STATUS_OK == status) {
            Driver_Delay(1U);

            /* Perform transfer */
            status = Spi_Sensor_Hal_TransferBlocking(
                Driver_State.spiInstance,
                txBuffer,
                rxBuffer,
                3U,
                timeout
            );

            /* Deassert CS */
            Spi_Sensor_Hal_DeassertCs(Driver_State.spiInstance, Driver_State.csPin);
        }

        if (SPI_SENSOR_STATUS_OK == status) {
            /* Extract data (third byte) */
            *dataPtr = rxBuffer[2];

            /* Success - exit retry loop */
            break;
        }
        else {
            /* Update error counter based on error type */
            if (SPI_SENSOR_STATUS_TIMEOUT == status) {
                Driver_UpdateCounter(&Driver_State.diagCounters.timeoutCount);
            }
            else {
                Driver_UpdateCounter(&Driver_State.diagCounters.commErrorCount);
            }

            /* Delay before retry */
            Driver_Delay(RETRY_DELAY_MS);
        }
    }

    return status;
}

static Spi_Sensor_StatusType Driver_WriteRegisterWithRetry(
    uint8_t registerAddr,
    uint8_t data,
    uint32_t maxAttempts)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_ERROR;
    uint32_t attempt;
    uint8_t txBuffer[3];
    uint32_t timeout;

    timeout = Spi_Sensor_Hal_CalcTimeoutTicks(SPI_SENSOR_DEFAULT_TIMEOUT_MS);

    for (attempt = 0U; attempt < maxAttempts; attempt++) {
        /* Build write command */
        txBuffer[0] = SPI_CMD_WRITE;
        txBuffer[1] = registerAddr;
        txBuffer[2] = data;

        /* Assert CS */
        status = Spi_Sensor_Hal_AssertCs(Driver_State.spiInstance, Driver_State.csPin);

        if (SPI_SENSOR_STATUS_OK == status) {
            Driver_Delay(1U);

            /* Perform transfer */
            status = Spi_Sensor_Hal_TransferBlocking(
                Driver_State.spiInstance,
                txBuffer,
                NULL,  /* No RX needed for write */
                3U,
                timeout
            );

            /* Deassert CS */
            Spi_Sensor_Hal_DeassertCs(Driver_State.spiInstance, Driver_State.csPin);
        }

        if (SPI_SENSOR_STATUS_OK == status) {
            /* Success - exit retry loop */
            break;
        }
        else {
            /* Update error counter */
            if (SPI_SENSOR_STATUS_TIMEOUT == status) {
                Driver_UpdateCounter(&Driver_State.diagCounters.timeoutCount);
            }
            else {
                Driver_UpdateCounter(&Driver_State.diagCounters.commErrorCount);
            }

            /* Delay before retry */
            Driver_Delay(RETRY_DELAY_MS);
        }
    }

    return status;
}

static void Driver_UpdateCounter(uint32_t* const counterPtr)
{
    /* Check for saturation before incrementing */
    if (*counterPtr < 0xFFFFFFFFU) {
        (*counterPtr)++;
    }
    /* If already at max, keep at max (saturation) */
}

static bool Driver_IsValidRegisterAddress(uint8_t addr)
{
    bool valid = true;

    /* Check if address is within valid range */
    if (addr > SPI_SENSOR_MAX_REGISTER_ADDR) {
        valid = false;
    }

    return valid;
}

static void Driver_Delay(uint32_t delayMs)
{
    /*
     * In real implementation:
     * - FreeRTOS: vTaskDelay(pdMS_TO_TICKS(delayMs))
     * - Bare-metal: Use timer-based delay or busy-wait with timeout
     *
     * For demo, this is a placeholder
     */
    (void)delayMs;  /* Suppress unused warning */
}

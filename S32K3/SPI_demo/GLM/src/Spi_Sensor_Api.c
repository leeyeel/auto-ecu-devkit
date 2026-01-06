/**
 * @file Spi_Sensor_Api.c
 * @brief High-level API implementation for periodic SPI sensor reading
 *
 * @ assumptions & constraints
 *   - Single sensor instance
 *   - Cyclic function called from application's main loop
 *   - System provides millisecond timestamp
 *
 * @ safety considerations
 *   - Non-blocking cyclic processing
 *   - Data validity flags indicate data quality
 *   - Degraded mode prevents use of stale data
 *   - All errors logged for diagnostics
 *   - Timestamp tracking for data freshness
 *
 * @ execution context
 *   - Init/Deinit: Task context only
 *   - Cyclic: Cyclic task context (e.g., 10ms task)
 *   - Get functions: Any context, reentrant
 *
 * @ verification checklist
 *   - [ ] Cyclic execution time bounded
 *   - [ ] Data access is thread-safe
 *   - [ ] All array accesses bounds-checked
 */

#include "Spi_Sensor_Api.h"
#include "Spi_Sensor_Driver.h"

/*============================================================================*/
/* Private Constants                                                          */
/*============================================================================*/

/**
 * @brief Data staleness threshold (milliseconds)
 *
 * @safety Reasoning:
 *   Data older than this threshold is marked stale.
 *   Adjust based on application requirements and sensor update rates.
 */
#define DATA_STALENESS_THRESHOLD_MS  (1000U)

/**
 * @brief Maximum consecutive errors before degraded mode
 *
 * @safety Enters degraded mode after consecutive failures
 */
#define MAX_CONSECUTIVE_ERRORS  (10U)

/*============================================================================*/
/* Private Types                                                              */
/*============================================================================*/

/**
 * @brief Channel runtime state
 *
 * @description Tracks state for each periodic read channel.
 */
typedef struct {
    Spi_Sensor_ChannelConfigType config;       /**< Channel configuration */
    Spi_Sensor_ReadResultType lastResult;      /**< Last read result */
    Spi_Sensor_TaskStateType taskState;        /**< Task scheduling state */
    uint32_t consecutiveErrorCount;            /**< Consecutive error counter */
} Spi_Sensor_ChannelStateType;

/**
 * @brief API module state
 *
 * @description Global state for the sensor API module.
 */
typedef struct {
    bool initialized;                           /**< Module initialized flag */
    Spi_Sensor_ChannelStateType channels[SPI_SENSOR_MAX_CHANNELS];
    uint8_t numberOfChannels;                   /**< Active channel count */
    bool degradedMode;                          /**< Degraded mode flag */
    Spi_Sensor_DiagCountersType diagCounters;   /**< Aggregated error counters */
} Spi_Sensor_ApiStateType;

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

/**
 * @brief API module state
 *
 * @critical Shared state requires protection:
 *          - lastResult accessed by cyclic and get functions
 *          - Use critical sections if multi-threaded
 */
static Spi_Sensor_ApiStateType Api_State = {
    .initialized = false,
    .numberOfChannels = 0,
    .degradedMode = false,
    .diagCounters = {0, 0, 0, 0},
    /* channels array will be initialized at runtime */
};

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

/**
 * @brief Validate API configuration
 *
 * @param configPtr Configuration to validate
 *
 * @return true if valid, false otherwise
 *
 * @reentrant Yes
 * @safety Read-only validation
 */
static bool Api_ValidateConfig(const Spi_Sensor_ApiConfigType* const configPtr);

/**
 * @brief Check if channel is due for execution
 *
 * @param channelId   Channel to check
 * @param currentTime Current system time (ms)
 *
 * @return true if due, false otherwise
 *
 * @reentrant Yes
 * @safety Pure function, no side effects
 */
static bool Api_IsChannelDue(uint8_t channelId, uint32_t currentTime);

/**
 * @brief Execute periodic read for a channel
 *
 * @param channelId Channel to read
 *
 * @return Spi_Sensor_StatusType
 *
 * @safety Updates result atomically
 */
static Spi_Sensor_StatusType Api_ExecuteChannelRead(uint8_t channelId);

/**
 * @brief Update result with validity flags
 *
 * @param resultPtr Result to update
 * @param status    Driver status
 *
 * @safety Sets all validity flags based on status and age
 */
static void Api_UpdateValidityFlags(
    Spi_Sensor_ReadResultType* const resultPtr,
    Spi_Sensor_StatusType status
);

/**
 * @brief Check and enter degraded mode if needed
 *
 * @param channelId Channel that had error
 *
 * @return true if degraded mode entered, false otherwise
 *
 * @safety Automatic degraded mode on persistent errors
 */
static bool Api_CheckDegradedMode(uint8_t channelId);

/**
 * @brief Get current system time in milliseconds
 *
 * @return Current time in ms
 *
 * @note In real implementation, get from OS or hardware timer
 */
static uint32_t Api_GetCurrentTime(void);

/**
 * @brief Check if data is stale
 *
 * @param timestamp Data timestamp
 * @param currentTime Current time
 *
 * @return true if stale, false otherwise
 *
 * @reentrant Yes
 * @safety Pure comparison function
 */
static bool Api_IsDataStale(uint32_t timestamp, uint32_t currentTime);

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

Spi_Sensor_StatusType Spi_Sensor_Api_Init(
    const Spi_Sensor_ApiConfigType* const configPtr)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;
    uint8_t i;

    /* Parameter validation */
    if (NULL == configPtr) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Api_ValidateConfig(configPtr)) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Spi_Sensor_Driver_IsInitialized()) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
        /* Driver must be initialized before API */
    }
    else {
        /* All valid, proceed with initialization */
    }

    if (SPI_SENSOR_STATUS_OK == status) {
        /* Check if already initialized */
        if (true == Api_State.initialized) {
            /* Already initialized - reinitialization not supported */
            status = SPI_SENSOR_STATUS_ERROR;
        }
        else {
            /* Initialize module state */

            /* Store configuration */
            Api_State.numberOfChannels = configPtr->numberOfChannels;

            /* Initialize each channel */
            for (i = 0U; i < configPtr->numberOfChannels; i++) {
                Api_State.channels[i].config = configPtr->channels[i];

                /* Initialize result structure */
                Api_State.channels[i].lastResult.registerAddress =
                    configPtr->channels[i].registerAddress;
                Api_State.channels[i].lastResult.registerValue = 0U;
                Api_State.channels[i].lastResult.status = SPI_SENSOR_STATUS_NOT_READY;
                Api_State.channels[i].lastResult.timestamp = 0U;
                Api_State.channels[i].lastResult.validity.dataValid = false;
                Api_State.channels[i].lastResult.validity.dataStale = true;
                Api_State.channels[i].lastResult.validity.sensorOk = false;
                Api_State.channels[i].lastResult.validity.commOk = false;

                /* Initialize task state */
                Api_State.channels[i].taskState.enabled =
                    configPtr->channels[i].enabled;
                Api_State.channels[i].taskState.periodMs =
                    configPtr->channels[i].periodMs;
                Api_State.channels[i].taskState.lastExecution = 0U;
                Api_State.channels[i].taskState.executionCount = 0U;

                /* Reset error counters */
                Api_State.channels[i].consecutiveErrorCount = 0U;
            }

            /* Reset module-level state */
            Api_State.degradedMode = false;
            Api_State.diagCounters.crcErrorCount = 0U;
            Api_State.diagCounters.timeoutCount = 0U;
            Api_State.diagCounters.invalidDataCount = 0U;
            Api_State.diagCounters.commErrorCount = 0U;

            /* Mark as initialized */
            Api_State.initialized = true;
        }
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_Deinit(void)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* Mark as not initialized */
        Api_State.initialized = false;

        /* Note: We don't deinit the driver here, as it may be shared */
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_Cyclic(uint32_t currentTimeMs)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;
    uint8_t i;
    Spi_Sensor_StatusType readStatus;

    /* Check initialization */
    if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* Process each enabled channel */
        for (i = 0U; i < Api_State.numberOfChannels; i++) {
            /* Check if channel is enabled and due */
            if ((true == Api_State.channels[i].taskState.enabled) &&
                (true == Api_IsChannelDue(i, currentTimeMs))) {

                /* Execute channel read */
                readStatus = Api_ExecuteChannelRead(i);

                /* Update error tracking */
                if (SPI_SENSOR_STATUS_OK != readStatus) {
                    Api_State.channels[i].consecutiveErrorCount++;

                    /* Check for degraded mode */
                    if (true == Api_CheckDegradedMode(i)) {
                        /* Entered degraded mode */
                    }
                }
                else {
                    /* Reset consecutive error count on success */
                    Api_State.channels[i].consecutiveErrorCount = 0U;
                }

                /* Update task state */
                Api_State.channels[i].taskState.lastExecution = currentTimeMs;
                Api_State.channels[i].taskState.executionCount++;
            }
        }
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_GetReading(
    uint8_t channelId,
    Spi_Sensor_ReadResultType* const resultPtr)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;
    uint32_t currentTime;

    /* Parameter validation */
    if (NULL == resultPtr) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (channelId >= Api_State.numberOfChannels) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* Get current time for staleness check */
        currentTime = Api_GetCurrentTime();

        /* Critical section start */
        /* SchM_Enter_Spi_Sensor_Api_SECTION(); */

        /* Copy result (atomic copy) */
        *resultPtr = Api_State.channels[channelId].lastResult;

        /* Update staleness flag */
        if (true == Api_IsDataStale(resultPtr->timestamp, currentTime)) {
            resultPtr->validity.dataStale = true;
        }

        /* If in degraded mode, mark data as potentially stale */
        if (true == Api_State.degradedMode) {
            resultPtr->validity.dataStale = true;
        }

        /* Critical section end */
        /* SchM_Exit_Spi_Sensor_Api_SECTION(); */
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_GetAllReadings(
    Spi_Sensor_ReadResultType* const resultArray,
    uint8_t arraySize)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;
    uint8_t i;
    uint32_t currentTime;

    /* Parameter validation */
    if (NULL == resultArray) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (arraySize < Api_State.numberOfChannels) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* Get current time */
        currentTime = Api_GetCurrentTime();

        /* Copy all results */
        for (i = 0U; i < Api_State.numberOfChannels; i++) {
            /* Critical section start */
            /* SchM_Enter_Spi_Sensor_Api_SECTION(); */

            resultArray[i] = Api_State.channels[i].lastResult;

            /* Update staleness */
            if (true == Api_IsDataStale(resultArray[i].timestamp, currentTime)) {
                resultArray[i].validity.dataStale = true;
            }

            if (true == Api_State.degradedMode) {
                resultArray[i].validity.dataStale = true;
            }

            /* Critical section end */
            /* SchM_Exit_Spi_Sensor_Api_SECTION(); */
        }
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_EnableChannel(uint8_t channelId)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    if (channelId >= Api_State.numberOfChannels) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        Api_State.channels[channelId].taskState.enabled = true;
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_DisableChannel(uint8_t channelId)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    if (channelId >= Api_State.numberOfChannels) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        Api_State.channels[channelId].taskState.enabled = false;
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_SetPeriod(uint8_t channelId, uint32_t periodMs)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    if (channelId >= Api_State.numberOfChannels) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if ((periodMs < 1U) || (periodMs > SPI_SENSOR_MAX_PERIOD_MS)) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        Api_State.channels[channelId].taskState.periodMs = periodMs;
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_GetDiagCounters(
    Spi_Sensor_DiagCountersType* const countersPtr)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;
    Spi_Sensor_DiagCountersType driverCounters;

    if (NULL == countersPtr) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* Get driver counters */
        (void)Spi_Sensor_Driver_GetDiagCounters(&driverCounters);

        /* Aggregate API + driver counters */
        /* Critical section start */
        countersPtr->crcErrorCount =
            Api_State.diagCounters.crcErrorCount + driverCounters.crcErrorCount;
        countersPtr->timeoutCount =
            Api_State.diagCounters.timeoutCount + driverCounters.timeoutCount;
        countersPtr->invalidDataCount =
            Api_State.diagCounters.invalidDataCount + driverCounters.invalidDataCount;
        countersPtr->commErrorCount =
            Api_State.diagCounters.commErrorCount + driverCounters.commErrorCount;
        /* Critical section end */
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_ResetDiagCounters(void)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        /* Reset API counters */
        /* Critical section start */
        Api_State.diagCounters.crcErrorCount = 0U;
        Api_State.diagCounters.timeoutCount = 0U;
        Api_State.diagCounters.invalidDataCount = 0U;
        Api_State.diagCounters.commErrorCount = 0U;
        /* Critical section end */

        /* Reset driver counters */
        Spi_Sensor_Driver_ResetDiagCounters();
    }

    return status;
}

bool Spi_Sensor_Api_IsInitialized(void)
{
    return Api_State.initialized;
}

Spi_Sensor_StatusType Spi_Sensor_Api_SetDegradedMode(void)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        Api_State.degradedMode = true;
    }

    return status;
}

Spi_Sensor_StatusType Spi_Sensor_Api_ExitDegradedMode(void)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;

    if (false == Api_State.initialized) {
        status = SPI_SENSOR_STATUS_NOT_INIT;
    }
    else {
        Api_State.degradedMode = false;
    }

    return status;
}

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static bool Api_ValidateConfig(const Spi_Sensor_ApiConfigType* const configPtr)
{
    bool valid = true;
    uint8_t i;

    /* Check number of channels */
    if ((configPtr->numberOfChannels < 1U) ||
        (configPtr->numberOfChannels > SPI_SENSOR_MAX_CHANNELS)) {
        valid = false;
    }

    /* Validate each channel configuration */
    for (i = 0U; i < configPtr->numberOfChannels; i++) {
        /* Check register address */
        if (configPtr->channels[i].registerAddress > SPI_SENSOR_MAX_REGISTER_ADDR) {
            valid = false;
        }

        /* Check period */
        if ((configPtr->channels[i].periodMs < 1U) ||
            (configPtr->channels[i].periodMs > SPI_SENSOR_MAX_PERIOD_MS)) {
            valid = false;
        }
    }

    return valid;
}

static bool Api_IsChannelDue(uint8_t channelId, uint32_t currentTime)
{
    bool isDue = false;
    uint32_t elapsed;
    uint32_t period;

    period = Api_State.channels[channelId].taskState.periodMs;

    if (0U == Api_State.channels[channelId].taskState.lastExecution) {
        /* Never executed - due now */
        isDue = true;
    }
    else {
        /* Calculate elapsed time */
        elapsed = currentTime - Api_State.channels[channelId].taskState.lastExecution;

        /* Check if period elapsed */
        if (elapsed >= period) {
            isDue = true;
        }
    }

    return isDue;
}

static Spi_Sensor_StatusType Api_ExecuteChannelRead(uint8_t channelId)
{
    Spi_Sensor_StatusType status;
    Spi_Sensor_ReadResultType* resultPtr;
    uint8_t registerAddr;
    uint8_t registerValue;

    resultPtr = &Api_State.channels[channelId].lastResult;
    registerAddr = Api_State.channels[channelId].config.registerAddress;

    /* Perform register read */
    status = Spi_Sensor_Driver_ReadRegister(registerAddr, &registerValue);

    /* Update result */
    /* Critical section start */
    /* SchM_Enter_Spi_Sensor_Api_SECTION(); */

    resultPtr->registerAddress = registerAddr;
    resultPtr->registerValue = registerValue;
    resultPtr->status = status;
    resultPtr->timestamp = Api_GetCurrentTime();

    /* Update validity flags */
    Api_UpdateValidityFlags(resultPtr, status);

    /* Update error counters */
    if (SPI_SENSOR_STATUS_OK != status) {
        if (SPI_SENSOR_STATUS_CRC_ERROR == status) {
            Api_State.diagCounters.crcErrorCount++;
        }
        else if (SPI_SENSOR_STATUS_TIMEOUT == status) {
            Api_State.diagCounters.timeoutCount++;
        }
        else {
            Api_State.diagCounters.commErrorCount++;
        }
    }

    /* Critical section end */
    /* SchM_Exit_Spi_Sensor_Api_SECTION(); */

    return status;
}

static void Api_UpdateValidityFlags(
    Spi_Sensor_ReadResultType* const resultPtr,
    Spi_Sensor_StatusType status)
{
    /* Initialize flags */
    resultPtr->validity.dataValid = false;
    resultPtr->validity.dataStale = false;
    resultPtr->validity.sensorOk = false;
    resultPtr->validity.commOk = false;

    if (SPI_SENSOR_STATUS_OK == status) {
        /* Success - all flags positive */
        resultPtr->validity.dataValid = true;
        resultPtr->validity.sensorOk = true;
        resultPtr->validity.commOk = true;
    }
    else if (SPI_SENSOR_STATUS_TIMEOUT == status) {
        /* Timeout - communication failed */
        resultPtr->validity.commOk = false;
        resultPtr->validity.sensorOk = false;
    }
    else if (SPI_SENSOR_STATUS_CRC_ERROR == status) {
        /* CRC error - data invalid */
        resultPtr->validity.commOk = true;   /* Comm worked, data bad */
        resultPtr->validity.sensorOk = false;
    }
    else {
        /* Other error */
        resultPtr->validity.commOk = false;
        resultPtr->validity.sensorOk = false;
    }
}

static bool Api_CheckDegradedMode(uint8_t channelId)
{
    bool entered = false;

    if (Api_State.channels[channelId].consecutiveErrorCount >=
        MAX_CONSECUTIVE_ERRORS) {
        /* Too many consecutive errors - enter degraded mode */
        Api_State.degradedMode = true;
        entered = true;
    }

    return entered;
}

static uint32_t Api_GetCurrentTime(void)
{
    /*
     * In real implementation:
     * - FreeRTOS: xTaskGetTickCount() * portTICK_PERIOD_MS
     * - AUTOSAR: Std_GetTime()
     * - Bare-metal: Hardware timer counter
     *
     * For demo, this would be provided by the system
     */

    /* Placeholder - would return actual system time */
    return 0U;
}

static bool Api_IsDataStale(uint32_t timestamp, uint32_t currentTime)
{
    bool isStale = false;
    uint32_t age;

    if (0U == timestamp) {
        /* Never updated - definitely stale */
        isStale = true;
    }
    else {
        /* Calculate data age */
        age = currentTime - timestamp;

        if (age > DATA_STALENESS_THRESHOLD_MS) {
            isStale = true;
        }
    }

    return isStale;
}

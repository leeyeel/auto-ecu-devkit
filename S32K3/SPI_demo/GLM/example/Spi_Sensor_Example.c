/**
 * @file Spi_Sensor_Example.c
 * @brief Example application demonstrating SPI Sensor API usage
 *
 * @description This example shows how to:
 *              1. Initialize the SPI sensor module
 *              2. Configure periodic read channels
 *              3. Call cyclic processing function
 *              4. Read sensor data
 *              5. Handle errors and diagnostics
 *
 * @ assumptions & constraints
 *   - S32K3 target with FreeRTOS
 *   - System timer configured for 1ms tick
 *   - SPI pins properly muxed
 *
 * @ safety considerations
 *   - All return values checked
 *   - Error handling implemented
 *   - Data validity verified before use
 */

#include "Spi_Sensor_Api.h"
#include "Spi_Sensor_Driver.h"
#include "Spi_Sensor_Cfg.h"

/*============================================================================*/
/* Includes                                                                   */
/*============================================================================*/

/*
 * In real application:
 * - #include "OsIf.h"          (OS interface)
 * - #include "Det.h"           (Error detection, if using AUTOSAR)
 * - #include "Mcal.h"          (MCAL initialization)
 */

/*============================================================================*/
/* Private Constants                                                          */
/*============================================================================*/

/**
 * @brief Cyclic task period (milliseconds)
 */
#define CYCLIC_TASK_PERIOD_MS  (10U)

/**
 * @brief Maximum initialization retry attempts
 */
#define MAX_INIT_RETRY  (3U)

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

/**
 * @brief Module initialization state
 */
static volatile bool Example_ModuleInitialized = false;

/**
 * @brief Last sensor readings
 */
static Spi_Sensor_ReadResultType Example_LastReadings[SPI_SENSOR_MAX_CHANNELS];

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

/**
 * @brief Initialize hardware (SPI pins, clocks)
 *
 * @safety Must be called before any SPI operation
 */
static void Example_InitHardware(void);

/**
 * @brief Initialize SPI sensor module
 *
 * @return Spi_Sensor_StatusType
 */
static Spi_Sensor_StatusType Example_InitSensorModule(void);

/**
 * @brief Cyclic task function
 *
 * @note Called periodically from OS timer or main loop
 */
static void Example_CyclicTask(void);

/**
 * @brief Process sensor readings
 *
 * @param readings   Array of sensor readings
 * @param numReadings Number of readings
 *
 * @safety Data validity checked before use
 */
static void Example_ProcessReadings(
    const Spi_Sensor_ReadResultType* const readings,
    uint8_t numReadings
);

/**
 * @brief Handle sensor error
 *
 * @param channel    Channel where error occurred
 * @param status     Error status
 *
 * @safety Error handling prevents use of invalid data
 */
static void Example_HandleError(uint8_t channel, Spi_Sensor_StatusType status);

/**
 * @brief Get system time in milliseconds
 *
 * @return Current system time (ms)
 */
static uint32_t Example_GetSystemTime(void);

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

/**
 * @brief Application entry point
 *
 * @description Main function for the example application.
 *
 * @safety System startup sequence followed
 */
int main(void)
{
    Spi_Sensor_StatusType status;
    uint32_t initRetryCount = 0U;

    /* Step 1: Initialize hardware */
    Example_InitHardware();

    /* Step 2: Initialize sensor module */
    do {
        status = Example_InitSensorModule();

        if (SPI_SENSOR_STATUS_OK != status) {
            initRetryCount++;

            /* Handle initialization error */
            if (initRetryCount >= MAX_INIT_RETRY) {
                /* Critical error - could enter safe state */
                /* For example: disable outputs, set error indicators */
                while (1) {
                    /* Safe state: system halted or minimal operation */
                    /* In real application, might use watchdog reset */
                }
            }
        }
    } while ((SPI_SENSOR_STATUS_OK != status) &&
             (initRetryCount < MAX_INIT_RETRY));

    /* Step 3: Start scheduler (if using RTOS) */
    /* OsIf_StartScheduler(); */

    /* Step 4: Main loop (for bare-metal) */
    while (1) {
        Example_CyclicTask();

        /* In real application, this would be:
         * - OS-controlled cyclic task
         * - Timer interrupt driven
         * - Or with proper sleep/wake mechanisms
         */
    }

    return 0;
}

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static void Example_InitHardware(void)
{
    /*
     * In real S32K3 application:
     *
     * 1. Initialize clocks:
     *    - Enable LPSPI clock (PCC module)
     *    - Configure system clocks
     *
     * 2. Configure pins (SIUL2):
     *    - MISO, MOSI, SCK, CS pins
     *    - Set mux to LPSPI function
     *    - Configure pull-ups/downs as needed
     *
     * 3. Initialize timer for system tick:
     *    - Configure LPIT or STM
     *    - Set tick period (e.g., 1ms)
     *
     * Example (using S32K3 SDK):
     *
     * PCC_EnableClock(PCC_LPSPI0_IDX);
     *
     * SIUL2_Ip_Init(&SIUL2_Ip_Config);
     *
     * OsIf_Init(NULL_PTR);
     */
}

static Spi_Sensor_StatusType Example_InitSensorModule(void)
{
    Spi_Sensor_StatusType status;
    Spi_Sensor_ConfigType hwConfig;
    Spi_Sensor_ApiConfigType apiConfig;

    /* Step 1: Create hardware configuration */
    /* Using explicit initialization instead of macro for C99 compatibility */
    hwConfig.instance = SPI_SENSOR_CFG_INSTANCE;
    hwConfig.csPin = SPI_SENSOR_CFG_CS_PIN;
    hwConfig.baudrate = SPI_SENSOR_CFG_BAUDRATE;
    hwConfig.cpol = SPI_SENSOR_CFG_CPOL;
    hwConfig.cpha = SPI_SENSOR_CFG_CPHA;
    hwConfig.csToClkDelay_ns = SPI_SENSOR_CFG_CS_TO_CLK_DELAY_NS;
    hwConfig.clkToCsDelay_ns = SPI_SENSOR_CFG_CLK_TO_CS_DELAY_NS;
    hwConfig.interTransferDelay_ns = SPI_SENSOR_CFG_INTER_TRANSFER_DELAY_NS;

    /* Step 2: Initialize driver layer */
    status = Spi_Sensor_Driver_Init(&hwConfig);

    if (SPI_SENSOR_STATUS_OK != status) {
        return status;
    }

    /* Step 3: Verify sensor communication */
    status = Spi_Sensor_Driver_VerifyCommunication();

    if (SPI_SENSOR_STATUS_OK != status) {
        /* Communication failed - possible HW issue */
        return status;
    }

    /* Step 4: Create API configuration */
    /* Using explicit initialization instead of macro for C99 compatibility */
    apiConfig.channels[0].registerAddress = SPI_SENSOR_CFG_CH1_REGISTER_ADDR;
    apiConfig.channels[0].periodMs = SPI_SENSOR_CFG_CH1_PERIOD_MS;
    apiConfig.channels[0].enabled = SPI_SENSOR_CFG_CH1_ENABLED;

    apiConfig.channels[1].registerAddress = SPI_SENSOR_CFG_CH2_REGISTER_ADDR;
    apiConfig.channels[1].periodMs = SPI_SENSOR_CFG_CH2_PERIOD_MS;
    apiConfig.channels[1].enabled = SPI_SENSOR_CFG_CH2_ENABLED;

    apiConfig.channels[2].registerAddress = SPI_SENSOR_CFG_CH3_REGISTER_ADDR;
    apiConfig.channels[2].periodMs = SPI_SENSOR_CFG_CH3_PERIOD_MS;
    apiConfig.channels[2].enabled = SPI_SENSOR_CFG_CH3_ENABLED;

    apiConfig.numberOfChannels = SPI_SENSOR_CFG_NUM_CHANNELS;

    /* Step 5: Initialize API layer */
    status = Spi_Sensor_Api_Init(&apiConfig);

    if (SPI_SENSOR_STATUS_OK != status) {
        return status;
    }

    /* Step 6: Reset diagnostic counters */
    (void)Spi_Sensor_Api_ResetDiagCounters();

    /* Step 7: Module successfully initialized */
    Example_ModuleInitialized = true;

    return SPI_SENSOR_STATUS_OK;
}

static void Example_CyclicTask(void)
{
    Spi_Sensor_StatusType status;
    uint32_t currentTime;
    uint8_t i;
    Spi_Sensor_ReadResultType reading;
    static uint32_t lastDiagCheck = 0U;  /* Static must be at top of function */

    /* Check if module is initialized */
    if (false == Example_ModuleInitialized) {
        /* Module not initialized - cannot proceed */
        return;
    }

    /* Get current time */
    currentTime = Example_GetSystemTime();

    /* Call API cyclic function */
    status = Spi_Sensor_Api_Cyclic(currentTime);

    if (SPI_SENSOR_STATUS_OK != status) {
        /* Cyclic processing error - log or handle */
        /* In real application, might use DET reporting */
    }

    /* Check if we need to process readings */
    /* For this example, we check all channels on each cycle */
    /* In real application, might only process when data is due */

    for (i = 0U; i < SPI_SENSOR_CFG_NUM_CHANNELS; i++) {
        /* Get reading from channel */
        status = Spi_Sensor_Api_GetReading(i, &reading);

        if (SPI_SENSOR_STATUS_OK == status) {
            /* Store reading locally */
            Example_LastReadings[i] = reading;

            /* Check if data is valid before use */
            if (true == reading.validity.dataValid) {
                /* Data is valid - can use it */
                /* Example: reading.registerValue contains the sensor data */

                /* Check for staleness */
                if (true == reading.validity.dataStale) {
                    /* Data is old - might want to flag this */
                }

                /* Process the valid data */
                Example_ProcessReadings(&reading, 1U);
            }
            else {
                /* Data not valid - check status */
                if (SPI_SENSOR_STATUS_TIMEOUT == reading.status) {
                    Example_HandleError(i, SPI_SENSOR_STATUS_TIMEOUT);
                }
                else if (SPI_SENSOR_STATUS_CRC_ERROR == reading.status) {
                    Example_HandleError(i, SPI_SENSOR_STATUS_CRC_ERROR);
                }
                else {
                    Example_HandleError(i, reading.status);
                }
            }
        }
    }

    /* Periodically check diagnostics (e.g., every 100ms) */
    if ((currentTime - lastDiagCheck) >= 100U) {
        Spi_Sensor_DiagCountersType counters;

        if (SPI_SENSOR_STATUS_OK == Spi_Sensor_Api_GetDiagCounters(&counters)) {
            /* Check error counters */
            if (counters.timeoutCount > 0U) {
                /* Handle timeout errors */
            }
            if (counters.crcErrorCount > 0U) {
                /* Handle CRC errors */
            }
            if (counters.commErrorCount > 0U) {
                /* Handle communication errors */
            }
        }

        lastDiagCheck = currentTime;
    }
}

static void Example_ProcessReadings(
    const Spi_Sensor_ReadResultType* const readings,
    uint8_t numReadings)
{
    uint8_t i;

    /*
     * In real application, this would:
     * 1. Process sensor data
     * 2. Convert to physical units if needed
     * 3. Perform application logic
     * 4. Update outputs or control signals
     *
     * Example use cases:
     * - Temperature sensor: update temperature value
     * - Pressure sensor: convert to pressure, check limits
     * - Accelerometer: compute orientation, detect events
     */

    for (i = 0U; i < numReadings; i++) {
        /* Example: Use sensor value */
        uint8_t sensorValue = readings[i].registerValue;

        /* Suppress unused warning in demo */
        (void)sensorValue;
        /* In real application, process the sensor value here */
    }
}

static void Example_HandleError(uint8_t channel, Spi_Sensor_StatusType status)
{
    /* Suppress unused parameter warning for demo */
    (void)channel;
    /*
     * In real application, error handling would include:
     *
     * 1. Log the error (DET, error logger, etc.)
     * 2. Increment error counters
     * 3. Take corrective action:
     *    - Retry operation
     *    - Switch to redundant sensor
     *    - Enter degraded mode
     *    - Set safe state
     *
     * Example actions:
     */

    switch (status) {
        case SPI_SENSOR_STATUS_TIMEOUT:
            /* Communication timeout */
            /* - Check sensor power */
            /* - Verify connections */
            /* - Consider sensor fault */
            break;

        case SPI_SENSOR_STATUS_CRC_ERROR:
            /* Data corruption detected */
            /* - Retry read */
            /* - Check for noise issues */
            break;

        case SPI_SENSOR_STATUS_HW_ERROR:
            /* Hardware error */
            /* - Disable sensor */
            /* - Enter safe mode */
            break;

        case SPI_SENSOR_STATUS_NOT_INIT:
            /* Module not initialized */
            /* - Should not happen in normal operation */
            /* - Consider reinitialization */
            break;

        default:
            /* Other errors */
            break;
    }
}

static uint32_t Example_GetSystemTime(void)
{
    /*
     * In real application:
     *
     * - FreeRTOS: xTaskGetTickCount() * portTICK_PERIOD_MS
     * - AUTOSAR: Std_GetTime()
     * - Bare-metal: Read timer counter (LPIT, STM, etc.)
     *
     * Example using FreeRTOS:
     * return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
     */

    /* Placeholder - would return actual system time */
    static uint32_t dummyTime = 0U;
    return ++dummyTime;
}

/*============================================================================*/
/* Error Handling Hooks (AUTOSAR-style)                                       */
/*============================================================================*/

/**
 * @brief Development Error Trace hook
 *
 * @description Called when a development error is detected.
 *              Implement based on your error reporting mechanism.
 *
 * @param apiId  Function ID where error occurred
 * @param error  Error code
 *
 * @note Only active if DET (Development Error Tracer) is enabled
 * @note Marked unused for demo (placeholder for AUTOSAR integration)
 */
static void Spi_Sensor_Det_ReportError(uint16_t apiId, uint8_t error) __attribute__((unused));

static void Spi_Sensor_Det_ReportError(uint16_t apiId, uint8_t error)
{
    /*
     * In real application:
     * - Call Det_ReportError()
     * - Log to error buffer
     * - Set error flag
     * - May trigger safe state
     */

    (void)apiId;
    (void)error;

    /* Prevent unused warning */
}

/**
 * @brief Production Error Notification hook
 *
 * @description Called when a production error is detected.
 *
 * @param apiId  Function ID where error occurred
 *
 * @note Only active if production error notification is enabled
 * @note Marked unused for demo (placeholder for AUTOSAR integration)
 */
static void Spi_Sensor_Pev_NotifyError(uint16_t apiId) __attribute__((unused));

static void Spi_Sensor_Pev_NotifyError(uint16_t apiId)
{
    /*
     * In real application:
     * - Log production error
     * - May trigger fault handling
     */

    (void)apiId;
}

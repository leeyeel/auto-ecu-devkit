/**
 * @file Spi_Sensor_Example_S32K4.c
 * @brief Example application demonstrating SPI register read on S32K4
 *
 * @description This example demonstrates how to use the SPI HAL layer
 *              to read registers from an external SPI sensor.
 *
 *              Target: NXP S32K4 series automotive MCUs
 *              SPI:    LPSPI (Low Power SPI)
 *
 * @ assumptions & constraints
 *   - S32K4 SDK (RTD) is available and configured
 *   - SPI pins are configured via SIUL2
 *   - System clock is configured (typically 80 MHz)
 *   - External SPI sensor is connected
 *
 * @ safety considerations
 *   - Timeout protection on all SPI transfers
 *   - Error handling for all operations
 *   - Data validation before use
 *
 * @ execution context
 *   - Main function: Task context (or main() loop)
 *   - Cyclic functions: Called from periodic task/loop
 *
 * @ verification checklist
 *   - [ ] SPI communication verified with logic analyzer
 *   - [ ] Timeout behavior tested
 *   - [ ] Static analysis passes (MISRA C:2012)
 *   - [ ] Integration with S32K4 SDK verified
 */

/*============================================================================*/
/* Includes                                                                   */
/*============================================================================*/

#include "Spi_Sensor_Hal_S32K4.h"
#include "Spi_Sensor_Cfg_S32K4.h"
#include "Spi_Sensor_Types.h"

/*
 * Note: In actual S32K4 project, include SDK headers:
 * #include "Lpspi_Ip.h"
 * #include "Siul2_Ip.h"
 * #include "Mcu.h"           /* For clock initialization */
/* #include "Platform.h"      /* For STARTUP, RTE */

/*============================================================================*/
/* Private Constants                                                          */
/*============================================================================*/

/**
 * @brief Number of registers to read in demo
 */
#define EXAMPLE_NUM_REGISTERS   (4U)

/**
 * @brief Register addresses for demo
 *
 * @note Replace with actual sensor register addresses.
 *       These are example addresses for demonstration.
 */
#define EXAMPLE_REG_WHO_AM_I    (0x0FU)
#define EXAMPLE_REG_CTRL1       (0x20U)
#define EXAMPLE_REG_CTRL2       (0x21U)
#define EXAMPLE_REG_OUT_X_L     (0x28U)

/**
 * @brief Delay between demo cycles (milliseconds)
 */
#define EXAMPLE_CYCLE_DELAY_MS  (1000U)

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

/**
 * @brief SPI configuration instance
 */
static Spi_Sensor_ConfigType SpiConfig;

/**
 * @brief Storage for register read values
 */
static uint8_t RegValues[EXAMPLE_NUM_REGISTERS];

/**
 * @brief Register addresses to read
 */
static const uint8_t RegAddresses[EXAMPLE_NUM_REGISTERS] = {
    EXAMPLE_REG_WHO_AM_I,
    EXAMPLE_REG_CTRL1,
    EXAMPLE_REG_CTRL2,
    EXAMPLE_REG_OUT_X_L
};

/**
 * @brief Register names for display (debug)
 */
static const char* const RegNames[EXAMPLE_NUM_REGISTERS] = {
    "WHO_AM_I",
    "CTRL1",
    "CTRL2",
    "OUT_X_L"
};

/**
 * @brief Demo cycle counter
 */
static uint32_t CycleCounter = 0U;

/**
 * @brief Demo error counter
 */
static uint32_t ErrorCounter = 0U;

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

/**
 * @brief Initialize SPI for sensor communication
 *
 * @return Spi_Sensor_StatusType Initialization status
 *
 * @safety All parameters validated before use
 */
static Spi_Sensor_StatusType Example_InitSpi(void);

/**
 * @brief Read multiple registers from sensor
 *
 * @param regAddr   Array of register addresses
 * @param values    Buffer for received values
 * @param count     Number of registers to read
 *
 * @return Spi_Sensor_StatusType Read status
 *
 * @safety Timeout and error handling on each transfer
 */
static Spi_Sensor_StatusType Example_ReadRegisters(
    const uint8_t* const regAddr,
    uint8_t* const values,
    uint8_t count
);

/**
 * @brief Verify sensor communication
 *
 * @description Reads WHO_AM_I register and verifies expected value.
 *
 * @return true if communication OK, false otherwise
 *
 * @safety Returns safe default on failure
 */
static bool Example_VerifySensor(void);

/**
 * @brief Print register values (for debugging)
 *
 * @param names     Array of register names
 * @param values    Register values
 * @param count     Number of registers
 *
 * @reentrant Yes
 * @note Only for debugging - remove in production
 */
static void Example_PrintValues(
    const char* const names[],
    const uint8_t values[],
    uint8_t count
);

/**
 * @brief Get system time in milliseconds
 *
 * @description Placeholder - replace with actual implementation.
 *              Could use OS tick counter or hardware timer.
 *
 * @return Current time in milliseconds
 *
 * @reentrant Yes
 */
static uint32_t Example_GetSystemTimeMs(void);

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

/**
 * @brief Main entry point for SPI sensor demo
 *
 * @description Demonstrates initialization, verification, and
 *              cyclic reading of sensor registers.
 *
 * @return int Exit status (0 = success)
 *
 * @pre  System clocks configured
 * @pre  SPI pins muxed
 * @post Sensor communication demonstrated
 */
int main(void)
{
    Spi_Sensor_StatusType status;
    uint32_t startTime;
    uint32_t currentTime;
    uint32_t cycleStartMs;

    /*
     * Demo Sequence:
     * 1. Initialize SPI
     * 2. Verify sensor communication (read WHO_AM_I)
     * 3. Cyclic: Read registers every second
     * 4. Print results
     */

    /* Initialize SPI */
    status = Example_InitSpi();

    if (SPI_SENSOR_STATUS_OK != status) {
        /* Initialization failed - in production, would enter safe state */
        /* For demo, just increment error counter */
        ErrorCounter++;
        /* Return error code */
        return (int)status;
    }

    /* Verify sensor communication */
    if (false == Example_VerifySensor()) {
        /* Sensor verification failed */
        ErrorCounter++;
        /* Continue anyway to demonstrate read attempts */
    }

    /* Main demo loop */
    cycleStartMs = Example_GetSystemTimeMs();

    while (1U) {
        /* Track cycle timing */
        startTime = Example_GetSystemTimeMs();

        /* Increment cycle counter */
        CycleCounter++;

        /*
         * Read multiple registers
         * In production, this would be called from cyclic task
         */
        status = Example_ReadRegisters(
            RegAddresses,
            RegValues,
            EXAMPLE_NUM_REGISTERS
        );

        if (SPI_SENSOR_STATUS_OK == status) {
            /* Print values (for debugging/demo) */
            Example_PrintValues(RegNames, RegValues, EXAMPLE_NUM_REGISTERS);
        }
        else {
            /* Read failed - increment error counter */
            ErrorCounter++;
        }

        /* Wait for next cycle */
        currentTime = Example_GetSystemTimeMs();

        /* Simple busy-wait for demo (replace with OS delay in production) */
        while ((currentTime - cycleStartMs) < EXAMPLE_CYCLE_DELAY_MS) {
            currentTime = Example_GetSystemTimeMs();
        }

        cycleStartMs = currentTime;
    }

    /* Never reached - keep compiler happy */
    return 0;
}

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static Spi_Sensor_StatusType Example_InitSpi(void)
{
    Spi_Sensor_StatusType status;

    /*
     * Initialize SPI configuration from compile-time settings.
     * Configuration is created from preprocessor definitions in Spi_Sensor_Cfg_S32K4.h
     */
    SpiConfig = SPI_SENSOR_CFG_CREATE_CONFIG();

    /*
     * Initialize the SPI HAL layer.
     * This configures the LPSPI peripheral with:
     * - Selected instance (LPSPI0, LPSPI1, etc.)
     * - Baudrate (clock speed)
     * - Clock polarity and phase
     * - Timing delays
     *
     * Note: Before calling this, ensure:
     * 1. System clocks are configured (CCGE module)
     * 2. SPI pins are muxed (SIUL2 module)
     */
    status = Spi_Sensor_Hal_S32K4_Init(&SpiConfig);

    return status;
}

static Spi_Sensor_StatusType Example_ReadRegisters(
    const uint8_t* const regAddr,
    uint8_t* const values,
    uint8_t count)
{
    Spi_Sensor_StatusType status = SPI_SENSOR_STATUS_OK;
    uint8_t i;

    /* Parameter validation */
    if (NULL == regAddr) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (NULL == values) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else if (0U == count) {
        status = SPI_SENSOR_STATUS_INVALID_PARAM;
    }
    else {
        /* Continue with read operations */
    }

    /*
     * Read each register.
     * For each register:
     * 1. Transmit READ command + register address
     * 2. Receive data byte(s)
     *
     * Note: Some sensors support burst read (multiple consecutive
     * addresses). For those, you could optimize by reading all
     * registers in one transfer.
     */
    for (i = 0U; i < count; i++) {
        if (SPI_SENSOR_STATUS_OK == status) {
            /* Read single register */
            status = Spi_Sensor_Hal_S32K4_ReadRegister(
                SpiConfig.instance,          /* SPI instance */
                regAddr[i],                  /* Register address */
                &values[i],                  /* Buffer for value */
                1U,                          /* Read 1 byte */
                SPI_SENSOR_CFG_DEFAULT_TIMEOUT_MS
            );
        }
        else {
            /* Previous read failed - break early */
            break;
        }
    }

    return status;
}

static bool Example_VerifySensor(void)
{
    Spi_Sensor_StatusType status;
    uint8_t whoAmI;

    /*
     * Verify sensor by reading WHO_AM_I register.
     * Most sensors have a fixed device ID at this register.
     * Comparing against expected value confirms:
     * 1. SPI communication is working
     * 2. Sensor is responding
     * 3. Correct sensor type is connected
     */

    status = Spi_Sensor_Hal_S32K4_ReadRegister(
        SpiConfig.instance,
        SPI_SENSOR_CFG_WHO_AM_I_ADDR,
        &whoAmI,
        1U,
        SPI_SENSOR_CFG_DEFAULT_TIMEOUT_MS
    );

    if (SPI_SENSOR_STATUS_OK == status) {
        /* Compare with expected value */
        if (whoAmI == SPI_SENSOR_CFG_WHO_AM_I_EXPECTED) {
            /* Verification passed */
            return true;
        }
        else {
            /* Wrong sensor or communication error */
            /* Log unexpected value */
            (void)whoAmI;  /* Could be used for debug */
            return false;
        }
    }

    /* Read failed */
    return false;
}

static void Example_PrintValues(
    const char* const names[],
    const uint8_t values[],
    uint8_t count)
{
    uint8_t i;

    /* Print cycle counter and error counter */
    (void)names;  /* Not used in production - for debug only */

    /*
     * Note: In production, remove or replace with proper logging.
     * Standard printf is typically not available in safety-critical
     * automotive applications.
     *
     * Use:
     * - AUTOSAR Det/Dlt for development
     * - Runtime diagnostic interface for production
     */

    for (i = 0U; i < count; i++) {
        /*
         * In actual implementation, use:
         * Det_ReportError() for errors
         * Dlt_Log() for debug output
         * Custom diagnostic interface
         */
        (void)values[i];
    }
}

static uint32_t Example_GetSystemTimeMs(void)
{
    /*
     * Get system time in milliseconds.
     *
     * In actual S32K4 implementation, options include:
     *
     * 1. OS Tick (FreeRTOS/AUTOSAR OS):
     *    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
     *
     * 2. LPTMR or PIT timer:
     *    return Lptmr_GetElapsedTimeMs();
     *
     * 3. System Core Timer (if available):
     *    return SysTick_GetMs();
     *
     * For demo, return simple counter.
     */
    static uint32_t timeCounter = 0U;

    /* Increment and return (not accurate, just for demo) */
    timeCounter++;

    return timeCounter;
}

/*============================================================================*/
/* Integration Notes                                                          */
/*============================================================================*/

/*
 * To integrate this demo into your S32K4 project:
 *
 * 1. SDK Configuration (S32DS or Lauterbach):
 *    - Enable LPSPI in SDK Configurator
 *    - Configure SPI pins (MOSI, MISO, SCK, PCS)
 *    - Set up clock configuration (CCGE)
 *
 * 2. Pin Muxing (SIUL2):
 *    - Configure GPIO for SPI functions
 *    - Ensure proper drive strength for high-speed SPI
 *
 * 3. Memory Configuration:
 *    - Ensure code fits in flash
 *    - Stack size adequate for SPI operations
 *
 * 4. Safety Configuration:
 *    - Configure watchdog (WDOG) for timeout protection
 *    - Set up error detection (MCGM, RCCU)
 *    - Implement safe state handler
 *
 * 5. For AUTOSAR Integration:
 *    - Use Spi_Ip driver from RTD
 *    - Configure Spi driver stack
 *    - Implement SchM for critical sections
 *
 * 6. For FreeRTOS Integration:
 *    - Call from periodic task
 *    - Use mutex for SPI access protection
 *    - Consider DMA for efficient transfers
 */

/* End of file */

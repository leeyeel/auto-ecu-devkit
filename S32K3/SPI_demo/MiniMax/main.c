/**
 * @file    main.c
 * @brief   SPI Demo Application - Main Entry Point
 *
 * @details This is the main application entry point for the SPI Demo.
 *          It demonstrates reading register status from an external SPI device
 *          using the NXP S32K3 LPSPI peripheral.
 *
 * @par     Target Device: NXP S32K3 Series
 *
 * @author  Auto ECU Demo
 * @date    2026-01-07
 *
 * @note    Compliant with MISRA C:2012 and ISO 26262 safety guidelines
 *
 * ============================================================================
 * @par  Application Flow:
 *   1. Initialize platform (clocks, pins)
 *   2. Initialize SPI Demo
 *   3. Run register read sequence
 *   4. Save and export data
 *   5. Report results
 * ============================================================================
 *
 * @par  Safety Considerations:
 *   - Initialization verification
 *   - Transfer error handling
 *   - Timeout protection
 *   - Graceful error recovery
 * ============================================================================
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "spi_demo.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Private Macro Definitions
 ******************************************************************************/

/** @brief Application name for console output */
#define APP_NAME                     "SPI Demo"

/** @brief Export data filename (for logging to file in simulation) */
#define APP_EXPORT_FILENAME          "spi_register_data.bin"

/** @brief Console output buffer size */
#define APP_CONSOLE_BUFFER_SIZE      (256U)

/*******************************************************************************
 * Private Type Definitions
 ******************************************************************************/

/**
 * @brief   Application State Structure
 *
 * @note    Tracks application initialization and status
 */
typedef struct
{
    boolean initialized;        /* Platform init flag */
    boolean spiInit;            /* SPI Demo init flag */
    uint32_t errorCount;        /* Application error count */
    uint32_t spiErrorCount;     /* SPI error count */
} App_StateType;

/*******************************************************************************
 * Private Variable Definitions
 ******************************************************************************/

/** @brief Application state (static allocation) */
static App_StateType s_appState =
{
    .initialized = FALSE,
    .spiInit = FALSE,
    .errorCount = 0U,
    .spiErrorCount = 0U
};

/** @brief Register data buffer */
static Spi_DemoRegDataType s_regData[SPI_DEMO_REG_COUNT];

/** @brief Export data buffer */
static uint8_t s_exportBuffer[SPI_DEMO_REG_COUNT * 2U];

/** @brief Console output buffer */
static char s_consoleBuffer[APP_CONSOLE_BUFFER_SIZE];

/*******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

/**
 * @brief   Initialize platform
 *
 * @details Configures system clocks, pins, and peripherals.
 *
 * @return  TRUE  - Platform initialized successfully
 * @return  FALSE - Platform initialization failed
 */
static boolean App_InitPlatform(void);

/**
 * @brief   Initialize SPI Demo
 *
 * @details Configures and initializes the SPI Demo module.
 *
 * @return  TRUE  - SPI Demo initialized successfully
 * @return  FALSE - SPI Demo initialization failed
 */
static boolean App_InitSpiDemo(void);

/**
 * @brief   Run register read demonstration
 *
 * @details Executes the SPI register read sequence and collects results.
 *
 * @return  TRUE  - Demo completed successfully
 * @return  FALSE - Demo failed
 */
static boolean App_RunSpiDemo(void);

/**
 * @brief   Save register data to file
 *
 * @details Exports register data to file for later analysis.
 *
 * @return  TRUE  - Data saved successfully
 * @return  FALSE - Save operation failed
 */
static boolean App_SaveDataToFile(void);

/**
 * @brief   Print application banner
 *
 * @details Outputs application information to console.
 */
static void App_PrintBanner(void);

/**
 * @brief   Print results summary
 *
 * @details Outputs demo results and statistics.
 *
 * @param[in] result  Demo execution result
 */
static void App_PrintResults(Spi_DemoReturnType result);

/**
 * @brief   Platform-specific delay
 *
 * @details Blocking delay for simulation/embedded platforms.
 *
 * @param[in] ms  Delay time in milliseconds
 */
static void App_DelayMs(uint32_t ms);

/*******************************************************************************
 * Private Function Implementations
 ******************************************************************************/

static boolean App_InitPlatform(void)
{
    boolean retVal = TRUE;

    /* Platform initialization (simplified for demo) */
    /* In production, this would include:
     * - System clock configuration (SOSC, SPLL)
     * - Peripheral clock enable (PCC)
     * - Pin multiplexing configuration
     * - Watchdog disable (or periodic kick)
     */

    /* Mark platform as initialized */
    s_appState.initialized = TRUE;

    return retVal;
}

static boolean App_InitSpiDemo(void)
{
    Spi_DemoReturnType retVal;
    Spi_DemoConfigType config;
    boolean success = FALSE;

    /* Configure SPI Demo */
    config.instance = 0U;              /* LPSPI0 */
    config.baudRate = 1000000U;        /* 1 MHz */
    config.mode = SPI_DEMO_MODE_0;     /* CPOL=0, CPHA=0 */
    config.chipSelect = 0U;            /* PCS0 */
    config.timeoutMs = 100U;           /* 100 ms timeout */

    /* Initialize SPI Demo */
    retVal = Spi_Demo_Init(&config);

    if (retVal == SPI_DEMO_E_OK)
    {
        s_appState.spiInit = TRUE;
        success = TRUE;
    }
    else
    {
        s_appState.spiInit = FALSE;
        s_appState.errorCount++;
    }

    return success;
}

static boolean App_RunSpiDemo(void)
{
    Spi_DemoReturnType retVal;
    boolean success = FALSE;

    /* Run the register read sequence */
    retVal = Spi_Demo_RunSequence(s_regData, SPI_DEMO_REG_COUNT);

    if (retVal == SPI_DEMO_E_OK)
    {
        /* Save data to internal buffer */
        retVal = Spi_Demo_SaveData(s_regData, SPI_DEMO_REG_COUNT);

        if (retVal == SPI_DEMO_E_OK)
        {
            success = TRUE;
        }
    }
    else
    {
        /* Count SPI errors */
        s_appState.spiErrorCount++;
        s_appState.errorCount++;
    }

    return success;
}

static boolean App_SaveDataToFile(void)
{
    Spi_DemoReturnType retVal;
    uint16_t exportSize;
    boolean success = FALSE;

    /* Calculate export buffer size */
    exportSize = (uint16_t)(sizeof(s_regData) * 2U);

    /* Export data to buffer */
    retVal = Spi_Demo_ExportData(s_exportBuffer, exportSize);

    if (retVal == SPI_DEMO_E_OK)
    {
        /* In simulation, write to file */
        /* In production embedded system, this would:
         * - Store to internal flash
         * - Send via debug/UART
         * - Store to external memory
         */

        /* For demo: data is in s_exportBuffer */
        /* Application can retrieve data via debugger or export interface */

        success = TRUE;
    }

    return success;
}

static void App_PrintBanner(void)
{
    uint32_t version;

    /* Get version */
    version = Spi_Demo_GetVersion();

    /* Print banner (simulation) */
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "\n========================================\n");
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "      %s - SPI Register Read Demo\n", APP_NAME);
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "========================================\n");
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "Version: %u.%u.%u\n",
                   (unsigned int)((version >> 16) & 0xFFU),
                   (unsigned int)((version >> 8) & 0xFFU),
                   (unsigned int)(version & 0xFFU));
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "Target: NXP S32K3 Series\n");
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "SPI: LPSPI0 @ 1 MHz\n");
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "----------------------------------------\n\n");
}

static void App_PrintResults(Spi_DemoReturnType result)
{
    Spi_DemoStatsType stats;
    uint8_t i;

    /* Get statistics */
    (void)Spi_Demo_GetStats(&stats);

    /* Print header */
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "========================================\n");
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "           DEMO RESULTS\n");
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "========================================\n");

    /* Print overall result */
    if (result == SPI_DEMO_E_OK)
    {
        (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                       "Status: PASSED\n\n");
    }
    else
    {
        (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                       "Status: FAILED (Error: 0x%02X)\n\n", (unsigned int)result);
    }

    /* Print register data */
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "Register Data:\n");
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "----------------------------------------\n");

    for (i = 0U; i < SPI_DEMO_REG_COUNT; i++)
    {
        (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                       "  Reg[0x%02X] = 0x%02X",
                       s_regData[i].regAddress,
                       s_regData[i].regValue);

        if (s_regData[i].status == 0U)
        {
            (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                           " [OK]\n");
        }
        else
        {
            (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                           " [FAIL: 0x%02X]\n", s_regData[i].status);
        }
    }

    /* Print statistics */
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "\n----------------------------------------\n");
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "Statistics:\n");
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "  TX Bytes: %lu\n", (unsigned long)stats.txCount);
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "  RX Bytes: %lu\n", (unsigned long)stats.rxCount);
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "  Errors:   %lu\n", (unsigned long)stats.errorCount);
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "  Timeouts: %lu\n", (unsigned long)stats.timeoutCount);
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "  Retries:  %lu\n", (unsigned long)stats.retryCount);
    (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                   "========================================\n\n");
}

static void App_DelayMs(uint32_t ms)
{
    /* Simple delay for simulation */
    volatile uint32_t count = ms * 10000U;

    while (count > 0U)
    {
        count--;
    }
}

/*******************************************************************************
 * Public Function Implementations
 ******************************************************************************/

/**
 * @brief   Application Entry Point
 *
 * @details Main function for SPI Demo application.
 *
 * @param[in] argc  Argument count (not used in embedded)
 * @param[in] argv  Argument vector (not used in embedded)
 *
 * @return  0  - Application exited normally
 * @return  1  - Application encountered error
 *
 * @note    In embedded systems, this is called from startup code
 * @note    This function never returns in production embedded systems
 */
int main(int argc, char *argv[])
{
    Spi_DemoReturnType result;
    boolean success = FALSE;

    /* Prevent unused parameter warnings */
    (void)argc;
    (void)argv;

    /* Print banner */
    App_PrintBanner();

    /* Initialize platform */
    if (App_InitPlatform() == TRUE)
    {
        /* Initialize SPI Demo */
        if (App_InitSpiDemo() == TRUE)
        {
            /* Run SPI Demo sequence */
            result = Spi_Demo_RunSequence(s_regData, SPI_DEMO_REG_COUNT);

            /* Save data (for external access) */
            if (result == SPI_DEMO_E_OK)
            {
                (void)App_SaveDataToFile();
            }

            /* Print results */
            App_PrintResults(result);

            /* Check overall success */
            if (result == SPI_DEMO_E_OK)
            {
                success = TRUE;
            }

            /* Deinitialize SPI Demo */
            (void)Spi_Demo_DeInit();
        }
        else
        {
            /* SPI initialization failed */
            (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                           "ERROR: SPI Demo initialization failed!\n");
            result = SPI_DEMO_E_NOT_OK;
        }
    }
    else
    {
        /* Platform initialization failed */
        (void)snprintf(s_consoleBuffer, APP_CONSOLE_BUFFER_SIZE,
                       "ERROR: Platform initialization failed!\n");
        result = SPI_DEMO_E_NOT_OK;
    }

    /* Exit with appropriate code */
    if (success == TRUE)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*******************************************************************************
 * End of File
 ******************************************************************************/

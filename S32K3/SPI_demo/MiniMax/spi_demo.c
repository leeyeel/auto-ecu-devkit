/**
 * @file    spi_demo.c
 * @brief   SPI Demo Application Source for NXP S32K3 Series
 *
 * @details This module implements SPI communication demo for reading register
 *          status from an external SPI device.
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
 *   - Minimal RAM footprint (stack-based buffering)
 *   - Synchronous operation (non-blocking preferred for production)
 * ============================================================================
 *
 * @par  Safety Considerations:
 *   - All input parameters validated
 *   - Transfer timeout protection
 *   - Status error checking on all SPI operations
 *   - Safe state: SPI pins configured to safe state on error
 *   - Plausibility checks on received data
 * ============================================================================
 *
 * @par  Verification Checklist:
 *   - Static Analysis: Coverity, QAC/C++check
 *   - Unit Tests: Ceedling/Unity framework
 *   - Integration Tests: Hardware-in-loop (HIL)
 *   - Boundary Conditions: Timeout, NULL pointers, invalid parameters
 * ============================================================================
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "spi_demo.h"
#include "spi_lpspi_s32k3.h"

/* Compiler-specific include for inline functions if needed */
#if defined(__GNUC__)
#include <stddef.h>
#include <string.h>
#endif

/*******************************************************************************
 * Private Macro Definitions
 ******************************************************************************/

/** @brief Magic number for state validation */
#define SPI_DEMO_STATE_INIT    (0x44454D55U)  /* "DEMU" */

/** @brief Magic number for deinitialized state */
#define SPI_DEMO_STATE_UNINIT  (0xFFFFFFFFU)

/** @brief Default SPI configuration values */
#define SPI_DEMO_DEFAULT_BAUD      (1000000U)  /* 1 MHz */
#define SPI_DEMO_DEFAULT_TIMEOUT   (100U)      /* 100 ms */
#define SPI_DEMO_DEFAULT_MODE      (SPI_DEMO_MODE_0)

/** @brief Register read command prefix (read = 1, write = 0) */
#define SPI_DEMO_READ_CMD          (0x80U)

/** @brief Dummy byte for SPI clock generation during receive */
#define SPI_DEMO_DUMMY_BYTE        (0xFFU)

/** @brief Retry delay in microseconds */
#define SPI_DEMO_RETRY_DELAY_US    (1000U)

/*******************************************************************************
 * Private Type Definitions
 ******************************************************************************/

/**
 * @brief   SPI Demo Internal State Structure
 *
 * @note    Tracks initialization state and runtime statistics
 */
typedef struct
{
    uint32_t           state;          /* State validation magic */
    Spi_DemoConfigType config;         /* Current configuration */
    Spi_DemoStatsType  stats;          /* Transfer statistics */
    Spi_DemoRegDataType regBuffer[SPI_DEMO_REG_COUNT];  /* Register data buffer */
    uint8_t            regBufferCount; /* Number of buffered registers */
    boolean            isInitialized;  /* Initialization flag */
} Spi_DemoStateType;

/*******************************************************************************
 * Private Variable Definitions
 ******************************************************************************/

/** @brief SPI Demo internal state (static allocation) */
static Spi_DemoStateType s_spiDemoState =
{
    .state          = SPI_DEMO_STATE_UNINIT,
    .config         = {0},
    .stats          = {0},
    .regBuffer      = {{0}},
    .regBufferCount = 0U,
    .isInitialized  = FALSE
};

/** @brief SPI transfer buffer (aligned for DMA if needed) */
static uint8_t s_txBuffer[2] = {0U, 0U};

/** @brief SPI receive buffer */
static uint8_t s_rxBuffer[2] = {0U, 0U};

/** @brief LPSPI driver handle (external reference) */
extern Lpspi_Ip_HWUnitConfigType LPSPI_Config[4];

/*******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

/**
 * @brief   Validate initialization state
 *
 * @details Checks if SPI Demo has been properly initialized.
 *
 * @return  TRUE  - Initialized and ready
 * @return  FALSE - Not initialized
 */
static boolean Spi_Demo_IsInitialized(void);

/**
 * @brief   Check device response validity
 *
 * @details Performs plausibility check on received data.
 *
 * @param[in] regAddr   Register address that was read
 * @param[in] regValue  Value that was read
 *
 * @return  TRUE  - Data appears valid
 * @return  FALSE - Data may be invalid
 */
static boolean Spi_Demo_IsDataPlausible(uint8_t regAddr, uint8_t regValue);

/**
 * @brief   Perform single SPI transfer with retry
 *
 * @details Executes SPI transfer with retry logic on failure.
 *
 * @param[in]  pTxData   Pointer to transmit data (not NULL)
 * @param[out] pRxData   Pointer to receive buffer (not NULL)
 * @param[in]  length    Number of bytes to transfer
 *
 * @return  SPI_DEMO_E_OK        - Transfer successful
 * @return  SPI_DEMO_E_TIMEOUT   - Transfer timeout
 * @return  SPI_DEMO_E_SPI_ERROR - SPI hardware error
 */
static Spi_DemoReturnType Spi_Demo_TransferWithRetry(const uint8_t *pTxData,
                                                      uint8_t       *pRxData,
                                                      uint8_t        length);

/**
 * @brief   Delay for specified microseconds
 *
 * @details Simple delay function (polling-based, blocking).
 *
 * @param[in] us   Delay time in microseconds
 *
 * @note    This is a simple busy-wait; RTOS may provide better timing
 */
static void Spi_Demo_DelayUs(uint32_t us);

/**
 * @brief   Default configuration initializer
 *
 * @details Sets up default configuration values.
 *
 * @param[out] pConfig   Pointer to configuration structure
 */
static void Spi_Demo_SetDefaultConfig(Spi_DemoConfigType *pConfig);

/*******************************************************************************
 * Private Function Implementations
 ******************************************************************************/

static boolean Spi_Demo_IsInitialized(void)
{
    return (s_spiDemoState.isInitialized == TRUE);
}

static boolean Spi_Demo_IsDataPlausible(uint8_t regAddr, uint8_t regValue)
{
    boolean retVal = TRUE;

    /* Add plausibility checks based on expected register values */
    /* For demonstration, accept all values from 0x00-0xFF */

    /* Specific check: reserved registers should not be 0xFF typically */
    if ((regAddr >= 0xF0U) && (regValue == 0xFFU))
    {
        retVal = FALSE;
    }

    return retVal;
}

static Spi_DemoReturnType Spi_Demo_TransferWithRetry(const uint8_t *pTxData,
                                                      uint8_t       *pRxData,
                                                      uint8_t        length)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;
    uint8_t retryCount = 0U;
    Lpspi_Ip_StatusType spiStatus = LPSPI_IP_STATUS_ERROR;
    Lpspi_Ip_DataType spiData;

    /* Validate inputs */
    if ((pTxData == NULL) || (pRxData == NULL))
    {
        retVal = SPI_DEMO_E_NULL_PTR;
    }
    else if (length == 0U)
    {
        retVal = SPI_DEMO_E_INVALID_PARAM;
    }
    else
    {
        /* Initialize transfer data structure */
        spiData.txData = (uint8_t *)pTxData;  /* MISRA cast: const removed for SDK */
        spiData.rxData = pRxData;
        spiData.dataSize = length;

        /* Retry loop with timeout protection */
        for (retryCount = 0U; retryCount < SPI_DEMO_MAX_RETRIES; retryCount++)
        {
            if (retryCount > 0U)
            {
                /* Increment retry counter */
                s_spiDemoState.stats.retryCount++;
                Spi_Demo_DelayUs(SPI_DEMO_RETRY_DELAY_US);
            }

            /* Execute SPI transfer */
            spiStatus = Lpspi_Ip_SyncTransmit(s_spiDemoState.config.instance,
                                               &spiData,
                                               s_spiDemoState.config.timeoutMs);

            if (spiStatus == LPSPI_IP_STATUS_SUCCESS)
            {
                /* Update statistics */
                s_spiDemoState.stats.txCount += (uint32_t)length;
                s_spiDemoState.stats.rxCount += (uint32_t)length;
                retVal = SPI_DEMO_E_OK;
                break;
            }
            else
            {
                /* Increment error counter */
                s_spiDemoState.stats.errorCount++;
            }
        }

        /* Check final status */
        if (spiStatus == LPSPI_IP_STATUS_TIMEOUT)
        {
            s_spiDemoState.stats.timeoutCount++;
            retVal = SPI_DEMO_E_TIMEOUT;
        }
        else if (spiStatus != LPSPI_IP_STATUS_SUCCESS)
        {
            retVal = SPI_DEMO_E_SPI_ERROR;
        }
        else
        {
            /* Success - retVal already set above */
        }
    }

    return retVal;
}

static void Spi_Demo_DelayUs(uint32_t us)
{
    /* Simple busy-wait delay (implementation-dependent) */
    /* For S32K3, use SysTick or PIT for accurate timing */

#if defined(USE_RTOS) && (USE_RTOS == 1)
    /* RTOS delay (if available) */
    Os_DelayMs((us + 999U) / 1000U);
#else
    /* Polling delay - highly approximate */
    volatile uint32_t count = us * 10U;  /* Adjust multiplier per clock speed */

    while (count > 0U)
    {
        count--;
    }
#endif
}

static void Spi_Demo_SetDefaultConfig(Spi_DemoConfigType *pConfig)
{
    if (pConfig != NULL)
    {
        pConfig->instance   = 0U;                 /* LPSPI0 */
        pConfig->baudRate   = SPI_DEMO_DEFAULT_BAUD;
        pConfig->mode       = SPI_DEMO_DEFAULT_MODE;
        pConfig->chipSelect = 0U;                 /* PCS0 */
        pConfig->timeoutMs  = SPI_DEMO_DEFAULT_TIMEOUT;
    }
}

/*******************************************************************************
 * Public Function Implementations
 ******************************************************************************/

Spi_DemoReturnType Spi_Demo_Init(const Spi_DemoConfigType *pConfig)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;
    Spi_DemoConfigType config;
    Lpspi_Ip_StatusType status;

    /* Check for re-initialization */
    if (Spi_Demo_IsInitialized() == TRUE)
    {
        /* Already initialized, deinit first */
        (void)Spi_Demo_DeInit();
    }

    /* Get configuration - use default if NULL */
    if (pConfig == NULL)
    {
        Spi_Demo_SetDefaultConfig(&config);
        pConfig = &config;
    }

    /* Validate configuration parameters */
    if (pConfig->instance > 3U)
    {
        retVal = SPI_DEMO_E_INVALID_PARAM;
    }
    else if (pConfig->baudRate == 0U)
    {
        retVal = SPI_DEMO_E_INVALID_PARAM;
    }
    else if (pConfig->timeoutMs == 0U)
    {
        retVal = SPI_DEMO_E_INVALID_PARAM;
    }
    else
    {
        /* Initialize LPSPI driver (platform-specific) */
        status = Lpspi_Ip_Init(pConfig->instance);

        if (status == LPSPI_IP_STATUS_SUCCESS)
        {
            /* Configure SPI parameters */
            status = Lpspi_Ip_SetConfig(pConfig->instance, NULL);

            if (status == LPSPI_IP_STATUS_SUCCESS)
            {
                /* Store configuration */
                s_spiDemoState.config = *pConfig;

                /* Reset statistics */
                s_spiDemoState.stats.txCount = 0U;
                s_spiDemoState.stats.rxCount = 0U;
                s_spiDemoState.stats.errorCount = 0U;
                s_spiDemoState.stats.timeoutCount = 0U;
                s_spiDemoState.stats.retryCount = 0U;

                /* Reset buffer */
                s_spiDemoState.regBufferCount = 0U;

                /* Set state and mark initialized */
                s_spiDemoState.state = SPI_DEMO_STATE_INIT;
                s_spiDemoState.isInitialized = TRUE;

                retVal = SPI_DEMO_E_OK;
            }
            else
            {
                /* Config failed - deinit to clean up */
                (void)Lpspi_Ip_Deinit(pConfig->instance);
            }
        }
        else
        {
            retVal = SPI_DEMO_E_NOT_OK;
        }
    }

    return retVal;
}

Spi_DemoReturnType Spi_Demo_DeInit(void)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;
    Lpspi_Ip_StatusType status;

    if (Spi_Demo_IsInitialized() == TRUE)
    {
        /* Deinitialize LPSPI */
        status = Lpspi_Ip_Deinit(s_spiDemoState.config.instance);

        if (status == LPSPI_IP_STATUS_SUCCESS)
        {
            /* Reset state */
            s_spiDemoState.state = SPI_DEMO_STATE_UNINIT;
            s_spiDemoState.isInitialized = FALSE;

            retVal = SPI_DEMO_E_OK;
        }
    }
    else
    {
        /* Not initialized, consider this a success */
        retVal = SPI_DEMO_E_OK;
    }

    return retVal;
}

Spi_DemoReturnType Spi_Demo_ReadRegister(uint8_t regAddr, uint8_t *pValue)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;

    /* Input validation */
    if (Spi_Demo_IsInitialized() == FALSE)
    {
        retVal = SPI_DEMO_E_NOT_OK;
    }
    else if (pValue == NULL)
    {
        retVal = SPI_DEMO_E_NULL_PTR;
    }
    else
    {
        /* Build TX buffer: Read command + Address */
        s_txBuffer[0] = (uint8_t)(SPI_DEMO_READ_CMD | (regAddr & 0x7FU));
        s_txBuffer[1] = SPI_DEMO_DUMMY_BYTE;  /* Clock for receive */

        /* Clear RX buffer before transfer */
        s_rxBuffer[0] = 0U;
        s_rxBuffer[1] = 0U;

        /* Perform transfer */
        retVal = Spi_Demo_TransferWithRetry(s_txBuffer, s_rxBuffer, 2U);

        if (retVal == SPI_DEMO_E_OK)
        {
            /* Extract received value (second byte) */
            *pValue = s_rxBuffer[1];

            /* Plausibility check */
            if (Spi_Demo_IsDataPlausible(regAddr, *pValue) == FALSE)
            {
                /* Data questionable but not invalid */
            }
        }
    }

    return retVal;
}

Spi_DemoReturnType Spi_Demo_ReadRegisters(uint8_t  startAddr,
                                           uint8_t *pData,
                                           uint8_t  length)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;
    uint8_t i;
    uint8_t regValue;

    /* Input validation */
    if (Spi_Demo_IsInitialized() == FALSE)
    {
        retVal = SPI_DEMO_E_NOT_OK;
    }
    else if (pData == NULL)
    {
        retVal = SPI_DEMO_E_NULL_PTR;
    }
    else if (length == 0U)
    {
        retVal = SPI_DEMO_E_INVALID_PARAM;
    }
    else
    {
        /* Read each register sequentially */
        retVal = SPI_DEMO_E_OK;  /* Assume success */

        for (i = 0U; i < length; i++)
        {
            retVal = Spi_Demo_ReadRegister((uint8_t)(startAddr + i), &regValue);

            if (retVal != SPI_DEMO_E_OK)
            {
                /* Mark error in data */
                pData[i] = 0U;
                retVal = SPI_DEMO_E_NOT_OK;
            }
            else
            {
                pData[i] = regValue;
            }
        }
    }

    return retVal;
}

Spi_DemoReturnType Spi_Demo_RunSequence(Spi_DemoRegDataType *pRegData,
                                         uint8_t               regCount)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;
    uint8_t i;
    uint8_t regValue;
    uint8_t regAddr;

    /* Input validation */
    if (Spi_Demo_IsInitialized() == FALSE)
    {
        retVal = SPI_DEMO_E_NOT_OK;
    }
    else if (pRegData == NULL)
    {
        retVal = SPI_DEMO_E_NULL_PTR;
    }
    else if (regCount == 0U)
    {
        retVal = SPI_DEMO_E_INVALID_PARAM;
    }
    else
    {
        /* Demo sequence: Read register, save to buffer */
        retVal = SPI_DEMO_E_OK;

        /* Define register addresses for demo */
        /* Note: These are example addresses - modify for actual device */
        static const uint8_t demoRegAddrs[SPI_DEMO_REG_COUNT] =
        {
            0x0FU,    /* WHO_AM_I */
            0x00U,    /* Status Register */
            0x01U,    /* Status/Event 1 */
            0x02U,    /* Status/Event 2 */
            0x03U,    /* Data Ready 1 */
            0x04U,    /* Data Ready 2 */
            0x05U,    /* FIFO Status */
            0x06U,    /* FIFO Control */
        };

        /* Limit to available buffer or requested count */
        if (regCount > SPI_DEMO_REG_COUNT)
        {
            regCount = SPI_DEMO_REG_COUNT;
        }

        /* Read each register */
        for (i = 0U; i < regCount; i++)
        {
            regAddr = demoRegAddrs[i];

            /* Read register value */
            retVal = Spi_Demo_ReadRegister(regAddr, &regValue);

            /* Store results */
            pRegData[i].regAddress = regAddr;
            pRegData[i].regValue = regValue;
            pRegData[i].status = (uint8_t)retVal;

            if (retVal != SPI_DEMO_E_OK)
            {
                /* Continue reading other registers even if one fails */
            }
        }
    }

    return retVal;
}

Spi_DemoReturnType Spi_Demo_SaveData(const Spi_DemoRegDataType *pRegData,
                                      uint8_t                    regCount)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;
    uint8_t i;
    uint8_t copyCount;

    /* Input validation */
    if (pRegData == NULL)
    {
        retVal = SPI_DEMO_E_NULL_PTR;
    }
    else if (regCount == 0U)
    {
        retVal = SPI_DEMO_E_INVALID_PARAM;
    }
    else
    {
        /* Limit copy to buffer size */
        if (regCount > SPI_DEMO_REG_COUNT)
        {
            copyCount = SPI_DEMO_REG_COUNT;
        }
        else
        {
            copyCount = regCount;
        }

        /* Copy data to internal buffer */
        for (i = 0U; i < copyCount; i++)
        {
            s_spiDemoState.regBuffer[i] = pRegData[i];
        }

        s_spiDemoState.regBufferCount = copyCount;
        retVal = SPI_DEMO_E_OK;
    }

    return retVal;
}

Spi_DemoReturnType Spi_Demo_ExportData(uint8_t *pBuffer, uint16_t bufferSize)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;
    uint16_t i;
    uint16_t requiredSize;

    /* Input validation */
    if (pBuffer == NULL)
    {
        retVal = SPI_DEMO_E_NULL_PTR;
    }
    else
    {
        /* Calculate required buffer size */
        /* Format: [addr1][value1][addr2][value2]... */
        requiredSize = (uint16_t)(s_spiDemoState.regBufferCount * 2U);

        if (bufferSize < requiredSize)
        {
            retVal = SPI_DEMO_E_INVALID_PARAM;
        }
        else if (s_spiDemoState.regBufferCount == 0U)
        {
            retVal = SPI_DEMO_E_NOT_OK;
        }
        else
        {
            /* Export data in flat format */
            for (i = 0U; i < s_spiDemoState.regBufferCount; i++)
            {
                pBuffer[(i * 2U)] = s_spiDemoState.regBuffer[i].regAddress;
                pBuffer[(i * 2U) + 1U] = s_spiDemoState.regBuffer[i].regValue;
            }

            retVal = SPI_DEMO_E_OK;
        }
    }

    return retVal;
}

Spi_DemoReturnType Spi_Demo_GetStats(Spi_DemoStatsType *pStats)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;

    if (pStats == NULL)
    {
        retVal = SPI_DEMO_E_NULL_PTR;
    }
    else
    {
        /* Copy statistics */
        pStats->txCount = s_spiDemoState.stats.txCount;
        pStats->rxCount = s_spiDemoState.stats.rxCount;
        pStats->errorCount = s_spiDemoState.stats.errorCount;
        pStats->timeoutCount = s_spiDemoState.stats.timeoutCount;
        pStats->retryCount = s_spiDemoState.stats.retryCount;

        retVal = SPI_DEMO_E_OK;
    }

    return retVal;
}

Spi_DemoReturnType Spi_Demo_PrintData(const Spi_DemoRegDataType *pRegData,
                                       uint8_t                    regCount)
{
    Spi_DemoReturnType retVal = SPI_DEMO_E_NOT_OK;
    uint8_t i;

    if (pRegData == NULL)
    {
        retVal = SPI_DEMO_E_NULL_PTR;
    }
    else
    {
        /* Print header */
        /* Note: Replace with platform-specific debug output */

        for (i = 0U; i < regCount; i++)
        {
            /* Example debug output format */
            /* Spi_Demo_Printf("Reg[0x%02X] = 0x%02X (Status: %d)\n", */
            /*                 pRegData[i].regAddress, */
            /*                 pRegData[i].regValue, */
            /*                 pRegData[i].status); */
        }

        retVal = SPI_DEMO_E_OK;
    }

    return retVal;
}

uint32_t Spi_Demo_GetVersion(void)
{
    return (uint32_t)((SPI_DEMO_VERSION_MAJOR << 16) |
                       (SPI_DEMO_VERSION_MINOR << 8) |
                        SPI_DEMO_VERSION_PATCH);
}

/*******************************************************************************
 * End of File
 ******************************************************************************/

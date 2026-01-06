/**
 * @file Soc_Timer_S32K4.c
 * @brief LPIT Timer driver implementation for S32K4
 * @module Soc_Timer_S32K4
 *
 * @assumptions & constraints
 *   - S32K4 LPIT peripheral available at 0x40360000
 *   - Clock: SIRC divided to ~1MHz for 1us resolution
 *   - Using LPIT Channel 0 for SOC timing
 *
 * @safety considerations
 *   - Timer drives critical SOC estimation function
 *   - Interrupt must be cleared to prevent overflow
 *   - Watchdog must be fed during initialization
 *
 * @execution context
 *   - Init: Task context
 *   - ISR: High-priority interrupt context
 *   - GetTick: Task context (may be called from ISR)
 *
 * @version 1.0.0
 * @compliance MISRA C:2012, ISO 26262
 */

#include "Soc_Timer_S32K4.h"
#include "Soc_Cfg.h"
#include <stdbool.h>

/*============================================================================*/
/* Compiler Abstraction                                                      */
/*============================================================================*/

/**
 * @brief Critical section enter (disable interrupts)
 */
#define SOC_TIMER_ENTER_CRITICAL()   __asm volatile ("CPSID i" ::: "memory")

/**
 * @brief Critical section exit (enable interrupts)
 */
#define SOC_TIMER_EXIT_CRITICAL()    __asm volatile ("CPSIE i" ::: "memory")

/**
 * @brief Memory barrier
 */
#define SOC_TIMER_MEMORY_BARRIER()   __asm volatile ("DSB" ::: "memory"); \
                                     __asm volatile ("ISB" ::: "memory")

/*============================================================================*/
/* Register Access Macros                                                    */
/*============================================================================*/

/**
 * @brief Read from LPIT register
 */
#define SOC_LPIT_READ_REG(offset) \
    (*(volatile uint32_t *)(SOC_LPIT_BASE_ADDR + (offset)))

/**
 * @brief Write to LPIT register
 */
#define SOC_LPIT_WRITE_REG(offset, value) \
    (*(volatile uint32_t *)(SOC_LPIT_BASE_ADDR + (offset)) = (uint32_t)(value))

/**
 * @brief Read from LPIT channel register
 */
#define SOC_LPIT_READ_CH_REG(channel, offset) \
    (*(volatile uint32_t *)(SOC_LPIT_BASE_ADDR + SOC_LPIT_CH0_OFFSET + \
     ((channel) * SOC_LPIT_CH_OFFSET) + (offset)))

/**
 * @brief Write to LPIT channel register
 */
#define SOC_LPIT_WRITE_CH_REG(channel, offset, value) \
    (*(volatile uint32_t *)(SOC_LPIT_BASE_ADDR + SOC_LPIT_CH0_OFFSET + \
     ((channel) * SOC_LPIT_CH_OFFSET) + (offset)) = (uint32_t)(value))

/*============================================================================*/
/* Local Variables                                                           */
/*============================================================================*/

/**
 * @brief Timer initialization state
 */
static bool s_TimerInitialized = false;

/**
 * @brief Timer running state
 */
static bool s_TimerRunning = false;

/**
 * @brief Timer period in microseconds
 */
static uint32_t s_TimerPeriodUs = 0U;

/**
 * @brief Timer frequency in Hz
 */
static uint32_t s_TimerFreqHz = 0U;

/**
 * @brief Timer callback function
 */
static void (*s_CallbackFunc)(void) = NULL;

/**
 * @brief Interrupt flag
 */
static volatile bool s_IrqFlag = false;

/**
 * @brief Tick counter for SOC calculation
 */
static volatile uint32_t s_TickCount = 0U;

/**
 * @brief Last tick value for elapsed calculation
 */
static uint32_t s_LastTick = 0U;

/*============================================================================*/
/* Local Function Prototypes                                                 */
/*============================================================================*/

/**
 * @brief Configure clock for LPIT
 *
 * @return SOC_STATUS_OK on success
 */
static Soc_StatusType Soc_Timer_S32K4_ConfigClock(void);

/**
 * @brief Configure interrupt for LPIT channel
 *
 * @return SOC_STATUS_OK on success
 */
static Soc_StatusType Soc_Timer_S32K4_ConfigInterrupt(void);

/**
 * @brief Enable LPIT module
 */
static void Soc_Timer_S32K4_EnableModule(void);

/**
 * @brief Disable LPIT module
 */
static void Soc_Timer_S32K4_DisableModule(void);

/*============================================================================*/
/* Clock Configuration                                                       */
/*============================================================================*/

/**
 * @brief Configure LPIT clock source
 *
 * @description S32K4 LPIT uses SIRC divided by configurable divider.
 *              Default configuration uses SIRC at 8MHz divided by 8 = 1MHz.
 *
 * @return SOC_STATUS_OK on success
 *
 * @note This is a simplified version. In production, use SDK functions.
 */
static Soc_StatusType Soc_Timer_S32K4_ConfigClock(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /*
     * S32K4 Clock Control (CCGE) Configuration:
     *
     * PCC[LPTMR0] = clock_source | divider | enable
     *
     * Clock source options:
     *   0 = FXOSC (external crystal)
     *   1 = SIRCDIV1 (SIRC divided by 1 = 8MHz)
     *   2 = SIRCDIV2 (SIRC divided by 2 = 4MHz)
     *   3 = SIRCDIV3 (SIRC divided by 8 = 1MHz)
     *
     * Using SIRCDIV3 directly at 1MHz for simplicity.
     */

    /* PCC base address and offsets for LPIT0 */
    #define SOC_PCC_BASE_ADDR       (0x403C0000U)
    #define SOC_PCC_LPIT0_INDEX     (68U)  /* LPIT0 index in PCC */
    #define SOC_PCC_OFFSET          (0x0004U)  /* Offset per peripheral */

    #define SOC_PCC_CGC_MASK        (0x1U << 30)  /* Clock gate control */
    #define SOC_PCC_SRC_MASK        (0x3U << 24)  /* Clock source */

    #if (SOC_CFG_TIMER_CLOCK_SOURCE == 0U)
        #define SOC_PCC_SRC_VAL     (0U << 24)   /* FXOSC */
        #define SOC_LPIT_SOURCE_FREQ (16000000U) /* Assume 16MHz FXOSC */
    #elif (SOC_CFG_TIMER_CLOCK_SOURCE == 1U)
        #define SOC_PCC_SRC_VAL     (1U << 24)   /* SIRCDIV1 */
        #define SOC_LPIT_SOURCE_FREQ (8000000U)  /* 8MHz */
    #elif (SOC_CFG_TIMER_CLOCK_SOURCE == 2U)
        #define SOC_PCC_SRC_VAL     (2U << 24)   /* SIRCDIV2 */
        #define SOC_LPIT_SOURCE_FREQ (4000000U)  /* 4MHz */
    #else
        #define SOC_PCC_SRC_VAL     (3U << 24)   /* SIRCDIV3 */
        #define SOC_LPIT_SOURCE_FREQ (1000000U)  /* 1MHz */
    #endif

    /*
     * PCC[LPTMR0] Configuration:
     * In production, use SDK function: PCC_SetClockSource(PCC, kPCC_Lptmr0_Group, ...)
     *
     * This code shows the register configuration approach.
     */

    /* Enable clock gate for LPIT0 */
    volatile uint32_t *pcc_lpit = (volatile uint32_t *)(SOC_PCC_BASE_ADDR +
                                         SOC_PCC_LPIT0_INDEX * SOC_PCC_OFFSET);

    /* Read-modify-write to enable clock */
    *pcc_lpit |= SOC_PCC_CGC_MASK;   /* Enable clock */
    *pcc_lpit |= SOC_PCC_SRC_VAL;    /* Set clock source */

    /* Calculate timer frequency */
    s_TimerFreqHz = SOC_LPIT_SOURCE_FREQ / SOC_CFG_TIMER_CLOCK_DIVIDER;

    return status;
}

/*============================================================================*/
/* Interrupt Configuration                                                   */
/*============================================================================*/

/**
 * @brief Configure LPIT interrupt
 *
 * @description Enables NVIC interrupt for LPIT Channel 0.
 *
 * @return SOC_STATUS_OK on success
 */
static Soc_StatusType Soc_Timer_S32K4_ConfigInterrupt(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /*
     * S32K4 NVIC Configuration for LPIT0 Channel 0:
     * IRQ number: LPIT0_Ch0_IRQn = 111
     *
     * In production, use SDK function:
     * NVIC_SetPriority(LPIT0_Ch0_IRQn, SOC_CFG_TIMER_IRQ_PRIORITY);
     * NVIC_EnableIRQ(LPIT0_Ch0_IRQn);
     *
     * Below shows the NVIC register access approach.
     */

    /* NVIC base address */
    #define SOC_NVIC_BASE_ADDR      (0xE000E100U)
    #define SOC_NVIC_ISER_OFFSET    (0x0000U)   /* Interrupt Set Enable */
    #define SOC_NVIC_ICER_OFFSET    (0x0080U)   /* Interrupt Clear Enable */
    #define SOC_NVIC_ISPR_OFFSET    (0x0100U)   /* Interrupt Set Pending */
    #define SOC_NVIC_ICPR_OFFSET    (0x0180U)   /* Interrupt Clear Pending */
    #define SOC_NVIC_IPR_OFFSET     (0x0300U)   /* Interrupt Priority */

    #define SOC_LPIT0_CH0_IRQN      (111U)      /* LPIT0 Channel 0 IRQ */

    /* Calculate register and bit positions */
    uint32_t regOffset = (SOC_LPIT0_CH0_IRQN / 32U) * 4U;
    uint32_t bitMask = 1U << (SOC_LPIT0_CH0_IRQN % 32U);

    /* Set priority (in IPR register) */
    volatile uint8_t *nvic_ipr = (volatile uint8_t *)(SOC_NVIC_BASE_ADDR +
                                        SOC_NVIC_IPR_OFFSET + SOC_LPIT0_CH0_IRQN);

    /* S32K4 uses 4 bits per priority, upper 4 bits valid */
    *nvic_ipr = (uint8_t)(SOC_CFG_TIMER_IRQ_PRIORITY << 4);

    /* Enable interrupt in ISER */
    volatile uint32_t *nvic_iser = (volatile uint32_t *)(SOC_NVIC_BASE_ADDR +
                                         SOC_NVIC_ISER_OFFSET + regOffset);
    *nvic_iser |= bitMask;

    return status;
}

/*============================================================================*/
/* Module Control                                                            */
/*============================================================================*/

/**
 * @brief Enable LPIT module
 */
static void Soc_Timer_S32K4_EnableModule(void)
{
    /* Enable module by setting M_CEN in MCR */
    uint32_t mcr = SOC_LPIT_READ_REG(SOC_LPIT_MCR_OFFSET);
    mcr |= SOC_LPIT_MCR_M_CEN;
    SOC_LPIT_WRITE_REG(SOC_LPIT_MCR_OFFSET, mcr);
}

/**
 * @brief Disable LPIT module
 */
static void Soc_Timer_S32K4_DisableModule(void)
{
    /* Disable module by clearing M_CEN in MCR */
    uint32_t mcr = SOC_LPIT_READ_REG(SOC_LPIT_MCR_OFFSET);
    mcr &= ~SOC_LPIT_MCR_M_CEN;
    SOC_LPIT_WRITE_REG(SOC_LPIT_MCR_OFFSET, mcr);
}

/*============================================================================*/
/* Public Functions                                                          */
/*============================================================================*/

Soc_StatusType Soc_Timer_S32K4_Init(uint32_t periodUs)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /* Parameter validation */
    if ((periodUs < 100U) || (periodUs > 1000000U))
    {
        status = SOC_STATUS_INVALID_PARAM;
    }
    else if (s_TimerInitialized == true)
    {
        /* Already initialized */
        status = SOC_STATUS_ERROR;
    }
    else
    {
        /* Store period */
        s_TimerPeriodUs = periodUs;

        /* Configure clock */
        status = Soc_Timer_S32K4_ConfigClock();

        if (status == SOC_STATUS_OK)
        {
            /* Disable module during configuration */
            Soc_Timer_S32K4_DisableModule();

            /* Clear any pending interrupts */
            SOC_LPIT_WRITE_REG(SOC_LPIT_MSR_OFFSET, 0xFU);

            /* Configure channel 0 for 32-bit periodic mode */
            uint32_t ch0Ctrl = SOC_LPIT_CH_CTRL_MODE(SOC_LPIT_MODE_32BIT_PERIODIC);
            SOC_LPIT_WRITE_CH_REG(0U, 0x00U, ch0Ctrl);  /* TCTRL0 at offset 0x00 */

            /* Calculate reload value for desired period */
            /* Timer ticks = period_us * timer_freq_hz / 1000000 */
            uint32_t reloadValue = (periodUs * s_TimerFreqHz) / 1000000U;

            /* Load value into TVAL register */
            SOC_LPIT_WRITE_CH_REG(0U, 0x04U, reloadValue);  /* TVAL0 at offset 0x04 */

            /* Enable interrupt for channel 0 */
            uint32_t mier = SOC_LPIT_READ_REG(SOC_LPIT_MIER_OFFSET);
            mier |= 0x1U;  /* Enable channel 0 interrupt */
            SOC_LPIT_WRITE_REG(SOC_LPIT_MIER_OFFSET, mier);

            /* Configure interrupt in NVIC */
            status = Soc_Timer_S32K4_ConfigInterrupt();

            if (status == SOC_STATUS_OK)
            {
                /* Enable module */
                Soc_Timer_S32K4_EnableModule();

                /* Initialize state */
                s_TimerInitialized = true;
                s_TickCount = 0U;
                s_LastTick = 0U;
            }
        }
    }

    return status;
}

Soc_StatusType Soc_Timer_S32K4_Start(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    if (s_TimerInitialized == false)
    {
        status = SOC_STATUS_NOT_INITIALIZED;
    }
    else if (s_TimerRunning == true)
    {
        /* Already running */
        status = SOC_STATUS_ERROR;
    }
    else
    {
        /* Enable timer via SETTEN register */
        SOC_LPIT_WRITE_REG(SOC_LPIT_SETTEN_OFFSET, 0x1U);

        /* Manually set T_EN in channel control */
        uint32_t ch0Ctrl = SOC_LPIT_READ_CH_REG(0U, 0x00U);
        ch0Ctrl |= SOC_LPIT_CH_CTRL_T_EN;
        SOC_LPIT_WRITE_CH_REG(0U, 0x00U, ch0Ctrl);

        s_TimerRunning = true;
    }

    return status;
}

Soc_StatusType Soc_Timer_S32K4_Stop(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    if (s_TimerRunning == true)
    {
        /* Disable timer via CLRTEN register */
        SOC_LPIT_WRITE_REG(SOC_LPIT_CLRTEN_OFFSET, 0x1U);

        /* Manually clear T_EN in channel control */
        uint32_t ch0Ctrl = SOC_LPIT_READ_CH_REG(0U, 0x00U);
        ch0Ctrl &= ~SOC_LPIT_CH_CTRL_T_EN;
        SOC_LPIT_WRITE_CH_REG(0U, 0x00U, ch0Ctrl);

        s_TimerRunning = false;
    }

    return status;
}

uint32_t Soc_Timer_S32K4_GetTick(void)
{
    /* Read current value from timer (counts down) */
    uint32_t currentVal = SOC_LPIT_READ_CH_REG(0U, 0x08U);  /* CVAL0 at offset 0x08 */

    /* Convert to count-up by subtracting from reload value */
    uint32_t reloadVal = SOC_LPIT_READ_CH_REG(0U, 0x04U);  /* TVAL */

    /* Return count-up value */
    return reloadVal - currentVal;
}

uint32_t Soc_Timer_S32K4_GetFreqHz(void)
{
    return s_TimerFreqHz;
}

uint32_t Soc_Timer_S32K4_GetElapsedUs(uint32_t startTick)
{
    uint32_t currentTick = Soc_Timer_S32K4_GetTick();
    uint32_t elapsedTick;

    /* Handle wrap-around */
    if (currentTick >= startTick)
    {
        elapsedTick = currentTick - startTick;
    }
    else
    {
        elapsedTick = (uint32_t)(0xFFFFFFFFU - startTick) + currentTick + 1U;
    }

    /* Convert to microseconds */
    return (elapsedTick * 1000000U) / s_TimerFreqHz;
}

Soc_StatusType Soc_Timer_S32K4_RegisterCallback(void (*callback)(void))
{
    Soc_StatusType status = SOC_STATUS_OK;

#if (SOC_CFG_PARAM_VALIDATION_ENABLED == 1U)
    if (callback == NULL)
    {
        status = SOC_STATUS_NULL_PTR;
    }
    else
#endif
    {
        s_CallbackFunc = callback;
    }

    return status;
}

bool Soc_Timer_S32K4_GetIrqFlag(void)
{
    return s_IrqFlag;
}

void Soc_Timer_S32K4_ClearIrqFlag(void)
{
    s_IrqFlag = false;
}

void Soc_Timer_S32K4_Isr(void)
{
    /* Set interrupt flag */
    s_IrqFlag = true;

    /* Increment tick counter */
    s_TickCount++;

    /* Call registered callback if any */
    if (s_CallbackFunc != NULL)
    {
        s_CallbackFunc();
    }

    /* Clear interrupt flag in MSR */
    SOC_LPIT_WRITE_REG(SOC_LPIT_MSR_OFFSET, 0x1U);  /* Clear channel 0 flag */

    /* Memory barrier to ensure ISR completion */
    SOC_TIMER_MEMORY_BARRIER();
}

Soc_StatusType Soc_Timer_S32K4_Deinit(void)
{
    Soc_StatusType status = SOC_STATUS_OK;

    /* Stop timer if running */
    (void)Soc_Timer_S32K4_Stop();

    /* Disable module */
    Soc_Timer_S32K4_DisableModule();

    /* Clear callback */
    s_CallbackFunc = NULL;

    /* Reset state */
    s_TimerInitialized = false;
    s_TimerRunning = false;
    s_TimerPeriodUs = 0U;
    s_TickCount = 0U;

    return status;
}

/*============================================================================*/
/* Timer Interface Implementation                                             */
/*============================================================================*/

/**
 * @brief Timer driver interface
 *
 * @note Provides abstraction for timer operations
 */
const Soc_TimerInterfaceType g_SocTimerInterface =
{
    .Init       = Soc_Timer_S32K4_Init,
    .Start      = Soc_Timer_S32K4_Start,
    .Stop       = Soc_Timer_S32K4_Stop,
    .GetTick    = Soc_Timer_S32K4_GetTick,
    .GetFreqHz  = Soc_Timer_S32K4_GetFreqHz
};

/*============================================================================*/
/* ISR Handler Alias                                                         */
/*============================================================================*/

/**
 * @brief LPIT Channel 0 Interrupt Handler
 *
 * @description This is the actual interrupt handler that should be called
 *              from the platform's interrupt handling code.
 *
 * @note Place this in your interrupt vector table or call from primary ISR:
 *
 *       void LPIT0_Ch0_IRQHandler(void)
 *       {
 *           Soc_Timer_S32K4_Isr();
 *       }
 */
void LPIT0_Ch0_IRQHandler(void)
{
    Soc_Timer_S32K4_Isr();
}

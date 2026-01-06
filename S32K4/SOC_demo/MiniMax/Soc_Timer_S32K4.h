/**
 * @file Soc_Timer_S32K4.h
 * @brief LPIT Timer driver for S32K4
 * @module Soc_Timer_S32K4
 *
 * @assumptions & constraints
 *   - S32K4 LPIT peripheral available
 *   - Clock configuration: SIRC at 8MHz divided by 8 = 1MHz base
 *   - Interrupt priority must be configurable
 *   - Target period: 5ms (5000 microseconds)
 *
 * @safety considerations
 *   - Timer drives 5ms task for battery SOC estimation
 *   - Loss of timer interrupt causes SOC estimation drift
 *   - Interrupt latency affects timing accuracy
 *
 * @execution context
 *   - Init: Task context
 *   - Interrupt: LPIT interrupt (high priority)
 *   - GetTick: Task context (read-only)
 *
 * @version 1.0.0
 * @compliance MISRA C:2012, ISO 26262
 */

#ifndef SOC_TIMER_S32K4_H
#define SOC_TIMER_S32K4_H

#include "Soc_Types.h"

/*============================================================================*/
/* LPIT Hardware Register Addresses (S32K4)                                  */
/*============================================================================*/

/**
 * @brief LPIT base address
 *
 * @note S32K4 has single LPIT with 4 channels
 */
#define SOC_LPIT_BASE_ADDR          (0x40360000U)

/**
 * @brief LPIT register offsets
 */
#define SOC_LPIT_MCR_OFFSET         (0x0000U)  /**< Module Control Register */
#define SOC_LPIT_MSR_OFFSET         (0x0004U)  /**< Module Status Register */
#define SOC_LPIT_MIER_OFFSET        (0x0008U)  /**< Module Interrupt Enable */
#define SOC_LPIT_SETTEN_OFFSET      (0x000CU)  /**< Set Timer Enable Register */
#define SOC_LPIT_CLRTEN_OFFSET      (0x0010U)  /**< Clear Timer Enable Register */
#define SOC_LPIT_CH0_OFFSET         (0x0100U)  /**< Channel 0 registers start */
#define SOC_LPIT_CH_OFFSET          (0x0020U)  /**< Offset per channel */

/*============================================================================*/
/* Register Masks                                                            */
/*============================================================================*/

/**
 * @brief LPIT Module Control Register masks
 */
#define SOC_LPIT_MCR_M_CEN          (0x1U << 0)   /**< Module enable */
#define SOC_LPIT_MCR_SWR_TRIG       (0x1U << 15)  /**< Software trigger */
#define SOC_LPIT_MCR_DBG_EN         (0x1U << 31)  /**< Debug mode enable */

/**
 * @brief LPIT Channel Control Register masks
 */
#define SOC_LPIT_CH_CTRL_T_EN       (0x1U << 0)   /**< Timer enable */
#define SOC_LPIT_CH_CTRL_MODE(n)    (((n) & 0x3U) << 2)  /**< Timer mode */
#define SOC_LPIT_CH_CTRL_TRG_SRC    (0x1U << 4)   /**< Trigger source */
#define SOC_LPIT_CH_CTRL_TRG_VAL    (0x1U << 5)   /**< Trigger value */
#define SOC_LPIT_CH_CTRL_START      (0x1U << 6)   /**< Timer start */
#define SOC_LPIT_CH_CTRL_DOZE_EN    (0x1U << 7)   /**< Doze mode enable */

/**
 * @brief Timer modes
 */
#define SOC_LPIT_MODE_32BIT_PERIODIC   (0U)   /**< 32-bit periodic counter */
#define SOC_LPIT_MODE_32BIT_TRIGGER    (1U)   /**< 32-bit trigger counter */
#define SOC_LPIT_MODE_16BIT_PERIODIC   (2U)   /**< Dual 16-bit periodic */
#define SOC_LPIT_MODE_TRIGGER_CAPTURE  (3U)   /**< Trigger capture */

/*============================================================================*/
/* Timer Interface                                                           */
/*============================================================================*/

/**
 * @brief Timer driver interface
 */
extern const Soc_TimerInterfaceType g_SocTimerInterface;

/*============================================================================*/
/* API Functions                                                             */
/*============================================================================*/

/**
 * @brief Initialize LPIT timer for SOC estimation
 *
 * @description Configures LPIT channel 0 for 5ms periodic interrupt.
 *              Uses 32-bit periodic counter mode.
 *
 * @param periodUs Timer period in microseconds
 *
 * @return Soc_StatusType
 *         - SOC_STATUS_OK: Initialization successful
 *         - SOC_STATUS_INVALID_PARAM: Invalid period
 *         - SOC_STATUS_ERROR: Hardware configuration failed
 *
 * @pre  Clock must be configured for LPIT
 * @post Timer ready to start
 *
 * @safety Timer configuration affects SOC calculation accuracy
 */
Soc_StatusType Soc_Timer_S32K4_Init(uint32_t periodUs);

/**
 * @brief Start LPIT timer
 *
 * @description Enables the timer and interrupt.
 *
 * @return Soc_StatusType
 *         - SOC_STATUS_OK: Timer started
 *         - SOC_STATUS_NOT_INITIALIZED: Timer not initialized
 *
 * @pre  Soc_Timer_S32K4_Init must be called
 * @post Periodic interrupts enabled
 */
Soc_StatusType Soc_Timer_S32K4_Start(void);

/**
 * @brief Stop LPIT timer
 *
 * @description Disables timer and interrupt.
 *
 * @return Soc_StatusType
 *         - SOC_STATUS_OK: Timer stopped
 *
 * @safety Stopping timer pauses SOC estimation
 */
Soc_StatusType Soc_Timer_S32K4_Stop(void);

/**
 * @brief Get current timer tick
 *
 * @description Returns current 32-bit tick count.
 *
 * @return Current tick value (counts up)
 *
 * @note Read-clear on some registers, may affect synchronization
 */
uint32_t Soc_Timer_S32K4_GetTick(void);

/**
 * @brief Get timer frequency
 *
 * @description Returns timer clock frequency in Hz.
 *
 * @return Timer frequency in Hz
 */
uint32_t Soc_Timer_S32K4_GetFreqHz(void);

/**
 * @brief Get elapsed time in microseconds
 *
 * @param startTick Start tick value
 *
 * @return Elapsed time in microseconds
 */
uint32_t Soc_Timer_S32K4_GetElapsedUs(uint32_t startTick);

/**
 * @brief Timer interrupt handler
 *
 * @description LPIT Channel 0 interrupt handler.
 *              Must be called from LPIT0_Ch0_IRQHandler.
 *
 * @note This function should be called from the actual ISR
 */
void Soc_Timer_S32K4_Isr(void);

/**
 * @brief Register SOC update callback
 *
 * @description Registers function to be called on timer interrupt.
 *
 * @param callback Pointer to callback function
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Timer_S32K4_RegisterCallback(void (*callback)(void));

/**
 * @brief Get interrupt flag
 *
 * @description Returns true if timer interrupt flag is set.
 *
 * @return Interrupt flag state
 */
bool Soc_Timer_S32K4_GetIrqFlag(void);

/**
 * @brief Clear interrupt flag
 *
 * @description Clears timer interrupt flag.
 */
void Soc_Timer_S32K4_ClearIrqFlag(void);

/**
 * @brief Deinitialize timer
 *
 * @description Resets timer to default state.
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Timer_S32K4_Deinit(void);

#endif /* SOC_TIMER_S32K4_H */

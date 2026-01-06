/**
 * @file    Can.c
 * @brief   CAN Driver Implementation for MPC5744
 * @details AUTOSAR-like CAN driver for FlexCAN on MPC5744.
 *          Implements CAN initialization, transmission, reception,
 *          and interrupt handling.
 *
 * @note    Compliant with MISRA C:2012 coding standard
 *
 * @version 1.0.0
 * @date    2025-01-07
 *
 * @copyright Copyright (c) 2025
 */

/*
 *****************************************************************************************
 * INCLUDES
 *****************************************************************************************
 */
#include "Can.h"
#include "Can_PBcfg.h"
#include "Mpc5744_Siu_Cfg.h"      /* Platform specific SIU/INTC definitions */
#include "Mpc5744_FlexCAN.h"      /* FlexCAN register definitions */

/*
 *****************************************************************************************
 * FILE VERSION CHECK
 *****************************************************************************************
 */

#if (CAN_VERSION_MAJOR != CAN_CFG_VERSION_MAJOR) || \
    (CAN_VERSION_MINOR != CAN_CFG_VERSION_MINOR)
#error "Version mismatch between Can.h and Can_Cfg.h"
#endif

/*
 *****************************************************************************************
 * MACRO DEFINITIONS
 *****************************************************************************************
 */

/** @brief Macro to calculate register offset for message buffer */
#define CAN_MB_OFFSET_U32(mb)       (((uint32_t)(mb)) * 4U)

/** @brief Macro to extract message buffer number from ESR register */
#define CAN_ESR_BUF_IDX(esr)        (((esr) >> 8U) & 0x3FU)

/** @brief CAN ID standard mask (11-bit) */
#define CAN_ID_STANDARD_MASK        (0x7FFU)

/** @brief CAN ID extended mask (29-bit) */
#define CAN_ID_EXTENDED_MASK        (0x1FFFFFFFU)

/** @brief FlexCAN CTRL1 register default value (freeze, supervisor) */
#define CAN_CTRL1_DEFAULT           (0x00000001UL)

/** @brief FlexCAN MCR register default value (freeze, supervisor) */
#define CAN_MCR_DEFAULT             (0xD890000FUL)

/** @brief FlexCAN message buffer control word mask */
#define CAN_MB_CTRL_MASK            (0x0FFFFFFFUL)

/** @brief Enable MB interrupt bit in CTRL1 */
#define CAN_CTRL1_MB_INT_EN(mb)     (1UL << ((mb) + 16U))

/** @brief Check if bit is set in register */
#define CAN_REG_BIT_IS_SET(reg, bit)  (((reg) & (bit)) != 0U)

/** @brief Check if bit is clear in register */
#define CAN_REG_BIT_IS_CLEAR(reg, bit) (((reg) & (bit)) == 0U)

/*
 *****************************************************************************************
 * TYPE DEFINITIONS
 *****************************************************************************************
 */

/**
 * @brief   FlexCAN Register Structure
 * @details MPC5744 FlexCAN module register layout
 */
typedef volatile struct
{
    uint32_t MCR;        /**< Module Configuration Register */
    uint32_t CTRL1;      /**< Control 1 Register */
    uint32_t TIMER;      /**< Free Running Timer */
    uint32_t Reserved0;  /**< Reserved */
    uint32_t RXMGMASK;   /**< Rx Mailbox Global Mask */
    uint32_t RX14MASK;   /**< Rx Buffer 14 Mask */
    uint32_t RX15MASK;   /**< Rx Buffer 15 Mask */
    uint32_t ECR;        /**< Error Counter Register */
    uint32_t ESR1;       /**< Error and Status 1 Register */
    uint32_t IMASK2;     /**< Interrupt Masks 2 Register */
    uint32_t IMASK1;     /**< Interrupt Masks 1 Register */
    uint32_t IFLAG2;     /**< Interrupt Flags 2 Register */
    uint32_t IFLAG1;     /**< Interrupt Flags 1 Register */
    uint32_t Reserved1[49]; /**< Reserved */
    uint32_t RXIMR[64];  /**< Rx Individual Mask Registers */
    uint32_t Reserved2[24]; /**< Reserved */
    uint32_t GFWR;       /**< Glitch Filter Width Register */
} Can_FlexCAN_RegType;

/**
 * @brief   FlexCAN Message Buffer Structure
 * @details Layout of a single message buffer (16 bytes = 4 uint32_t)
 */
typedef volatile struct
{
    uint32_t CS;         /**< Control/Status word */
    uint32_t ID;         /**< Identifier word */
    uint32_t DATA[2];    /**< Data payload (8 bytes) */
} Can_MessageBufferType;

/**
 * @brief   CAN Driver State
 * @details Internal state tracking for CAN driver
 */
typedef struct
{
    /** @brief Driver initialization status */
    boolean bInitialized;

    /** @brief Current mode for each controller */
    Can_ControllerStateType aeControllerState[CAN_CONTROLLER_COUNT_U8];

    /** @brief Error counters for each controller */
    Can_ErrorCountersType   astrErrorCounters[CAN_CONTROLLER_COUNT_U8];

    /** @brief Interrupt mask for each controller */
    uint32_t                au32InterruptMask[CAN_CONTROLLER_COUNT_U8];

    /** @brief Message buffer lock status */
    uint8_t                 au8MbLocked[CAN_CONTROLLER_COUNT_U8];

    /** @brief Free running timer snapshot at last read */
    uint16_t                au16TimerSnapshot[CAN_CONTROLLER_COUNT_U8];

} Can_DriverStateType;

/**
 * @brief   Message Buffer Status
 * @details Runtime status tracking for each message buffer
 */
typedef struct
{
    /** @brief Message buffer busy status */
    boolean bBusy;

    /** @brief Message buffer configured status */
    boolean bConfigured;

    /** @brief PDU being transmitted (for callbacks) */
    const Can_PduType* pPduInfo;

} Can_MbStatusType;

/*
 *****************************************************************************************
 * STATIC VARIABLES
 *****************************************************************************************
 */

/** @brief CAN driver global state */
static Can_DriverStateType Can_strDriverState =
{
    .bInitialized          = FALSE,
    .aeControllerState     = {CAN_CS_UNINIT, CAN_CS_UNINIT},
    .astrErrorCounters     = {{0U, 0U}, {0U, 0U}},
    .au32InterruptMask     = {0U, 0U},
    .au8MbLocked           = {0U, 0U},
    .au16TimerSnapshot     = {0U, 0U}
};

/** @brief Message buffer runtime status */
static Can_MbStatusType Can_strMbStatus[CAN_CONTROLLER_COUNT_U8][CAN_MB_COUNT_U8];

/** @brief Tx Confirmation callback */
static Can_TxConfirmationCallbackType Can_pfnTxConfirmation = NULL_PTR;

/** @brief Rx Indication callback */
static Can_RxIndicationCallbackType Can_pfnRxIndication = NULL_PTR;

/** @brief Busoff Notification callback */
static Can_BusoffNotificationCallbackType Can_pfnBusoffNotification = NULL_PTR;

/** @brief Error Notification callback */
static Can_ErrorNotificationCallbackType Can_pfnErrorNotification = NULL_PTR;

/*
 *****************************************************************************************
 * STATIC FUNCTION DECLARATIONS
 *****************************************************************************************
 */

/**
 * @brief   Enter Freeze Mode
 * @details Places the FlexCAN controller in freeze mode for configuration.
 * @param   pBase  Pointer to FlexCAN register base
 * @return  CAN_OK     - Freeze mode entered
 * @return  CAN_NOT_OK - Failed to enter freeze mode
 */
static Can_ReturnType Can_EnterFreezeMode(Can_FlexCAN_RegType* pBase);

/**
 * @brief   Exit Freeze Mode
 * @details Takes the FlexCAN controller out of freeze mode.
 * @param   pBase  Pointer to FlexCAN register base
 * @return  CAN_OK     - Freeze mode exited
 * @return  CAN_NOT_OK - Failed to exit freeze mode
 */
static Can_ReturnType Can_ExitFreezeMode(Can_FlexCAN_RegType* pBase);

/**
 * @brief   Configure Baudrate
 * @details Programs the timing parameters for a given baudrate.
 * @param   pBase        Pointer to FlexCAN register base
 * @param   pBaudConfig  Pointer to baudrate configuration
 * @return  CAN_OK     - Configuration successful
 * @return  CAN_NOT_OK - Configuration failed
 */
static Can_ReturnType Can_ConfigureBaudrate(Can_FlexCAN_RegType* pBase,
                                            const Can_BaudrateConfigType* pBaudConfig);

/**
 * @brief   Configure Message Buffer
 * @details Sets up a message buffer for transmit or receive.
 * @param   pBase     Pointer to FlexCAN register base
 * @param   u8MbIdx   Message buffer index
 * @param   pMbConfig Pointer to message buffer configuration
 * @return  CAN_OK     - Configuration successful
 * @return  CAN_NOT_OK - Configuration failed
 */
static Can_ReturnType Can_ConfigureMessageBuffer(Can_FlexCAN_RegType* pBase,
                                                  uint8_t u8MbIdx,
                                                  const Can_MessageBufferConfigType* pMbConfig);

/**
 * @brief   Configure All Message Buffers
 * @details Configures all message buffers for a controller.
 * @param   u8Controller  Controller ID
 * @return  CAN_OK     - All buffers configured
 * @return  CAN_NOT_OK - Configuration failed
 */
static Can_ReturnType Can_ConfigureAllMessageBuffers(uint8_t u8Controller);

/**
 * @brief   Lock Message Buffer
 * @details Locks a message buffer to prevent race conditions.
 * @param   pBase     Pointer to FlexCAN register base
 * @param   u8MbIdx   Message buffer index
 * @return  CAN_OK     - Buffer locked
 * @return  CAN_NOT_OK - Failed to lock
 */
static Can_ReturnType Can_LockMessageBuffer(Can_FlexCAN_RegType* pBase, uint8_t u8MbIdx);

/**
 * @brief   Unlock Message Buffer
 * @details Unlocks a previously locked message buffer.
 * @param   pBase  Pointer to FlexCAN register base
 */
static void Can_UnlockMessageBuffer(Can_FlexCAN_RegType* pBase);

/**
 * @brief   Write Message Buffer Data
 * @details Writes data payload to a transmit message buffer.
 * @param   pMb        Pointer to message buffer
 * @param   pPduInfo   Pointer to PDU information
 */
static void Can_WriteMbData(Can_MessageBufferType* pMb, const Can_PduType* pPduInfo);

/**
 * @brief   Read Message Buffer Data
 * @details Reads data payload from a receive message buffer.
 * @param   pMb        Pointer to message buffer
 * @param   pPduInfo   Pointer to PDU information
 */
static void Can_ReadMbData(Can_MessageBufferType* pMb, Can_PduType* pPduInfo);

/**
 * @brief   Process TX Interrupt
 * @details Handles transmit complete interrupts.
 * @param   u8Controller  Controller ID
 * @param   u8MbIdx       Message buffer index
 */
static void Can_ProcessTxInterrupt(uint8_t u8Controller, uint8_t u8MbIdx);

/**
 * @brief   Process RX Interrupt
 * @details Handles receive complete interrupts.
 * @param   u8Controller  Controller ID
 * @param   u8MbIdx       Message buffer index
 */
static void Can_ProcessRxInterrupt(uint8_t u8Controller, uint8_t u8MbIdx);

/**
 * @brief   Process Error Interrupt
 * @details Handles error and bus-off interrupts.
 * @param   u8Controller  Controller ID
 * @param   u32Esr        Error and status register value
 */
static void Can_ProcessErrorInterrupt(uint8_t u8Controller, uint32_t u32Esr);

/**
 * @brief   Get Controller Base Address
 * @details Returns the FlexCAN register base for a controller.
 * @param   u8Controller  Controller ID
 * @return  Pointer to FlexCAN register base, or NULL if invalid
 */
static Can_FlexCAN_RegType* Can_GetControllerBase(uint8_t u8Controller);

/**
 * @brief   Update Error Counters
 * @details Reads and updates error counters from hardware.
 * @param   u8Controller  Controller ID
 */
static void Can_UpdateErrorCounters(uint8_t u8Controller);

/*
 *****************************************************************************************
 * STATIC FUNCTION DEFINITIONS
 *****************************************************************************************
 */

static Can_FlexCAN_RegType* Can_GetControllerBase(uint8_t u8Controller)
{
    Can_FlexCAN_RegType* pBase = NULL_PTR;

    /* MISRA deviation: switch-case for controller selection */
    /* Controller selection - clear and intentional */
    switch (u8Controller)
    {
        case 0U:
        {
            pBase = (Can_FlexCAN_RegType*)CAN0_BASE;
        }
        break;

        case 1U:
        {
            pBase = (Can_FlexCAN_RegType*)CAN1_BASE;
        }
        break;

        default:
        {
            /* Invalid controller - return NULL */
        }
        break;
    }

    return pBase;
}

static Can_ReturnType Can_EnterFreezeMode(Can_FlexCAN_RegType* pBase)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    uint32_t u32McrValue;
    uint16_t u16Timeout;

    /* MISRA deviation: timeout loop with fixed iteration count */
    /* Safety: Timeout prevents infinite loop in case of hardware failure */
    u16Timeout = 1000U;

    /* Read current MCR value */
    u32McrValue = pBase->MCR;

    /* Set FRZ (Freeze) bit and clear HALT bit */
    u32McrValue |= (uint32_t)CAN_MCR_FRZ_MASK;
    u32McrValue &= ~(uint32_t)CAN_MCR_HALT_MASK;
    pBase->MCR = u32McrValue;

    /* Wait for Freeze Acknowledge (FRZACK) */
    while (u16Timeout > 0U)
    {
        if ((pBase->MCR & (uint32_t)CAN_MCR_FRZACK_MASK) != 0U)
        {
            eRetVal = CAN_OK;
            break;
        }
        u16Timeout--;
    }

    return eRetVal;
}

static Can_ReturnType Can_ExitFreezeMode(Can_FlexCAN_RegType* pBase)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    uint32_t u32McrValue;
    uint16_t u16Timeout;

    /* MISRA deviation: timeout loop with fixed iteration count */
    /* Safety: Timeout prevents infinite loop in case of hardware failure */
    u16Timeout = 1000U;

    /* Read current MCR value */
    u32McrValue = pBase->MCR;

    /* Clear FRZ bit to exit freeze mode */
    u32McrValue &= ~(uint32_t)CAN_MCR_FRZ_MASK;
    pBase->MCR = u32McrValue;

    /* Wait for Not Frozen (NOTRDY) to clear */
    while (u16Timeout > 0U)
    {
        if ((pBase->MCR & (uint32_t)CAN_MCR_NOTRDY_MASK) == 0U)
        {
            eRetVal = CAN_OK;
            break;
        }
        u16Timeout--;
    }

    return eRetVal;
}

static Can_ReturnType Can_ConfigureBaudrate(Can_FlexCAN_RegType* pBase,
                                            const Can_BaudrateConfigType* pBaudConfig)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    uint32_t u32CtrlValue;
    uint16_t u16Timeout;

    /* MISRA deviation: timeout loop with fixed iteration count */
    /* Safety: Timeout prevents infinite loop in case of hardware failure */
    u16Timeout = 1000U;

    /* Enter freeze mode for configuration */
    eRetVal = Can_EnterFreezeMode(pBase);
    if (eRetVal != CAN_OK)
    {
        /* Failed to enter freeze mode */
    }
    else
    {
        /* Configure CTRL1 register with timing parameters */
        u32CtrlValue = pBase->CTRL1;

        /* Clear timing-related fields */
        u32CtrlValue &= ~(uint32_t)(CAN_CTRL1_PRESDIV_MASK |
                                    CAN_CTRL1_RJW_MASK |
                                    CAN_CTRL1_PS1_MASK |
                                    CAN_CTRL1_PS2_MASK |
                                    CAN_CTRL1_SMP_MASK);

        /* Set prescaler */
        u32CtrlValue |= ((pBaudConfig->u32Prescaler - 1U) << CAN_CTRL1_PRESDIV_SHIFT);

        /* Set resynchronization jump width */
        u32CtrlValue |= ((pBaudConfig->u8RJW - 1U) << CAN_CTRL1_RJW_SHIFT);

        /* Set phase segment 1 */
        u32CtrlValue |= ((pBaudConfig->u8PS1 - 1U) << CAN_CTRL1_PS1_SHIFT);

        /* Set phase segment 2 */
        u32CtrlValue |= ((pBaudConfig->u8PS2 - 1U) << CAN_CTRL1_PS2_SHIFT);

        /* Set triple sampling if enabled */
        if (pBaudConfig->bTripleSampling == TRUE)
        {
            u32CtrlValue |= (uint32_t)CAN_CTRL1_SMP_MASK;
        }
        else
        {
            /* Single sampling - leave bit clear */
        }

        /* Write configuration to CTRL1 */
        pBase->CTRL1 = u32CtrlValue;

        /* Wait for configuration to complete */
        u16Timeout = 1000U;
        while (u16Timeout > 0U)
        {
            if ((pBase->CTRL1 & CAN_CTRL1_LPB_MASK) == 0U)
            {
                /* Loopback disabled - normal operation */
                break;
            }
            u16Timeout--;
        }

        /* Exit freeze mode */
        (void)Can_ExitFreezeMode(pBase);
        eRetVal = CAN_OK;
    }

    return eRetVal;
}

static Can_ReturnType Can_ConfigureMessageBuffer(Can_FlexCAN_RegType* pBase,
                                                  uint8_t u8MbIdx,
                                                  const Can_MessageBufferConfigType* pMbConfig)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_MessageBufferType* pMb;
    uint32_t u32IdValue;
    uint32_t u32CsValue;

    /* Validate parameters */
    if ((pBase == NULL_PTR) || (pMbConfig == NULL_PTR))
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else if (u8MbIdx >= CAN_MB_COUNT_U8)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else
    {
        /* Enter freeze mode for message buffer configuration */
        eRetVal = Can_EnterFreezeMode(pBase);
        if (eRetVal == CAN_OK)
        {
            /* Get message buffer pointer */
            pMb = (Can_MessageBufferType*)((uint32_t)pBase + CAN_MB_OFFSET_U32(u8MbIdx));

            /* Configure identifier */
            if (pMbConfig->u8IdType == CAN_ID_TYPE_STANDARD)
            {
                /* Standard ID: 11-bit, shift to bits 28-18 */
                u32IdValue = ((pMbConfig->u32CanId & CAN_ID_STANDARD_MASK) << CAN_MB_ID_STD_SHIFT);
                /* Clear extended ID flag */
                u32IdValue &= ~(uint32_t)CAN_MB_ID_EXT_MASK;
            }
            else
            {
                /* Extended ID: 29-bit */
                u32IdValue = ((pMbConfig->u32CanId & CAN_ID_EXTENDED_MASK) << CAN_MB_ID_EXT_SHIFT);
                /* Set extended ID flag */
                u32IdValue |= (uint32_t)CAN_MB_ID_EXT_MASK;
            }
            pMb->ID = u32IdValue;

            /* Configure control word: DLC */
            u32CsValue = ((uint32_t)pMbConfig->u8DLC << CAN_MB_DLC_SHIFT);

            /* Enable transmit interrupt if configured */
            if ((pMbConfig->u8MessageBufferType == CAN_MB_TX) && (pMbConfig->bTxInterrupt == TRUE))
            {
                /* Interrupt will be enabled globally in Can_EnableInterrupt */
            }

            /* Enable receive interrupt if configured */
            if ((pMbConfig->u8MessageBufferType == CAN_MB_RX) && (pMbConfig->bRxInterrupt == TRUE))
            {
                /* Interrupt will be enabled globally in Can_EnableInterrupt */
            }

            /* Initialize data payload to zero */
            pMb->DATA[0] = 0U;
            pMb->DATA[1] = 0U;

            /* Write control word */
            pMb->CS = u32CsValue;

            /* Mark message buffer as configured */
            Can_strMbStatus[0][u8MbIdx].bConfigured = TRUE;

            /* Exit freeze mode */
            (void)Can_ExitFreezeMode(pBase);
            eRetVal = CAN_OK;
        }
    }

    return eRetVal;
}

static Can_ReturnType Can_ConfigureAllMessageBuffers(uint8_t u8Controller)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_FlexCAN_RegType* pBase;
    uint8_t u8MbCount;
    uint8_t u8MbIdx;
    const Can_MessageBufferConfigType* pMbConfig;

    /* Validate controller */
    if (u8Controller >= CAN_CONTROLLER_COUNT_U8)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else
    {
        pBase = Can_GetControllerBase(u8Controller);
        if (pBase == NULL_PTR)
        {
            eRetVal = CAN_PARAM_ERROR;
        }
        else
        {
            /* Get message buffer count for this controller */
            u8MbCount = Can_au8MbConfigCount[u8Controller];

            /* Configure each message buffer */
            pMbConfig = Can_apstrMbConfig[u8Controller];

            for (u8MbIdx = 0U; u8MbIdx < u8MbCount; u8MbIdx++)
            {
                eRetVal = Can_ConfigureMessageBuffer(pBase, u8MbIdx, pMbConfig);
                if (eRetVal != CAN_OK)
                {
                    /* Configuration failed for this buffer */
                    break;
                }
                pMbConfig++;
            }
        }
    }

    return eRetVal;
}

static Can_ReturnType Can_LockMessageBuffer(Can_FlexCAN_RegType* pBase, uint8_t u8MbIdx)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;

    /* Read from message buffer to lock it */
    /* This is a FlexCAN requirement: must read CS word to lock */
    volatile uint32_t u32CsValue;

    /* MISRA deviation: volatile access for hardware locking */
    /* Intentional: Required by FlexCAN hardware protocol */
    u32CsValue = ((volatile uint32_t*)pBase)[u8MbIdx];

    if ((u32CsValue & CAN_MB_CODE_MASK) != CAN_MB_CODE_INACTIVE)
    {
        /* Buffer is busy/locked */
        Can_strMbStatus[0][u8MbIdx].bBusy = TRUE;
        eRetVal = CAN_OK;
    }
    else
    {
        eRetVal = CAN_BUSY;
    }

    return eRetVal;
}

static void Can_UnlockMessageBuffer(Can_FlexCAN_RegType* pBase)
{
    /* MISRA deviation: volatile read for hardware unlock */
    /* Intentional: Required by FlexCAN hardware protocol */
    /* Read from timer register to unlock buffers */
    (void)pBase->TIMER;
}

static void Can_WriteMbData(Can_MessageBufferType* pMb, const Can_PduType* pPduInfo)
{
    uint8_t u8Dlc;
    uint8_t u8ByteIdx;

    /* Validate inputs */
    if ((pMb == NULL_PTR) || (pPduInfo == NULL_PTR))
    {
        /* Invalid parameters - return early */
    }
    else
    {
        /* Get DLC (capped at 8) */
        u8Dlc = pPduInfo->u8Dlc;
        if (u8Dlc > 8U)
        {
            u8Dlc = 8U;
        }

        /* Copy data payload byte by byte */
        for (u8ByteIdx = 0U; u8ByteIdx < u8Dlc; u8ByteIdx++)
        {
            /* MISRA deviation: Byte-wise copy for data transfer */
            /* Justified: Required for flexible data handling */
            /* Cast to volatile uint8_t* for byte access */
            ((volatile uint8_t*)pMb->DATA)[u8ByteIdx] = pPduInfo->au8Sdu[u8ByteIdx];
        }

        /* Clear remaining bytes if DLC < 8 */
        for (; u8ByteIdx < 8U; u8ByteIdx++)
        {
            ((volatile uint8_t*)pMb->DATA)[u8ByteIdx] = 0U;
        }
    }
}

static void Can_ReadMbData(Can_MessageBufferType* pMb, Can_PduType* pPduInfo)
{
    uint8_t u8Dlc;
    uint8_t u8ByteIdx;

    /* Validate inputs */
    if ((pMb == NULL_PTR) || (pPduInfo == NULL_PTR))
    {
        /* Invalid parameters - return early */
    }
    else
    {
        /* Get DLC from control word */
        u8Dlc = (uint8_t)((pMb->CS & CAN_MB_DLC_MASK) >> CAN_MB_DLC_SHIFT);

        /* Store DLC in PDU */
        pPduInfo->u8Dlc = u8Dlc;

        /* Copy data payload byte by byte */
        for (u8ByteIdx = 0U; u8ByteIdx < u8Dlc; u8ByteIdx++)
        {
            /* MISRA deviation: Byte-wise copy for data transfer */
            /* Justified: Required for flexible data handling */
            /* Cast to volatile uint8_t* for byte access */
            pPduInfo->au8Sdu[u8ByteIdx] = ((volatile uint8_t*)pMb->DATA)[u8ByteIdx];
        }
    }
}

static void Can_ProcessTxInterrupt(uint8_t u8Controller, uint8_t u8MbIdx)
{
    Can_FlexCAN_RegType* pBase;
    Can_MessageBufferType* pMb;
    uint32_t u32CsValue;

    pBase = Can_GetControllerBase(u8Controller);
    if (pBase == NULL_PTR)
    {
        /* Invalid controller - return early */
    }
    else
    {
        /* Get message buffer */
        pMb = (Can_MessageBufferType*)((uint32_t)pBase + CAN_MB_OFFSET_U32(u8MbIdx));

        /* Read control word to get transmission status */
        u32CsValue = pMb->CS;

        /* Check for successful transmission (CODE = TX_INACTIVE or TX_ABORT) */
        if (((u32CsValue & CAN_MB_CODE_MASK) == CAN_MB_CODE_TX_INACTIVE) ||
            ((u32CsValue & CAN_MB_CODE_MASK) == CAN_MB_CODE_INACTIVE))
        {
            /* Clear busy status */
            Can_strMbStatus[u8Controller][u8MbIdx].bBusy = FALSE;

            /* Call tx confirmation callback if registered */
            if (Can_pfnTxConfirmation != NULL_PTR)
            {
                Can_pfnTxConfirmation();
            }
        }

        /* Clear interrupt flag */
        if (u8MbIdx < 32U)
        {
            pBase->IFLAG1 = (1U << u8MbIdx);
        }
        else
        {
            pBase->IFLAG2 = (1U << (u8MbIdx - 32U));
        }
    }
}

static void Can_ProcessRxInterrupt(uint8_t u8Controller, uint8_t u8MbIdx)
{
    Can_FlexCAN_RegType* pBase;
    Can_MessageBufferType* pMb;
    Can_PduType strPduInfo;
    uint32_t u32IdValue;
    uint32_t u32CsValue;

    pBase = Can_GetControllerBase(u8Controller);
    if (pBase == NULL_PTR)
    {
        /* Invalid controller - return early */
    }
    else
    {
        /* Get message buffer */
        pMb = (Can_MessageBufferType*)((uint32_t)pBase + CAN_MB_OFFSET_U32(u8MbIdx));

        /* Read control word and identifier */
        u32CsValue = pMb->CS;
        u32IdValue = pMb->ID;

        /* Check if message buffer contains data (CODE = RX_FULL or RX_OVERRUN) */
        if (((u32CsValue & CAN_MB_CODE_MASK) == CAN_MB_CODE_RX_FULL) ||
            ((u32CsValue & CAN_MB_CODE_MASK) == CAN_MB_CODE_RX_OVERRUN))
        {
            /* Extract CAN ID */
            if ((u32IdValue & CAN_MB_ID_EXT_MASK) != 0U)
            {
                /* Extended ID */
                strPduInfo.u8IdType = CAN_ID_TYPE_EXTENDED;
                strPduInfo.u32Id = (u32IdValue >> CAN_MB_ID_EXT_SHIFT) & CAN_ID_EXTENDED_MASK;
            }
            else
            {
                /* Standard ID */
                strPduInfo.u8IdType = CAN_ID_TYPE_STANDARD;
                strPduInfo.u32Id = (u32IdValue >> CAN_MB_ID_STD_SHIFT) & CAN_ID_STANDARD_MASK;
            }

            /* Read data payload */
            Can_ReadMbData(pMb, &strPduInfo);

            /* Call rx indication callback if registered */
            if (Can_pfnRxIndication != NULL_PTR)
            {
                Can_pfnRxIndication(&strPduInfo);
            }

            /* Mark buffer as free (write inactive code) */
            pMb->CS = CAN_MB_CODE_INACTIVE;
        }

        /* Clear interrupt flag */
        if (u8MbIdx < 32U)
        {
            pBase->IFLAG1 = (1U << u8MbIdx);
        }
        else
        {
            pBase->IFLAG2 = (1U << (u8MbIdx - 32U));
        }
    }
}

static void Can_ProcessErrorInterrupt(uint8_t u8Controller, uint32_t u32Esr)
{
    /* Check for bus-off state */
    if ((u32Esr & CAN_ESR_BOFF_INT_MASK) != 0U)
    {
        /* Bus-off detected */
        if (Can_pfnBusoffNotification != NULL_PTR)
        {
            Can_pfnBusoffNotification();
        }
    }

    /* Check for error interrupts */
    if ((u32Esr & (CAN_ESR_ERR_INT_MASK | CAN_ESR_BIT1_ERR_MASK |
                   CAN_ESR_BIT0_ERR_MASK | CAN_ESR_STUFF_ERR_MASK |
                   CAN_ESR_CRC_ERR_MASK | CAN_ESR_ACK_ERR_MASK |
                   CAN_ESR_FORM_ERR_MASK)) != 0U)
    {
        /* Determine error type */
        if ((u32Esr & CAN_ESR_BIT1_ERR_MASK) != 0U)
        {
            if (Can_pfnErrorNotification != NULL_PTR)
            {
                Can_pfnErrorNotification(CAN_ERROR_BIT);
            }
        }
        else if ((u32Esr & CAN_ESR_STUFF_ERR_MASK) != 0U)
        {
            if (Can_pfnErrorNotification != NULL_PTR)
            {
                Can_pfnErrorNotification(CAN_ERROR_STUFF);
            }
        }
        else if ((u32Esr & CAN_ESR_CRC_ERR_MASK) != 0U)
        {
            if (Can_pfnErrorNotification != NULL_PTR)
            {
                Can_pfnErrorNotification(CAN_ERROR_CRC);
            }
        }
        else if ((u32Esr & CAN_ESR_ACK_ERR_MASK) != 0U)
        {
            if (Can_pfnErrorNotification != NULL_PTR)
            {
                Can_pfnErrorNotification(CAN_ERROR_ACK);
            }
        }
        else if ((u32Esr & CAN_ESR_FORM_ERR_MASK) != 0U)
        {
            if (Can_pfnErrorNotification != NULL_PTR)
            {
                Can_pfnErrorNotification(CAN_ERROR_FORM);
            }
        }
        else
        {
            /* Other error type */
            if (Can_pfnErrorNotification != NULL_PTR)
            {
                Can_pfnErrorNotification(CAN_ERROR_TX);
            }
        }
    }

    /* Update error counters */
    Can_UpdateErrorCounters(u8Controller);
}

static void Can_UpdateErrorCounters(uint8_t u8Controller)
{
    Can_FlexCAN_RegType* pBase;

    pBase = Can_GetControllerBase(u8Controller);
    if (pBase != NULL_PTR)
    {
        /* Read error counters from hardware */
        Can_strDriverState.astrErrorCounters[u8Controller].u8TxErrorCount =
            (uint8_t)((pBase->ECR & CAN_ECR_TXECTR_MASK) >> CAN_ECR_TXECTR_SHIFT);
        Can_strDriverState.astrErrorCounters[u8Controller].u8RxErrorCount =
            (uint8_t)((pBase->ECR & CAN_ECR_RXECTR_MASK) >> CAN_ECR_RXECTR_SHIFT);
    }
}

/*
 *****************************************************************************************
 * API FUNCTION DEFINITIONS
 *****************************************************************************************
 */

Can_ReturnType Can_Init(void)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_FlexCAN_RegType* pBase;
    uint8_t u8Controller;
    uint8_t u8MbIdx;
    const Can_ControllerConfigType* pCtrlConfig;
    const Can_BaudrateConfigType* pBaudConfig;

    /* Check if already initialized */
    if (Can_strDriverState.bInitialized == TRUE)
    {
        eRetVal = CAN_NOT_OK;
    }
    else
    {
        /* Initialize all controllers */
        for (u8Controller = 0U; u8Controller < CAN_CONTROLLER_COUNT_U8; u8Controller++)
        {
            pCtrlConfig = &Can_astrControllerConfig[u8Controller];
            pBase = (Can_FlexCAN_RegType*)pCtrlConfig->pBaseAddress;

            if (pBase == NULL_PTR)
            {
                continue;
            }

            /* Enter freeze mode */
            eRetVal = Can_EnterFreezeMode(pBase);
            if (eRetVal != CAN_OK)
            {
                continue;
            }

            /* Configure baudrate */
            pBaudConfig = &Can_au8BaudrateConfig[pCtrlConfig->u8BaudrateConfigId];
            eRetVal = Can_ConfigureBaudrate(pBase, pBaudConfig);
            if (eRetVal != CAN_OK)
            {
                (void)Can_ExitFreezeMode(pBase);
                continue;
            }

            /* Configure all message buffers */
            eRetVal = Can_ConfigureAllMessageBuffers(u8Controller);
            if (eRetVal != CAN_OK)
            {
                continue;
            }

            /* Initialize message buffer status */
            for (u8MbIdx = 0U; u8MbIdx < CAN_MB_COUNT_U8; u8MbIdx++)
            {
                Can_strMbStatus[u8Controller][u8MbIdx].bBusy = FALSE;
                Can_strMbStatus[u8Controller][u8MbIdx].bConfigured = FALSE;
                Can_strMbStatus[u8Controller][u8MbIdx].pPduInfo = NULL_PTR;
            }

            /* Set initial controller state */
            Can_strDriverState.aeControllerState[u8Controller] = CAN_CS_STOPPED;

            /* Enable error and bus-off interrupts */
            pBase->CTRL1 |= (CAN_CTRL1_ERRMSK_MASK | CAN_CTRL1_BOFFMSK_MASK);

            /* Enable message buffer interrupts for configured MBs */
            /* Interrupts will be enabled per-MB when configured */
        }

        /* Mark driver as initialized */
        Can_strDriverState.bInitialized = TRUE;
        eRetVal = CAN_OK;
    }

    return eRetVal;
}

Can_ReturnType Can_DeInit(void)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_FlexCAN_RegType* pBase;
    uint8_t u8Controller;

    /* Check if initialized */
    if (Can_strDriverState.bInitialized == FALSE)
    {
        eRetVal = CAN_UNINIT;
    }
    else
    {
        /* Stop all controllers */
        for (u8Controller = 0U; u8Controller < CAN_CONTROLLER_COUNT_U8; u8Controller++)
        {
            pBase = Can_GetControllerBase(u8Controller);
            if (pBase != NULL_PTR)
            {
                /* Enter freeze mode */
                (void)Can_EnterFreezeMode(pBase);

                /* Disable all interrupts */
                pBase->IMASK1 = 0U;
                pBase->IMASK2 = 0U;

                /* Clear all interrupt flags */
                pBase->IFLAG1 = 0xFFFFFFFFU;
                pBase->IFLAG2 = 0xFFFFFFFFU;

                /* Set controller to stopped state */
                Can_strDriverState.aeControllerState[u8Controller] = CAN_CS_UNINIT;
            }
        }

        /* Reset driver state */
        Can_strDriverState.bInitialized = FALSE;

        /* Clear callbacks */
        Can_pfnTxConfirmation = NULL_PTR;
        Can_pfnRxIndication = NULL_PTR;
        Can_pfnBusoffNotification = NULL_PTR;
        Can_pfnErrorNotification = NULL_PTR;

        eRetVal = CAN_OK;
    }

    return eRetVal;
}

Can_ReturnType Can_SetControllerMode(uint8_t u8Controller, Can_ControllerStateType eMode)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_FlexCAN_RegType* pBase;
    uint32_t u32McrValue;
    uint16_t u16Timeout;

    /* Validate controller */
    if (u8Controller >= CAN_CONTROLLER_COUNT_U8)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else if (Can_strDriverState.bInitialized == FALSE)
    {
        eRetVal = CAN_UNINIT;
    }
    else
    {
        pBase = Can_GetControllerBase(u8Controller);
        if (pBase == NULL_PTR)
        {
            eRetVal = CAN_PARAM_ERROR;
        }
        else
        {
            /* MISRA deviation: switch-case for mode selection */
            /* Mode selection - clear and intentional */
            switch (eMode)
            {
                case CAN_CS_STARTED:
                {
                    /* Clear HALT bit to start controller */
                    u32McrValue = pBase->MCR;
                    u32McrValue &= ~(uint32_t)CAN_MCR_HALT_MASK;
                    pBase->MCR = u32McrValue;

                    /* Wait for NOTRDY to clear */
                    u16Timeout = 1000U;
                    while (u16Timeout > 0U)
                    {
                        if ((pBase->MCR & CAN_MCR_NOTRDY_MASK) == 0U)
                        {
                            Can_strDriverState.aeControllerState[u8Controller] = CAN_CS_STARTED;
                            eRetVal = CAN_OK;
                            break;
                        }
                        u16Timeout--;
                    }
                }
                break;

                case CAN_CS_STOPPED:
                {
                    /* Set HALT bit to stop controller */
                    u32McrValue = pBase->MCR;
                    u32McrValue |= (uint32_t)CAN_MCR_HALT_MASK;
                    pBase->MCR = u32McrValue;

                    /* Wait for NOTRDY to set */
                    u16Timeout = 1000U;
                    while (u16Timeout > 0U)
                    {
                        if ((pBase->MCR & CAN_MCR_NOTRDY_MASK) != 0U)
                        {
                            Can_strDriverState.aeControllerState[u8Controller] = CAN_CS_STOPPED;
                            eRetVal = CAN_OK;
                            break;
                        }
                        u16Timeout--;
                    }
                }
                break;

                case CAN_CS_SLEEP:
                {
                    /* Enable stop mode and enter sleep */
                    u32McrValue = pBase->MCR;
                    u32McrValue |= (uint32_t)CAN_MCR_SLFWAK_MASK;
                    pBase->MCR = u32McrValue;

                    Can_strDriverState.aeControllerState[u8Controller] = CAN_CS_SLEEP;
                    eRetVal = CAN_OK;
                }
                break;

                case CAN_CS_UNINIT:
                default:
                {
                    eRetVal = CAN_PARAM_ERROR;
                }
                break;
            }
        }
    }

    return eRetVal;
}

Can_ReturnType Can_Write(uint8_t u8Hoh, const Can_PduType* pPduInfo)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_FlexCAN_RegType* pBase;
    Can_MessageBufferType* pMb;
    uint8_t u8Controller;
    uint8_t u8MbIdx;
    uint32_t u32CsValue;
    uint8_t u8IdType;

    /* Validate inputs */
    if (pPduInfo == NULL_PTR)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else if (Can_strDriverState.bInitialized == FALSE)
    {
        eRetVal = CAN_UNINIT;
    }
    else if (pPduInfo->u8Dlc > 8U)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else
    {
        /* Find controller and message buffer from HOH */
        u8Controller = u8Hoh;
        u8MbIdx = u8Hoh;

        if (u8Controller >= CAN_CONTROLLER_COUNT_U8)
        {
            eRetVal = CAN_PARAM_ERROR;
        }
        else
        {
            pBase = Can_GetControllerBase(u8Controller);
            if (pBase == NULL_PTR)
            {
                eRetVal = CAN_PARAM_ERROR;
            }
            else if (Can_strDriverState.aeControllerState[u8Controller] != CAN_CS_STARTED)
            {
                eRetVal = CAN_NOT_OK;
            }
            else
            {
                /* Get message buffer */
                pMb = (Can_MessageBufferType*)((uint32_t)pBase + CAN_MB_OFFSET_U32(u8MbIdx));

                /* Lock message buffer for transmission */
                /* Read CS word to lock */
                u32CsValue = pMb->CS;

                /* Check if buffer is ready for transmission */
                if ((u32CsValue & CAN_MB_CODE_MASK) == CAN_MB_CODE_INACTIVE)
                {
                    /* Configure ID based on ID type */
                    if (pPduInfo->u8IdType == CAN_ID_TYPE_STANDARD)
                    {
                        /* Standard ID */
                        pMb->ID = ((pPduInfo->u32Id & CAN_ID_STANDARD_MASK) << CAN_MB_ID_STD_SHIFT);
                    }
                    else
                    {
                        /* Extended ID */
                        pMb->ID = (((pPduInfo->u32Id & CAN_ID_EXTENDED_MASK) << CAN_MB_ID_EXT_SHIFT) |
                                   CAN_MB_ID_EXT_MASK);
                    }

                    /* Write data payload */
                    Can_WriteMbData(pMb, pPduInfo);

                    /* Configure control word: DLC and TX code */
                    u32CsValue = ((uint32_t)pPduInfo->u8DLC << CAN_MB_DLC_SHIFT);
                    u32CsValue |= CAN_MB_CODE_TX_DATA; /* One-shot transmission */

                    /* Write control word to start transmission */
                    pMb->CS = u32CsValue;

                    /* Mark buffer as busy */
                    Can_strMbStatus[u8Controller][u8MbIdx].bBusy = TRUE;

                    eRetVal = CAN_OK;
                }
                else
                {
                    /* Buffer busy or not ready */
                    eRetVal = CAN_BUSY;
                }
            }
        }
    }

    return eRetVal;
}

Can_ReturnType Can_GetControllerStatus(uint8_t u8Controller,
                                        Can_ControllerStateType* pControllerState)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;

    /* Validate inputs */
    if (u8Controller >= CAN_CONTROLLER_COUNT_U8)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else if (pControllerState == NULL_PTR)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else
    {
        /* Return current controller state */
        *pControllerState = Can_strDriverState.aeControllerState[u8Controller];
        eRetVal = CAN_OK;
    }

    return eRetVal;
}

Can_ReturnType Can_GetErrorCounters(uint8_t u8Controller,
                                     Can_ErrorCountersType* pErrorCounters)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;

    /* Validate inputs */
    if (u8Controller >= CAN_CONTROLLER_COUNT_U8)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else if (pErrorCounters == NULL_PTR)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else
    {
        /* Update error counters from hardware */
        Can_UpdateErrorCounters(u8Controller);

        /* Return error counters */
        *pErrorCounters = Can_strDriverState.astrErrorCounters[u8Controller];
        eRetVal = CAN_OK;
    }

    return eRetVal;
}

boolean Can_CheckWakeup(uint8_t u8Controller)
{
    boolean bRetVal = FALSE;
    Can_FlexCAN_RegType* pBase;

    /* Validate controller */
    if (u8Controller < CAN_CONTROLLER_COUNT_U8)
    {
        pBase = Can_GetControllerBase(u8Controller);
        if (pBase != NULL_PTR)
        {
            /* Check wakeup flag in ESR1 */
            if ((pBase->ESR1 & CAN_ESR_WAK_INT_MASK) != 0U)
            {
                /* Clear wakeup interrupt flag */
                pBase->ESR1 = CAN_ESR_WAK_INT_MASK;
                bRetVal = TRUE;
            }
        }
    }

    return bRetVal;
}

Can_ReturnType Can_EnableInterrupt(uint8_t u8Controller, uint16_t u16InterruptMask)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_FlexCAN_RegType* pBase;

    /* Validate controller */
    if (u8Controller >= CAN_CONTROLLER_COUNT_U8)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else
    {
        pBase = Can_GetControllerBase(u8Controller);
        if (pBase == NULL_PTR)
        {
            eRetVal = CAN_PARAM_ERROR;
        }
        else
        {
            /* Store interrupt mask */
            Can_strDriverState.au32InterruptMask[u8Controller] |= u16InterruptMask;

            /* Enable individual message buffer interrupts */
            /* Message buffer interrupts are enabled via IMASK1/IMASK2 */
            /* MB 0-31: IMASK1, MB 32-63: IMASK2 */

            /* For simplicity, enable all MB interrupts if any MB interrupt requested */
            if ((u16InterruptMask & (CAN_IT_TX | CAN_IT_RX)) != 0U)
            {
                pBase->IMASK1 = 0xFFFFFFFFU;
                pBase->IMASK2 = 0xFFFFFFFFU;
            }

            /* Enable error and bus-off interrupts */
            if ((u16InterruptMask & (CAN_IT_ERROR | CAN_IT_BUSOFF)) != 0U)
            {
                pBase->CTRL1 |= (CAN_CTRL1_ERRMSK_MASK | CAN_CTRL1_BOFFMSK_MASK);
            }

            eRetVal = CAN_OK;
        }
    }

    return eRetVal;
}

Can_ReturnType Can_DisableInterrupt(uint8_t u8Controller, uint16_t u16InterruptMask)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_FlexCAN_RegType* pBase;

    /* Validate controller */
    if (u8Controller >= CAN_CONTROLLER_COUNT_U8)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else
    {
        pBase = Can_GetControllerBase(u8Controller);
        if (pBase == NULL_PTR)
        {
            eRetVal = CAN_PARAM_ERROR;
        }
        else
        {
            /* Store interrupt mask */
            Can_strDriverState.au32InterruptMask[u8Controller] &= ~(uint32_t)u16InterruptMask;

            /* Disable message buffer interrupts */
            if ((u16InterruptMask & (CAN_IT_TX | CAN_IT_RX)) != 0U)
            {
                pBase->IMASK1 = 0U;
                pBase->IMASK2 = 0U;
            }

            /* Disable error and bus-off interrupts */
            if ((u16InterruptMask & (CAN_IT_ERROR | CAN_IT_BUSOFF)) != 0U)
            {
                pBase->CTRL1 &= ~(CAN_CTRL1_ERRMSK_MASK | CAN_CTRL1_BOFFMSK_MASK);
            }

            eRetVal = CAN_OK;
        }
    }

    return eRetVal;
}

Can_ReturnType Can_GetInterruptStatus(uint8_t u8Controller, uint16_t* pu16Status)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_FlexCAN_RegType* pBase;

    /* Validate inputs */
    if (u8Controller >= CAN_CONTROLLER_COUNT_U8)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else if (pu16Status == NULL_PTR)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else
    {
        pBase = Can_GetControllerBase(u8Controller);
        if (pBase == NULL_PTR)
        {
            eRetVal = CAN_PARAM_ERROR;
        }
        else
        {
            /* Read ESR1 for error and status */
            *pu16Status = (uint16_t)(pBase->ESR1 & 0xFFFFU);
            eRetVal = CAN_OK;
        }
    }

    return eRetVal;
}

Can_ReturnType Can_ClearInterruptFlags(uint8_t u8Controller, uint16_t u16FlagMask)
{
    Can_ReturnType eRetVal = CAN_NOT_OK;
    Can_FlexCAN_RegType* pBase;

    /* Validate controller */
    if (u8Controller >= CAN_CONTROLLER_COUNT_U8)
    {
        eRetVal = CAN_PARAM_ERROR;
    }
    else
    {
        pBase = Can_GetControllerBase(u8Controller);
        if (pBase == NULL_PTR)
        {
            eRetVal = CAN_PARAM_ERROR;
        }
        else
        {
            /* Clear IFLAG1 bits 0-31 */
            if ((u16FlagMask & 0xFFFFU) != 0U)
            {
                pBase->IFLAG1 = (uint32_t)u16FlagMask;
            }
            /* Clear IFLAG2 bits 32-63 */
            if ((u16FlagMask & 0xFFFF0000U) != 0U)
            {
                pBase->IFLAG2 = (uint32_t)(u16FlagMask >> 16U);
            }
            /* Clear ESR1 error flags */
            if ((u16FlagMask & 0xFFFFU) != 0U)
            {
                pBase->ESR1 = (uint32_t)u16FlagMask;
            }

            eRetVal = CAN_OK;
        }
    }

    return eRetVal;
}

void Can_RegisterTxConfirmationCallback(Can_TxConfirmationCallbackType pCallback)
{
    Can_pfnTxConfirmation = pCallback;
}

void Can_RegisterRxIndicationCallback(Can_RxIndicationCallbackType pCallback)
{
    Can_pfnRxIndication = pCallback;
}

void Can_RegisterBusoffNotificationCallback(Can_BusoffNotificationCallbackType pCallback)
{
    Can_pfnBusoffNotification = pCallback;
}

void Can_RegisterErrorNotificationCallback(Can_ErrorNotificationCallbackType pCallback)
{
    Can_pfnErrorNotification = pCallback;
}

/*
 *****************************************************************************************
 * INTERRUPT SERVICE ROUTINES
 *****************************************************************************************
 */

void Can_IsrHandler_Controller0(void)
{
    Can_FlexCAN_RegType* pBase;
    uint32_t u32Iflag1;
    uint32_t u32Iflag2;
    uint32_t u32Esr;
    uint8_t u8MbIdx;

    pBase = (Can_FlexCAN_RegType*)CAN0_BASE;
    if (pBase == NULL_PTR)
    {
        return;
    }

    /* Read interrupt flags */
    u32Iflag1 = pBase->IFLAG1;
    u32Iflag2 = pBase->IFLAG2;
    u32Esr = pBase->ESR1;

    /* Clear ESR1 error flags (will be re-read if needed) */
    pBase->ESR1 = u32Esr;

    /* Process TX interrupts (MB 0-31) */
    if (u32Iflag1 != 0U)
    {
        /* Find first set bit and process */
        u8MbIdx = 0U;
        while (u32Iflag1 != 0U)
        {
            if ((u32Iflag1 & 0x01U) != 0U)
            {
                Can_ProcessTxInterrupt(0U, u8MbIdx);
            }
            u32Iflag1 >>= 1U;
            u8MbIdx++;
        }
    }

    /* Process TX interrupts (MB 32-63) */
    if (u32Iflag2 != 0U)
    {
        u8MbIdx = 32U;
        while (u32Iflag2 != 0U)
        {
            if ((u32Iflag2 & 0x01U) != 0U)
            {
                Can_ProcessTxInterrupt(0U, u8MbIdx);
            }
            u32Iflag2 >>= 1U;
            u8MbIdx++;
        }
    }

    /* Process error interrupts */
    if ((u32Esr & (CAN_ESR_BOFF_INT_MASK | CAN_ESR_ERR_INT_MASK |
                   CAN_ESR_BIT1_ERR_MASK | CAN_ESR_BIT0_ERR_MASK |
                   CAN_ESR_STUFF_ERR_MASK | CAN_ESR_CRC_ERR_MASK |
                   CAN_ESR_ACK_ERR_MASK | CAN_ESR_FORM_ERR_MASK)) != 0U)
    {
        Can_ProcessErrorInterrupt(0U, u32Esr);
    }
}

void Can_IsrHandler_Controller1(void)
{
    Can_FlexCAN_RegType* pBase;
    uint32_t u32Iflag1;
    uint32_t u32Iflag2;
    uint32_t u32Esr;
    uint8_t u8MbIdx;

    pBase = (Can_FlexCAN_RegType*)CAN1_BASE;
    if (pBase == NULL_PTR)
    {
        return;
    }

    /* Read interrupt flags */
    u32Iflag1 = pBase->IFLAG1;
    u32Iflag2 = pBase->IFLAG2;
    u32Esr = pBase->ESR1;

    /* Clear ESR1 error flags */
    pBase->ESR1 = u32Esr;

    /* Process TX interrupts (MB 0-31) */
    if (u32Iflag1 != 0U)
    {
        u8MbIdx = 0U;
        while (u32Iflag1 != 0U)
        {
            if ((u32Iflag1 & 0x01U) != 0U)
            {
                Can_ProcessTxInterrupt(1U, u8MbIdx);
            }
            u32Iflag1 >>= 1U;
            u8MbIdx++;
        }
    }

    /* Process TX interrupts (MB 32-63) */
    if (u32Iflag2 != 0U)
    {
        u8MbIdx = 32U;
        while (u32Iflag2 != 0U)
        {
            if ((u32Iflag2 & 0x01U) != 0U)
            {
                Can_ProcessTxInterrupt(1U, u8MbIdx);
            }
            u32Iflag2 >>= 1U;
            u8MbIdx++;
        }
    }

    /* Process error interrupts */
    if ((u32Esr & (CAN_ESR_BOFF_INT_MASK | CAN_ESR_ERR_INT_MASK |
                   CAN_ESR_BIT1_ERR_MASK | CAN_ESR_BIT0_ERR_MASK |
                   CAN_ESR_STUFF_ERR_MASK | CAN_ESR_CRC_ERR_MASK |
                   CAN_ESR_ACK_ERR_MASK | CAN_ESR_FORM_ERR_MASK)) != 0U)
    {
        Can_ProcessErrorInterrupt(1U, u32Esr);
    }
}

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

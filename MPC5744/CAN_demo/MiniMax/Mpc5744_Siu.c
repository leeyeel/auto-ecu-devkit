/**
 * @file    Mpc5744_Siu.c
 * @brief   SIU Driver Implementation for MPC5744
 * @details System Integration Unit driver for pin configuration
 *          and interrupt setup.
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
#include "Mpc5744_Siu_Cfg.h"

/*
 *****************************************************************************************
 * LOCAL MACROS
 *****************************************************************************************
 */

/** @brief SIU PCR register address calculation */
#define SIU_PCR_ADDR(p, n)             (SIU_BASE + 0x40U + (((p) * 16U) + (n)) * 2U)

/*
 *****************************************************************************************
 * STATIC VARIABLES
 *****************************************************************************************
 */

/**
 * @brief   SIU Interrupt Priority Configuration
 * @details Stores the configured priority for each IRQ
 */
static uint8_t Siu_au8IntPriority[256U];

/*
 *****************************************************************************************
 * FUNCTION DEFINITIONS
 *****************************************************************************************
 */

void Siu_CanPinConfig(uint8_t u8Port, uint8_t u8Pin, boolean bIsTx)
{
    volatile uint16_t* pPcrReg;
    uint16_t u16PcrValue;

    /* Validate port and pin */
    if ((u8Port > 7U) || (u8Pin > 15U))
    {
        /* Invalid port or pin - return early */
        return;
    }

    /* Get PCR register address */
    pPcrReg = (volatile uint16_t*)(SIU_BASE + 0x40U + (((u8Port * 16U) + u8Pin) * 2U));

    /* Configure PCR register */
    if (bIsTx == TRUE)
    {
        /* TX pin: Output, no pull, not open drain */
        u16PcrValue = (uint16_t)(SIU_PCR_DIR_OUTPUT | SIU_PCR_PUE_DISABLED);
    }
    else
    {
        /* RX pin: Input, enable input buffer, pull-up */
        u16PcrValue = (uint16_t)(SIU_PCR_IBE_MASK | SIU_PCR_PUE_ENABLED | SIU_PCR_PUS_UP);
    }

    /* Write PCR value */
    *pPcrReg = u16PcrValue;
}

void Siu_SetIntPriority(uint8_t u8Irq, uint8_t u8Priority)
{
    /* Validate IRQ number */
    if (u8Irq >= 256U)
    {
        return;
    }

    /* Validate priority range (0-15) */
    if (u8Priority > 15U)
    {
        u8Priority = 15U;
    }

    /* Store priority */
    Siu_au8IntPriority[u8Irq] = u8Priority;

    /* Set INTC PSR priority (for software vector mode) */
    /* Note: This is a simplified implementation */
    /* In production, this would use the full INTC driver */
    if (u8Irq < 128U)
    {
        /* PSR0-PSR127 are in the first half of PSR space */
        INTC_PSR(u8Irq) = u8Priority;
    }
}

void Siu_EnableInt(uint8_t u8Irq)
{
    /* Validate IRQ number */
    if (u8Irq >= 256U)
    {
        return;
    }

    /* Enable interrupt in INTC */
    /* This is a simplified implementation */
    /* In production, this would use the full INTC driver */

    /* Set software priority if not already set */
    if (Siu_au8IntPriority[u8Irq] == 0U)
    {
        Siu_au8IntPriority[u8Irq] = SIU_CAN_INT_PRIORITY;
    }

    /* Enable the interrupt request */
    /* For external interrupts, this would be in the specific peripheral */
    /* For CAN, the interrupt enable is in the CAN controller registers */
}

void Siu_DisableInt(uint8_t u8Irq)
{
    /* Validate IRQ number */
    if (u8Irq >= 256U)
    {
        return;
    }

    /* Disable interrupt in INTC */
    /* This is a simplified implementation */
    /* In production, this would use the full INTC driver */
    /* For CAN, the interrupt disable is in the CAN controller registers */
}

/*
 *****************************************************************************************
 * INTERRUPT SERVICE ROUTINES
 *****************************************************************************************
 */

/**
 * @brief   CAN0 Interrupt Handler
 * @details External wrapper for CAN0 ISR.
 *          Calls the CAN driver handler.
 */
void CAN0_Handler(void)
{
    extern void Can_IsrHandler_Controller0(void);
    Can_IsrHandler_Controller0();
}

/**
 * @brief   CAN1 Interrupt Handler
 * @details External wrapper for CAN1 ISR.
 *          Calls the CAN driver handler.
 */
void CAN1_Handler(void)
{
    extern void Can_IsrHandler_Controller1(void);
    Can_IsrHandler_Controller1();
}

/**
 * @brief   CAN2 Interrupt Handler
 * @details External wrapper for CAN2 ISR.
 */
void CAN2_Handler(void)
{
    /* CAN2 not configured in this demo */
}

/**
 * @brief   CAN3 Interrupt Handler
 * @details External wrapper for CAN3 ISR.
 */
void CAN3_Handler(void)
{
    /* CAN3 not configured in this demo */
}

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

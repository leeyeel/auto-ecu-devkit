/**
 * @file    Mpc5744_Siu_Cfg.h
 * @brief   SIU Configuration for MPC5744
 * @details System Integration Unit configuration for interrupt
 *          setup and pin configuration.
 *
 * @note    Compliant with MISRA C:2012 coding standard
 *
 * @version 1.0.0
 * @date    2025-01-07
 *
 * @copyright Copyright (c) 2025
 */

#ifndef MPC5744_SIU_CFG_H
#define MPC5744_SIU_CFG_H

/*
 *****************************************************************************************
 * INCLUDES
 *****************************************************************************************
 */
#include "Std_Types.h"

/*
 *****************************************************************************************
 * INTERRUPT CONFIGURATION
 *****************************************************************************************
 */

/**
 * @brief   CAN0 Interrupt Request Number
 * @details FlexCAN0 interrupt vector number
 */
#define SIU_CAN0_IRQ                   (152U)

/**
 * @brief   CAN1 Interrupt Request Number
 * @details FlexCAN1 interrupt vector number
 */
#define SIU_CAN1_IRQ                   (153U)

/**
 * @brief   CAN2 Interrupt Request Number
 * @details FlexCAN2 interrupt vector number
 */
#define SIU_CAN2_IRQ                   (154U)

/**
 * @brief   CAN3 Interrupt Request Number
 * @details FlexCAN3 interrupt vector number
 */
#define SIU_CAN3_IRQ                   (155U)

/**
 * @brief   INTC Interrupt Priority Level for CAN
 * @details Software priority level for CAN interrupts (0-15)
 *          Lower value = higher priority
 */
#define SIU_CAN_INT_PRIORITY           (100U)

/**
 * @brief   INTC Software Vector Table Entry
 * @details Index into the software vector table
 */
#define SIU_CAN0_VEC_TABLE_ENTRY       (56U)
#define SIU_CAN1_VEC_TABLE_ENTRY       (57U)
#define SIU_CAN2_VEC_TABLE_ENTRY       (58U)
#define SIU_CAN3_VEC_TABLE_ENTRY       (59U)

/*
 *****************************************************************************************
 * CAN PIN CONFIGURATION
 *****************************************************************************************
 */

/**
 * @brief   CAN0 TX Pin Port
 */
#define SIU_CAN0_TX_PORT               (SIU_PORTC)

/**
 * @brief   CAN0 TX Pin Index
 */
#define SIU_CAN0_TX_PIN                (10U)

/**
 * @brief   CAN0 RX Pin Port
 */
#define SIU_CAN0_RX_PORT               (SIU_PORTC)

/**
 * @brief   CAN0 RX Pin Index
 */
#define SIU_CAN0_RX_PIN                (11U)

/**
 * @brief   CAN1 TX Pin Port
 */
#define SIU_CAN1_TX_PORT               (SIU_PORTA)

/**
 * @brief   CAN1 TX Pin Index
 */
#define SIU_CAN1_TX_PIN                (12U)

/**
 * @brief   CAN1 RX Pin Port
 */
#define SIU_CAN1_RX_PORT               (SIU_PORTA)

/**
 * @brief   CAN1 RX Pin Index
 */
#define SIU_CAN1_RX_PIN                (13U)

/**
 * @brief   CAN2 TX Pin Port
 */
#define SIU_CAN2_TX_PORT               (SIU_PORTE)

/**
 * @brief   CAN2 TX Pin Index
 */
#define SIU_CAN2_TX_PIN                (4U)

/**
 * @brief   CAN2 RX Pin Port
 */
#define SIU_CAN2_RX_PORT               (SIU_PORTE)

/**
 * @brief   CAN2 RX Pin Index
 */
#define SIU_CAN2_RX_PIN                (5U)

/**
 * @brief   CAN3 TX Pin Port
 */
#define SIU_CAN3_TX_PORT               (SIU_PORTC)

/**
 * @brief   CAN3 TX Pin Index
 */
#define SIU_CAN3_TX_PIN                (6U)

/**
 * @brief   CAN3 RX Pin Port
 */
#define SIU_CAN3_RX_PORT               (SIU_PORTC)

/**
 * @brief   CAN3 RX Pin Index
 */
#define SIU_CAN3_RX_PIN                (7U)

/*
 *****************************************************************************************
 * SIU REGISTER ADDRESSES
 *****************************************************************************************
 */

/**
 * @brief   SIU Base Address
 */
#define SIU_BASE                       (0xFFEC0000UL)

/**
 * @brief   SIU PCR Register (Pin Configuration)
 */
#define SIU_PCR(pin)                   (*(volatile uint16_t*)(SIU_BASE + 0x40 + ((pin) * 2U)))

/**
 * @brief   SIU GPIO Input Data Register
 */
#define SIU_GPDI(port)                 (*(volatile uint32_t*)(SIU_BASE + 0x800 + ((port) * 64U)))

/**
 * @brief   SIU GPIO Output Data Register
 */
#define SIU_GPDO(port)                 (*(volatile uint32_t*)(SIU_BASE + 0xC00 + ((port) * 64U)))

/*
 *****************************************************************************************
 * PORT DEFINITIONS
 *****************************************************************************************
 */

/** @brief Port A */
#define SIU_PORTA                      (0U)

/** @brief Port B */
#define SIU_PORTB                      (1U)

/** @brief Port C */
#define SIU_PORTC                      (2U)

/** @brief Port D */
#define SIU_PORTD                      (3U)

/** @brief Port E */
#define SIU_PORTE                      (4U)

/** @brief Port F */
#define SIU_PORTF                      (5U)

/** @brief Port G */
#define SIU_PORTG                      (6U)

/** @brief Port H */
#define SIU_PORTH                      (7U)

/*
 *****************************************************************************************
 * PCR (Pin Control Register) BIT DEFINITIONS
 *****************************************************************************************
 */

/** @brief PCR: Port Number (0-7) */
#define SIU_PCR_PORT_MASK              (0xE000U)
#define SIU_PCR_PORT_SHIFT             (13U)

/** @brief PCR: Pin Number (0-15) */
#define SIU_PCR_PIN_MASK               (0x1F00U)
#define SIU_PCR_PIN_SHIFT              (8U)

/** @brief PCR: Input/Output */
#define SIU_PCR_ODE_MASK               (0x0020U)
#define SIU_PCR_ODE_SHIFT              (5U)

/** @brief PCR: Input/Output */
#define SIU_PCR_DIR_MASK               (0x0010U)
#define SIU_PCR_DIR_SHIFT              (4U)

/** @brief PCR: Pull Configuration */
#define SIU_PCR_PUE_MASK               (0x000CU)
#define SIU_PCR_PUE_SHIFT              (2U)

/** @brief PCR: Pull Select */
#define SIU_PCR_PUS_MASK               (0x0002U)
#define SIU_PCR_PUS_SHIFT              (1U)

/** @brief PCR: Input Buffer Enable */
#define SIU_PCR_IBE_MASK               (0x0001U)
#define SIU_PCR_IBE_SHIFT              (0U)

/* PCR: Pull Enable Values */
#define SIU_PCR_PUE_DISABLED           (0U)
#define SIU_PCR_PUE_ENABLED            (3U)

/* PCR: Pull Select Values */
#define SIU_PCR_PUS_DOWN               (0U)
#define SIU_PCR_PUS_UP                 (1U)

/* PCR: Direction Values */
#define SIU_PCR_DIR_INPUT              (0U)
#define SIU_PCR_DIR_OUTPUT             (1U)

/*
 *****************************************************************************************
 * INTC REGISTER ADDRESSES
 *****************************************************************************************
 */

/**
 * @brief   INTC Base Address
 */
#define INTC_BASE                      (0xFFEC0000UL)

/**
 * @brief   INTC Current Priority Register
 */
#define INTC_CPR(n)                    (*(volatile uint8_t*)(INTC_BASE + 0x08 + (n)))

/**
 * @brief   INTC Software Interrupt Acknowledge Register
 */
#define INTC_SOFT_SIR                  (*(volatile uint32_t*)(INTC_BASE + 0x18))

/**
 * @brief   INTC Interrupt Acknowledge Register
 */
#define INTC_IACKR                     (*(volatile uint32_t*)(INTC_BASE + 0x10))

/**
 * @brief   INTC Vector Table Base Address Register
 */
#define INTC_VTBAR                     (*(volatile uint32_t*)(INTC_BASE + 0x00))

/**
 * @brief   INTC Priority Select Register (PSR)
 */
#define INTC_PSR(n)                    (*(volatile uint8_t*)(INTC_BASE + 0x40 + (n)))

/*
 *****************************************************************************************
 * FUNCTION DECLARATIONS
 *****************************************************************************************
 */

/**
 * @brief   Configure CAN Pin
 * @details Configures a pin for CAN TX or RX function.
 * @param   u8Port       Port number (0-7)
 * @param   u8Pin        Pin number (0-15)
 * @param   bIsTx        TRUE for TX, FALSE for RX
 * @return  None
 */
void Siu_CanPinConfig(uint8_t u8Port, uint8_t u8Pin, boolean bIsTx);

/**
 * @brief   Set Interrupt Priority
 * @details Sets the priority for an interrupt source.
 * @param   u8Irq        Interrupt request number
 * @param   u8Priority   Priority level (0-15, lower is higher priority)
 * @return  None
 */
void Siu_SetIntPriority(uint8_t u8Irq, uint8_t u8Priority);

/**
 * @brief   Enable Interrupt
 * @details Enables an interrupt source in the INTC.
 * @param   u8Irq        Interrupt request number
 * @return  None
 */
void Siu_EnableInt(uint8_t u8Irq);

/**
 * @brief   Disable Interrupt
 * @details Disables an interrupt source in the INTC.
 * @param   u8Irq        Interrupt request number
 * @return  None
 */
void Siu_DisableInt(uint8_t u8Irq);

#endif /* MPC5744_SIU_CFG_H */

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

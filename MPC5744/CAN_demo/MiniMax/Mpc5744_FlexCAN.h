/**
 * @file    Mpc5744_FlexCAN.h
 * @brief   FlexCAN Register Definitions for MPC5744
 * @details Register and bit definitions for MPC5744 FlexCAN module.
 *
 * @note    Compliant with MISRA C:2012 coding standard
 *
 * @version 1.0.0
 * @date    2025-01-07
 *
 * @copyright Copyright (c) 2025
 */

#ifndef MPC5744_FLEXCAN_H
#define MPC5744_FLEXCAN_H

/*
 *****************************************************************************************
 * INCLUDES
 *****************************************************************************************
 */
#include "Std_Types.h"

/*
 *****************************************************************************************
 * BASE ADDRESSES
 *****************************************************************************************
 */

/** @brief FlexCAN 0 base address */
#define CAN0_BASE                      (0xFFEC0000UL)

/** @brief FlexCAN 1 base address */
#define CAN1_BASE                      (0xFFED0000UL)

/** @brief FlexCAN 2 base address */
#define CAN2_BASE                      (0xFFEE0000UL)

/** @brief FlexCAN 3 base address */
#define CAN3_BASE                      (0xFFEF0000UL)

/*
 *****************************************************************************************
 * MODULE CONFIGURATION REGISTER (MCR) - 0x000
 *****************************************************************************************
 */

/** @brief MCR: Module Enable */
#define CAN_MCR_MDIS_MASK              (0x80000000UL)
#define CAN_MCR_MDIS_SHIFT             (31U)

/** @brief MCR: Freeze Enable */
#define CAN_MCR_FRZ_MASK               (0x40000000UL)
#define CAN_MCR_FRZ_SHIFT              (30U)

/** @brief MCR: Freeze Acknowledge */
#define CAN_MCR_FRZACK_MASK            (0x20000000UL)
#define CAN_MCR_FRZACK_SHIFT           (29U)

/** @brief MCR: Halt */
#define CAN_MCR_HALT_MASK              (0x10000000UL)
#define CAN_MCR_HALT_SHIFT             (28U)

/** @brief MCR: Not Ready */
#define CAN_MCR_NOTRDY_MASK            (0x08000000UL)
#define CAN_MCR_NOTRDY_SHIFT           (27U)

/** @brief MCR: Wakeup Interrupt */
#define CAN_MCR_WAKINT_MASK            (0x04000000UL)
#define CAN_MCR_WAKINT_SHIFT           (26U)

/** @brief MCR: Self Wakeup */
#define CAN_MCR_SLFWAK_MASK            (0x02000000UL)
#define CAN_MCR_SLFWAK_SHIFT           (25U)

/** @brief MCR: Supervisor Mode */
#define CAN_MCR_SUPV_MASK              (0x01000000UL)
#define CAN_MCR_SUPV_SHIFT             (24U)

/** @brief MCR: FIFO Enable */
#define CAN_MCR_FEN_MASK               (0x00800000UL)
#define CAN_MCR_FEN_SHIFT              (23U)

/** @brief MCR: Remote Request Storing */
#define CAN_MCR_RFFN_MASK              (0x00780000UL)
#define CAN_MCR_RFFN_SHIFT             (19U)

/** @brief MCR: Number of Message Buffers */
#define CAN_MCR_MAXMB_MASK             (0x0000007FUL)
#define CAN_MCR_MAXMB_SHIFT            (0U)

/** @brief MCR: Default value for initialization */
#define CAN_MCR_DEFAULT                (0xD890000FUL)

/*
 *****************************************************************************************
 * CONTROL REGISTER 1 (CTRL1) - 0x004
 *****************************************************************************************
 */

/** @brief CTRL1: Prescaler Division Factor */
#define CAN_CTRL1_PRESDIV_MASK         (0xFF000000UL)
#define CAN_CTRL1_PRESDIV_SHIFT        (24U)

/** @brief CTRL1: Resynchronization Jump Width */
#define CAN_CTRL1_RJW_MASK             (0x00C00000UL)
#define CAN_CTRL1_RJW_SHIFT            (22U)

/** @brief CTRL1: Phase Segment 1 */
#define CAN_CTRL1_PS1_MASK             (0x00380000UL)
#define CAN_CTRL1_PS1_SHIFT            (19U)

/** @brief CTRL1: Phase Segment 2 */
#define CAN_CTRL1_PS2_MASK             (0x00070000UL)
#define CAN_CTRL1_PS2_SHIFT            (16U)

/** @brief CTRL1: Loop Back Mode */
#define CAN_CTRL1_LPB_MASK             (0x00008000UL)
#define CAN_CTRL1_LPB_SHIFT            (15U)

/** @brief CTRL1: Triple Sampling */
#define CAN_CTRL1_SMP_MASK             (0x00004000UL)
#define CAN_CTRL1_SMP_SHIFT            (14U)

/** @brief CTRL1: Bus Off Mask */
#define CAN_CTRL1_BOFFMSK_MASK         (0x00002000UL)
#define CAN_CTRL1_BOFFMSK_SHIFT        (13U)

/** @brief CTRL1: Error Mask */
#define CAN_CTRL1_ERRMSK_MASK          (0x00001000UL)
#define CAN_CTRL1_ERRMSK_SHIFT         (12U)

/** @brief CTRL1: Transmit Attenuator */
#define CAN_CTRL1_TXATT_MASK           (0x00000800UL)
#define CAN_CTRL1_TXATT_SHIFT          (11U)

/** @brief CTRL1: Sampling Mode */
#define CAN_CTRL1_SAMP_MASK            (0x00000400UL)
#define CAN_CTRL1_SAMP_SHIFT           (10U)

/** @brief CTRL1: Receiver Wakeup Mode */
#define CAN_CTRL1_RWRNMSK_MASK         (0x00000200UL)
#define CAN_CTRL1_RWRNMSK_SHIFT        (9U)

/** @brief CTRL1: Transmitter Wakeup Mode */
#define CAN_CTRL1_TWRNMSK_MASK         (0x00000100UL)
#define CAN_CTRL1_TWRNMSK_SHIFT        (8U)

/** @brief CTRL1: Error Counter */
#define CAN_CTRL1_ERRCNT_MASK          (0x00000060UL)
#define CAN_CTRL1_ERRCNT_SHIFT         (5U)

/** @brief CTRL1: Default value for initialization */
#define CAN_CTRL1_DEFAULT              (0x00000001UL)

/*
 *****************************************************************************************
 * ERROR COUNTER REGISTER (ECR) - 0x01C
 *****************************************************************************************
 */

/** @brief ECR: Transmit Error Counter */
#define CAN_ECR_TXECTR_MASK            (0x00FF0000UL)
#define CAN_ECR_TXECTR_SHIFT           (16U)

/** @brief ECR: Receive Error Counter */
#define CAN_ECR_RXECTR_MASK            (0x0000FF00UL)
#define CAN_ECR_RXECTR_SHIFT           (8U)

/** @brief ECR: Receive Error Counter (legacy) */
#define CAN_ECR_RXECTR_LEGACY_MASK     (0x000000FFUL)
#define CAN_ECR_RXECTR_LEGACY_SHIFT    (0U)

/*
 *****************************************************************************************
 * ERROR AND STATUS REGISTER 1 (ESR1) - 0x020
 *****************************************************************************************
 */

/** @brief ESR1: Message Buffer Interrupt */
#define ESR1_MB_INT_MASK               (0xFFFF0000UL)
#define ESR1_MB_INT_SHIFT              (16U)

/** @brief ESR1: Reserved */
#define ESR1_RESERVED1_MASK            (0x00008000UL)
#define ESR1_RESERVED1_SHIFT           (15U)

/** @brief ESR1: Bus Off Interrupt */
#define CAN_ESR_BOFF_INT_MASK          (0x00004000UL)
#define CAN_ESR_BOFF_INT_SHIFT         (14U)

/** @brief ESR1: Error Interrupt */
#define CAN_ESR_ERR_INT_MASK           (0x00002000UL)
#define CAN_ESR_ERR_INT_SHIFT          (13U)

/** @brief ESR1: Wakeup Interrupt */
#define CAN_ESR_WAK_INT_MASK           (0x00001000UL)
#define CAN_ESR_WAK_INT_SHIFT          (12U)

/** @brief ESR1: FLTCONFN (Fault Confinement State) */
#define CAN_ESR_FLT_CONF_MASK          (0x00000C00UL)
#define CAN_ESR_FLT_CONF_SHIFT         (10U)

/** @brief ESR1: Receive Error Warning */
#define CAN_ESR_RXWRN_MASK             (0x00000200UL)
#define CAN_ESR_RXWRN_SHIFT            (9U)

/** @brief ESR1: Transmit Error Warning */
#define CAN_ESR_TXWRN_MASK             (0x00000100UL)
#define CAN_ESR_TXWRN_SHIFT            (8U)

/** @brief ESR1: IDLE */
#define CAN_ESR_IDLE_MASK              (0x00000080UL)
#define CAN_ESR_IDLE_SHIFT             (7U)

/** @brief ESR1: TXRX (Transmit/Receive State) */
#define CAN_ESR_TXRX_MASK              (0x00000040UL)
#define CAN_ESR_TXRX_SHIFT             (6U)

/** @brief ESR1: Least Significant Bit of Code */
#define CAN_ESR_LST_CODE_MASK          (0x00000038UL)
#define CAN_ESR_LST_CODE_SHIFT         (3U)

/** @brief ESR1: Bit 1 Error */
#define CAN_ESR_BIT1_ERR_MASK          (0x00000004UL)
#define CAN_ESR_BIT1_ERR_SHIFT         (2U)

/** @brief ESR1: Bit 0 Error */
#define CAN_ESR_BIT0_ERR_MASK          (0x00000002UL)
#define CAN_ESR_BIT0_ERR_SHIFT         (1U)

/** @brief ESR1: Stuff Error */
#define CAN_ESR_STUFF_ERR_MASK         (0x00000020UL)
#define CAN_ESR_STUFF_ERR_SHIFT        (5U)

/** @brief ESR1: CRC Error */
#define CAN_ESR_CRC_ERR_MASK           (0x00000010UL)
#define CAN_ESR_CRC_ERR_SHIFT          (4U)

/** @brief ESR1: ACK Error */
#define CAN_ESR_ACK_ERR_MASK           (0x00000008UL)
#define CAN_ESR_ACK_ERR_SHIFT          (3U)

/** @brief ESR1: Form Error */
#define CAN_ESR_FORM_ERR_MASK          (0x00000002UL)
#define CAN_ESR_FORM_ERR_SHIFT         (1U)

/** @brief ESR1: All Error Flags Mask */
#define CAN_ESR_ALL_ERR_MASK           (0x0000003FU)

/*
 *****************************************************************************************
 * INTERRUPT MASK REGISTER 1 (IMASK1) - 0x028
 *****************************************************************************************
 */

/** @brief IMASK1: Message Buffer Mask (MB 0-31) */
#define CAN_IMASK1_MB_MASK             (0xFFFFFFFFUL)

/*
 *****************************************************************************************
 * INTERRUPT FLAG REGISTER 1 (IFLAG1) - 0x02C
 *****************************************************************************************
 */

/** @brief IFLAG1: Message Buffer 0 Interrupt Flag */
#define CAN_IFLAG1_MB0_MASK            (0x00000001UL)

/** @brief IFLAG1: Message Buffer 1 Interrupt Flag */
#define CAN_IFLAG1_MB1_MASK            (0x00000002UL)

/** @brief IFLAG1: Message Buffer 31 Interrupt Flag */
#define CAN_IFLAG1_MB31_MASK           (0x80000000UL)

/** @brief IFLAG1: Bus Off Interrupt Flag */
#define CAN_IFLAG1_BOFF_MASK           (0x00008000UL)

/** @brief IFLAG1: Error Interrupt Flag */
#define CAN_IFLAG1_ERR_MASK            (0x00004000UL)

/** @brief IFLAG1: Wakeup Interrupt Flag */
#define CAN_IFLAG1_WAK_MASK            (0x00002000UL)

/** @brief IFLAG1: Rx FIFO Overflow Flag */
#define CAN_IFLAG1_RXFIFO_OVR_MASK     (0x00000800UL)

/** @brief IFLAG1: Rx FIFO Warning Flag */
#define CAN_IFLAG1_RXFIFO_WARN_MASK    (0x00000400UL)

/** @brief IFLAG1: Rx FIFO Interrupt Flag */
#define CAN_IFLAG1_RXFIFO_MASK         (0x00000200UL)

/*
 *****************************************************************************************
 * MESSAGE BUFFER CONTROL/STATUS (CS) - Per MB
 *****************************************************************************************
 */

/** @brief MB CS: Code Field */
#define CAN_MB_CODE_MASK               (0x0F000000UL)
#define CAN_MB_CODE_SHIFT              (24U)

/** @brief MB CS: Message Buffer Direction */
#define CAN_MB_DIR_MASK                (0x01000000UL)
#define CAN_MB_DIR_SHIFT               (24U)

/** @brief MB CS: Reserved */
#define CAN_MB_SRR_MASK                (0x00400000UL)
#define CAN_MB_SRR_SHIFT               (22U)

/** @brief MB CS: IDE (Extended ID) */
#define CAN_MB_IDE_MASK                (0x00200000UL)
#define CAN_MB_IDE_SHIFT               (21U)

/** @brief MB CS: SRR (Substitute Remote Request) */
#define CAN_MB_SRR_MASK                (0x00100000UL)
#define CAN_MB_SRR_SHIFT               (20U)

/** @brief MB CS: Data Length Code */
#define CAN_MB_DLC_MASK                (0x000F0000UL)
#define CAN_MB_DLC_SHIFT               (16U)

/** @brief MB CS: Time Stamp (valid when TX/RX complete) */
#define CAN_MB_TIME_MASK               (0x0000FFFFUL)
#define CAN_MB_TIME_SHIFT              (0U)

/** @brief MB Code: TX Inactive */
#define CAN_MB_CODE_TX_INACTIVE        (0x08U)

/** @brief MB Code: TX Data/Remote (start transmission) */
#define CAN_MB_CODE_TX_DATA            (0x0CU)

/** @brief MB Code: TX Remote Request */
#define CAN_MB_CODE_TX_REMOTE          (0x0AU)

/** @brief MB Code: RX Inactive */
#define CAN_MB_CODE_INACTIVE           (0x00U)

/** @brief MB Code: RX Empty */
#define CAN_MB_CODE_RX_EMPTY           (0x04U)

/** @brief MB Code: RX Full */
#define CAN_MB_CODE_RX_FULL            (0x02U)

/** @brief MB Code: RX Overrun */
#define CAN_MB_CODE_RX_OVERRUN         (0x06U)

/*
 *****************************************************************************************
 * MESSAGE BUFFER IDENTIFIER (ID) - Per MB
 *****************************************************************************************
 */

/** @brief MB ID: Extended ID mask (29-bit) */
#define CAN_MB_ID_EXT_MASK             (0x1FFFFFFFU)
#define CAN_MB_ID_EXT_SHIFT            (0U)

/** @brief MB ID: Standard ID mask (11-bit) */
#define CAN_MB_ID_STD_MASK             (0x1FFFFFFFU)
#define CAN_MB_ID_STD_SHIFT            (18U)

/** @brief MB ID: Extended ID Flag */
#define CAN_MB_ID_EXT_FLAG             (0x40000000UL)

/** @brief MB ID: Priority Enhancement */
#define CAN_MB_ID_PRIO_MASK            (0xE0000000UL)
#define CAN_MB_ID_PRIO_SHIFT           (29U)

/*
 *****************************************************************************************
 * FREE RUNNING TIMER (TIMER) - 0x008
 *****************************************************************************************
 */

/** @brief TIMER: Free running timer value */
#define CAN_TIMER_MASK                 (0x0000FFFFUL)

/*
 *****************************************************************************************
 * RX FIFO GLOBAL MASK (RXMGMASK) - 0x010
 *****************************************************************************************
 */

/** @brief RXMGMASK: Acceptance mask bits */
#define CAN_RXMGMASK_MASK              (0x1FFFFFFFU)

/*
 *****************************************************************************************
 * RX FIFO FILTER ELEMENTS - Per Filter
 *****************************************************************************************
 */

/** @brief Rx FIFO: Acceptance Filter Extended */
#define CAN_RXFIFO_EXT_MASK            (0x1FFFFFFFU)

/** @brief Rx FIFO: Acceptance Filter Standard */
#define CAN_RXFIFO_STD_MASK            (0x1FFFFFFFU)

/*
 *****************************************************************************************
 * RX INDIVIDUAL MASK REGISTERS (RXIMR) - 0x880 - 0x9FC
 *****************************************************************************************
 */

/** @brief RXIMR: Individual message buffer mask */
#define CAN_RXIMR_MASK                 (0x1FFFFFFFU)

#endif /* MPC5744_FLEXCAN_H */

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

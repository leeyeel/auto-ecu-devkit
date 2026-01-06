/**
 * @file    Can_PBcfg.h
 * @brief   CAN Driver Post-Build Configuration for MPC5744
 * @details Post-build configuration header for CAN driver.
 *          Contains CAN controller, baudrate, and mailbox configurations.
 *
 * @note    Compliant with MISRA C:2012 coding standard
 *
 * @version 1.0.0
 * @date    2025-01-07
 *
 * @copyright Copyright (c) 2025
 */

#ifndef CAN_PBCFG_H
#define CAN_PBCFG_H

/*
 *****************************************************************************************
 * INCLUDES
 *****************************************************************************************
 */
#include "Can.h"
#include "Can_Cfg.h"

/*
 *****************************************************************************************
 * CONFIGURATION CONSTANTS
 *****************************************************************************************
 */

/** @brief Number of CAN controllers available on MPC5744 */
#define CAN_CONTROLLER_COUNT_U8         (2U)

/** @brief Number of message buffers per CAN controller (FlexCAN) */
#define CAN_MB_COUNT_U8                 (64U)

/** @brief Standard ID filter count for controller 0 */
#define CAN_CTRL0_RX_FIFO_FILTERS_U8   (8U)

/** @brief Standard ID filter count for controller 1 */
#define CAN_CTRL1_RX_FIFO_FILTERS_U8   (8U)

/**
 * @brief   CAN Controller Configuration
 * @details Post-build configuration for each CAN controller
 */
typedef struct
{
    /** @brief CAN controller ID (0 or 1) */
    uint8_t             u8ControllerId;

    /** @brief CAN controller base address */
    Can_ControllerType  pBaseAddress;

    /** @brief CAN bus-off recovery mode:
     *         - CAN_BUSOFF_RECOVERY_AUTO: Automatic recovery
     *         - CAN_BUSOFF_RECOVERY_MANUAL: Manual recovery */
    uint8_t             u8BusoffRecoveryMode;

    /** @brief CAN controller initialization status */
    boolean             bControllerActivation;

    /** @brief Baudrate configuration index */
    uint8_t             u8BaudrateConfigId;

    /** @brief Wakeup source:
     *         - CAN_WAKEUP_SOURCE_NONE: No wakeup
     *         - CAN_WAKEUP_SOURCE_CAN: CAN wakeup enabled */
    uint8_t             u8WakeupSource;

    /** @brief Transmit FIFO/Queue configuration */
    Can_TxFifoConfigType stTxFifoConfig;

    /** @brief Rx FIFO configuration (optional) */
    Can_RxFifoConfigType stRxFifoConfig;

} Can_ControllerConfigType;

/**
 * @brief   Baudrate Configuration
 * @details FlexCAN baudrate timing parameters
 */
typedef struct
{
    /** @brief Prescaler division factor (from CAN clock) */
    uint32_t            u32Prescaler;

    /** @brief Resynchronization Jump Width (1-4) */
    uint8_t             u8RJW;

    /** @brief Propagation Segment (1-8) */
    uint8_t             u8PropSeg;

    /** @brief Phase Segment 1 (1-8) */
    uint8_t             u8PS1;

    /** @brief Phase Segment 2 (1-8) */
    uint8_t             u8PS2;

    /** @brief Triple sampling mode:
     *         - FALSE: Single sampling at sample point
     *         - TRUE: Triple sampling */
    boolean             bTripleSampling;

} Can_BaudrateConfigType;

/**
 * @brief   Message Buffer Configuration
 * @details Configuration for individual message buffers
 */
typedef struct
{
    /** @brief Message buffer number */
    uint8_t             u8MessageBuffer;

    /** @brief Message buffer type:
     *         - CAN_MB_TX: Transmit buffer
     *         - CAN_MB_RX: Receive buffer
     *         - CAN_MB_RXFIFO: Rx FIFO buffer */
    uint8_t             u8MessageBufferType;

    /** @brief CAN ID type:
     *         - CAN_ID_TYPE_STANDARD: Standard ID (11-bit)
     *         - CAN_ID_TYPE_EXTENDED: Extended ID (29-bit) */
    uint8_t             u8IdType;

    /** @brief CAN Identifier (11-bit or 29-bit based on u8IdType) */
    uint32_t            u32CanId;

    /** @brief Data length code (0-8 bytes) */
    uint8_t             u8DLC;

    /** @brief Enable tx interrupt for this MB */
    boolean             bTxInterrupt;

    /** @brief Enable rx interrupt for this MB */
    boolean             bRxInterrupt;

} Can_MessageBufferConfigType;

/**
 * @brief   Tx FIFO Configuration
 * @details Transmit FIFO/Queue configuration
 */
typedef struct
{
    /** @brief Enable Tx FIFO mode */
    boolean             bFifoEnable;

    /** @brief Number of elements in Tx FIFO (if enabled) */
    uint8_t             u8FifoDepth;

    /** @brief Priority mechanism:
     *         - CAN_FIFO_PRIORITY_ORDER: Priority by ID
     *         - CAN_FIFO_PRIORITY_REQUEST: Priority by request order */
    uint8_t             u8PriorityMode;

} Can_TxFifoConfigType;

/**
 * @brief   Rx FIFO Configuration
 * @details Receive FIFO configuration for receive filtering
 */
typedef struct
{
    /** @brief Enable Rx FIFO */
    boolean             bFifoEnable;

    /** @brief Rx FIFO watermark (interrupt trigger) */
    uint8_t             u8Watermark;

    /** @brief Number of filters to use */
    uint8_t             u8FilterCount;

    /** @brief Filter mechanism:
     *         - CAN_FIFO_FILTER_A: One full ID per filter
     *         - CAN_FIFO_FILTER_B: Two full IDs per filter
     *         - CAN_FIFO_FILTER_C: Four partial IDs per filter */
    uint8_t             u8FilterMechanism;

} Can_RxFifoConfigType;

/*
 *****************************************************************************************
 * BAUDRATE CONFIGURATIONS
 *****************************************************************************************
 */

/**
 * @brief   Baudrate Configuration Array
 * @details Pre-defined baudrate configurations for 500kbps at 40MHz CAN clock
 */
extern const Can_BaudrateConfigType Can_au8BaudrateConfig[CAN_BAUD_RATE_CONFIG_COUNT];

/**
 * @brief   500 kbps @ 40MHz CAN clock
 * @details Detailed timing calculation:
 *          - CAN clock: 40 MHz
 *          - Prescaler: 4 -> 10 MHz SCK
 *          - Time quantum: 100 ns
 *          - Total TQ: 20 (1 + 1 + 8 + 10)
 *          - Sample point: 65% (13/20)
 */
#define CAN_BAUD_500KBPS_40MHZ  \
    {                           \
        .u32Prescaler    = 4U,  \
        .u8RJW           = 4U,  \
        .u8PropSeg       = 2U,  \
        .u8PS1           = 8U,  \
        .u8PS2           = 10U, \
        .bTripleSampling = FALSE \
    }

/**
 * @brief   1 Mbps @ 40MHz CAN clock
 * @details High speed configuration
 */
#define CAN_BAUD_1MBPS_40MHZ   \
    {                           \
        .u32Prescaler    = 2U,  \
        .u8RJW           = 2U,  \
        .u8PropSeg       = 1U,  \
        .u8PS1           = 5U,  \
        .u8PS2           = 5U,  \
        .bTripleSampling = FALSE \
    }

#endif /* CAN_PBCFG_H */

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

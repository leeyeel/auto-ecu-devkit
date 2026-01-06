/**
 * @file    Can_PBcfg.c
 * @brief   CAN Driver Post-Build Configuration for MPC5744
 * @details Post-build configuration source for CAN driver.
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
#include "Can_PBcfg.h"

/*
 *****************************************************************************************
 * CONSTANT DEFINITIONS
 *****************************************************************************************
 */

/**
 * @brief   Baudrate Configuration Array
 * @details Pre-defined baudrate configurations
 */
const Can_BaudrateConfigType Can_au8BaudrateConfig[CAN_BAUD_RATE_CONFIG_COUNT] =
{
    /* Baudrate configuration 0: 500 kbps @ 40MHz */
    CAN_BAUD_500KBPS_40MHZ,

    /* Baudrate configuration 1: 1 Mbps @ 40MHz */
    CAN_BAUD_1MBPS_40MHZ
};

/**
 * @brief   Message Buffer Configuration for Controller 0
 * @details Configuration of Tx/Rx message buffers for CAN0
 */
static const Can_MessageBufferConfigType Can_astrMbConfig_Can0[] =
{
    /* TX MB 0: Battery Voltage Broadcast - Standard ID 0x101 */
    {
        .u8MessageBuffer   = 0U,
        .u8MessageBufferType = CAN_MB_TX,
        .u8IdType          = CAN_ID_TYPE_STANDARD,
        .u32CanId          = 0x101U,
        .u8DLC             = 8U,
        .bTxInterrupt      = TRUE,
        .bRxInterrupt      = FALSE
    },

    /* TX MB 1: Battery Status Message - Standard ID 0x102 */
    {
        .u8MessageBuffer   = 1U,
        .u8MessageBufferType = CAN_MB_TX,
        .u8IdType          = CAN_ID_TYPE_STANDARD,
        .u32CanId          = 0x102U,
        .u8DLC             = 4U,
        .bTxInterrupt      = TRUE,
        .bRxInterrupt      = FALSE
    },

    /* RX MB 8: Receive CAN Diagnostic Messages - Standard ID 0x200 */
    {
        .u8MessageBuffer   = 8U,
        .u8MessageBufferType = CAN_MB_RX,
        .u8IdType          = CAN_ID_TYPE_STANDARD,
        .u32CanId          = 0x200U,
        .u8DLC             = 8U,
        .bTxInterrupt      = FALSE,
        .bRxInterrupt      = TRUE
    },

    /* RX MB 9: Receive Remote Request - Standard ID 0x300 */
    {
        .u8MessageBuffer   = 9U,
        .u8MessageBufferType = CAN_MB_RX,
        .u8IdType          = CAN_ID_TYPE_STANDARD,
        .u32CanId          = 0x300U,
        .u8DLC             = 0U,
        .bTxInterrupt      = FALSE,
        .bRxInterrupt      = TRUE
    }
};

/**
 * @brief   Message Buffer Configuration for Controller 1
 * @details Configuration of Tx/Rx message buffers for CAN1
 */
static const Can_MessageBufferConfigType Can_astrMbConfig_Can1[] =
{
    /* TX MB 0: Battery Voltage Broadcast - Standard ID 0x201 */
    {
        .u8MessageBuffer   = 0U,
        .u8MessageBufferType = CAN_MB_TX,
        .u8IdType          = CAN_ID_TYPE_STANDARD,
        .u32CanId          = 0x201U,
        .u8DLC             = 8U,
        .bTxInterrupt      = TRUE,
        .bRxInterrupt      = FALSE
    },

    /* RX MB 8: Receive Gateway Messages - Extended ID 0x1000001 */
    {
        .u8MessageBuffer   = 8U,
        .u8MessageBufferType = CAN_MB_RX,
        .u8IdType          = CAN_ID_TYPE_EXTENDED,
        .u32CanId          = 0x1000001U,
        .u8DLC             = 8U,
        .bTxInterrupt      = FALSE,
        .bRxInterrupt      = TRUE
    }
};

/**
 * @brief   Controller Configuration Array
 * @details Post-build configuration for all CAN controllers
 */
const Can_ControllerConfigType Can_astrControllerConfig[CAN_CONTROLLER_COUNT_U8] =
{
    /* Controller 0 Configuration */
    {
        .u8ControllerId       = 0U,
        .pBaseAddress         = (Can_ControllerType)CAN0_BASE,
        .u8BusoffRecoveryMode = CAN_BUSOFF_RECOVERY_AUTO,
        .bControllerActivation = TRUE,
        .u8BaudrateConfigId   = 0U, /* 500 kbps */
        .u8WakeupSource       = CAN_WAKEUP_SOURCE_NONE,
        .stTxFifoConfig       =
        {
            .bFifoEnable    = FALSE,
            .u8FifoDepth    = 0U,
            .u8PriorityMode = CAN_FIFO_PRIORITY_ORDER
        },
        .stRxFifoConfig       =
        {
            .bFifoEnable      = FALSE,
            .u8Watermark      = 0U,
            .u8FilterCount    = 0U,
            .u8FilterMechanism = CAN_FIFO_FILTER_A
        }
    },

    /* Controller 1 Configuration */
    {
        .u8ControllerId       = 1U,
        .pBaseAddress         = (Can_ControllerType)CAN1_BASE,
        .u8BusoffRecoveryMode = CAN_BUSOFF_RECOVERY_AUTO,
        .bControllerActivation = TRUE,
        .u8BaudrateConfigId   = 0U, /* 500 kbps */
        .u8WakeupSource       = CAN_WAKEUP_SOURCE_NONE,
        .stTxFifoConfig       =
        {
            .bFifoEnable    = FALSE,
            .u8FifoDepth    = 0U,
            .u8PriorityMode = CAN_FIFO_PRIORITY_ORDER
        },
        .stRxFifoConfig       =
        {
            .bFifoEnable      = FALSE,
            .u8Watermark      = 0U,
            .u8FilterCount    = 0U,
            .u8FilterMechanism = CAN_FIFO_FILTER_A
        }
    }
};

/**
 * @brief   Message Buffer Configuration Pointers
 * @details Pointers to message buffer configurations for each controller
 */
const Can_MessageBufferConfigType* const Can_apstrMbConfig[CAN_CONTROLLER_COUNT_U8] =
{
    &Can_astrMbConfig_Can0[0],
    &Can_astrMbConfig_Can1[0]
};

/**
 * @brief   Message Buffer Count per Controller
 * @details Number of configured message buffers for each controller
 */
const uint8_t Can_au8MbConfigCount[CAN_CONTROLLER_COUNT_U8] =
{
    (uint8_t)(sizeof(Can_astrMbConfig_Can0) / sizeof(Can_MessageBufferConfigType)),
    (uint8_t)(sizeof(Can_astrMbConfig_Can1) / sizeof(Can_MessageBufferConfigType))
};

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

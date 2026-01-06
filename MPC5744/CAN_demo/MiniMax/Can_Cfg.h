/**
 * @file    Can_Cfg.h
 * @brief   CAN Driver Configuration for MPC5744
 * @details Basic configuration constants for CAN driver.
 *
 * @note    Compliant with MISRA C:2012 coding standard
 *
 * @version 1.0.0
 * @date    2025-01-07
 *
 * @copyright Copyright (c) 2025
 */

#ifndef CAN_CFG_H
#define CAN_CFG_H

/*
 *****************************************************************************************
 * INCLUDES
 *****************************************************************************************
 */
#include "Std_Types.h"

/*
 *****************************************************************************************
 * VERSION CHECK
 *****************************************************************************************
 */

/** @brief CAN Driver Module Version (Major.Minor.Patch) */
#define CAN_CFG_VERSION_MAJOR      (1U)
#define CAN_CFG_VERSION_MINOR      (0U)
#define CAN_CFG_VERSION_PATCH      (0U)

/*
 *****************************************************************************************
 * GENERAL CONFIGURATION
 *****************************************************************************************
 */

/** @brief CAN Driver development error detection */
#define CAN_CFG_DEV_ERROR_DETECT       (STD_ON)

/** @brief CAN Driver version info API */
#define CAN_CFG_VERSION_INFO_API      (STD_ON)

/** @brief CAN Driver wakeup support */
#define CAN_CFG_WAKEUP_SUPPORT        (STD_OFF)

/** @brief CAN Driver busoff notification API */
#define CAN_CFG_BUSOFF_NOTIFICATION   (STD_ON)

/** @brief CAN Driver transmit confirmation API */
#define CAN_CFG_TX_CONFIRMATION_API   (STD_ON)

/** @brief CAN Driver receive indication API */
#define CAN_CFG_RX_INDICATION_API     (STD_ON)

/** @brief CAN Driver error notification API */
#define CAN_CFG_ERROR_NOTIFICATION    (STD_ON)

/** @brief CAN Driver multi-bitfield support (Rx FIFO) */
#define CAN_CFG_MULTI_BITFIELD        (STD_OFF)

/** @brief CAN Driver message buffer locking support */
#define CAN_CFG_MB_LOCKING            (STD_ON)

/**
 * @brief   Number of Baudrate Configurations
 * @details Number of pre-defined baudrate configurations
 */
#define CAN_BAUD_RATE_CONFIG_COUNT    (2U)

/*
 *****************************************************************************************
 * CONTROLLER-SPECIFIC CONFIGURATIONS
 *****************************************************************************************
 */

/**
 * @brief   FlexCAN Clock Source Selection
 * @details MPC5744 FlexCAN can use different clock sources:
 *         - 0: Peripheral clock (default)
 *         - 1: Oscillator clock (more stable)
 */
#define CAN_FLEXCAN_CLKSRC            (0U)

/**
 * @brief   FlexCAN Maximum Payload Size
 * @details Maximum CAN FD payload size in bytes:
 *         - 8: Classic CAN (max 8 bytes)
 *         - 64: CAN FD (max 64 bytes)
 */
#define CAN_FLEXCAN_MAX_PAYLOAD       (8U)

/**
 * @brief   Rx FIFO Filter Table Size
 * @details Number of 32-bit words in Rx FIFO filter table
 *         Each filter element is 8 bytes (two 32-bit words)
 */
#define CAN_FLEXCAN_RX_FIFO_SIZE      (16U)

/*
 *****************************************************************************************
 * INTERRUPT CONFIGURATION
 *****************************************************************************************
 */

/** @brief Enable CAN interrupt for Controller 0 */
#define CAN_INTERRUPT_CONTROLLER0     (STD_ON)

/** @brief Enable CAN interrupt for Controller 1 */
#define CAN_INTERRUPT_CONTROLLER1     (STD_ON)

/**
 * @brief   CAN Interrupt Priority
 * @details Interrupt priority levels for CAN controllers
 *         (Lower value = higher priority on NVIC)
 */
#define CAN_INTERRUPT_PRIORITY_0      (100U)
#define CAN_INTERRUPT_PRIORITY_1      (100U)

/**
 * @brief   CAN Interrupt Subpriority
 * @details Subpriority for interrupt grouping (if applicable)
 */
#define CAN_INTERRUPT_SUBPRIORITY     (0U)

/*
 *****************************************************************************************
 * ERROR HANDLING CONFIGURATION
 *****************************************************************************************
 */

/** @brief Enable transmission error counter reporting */
#define CAN_CFG_ERROR_COUNTER         (STD_ON)

/** @brief Enable RX error counter reporting */
#define CAN_CFG_RX_ERROR_COUNTER      (STD_ON)

/** @brief Enable TX error counter reporting */
#define CAN_CFG_TX_ERROR_COUNTER      (STD_ON)

/**
 * @brief   Error Warning Threshold
 * @details Error counter value that triggers error warning interrupt
 */
#define CAN_ERROR_WARNING_THRESHOLD   (96U)

/**
 * @brief   Error Passive Threshold
 * @details Error counter value that triggers error passive state
 */
#define CAN_ERROR_PASSIVE_THRESHOLD   (127U)

/**
 * @brief   Bus-off Entry Threshold
 * @details Error counter value that triggers bus-off state
 */
#define CAN_BUSOFF_THRESHOLD          (255U)

/*
 *****************************************************************************************
 * BAUDRATE CONFIGURATION LIMITS
 *****************************************************************************************
 */

/** @brief Minimum CAN baudrate (bps) */
#define CAN_MIN_BAUDRATE              (10000U)

/** @brief Maximum CAN baudrate (bps) */
#define CAN_MAX_BAUDRATE              (1000000U)

/** @brief Maximum allowed sample point (percentage * 1000) */
#define CAN_MAX_SAMPLE_POINT          (900U)

/** @brief Minimum allowed sample point (percentage * 1000) */
#define CAN_MIN_SAMPLE_POINT          (500U)

/*
 *****************************************************************************************
 * TYPE DEFINITIONS
 *****************************************************************************************
 */

/** @brief CAN Controller base address type */
typedef volatile void* Can_ControllerType;

/** @brief CAN baudrate configuration index type */
typedef uint8_t Can_BaudrateConfigIndexType;

/** @brief CAN message buffer index type */
typedef uint8_t Can_MessageBufferIndexType;

/** @brief CAN HOH (Hardware Object Handle) type */
typedef uint8_t Can_HohType;

/** @brief CAN Mailbox number type */
typedef uint8_t Can_MailboxNumberType;

/*
 *****************************************************************************************
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */

/**
 * @brief   CAN Controller State
 */
typedef enum
{
    CAN_CS_UNINIT      = 0x00U,   /**< Controller not initialized */
    CAN_CS_STOPPED     = 0x01U,   /**< Controller stopped */
    CAN_CS_STARTED     = 0x02U,   /**< Controller started */
    CAN_CS_SLEEP       = 0x03U    /**< Controller in sleep mode */
} Can_ControllerStateType;

/**
 * @brief   CAN ID Type
 */
typedef enum
{
    CAN_ID_TYPE_STANDARD = 0x00U, /**< Standard ID (11-bit) */
    CAN_ID_TYPE_EXTENDED = 0x01U  /**< Extended ID (29-bit) */
} Can_IdType;

/**
 * @brief   CAN Message Buffer Type
 */
typedef enum
{
    CAN_MB_TX       = 0x00U,      /**< Transmit message buffer */
    CAN_MB_RX       = 0x01U,      /**< Receive message buffer */
    CAN_MB_RXFIFO   = 0x02U       /**< Rx FIFO buffer */
} Can_MessageBufferType;

/**
 * @brief   CAN Tx FIFO Priority Mode
 */
typedef enum
{
    CAN_FIFO_PRIORITY_ORDER    = 0x00U, /**< Priority by ID order */
    CAN_FIFO_PRIORITY_REQUEST  = 0x01U  /**< Priority by request order */
} Can_TxFifoPriorityModeType;

/**
 * @brief   CAN Rx FIFO Filter Mechanism
 */
typedef enum
{
    CAN_FIFO_FILTER_A = 0x00U,   /**< One full ID per filter element */
    CAN_FIFO_FILTER_B = 0x01U,   /**< Two full IDs per filter element */
    CAN_FIFO_FILTER_C = 0x02U    /**< Four partial IDs per filter element */
} Can_RxFifoFilterMechanismType;

/**
 * @brief   CAN Bus-off Recovery Mode
 */
typedef enum
{
    CAN_BUSOFF_RECOVERY_AUTO    = 0x00U, /**< Automatic recovery */
    CAN_BUSOFF_RECOVERY_MANUAL  = 0x01U  /**< Manual recovery required */
} Can_BusoffRecoveryModeType;

/**
 * @brief   CAN Wakeup Source
 */
typedef enum
{
    CAN_WAKEUP_SOURCE_NONE      = 0x00U, /**< No wakeup source */
    CAN_WAKEUP_SOURCE_CAN       = 0x01U  /**< CAN wakeup enabled */
} Can_WakeupSourceType;

/**
 * @brief   CAN Return Type
 */
typedef enum
{
    CAN_OK          = 0x00U,   /**< Operation successful */
    CAN_NOT_OK      = 0x01U,   /**< Operation failed */
    CAN_BUSY        = 0x02U,   /**< Controller busy */
    CAN_PARAM_ERROR = 0x04U,   /**< Parameter error */
    CAN_UNINIT      = 0x08U    /**< Driver uninitialized */
} Can_ReturnType;

/**
 * @brief   CAN Interrupt Sources
 */
typedef enum
{
    CAN_IT_NONE           = 0x0000U,   /**< No interrupt */
    CAN_IT_TX             = 0x0001U,   /**< TX interrupt */
    CAN_IT_RX             = 0x0002U,   /**< RX interrupt */
    CAN_IT_WARN           = 0x0004U,   /**< Warning interrupt */
    CAN_IT_ERROR          = 0x0008U,   /**< Error interrupt */
    CAN_IT_BUSOFF         = 0x0010U,   /**< Bus-off interrupt */
    CAN_IT_WAKEUP         = 0x0020U,   /**< Wakeup interrupt */
    CAN_IT_RXFIFO         = 0x0040U,   /**< Rx FIFO interrupt */
    CAN_IT_RXFIFO_WARNING = 0x0080U,   /**< Rx FIFO warning */
    CAN_IT_RXFIFO_OVERRUN = 0x0100U,   /**< Rx FIFO overrun */
    CAN_IT_BIT_ERROR      = 0x0200U,   /**< Bit error */
    CAN_IT_STUFF_ERROR    = 0x0400U,   /**< Stuff error */
    CAN_IT_CRC_ERROR      = 0x0800U,   /**< CRC error */
    CAN_IT_ACK_ERROR      = 0x1000U,   /**< Acknowledge error */
    CAN_IT_FORM_ERROR     = 0x2000U,   /**< Form error */
    CAN_IT_RX_WARNING     = 0x4000U,   /**< Rx error warning */
    CAN_IT_TX_WARNING     = 0x8000U    /**< Tx error warning */
} Can_InterruptSourceType;

/**
 * @brief   CAN Error Type
 */
typedef enum
{
    CAN_ERROR_BIT       = 0x01U,   /**< Bit error */
    CAN_ERROR_STUFF     = 0x02U,   /**< Stuff error */
    CAN_ERROR_CRC       = 0x04U,   /**< CRC error */
    CAN_ERROR_ACK       = 0x08U,   /**< Acknowledge error */
    CAN_ERROR_FORM      = 0x10U,   /**< Form error */
    CAN_ERROR_TX        = 0x20U,   /**< Transmit error */
    CAN_ERROR_RX        = 0x40U,   /**< Receive error */
    CAN_ERROR_OVERLOAD  = 0x80U    /**< Overload error */
} Can_ErrorType;

/*
 *****************************************************************************************
 * STRUCTURE DEFINITIONS
 *****************************************************************************************
 */

/**
 * @brief   Tx FIFO Configuration Structure
 */
typedef struct
{
    boolean bFifoEnable;                  /**< Enable Tx FIFO */
    uint8_t u8FifoDepth;                  /**< FIFO depth */
    uint8_t u8PriorityMode;               /**< Priority mode */
} Can_TxFifoConfigType;

/**
 * @brief   Rx FIFO Configuration Structure
 */
typedef struct
{
    boolean bFifoEnable;                  /**< Enable Rx FIFO */
    uint8_t u8Watermark;                  /**< FIFO watermark */
    uint8_t u8FilterCount;                /**< Number of filters */
    uint8_t u8FilterMechanism;            /**< Filter mechanism */
} Can_RxFifoConfigType;

/**
 * @brief   CAN Error Counter Structure
 */
typedef struct
{
    uint8_t u8TxErrorCount;               /**< Transmit error counter */
    uint8_t u8RxErrorCount;               /**< Receive error counter */
} Can_ErrorCountersType;

/**
 * @brief   CAN PDU (Protocol Data Unit)
 * @details Used for transmit/receive data encapsulation
 */
typedef struct
{
    Can_IdType u8IdType;                  /**< Standard or Extended ID */
    uint32_t   u32Id;                     /**< CAN Identifier */
    uint8_t    u8Dlc;                     /**< Data length code */
    uint8_t    au8Sdu[8];                 /**< Data (max 8 bytes) */
} Can_PduType;

/**
 * @brief   CAN Hardware Object Configuration
 * @details Configuration for a hardware object (HOH)
 */
typedef struct
{
    uint8_t u8ControllerId;               /**< Associated controller */
    uint8_t u8MessageBuffer;              /**< Message buffer number */
    uint8_t u8MessageBufferType;          /**< TX, RX, or FIFO */
    uint8_t u8IdType;                     /**< Standard or Extended */
    uint32_t u32CanId;                    /**< CAN ID or mask */
    uint8_t u8Dlc;                        /**< Data length code */
} Can_HardwareObjectConfigType;

/*
 *****************************************************************************************
 * FUNCTION POINTER TYPES
 *****************************************************************************************
 */

/**
 * @brief   Tx Confirmation Callback
 * @details Called when a transmission is completed
 */
typedef void (*Can_TxConfirmationCallbackType)(void);

/**
 * @brief   Rx Indication Callback
 * @details Called when a message is received
 * @param   pduInfo  Pointer to received PDU information
 */
typedef void (*Can_RxIndicationCallbackType)(const Can_PduType* pduInfo);

/**
 * @brief   Busoff Notification Callback
 * @details Called when bus-off state is entered
 */
typedef void (*Can_BusoffNotificationCallbackType)(void);

/**
 * @brief   Error Notification Callback
 * @details Called when an error occurs
 * @param   errorType  Type of error
 */
typedef void (*Can_ErrorNotificationCallbackType)(Can_ErrorType errorType);

#endif /* CAN_CFG_H */

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

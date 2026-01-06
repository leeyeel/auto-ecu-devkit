/**
 * @file    BatCanSender.h
 * @brief   Battery Voltage CAN Sender for MPC5744
 * @details Module for sending battery voltage information via CAN.
 *          Encapsulates battery data formatting and CAN transmission.
 *
 * @note    Compliant with MISRA C:2012 coding standard
 *
 * @version 1.0.0
 * @date    2025-01-07
 *
 * @copyright Copyright (c) 2025
 */

#ifndef BAT_CANSENDER_H
#define BAT_CANSENDER_H

/*
 *****************************************************************************************
 * INCLUDES
 *****************************************************************************************
 */
#include "Std_Types.h"
#include "Can.h"

/*
 *****************************************************************************************
 * VERSION CHECK
 *****************************************************************************************
 */

/** @brief BatCanSender Module Version (Major.Minor.Patch) */
#define BAT_CANSENDER_VERSION_MAJOR    (1U)
#define BAT_CANSENDER_VERSION_MINOR    (0U)
#define BAT_CANSENDER_VERSION_PATCH    (0U)

/*
 *****************************************************************************************
 * CONSTANT DEFINITIONS
 *****************************************************************************************
 */

/**
 * @brief   Battery Voltage CAN Message IDs
 * @details CAN message identifiers for battery voltage messages
 */
#define BAT_CAN_ID_VOLTAGE_BROADCAST   (0x101U)  /**< Main voltage broadcast */
#define BAT_CAN_ID_STATUS              (0x102U)  /**< Battery status message */
#define BAT_CAN_ID_DIAGNOSTIC          (0x200U)  /**< Diagnostic response */

/**
 * @brief   Battery Voltage Data Length
 * @details DLC for voltage messages
 */
#define BAT_VOLTAGE_DLC                (8U)
#define BAT_STATUS_DLC                 (4U)

/**
 * @brief   Minimum/Maximum Battery Voltage (millivolts)
 * @details Plausibility limits for battery voltage
 */
#define BAT_VOLTAGE_MIN_MV             (9000U)   /**< 9.0V minimum */
#define BAT_VOLTAGE_MAX_MV             (16800U)  /**< 16.8V maximum */
#define BAT_VOLTAGE_NOMINAL_MV         (12000U)  /**< 12.0V nominal */

/**
 * @brief   CAN Transmission Cycle Time (milliseconds)
 * @details Interval between periodic voltage broadcasts
 */
#define BAT_TX_CYCLE_TIME_MS           (100U)    /**< 100ms broadcast interval */

/**
 * @brief   Maximum Number of Cells
 * @details Maximum number of battery cells supported
 */
#define BAT_MAX_CELLS                  (12U)

/**
 * @brief   Cell Voltage Resolution (millivolts per LSB)
 */
#define BAT_CELL_VOLTAGE_RESOLUTION    (1U)      /**< 1mV per LSB */

/**
 * @brief   Total Voltage Resolution (millivolts per LSB)
 */
#define BAT_TOTAL_VOLTAGE_RESOLUTION   (1U)      /**< 1mV per LSB */

/**
 * @brief   Current Resolution (milliamperes per LSB)
 */
#define BAT_CURRENT_RESOLUTION         (10U)     /**< 10mA per LSB */

/**
 * @brief   Temperature Resolution (degrees Celsius per LSB)
 */
#define BAT_TEMP_RESOLUTION            (1U)      /**< 1°C per LSB */

/*
 *****************************************************************************************
 * TYPE DEFINITIONS
 *****************************************************************************************
 */

/**
 * @brief   Battery Cell Voltage Structure
 * @details Stores individual cell voltages
 */
typedef struct
{
    /** @brief Number of cells in the pack */
    uint8_t u8CellCount;

    /** @brief Cell voltages in millivolts */
    uint16_t au16CellVoltage[BAT_MAX_CELLS];

} Bat_CellVoltageType;

/**
 * @brief   Battery Total Voltage and Current
 * @details Stores total voltage and current measurements
 */
typedef struct
{
    /** @brief Total pack voltage in millivolts */
    uint16_t u16TotalVoltage;

    /** @brief Pack current in milliamperes (signed) */
    int16_t  i16Current;

    /** @brief Charge/Discharge flag: TRUE = charging */
    boolean  bCharging;

} Bat_TotalMeasurementType;

/**
 * @brief   Battery Temperature Data
 * @details Stores temperature sensor readings
 */
typedef struct
{
    /** @brief Cell temperature (average) in degrees Celsius */
    int8_t i8CellTemp;

    /** @brief MOSFET temperature in degrees Celsius */
    int8_t i8MosfetTemp;

    /** @brief Minimum cell temperature */
    int8_t i8MinTemp;

    /** @brief Maximum cell temperature */
    int8_t i8MaxTemp;

} Bat_TemperatureType;

/**
 * @brief   Battery Status Flags
 * @details Stores various battery status indicators
 */
typedef struct
{
    /** @brief System is operational */
    boolean bSystemReady;

    /** @brief Battery is balanced */
    boolean bBalancingActive;

    /** @brief Over-temperature warning */
    boolean bOvertempWarning;

    /** @brief Over-voltage warning */
    boolean bOvervoltWarning;

    /** @brief Under-voltage warning */
    boolean bUndervoltWarning;

    /** @brief Over-current detected */
    boolean bOvercurrentFault;

    /** @brief Communication error */
    boolean bCommsError;

    /** @brief General fault indicator */
    boolean bFaultActive;

} Bat_StatusFlagsType;

/**
 * @brief   Complete Battery State
 * @details Aggregates all battery measurements and status
 */
typedef struct
{
    /** @brief Cell voltage measurements */
    Bat_CellVoltageType     strCellVoltage;

    /** @brief Total voltage and current */
    Bat_TotalMeasurementType strTotalMeasurement;

    /** @brief Temperature readings */
    Bat_TemperatureType     strTemperature;

    /** @brief Status flags */
    Bat_StatusFlagsType     strStatusFlags;

    /** @brief State of charge estimate (0-100%) */
    uint8_t                 u8SocPercent;

    /** @brief Remaining capacity in mAh */
    uint16_t                u16RemainingCapacity;

    /** @brief Timestamp of last update (system tick) */
    uint32_t                u32Timestamp;

} Bat_StateType;

/**
 * @brief   CAN Voltage Message Structure
 * @details Structure for voltage broadcast message payload
 */
typedef struct
{
    /** @brief Byte 0-1: Total pack voltage (little-endian) */
    uint8_t u8TotalVoltageLow;
    uint8_t u8TotalVoltageHigh;

    /** @brief Byte 2-3: Pack current (little-endian, signed) */
    uint8_t u8CurrentLow;
    uint8_t u8CurrentHigh;

    /** @brief Byte 4: Cell count */
    uint8_t u8CellCount;

    /** @brief Byte 5: State of charge */
    uint8_t u8Soc;

    /** @brief Byte 6-7: Reserved (CRC placeholder) */
    uint8_t u8Reserved0;
    uint8_t u8Reserved1;

} Bat_CanVoltageMsgType;

/**
 * @brief   CAN Status Message Structure
 * @details Structure for status message payload
 */
typedef struct
{
    /** @brief Byte 0: Status flags */
    uint8_t u8StatusFlags;

    /** @brief Byte 1: Cell temperature */
    uint8_t u8CellTemp;

    /** @brief Byte 2: MOSFET temperature */
    uint8_t u8MosfetTemp;

    /** @brief Byte 3: Fault code */
    uint8_t u8FaultCode;

} Bat_CanStatusMsgType;

/**
 * @brief   BatCanSender Initialization Result
 */
typedef enum
{
    BAT_SENDER_OK             = 0x00U,   /**< Initialization successful */
    BAT_SENDER_ERR_NULL       = 0x01U,   /**< NULL pointer parameter */
    BAT_SENDER_ERR_INIT       = 0x02U,   /**< Initialization failed */
    BAT_SENDER_ERR_BUS        = 0x03U,   /**< CAN bus error */
    BAT_SENDER_ERR_TIMEOUT    = 0x04U,   /**< Timeout error */
    BAT_SENDER_ERR_INVALID    = 0x05U,   /**< Invalid data */
    BAT_SENDER_ERR_OVERFLOW   = 0x06U    /**< Data overflow */
} Bat_SenderReturnType;

/*
 *****************************************************************************************
 * GLOBAL FUNCTION DECLARATIONS
 *****************************************************************************************
 */

/**
 * @brief   Initialize Battery CAN Sender
 * @details Initializes the battery voltage sender module.
 *          Configures CAN callbacks and prepares for transmission.
 * @return  BAT_SENDER_OK     - Initialization successful
 * @return  BAT_SENDER_ERR_INIT - Initialization failed
 */
Bat_SenderReturnType BatCanSender_Init(void);

/**
 * @brief   De-initialize Battery CAN Sender
 * @details Shuts down the battery voltage sender module.
 * @return  BAT_SENDER_OK - Always returns success
 */
Bat_SenderReturnType BatCanSender_DeInit(void);

/**
 * @brief   Update Battery State
 * @details Updates the internal battery state with new measurements.
 * @param   pState     Pointer to new battery state
 * @return  BAT_SENDER_OK         - Update successful
 * @return  BAT_SENDER_ERR_NULL   - NULL pointer parameter
 * @return  BAT_SENDER_ERR_INVALID - Invalid data detected
 */
Bat_SenderReturnType BatCanSender_UpdateState(const Bat_StateType* pState);

/**
 * @brief   Send Voltage Broadcast
 * @details Sends the battery voltage information via CAN.
 *          Uses non-blocking transmission.
 * @return  BAT_SENDER_OK     - Transmission initiated
 * @return  BAT_SENDER_ERR_BUS - CAN write failed
 */
Bat_SenderReturnType BatCanSender_SendVoltageBroadcast(void);

/**
 * @brief   Send Status Message
 * @details Sends the battery status information via CAN.
 * @return  BAT_SENDER_OK     - Transmission initiated
 * @return  BAT_SENDER_ERR_BUS - CAN write failed
 */
Bat_SenderReturnType BatCanSender_SendStatusMessage(void);

/**
 * @brief   Periodic Task
 * @details Call this function periodically to send voltage broadcasts.
 *          Implements the transmission cycle timing.
 * @return  BAT_SENDER_OK     - Task completed
 * @return  BAT_SENDER_ERR_BUS - CAN transmission error
 */
Bat_SenderReturnType BatCanSender_Task(void);

/**
 * @brief   Get Battery State
 * @details Returns the current battery state.
 * @param   pState     Pointer to store battery state
 * @return  BAT_SENDER_OK         - State retrieved
 * @return  BAT_SENDER_ERR_NULL   - NULL pointer parameter
 */
Bat_SenderReturnType BatCanSender_GetState(Bat_StateType* pState);

/**
 * @brief   Validate Battery Data
 * @details Checks if battery data is within plausible limits.
 * @param   pState     Pointer to battery state to validate
 * @return  TRUE  - Data is valid
 * @return  FALSE - Data is invalid
 */
boolean BatCanSender_ValidateData(const Bat_StateType* pState);

/**
 * @brief   Get Software Version
 * @details Returns the version of the module.
 * @return  Version in format (Major << 8) | Minor
 */
uint16_t BatCanSender_GetVersion(void);

#endif /* BAT_CANSENDER_H */

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

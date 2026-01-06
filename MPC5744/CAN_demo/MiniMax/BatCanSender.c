/**
 * @file    BatCanSender.c
 * @brief   Battery Voltage CAN Sender Implementation for MPC5744
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

/*
 *****************************************************************************************
 * INCLUDES
 *****************************************************************************************
 */
#include "BatCanSender.h"
#include "Can.h"
#include "Can_PBcfg.h"

/*
 *****************************************************************************************
 * FILE VERSION CHECK
 *****************************************************************************************
 */

#if (BAT_CANSENDER_VERSION_MAJOR != 1U) || (BAT_CANSENDER_VERSION_MINOR != 0U)
#error "Version mismatch in BatCanSender"
#endif

/*
 *****************************************************************************************
 * LOCAL CONSTANTS
 *****************************************************************************************
 */

/**
 * @brief   Module Name for Debug
 */
#define BAT_SENDER_MODULE_NAME         "BatCanSender"

/**
 * @brief   CRC-8 Polynomial
 * @details Used for data integrity check
 */
#define BAT_CRC8_POLYNOMIAL            (0x07U)

/**
 * @brief   CRC-8 Init Value
 */
#define BAT_CRC8_INIT                  (0xFFU)

/*
 *****************************************************************************************
 * LOCAL VARIABLES
 *****************************************************************************************
 */

/** @brief Current battery state */
static Bat_StateType Bat_strCurrentState;

/** @brief Previous battery state (for change detection) */
static Bat_StateType Bat_strPreviousState;

/** @brief Module initialization status */
static boolean Bat_bInitialized = FALSE;

/** @brief Last transmission time (system tick) */
static uint32_t Bat_u32LastTxTime = 0U;

/** @brief Transmission cycle counter */
static uint16_t Bat_u16TxCycleCount = 0U;

/** @brief Send queue counter */
static uint8_t Bat_u8SendQueueCount = 0U;

/**
 * @brief   CAN PDU for Voltage Transmission
 * @details Reused for each transmission to avoid stack allocation
 */
static Can_PduType Bat_strVoltagePdu;

/**
 * @brief   CAN PDU for Status Transmission
 * @details Reused for each status transmission
 */
static Can_PduType Bat_strStatusPdu;

/**
 * @brief   Message Buffer Hardware Object Handle
 * @details Maps to the transmit message buffer configured in Can_PBcfg
 */
static uint8_t Bat_u8VoltageHoh = 0U;    /* MB 0: Voltage broadcast */
static uint8_t Bat_u8StatusHoh = 1U;     /* MB 1: Status message */

/*
 *****************************************************************************************
 * STATIC FUNCTION DECLARATIONS
 *****************************************************************************************
 */

/**
 * @brief   Calculate CRC-8
 * @details Calculates CRC-8 checksum for data integrity.
 * @param   pData       Pointer to data buffer
 * @param   u8Length    Data length in bytes
 * @return  CRC-8 checksum value
 */
static uint8_t Bat_CalculateCrc8(const uint8_t* pData, uint8_t u8Length);

/**
 * @brief   Format Voltage Message
 * @details Formats battery voltage data into CAN message payload.
 * @param   pState      Pointer to battery state
 * @param   pMsg        Pointer to formatted message
 */
static void Bat_FormatVoltageMessage(const Bat_StateType* pState,
                                      Bat_CanVoltageMsgType* pMsg);

/**
 * @brief   Format Status Message
 * @details Formats battery status into CAN message payload.
 * @param   pState      Pointer to battery state
 * @param   pMsg        Pointer to formatted message
 */
static void Bat_FormatStatusMessage(const Bat_StateType* pState,
                                     Bat_CanStatusMsgType* pMsg);

/**
 * @brief   Validate Voltage Range
 * @details Checks if a voltage measurement is within valid range.
 * @param   u16Voltage  Voltage in millivolts
 * @return  TRUE  - Voltage is valid
 * @return  FALSE - Voltage is out of range
 */
static boolean Bat_ValidateVoltage(uint16_t u16Voltage);

/**
 * @brief   Validate Current Range
 * @details Checks if a current measurement is within valid range.
 * @param   i16Current  Current in milliamperes (signed)
 * @return  TRUE  - Current is valid
 * @return  FALSE - Current is out of range
 */
static boolean Bat_ValidateCurrent(int16_t i16Current);

/**
 * @brief   Validate Temperature
 * @details Checks if a temperature measurement is within valid range.
 * @param   i8Temp      Temperature in degrees Celsius
 * @return  TRUE  - Temperature is valid
 * @return  FALSE - Temperature is out of range
 */
static boolean Bat_ValidateTemperature(int8_t i8Temp);

/**
 * @brief   Get System Tick
 * @details Returns current system tick count (milliseconds).
 *          Implementation-dependent, may use SysTick or hardware timer.
 * @return  Current tick count in milliseconds
 */
static uint32_t Bat_GetSystemTick(void);

/**
 * @brief   Calculate SOC from Voltage
 * @details Estimates state of charge based on voltage.
 *          Simple linear model for demonstration.
 * @param   u16TotalVoltage  Total pack voltage
 * @return  Estimated SOC percentage (0-100)
 */
static uint8_t Bat_EstimateSoc(uint16_t u16TotalVoltage);

/*
 *****************************************************************************************
 * STATIC FUNCTION DEFINITIONS
 *****************************************************************************************
 */

static uint8_t Bat_CalculateCrc8(const uint8_t* pData, uint8_t u8Length)
{
    uint8_t u8Crc = BAT_CRC8_INIT;
    uint8_t u8ByteIdx;
    uint8_t u8BitIdx;
    uint8_t u8DataByte;

    /* Validate input */
    if (pData == NULL_PTR)
    {
        return BAT_CRC8_INIT;
    }

    /* Calculate CRC-8 */
    for (u8ByteIdx = 0U; u8ByteIdx < u8Length; u8ByteIdx++)
    {
        u8DataByte = pData[u8ByteIdx];
        u8Crc ^= u8DataByte;

        for (u8BitIdx = 0U; u8BitIdx < 8U; u8BitIdx++)
        {
            if ((u8Crc & 0x80U) != 0U)
            {
                u8Crc = (uint8_t)((u8Crc << 1U) ^ BAT_CRC8_POLYNOMIAL);
            }
            else
            {
                u8Crc <<= 1U;
            }
        }
    }

    return u8Crc;
}

static void Bat_FormatVoltageMessage(const Bat_StateType* pState,
                                      Bat_CanVoltageMsgType* pMsg)
{
    uint16_t u16Voltage;
    int16_t i16Current;

    /* Validate inputs */
    if ((pState == NULL_PTR) || (pMsg == NULL_PTR))
    {
        return;
    }

    /* Extract total voltage (little-endian) */
    u16Voltage = pState->strTotalMeasurement.u16TotalVoltage;
    pMsg->u8TotalVoltageLow = (uint8_t)(u16Voltage & 0xFFU);
    pMsg->u8TotalVoltageHigh = (uint8_t)((u16Voltage >> 8U) & 0xFFU);

    /* Extract current (little-endian, signed) */
    i16Current = pState->strTotalMeasurement.i16Current;
    pMsg->u8CurrentLow = (uint8_t)(i16Current & 0xFFU);
    pMsg->u8CurrentHigh = (uint8_t)((i16Current >> 8U) & 0xFFU);

    /* Cell count */
    pMsg->u8CellCount = pState->strCellVoltage.u8CellCount;

    /* State of charge */
    pMsg->u8Soc = pState->u8SocPercent;

    /* Reserved bytes (CRC placeholder) */
    pMsg->u8Reserved0 = 0U;
    pMsg->u8Reserved1 = 0U;
}

static void Bat_FormatStatusMessage(const Bat_StateType* pState,
                                     Bat_CanStatusMsgType* pMsg)
{
    uint8_t u8StatusFlags = 0U;

    /* Validate inputs */
    if ((pState == NULL_PTR) || (pMsg == NULL_PTR))
    {
        return;
    }

    /* Format status flags byte */
    if (pState->strStatusFlags.bSystemReady == TRUE)
    {
        u8StatusFlags |= 0x01U;
    }
    if (pState->strStatusFlags.bBalancingActive == TRUE)
    {
        u8StatusFlags |= 0x02U;
    }
    if (pState->strStatusFlags.bOvertempWarning == TRUE)
    {
        u8StatusFlags |= 0x04U;
    }
    if (pState->strStatusFlags.bOvervoltWarning == TRUE)
    {
        u8StatusFlags |= 0x08U;
    }
    if (pState->strStatusFlags.bUndervoltWarning == TRUE)
    {
        u8StatusFlags |= 0x10U;
    }
    if (pState->strStatusFlags.bOvercurrentFault == TRUE)
    {
        u8StatusFlags |= 0x20U;
    }
    if (pState->strStatusFlags.bCommsError == TRUE)
    {
        u8StatusFlags |= 0x40U;
    }
    if (pState->strStatusFlags.bFaultActive == TRUE)
    {
        u8StatusFlags |= 0x80U;
    }

    pMsg->u8StatusFlags = u8StatusFlags;

    /* Temperature values */
    pMsg->u8CellTemp = (uint8_t)pState->strTemperature.i8CellTemp;
    pMsg->u8MosfetTemp = (uint8_t)pState->strTemperature.i8MosfetTemp;

    /* Fault code (0 = no fault) */
    pMsg->u8FaultCode = 0U;
    if (pState->strStatusFlags.bFaultActive == TRUE)
    {
        if (pState->strStatusFlags.bOvertempWarning == TRUE)
        {
            pMsg->u8FaultCode = 0x01U;
        }
        else if (pState->strStatusFlags.bOvervoltWarning == TRUE)
        {
            pMsg->u8FaultCode = 0x02U;
        }
        else if (pState->strStatusFlags.bUndervoltWarning == TRUE)
        {
            pMsg->u8FaultCode = 0x03U;
        }
        else if (pState->strStatusFlags.bOvercurrentFault == TRUE)
        {
            pMsg->u8FaultCode = 0x04U;
        }
        else if (pState->strStatusFlags.bCommsError == TRUE)
        {
            pMsg->u8FaultCode = 0x05U;
        }
        else
        {
            pMsg->u8FaultCode = 0xFFU;  /* Unknown fault */
        }
    }
}

static boolean Bat_ValidateVoltage(uint16_t u16Voltage)
{
    /* Check voltage is within plausible range */
    if ((u16Voltage < BAT_VOLTAGE_MIN_MV) || (u16Voltage > BAT_VOLTAGE_MAX_MV))
    {
        return FALSE;
    }
    return TRUE;
}

static boolean Bat_ValidateCurrent(int16_t i16Current)
{
    /* Check current is within plausible range (-100A to +100A) */
    if ((i16Current < -10000) || (i16Current > 10000))
    {
        return FALSE;
    }
    return TRUE;
}

static boolean Bat_ValidateTemperature(int8_t i8Temp)
{
    /* Check temperature is within plausible range (-40°C to +85°C) */
    if ((i8Temp < -40) || (i8Temp > 85))
    {
        return FALSE;
    }
    return TRUE;
}

static uint32_t Bat_GetSystemTick(void)
{
    uint32_t u32Tick = 0U;

    /* TODO: Implement actual system tick retrieval */
    /* This is a placeholder - in production, this would read */
    /* from a hardware timer or RTOS tick counter */
    /* Example using SysTick: */
    /* u32Tick = SysTick->VAL; */

    return u32Tick;
}

static uint8_t Bat_EstimateSoc(uint16_t u16TotalVoltage)
{
    uint16_t u16Soc;
    uint16_t u16VoltageRange;
    uint16_t u16VoltagePercent;

    /* Simple linear SOC estimation based on voltage */
    /* Full charge: 16.8V, Empty: 12.0V */
    u16VoltageRange = BAT_VOLTAGE_MAX_MV - BAT_VOLTAGE_MIN_MV;

    if (u16TotalVoltage >= BAT_VOLTAGE_MAX_MV)
    {
        u16Soc = 100U;
    }
    else if (u16TotalVoltage <= BAT_VOLTAGE_MIN_MV)
    {
        u16Soc = 0U;
    }
    else
    {
        u16VoltagePercent = ((u16TotalVoltage - BAT_VOLTAGE_MIN_MV) * 100U) / u16VoltageRange;
        u16Soc = u16VoltagePercent;
    }

    return (uint8_t)u16Soc;
}

/*
 *****************************************************************************************
 * GLOBAL FUNCTION DEFINITIONS
 *****************************************************************************************
 */

Bat_SenderReturnType BatCanSender_Init(void)
{
    Bat_SenderReturnType eRetVal = BAT_SENDER_OK;

    /* Check if already initialized */
    if (Bat_bInitialized == TRUE)
    {
        eRetVal = BAT_SENDER_ERR_INIT;
    }
    else
    {
        /* Initialize CAN PDU for voltage broadcast */
        Bat_strVoltagePdu.u8IdType = CAN_ID_TYPE_STANDARD;
        Bat_strVoltagePdu.u32Id = BAT_CAN_ID_VOLTAGE_BROADCAST;
        Bat_strVoltagePdu.u8Dlc = BAT_VOLTAGE_DLC;

        /* Initialize CAN PDU for status message */
        Bat_strStatusPdu.u8IdType = CAN_ID_TYPE_STANDARD;
        Bat_strStatusPdu.u32Id = BAT_CAN_ID_STATUS;
        Bat_strStatusPdu.u8Dlc = BAT_STATUS_DLC;

        /* Initialize current state to zero */
        Bat_strCurrentState.strCellVoltage.u8CellCount = 0U;

        /* Initialize timestamp */
        Bat_u32LastTxTime = Bat_GetSystemTick();

        /* Mark as initialized */
        Bat_bInitialized = TRUE;
    }

    return eRetVal;
}

Bat_SenderReturnType BatCanSender_DeInit(void)
{
    /* Reset initialization status */
    Bat_bInitialized = FALSE;

    /* Reset state */
    (void)memset(&Bat_strCurrentState, 0U, sizeof(Bat_strCurrentState));
    (void)memset(&Bat_strPreviousState, 0U, sizeof(Bat_strPreviousState));

    /* Reset counters */
    Bat_u16TxCycleCount = 0U;
    Bat_u8SendQueueCount = 0U;

    return BAT_SENDER_OK;
}

Bat_SenderReturnType BatCanSender_UpdateState(const Bat_StateType* pState)
{
    Bat_SenderReturnType eRetVal = BAT_SENDER_OK;

    /* Validate input */
    if (pState == NULL_PTR)
    {
        eRetVal = BAT_SENDER_ERR_NULL;
    }
    else if (Bat_bInitialized == FALSE)
    {
        eRetVal = BAT_SENDER_ERR_INIT;
    }
    else
    {
        /* Validate data plausibility */
        if (BatCanSender_ValidateData(pState) == FALSE)
        {
            eRetVal = BAT_SENDER_ERR_INVALID;
        }
        else
        {
            /* Store previous state */
            Bat_strPreviousState = Bat_strCurrentState;

            /* Update current state */
            Bat_strCurrentState = *pState;

            /* Update timestamp */
            Bat_strCurrentState.u32Timestamp = Bat_GetSystemTick();

            /* Update SOC if not provided */
            if (Bat_strCurrentState.u8SocPercent == 0U)
            {
                Bat_strCurrentState.u8SocPercent =
                    Bat_EstimateSoc(Bat_strCurrentState.strTotalMeasurement.u16TotalVoltage);
            }

            eRetVal = BAT_SENDER_OK;
        }
    }

    return eRetVal;
}

Bat_SenderReturnType BatCanSender_SendVoltageBroadcast(void)
{
    Bat_SenderReturnType eRetVal = BAT_SENDER_OK;
    Bat_CanVoltageMsgType strVoltageMsg;
    Can_ReturnType eCanRet;

    /* Check initialization */
    if (Bat_bInitialized == FALSE)
    {
        return BAT_SENDER_ERR_INIT;
    }

    /* Format voltage message */
    Bat_FormatVoltageMessage(&Bat_strCurrentState, &strVoltageMsg);

    /* Copy data to PDU */
    Bat_strVoltagePdu.au8Sdu[0] = strVoltageMsg.u8TotalVoltageLow;
    Bat_strVoltagePdu.au8Sdu[1] = strVoltageMsg.u8TotalVoltageHigh;
    Bat_strVoltagePdu.au8Sdu[2] = strVoltageMsg.u8CurrentLow;
    Bat_strVoltagePdu.au8Sdu[3] = strVoltageMsg.u8CurrentHigh;
    Bat_strVoltagePdu.au8Sdu[4] = strVoltageMsg.u8CellCount;
    Bat_strVoltagePdu.au8Sdu[5] = strVoltageMsg.u8Soc;
    Bat_strVoltagePdu.au8Sdu[6] = strVoltageMsg.u8Reserved0;
    Bat_strVoltagePdu.au8Sdu[7] = strVoltageMsg.u8Reserved1;

    /* Calculate and append CRC */
    Bat_strVoltagePdu.au8Sdu[6] = Bat_CalculateCrc8(Bat_strVoltagePdu.au8Sdu, 6U);

    /* Send CAN message */
    eCanRet = Can_Write(Bat_u8VoltageHoh, &Bat_strVoltagePdu);

    if (eCanRet == CAN_OK)
    {
        eRetVal = BAT_SENDER_OK;
    }
    else if (eCanRet == CAN_BUSY)
    {
        eRetVal = BAT_SENDER_ERR_BUS;
    }
    else
    {
        eRetVal = BAT_SENDER_ERR_BUS;
    }

    return eRetVal;
}

Bat_SenderReturnType BatCanSender_SendStatusMessage(void)
{
    Bat_SenderReturnType eRetVal = BAT_SENDER_OK;
    Bat_CanStatusMsgType strStatusMsg;
    Can_ReturnType eCanRet;

    /* Check initialization */
    if (Bat_bInitialized == FALSE)
    {
        return BAT_SENDER_ERR_INIT;
    }

    /* Format status message */
    Bat_FormatStatusMessage(&Bat_strCurrentState, &strStatusMsg);

    /* Copy data to PDU */
    Bat_strStatusPdu.au8Sdu[0] = strStatusMsg.u8StatusFlags;
    Bat_strStatusPdu.au8Sdu[1] = strStatusMsg.u8CellTemp;
    Bat_strStatusPdu.au8Sdu[2] = strStatusMsg.u8MosfetTemp;
    Bat_strStatusPdu.au8Sdu[3] = strStatusMsg.u8FaultCode;

    /* Send CAN message */
    eCanRet = Can_Write(Bat_u8StatusHoh, &Bat_strStatusPdu);

    if (eCanRet == CAN_OK)
    {
        eRetVal = BAT_SENDER_OK;
    }
    else if (eCanRet == CAN_BUSY)
    {
        eRetVal = BAT_SENDER_ERR_BUS;
    }
    else
    {
        eRetVal = BAT_SENDER_ERR_BUS;
    }

    return eRetVal;
}

Bat_SenderReturnType BatCanSender_Task(void)
{
    Bat_SenderReturnType eRetVal = BAT_SENDER_OK;
    uint32_t u32CurrentTime;
    uint32_t u32ElapsedTime;

    /* Check initialization */
    if (Bat_bInitialized == FALSE)
    {
        return BAT_SENDER_ERR_INIT;
    }

    /* Get current time */
    u32CurrentTime = Bat_GetSystemTick();

    /* Calculate elapsed time */
    if (u32CurrentTime >= Bat_u32LastTxTime)
    {
        u32ElapsedTime = u32CurrentTime - Bat_u32LastTxTime;
    }
    else
    {
        /* Handle timer wrap-around */
        u32ElapsedTime = (0xFFFFFFFFU - Bat_u32LastTxTime) + u32CurrentTime;
    }

    /* Check if it's time to send */
    if (u32ElapsedTime >= BAT_TX_CYCLE_TIME_MS)
    {
        /* Send voltage broadcast */
        eRetVal = BatCanSender_SendVoltageBroadcast();

        if (eRetVal == BAT_SENDER_OK)
        {
            /* Update transmission time */
            Bat_u32LastTxTime = u32CurrentTime;

            /* Increment cycle counter */
            Bat_u16TxCycleCount++;

            /* Send status message every 10 cycles (1 second) */
            if ((Bat_u16TxCycleCount % 10U) == 0U)
            {
                (void)BatCanSender_SendStatusMessage();
            }
        }
    }

    return eRetVal;
}

Bat_SenderReturnType BatCanSender_GetState(Bat_StateType* pState)
{
    Bat_SenderReturnType eRetVal = BAT_SENDER_OK;

    /* Validate input */
    if (pState == NULL_PTR)
    {
        eRetVal = BAT_SENDER_ERR_NULL;
    }
    else if (Bat_bInitialized == FALSE)
    {
        eRetVal = BAT_SENDER_ERR_INIT;
    }
    else
    {
        /* Return current state */
        *pState = Bat_strCurrentState;
        eRetVal = BAT_SENDER_OK;
    }

    return eRetVal;
}

boolean BatCanSender_ValidateData(const Bat_StateType* pState)
{
    uint8_t u8CellIdx;

    /* Validate input */
    if (pState == NULL_PTR)
    {
        return FALSE;
    }

    /* Validate cell count */
    if (pState->strCellVoltage.u8CellCount > BAT_MAX_CELLS)
    {
        return FALSE;
    }

    /* Validate total voltage */
    if (Bat_ValidateVoltage(pState->strTotalMeasurement.u16TotalVoltage) == FALSE)
    {
        return FALSE;
    }

    /* Validate current */
    if (Bat_ValidateCurrent(pState->strTotalMeasurement.i16Current) == FALSE)
    {
        return FALSE;
    }

    /* Validate cell voltages */
    for (u8CellIdx = 0U; u8CellIdx < pState->strCellVoltage.u8CellCount; u8CellIdx++)
    {
        /* Individual cell voltage should be between 2.5V and 4.35V */
        if ((pState->strCellVoltage.au16CellVoltage[u8CellIdx] < 2500U) ||
            (pState->strCellVoltage.au16CellVoltage[u8CellIdx] > 4350U))
        {
            return FALSE;
        }
    }

    /* Validate temperatures */
    if (Bat_ValidateTemperature(pState->strTemperature.i8CellTemp) == FALSE)
    {
        return FALSE;
    }
    if (Bat_ValidateTemperature(pState->strTemperature.i8MosfetTemp) == FALSE)
    {
        return FALSE;
    }

    /* Validate SOC */
    if (pState->u8SocPercent > 100U)
    {
        return FALSE;
    }

    return TRUE;
}

uint16_t BatCanSender_GetVersion(void)
{
    return (uint16_t)((BAT_CANSENDER_VERSION_MAJOR << 8U) | BAT_CANSENDER_VERSION_MINOR);
}

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

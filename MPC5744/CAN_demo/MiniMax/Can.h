/**
 * @file    Can.h
 * @brief   CAN Driver Interface Header for MPC5744
 * @details AUTOSAR-like CAN driver interface for FlexCAN on MPC5744.
 *          Provides CAN initialization, transmission, reception, and
 *          interrupt handling capabilities.
 *
 * @note    Compliant with MISRA C:2012 coding standard
 *
 * @version 1.0.0
 * @date    2025-01-07
 *
 * @copyright Copyright (c) 2025
 */

#ifndef CAN_H
#define CAN_H

/*
 *****************************************************************************************
 * INCLUDES
 *****************************************************************************************
 */
#include "Can_Cfg.h"
#include "Std_Types.h"

/*
 *****************************************************************************************
 * VERSION CHECK
 *****************************************************************************************
 */

/** @brief CAN Driver Module Version (Major.Minor.Patch) */
#define CAN_VERSION_MAJOR      (1U)
#define CAN_VERSION_MINOR      (0U)
#define CAN_VERSION_PATCH      (0U)

/**
 * @brief   Get CAN Driver Version
 * @details Returns the version of the CAN driver
 */
#define CAN_GET_VERSION \
    ( (uint16_t)((CAN_VERSION_MAJOR << 8U) | CAN_VERSION_MINOR) )

/*
 *****************************************************************************************
 * API SERVICES
 *****************************************************************************************
 */

/**
 * @brief   CAN Driver Initialization
 * @details Initializes all configured CAN controllers with the settings
 *          from Can_PBcfg.c.
 * @return  CAN_OK     - Initialization successful
 * @return  CAN_NOT_OK - Initialization failed
 */
Can_ReturnType Can_Init(void);

/**
 * @brief   CAN Driver De-Initialization
 * @details Shuts down all CAN controllers, disables interrupts,
 *          and clears any pending flags.
 * @return  CAN_OK     - De-initialization successful
 * @return  CAN_NOT_OK - De-initialization failed
 */
Can_ReturnType Can_DeInit(void);

/**
 * @brief   Set CAN Controller Mode
 * @details Changes the operational mode of a CAN controller.
 *          Valid modes are STOPPED, STARTED, and SLEEP.
 * @param   u8Controller  Controller ID (0 or 1)
 * @param   eMode         Target controller mode
 * @return  CAN_OK     - Mode change successful
 * @return  CAN_NOT_OK - Mode change failed or invalid parameter
 */
Can_ReturnType Can_SetControllerMode(uint8_t u8Controller,
                                     Can_ControllerStateType eMode);

/**
 * @brief   Write CAN Message
 * @details Schedules a message for transmission on the specified
 *          hardware object.
 * @param   u8Hoh         Hardware object handle
 * @param   pPduInfo      Pointer to PDU information (ID, DLC, Data)
 * @return  CAN_OK     - Write request accepted
 * @return  CAN_NOT_OK - Write request rejected
 * @return  CAN_BUSY   - No free transmit buffer available
 */
Can_ReturnType Can_Write(uint8_t u8Hoh, const Can_PduType* pPduInfo);

/**
 * @brief   Check CAN Controller Status
 * @details Retrieves the current state of a CAN controller.
 * @param   u8Controller  Controller ID
 * @param   pControllerState  Pointer to store controller state
 * @return  CAN_OK     - Status retrieved successfully
 * @return  CAN_NOT_OK - Invalid controller or parameter
 */
Can_ReturnType Can_GetControllerStatus(uint8_t u8Controller,
                                        Can_ControllerStateType* pControllerState);

/**
 * @brief   Get CAN Error Counters
 * @details Retrieves the transmit and receive error counters.
 * @param   u8Controller      Controller ID
 * @param   pErrorCounters    Pointer to store error counters
 * @return  CAN_OK     - Error counters retrieved
 * @return  CAN_NOT_OK - Invalid controller or parameter
 */
Can_ReturnType Can_GetErrorCounters(uint8_t u8Controller,
                                     Can_ErrorCountersType* pErrorCounters);

/**
 * @brief   Check for Wakeup
 * @details Checks if a wakeup event occurred on the specified controller.
 * @param   u8Controller  Controller ID
 * @return  TRUE  - Wakeup detected
 * @return  FALSE - No wakeup detected
 */
boolean Can_CheckWakeup(uint8_t u8Controller);

/**
 * @brief   Enable CAN Controller Interrupts
 * @details Enables specific interrupt sources for a controller.
 * @param   u8Controller      Controller ID
 * @param   u16InterruptMask  Bitmask of interrupt sources to enable
 * @return  CAN_OK     - Interrupts enabled
 * @return  CAN_NOT_OK - Operation failed
 */
Can_ReturnType Can_EnableInterrupt(uint8_t u8Controller,
                                    uint16_t u16InterruptMask);

/**
 * @brief   Disable CAN Controller Interrupts
 * @details Disables specific interrupt sources for a controller.
 * @param   u8Controller      Controller ID
 * @param   u16InterruptMask  Bitmask of interrupt sources to disable
 * @return  CAN_OK     - Interrupts disabled
 * @return  CAN_NOT_OK - Operation failed
 */
Can_ReturnType Can_DisableInterrupt(uint8_t u8Controller,
                                     uint16_t u16InterruptMask);

/**
 * @brief   Get CAN Interrupt Status
 * @details Retrieves the current interrupt status of a controller.
 * @param   u8Controller   Controller ID
 * @param   pu16Status     Pointer to store interrupt status
 * @return  CAN_OK     - Status retrieved
 * @return  CAN_NOT_OK - Invalid controller or parameter
 */
Can_ReturnType Can_GetInterruptStatus(uint8_t u8Controller,
                                       uint16_t* pu16Status);

/**
 * @brief   Clear CAN Interrupt Flags
 * @details Clears specified interrupt flags for a controller.
 * @param   u8Controller     Controller ID
 * @param   u16FlagMask      Bitmask of flags to clear
 * @return  CAN_OK     - Flags cleared
 * @return  CAN_NOT_OK - Operation failed
 */
Can_ReturnType Can_ClearInterruptFlags(uint8_t u8Controller,
                                        uint16_t u16FlagMask);

/*
 *****************************************************************************************
 * CALLBACK REGISTRATION
 *****************************************************************************************
 */

/**
 * @brief   Register Tx Confirmation Callback
 * @details Registers a callback function for transmit confirmation events.
 * @param   pCallback  Pointer to callback function (NULL to disable)
 */
void Can_RegisterTxConfirmationCallback(Can_TxConfirmationCallbackType pCallback);

/**
 * @brief   Register Rx Indication Callback
 * @details Registers a callback function for receive indication events.
 * @param   pCallback  Pointer to callback function (NULL to disable)
 */
void Can_RegisterRxIndicationCallback(Can_RxIndicationCallbackType pCallback);

/**
 * @brief   Register Busoff Notification Callback
 * @details Registers a callback function for bus-off events.
 * @param   pCallback  Pointer to callback function (NULL to disable)
 */
void Can_RegisterBusoffNotificationCallback(Can_BusoffNotificationCallbackType pCallback);

/**
 * @brief   Register Error Notification Callback
 * @details Registers a callback function for error events.
 * @param   pCallback  Pointer to callback function (NULL to disable)
 */
void Can_RegisterErrorNotificationCallback(Can_ErrorNotificationCallbackType pCallback);

/*
 *****************************************************************************************
 * INTERRUPT SERVICE ROUTINES
 *****************************************************************************************
 */

/**
 * @brief   CAN0 Interrupt Service Routine
 * @details Handles all CAN0 interrupt sources.
 *          This function must be called from the CAN0 interrupt handler.
 */
void Can_IsrHandler_Controller0(void);

/**
 * @brief   CAN1 Interrupt Service Routine
 * @details Handles all CAN1 interrupt sources.
 *          This function must be called from the CAN1 interrupt handler.
 */
void Can_IsrHandler_Controller1(void);

#endif /* CAN_H */

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

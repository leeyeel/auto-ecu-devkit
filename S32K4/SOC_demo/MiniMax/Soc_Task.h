/**
 * @file Soc_Task.h
 * @brief FreeRTOS Task for 5ms SOC estimation
 * @module Soc_Task
 *
 * @assumptions & constraints
 *   - FreeRTOS kernel available
 *   - Timer driver configured for 5ms interrupts
 *   - Current measurement available via ADC or communication
 *
 * @safety considerations
 *   - Task runs at 5ms period for real-time SOC estimation
 *   - Task priority must be appropriate for system requirements
 *   - Watchdog should be fed in task loop
 *
 * @execution context
 *   - Task context (not ISR)
 *   - Blocked on timer/semaphore notification
 *
 * @version 1.0.0
 * @compliance MISRA C:2012, ISO 26262
 */

#ifndef SOC_TASK_H
#define SOC_TASK_H

#include "Soc_Types.h"
#include <stdbool.h>

/*============================================================================*/
/* FreeRTOS Includes                                                         */
/*============================================================================*/

/**
 * @note In actual implementation, include FreeRTOS headers:
 * #include "FreeRTOS.h"
 * #include "task.h"
 * #include "semphr.h"
 */

/*============================================================================*/
/* Task Configuration                                                        */
/*============================================================================*/

/**
 * @brief SOC task notification bit for timer event
 */
#define SOC_TASK_NOTIFY_TIMER_BIT   (0x01U)

/**
 * @brief Task handle (external reference)
 */
extern void* g_SocTaskHandle;

/**
 * @brief Task notification semaphore (external reference)
 */
extern void* g_SocTaskNotification;

/*============================================================================*/
/* API Functions                                                             */
/*============================================================================*/

/**
 * @brief Create and start SOC estimation task
 *
 * @description Creates FreeRTOS task with configured priority and stack.
 *              Task will wait for timer notification and run SOC update.
 *
 * @return Soc_StatusType
 *         - SOC_STATUS_OK: Task created successfully
 *         - SOC_STATUS_ERROR: Task creation failed
 *
 * @pre  Soc_Timer_S32K4_Init must be called
 * @pre  Soc_Algorithm_Init must be called
 * @post Task created and ready to run
 *
 * @safety Task creation failure leaves system without SOC estimation
 */
Soc_StatusType Soc_Task_Create(void);

/**
 * @brief Delete SOC estimation task
 *
 * @description Deletes task and cleans up resources.
 *
 * @return Soc_StatusType
 *         - SOC_STATUS_OK: Task deleted
 *
 * @pre  Task must have been created
 * @post Task deleted, resources freed
 */
Soc_StatusType Soc_Task_Delete(void);

/**
 * @brief Suspend SOC task
 *
 * @description Temporarily suspends task execution.
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Task_Suspend(void);

/**
 * @brief Resume SOC task
 *
 * @description Resumes previously suspended task.
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Task_Resume(void);

/**
 * @brief Notify task from timer ISR
 *
 * @description Called from timer ISR to wake up task.
 *
 * @note Uses FromISR variant of notification
 *
 * @return true if notification sent successfully
 */
bool Soc_Task_NotifyFromIsr(void);

/**
 * @brief Get task runtime statistics
 *
 * @param[out] runTime Pointer to store runtime in microseconds
 * @param[out] cycleCount Pointer to store cycle count
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Task_GetStats(uint32_t* const runTime, uint32_t* const cycleCount);

/**
 * @brief Check if task is running
 *
 * @return true if task is running
 */
bool Soc_Task_IsRunning(void);

/**
 * @brief Get task notification value
 *
 * @return Current notification value
 */
uint32_t Soc_Task_GetNotificationValue(void);

/**
 * @brief Clear task notification
 *
 * @return Previous notification value
 */
uint32_t Soc_Task_ClearNotification(void);

/**
 * @brief SOC task implementation
 *
 * @description This is the actual task function that runs the SOC estimation.
 *
 * @param params Task parameters (unused)
 *
 * @note This function should be passed to xTaskCreate
 */
void Soc_Task_Main(void* params);

/**
 * @brief Initialize current sensor interface
 *
 * @description Sets up interface for current measurements.
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Task_InitCurrentSensor(void);

/**
 * @brief Read current from sensor
 *
 * @description Reads current measurement from hardware.
 *
 * @param[out] current_mA Pointer to store current in mA
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Task_ReadCurrent(int32_t* const current_mA);

/**
 * @brief Read voltage from sensor
 *
 * @description Reads voltage measurement from hardware.
 *
 * @param[out] voltage_mV Pointer to store voltage in mV
 *
 * @return SOC_STATUS_OK on success
 */
Soc_StatusType Soc_Task_ReadVoltage(uint32_t* const voltage_mV);

#endif /* SOC_TASK_H */

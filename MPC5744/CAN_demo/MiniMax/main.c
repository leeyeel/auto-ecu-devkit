/**
 * @file    main.c
 * @brief   Main Application for MPC5744 CAN Demo
 * @details Demonstrates CAN communication for battery voltage transmission.
 *          Initializes CAN, configures interrupts, and periodically sends
 *          battery voltage data via CAN.
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
#include "Std_Types.h"
#include "Can.h"
#include "Can_PBcfg.h"
#include "Mpc5744_Siu_Cfg.h"
#include "BatCanSender.h"

/*
 *****************************************************************************************
 * PROJECT INFORMATION
 *****************************************************************************************
 */

/** @brief Project Name */
#define PROJECT_NAME                   "MPC5744 CAN Battery Demo"

/** @brief Project Version */
#define PROJECT_VERSION                "1.0.0"

/**
 * @brief   System Clock Frequency (Hz)
 * @details MPC5744 default system clock
 */
#define SYS_CORE_FREQ_HZ               (200000000UL)   /**< 200 MHz */

/**
 * @brief   Peripheral Clock Frequency (Hz)
 * @details FlexCAN clock (usually half of system clock)
 */
#define CAN_PCLK_FREQ_HZ               (40000000UL)    /**< 40 MHz */

/*
 *****************************************************************************************
 * LOCAL CONSTANTS
 *****************************************************************************************
 */

/** @brief Main loop cycle time in milliseconds */
#define MAIN_LOOP_CYCLE_MS             (10U)

/** @brief LED toggle period in milliseconds */
#define LED_TOGGLE_PERIOD_MS           (500U)

/**
 * @brief   System Tick Configuration
 * @details Configure SysTick for 1ms interrupts
 */
#define SYSTICK_LOAD                   ((CAN_PCLK_FREQ_HZ / 1000U) - 1U)

/*
 *****************************************************************************************
 * LOCAL TYPES
 *****************************************************************************************
 */

/**
 * @brief   LED State Structure
 */
typedef struct
{
    boolean bLed1State;   /**< LED1 (Power) state */
    boolean bLed2State;   /**< LED2 (CAN TX) state */
    boolean bLed3State;   /**< LED3 (CAN RX) state */
    boolean bLed4State;   /**< LED4 (Error) state */

} Main_LedStateType;

/**
 * @brief   Demo State Structure
 */
typedef struct
{
    boolean bInitialized;     /**< System initialization status */
    boolean bCanError;        /**< CAN error flag */
    uint32_t u32ErrorCount;   /**< Error counter */
    uint32_t u32TxCount;      /**< TX message counter */
    uint32_t u32RxCount;      /**< RX message counter */
    uint32_t u32TickCount;    /**< System tick counter */

} Main_DemoStateType;

/*
 *****************************************************************************************
 * LOCAL VARIABLES
 *****************************************************************************************
 */

/** @brief Demo state */
static Main_DemoStateType Main_strDemoState =
{
    .bInitialized  = FALSE,
    .bCanError     = FALSE,
    .u32ErrorCount = 0U,
    .u32TxCount    = 0U,
    .u32RxCount    = 0U,
    .u32TickCount  = 0U
};

/** @brief LED state */
static Main_LedStateType Main_strLedState =
{
    .bLed1State = TRUE,
    .bLed2State = FALSE,
    .bLed3State = FALSE,
    .bLed4State = FALSE
};

/** @brief Simulated battery state */
static Bat_StateType Main_strBatteryState;

/** @brief Current tick timestamp */
static uint32_t Main_u32CurrentTick = 0U;

/** @brief Last LED update timestamp */
static uint32_t Main_u32LastLedUpdate = 0U;

/*
 *****************************************************************************************
 * STATIC FUNCTION DECLARATIONS
 *****************************************************************************************
 */

/**
 * @brief   Initialize System Hardware
 * @details Initializes clocks, pins, and peripherals.
 */
static void Main_InitHardware(void);

/**
 * @brief   Configure System Clocks
 * @details Configures MPC5744 clock tree.
 */
static void Main_InitClocks(void);

/**
 * @brief   Configure CAN Pins
 * @details Configures CAN0 TX/RX pins.
 */
static void Main_InitCanPins(void);

/**
 * @brief   Initialize Interrupts
 * @details Configures and enables CAN interrupts.
 */
static void Main_InitInterrupts(void);

/**
 * @brief   Initialize LEDs
 * @details Configures LED pins for status indication.
 */
static void Main_InitLeds(void);

/**
 * @brief   Initialize Simulated Battery Data
 * @details Sets up initial battery state for demo.
 */
static void Main_InitBatteryData(void);

/**
 * @brief   Update Simulated Battery Data
 * @details Updates battery readings with simulated values.
 */
static void Main_UpdateBatteryData(void);

/**
 * @brief   Update LEDs
 * @details Updates LED states based on system status.
 */
static void Main_UpdateLeds(void);

/**
 * @brief   CAN Transmit Callback
 * @details Called when CAN transmission completes.
 */
static void Main_CanTxConfirmation(void);

/**
 * @brief   CAN Receive Callback
 * @details Called when CAN message is received.
 * @param   pPduInfo  Pointer to received PDU
 */
static void Main_CanRxIndication(const Can_PduType* pPduInfo);

/**
 * @brief   CAN Busoff Callback
 * @details Called when CAN bus-off occurs.
 */
static void Main_CanBusoffNotification(void);

/**
 * @brief   CAN Error Callback
 * @details Called when CAN error occurs.
 * @param   errorType  Type of error
 */
static void Main_CanErrorNotification(Can_ErrorType errorType);

/**
 * @brief   System Tick Handler
 * @details Called every system tick (1ms).
 */
static void Main_SysTickHandler(void);

/**
 * @brief   Main Loop
 * @details Main application loop.
 */
static void Main_Loop(void);

/**
 * @brief   Print Status
 * @details Prints demo status via debug output.
 */
static void Main_PrintStatus(void);

/*
 *****************************************************************************************
 * STATIC FUNCTION DEFINITIONS
 *****************************************************************************************
 */

static void Main_InitHardware(void)
{
    /* Initialize system clocks */
    Main_InitClocks();

    /* Configure CAN pins */
    Main_InitCanPins();

    /* Initialize LEDs */
    Main_InitLeds();

    /* Initialize interrupts */
    Main_InitInterrupts();
}

static void Main_InitClocks(void)
{
    /* Note: In production, this would configure:
     * - PLL0 for core clock
     * - PLL1 for peripheral clock
     * - System clock divider
     * - FlexCAN clock source
     *
     * For this demo, we assume default clock configuration
     * with:
     * - Core clock: 200 MHz
     * - Peripheral clock: 40 MHz (FlexCAN source)
     */
}

static void Main_InitCanPins(void)
{
    /* Configure CAN0 TX pin (PC10) */
    Siu_CanPinConfig(SIU_PORTC, 10U, TRUE);

    /* Configure CAN0 RX pin (PC11) */
    Siu_CanPinConfig(SIU_PORTC, 11U, FALSE);
}

static void Main_InitInterrupts(void)
{
    /* Configure CAN0 interrupt priority */
    Siu_SetIntPriority(SIU_CAN0_IRQ, SIU_CAN_INT_PRIORITY);

    /* Enable CAN0 interrupt */
    Siu_EnableInt(SIU_CAN0_IRQ);

    /* Register CAN callbacks */
    Can_RegisterTxConfirmationCallback(Main_CanTxConfirmation);
    Can_RegisterRxIndicationCallback(Main_CanRxIndication);
    Can_RegisterBusoffNotificationCallback(Main_CanBusoffNotification);
    Can_RegisterErrorNotificationCallback(Main_CanErrorNotification);
}

static void Main_InitLeds(void)
{
    /* Note: LED configuration is platform-specific */
    /* In production, this would configure GPIO pins for LEDs */
}

static void Main_InitBatteryData(void)
{
    /* Initialize battery state structure */
    Main_strBatteryState.strCellVoltage.u8CellCount = 4U;

    /* Cell voltages (mV) - 3.7V nominal per cell */
    Main_strBatteryState.strCellVoltage.au16CellVoltage[0] = 3700U;
    Main_strBatteryState.strCellVoltage.au16CellVoltage[1] = 3710U;
    Main_strBatteryState.strCellVoltage.au16CellVoltage[2] = 3695U;
    Main_strBatteryState.strCellVoltage.au16CellVoltage[3] = 3705U;

    /* Total voltage */
    Main_strBatteryState.strTotalMeasurement.u16TotalVoltage = 14810U;

    /* Current (mA) - positive = charging */
    Main_strBatteryState.strTotalMeasurement.i16Current = 1500U;
    Main_strBatteryState.strTotalMeasurement.bCharging = TRUE;

    /* Temperatures */
    Main_strBatteryState.strTemperature.i8CellTemp = 25U;
    Main_strBatteryState.strTemperature.i8MosfetTemp = 30U;
    Main_strBatteryState.strTemperature.i8MinTemp = 24U;
    Main_strBatteryState.strTemperature.i8MaxTemp = 26U;

    /* Status flags */
    Main_strBatteryState.strStatusFlags.bSystemReady = TRUE;
    Main_strBatteryState.strStatusFlags.bBalancingActive = FALSE;
    Main_strBatteryState.strStatusFlags.bOvertempWarning = FALSE;
    Main_strBatteryState.strStatusFlags.bOvervoltWarning = FALSE;
    Main_strBatteryState.strStatusFlags.bUndervoltWarning = FALSE;
    Main_strBatteryState.strStatusFlags.bOvercurrentFault = FALSE;
    Main_strBatteryState.strStatusFlags.bCommsError = FALSE;
    Main_strBatteryState.strStatusFlags.bFaultActive = FALSE;

    /* State of charge */
    Main_strBatteryState.u8SocPercent = 75U;

    /* Remaining capacity */
    Main_strBatteryState.u16RemainingCapacity = 6000U;

    /* Timestamp */
    Main_strBatteryState.u32Timestamp = 0U;
}

static void Main_UpdateBatteryData(void)
{
    uint8_t u8CellIdx;
    int16_t i16VoltageDelta;
    int16_t i16CurrentDelta;
    int8_t i8TempDelta;

    /* Update cell voltages with slight variations */
    for (u8CellIdx = 0U; u8CellIdx < Main_strBatteryState.strCellVoltage.u8CellCount; u8CellIdx++)
    {
        /* Small random variation (±10mV) */
        i16VoltageDelta = (int16_t)((Main_strBatteryState.u32TickCount + u8CellIdx) % 21U) - 10U;
        Main_strBatteryState.strCellVoltage.au16CellVoltage[u8CellIdx] =
            (uint16_t)((int32_t)Main_strBatteryState.strCellVoltage.au16CellVoltage[u8CellIdx] + i16VoltageDelta);
    }

    /* Update total voltage */
    Main_strBatteryState.strTotalMeasurement.u16TotalVoltage = 0U;
    for (u8CellIdx = 0U; u8CellIdx < Main_strBatteryState.strCellVoltage.u8CellCount; u8CellIdx++)
    {
        Main_strBatteryState.strTotalMeasurement.u16TotalVoltage +=
            Main_strBatteryState.strCellVoltage.au16CellVoltage[u8CellIdx];
    }

    /* Update current with slight variation */
    i16CurrentDelta = (int16_t)((Main_strBatteryState.u32TickCount % 7U) - 3U);
    Main_strBatteryState.strTotalMeasurement.i16Current =
        (int16_t)((int32_t)Main_strBatteryState.strTotalMeasurement.i16Current + i16CurrentDelta);

    /* Update temperature */
    i8TempDelta = (int8_t)((Main_strBatteryState.u32TickCount % 3U) - 1U);
    Main_strBatteryState.strTemperature.i8CellTemp =
        (int8_t)((int32_t)Main_strBatteryState.strTemperature.i8CellTemp + i8TempDelta);

    /* Update SOC (very slow discharge) */
    if ((Main_strBatteryState.u32TickCount % 60000U) == 0U)  /* Every minute */
    {
        if (Main_strBatteryState.u8SocPercent > 0U)
        {
            Main_strBatteryState.u8SocPercent--;
        }
    }

    /* Update timestamp */
    Main_strBatteryState.u32Timestamp = Main_strBatteryState.u32TickCount;
}

static void Main_UpdateLeds(void)
{
    /* LED1 (Power) - always on during normal operation */
    Main_strLedState.bLed1State = TRUE;

    /* LED2 (TX) - blinks on transmission */
    if (Main_strDemoState.u32TxCount > 0U)
    {
        Main_strLedState.bLed2State = (Main_strDemoState.u32TxCount % 2U) == 1U;
    }

    /* LED3 (RX) - blinks on receive */
    if (Main_strDemoState.u32RxCount > 0U)
    {
        Main_strLedState.bLed3State = (Main_strDemoState.u32RxCount % 2U) == 1U;
    }

    /* LED4 (Error) - on if error occurred */
    Main_strLedState.bLed4State = Main_strDemoState.bCanError;
}

static void Main_CanTxConfirmation(void)
{
    /* Increment TX counter */
    Main_strDemoState.u32TxCount++;

    /* Reset error flag on successful TX */
    Main_strDemoState.bCanError = FALSE;
}

static void Main_CanRxIndication(const Can_PduType* pPduInfo)
{
    /* Validate input */
    if (pPduInfo == NULL_PTR)
    {
        return;
    }

    /* Increment RX counter */
    Main_strDemoState.u32RxCount++;

    /* Reset error flag on successful RX */
    Main_strDemoState.bCanError = FALSE;
}

static void Main_CanBusoffNotification(void)
{
    /* Set error flag */
    Main_strDemoState.bCanError = TRUE;

    /* Increment error counter */
    Main_strDemoState.u32ErrorCount++;
}

static void Main_CanErrorNotification(Can_ErrorType errorType)
{
    /* Set error flag */
    Main_strDemoState.bCanError = TRUE;

    /* Increment error counter */
    Main_strDemoState.u32ErrorCount++;
}

static void Main_SysTickHandler(void)
{
    /* Increment tick counter */
    Main_u32CurrentTick++;
    Main_strDemoState.u32TickCount++;
}

static void Main_Loop(void)
{
    Bat_SenderReturnType eBatRet;

    /* Update simulated battery data */
    Main_UpdateBatteryData();

    /* Update battery state in sender module */
    eBatRet = BatCanSender_UpdateState(&Main_strBatteryState);

    if (eBatRet != BAT_SENDER_OK)
    {
        /* Update failed */
    }

    /* Execute battery sender task (sends periodic voltage broadcasts) */
    eBatRet = BatCanSender_Task();

    if (eBatRet != BAT_SENDER_OK)
    {
        Main_strDemoState.bCanError = TRUE;
        Main_strDemoState.u32ErrorCount++;
    }

    /* Update LEDs periodically */
    if ((Main_u32CurrentTick - Main_u32LastLedUpdate) >= LED_TOGGLE_PERIOD_MS)
    {
        Main_UpdateLeds();
        Main_u32LastLedUpdate = Main_u32CurrentTick;
    }
}

static void Main_PrintStatus(void)
{
    /* Print status via debug output */
    /* In production, this would use UART/SWD for debug */
}

/*
 *****************************************************************************************
 * INTERRUPT SERVICE ROUTINES
 *****************************************************************************************
 */

/**
 * @brief   CAN0 Interrupt Handler
 * @details Handles CAN0 interrupt events.
 */
void CAN0_Handler(void)
{
    Can_IsrHandler_Controller0();
}

/**
 * @brief   SysTick Interrupt Handler
 * @details Handles system timer interrupt.
 */
void SysTick_Handler(void)
{
    Main_SysTickHandler();
}

/*
 *****************************************************************************************
 * GLOBAL FUNCTION DEFINITIONS
 *****************************************************************************************
 */

/**
 * @brief   Main Entry Point
 * @details System startup and main loop execution.
 * @return  Does not return (infinite loop)
 */
int main(void)
{
    Can_ReturnType eCanRet;
    Bat_SenderReturnType eBatRet;

    /* Print banner */
    /* Note: Would use UART/SWD for actual output */

    /* Initialize hardware */
    Main_InitHardware();

    /* Initialize CAN driver */
    eCanRet = Can_Init();

    if (eCanRet != CAN_OK)
    {
        /* Initialization failed */
        while (1U)
        {
            /* Wait for watchdog reset */
        }
    }

    /* Set CAN controller to started mode */
    eCanRet = Can_SetControllerMode(0U, CAN_CS_STARTED);

    if (eCanRet != CAN_OK)
    {
        /* Failed to start controller */
        while (1U)
        {
            /* Wait for watchdog reset */
        }
    }

    /* Enable CAN interrupts */
    (void)Can_EnableInterrupt(0U, (CAN_IT_TX | CAN_IT_RX | CAN_IT_ERROR | CAN_IT_BUSOFF));

    /* Initialize battery data */
    Main_InitBatteryData();

    /* Initialize battery CAN sender */
    eBatRet = BatCanSender_Init();

    if (eBatRet != BAT_SENDER_OK)
    {
        /* Initialization failed */
        while (1U)
        {
            /* Wait for watchdog reset */
        }
    }

    /* Mark as initialized */
    Main_strDemoState.bInitialized = TRUE;

    /* Main loop */
    while (1U)
    {
        /* Execute main loop */
        Main_Loop();
    }

    /* Never reached */
    return 0;
}

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

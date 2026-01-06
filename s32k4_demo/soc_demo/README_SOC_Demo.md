# SOC Estimation Demo for S32K4

## Overview

This demo implements a 5ms periodic battery State of Charge (SOC) estimation task using the Coulomb Counting method on NXP S32K4 microcontroller with FreeRTOS.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Application Layer                             │
│                  (Soc_Demo_Main.c)                               │
│  - System initialization                                         │
│  - Public API (Soc_Demo_GetSoc, etc.)                           │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                    Task Layer (Soc_Task.c)                       │
│  - 5ms periodic task                                              │
│  - Sensor reading interface                                      │
│  - Error handling                                                │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                  Algorithm Layer (Soc_Algorithm.c)               │
│  - Coulomb Counting integration                                  │
│  - OCV fusion (optional)                                         │
│  - Plausibility checks                                           │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                  Driver Layer (Soc_Timer_S32K4.c)                │
│  - LPIT timer configuration                                      │
│  - 5ms interrupt generation                                      │
│  - Task notification                                             │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                      S32K4 Hardware                              │
│  - LPIT (Low Power Interrupt Timer)                              │
│  - ADC (Current/Voltage measurement)                             │
│  - FreeRTOS kernel                                               │
└─────────────────────────────────────────────────────────────────┘
```

## Files

```
s32k4/
├── Soc_Types.h              # Type definitions and error codes
├── Soc_Cfg.h                # Configuration parameters
├── Soc_Cfg.c                # Configuration implementation
├── Soc_Timer_S32K4.h        # LPIT timer driver interface
├── Soc_Timer_S32K4.c        # LPIT timer driver implementation
├── Soc_Algorithm.h          # Coulomb Counting algorithm interface
├── Soc_Algorithm.c          # Coulomb Counting algorithm implementation
├── Soc_Task.h               # Task interface
├── Soc_Task.c               # FreeRTOS task implementation
└── Soc_Demo_Main.c          # Main entry point
```

## Key Features

### 5ms Timer (LPIT)
- Uses S32K4 LPIT Channel 0
- Configurable period (default: 5000μs)
- Interrupt-driven task notification
- 1μs resolution (1MHz timer clock)

### Coulomb Counting Algorithm
- Charge integration: Q = ∫I·dt
- Coulomb efficiency compensation (default: 98.5%)
- SOC in per-mille (0-1000) for 0.1% resolution
- Automatic direction detection (charge/discharge)

### Safety Features
- Parameter validation
- Plausibility checks (rate-of-change monitoring)
- Data freshness detection
- Consecutive error counting
- Safe state transition

## Configuration

Edit `Soc_Cfg.h` to customize:

```c
/* Battery Configuration */
#define SOC_CFG_BATTERY_CAPACITY_AH         (50U)    // 50Ah battery
#define SOC_CFG_NOMINAL_VOLTAGE_MV          (37000U) // 37V nominal
#define SOC_CFG_MIN_VOLTAGE_MV              (30000U) // 30V minimum
#define SOC_CFG_MAX_VOLTAGE_MV              (42000U) // 42V maximum
#define SOC_CFG_COULOMB_EFFICIENCY_0P1      (985U)   // 98.5% efficiency

/* Timer Configuration */
#define SOC_CFG_TIMER_PERIOD_US             (5000U)  // 5ms period
#define SOC_CFG_TIMER_IRQ_PRIORITY          (6U)     // Interrupt priority

/* Task Configuration */
#define SOC_CFG_TASK_PRIORITY               (4U)     // Task priority
#define SOC_CFG_TASK_STACK_SIZE             (256U)   // Stack size in words
```

## Usage

### Basic Usage

```c
#include "Soc_Demo_Main.h"

void MyTask(void)
{
    // Wait for system initialization
    while (!Soc_Demo_IsReady())
    {
        vTaskDelay(10);
    }

    // Get SOC
    uint16_t socPermille = Soc_Demo_GetSoc();  // 0-1000 (0-100%)
    float socPercent = Soc_Demo_GetSocPercent(); // 0.0-100.0%

    // Get remaining capacity
    int32_t remaining_mAh = Soc_Demo_GetRemainingCapacity();

    // Get direction
    Soc_DirectionType dir = Soc_Demo_GetDirection();
}
```

### Reset SOC

```c
// Reset to 100% (e.g., after full charge)
Soc_Demo_Reset(1000U);

// Reset to specific value
Soc_Demo_Reset(750U);  // 75%
```

## Integration

### 1. Include Headers

Add the SOC demo source files to your build and include:

```c
#include "Soc_Types.h"
#include "Soc_Cfg.h"
#include "Soc_Algorithm.h"
#include "Soc_Timer_S32K4.h"
#include "Soc_Task.h"
#include "Soc_Demo_Main.h"
```

### 2. Configure Interrupt

Place LPIT0_Ch0_IRQHandler in your vector table:

```c
void LPIT0_Ch0_IRQHandler(void)
{
    Soc_Timer_S32K4_Isr();
    Soc_Task_NotifyFromIsr();
}
```

### 3. Add to Startup

In your main initialization:

```c
int main(void)
{
    // Hardware init
    SystemInit();
    BoardInit();

    // Initialize SOC
    System_Init();

    // Start scheduler
    vTaskStartScheduler();

    for (;;);
}
```

### 4. ADC Integration

Implement `Soc_Task_ReadCurrent()` and `Soc_Task_ReadVoltage()` with actual ADC reads:

```c
Soc_StatusType Soc_Task_ReadCurrent(int32_t* const current_mA)
{
    // Read ADC channel for current
    uint16_t adcValue = Adc_ReadChannel(ADC_CURRENT_CHANNEL);

    // Convert to mA using calibration
    *current_mA = (int32_t)adcValue - CURRENT_ZERO_OFFSET;
    *current_mA = (*current_mA * CURRENT_SCALE) / 1000;

    return SOC_STATUS_OK;
}
```

## Algorithm Details

### Coulomb Counting

The SOC is calculated by integrating current over time:

```
Q(t) = Q(t₀) + ∫₀ᵗ I(τ) dτ
SOC(t) = SOC(t₀) + (Q(t) - Q(t₀)) / Q_capacity × 1000
```

Where:
- Q(t) = Accumulated charge in microampere-seconds
- I(t) = Current in milliamperes
- Q_capacity = Battery capacity in microampere-seconds

### Coulomb Efficiency

During charging, efficiency < 100% is applied:

```
Q_stored = Q_charged × η
```

Where η is the coulomb efficiency (default: 0.985).

### OCV Fusion (Optional)

When current is near zero, OCV (Open Circuit Voltage) can be used to correct drift:

```c
// In Soc_Algorithm_UpdateWithOcvFusion
if (|I| < I_min) {
    SOC_final = SOC_cc × (1 - w) + SOC_ocv × w
}
```

## Safety Considerations

### Plausibility Checks
- Maximum SOC change per update: 10 per-mille (1%)
- Valid current range: ±500A
- Valid voltage range: 30V - 42V

### Error Handling
- Consecutive error threshold: 5 errors
- Transition to degraded mode on threshold
- Algorithm reset on excessive errors

### Limitations
- Coulomb counting accumulates error over time
- Recommended: Periodic OCV fusion or full charge resets
- Not a substitute for battery management system (BMS)

## Testing

### Unit Tests
- Test algorithm with known current/time inputs
- Verify SOC updates correctly
- Test plausibility check limits

### Integration Tests
- Verify 5ms timing accuracy
- Test task notification mechanism
- Verify interrupt latency

## Compliance

- **MISRA C:2012**: All code follows mandatory rules
- **ISO 26262**: Designed with safety principles
- **AUTOSAR**: Architecture aligns with AUTOSAR requirements

## References

- NXP S32K4 Reference Manual
- NXP S32K4 SDK Documentation
- Coulomb Counting SOC Estimation (IEEE)
- MISRA C:2012 Guidelines

## Version

- **Version**: 1.0.0
- **Target**: S32K4xx Series
- **Compliance**: MISRA C:2012, ISO 26262

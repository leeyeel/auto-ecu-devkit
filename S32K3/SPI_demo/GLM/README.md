# SPI Sensor Demo for S32K3

## Overview

This is a production-ready SPI sensor reading module for S32K3 automotive MCUs, designed with functional safety (ISO 26262) and automotive ECU development best practices in mind.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Application Layer                        │
│                    (Spi_Sensor_Example.c)                        │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                      API Layer (Spi_Sensor_Api)                  │
│  - Periodic task management                                     │
│  - Channel configuration                                        │
│  - Data validity management                                      │
│  - Degraded mode handling                                        │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                   Driver Layer (Spi_Sensor_Driver)               │
│  - Register read/write operations                                │
│  - Retry logic                                                   │
│  - Diagnostic counters                                          │
│  - CRC validation (optional)                                    │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                    HAL Layer (Spi_Sensor_Hal)                    │
│  - S32K3 LPSPI peripheral control                               │
│  - Chip select management                                       │
│  - Hardware initialization                                      │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                       S32K3 Hardware                             │
│  - LPSPI peripheral                                             │
│  - SIUL2 (pin muxing)                                           │
│  - PCC (clock control)                                          │
└─────────────────────────────────────────────────────────────────┘
```

## Files Structure

```
Spi_Sensor_Demo/
├── inc/
│   ├── Spi_Sensor_Types.h       # Type definitions and status codes
│   ├── Spi_Sensor_Hal.h         # HAL layer interface
│   ├── Spi_Sensor_Driver.h      # Driver layer interface
│   ├── Spi_Sensor_Api.h         # API layer interface
│   └── Spi_Sensor_Cfg.h         # Compile-time configuration
├── src/
│   ├── Spi_Sensor_Hal.c         # HAL implementation
│   ├── Spi_Sensor_Driver.c      # Driver implementation
│   └── Spi_Sensor_Api.c         # API implementation
└── example/
    └── Spi_Sensor_Example.c     # Example application
```

## Key Features

### Functional Safety Compliance

- **Deterministic behavior**: No dynamic memory allocation, fixed buffer sizes
- **Error handling**: All functions return status codes, all errors are handled
- **Input validation**: All parameters validated before use
- **Timeout protection**: All blocking operations have timeouts
- **Data validity flags**: Clear indication of data quality
- **Degraded mode**: Safe state when errors persist

### Coding Standard Compliance

- **MISRA C:2012**: Follows automotive coding rules
- **No forbidden constructs**: No goto, recursion, VLAs, or dynamic allocation
- **Fixed-width types**: All integers use stdint.h types
- **Explicit type conversions**: No implicit signed/unsigned mixing
- **Documentation**: All functions have Doxygen comments with safety info

### Diagnostic Support

- **Error counters**: Track CRC errors, timeouts, communication failures
- **Data freshness**: Timestamp-based staleness detection
- **Communication verification**: Ping function to check sensor presence
- **Channel-level monitoring**: Per-channel execution tracking

## Quick Start

### 1. Configure Hardware

Edit `Spi_Sensor_Cfg.h` to match your hardware:

```c
#define SPI_SENSOR_CFG_INSTANCE  SPI_SENSOR_INSTANCE_0
#define SPI_SENSOR_CFG_CS_PIN    SPI_SENSOR_CS_0
#define SPI_SENSOR_CFG_BAUDRATE  SPI_SENSOR_BAUDRATE_1MHZ
```

### 2. Configure Read Channels

```c
#define SPI_SENSOR_CFG_CH1_REGISTER_ADDR  (0x00U)
#define SPI_SENSOR_CFG_CH1_PERIOD_MS      (10U)
```

### 3. Initialize in Your Application

```c
#include "Spi_Sensor_Api.h"
#include "Spi_Sensor_Cfg.h"

// Create configuration
Spi_Sensor_ConfigType hwConfig = SPI_SENSOR_CFG_CREATE_CONFIG();
Spi_Sensor_ApiConfigType apiConfig = SPI_SENSOR_CFG_CREATE_API_CONFIG();

// Initialize
Spi_Sensor_Driver_Init(&hwConfig);
Spi_Sensor_Api_Init(&apiConfig);
```

### 4. Call Cyclic Function

```c
void CyclicTask_10ms(void) {
    uint32_t time = GetSystemTimeMs();
    Spi_Sensor_Api_Cyclic(time);
}
```

### 5. Read Sensor Data

```c
Spi_Sensor_ReadResultType result;

if (Spi_Sensor_Api_GetReading(0, &result) == SPI_SENSOR_STATUS_OK) {
    if (result.validity.dataValid) {
        // Use result.registerValue
    }
}
```

## Configuration Guide

### Hardware Configuration

| Parameter | Description | Default | Range |
|-----------|-------------|---------|-------|
| `SPI_SENSOR_CFG_INSTANCE` | SPI instance | INSTANCE_0 | 0-2 |
| `SPI_SENSOR_CFG_CS_PIN` | Chip select pin | CS_0 | 0-3 |
| `SPI_SENSOR_CFG_BAUDRATE` | Clock speed | 1MHZ | 125KHz-4MHz |
| `SPI_SENSOR_CFG_CPOL` | Clock polarity | 0 | 0-1 |
| `SPI_SENSOR_CFG_CPHA` | Clock phase | 0 | 0-1 |

### Timing Configuration

| Parameter | Description | Default |
|-----------|-------------|---------|
| `SPI_SENSOR_CFG_DEFAULT_TIMEOUT_MS` | Communication timeout | 10ms |
| `SPI_SENSOR_CFG_MAX_RETRY_ATTEMPTS` | Retry attempts | 3 |
| `SPI_SENSOR_CFG_STALENESS_THRESHOLD_MS` | Data staleness | 1000ms |
| `SPI_SENSOR_CFG_MAX_CONSECUTIVE_ERRORS` | Errors before degraded | 10 |

## API Reference

### Initialization

```c
Spi_Sensor_StatusType Spi_Sensor_Driver_Init(const Spi_Sensor_ConfigType* config);
Spi_Sensor_StatusType Spi_Sensor_Api_Init(const Spi_Sensor_ApiConfigType* config);
```

### Cyclic Processing

```c
Spi_Sensor_StatusType Spi_Sensor_Api_Cyclic(uint32_t currentTimeMs);
```

### Data Access

```c
Spi_Sensor_StatusType Spi_Sensor_Api_GetReading(
    uint8_t channelId,
    Spi_Sensor_ReadResultType* result
);

Spi_Sensor_StatusType Spi_Sensor_Api_GetAllReadings(
    Spi_Sensor_ReadResultType* resultArray,
    uint8_t arraySize
);
```

### Diagnostics

```c
Spi_Sensor_StatusType Spi_Sensor_Api_GetDiagCounters(
    Spi_Sensor_DiagCountersType* counters
);

Spi_Sensor_StatusType Spi_Sensor_Driver_VerifyCommunication(void);
```

## Error Handling

### Status Codes

| Code | Description |
|------|-------------|
| `SPI_SENSOR_STATUS_OK` | Operation successful |
| `SPI_SENSOR_STATUS_ERROR` | Generic error |
| `SPI_SENSOR_STATUS_NOT_INIT` | Module not initialized |
| `SPI_SENSOR_STATUS_BUSY` | Operation in progress |
| `SPI_SENSOR_STATUS_TIMEOUT` | Communication timeout |
| `SPI_SENSOR_STATUS_CRC_ERROR` | CRC check failed |
| `SPI_SENSOR_STATUS_INVALID_PARAM` | Invalid parameter |
| `SPI_SENSOR_STATUS_HW_ERROR` | Hardware error |

### Data Validity Flags

```c
typedef struct {
    bool dataValid;   // Main data valid flag
    bool dataStale;   // Data may be outdated
    bool sensorOk;    // Sensor overall health
    bool commOk;      // Communication status
} Spi_Sensor_DataValidType;
```

## Safety Considerations

### 1. Initialization Safety

- All modules track initialization state
- Functions check initialization before operation
- Initialization failure prevents use

### 2. Communication Safety

- Timeout on all SPI transfers
- Retry logic for transient failures
- Degraded mode on persistent errors

### 3. Data Safety

- Validity flags indicate data quality
- Staleness detection prevents use of old data
- Range checking catches invalid sensor values

### 4. Resource Safety

- Fixed memory allocation (no malloc/free)
- Array bounds checking
- No recursion or unbounded loops

## Integration Checklist

- [ ] Review and adjust `Spi_Sensor_Cfg.h` for your hardware
- [ ] Implement hardware initialization (SPI pins, clocks)
- [ ] Implement system time function (Example_GetSystemTime)
- [ ] Add cyclic function call to your main loop/OS task
- [ ] Test with actual sensor hardware
- [ ] Verify timing requirements (execution time measurement)
- [ ] Configure diagnostic monitoring (error counters)
- [ ] Implement error handling hooks
- [ ] Run static analysis (MISRA check)
- [ ] Perform unit testing

## Porting to Other MCUs

To port to a different MCU:

1. **HAL Layer**: Replace `Spi_Sensor_Hal.c` with your SPI driver
2. **Configuration**: Update hardware configuration in `Spi_Sensor_Cfg.h`
3. **Timing**: Implement `Example_GetSystemTime()` for your platform
4. **Interrupts**: Adjust ISR handling if using non-blocking mode

## License

This code is provided as an example for automotive ECU development. Adapt and modify as needed for your application.

## Support

For questions or issues related to:
- **S32K3 hardware**: Refer to NXP S32K3 Reference Manual
- **RTD/SDK**: Refer to NXP Real-Time Drivers documentation
- **AUTOSAR**: Refer to AUTOSAR specification

---

**Version**: 1.0.0
**Target**: S32K3xx Series
**Compliance**: MISRA C:2012, ISO 26262

# SPI Sensor Demo for S32K4

## Overview

This is a production-ready SPI sensor reading module for **NXP S32K4** automotive MCUs, designed with functional safety (ISO 26262) and automotive ECU development best practices in mind.

S32K4 is the next-generation S32K series with enhanced features:
- Up to 4 LPSPI instances (vs 3 on S32K3)
- Higher clock speeds
- Enhanced safety features

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Application Layer                             │
│              (Spi_Sensor_Example_S32K4.c)                        │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│              API Layer (Spi_Sensor_Api for S32K4)               │
│  - Periodic task management                                     │
│  - Channel configuration                                        │
│  - Data validity management                                      │
│  - Degraded mode handling                                        │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│              Driver Layer (Spi_Sensor_Driver for S32K4)         │
│  - Register read/write operations                                │
│  - Retry logic                                                   │
│  - Diagnostic counters                                          │
│  - CRC validation (optional)                                    │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│              HAL Layer (Spi_Sensor_Hal_S32K4)                   │
│  - S32K4 LPSPI peripheral control                               │
│  - Chip select management                                       │
│  - Hardware initialization                                      │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                       S32K4 Hardware                             │
│  - LPSPI peripheral (4 instances)                               │
│  - SIUL2 (pin muxing)                                           │
│  - CCGE (clock control)                                         │
└─────────────────────────────────────────────────────────────────┘
```

## Files

```
Spi_Sensor_Demo/s32k4/
├── Spi_Sensor_Hal_S32K4.h      # HAL layer interface
├── Spi_Sensor_Hal_S32K4.c      # HAL implementation
├── Spi_Sensor_Cfg_S32K4.h      # S32K4-specific configuration
└── Spi_Sensor_Example_S32K4.c  # Example application
```

## Key Differences from S32K3

| Feature | S32K3 | S32K4 |
|---------|-------|-------|
| LPSPI Instances | 3 | 4 |
| LPSPI Base Addr 0 | 0x4039C000 | 0x4039C000 |
| LPSPI Base Addr 1 | 0x403A0000 | 0x403A0000 |
| LPSPI Base Addr 2 | 0x403A4000 | 0x403A4000 |
| LPSPI Base Addr 3 | N/A | 0x403A8000 |
| Clock Control | PCC | CCGE |
| FIFO Size | 4 words | 4 words |

## Quick Start

### 1. Configure Hardware

Edit `Spi_Sensor_Cfg_S32K4.h`:

```c
#define SPI_SENSOR_CFG_INSTANCE  SPI_SENSOR_INSTANCE_0
#define SPI_SENSOR_CFG_BAUDRATE  SPI_SENSOR_BAUDRATE_1MHZ
#define SPI_SENSOR_CFG_CPOL      SPI_SENSOR_CLOCK_POLARITY_0
#define SPI_SENSOR_CFG_CPHA      SPI_SENSOR_CLOCK_PHASE_0
```

### 2. Configure Sensor Protocol

```c
#define SPI_SENSOR_CFG_CMD_READ   (0x03U)
#define SPI_SENSOR_CFG_CMD_WRITE  (0x02U)
#define SPI_SENSOR_CFG_WHO_AM_I_ADDR  (0x0FU)
#define SPI_SENSOR_CFG_WHO_AM_I_EXPECTED  (0xC5U)
```

### 3. Initialize in Application

```c
#include "Spi_Sensor_Hal_S32K4.h"
#include "Spi_Sensor_Cfg_S32K4.h"

// Create configuration
Spi_Sensor_ConfigType config = SPI_SENSOR_CFG_CREATE_CONFIG();

// Initialize SPI
Spi_Sensor_StatusType status = Spi_Sensor_Hal_S32K4_Init(&config);
```

### 4. Read Register

```c
uint8_t regValue;
Spi_Sensor_StatusType status;

status = Spi_Sensor_Hal_S32K4_ReadRegister(
    SPI_SENSOR_INSTANCE_0,    // SPI instance
    0x20U,                    // Register address
    &regValue,                // Buffer for value
    1U,                       // Number of bytes
    10U                       // Timeout in ms
);

if (status == SPI_SENSOR_STATUS_OK) {
    // Use regValue
}
```

## S32K4-Specific Features

### LPSPI Configuration

S32K4 LPSPI uses the following clock configuration:
- Source: Functional clock (typically 80 MHz)
- Prescaler: 1, 2, 4, 8, 16, 32, 64, 128
- Divider: 1-255

Example calculation for 1 MHz at 80 MHz:
```
Prescaler = 8, Divider = 10
LPSPI_SCK = 80MHz / (8 * 10) = 1MHz
```

### Auto-CS Mode

S32K4 LPSPI can automatically control the chip select signal:

```c
#define SPI_SENSOR_CFG_HW_AUTO_CS  (true)
```

When enabled, hardware manages CS timing based on transfers.

### FIFO Mode

For efficient transfers, enable FIFO mode:

```c
#define SPI_SENSOR_CFG_USE_FIFO    (true)
#define SPI_SENSOR_CFG_TX_FIFO_WATERMARK  (2U)
#define SPI_SENSOR_CFG_RX_FIFO_WATERMARK  (3U)
```

## Safety Considerations

### 1. Timeout Protection

All SPI transfers have timeout protection:
```c
#define SPI_SENSOR_CFG_DEFAULT_TIMEOUT_MS  (10U)
```

### 2. Retry Logic

Transient errors are handled with retry:
```c
#define SPI_SENSOR_CFG_MAX_RETRY_ATTEMPTS (3U)
```

### 3. Data Validation

Range checking for sensor data:
```c
#define SPI_SENSOR_CFG_RANGE_CHECK_ENABLED  (true)
#define SPI_SENSOR_CFG_MIN_SENSOR_VALUE     (0U)
#define SPI_SENSOR_CFG_MAX_SENSOR_VALUE     (255U)
```

### 4. Plausibility Checks

Rate-of-change monitoring:
- Data freshness detection
- Stuck-at detection
- Range violation detection

## Integration Checklist

- [ ] Configure SDK (LPSPI, SIUL2, CCGE)
- [ ] Update configuration in `Spi_Sensor_Cfg_S32K4.h`
- [ ] Set sensor-specific parameters (commands, addresses)
- [ ] Implement system time function
- [ ] Add cyclic call to main loop/OS task
- [ ] Configure watchdog
- [ ] Implement error handling hooks
- [ ] Run static analysis (MISRA check)
- [ ] Perform unit testing

## SDK Integration

### Using S32K4 SDK (RTD)

```c
// In actual implementation:
#include "Lpspi_Ip.h"
#include "Siul2_Ip.h"

// LPSPI configuration (from SDK)
Lpspi_Ip_ConfigurationType lpspiConfig = {
    .baudrate = 1000000U,
    .clockPhase = LPSPI_IP_CLOCK_PHASE_FIRST,
    .clockPolarity = LPSPI_IP_CLOCK_POLARITY_LOW,
    // ... other settings
};

// Initialize via SDK
Lpspi_Ip_Init(&lpspiConfig);
```

### Pin Configuration (SIUL2)

```c
// Example pin configuration
Siul2_Ip_PinConfigurationType pinConfig = {
    .port = PORTB,
    .pin = 0U,
    .mux = SIUL2_PIN_MUX_ALT2,  // LPSPI0_SCK
    // ... other settings
};
```

## Error Codes

| Code | Description |
|------|-------------|
| `SPI_SENSOR_STATUS_OK` | Operation successful |
| `SPI_SENSOR_STATUS_ERROR` | Generic error |
| `SPI_SENSOR_STATUS_NOT_INIT` | Module not initialized |
| `SPI_SENSOR_STATUS_BUSY` | Transfer in progress |
| `SPI_SENSOR_STATUS_TIMEOUT` | Communication timeout |
| `SPI_SENSOR_STATUS_CRC_ERROR` | CRC check failed |
| `SPI_SENSOR_STATUS_INVALID_PARAM` | Invalid parameter |
| `SPI_SENSOR_STATUS_HW_ERROR` | Hardware error |

## Compliance

- **MISRA C:2012**: All code follows mandatory coding rules
- **ISO 26262**: Designed with safety principles
- **AUTOSAR**: Architecture aligns with AUTOSAR SPI requirements

## References

- NXP S32K4 Reference Manual
- NXP S32K4 SDK Documentation
- AUTOSAR SPI Driver Specification
- MISRA C:2012 Guidelines

---

**Version**: 1.0.0
**Target**: S32K4xx Series
**Compliance**: MISRA C:2012, ISO 26262

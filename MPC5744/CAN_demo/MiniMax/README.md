# MPC5744 CAN Battery Voltage Demo

## Overview

This project demonstrates CAN communication for battery voltage transmission using NXP MPC5744 microcontroller. The demo periodically sends battery voltage, current, temperature, and status information via CAN bus.

## Features

- **CAN Driver**: AUTOSAR-like FlexCAN driver for MPC5744
- **Interrupt Handling**: Proper interrupt-driven CAN transmission and reception
- **Battery Data**: Structured battery state management and transmission
- **Data Validation**: Plausibility checks for battery measurements
- **Error Handling**: Comprehensive error detection and notification
- **MISRA Compliance**: Code follows MISRA C:2012 coding standard

## Architecture

```
main.c                 # Main application entry point
├── Can.c/h            # CAN driver implementation
├── Can_Cfg.h          # CAN driver configuration
├── Can_PBcfg.c/h      # Post-build configuration
├── BatCanSender.c/h   # Battery voltage sender module
├── Mpc5744_FlexCAN.h  # FlexCAN register definitions
├── Mpc5744_Siu.c/h    # SIU configuration
└── Std_Types.h        # Standard type definitions
```

## CAN Message Format

### Voltage Broadcast (ID: 0x101, DLC: 8 bytes)

| Byte | Content | Format | Resolution |
|------|---------|--------|------------|
| 0-1  | Total Pack Voltage | LE UINT16 | 1 mV |
| 2-3  | Pack Current | LE INT16 | 10 mA |
| 4    | Cell Count | UINT8 | 1 cell |
| 5    | State of Charge | UINT8 | 1% |
| 6    | CRC-8 | UINT8 | - |
| 7    | Reserved | UINT8 | - |

### Status Message (ID: 0x102, DLC: 4 bytes)

| Byte | Content | Format | Description |
|------|---------|--------|-------------|
| 0    | Status Flags | Bitmask | System status |
| 1    | Cell Temperature | INT8 | 1°C |
| 2    | MOSFET Temperature | INT8 | 1°C |
| 3    | Fault Code | UINT8 | 0 = OK |

### Status Flag Bitmask (Byte 0)

| Bit | Flag | Description |
|-----|------|-------------|
| 0   | System Ready | System operational |
| 1   | Balancing Active | Cell balancing active |
| 2   | Overtemp Warning | High temperature |
| 3   | Overvolt Warning | Voltage above limit |
| 4   | Undervolt Warning | Voltage below limit |
| 5   | Overcurrent Fault | Current exceeds limit |
| 6   | Comms Error | Communication error |
| 7   | Fault Active | General fault |

## CAN Configuration

- **Baudrate**: 500 kbps (configurable)
- **Clock Source**: 40 MHz peripheral clock
- **Message Buffers**: 64 per controller
- **Interrupt Priority**: 100 (configurable)

## Usage

### Initialization

```c
// Initialize CAN driver
Can_ReturnType ret = Can_Init();
if (ret != CAN_OK) {
    // Handle error
}

// Start CAN controller
ret = Can_SetControllerMode(0, CAN_CS_STARTED);
if (ret != CAN_OK) {
    // Handle error
}

// Enable interrupts
Can_EnableInterrupt(0, CAN_IT_TX | CAN_IT_RX | CAN_IT_ERROR | CAN_IT_BUSOFF);

// Initialize battery sender
BatCanSender_Init();
```

### Sending Battery Data

```c
// Update battery state
Bat_StateType state;
state.strTotalMeasurement.u16TotalVoltage = 14800;  // mV
state.strTotalMeasurement.i16Current = 1500;        // mA
state.u8SocPercent = 75;                             // %

BatCanSender_UpdateState(&state);

// Periodic task (call every 10ms)
BatCanSender_Task();
```

### Callbacks

```c
// Register transmission confirmation
Can_RegisterTxConfirmationCallback(MyTxCallback);

// Register receive indication
Can_RegisterRxIndicationCallback(MyRxCallback);

// Register bus-off notification
Can_RegisterBusoffNotificationCallback(MyBusoffCallback);

// Register error notification
Can_RegisterErrorNotificationCallback(MyErrorCallback);
```

## Safety Considerations

This code follows automotive safety principles:

1. **Input Validation**: All inputs are validated before use
2. **Bounds Checking**: Array and range checks prevent overflow
3. **Error Handling**: Errors are detected and propagated
4. **Plausibility Checks**: Battery data is validated against limits
5. **Defensive Programming**: NULL checks and safe defaults
6. **Timeout Protection**: Hardware access includes timeouts

## Plausibility Limits

| Parameter | Minimum | Maximum | Unit |
|-----------|---------|---------|------|
| Total Voltage | 9.0 | 16.8 | V |
| Cell Voltage | 2.5 | 4.35 | V |
| Current | -100 | +100 | A |
| Temperature | -40 | +85 | °C |
| SOC | 0 | 100 | % |

## Interrupt Handling

The CAN driver uses interrupt-driven transmission:

1. **TX Interrupt**: Notifies completion of transmission
2. **RX Interrupt**: Notifies receipt of message
3. **Error Interrupt**: Notifies bus errors
4. **Bus-off Interrupt**: Notifies bus-off state

### Interrupt Service Routine

```c
void CAN0_Handler(void)
{
    Can_IsrHandler_Controller0();
}
```

## Requirements

- NXP MPC5744P or compatible
- CAN transceiver (e.g., TJA1043)
- CANalyzer or similar for message monitoring
- C compiler with MISRA C support (e.g., Green Hills, Wind River, NXP S32DS)

## Compilation

### NXP S32 Design Studio

1. Import project
2. Select S32DS for Power Architecture
3. Configure project settings
4. Build

### Command Line (GCC)

```bash
powerpc-eabicompiler -mcpu=e200z4 -msdata=none -I. -c Can.c -o Can.o
powerpc-eabicompiler -mcpu=e200z4 -msdata=none -I. -c BatCanSender.c -o BatCanSender.o
powerpc-eabicompiler -mcpu=e200z4 -msdata=none -I. -c main.c -o main.o
```

## Testing

1. Connect CAN transceiver to MPC5744 CAN0
2. Connect CAN bus to CANalyzer or other CAN node
3. Flash firmware to device
4. Monitor CAN messages at 500 kbps
5. Verify message IDs 0x101 and 0x102 are transmitted

### Expected Output

```
ID: 0x101  DLC: 8  Data: 96 3A 00 00 04 4B 5A 12
ID: 0x102  DLC: 4  Data: 01 19 1E 00
```

## Limitations

- Single CAN controller (CAN0) used
- No CAN FD support
- No AUTOSAR BSW integration
- Simple SOC estimation (voltage-based)
- Simulated battery data for demo

## Future Enhancements

- CAN FD support
- AUTOSAR integration
- Hardware-in-the-loop testing
- Real battery management integration
- Signal-based message definition (AUTOSAR COM)
- CAN diagnostic services (UDS)
- Secure CAN communication

## License

Copyright (c) 2025. All rights reserved.

## References

- NXP MPC5744P Reference Manual
- NXP MPC5744P Data Sheet
- AUTOSAR CAN Driver Specification
- MISRA C:2012 Guidelines
- ISO 26262 Road Vehicles - Functional Safety

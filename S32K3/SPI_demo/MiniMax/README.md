# SPI Demo for NXP S32K3

SPI通讯Demo，通过LPSPI读取外部设备寄存器状态。

## 目录结构

```
MiniMax/
├── spi_demo.h           # SPI Demo接口头文件
├── spi_demo.c           # SPI Demo实现
├── spi_lpspi_s32k3.h    # S32K3 LPSPI驱动接口
├── spi_lpspi_s32k3.c    # S32K3 LPSPI驱动实现
├── main.c               # 应用入口
├── Makefile             # 构建脚本
└── README.md            # 本文档
```

## 功能特性

- **SPI主模式**: 使用LPSPI作为主机与外部SPI设备通讯
- **寄存器读取**: 读取外部设备寄存器值
- **超时保护**: 传输超时检测
- **重试机制**: 自动重试失败传输
- **错误统计**: 跟踪传输统计信息
- **MISRA-C合规**: 符合MISRA C:2012编码规范

## 硬件要求

- **目标芯片**: NXP S32K3系列 (S32K312/S32K342等)
- **SPI接口**: LPSPI0 - LPSPI3
- **时钟源**: SOSC/SPLL
- **引脚配置**: SCK, MOSI, MISO, PCS

## 软件依赖

- **S32SDK-RTD**: NXP S32K3实时驱动库
- **编译器**: arm-none-eabi-gcc 10.0+
- **C标准**: C11

## 构建说明

### 环境准备

```bash
# 安装ARM GCC工具链
sudo apt-get install gcc-arm-none-eabi

# 验证安装
arm-none-eabi-gcc --version
```

### 构建命令

```bash
# Release版本
make release

# Debug版本
make debug

# 清理
make clean

# 查看构建信息
make info

# 查看内存使用
make size
```

## 配置选项

在 `spi_demo.h` 中可配置以下参数：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `SPI_DEMO_REG_COUNT` | 8 | 每次读取的寄存器数量 |
| `SPI_DEMO_TIMEOUT_MS` | 100 | 传输超时(ms) |
| `SPI_DEMO_MAX_RETRIES` | 3 | 最大重试次数 |
| `SPI_DEMO_BAUD` | 1000000 | SPI波特率(Hz) |

## API说明

### 初始化/反初始化

```c
/* 初始化SPI Demo */
Spi_DemoReturnType Spi_Demo_Init(const Spi_DemoConfigType *pConfig);

/* 反初始化 */
Spi_DemoReturnType Spi_Demo_DeInit(void);
```

### 寄存器读取

```c
/* 读取单个寄存器 */
Spi_DemoReturnType Spi_Demo_ReadRegister(uint8_t regAddr, uint8_t *pValue);

/* 读取多个寄存器 */
Spi_DemoReturnType Spi_Demo_ReadRegisters(uint8_t startAddr,
                                           uint8_t *pData,
                                           uint8_t length);
```

### 数据管理

```c
/* 运行读取序列 */
Spi_DemoReturnType Spi_Demo_RunSequence(Spi_DemoRegDataType *pRegData,
                                         uint8_t regCount);

/* 保存数据到缓冲区 */
Spi_DemoReturnType Spi_Demo_SaveData(const Spi_DemoRegDataType *pRegData,
                                      uint8_t regCount);

/* 导出数据 */
Spi_DemoReturnType Spi_Demo_ExportData(uint8_t *pBuffer, uint16_t bufferSize);
```

### 统计信息

```c
/* 获取传输统计 */
Spi_DemoReturnType Spi_Demo_GetStats(Spi_DemoStatsType *pStats);

/* 获取版本 */
uint32_t Spi_Demo_GetVersion(void);
```

## 返回值说明

| 值 | 说明 |
|---|------|
| `SPI_DEMO_E_OK` | 成功 |
| `SPI_DEMO_E_NOT_OK` | 一般错误 |
| `SPI_DEMO_E_NULL_PTR` | 空指针 |
| `SPI_DEMO_E_TIMEOUT` | 超时 |
| `SPI_DEMO_E_SPI_ERROR` | SPI硬件错误 |
| `SPI_DEMO_E_CRC_ERROR` | CRC校验失败 |

## 安全考虑

本Demo遵循ISO 26262汽车安全标准设计：

- ✅ 所有输入参数验证
- ✅ 传输超时保护
- ✅ 错误状态检测
- ✅ 安全状态定义
- ✅ 数据合理性检查

## 静态分析

```bash
# 使用PC-lint或QAC进行MISRA检查
# 配置文件需单独配置
```

## 移植指南

### 1. 修改LPSPI驱动

在 `spi_lpspi_s32k3.c` 中替换模拟实现：

```c
// 替换模拟函数为实际SDK调用
Lpspi_Ip_StatusType Lpspi_Ip_SyncTransmit(Lpspi_Ip_InstanceType instance,
                                           Lpspi_Ip_DataType    *pData,
                                           uint32_t              timeoutMs)
{
    // 调用实际S32SDK-RTD LPSPI驱动
    return LPSPI_Ip_SyncTransfer(instance, pData, timeoutMs);
}
```

### 2. 配置引脚

根据硬件设计配置SPI引脚：

| 功能 | 默认引脚(S32K312) |
|------|-------------------|
| LPSPI0_SCK | PTC5 |
| LPSPI0_SOUT | PTC6 |
| LPSPI0_SIN | PTC7 |
| LPSPI0_PCS0 | PTC4 |

### 3. 配置时钟

在平台初始化中使能LPSPI时钟：

```c
PCC->LPSPI[0] = PCC_PCS_CLK_SRC_SOSCDIV2 | PCC_PCCR_MASK;
```

## 示例输出

```
========================================
      SPI Demo - SPI Register Read Demo
========================================
Version: 1.0.0
Target: NXP S32K3 Series
SPI: LPSPI0 @ 1 MHz
----------------------------------------

========================================
           DEMO RESULTS
========================================
Status: PASSED

Register Data:
----------------------------------------
  Reg[0x00] = 0x00 [OK]
  Reg[0x01] = 0x01 [OK]
  Reg[0x02] = 0x00 [OK]
  Reg[0x03] = 0x00 [OK]
  Reg[0x04] = 0x00 [OK]
  Reg[0x05] = 0x00 [OK]
  Reg[0x06] = 0x00 [OK]
  Reg[0x0F] = 0x55 [OK]

Statistics:
----------------------------------------
  TX Bytes: 16
  RX Bytes: 16
  Errors:   0
  Timeouts: 0
  Retries:  0
========================================
```

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-07 | 初始版本 |

## 许可证

本项目仅供学习和演示使用。

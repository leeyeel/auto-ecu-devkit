/**
 * @file    spi_lpspi_s32k3.h
 * @brief   LPSPI Driver Adaptation Layer for S32K3 Series
 *
 * @details This module provides the interface between the SPI Demo
 *          application and the NXP S32K3 LPSPI (Low Power SPI) driver.
 *
 * @par     Target Device: NXP S32K3 Series (e.g., S32K312, S32K342)
 * @par     SPI Peripheral: LPSPI0 - LPSPI3
 *
 * @author  Auto ECU Demo
 * @date    2026-01-07
 *
 * @note    This is a mock/interface layer. Actual implementation depends
 *          on S32SDK-RTD (Real-Time Drivers) version used.
 *
 * ============================================================================
 * @par  Assumptions & Constraints:
 *   - Uses S32K3 SDK (RTD) for LPSPI driver
 *   - LPSPI instances: 0, 1, 2, 3
 *   - Maximum transfer size: 256 bytes (FIFO depth)
 * ============================================================================
 */

#ifndef SPI_LPSPI_S32K3_H
#define SPI_LPSPI_S32K3_H

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "stdint.h"
#include "stdbool.h"

/*******************************************************************************
 * LPSPI Driver Type Definitions
 ******************************************************************************/

/**
 * @brief   LPSPI Instance Number
 *
 * @note    S32K312 has LPSPI0, LPSPI1, LPSPI2
 *          S32K342 has LPSPI0, LPSPI1, LPSPI2, LPSPI3
 */
typedef enum
{
    LPSPI_INSTANCE_0 = 0U,   /**< LPSPI0 */
    LPSPI_INSTANCE_1 = 1U,   /**< LPSPI1 */
    LPSPI_INSTANCE_2 = 2U,   /**< LPSPI2 */
    LPSPI_INSTANCE_3 = 3U,   /**< LPSPI3 (S32K342 only) */
    LPSPI_INSTANCE_COUNT = 4U
} Lpspi_Ip_InstanceType;

/**
 * @brief   LPSPI Transfer Status
 *
 * @note    Matches LPSPI_IP_STATUS_TYPE from S32SDK
 */
typedef enum
{
    LPSPI_IP_STATUS_SUCCESS     = 0x00U,  /**< Operation successful */
    LPSPI_IP_STATUS_ERROR       = 0x01U,  /**< General error */
    LPSPI_IP_STATUS_TIMEOUT     = 0x02U,  /**< Transfer timeout */
    LPSPI_IP_STATUS_BUSY        = 0x03U,  /**< Peripheral busy */
    LPSPI_IP_STATUS_RX_OVERRUN  = 0x04U,  /**< RX overrun error */
    LPSPI_IP_STATUS_TX_UNDERRUN = 0x05U,  /**< TX underrun error */
    LPSPI_IP_STATUS_PARITY_ERR  = 0x06U,  /**< Parity error */
    LPSPI_IP_STATUS_MODE_FAULT  = 0x07U,  /**< Mode fault (slave select) */
} Lpspi_Ip_StatusType;

/**
 * @brief   LPSPI Data Transfer Structure
 *
 * @note    Used for blocking transfers
 */
typedef struct
{
    const uint8_t *txData;   /**< Pointer to transmit data (const for safety) */
    uint8_t       *rxData;   /**< Pointer to receive buffer */
    uint16_t       dataSize; /**< Number of bytes to transfer */
} Lpspi_Ip_DataType;

/**
 * @brief   LPSPI Hardware Configuration
 *
 * @note    Configuration parameters for LPSPI peripheral
 */
typedef struct
{
    uint32_t baudRate;          /**< Baud rate in Hz */
    uint32_t clockSource;       /**< Clock source (e.g., SOSCDIV2) */
    uint8_t  bitOrder;          /**< 0=MSB first, 1=LSB first */
    uint8_t  clockPolarity;     /**< 0=low idle, 1=high idle */
    uint8_t  clockPhase;        /**< 0=sample on leading edge */
    uint8_t  chipSelectPin;     /**< Chip select pin number */
    uint8_t  pcsPolarity;       /**< PCS polarity (active low/high) */
} Lpspi_Ip_HWUnitConfigType;

/**
 * @brief   LPSPI Pin Configuration
 *
 * @note    Pin multiplexing for SPI signals
 */
typedef struct
{
    uint8_t  txPin;     /**< TX pin number */
    uint8_t  rxPin;     /**< RX pin number */
    uint8_t  sckPin;    /**< SCK pin number */
    uint8_t  pcsPin;    /**< PCS pin number */
    uint8_t  port;      /**< GPIO port (PTA, PTB, etc.) */
    uint8_t  mux;       /**< Pin mux function */
} Lpspi_Ip_PinConfigType;

/**
 * @brief   LPSPI Configuration Structure
 *
 * @note    Complete LPSPI configuration
 */
typedef struct
{
    Lpspi_Ip_InstanceType   instance;     /**< LPSPI instance number */
    Lpspi_Ip_HWUnitConfigType hwConfig;   /**< Hardware configuration */
    Lpspi_Ip_PinConfigType  pinConfig;    /**< Pin configuration */
} Lpspi_Ip_ConfigType;

/*******************************************************************************
 * LPSPI Driver API
 ******************************************************************************/

/**
 * @brief   Initialize LPSPI Instance
 *
 * @details Initializes the specified LPSPI instance with default settings.
 *
 * @param[in] instance  LPSPI instance number (0-3)
 *
 * @return  LPSPI_IP_STATUS_SUCCESS     - Initialization successful
 * @return  LPSPI_IP_STATUS_ERROR       - Initialization failed
 *
 * @note    Must be called before any other LPSPI functions
 * @note    Configures pins, clock, and enables peripheral
 */
Lpspi_Ip_StatusType Lpspi_Ip_Init(Lpspi_Ip_InstanceType instance);

/**
 * @brief   Deinitialize LPSPI Instance
 *
 * @details Resets LPSPI peripheral and releases resources.
 *
 * @param[in] instance  LPSPI instance number (0-3)
 *
 * @return  LPSPI_IP_STATUS_SUCCESS     - Deinitialization successful
 * @return  LPSPI_IP_STATUS_ERROR       - Deinitialization failed
 *
 * @note    Pins returned to GPIO mode (input)
 * @note    Clock gated off
 */
Lpspi_Ip_StatusType Lpspi_Ip_Deinit(Lpspi_Ip_InstanceType instance);

/**
 * @brief   Configure LPSPI Instance
 *
 * @details Configures LPSPI parameters (baud rate, mode, etc.).
 *
 * @param[in] instance  LPSPI instance number (0-3)
 * @param[in] pConfig   Pointer to configuration (NULL = default)
 *
 * @return  LPSPI_IP_STATUS_SUCCESS     - Configuration successful
 * @return  LPSPI_IP_STATUS_ERROR       - Configuration failed
 *
 * @note    Can be called to change settings after init
 */
Lpspi_Ip_StatusType Lpspi_Ip_SetConfig(Lpspi_Ip_InstanceType   instance,
                                        const Lpspi_Ip_ConfigType *pConfig);

/**
 * @brief   Synchronous SPI Transfer
 *
 * @details Performs blocking SPI transfer (send and receive).
 *
 * @param[in]  instance  LPSPI instance number (0-3)
 * @param[in]  pData     Pointer to transfer data (not NULL)
 * @param[in]  timeoutMs Transfer timeout in milliseconds
 *
 * @return  LPSPI_IP_STATUS_SUCCESS     - Transfer successful
 * @return  LPSPI_IP_STATUS_TIMEOUT     - Transfer timeout
 * @return  LPSPI_IP_STATUS_RX_OVERRUN  - RX overrun occurred
 * @return  LPSPI_IP_STATUS_TX_UNDERRUN - TX underrun occurred
 * @return  LPSPI_IP_STATUS_ERROR       - General error
 *
 * @par     Transfer Behavior:
 *         - Asserts chip select (PCS) before transfer
 *         - Deasserts chip select after transfer
 *         - Blocks until complete or timeout
 *
 * @note    For production code, prefer async transfer with callback
 */
Lpspi_Ip_StatusType Lpspi_Ip_SyncTransmit(Lpspi_Ip_InstanceType instance,
                                           Lpspi_Ip_DataType    *pData,
                                           uint32_t              timeoutMs);

/**
 * @brief   Asynchronous SPI Transfer
 *
 * @details Initiates non-blocking SPI transfer.
 *
 * @param[in]  instance   LPSPI instance number (0-3)
 * @param[in]  pData      Pointer to transfer data (not NULL)
 * @param[in]  callback   Completion callback function
 * @param[in]  userData   User data passed to callback
 *
 * @return  LPSPI_IP_STATUS_SUCCESS     - Transfer initiated
 * @return  LPSPI_IP_STATUS_BUSY        - Peripheral busy
 * @return  LPSPI_IP_STATUS_ERROR       - Initiation failed
 *
 * @note    Callback invoked when transfer completes
 * @note    Preferred for real-time applications
 */
Lpspi_Ip_StatusType Lpspi_Ip_AsyncTransmit(Lpspi_Ip_InstanceType  instance,
                                            Lpspi_Ip_DataType     *pData,
                                            void (*callback)(void *),
                                            void                  *userData);

/**
 * @brief   Get LPSPI Status
 *
 * @details Returns current LPSPI peripheral status.
 *
 * @param[in] instance  LPSPI instance number (0-3)
 *
 * @return  LPSPI status flags (bitmask)
 *
 * @par     Status Flags:
 *         - Bit 0: RX FIFO not empty
 *         - Bit 1: TX FIFO not full
 *         - Bit 2: Transfer busy
 *         - Bit 3: Error flag
 *
 * @note    Use for polling-based operation
 */
uint32_t Lpspi_Ip_GetStatus(Lpspi_Ip_InstanceType instance);

/**
 * @brief   LPSPI Interrupt Handler
 *
 * @details LPSPI interrupt service routine.
 *
 * @param[in] instance  LPSPI instance number (0-3)
 *
 * @note    Must be called from LPSPI ISR
 * @note    Handles RX/TX FIFO and error interrupts
 */
void Lpspi_Ip_IrqHandler(Lpspi_Ip_InstanceType instance);

/**
 * @brief   Set Chip Select
 *
 * @details Manually controls chip select signal.
 *
 * @param[in] instance   LPSPI instance number (0-3)
 * @param[in] csPin      Chip select pin number
 * @param[in] assert     TRUE=assert (low), FALSE=deassert (high)
 *
 * @return  LPSPI_IP_STATUS_SUCCESS     - Operation successful
 * @return  LPSPI_IP_STATUS_ERROR       - Operation failed
 *
 * @note    Only needed for manual CS control
 * @note    Normally handled automatically by transmit function
 */
Lpspi_Ip_StatusType Lpspi_Ip_SetCs(Lpspi_Ip_InstanceType instance,
                                    uint8_t               csPin,
                                    boolean               assert);

#endif /* SPI_LPSPI_S32K3_H */

/*******************************************************************************
 * End of File
 ******************************************************************************/

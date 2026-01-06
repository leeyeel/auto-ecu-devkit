/**
 * @file    Std_Types.h
 * @brief   Standard Types Header for Automotive ECU
 * @details AUTOSAR-like standard type definitions for embedded systems.
 *          Provides fixed-width integer types and common type definitions.
 *
 * @note    Compliant with MISRA C:2012 coding standard
 *
 * @version 1.0.0
 * @date    2025-01-07
 *
 * @copyright Copyright (c) 2025
 */

#ifndef STD_TYPES_H
#define STD_TYPES_H

/*
 *****************************************************************************************
 * INCLUDES
 *****************************************************************************************
 */
#include <stdint.h>

/*
 *****************************************************************************************
 * VERSION DEFINITIONS
 *****************************************************************************************
 */

/** @brief Std_Types Module Version (Major.Minor.Patch) */
#define STD_TYPES_VERSION_MAJOR        (4U)
#define STD_TYPES_VERSION_MINOR        (5U)
#define STD_TYPES_VERSION_PATCH        (0U)

/*
 *****************************************************************************************
 * STANDARD TYPES
 *****************************************************************************************
 */

/**
 * @brief   Signed 8-bit integer
 * @details Range: -128 to +127
 */
typedef int8_t                         int8;

/**
 * @brief   Signed 16-bit integer
 * @details Range: -32,768 to +32,767
 */
typedef int16_t                        int16;

/**
 * @brief   Signed 32-bit integer
 * @details Range: -2,147,483,648 to +2,147,483,647
 */
typedef int32_t                        int32;

/**
 * @brief   Signed 64-bit integer
 * @details Range: -9,223,372,036,854,775,808 to +9,223,372,036,854,775,807
 */
typedef int64_t                        int64;

/**
 * @brief   Unsigned 8-bit integer
 * @details Range: 0 to 255
 */
typedef uint8_t                        uint8;

/**
 * @brief   Unsigned 16-bit integer
 * @details Range: 0 to 65,535
 */
typedef uint16_t                       uint16;

/**
 * @brief   Unsigned 32-bit integer
 * @details Range: 0 to 4,294,967,295
 */
typedef uint32_t                       uint32;

/**
 * @brief   Unsigned 64-bit integer
 * @details Range: 0 to 18,446,744,073,709,551,615
 */
typedef uint64_t                       uint64;

/**
 * @brief   Single precision floating point (32-bit)
 */
typedef float                          float32;

/**
 * @brief   Double precision floating point (64-bit)
 */
typedef double                         float64;

/*
 *****************************************************************************************
 * POINTER TYPES
 *****************************************************************************************
 */

/**
 * @brief   Pointer to const uint8
 */
typedef uint8*                         puint8;

/**
 * @brief   Pointer to const uint8
 */
typedef const uint8*                   pcuint8;

/**
 * @brief   Pointer to uint16
 */
typedef uint16*                        puint16;

/**
 * @brief   Pointer to uint32
 */
typedef uint32*                        puint32;

/*
 *****************************************************************************************
 * BOOLEAN TYPE
 *****************************************************************************************
 */

/**
 * @brief   Boolean type for use in conditions
 * @details Values: FALSE (0) or TRUE (1)
 */
typedef uint8                          boolean;

/** @brief Boolean false value */
#ifndef FALSE
#define FALSE                          (0U)
#endif

/** @brief Boolean true value */
#ifndef TRUE
#define TRUE                           (1U)
#endif

/*
 *****************************************************************************************
 * NULL POINTER
 *****************************************************************************************
 */

/**
 * @brief   NULL pointer constant
 * @details Used to indicate an uninitialized or invalid pointer
 */
#ifndef NULL
#define NULL                           ((void*)0U)
#endif

/**
 * @brief   NULL pointer for const pointer types
 */
#ifndef NULL_PTR
#define NULL_PTR                       ((void*)0U)
#endif

/*
 *****************************************************************************************
 * STD_RETURN_TYPE
 *****************************************************************************************
 */

/**
 * @brief   Standard Return Type
 * @details Generic return type for functions that return status
 */
typedef uint8                          Std_ReturnType;

/** @brief Operation successful */
#ifndef E_OK
#define E_OK                           ((Std_ReturnType)0U)
#endif

/** @brief Operation failed */
#ifndef E_NOT_OK
#define E_NOT_OK                       ((Std_ReturnType)1U)
#endif

/** @brief Parameter null pointer */
#ifndef E_NULL_POINTER
#define E_NULL_POINTER                 ((Std_ReturnType)2U)
#endif

/** @brief Parameter out of range */
#ifndef E_OUT_OF_RANGE
#define E_OUT_OF_RANGE                 ((Std_ReturnType)3U)
#endif

/** @brief Parameter wrong data type */
#ifndef E_DATA_TYPE_MISMATCH
#define E_DATA_TYPE_MISMATCH           ((Std_ReturnType)4U)
#endif

/*
 *****************************************************************************************
 * STD_ON/OFF
 *****************************************************************************************
 */

/**
 * @brief   Standard On/Off type
 * @details Used for feature enable/disable configuration
 */
typedef uint8                          Std_OnOffType;

/** @brief Feature disabled */
#ifndef STD_OFF
#define STD_OFF                        (0U)
#endif

/** @brief Feature enabled */
#ifndef STD_ON
#define STD_ON                         (1U)
#endif

/*
 *****************************************************************************************
 * UNUSED PARAMETER MACRO
 *****************************************************************************************
 */

/**
 * @brief   Unused Parameter Macro
 * @details Used to suppress compiler warnings for unused function parameters.
 *          Must be used in function body.
 */
#define STD_UNUSED(param)              ((void)(param))

/*
 *****************************************************************************************
 * STATIC ASSERT MACRO
 *****************************************************************************************
 */

/**
 * @brief   Static Assertion Macro
 * @details Compile-time assertion for checking constant expressions.
 *          Generates a compilation error if condition is false.
 */
#define STD_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)

/*
 *****************************************************************************************
 * BIT ORDER AND BYTE ORDER DEFINITIONS
 *****************************************************************************************
 */

/** @brief Most Significant Bit first (Motorola format) */
#define MSB_FIRST                      (0U)

/** @brief Least Significant Bit first (Intel format) */
#define LSB_FIRST                      (1U)

/** @brief Big Endian byte order */
#define HIGH_BYTE_FIRST                (0U)

/** @brief Little Endian byte order */
#define LOW_BYTE_FIRST                 (1U)

/*
 *****************************************************************************************
 * VERSION MACRO
 *****************************************************************************************
 */

/**
 * @brief   Get Version
 * @details Returns the version of the module as a 16-bit value
 * @return  Version in format (Major << 8) | Minor
 */
#define STD_GET_VERSION(major, minor) \
    (((major) << 8U) | (minor))

#endif /* STD_TYPES_H */

/*
 *****************************************************************************************
 * END OF FILE
 *****************************************************************************************
 */

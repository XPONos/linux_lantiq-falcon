#ifndef _IFX_TYPES_H
#define _IFX_TYPES_H
/*******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
/** \defgroup IFX_BASIC_TYPES Basic IFX Data Types
    This section describes the Infineon / Lantiq basic data type definitions */
/*@{*/

/** This is the character datatype. */
typedef char                     IFX_char_t;
/** This is the integer datatype. */
typedef signed int               IFX_int_t;
/** This is the unsigned integer datatype. */
typedef unsigned int             IFX_uint_t;
/** This is the unsigned 8-bit datatype. */
typedef unsigned char            IFX_uint8_t;
/** This is the signed 8-bit datatype. */
typedef signed char              IFX_int8_t;
/** This is the unsigned 16-bit datatype. */
typedef unsigned short           IFX_uint16_t;
/** This is the signed 16-bit datatype. */
typedef signed short             IFX_int16_t;
/** This is the unsigned 32-bit datatype. */
typedef unsigned int             IFX_uint32_t;
/** This is the signed 32-bit datatype. */
typedef signed int               IFX_int32_t;
/** This is the float datatype. */
typedef float                    IFX_float_t;
/** This is the void datatype.
   It is only a define to be sure, the type
   is "exactly" the same for C++ compatibility! */
#define IFX_void_t               void

/** This is the unsigned 64-bit datatype. */
typedef unsigned long long int      IFX_uint64_t;
/** This is the signed 64-bit datatype. */
typedef signed long long int        IFX_int64_t;

/** This is the unsigned long datatype.
On 32bit systems it is 4 byte wide.
*/
typedef unsigned long               IFX_ulong_t;
#define HAVE_IFX_ULONG_T

/** This is the signed long datatype.
On 32bit systems it is 4 byte wide.
*/
typedef signed long                 IFX_long_t;
#define HAVE_IFX_LONG_T

/** This is the size data type (32 or 64 bit) */
typedef IFX_ulong_t                    IFX_size_t;
#define HAVE_IFX_SIZE_T

/** This is the signed size data type (32 or 64 bit) */
typedef IFX_long_t                     IFX_ssize_t;
#define HAVE_IFX_SSIZE_T

/** This is the time data type (32 or 64 bit) */
typedef IFX_ulong_t                    IFX_time_t;

/* NOTE: (ANSI X3.159-1989)
      While some of these architectures feature uniform pointers
      which are the size of some integer type, maximally portable
      code may not assume any necessary correspondence between
      different pointer types and the integral types.

      Since pointers and integers are now considered incommensurate,
      the only integer that can be safely converted to a pointer
      is the constant 0.
*/
/** Conversion pointer to unsigned values (32 or 64 bit) */
typedef IFX_ulong_t           IFX_uintptr_t;
#define HAVE_IFX_UINTPTR_T
/** Conversion pointer to signed values (32 or 64 bit) */
typedef IFX_long_t            IFX_intptr_t;
#define HAVE_IFX_INTPTR_T

/** This is the volatile unsigned 8-bit datatype. */
typedef volatile IFX_uint8_t  IFX_vuint8_t;
/** This is the volatile signed 8-bit datatype. */
typedef volatile IFX_int8_t   IFX_vint8_t;
/** This is the volatile unsigned 16-bit datatype. */
typedef volatile IFX_uint16_t IFX_vuint16_t;
/** This is the volatile signed 16-bit datatype. */
typedef volatile IFX_int16_t  IFX_vint16_t;
/** This is the volatile unsigned 32-bit datatype. */
typedef volatile IFX_uint32_t IFX_vuint32_t;
/** This is the volatile signed 32-bit datatype. */
typedef volatile IFX_int32_t  IFX_vint32_t;
/** This is the volatile unsigned 64-bit datatype. */
typedef volatile IFX_uint64_t IFX_vuint64_t;
/** This is the volatile signed 64-bit datatype. */
typedef volatile IFX_int64_t  IFX_vint64_t;
/** This is the volatile float datatype. */
typedef volatile IFX_float_t  IFX_vfloat_t;


/** A type for handling boolean issues. */
typedef enum {
   /** false */
   IFX_FALSE = 0,
   /** true */
   IFX_TRUE = 1
} IFX_boolean_t;


/**
   This type is used for parameters that should enable
   and disable a dedicated feature. */
typedef enum {
   /** disable */
   IFX_DISABLE = 0,
   /** enable */
   IFX_ENABLE = 1
} IFX_enDis_t;

/**
   This type is used for parameters that should enable
   and disable a dedicated feature. */
typedef IFX_enDis_t IFX_operation_t;

/**
   This type has two states, even and odd.
*/
typedef enum {
   /** even */
   IFX_EVEN = 0,
   /** odd */
   IFX_ODD = 1
} IFX_evenOdd_t;


/**
   This type has two states, high and low.
*/
typedef enum {
    /** low */
   IFX_LOW = 0,
   /** high */
   IFX_HIGH = 1
} IFX_highLow_t;

/**
   This type has two states, success and error
*/
typedef enum {
   /** operation failed */
   IFX_ERROR   = (-1),
   /** operation succeeded */
   IFX_SUCCESS = 0
} IFX_return_t;

/** NULL pointer */
#define IFX_NULL         ((IFX_void_t *)0)
/*@}*/ /* IFX_BASIC_TYPES */

#endif /* _IFX_TYPES_H */

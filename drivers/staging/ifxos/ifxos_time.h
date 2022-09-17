#ifndef _IFXOS_TIME_H
#define _IFXOS_TIME_H
/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file
   This file contains definitions for timer and wait handling within driver and
   user (application) space.
*/

/** \defgroup IFXOS_TIME Time and Wait.

   This Group contains the time and wait definitions and function.

   Depending on the environment dedicated functions are available for driver
   and / or user (application) code.

\ingroup IFXOS_INTERFACE
*/

/* ============================================================================
   IFX OS adaptation - Includes
   ========================================================================= */
#include "ifx_types.h"

/* ============================================================================
   IFX OS adaptation - time handling, typedefs
   ========================================================================= */
/** \addtogroup IFXOS_TIME
@{ */

/**
   This is the time datatype [ms]
*/
typedef IFX_uint32_t    IFX_timeMS_t;


/* ============================================================================
   IFX OS adaptation - time handling, functions
   ========================================================================= */

#ifdef CONFIG_LANTIQ_IFXOS_TIME_SLEEP_US
/**
   Sleep a given time in [us].

\param
   sleepTime_us   Time to sleep [us]

\remarks
   Available in Driver and Application Space
*/
IFX_void_t IFXOS_USecSleep(
               IFX_time_t sleepTime_us);
#endif

#ifdef CONFIG_LANTIQ_IFXOS_TIME_SLEEP_MS
/**
   Sleep at least the given time in [ms].

\param
   sleepTime_ms   Time to sleep [ms]

\remarks
   Available in Driver and Application Space

   Note that depending on the system tick setting the actual sleep time can be
   equal to or longer then the specified one, but never be shorter.
*/
IFX_void_t IFXOS_MSecSleep(
               IFX_time_t sleepTime_ms);
#endif

#ifdef CONFIG_LANTIQ_IFXOS_TIME_ELAPSED_TIME_GET_MS
/**
   Get the elapsed time since startup in [ms].

\param
   refTime_ms  Reference time to calculate the elapsed time in [ms].

\return
   Elapsed time in [ms] based on the given reference time

\remark
   Provide refTime_ms = 0 to get the current elapsed time. For measurement provide
   the current time as reference.

\remarks
   Available in Driver and Application Space
*/
IFX_time_t IFXOS_ElapsedTimeMSecGet(
               IFX_time_t refTime_ms);
#endif

/** @} */

#endif      /* #ifndef _IFXOS_TIME_H */





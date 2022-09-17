#ifndef _IFXOS_EVENT_H
#define _IFXOS_EVENT_H
/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file
   This file contains definitions for Event Synchronization and Signalization.
*/

/** \defgroup IFXOS_IF_SYNC Synchronization.

   This Group collect the synchronization and signaling mechanism used within
   IFXOS.

   The IFX OS differs between the synchronization on processes level
   (threads / tasks) and between user and driver space.

\par processes level Synchronization
   For synchronization on thread / task level a "Event feature" is provided.
\note
   The intention of these signaling feature is to communicate between different
   threads / tasks on the same level.
   Here you should take care to keep the split between driver and user space and
   so do not use this for signalization between driver and user code.

\par User-Driver Synchronization
   Therefore the poll/select mechanism is prepared.

\attention
   For the select feature the underlaying OS have corresponding support this.
   Further a corresponding adaptation on user and driver side is required.

\ingroup IFXOS_INTERFACE
*/

/** \defgroup IFXOS_IF_EVENT Event Synchronization.

   This Group contains the synchronization and signalization definitions and function
   for communication of threads and tasks

\note
   The intention of these signaling feature is to communicate between different
   threads / tasks on the same level.
   Here you should take care to keep the split between driver and user space and
   so do not use this for signalization between driver and user code.

\remarks
   For synchronization between user and driver space please have a look for the
   "select" feature.

\remarks
   Because of the above explanation and on the underlaying OS (Linux) the
   implementation of this feature may exist for driver and user space twice.

\ingroup IFXOS_IF_SYNC
*/

/* ============================================================================
   IFX OS adaptation - Includes
   ========================================================================= */
#include "ifx_types.h"
#include <linux/kernel.h>
#include <linux/poll.h> /* wait_queue_head_t */

/* ============================================================================
   IFX LINUX adaptation - EVENT types
   ========================================================================= */
/** \addtogroup IFXOS_EVENT_LINUX_KERNEL
@{ */

/** LINUX Kernel - EVENT, type for event handling. */
typedef struct
{
   /** event object */
   wait_queue_head_t object;
   /** valid flag */
   IFX_boolean_t  bValid;

   /** wakeup condition flag (used for Kernel Version 2.6) */
   int            bConditionFlag;

} IFXOS_event_t; 

/* ============================================================================
   IFX OS adaptation - EVENT handling, functions
   ========================================================================= */

/** \addtogroup IFXOS_IF_EVENT
@{ */

#ifdef CONFIG_LANTIQ_IFXOS_EVENT

/**
   Check the init status of the given event object
*/
#define IFXOS_EVENT_INIT_VALID(P_EVENT_ID)\
   (((P_EVENT_ID)) ? (((P_EVENT_ID)->bValid == IFX_TRUE) ? IFX_TRUE : IFX_FALSE) : IFX_FALSE)

/**
   Initialize a Event Object for synchronization.

\param
   pEventId    Pointer to the Event Object.

\return
   IFX_SUCCESS if the creation was successful, else
   IFX_ERROR in case of error.
*/
IFX_int_t IFXOS_EventInit(
               IFXOS_event_t  *pEventId);

/**
   Delete the given Event Object.

\param
   pEventId    Pointer to the Event Object.

\return
   IFX_SUCCESS if delete was successful, else
   IFX_ERROR if something was wrong
*/
IFX_int_t IFXOS_EventDelete(
               IFXOS_event_t  *pEventId);

/**
   Wakeup a Event Object to signal the occurrence of the "event" to
   the waiting processes.

\param
   pEventId    Pointer to the Event Object.

\return
   IFX_SUCCESS if wakeup was successful, else
   IFX_ERROR if something was wrong
*/
IFX_int_t IFXOS_EventWakeUp(
               IFXOS_event_t  *pEventId);

/**
   Wait for the occurrence of an "event" with timeout.

\param
   pEventId       Pointer to the Event Object.
\param
   waitTime_ms    Max time to wait [ms].

\param
   pRetCode    Points to the return code variable. [O]
               - If the pointer is NULL the return code will be ignored, else
                 the corresponding return code will be set
               - For timeout the return code is set to 1.

\return
   IFX_SUCCESS on success.
   IFX_ERROR   on error or timeout.

\remark
   This functions signals the return reason ("event occurred" or "timeout") via the
   pRetCode variable. To differ between error or timeout the value is set to 1.
*/
IFX_int_t IFXOS_EventWait(
               IFXOS_event_t  *pEventId,
               IFX_uint32_t   waitTime_ms,
               IFX_int32_t    *pRetCode);

#endif      /* #ifdef CONFIG_LANTIQ_IFXOS_EVENT */

/* @} */

#endif      /* #ifndef _IFXOS_EVENT_H */


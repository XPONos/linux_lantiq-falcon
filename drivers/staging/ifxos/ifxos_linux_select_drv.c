/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/* ============================================================================
   Description : IFX Linux adaptation - driver select handling (Kernel Space)
   Remark: ...
   ========================================================================= */

/** \file
   This file contains the IFXOS Layer implementation for LINUX Kernel
   Synchronization Poll / Select.
*/

/* ============================================================================
   IFX Linux adaptation - Global Includes - Kernel
   ========================================================================= */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include "ifx_types.h"

/* ============================================================================
   IFX LINUX adaptation - Kernel SELECT types
   ========================================================================= */
/** \addtogroup IFXOS_DRV_SELECT_LINUX_KERNEL
@{ */

/** Linux Kernel - Select, Wakeup List for select/poll handling. */
typedef wait_queue_head_t           IFXOS_drvSelectQueue_t;

/** Linux Kernel - Select, OS argument type (file) type for select/poll handling. */
typedef struct file                 IFXOS_drvSelectOSArg_t;

/** Linux Kernel - Select, poll table for select/poll handling. */
typedef poll_table                  IFXOS_drvSelectTable_t;

/** @} */

/* ============================================================================
   IFX Linux adaptation - Kernel Driver SELECT handling
   ========================================================================= */
/** \addtogroup IFXOS_DRV_SELECT_LINUX_KERNEL
@{ */

/**
   LINUX Kernel - Initialize a Select Queue Object for synchronization between user
   and driver space via the select / poll mechanism.

\par Implementation
   - setup a LINUX wait queue (see "init_waitqueue_head").

\param
   pDrvSelectQueue   Points to a Driver Select Queue object.

\return
   IFX_SUCCESS if the initialization was successful, else
   IFX_ERROR in case of error.
*/
IFX_int32_t IFXOS_DrvSelectQueueInit(
               IFXOS_drvSelectQueue_t  *pDrvSelectQueue)
{
   init_waitqueue_head(pDrvSelectQueue);
   return IFX_SUCCESS;
}

/**
   LINUX Kernel - Wakeup from the Select Queue all added task.
   This function is used from driver space to signal the occurrence of an event
   from driver space to one or several waiting user (poll / select mechanism).

\par Implementation
   - signal a wakeup for the given wait queue (see "wake_up_interruptible").

\param
   pDrvSelectQueue   Points to used Select Queue object.
\param
   drvSelType        OS specific parameter.
*/
IFX_void_t IFXOS_DrvSelectQueueWakeUp(
               IFXOS_drvSelectQueue_t  *pDrvSelectQueue,
               IFX_uint32_t            drvSelType)
{
   wake_up_interruptible(pDrvSelectQueue);
}

/**
   LINUX Kernel - Add an user thread / task to a Select Queue
   This function is used from user space to add a thread / task to a corresponding
   Select Queue. The thread / task will be waked up if the event occurs or if
   the time expires.

\par Implementation
   - add the user tasks for poll wait (see "poll_wait").

\param
   pDrvSelectOsArg   LINUX file struct, comes from the IO call.
\param
   pDrvSelectQueue   Points to used Select Queue object (Linux wait queue), which can
                     change the poll status.
\param
   pDrvSelectTable   LINUX poll table, comes from the IO call.

\return
   IFX_SUCCESS if the thread / task has been added, else
   IFX_ERROR.
*/
IFX_int32_t IFXOS_DrvSelectQueueAddTask(
               IFXOS_drvSelectOSArg_t  *pDrvSelectOsArg,
               IFXOS_drvSelectQueue_t  *pDrvSelectQueue,
               IFXOS_drvSelectTable_t  *pDrvSelectTable)
{
   poll_wait( (struct file *)pDrvSelectOsArg,
              pDrvSelectQueue,
              (poll_table *)pDrvSelectTable );
   return IFX_SUCCESS;
}

/** @} */

EXPORT_SYMBOL(IFXOS_DrvSelectQueueInit);
EXPORT_SYMBOL(IFXOS_DrvSelectQueueWakeUp);
EXPORT_SYMBOL(IFXOS_DrvSelectQueueAddTask);

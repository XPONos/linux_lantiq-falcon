/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/* ============================================================================
   Description : IFX Linux adaptation - time handling (Kernel Space)
   ========================================================================= */

/** \file
   This file contains the IFXOS Layer implementation for LINUX Kernel
   Time and Sleep.
*/

/* ============================================================================
   IFX Linux adaptation - Global Includes - Kernel
   ========================================================================= */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <asm/delay.h>
#include <linux/delay.h>

#include "ifx_types.h"


/* ============================================================================
   IFX Linux adaptation - Kernel time handling
   ========================================================================= */

/** \addtogroup IFXOS_TIME_LINUX_DRV
@{ */

#ifdef CONFIG_LANTIQ_IFXOS_TIME_ELAPSED_TIME_GET_MS

#if (HZ > 1000)
#error "HZ value higher than 1000 is not supported!"
#endif

#define IFXOS_RESOLUTION_MS (1000L / HZ)
#define IFXOS_JIFFIES_SCALE (ULONG_MAX / IFXOS_RESOLUTION_MS)
#define IFXOS_MSEC_SCALE    (IFXOS_JIFFIES_SCALE * IFXOS_RESOLUTION_MS)
#endif

#ifdef CONFIG_LANTIQ_IFXOS_TIME_SLEEP_US
/**
   LINUX Kernel - Sleep a given time in [us].

\par Implementation
   In case that the system is not calibrated, a calibration will be done.
   The "sleeping" will be performed by looping for a certain amount of loops. Therefore
   no scheduler is involved.

\attention
   The implementation is designed as "busy wait", the scheduler will not be called.

\param
   sleepTime_us   Time to sleep [us]

\remark
   Available in
   - Driver space
*/
IFX_void_t IFXOS_USecSleep(
               IFX_time_t sleepTime_us)
{
   udelay(sleepTime_us);
}
#endif


#ifdef CONFIG_LANTIQ_IFXOS_TIME_SLEEP_MS

/**
   LINUX Kernel - Sleep a given time in [ms].

\attention
   The sleep requires a "sleep wait". "busy wait" implementation will not work.

\par Implementation
   Within the Kernel we use the LINUX scheduler to set the thread into "sleep".

\param
   sleepTime_ms   Time to sleep [ms]

\remark
   Available in
   - Driver space
*/
IFX_void_t IFXOS_MSecSleep(
               IFX_time_t sleepTime_ms)
{
   /* current->state = TASK_INTERRUPTIBLE; */
   set_current_state(TASK_INTERRUPTIBLE);
   schedule_timeout(HZ * (sleepTime_ms) / 1000);

}
#endif

#ifdef CONFIG_LANTIQ_IFXOS_TIME_ELAPSED_TIME_GET_MS
/**
   LINUX Kernel - Get the elapsed time in [ms].

\par Implementation
   Based on the HZ and jiffies defines we calculate the elapsed time since
   startup or based on the given ref-time.

\param
   refTime_ms  Reference time to calculate the elapsed time in [ms].

\return
   Elapsed time in [ms] based on the given reference time

\remark
   Provide refTime_ms = 0 to get the current elapsed time. For measurement provide
   the current time as reference.
*/
IFX_time_t IFXOS_ElapsedTimeMSecGet(
               IFX_time_t refTime_ms)
{
   IFX_time_t currTime_ms, jiffies_mod;

   if (refTime_ms > IFXOS_MSEC_SCALE)
      return 0;

   jiffies_mod = (IFXOS_JIFFIES_SCALE + 1) ?
			(jiffies % (IFXOS_JIFFIES_SCALE + 1)) : jiffies;
   currTime_ms = jiffies_mod * IFXOS_RESOLUTION_MS;

   return (currTime_ms >= refTime_ms) ? (currTime_ms - refTime_ms) :
	    (IFXOS_MSEC_SCALE - refTime_ms + currTime_ms + IFXOS_RESOLUTION_MS);
}
#endif

/** @} */

#ifdef CONFIG_LANTIQ_IFXOS_TIME_SLEEP_US
EXPORT_SYMBOL(IFXOS_USecSleep);
#endif

#ifdef CONFIG_LANTIQ_IFXOS_TIME_SLEEP_MS
EXPORT_SYMBOL(IFXOS_MSecSleep);
#endif

#ifdef CONFIG_LANTIQ_IFXOS_TIME_ELAPSED_TIME_GET_MS
EXPORT_SYMBOL(IFXOS_ElapsedTimeMSecGet);
#endif

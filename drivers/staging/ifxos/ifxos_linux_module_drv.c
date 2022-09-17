/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/* ============================================================================
   Description : IFX OS adaptation, Linux Kernel copy form and to user
   ========================================================================= */

/** \file
   This file contains the IFXOS Layer implementation for LINUX Kernel
   Basic module setup and module load.
*/

/* ============================================================================
   IFX Linux adaptation - Global Includes - Kernel
   ========================================================================= */

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/version.h>
#include <linux/init.h>

#include "ifxos_debug.h"
#include "ifxos_version.h"

/* ============================================================================
   IFX Linux adaptation - module handling
   ========================================================================= */

int __init IFXOS_ModuleInit (void);
void __exit IFXOS_ModuleExit (void);


/* install parameter debug_level: LOW (1), NORMAL (2), HIGH (3), OFF (4) */
IFX_uint8_t debug_level = IFXOS_PRN_LEVEL_HIGH;

module_param(debug_level, byte, 0);
MODULE_PARM_DESC(debug_level, "set to get more (1) or fewer (4) debug outputs");

const char IFXOS_WHATVERSION[] = IFXOS_WHAT_STR;

/** \addtogroup IFXOS_IF_LINUX_DRV
@{ */

/**
   Initialize the module

\return
   Error code or 0 on success
\remark
   Called by the kernel.
*/
int __init IFXOS_ModuleInit (void)
{
   printk(KERN_INFO "%s (c) Copyright 2009, Lantiq Deutschland GmbH" IFXOS_CRLF, &IFXOS_WHATVERSION[4]);
   printk(KERN_INFO "%s (c) Copyright 2016 - 2019, Intel Corporation" IFXOS_CRLF, &IFXOS_WHATVERSION[4]);
   printk(KERN_INFO "%s (c) Copyright 2020, MaxLinear, Inc." IFXOS_CRLF, &IFXOS_WHATVERSION[4]);
   printk(KERN_INFO "%s - Ported to Linux staging platform by Ernesto Castellotti." IFXOS_CRLF, &IFXOS_WHATVERSION[4]);

   IFXOS_PRN_USR_LEVEL_SET(IFXOS, debug_level);
   IFXOS_PRN_INT_LEVEL_SET(IFXOS, debug_level);

   /*
      If required do the basic init here
   */

   return 0;
}

/**
   Clean up the module if unloaded.

   \remark
   Called by the kernel.
*/
void __exit IFXOS_ModuleExit (void)
{
   /*
      If required do the basic cleanup here
   */

   return;
}


/** @} */


/*
   register module init and exit
*/
module_init (IFXOS_ModuleInit);
module_exit (IFXOS_ModuleExit);

EXPORT_SYMBOL(IFXOS_PRN_USR_MODULE_NAME(IFXOS));
EXPORT_SYMBOL(IFXOS_PRN_INT_MODULE_NAME(IFXOS));


/****************************************************************************/

MODULE_AUTHOR("www.lantiq.com");
MODULE_DESCRIPTION("IFX - OS abstraction layer");
MODULE_LICENSE("Dual BSD/GPL");


/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/* ============================================================================
   Description : IFX Linux adaptation - memory allocation (Kernel Space)
   ========================================================================= */

/** \file
   This file contains the IFXOS Layer implementation for LINUX Kernel 
   Memory Allocation.
*/

/* ============================================================================
   IFX Linux adaptation - Global Includes - Kernel
   ========================================================================= */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "ifx_types.h"
#include "ifxos_debug.h"

#ifdef CONFIG_LANTIQ_IFXOS_MEMORY_LOCAL_CHECK
#include <linux/hardirq.h>
#endif


/* ============================================================================
   IFX Linux adaptation - Kernel memory handling, malloc
   ========================================================================= */

/** \addtogroup IFXOS_MEM_ALLOC_LINUX_DRV
@{ */

#ifdef CONFIG_LANTIQ_IFXOS_BLOCK_ALLOC

/**
   LINUX Kernel - Allocate a continuous memory block of the given size [byte]
\par Implementation
   - Allocates a continuous memory block with the kernel function "kmalloc"
   - The option "GFP_KERNEL" is used for normal kernel allocation (may sleep)
   - This implementation is not allowed on interrupt level (may sleep)

\param
   memSize_byte   Size of the requested memory block [byte]

\return
   IFX_NULL in case of error, else
   pointer to the allocated memory block.
*/
IFX_void_t *IFXOS_BlockAlloc(
               IFX_size_t memSize_byte)
{
   IFX_void_t *pMemBlock = IFX_NULL;

#ifdef CONFIG_LANTIQ_IFXOS_MEMORY_LOCAL_CHECK
   if (in_interrupt())
   {
      IFXOS_PRN_INT_ERR_NL( IFXOS, IFXOS_PRN_LEVEL_ERR,
         ("ERROR - kmalloc call within interrupt" IFXOS_CRLF));
      return (pMemBlock);
   }
#endif

   if(memSize_byte)
      pMemBlock = (IFX_void_t *)kmalloc((unsigned int)memSize_byte, GFP_KERNEL);

   return (pMemBlock);
}

/**
   LINUX Kernel - Free the given memory block.
\par Implementation
   Free a continuous memory block with the kernel function "kfree"

\param
   pMemBlock   Points to the memory block to free.
*/
IFX_void_t IFXOS_BlockFree(
               IFX_void_t *pMemBlock)
{

   if (pMemBlock)
   {
      kfree(pMemBlock);
   }
   else
   {
      IFXOS_PRN_INT_ERR_NL( IFXOS, IFXOS_PRN_LEVEL_ERR,
         ("WARNING - Cannot free NULL pointer" IFXOS_CRLF));
   }

}
#endif      /* #ifdef CONFIG_LANTIQ_IFXOS_BLOCK_ALLOC */


#ifdef CONFIG_LANTIQ_IFXOS_MEM_ALLOC
/**
   LINUX Kernel - Allocate Memory Space from the OS

\par Implementation
   Allocates a memory block with the kernel function "vmalloc"

\param
   memSize_byte   Size of the requested memory block [byte]

\return
   IFX_NULL in case of error, else
   pointer to the allocated memory block.
*/
IFX_void_t *IFXOS_MemAlloc(
               IFX_size_t memSize_byte)
{
   IFX_void_t *pMemBlock = IFX_NULL;

   if(memSize_byte)
      pMemBlock = (IFX_void_t*)vmalloc((unsigned long)memSize_byte);

   return (pMemBlock);
}

/**
   LINUX Kernel - Free Memory Space
\par Implementation
   Free a memory block with the kernel function "vfree"

\param
   pMemBlock   Points to the memory block to free.
*/
IFX_void_t IFXOS_MemFree(
               IFX_void_t *pMemBlock)
{

   if (pMemBlock)
   {
      vfree(pMemBlock);
   }
   else
   {
      IFXOS_PRN_INT_ERR_NL(IFXOS, IFXOS_PRN_LEVEL_WRN,
         ("WARNING - Cannot vfree NULL pointer" IFXOS_CRLF));
   }

}
#endif      /* #ifdef CONFIG_LANTIQ_IFXOS_MEM_ALLOC */

/** @} */

#ifdef CONFIG_LANTIQ_IFXOS_BLOCK_ALLOC
EXPORT_SYMBOL(IFXOS_BlockAlloc);
EXPORT_SYMBOL(IFXOS_BlockFree);
#endif

#ifdef CONFIG_LANTIQ_IFXOS_MEM_ALLOC
EXPORT_SYMBOL(IFXOS_MemAlloc);
EXPORT_SYMBOL(IFXOS_MemFree);
#endif


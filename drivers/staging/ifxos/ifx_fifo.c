/****************************************************************************

         Copyright (c) 2016 - 2019 Intel Corporation
         Copyright (c) 2011 - 2016 Lantiq Beteiligungs-GmbH & Co. KG

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*****************************************************************************/

/** \file
      Initialize with Fifo_Init with previously allocated memory.
      The functions Fifo_writeElement and Fifo_readElement return a pointer
      to the next element to be written or read respectively.
      If empty Fifo_readElement returns IFX_NULL. If full Fifo_writeElement
      returns IFX_NULL.
*/


/* ============================= */
/* Includes                      */
/* ============================= */

#include "ifx_types.h"
#include "ifxos_debug.h"
#include <linux/kernel.h>
#include <linux/module.h>

/**
   IFX_FIFO data structure
*/
typedef struct
{
   /** start pointer of IFX_FIFO buffer */
   IFX_ulong_t* pStart;
   /** end pointer of IFX_FIFO buffer */
   IFX_ulong_t* pEnd;
   /** read pointer of IFX_FIFO buffer */
   IFX_ulong_t* pRead;
   /** write pointer of IFX_FIFO buffer */
   IFX_ulong_t* pWrite;
   /** element size */
   IFX_ulong_t size;
   /** element count, changed on read and write: */
   IFX_vuint32_t count;
   /** maximum of IFX_FIFO elements (or maximum element size of IFX_VFIFO)*/
   IFX_uint32_t max_size;

   /** points to the internal system object - for debugging */
   IFX_void_t  *pSysObject;
} IFX_FIFO;

typedef IFX_FIFO IFX_VFIFO;

/* ============================= */
/* Local Macros  Definitions    */
/* ============================= */
#define IFX_FIFO_PREFIX   "-->FIFO_Library: "

/* Element size header is 1 long int. */
#define SIZE_HEADER          1
/* Element trailer is 1 long int */
#ifndef SIZE_TRAILER
#  define SIZE_TRAILER       1
#endif
#define SIZE_TRAILER_VALUE   0xDEADBEEF

/** increment IFX_FIFO index */
#define INCREMENT_INDEX(p)       \
{                                \
   if ((p) == pFifo->pEnd)         \
      (p) = pFifo->pStart;         \
   else                          \
      (p) = (p) + pFifo->size; \
}

/** decrement IFX_FIFO index */
#define DECREMENT_INDEX(p)     \
{                              \
   if ((p) == pFifo->pStart)   \
      (p) = pFifo->pEnd;       \
   else                        \
      (p) = (p) - pFifo->size; \
}

#define TO_ULONG_SIZE(esz)    \
      ((esz)/sizeof(IFX_ulong_t) + ((esz)%sizeof(IFX_ulong_t) > 0))

/* ============================= */
/* Global variable definition    */
/* ============================= */
IFXOS_PRN_USR_MODULE_CREATE(FIFO_MODULE, IFXOS_PRN_LEVEL_HIGH);

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   Initializes the fifo structure
   \param pFifo - Pointer to the Fifo structure
   \param pStart - Pointer to the fifo first element (IFX_ulong_t aligned)
   \param pEnd - Pointer to the fifo last element (IFX_ulong_t aligned)
   \param elSizeB - size of each element in bytes (the same for all elements)
   \return
   Always zero, otherwise error
*/
IFX_return_t IFX_Fifo_Init (IFX_FIFO* pFifo, IFX_ulong_t* pStart, IFX_ulong_t* pEnd, IFX_uint32_t elSizeB)
{
   /* Check if pStart and pEnd are word aligned pointers */
   {
      if ((IFX_ulong_t)pStart % sizeof(IFX_ulong_t) != 0)
      {
         IFXOS_PRN_USR_DBG_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_HIGH,
               (IFX_FIFO_PREFIX"ERROR - pStart is not unsigned long aligned (0x%lX)!!" IFXOS_CRLF,
               (IFX_ulong_t)pStart));
         return IFX_ERROR;
      }
      if ((IFX_ulong_t)pEnd % sizeof(IFX_ulong_t) != 0)
      {
         IFXOS_PRN_USR_DBG_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_HIGH,
               (IFX_FIFO_PREFIX"ERROR - pEnd is not unsigned long aligned (0x%lX)!!" IFXOS_CRLF,
               (IFX_ulong_t)pEnd));
         return IFX_ERROR;
      }
   }

   pFifo->pEnd   = pEnd;
   pFifo->pStart = pStart;
   pFifo->pRead  = pStart;
   pFifo->pWrite = pStart;
   pFifo->size   = TO_ULONG_SIZE(elSizeB);
   pFifo->count  = 0;

   if ((pFifo->pEnd - pFifo->pStart) % TO_ULONG_SIZE(elSizeB) != 0)
   {
      /* element size must be a multiple of fifo memory */
      return IFX_ERROR;
   }
   pFifo->max_size = (IFX_uint32_t)(1 + (pFifo->pEnd - pFifo->pStart) / pFifo->size);

   return IFX_SUCCESS;
}

/**
   Clear the fifo
   \param pFifo - Pointer to the Fifo structure
*/
IFX_void_t IFX_Fifo_Clear (IFX_FIFO *pFifo)
{
   pFifo->pRead  = pFifo->pStart;
   pFifo->pWrite = pFifo->pStart;
   pFifo->count = 0;
}

/**
   Get the next element to read from
   \param pFifo - Pointer to the Fifo structure
   \return Returns the element address to read from (IFX_ulong_t aligned),
           or IFX_NULL if no element available or an error occurred
   \remark
   Error occurs if fifo is empty
*/
IFX_ulong_t* IFX_Fifo_readElement (IFX_FIFO *pFifo)
{
   IFX_ulong_t *ret = IFX_NULL;

   if (pFifo->count == 0)
   {
      return IFX_NULL;
   }
   ret = pFifo->pRead;

   INCREMENT_INDEX(pFifo->pRead);
   pFifo->count--;

   return ret;
}

/**
   Delivers empty status
   \param pFifo - Pointer to the Fifo structure
   \return
   Returns TRUE if empty (no data available)
   \remark
   No change on fifo!
*/
IFX_int8_t IFX_Fifo_isEmpty (IFX_FIFO *pFifo)
{
   return (pFifo->count == 0);
}

/**
   Delivers full status
   \param pFifo - Pointer to the Fifo structure
   \return
   TRUE if full (overflow on next write)
   \remark
   No change on fifo!
*/
IFX_int8_t IFX_Fifo_isFull (IFX_FIFO *pFifo)
{
   return (pFifo->count >= pFifo->max_size);
}

/**
   Get the next element to write to
   \param pFifo - Pointer to the Fifo structure
   \return
   Returns the element address (IFX_ulong_t aligned) to write to, or IFX_NULL in case of error
   \remark
   Error occurs if the write pointer reaches the read pointer, meaning
   the fifo if full and would otherwise overwrite the next element.
*/
IFX_ulong_t* IFX_Fifo_writeElement (IFX_FIFO *pFifo)
{
   IFX_ulong_t* ret = IFX_NULL;

   if (IFX_Fifo_isFull(pFifo))
      return IFX_NULL;

   /* get the next entry of the Fifo */
   ret = pFifo->pWrite;
   INCREMENT_INDEX(pFifo->pWrite);
   pFifo->count++;

   return ret;
}

/**
   Returns the last element taken back to the fifo
   \param pFifo - Pointer to the Fifo structure
   \remark
   Makes an undo to a previously Fifo_writeElement
*/
IFX_void_t IFX_Fifo_returnElement (IFX_FIFO *pFifo)
{
   DECREMENT_INDEX(pFifo->pWrite);
   pFifo->count--;
}

/**
   Get the number of stored elements
   \param pFifo - Pointer to the Fifo structure
   \return
   Number of containing elements
*/
IFX_uint32_t IFX_Fifo_getCount(IFX_FIFO *pFifo)
{
   return pFifo->count;
}

/**
   Initializes the variable-sized fifo structure
   \param pFifo - Pointer to the IFX_VFIFO structure
   \param pStart - Pointer to the fifo first element (IFX_ulong_t aligned)
   \param pEnd - Pointer to the first address beyond the last fifo element (IFX_ulong_t aligned)
   \param maxElSize - maximum allowed size of an element in bytes
   \return
   Always zero, otherwise error
*/
IFX_return_t IFX_Var_Fifo_Init (IFX_VFIFO* pFifo, IFX_ulong_t* pStart,
                          IFX_ulong_t* pEnd, IFX_uint32_t maxElSize)
{
   if (pFifo == IFX_NULL)
      return IFX_ERROR;

   /* Check if pStart and pEnd are word aligned pointers */
   {
      if ((IFX_ulong_t)pStart % sizeof(IFX_ulong_t) != 0)
      {
         IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
               (IFX_FIFO_PREFIX"ERROR - pStart is not unsigned long aligned (0x%lX)!!" IFXOS_CRLF,
               (IFX_ulong_t)pStart));
         return IFX_ERROR;
      }
      if ((IFX_ulong_t)pEnd % sizeof(IFX_ulong_t) != 0)
      {
         IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
               (IFX_FIFO_PREFIX"ERROR - pEnd is not unsigned long aligned (0x%lX)!!" IFXOS_CRLF,
               (IFX_ulong_t)pEnd));
         return IFX_ERROR;
      }
   }

   pFifo->pEnd   = pEnd;
   pFifo->pStart = pStart;
   pFifo->pRead  = pStart;
   pFifo->pWrite = pStart;
   pFifo->size   = TO_ULONG_SIZE(maxElSize);
   pFifo->count  = 0;
   pFifo->max_size = 0;

   pFifo->pSysObject = (IFX_void_t*) IFX_NULL;

   return IFX_SUCCESS;
}

/**
   Clears the variable-sized fifo
   \param pFifo - Pointer to the Fifo structure
*/
IFX_void_t IFX_Var_Fifo_Clear (IFX_VFIFO *pFifo)
{
   pFifo->pRead  = pFifo->pStart;
   pFifo->pWrite = pFifo->pStart;
   pFifo->count = 0;
}

/**
   Get the next element to read from variable-sized fifo
   \param pFifo - Pointer to the Fifo structure
   \return Returns the element address (IFX_ulong_t aligned) to read from,
           or IFX_NULL if no element available or an error occurred
   \remark Error occurs if fifo is empty
*/
IFX_ulong_t* IFX_Var_Fifo_readElement (IFX_VFIFO *pFifo, IFX_uint32_t *elSizeB)
{
   IFX_ulong_t *ret = IFX_NULL;
   IFX_ulong_t elSizeUL;

   if (elSizeB)
      *elSizeB = 0;

   if ( (pFifo->pRead < pFifo->pStart) || (pFifo->pRead > pFifo->pEnd))
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - enter var read: pRead: 0x%lX out of range pStart: 0x%lX, pEnd: 0x%lX, !!" IFXOS_CRLF,
            (IFX_ulong_t)pFifo->pRead, (IFX_ulong_t)pFifo->pStart, (IFX_ulong_t)pFifo->pEnd ));

      return IFX_NULL;
   }

   if (pFifo->count == 0)
   {
      return IFX_NULL;
   }

   if ((pFifo->pRead[0] == (IFX_ulong_t)~0) ||
       ((pFifo->pEnd - pFifo->pRead) <= (SIZE_HEADER + SIZE_TRAILER)))
   {
      pFifo->pRead = pFifo->pStart;
   }

   elSizeUL = TO_ULONG_SIZE(pFifo->pRead[0]);

   if ( (pFifo->pRead + (SIZE_HEADER + SIZE_TRAILER) + elSizeUL) > pFifo->pEnd)
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - var read: overflow, pRead: 0x%lX, pEnd: 0x%lX, elSize: 0x%08lX (+ 0x%X) !!" IFXOS_CRLF,
            (IFX_ulong_t)pFifo->pRead, (IFX_ulong_t)pFifo->pEnd, elSizeUL, (SIZE_HEADER + SIZE_TRAILER)));

      return IFX_NULL;
   }

#if (SIZE_TRAILER == 1)
   if (pFifo->pRead[elSizeUL + SIZE_HEADER] != SIZE_TRAILER_VALUE)
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - var read: overwrite occurred: 0x%lX, pEnd: 0x%lX, elSize: 0x%08lX (+ 0x%X) !!" IFXOS_CRLF,
            (IFX_ulong_t)pFifo->pRead, (IFX_ulong_t)pFifo->pEnd, elSizeUL, (SIZE_HEADER + SIZE_TRAILER)));

      pFifo->pRead[elSizeUL + SIZE_HEADER] = (IFX_ulong_t)~0;
   }
#endif

   if (elSizeB)
      *elSizeB = (IFX_uint32_t)(pFifo->pRead[0]);

   ret = pFifo->pRead + SIZE_HEADER;
   pFifo->pRead += (SIZE_HEADER + SIZE_TRAILER) + elSizeUL;

   pFifo->count--;

   if ( (pFifo->pRead < pFifo->pStart) || (pFifo->pRead > pFifo->pEnd))
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - leave var read: pRead: 0x%lX out of range pStart: 0x%lX, pEnd: 0x%lX, !!" IFXOS_CRLF,
            (IFX_ulong_t)pFifo->pRead, (IFX_ulong_t)pFifo->pStart, (IFX_ulong_t)pFifo->pEnd ));

      return IFX_NULL;
   }

   return ret;
}

/**
   Peek the next element to read from variable-sized fifo
   \remark To free the element use the corresponding read function.

   \param pFifo - Pointer to the Fifo structure
   \return Returns the element address (IFX_ulong_t aligned) to read from,
           or IFX_NULL if no element available or an error occurred
   \remark Error occurs if fifo is empty
*/
IFX_ulong_t* IFX_Var_Fifo_peekElement (IFX_VFIFO *pFifo, IFX_uint32_t *elSizeB)
{
   IFX_ulong_t *ret = IFX_NULL, *pPeekRead = pFifo->pRead;
   IFX_ulong_t elSizeUL;

   if (elSizeB)
      *elSizeB = 0;

   if ( (pFifo->pRead < pFifo->pStart) || (pFifo->pRead > pFifo->pEnd))
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - enter var peek: pRead: 0x%lX out of range pStart: 0x%lX, pEnd: 0x%lX, !!" IFXOS_CRLF,
            (IFX_ulong_t)pFifo->pRead, (IFX_ulong_t)pFifo->pStart, (IFX_ulong_t)pFifo->pEnd ));

      return IFX_NULL;
   }

   if (pFifo->count == 0)
   {
      return IFX_NULL;
   }

   if ((pPeekRead[0] == (IFX_ulong_t)~0) ||
       ((pFifo->pEnd - pPeekRead) <= (SIZE_HEADER + SIZE_TRAILER)))
   {
      pPeekRead = pFifo->pStart;
   }

   elSizeUL = TO_ULONG_SIZE(pPeekRead[0]);
   if (elSizeUL > pFifo->size)
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - var peek: incorrect size, "
            "pRead: 0x%lX, pEnd: 0x%lX, FifoSize:  0x%08lX, elSize: 0x%08lX (+ 0x%X) !!" IFXOS_CRLF,
            (IFX_ulong_t)pPeekRead, (IFX_ulong_t)pFifo->pEnd, pFifo->size,
            elSizeUL, (SIZE_HEADER + SIZE_TRAILER)));

      return IFX_NULL;
   }

   if ( (pPeekRead + (SIZE_HEADER + SIZE_TRAILER) + elSizeUL) > pFifo->pEnd)
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - var peek: overflow, pRead: 0x%lX, pEnd: 0x%lX, elSize: 0x%08lX (+ 0x%X) !!" IFXOS_CRLF,
            (IFX_ulong_t)pPeekRead, (IFX_ulong_t)pFifo->pEnd, elSizeUL, (SIZE_HEADER + SIZE_TRAILER)));

      return IFX_NULL;
   }

#if (SIZE_TRAILER == 1)
   if (pPeekRead[elSizeUL + SIZE_HEADER] != SIZE_TRAILER_VALUE)
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - var peek: overwrite occurred: 0x%lX, pEnd: 0x%lX, elSize: 0x%08lX (+ 0x%X) !!" IFXOS_CRLF,
            (IFX_ulong_t)pPeekRead, (IFX_ulong_t)pFifo->pEnd, elSizeUL, (SIZE_HEADER + SIZE_TRAILER)));

      pPeekRead[elSizeUL + SIZE_HEADER] = (IFX_ulong_t)~0;
   }
#endif

   if (elSizeB)
      *elSizeB = (IFX_uint32_t)(pPeekRead[0]);

   ret = pPeekRead + SIZE_HEADER;

   if ( (pFifo->pRead < pFifo->pStart) || (pFifo->pRead > pFifo->pEnd))
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - leave var peek: pRead: 0x%lX out of range pStart: 0x%lX, pEnd: 0x%lX, !!" IFXOS_CRLF,
            (IFX_ulong_t)pFifo->pRead, (IFX_ulong_t)pFifo->pStart, (IFX_ulong_t)pFifo->pEnd ));

      return IFX_NULL;
   }

   return ret;
}


/**
   Delivers empty status
   \param pFifo - Pointer to the IFX_VFIFO structure
   \return Returns TRUE if empty (no data available)
   \remark No change on fifo!
*/
IFX_int8_t IFX_Var_Fifo_isEmpty (IFX_VFIFO *pFifo)
{
   return (pFifo->count == 0);
}

/**
 * Returns size of free room in the variable-sized IFX_FIFO.
 *    \param pFifo - Pointer to the Fifo structure
 *    \return
 *    The size of free room in the IFX_FIFO, in IFX_ulong_t integers.
 */
IFX_uint32_t IFX_Var_Fifo_getRoom (IFX_VFIFO *pFifo)
{
   IFX_long_t diff = pFifo->pWrite - pFifo->pRead;
   IFX_long_t headRoom, tailRoom;

   if (diff == 0)
   {
      if (pFifo->count)
         return 0;

      return (IFX_uint32_t)(pFifo->pEnd - pFifo->pStart - (SIZE_HEADER + SIZE_TRAILER));
   }
   if (diff > 0)
   {
      tailRoom = pFifo->pEnd  - pFifo->pWrite - (SIZE_HEADER + SIZE_TRAILER);
      headRoom = pFifo->pRead - pFifo->pStart - (SIZE_HEADER + SIZE_TRAILER);

      return (headRoom > tailRoom) ? (IFX_uint32_t)headRoom : (IFX_uint32_t)tailRoom;
   }
   if ((-diff) <= (SIZE_HEADER + SIZE_TRAILER) )
      return 0;

   return (IFX_uint32_t)((-diff) - (SIZE_HEADER + SIZE_TRAILER) );
}

/**
   Get the next element to write to
   \param pFifo - Pointer to the Fifo structure
   \param elSizeB - Size of a new element to be written in bytes
   \return
   Returns the element address to write to (IFX_ulong_t aligned), or IFX_NULL in case of error
   \remark
   Error occurs if the write pointer reaches the read pointer, meaning
   the fifo if full and would otherwise overwrite the next element.
*/
IFX_ulong_t* IFX_Var_Fifo_writeElement (IFX_VFIFO *pFifo, IFX_uint32_t elSizeB)
{
   IFX_ulong_t* ret = IFX_NULL;
   IFX_ulong_t elSizeUL = TO_ULONG_SIZE (elSizeB);

   if ( (pFifo->pWrite < pFifo->pStart) || (pFifo->pWrite >= pFifo->pEnd))
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - enter var write: pWrite: 0x%lX out of range pStart: 0x%lX, pEnd: 0x%lX, !!" IFXOS_CRLF,
            (IFX_ulong_t)pFifo->pWrite, (IFX_ulong_t)pFifo->pStart, (IFX_ulong_t)pFifo->pEnd ));

      return IFX_NULL;
   }

   if (elSizeUL > IFX_Var_Fifo_getRoom (pFifo) || elSizeUL > pFifo->size)
   {
      return IFX_NULL;
   }
   if (pFifo->pWrite >= pFifo->pRead &&
      (elSizeUL + SIZE_HEADER + SIZE_TRAILER) > (IFX_ulong_t)(pFifo->pEnd - pFifo->pWrite))
   {
      /* There is not enough free space at the end of buffer (pWrite--XXX--pEnd) */
      /* Check free space at the beginning of the buffer (pStart--XXX--pRead) */
      if ((IFX_long_t)(elSizeUL + SIZE_HEADER + SIZE_TRAILER) > pFifo->pRead - pFifo->pStart)
      {
          /* There is not enough free space at the beginning of the buffer (pStart--XXX--pRead) also */
          /* Return 0 */
          return IFX_NULL;
      }

      if ((pFifo->pEnd - pFifo->pWrite) >= SIZE_HEADER)
      {
         pFifo->pWrite[0] = (IFX_ulong_t)~0;
      }
      pFifo->pWrite = pFifo->pStart;
   }

   pFifo->pWrite[0] = elSizeB;
#if (SIZE_TRAILER == 1)
   pFifo->pWrite[elSizeUL + SIZE_HEADER] = SIZE_TRAILER_VALUE;
#endif

   ret = pFifo->pWrite + SIZE_HEADER;

   pFifo->pWrite   += elSizeUL + (SIZE_HEADER + SIZE_TRAILER);
   if (pFifo->pWrite == pFifo->pEnd)
      pFifo->pWrite = pFifo->pStart;


   if ( (pFifo->pWrite < pFifo->pStart) || (pFifo->pWrite >= pFifo->pEnd))
   {
      IFXOS_PRN_USR_ERR_NL(FIFO_MODULE, IFXOS_PRN_LEVEL_ERR,
            (IFX_FIFO_PREFIX "ERROR - leave var write: pWrite: 0x%lX out of range pStart: 0x%lX, pEnd: 0x%lX, !!" IFXOS_CRLF,
            (IFX_ulong_t)pFifo->pWrite, (IFX_ulong_t)pFifo->pStart, (IFX_ulong_t)pFifo->pEnd ));

      return IFX_NULL;
   }

   pFifo->count++;
   return ret;
}

/**
   Delivers full status
   The IFX_FIFO is full if there is not enough space
   for one element of maximum size defined in Var_Fifo_Init() call.
   \param pFifo - Pointer to the Fifo structure
   \return
   TRUE if full (overflow on next write)
   \remark
   No change on fifo!
*/
IFX_int8_t IFX_Var_Fifo_isFull (IFX_VFIFO *pFifo)
{
   return (IFX_Var_Fifo_getRoom(pFifo) < pFifo->size);
}

/**
   Get the number of stored elements
   \param pFifo - Pointer to the Fifo structure
   \return
   Number of containing elements
*/
IFX_uint32_t IFX_Var_Fifo_getCount(IFX_VFIFO *pFifo)
{
   return pFifo->count;
}

EXPORT_SYMBOL(IFX_Fifo_Init);
EXPORT_SYMBOL(IFX_Fifo_Clear);
EXPORT_SYMBOL(IFX_Fifo_readElement);
EXPORT_SYMBOL(IFX_Fifo_isEmpty);
EXPORT_SYMBOL(IFX_Fifo_writeElement);
EXPORT_SYMBOL(IFX_Fifo_isFull);
EXPORT_SYMBOL(IFX_Fifo_getCount);
EXPORT_SYMBOL(IFX_Var_Fifo_Init);
EXPORT_SYMBOL(IFX_Var_Fifo_Clear);
EXPORT_SYMBOL(IFX_Var_Fifo_readElement);
EXPORT_SYMBOL(IFX_Var_Fifo_peekElement);
EXPORT_SYMBOL(IFX_Var_Fifo_isEmpty);
EXPORT_SYMBOL(IFX_Var_Fifo_getRoom);
EXPORT_SYMBOL(IFX_Var_Fifo_writeElement);
EXPORT_SYMBOL(IFX_Var_Fifo_isFull);
EXPORT_SYMBOL(IFX_Var_Fifo_getCount);

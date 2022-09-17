/****************************************************************************

         Copyright (c) 2016 - 2019 Intel Corporation
         Copyright (c) 2011 - 2016 Lantiq Beteiligungs-GmbH & Co. KG

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file
   This file contains the IFXOS Layer implementation for LINUX Kernel
   Socket.
*/

/* ============================================================================
   IFX Linux adaptation - Global Includes - Kernel
   ========================================================================= */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/in.h>
#include <linux/net.h>
#include <linux/fs.h>
#include <linux/inet.h>
#include <asm/uaccess.h>
#ifdef CONFIG_LANTIQ_IFXOS_SOCKET_IPV6
#include <linux/in6.h>
#endif

#include "ifx_types.h"

/* ============================================================================
   IFX LINUX adaptation - types and defines
   ========================================================================= */
/** Wrap the socket types */
typedef enum
{
   /** For TCP connection*/
   IFXOS_SOC_TYPE_STREAM = SOCK_STREAM,
   /** For UDP connection*/
   IFXOS_SOC_TYPE_DGRAM  = SOCK_DGRAM
} IFXOS_socketType_t;

/** Wrap the socket fd for internal kernel handling */
typedef struct socket*        IFXOS_socket_t;

/** Wrap the socket address type */
typedef struct sockaddr_in    IFXOS_sockAddr_t;

/** Wrap the fd_set for socket handling */
typedef fd_set                IFXOS_socFd_set_t;

/** Wrap max fd for sockets */
typedef int                   IFXOS_socFd_t;
   
/** Wrap IP V6 address family define AF_INET6 */
#define IFXOS_SOC_AF_INET6	AF_INET6

/** Wrap the IP V6 socket address type */
typedef struct sockaddr_in6	IFXOS_sockAddr6_t;

/** Set the IP address in the 'IFXOS_sockAddr_t' - structure*/
#define IFXOS_SOC_ADDR_SET(a, ip)            (((IFXOS_sockAddr_t*)a)->sin_addr.s_addr = ip)

/* ============================================================================
   IFX Linux adaptation - Kernel Space, Socket
   ========================================================================= */

/** \addtogroup IFXOS_SOCKET_LINUX
@{ */

/**
   LINUX Kernel - This function init and setup the socket feature on the system.

\par Implementation
   - Nothing under LINUX Kernel.

\remark
   This function is available for compatibility reasons. On systems where no
   seperate setup is required the function will be empty.

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR   if operation failed
*/
IFX_int_t IFXOS_SocketInit(void)
{
   return IFX_SUCCESS;
}


/**
   LINUX Kernel - This function cleanup the socket feature on the system.

\par Implementation
   - Nothing under LINUX Application.

\remark
   This function is available for compatibility reasons. On systems where no
   seperate setup is required the function will be empty.

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR   if operation failed
*/
IFX_int_t IFXOS_SocketCleanup(void)
{
   return IFX_SUCCESS;
}



/**
   LINUX Kernel - This function creates a TCP/IP, UDP/IP or raw socket.

\par Implementation
   - Create a PF_INET socket, no specified protocol.

\param
   socType     specifies the type of the socket
               - IFXOS_SOC_TYPE_STREAM: TCP/IP socket
               - IFXOS_SOC_TYPE_DGRAM:  UDP/IP socket
\param
   pSocketFd   specifies the pointer where the value of the socket should be
               set. Value will be greater or equal zero

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR   if operation failed
*/
IFX_int_t IFXOS_SocketCreate(
                  IFXOS_socketType_t socType,
                  IFXOS_socket_t     *pSocketFd)
{
   /* arg3 = 0: do not specifiy the protocol */
   if (sock_create(PF_INET, socType, 0, pSocketFd) == -1)
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   This function closes specified socket.

\par Implementation
   - Close the given socket

\param
   socketFd     socket to close

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR   if operation failed
*/
IFX_int_t IFXOS_SocketClose(
                  IFXOS_socket_t socketFd)
{
   sock_release(socketFd);
   return IFX_SUCCESS;
}

/**
   LINUX Kernel - Receives data from a datagramm socket.

\attention
   Still not implemented

\par Implementation
   -  via "recv_from"

\param
   socFd         specifies the socket. Value has to be greater or equal zero
\param
   pBuffer       specifies the pointer to a buffer where the data will be copied
\param
   bufSize_byte  specifies the size in byte of the buffer 'pBuffer'
\param
   pSocAddr      specifies a pointer to the IFXOS_sockAddr_t structure

\return
   Returns the number of received bytes. Returns a negative value if an error
   occured
*/
IFX_int_t IFXOS_SocketRecvFrom(
   IFXOS_socket_t socFd,
   IFX_char_t *pBuffer,
   IFX_int_t bufSize_byte,
   IFXOS_sockAddr_t *pSocAddr)
{
   struct msghdr msg;
   struct iovec iov;
   mm_segment_t old_fs;
   int ret;

   iov.iov_base = pBuffer;
   iov.iov_len = (unsigned int) bufSize_byte;

   msg.msg_name = (void *) pSocAddr;
   msg.msg_namelen = sizeof(IFXOS_sockAddr_t);
   msg.msg_control = IFX_NULL;
   msg.msg_controllen = 0;
   msg.msg_flags = 0;
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;

   /* Modify address limitation which is used if user space is calling
      kernel space, otherwise sock_recvmsg() will fail.*/
   old_fs = get_fs();
   set_fs(KERNEL_DS);

   ret = sock_recvmsg (socFd, &msg, bufSize_byte, 0);
   set_fs(old_fs);

   return ret;
}

/**
   LINUX Kernel - Sends data to UDP socket.

\par Implementation
   -  via "sock_sendmsg"

\param
   socFd          specifies the socket. Value has to be greater or equal zero
\param
   pBuffer        specifies the pointer to a buffer where the data will be copied
\param
   bufSize_byte   specifies the size in byte of the buffer 'pBuffer'
\param
   pSocAddr    specifies a pointer to the IFXOS_sockAddr_t structure

\return
   Returns the number of sent bytes. Returns a negative value if an error
   occured
*/
IFX_int_t IFXOS_SocketSendTo(
                  IFXOS_socket_t socFd,
                  IFX_char_t     *pBuffer,
                  IFX_int_t      bufSize_byte,
                  IFXOS_sockAddr_t  *pSocAddr)
{
   struct msghdr msg;
   struct iovec iov;
   mm_segment_t old_fs;
   int ret;

   iov.iov_base = pBuffer;
   iov.iov_len = (unsigned int) bufSize_byte;

   msg.msg_name = (void *) pSocAddr;
   msg.msg_namelen = sizeof(IFXOS_sockAddr_t);
   msg.msg_control = IFX_NULL;
   msg.msg_controllen = 0;
   msg.msg_flags = MSG_DONTWAIT;
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;

   /* Modify address limitation which is used if user space is calling
      kernel space, otherwise sock_sendmsg() will fail.*/
   old_fs = get_fs();
   set_fs(KERNEL_DS);

   ret = sock_sendmsg(socFd, &msg, bufSize_byte);
   set_fs(old_fs);

   return ret;
}

/**
   LINUX Kernel - Assignes a local address to a TCP/IP, UDP/IP or raw socket.

\par Implementation
   -  via "bind"

\param
   socFd       specifies the socket should be bind to the address
               Value has to be greater or equal zero
\param
   pSocAddr    specifies a pointer to the IFXOS_sockAddr_t structure

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR if operation failed
*/
IFX_int_t IFXOS_SocketBind(
                  IFXOS_socket_t    socFd,
                  IFXOS_sockAddr_t  *pSocAddr)
{
   IFX_int_t ret;

   ret = (socFd->ops->bind(socFd, (struct sockaddr*) pSocAddr, sizeof(IFXOS_sockAddr_t)));

   if (ret != 0)
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   LINUX Application - This function converts a dotted decimal address to a network address.

\par Implementation
   -  convert the given ASCII address via "inet_aton".

\param
   pBufAddr    contains the ASCII address string. Must have size DSL_ADDR_LEN
\param
   pSocAddr    specifies a pointer to the DSL internal address structure

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR if operation failed
*/
IFX_int_t IFXOS_SocketAton(
                  const IFX_char_t  *pBufAddr,
                  IFXOS_sockAddr_t  *pSocAddr)
{
   IFXOS_SOC_ADDR_SET(pSocAddr, in_aton(pBufAddr));
   return IFX_SUCCESS;
}

/**
   LINUX Kernel - This function creates a TCP/IP, UDP/IP or raw IPv6 socket.

\par Implementation
   - Create a PF_INET6 socket, no specified protocol.

\param
   socType    specifies the type of the socket
              - IFXOS_SOC_TYPE_STREAM: TCP/IP socket
              - IFXOS_SOC_TYPE_DGRAM:  UDP/IP socket
\param
   pSocketFd  specifies the pointer where the value of the socket should be
              set. Value will be greater or equal zero

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR   if operation failed
*/
IFX_int_t IFXOS_SocketCreateIpV6(
   IFXOS_socketType_t socType,
   IFXOS_socket_t *pSocketFd)
{
#ifdef CONFIG_LANTIQ_IFXOS_SOCKET_IPV6
   /* arg3 = 0: do not specify the protocol */
   if (sock_create(PF_INET6, socType, 0, (struct socket **)pSocketFd) == -1)
      {return IFX_ERROR;}

   return IFX_SUCCESS;
#else
   return IFX_ERROR;
#endif
}

/**
   LINUX Kernel - Receives data from a UDP IP V6 socket.

\attention
   Still not implemented

\par Implementation
   -  via "recv_from"

\param
   socFd         specifies the socket. Value has to be greater or equal zero
\param
   pBuffer       specifies the pointer to a buffer where the data will be copied
\param
   bufSize_byte  specifies the size in byte of the buffer 'pBuffer'
\param
   pSocAddr6     specifies a pointer to the IFXOS_sockAddr6_t structure

\return
   Returns the number of received bytes. Returns a negative value if an error
   occurred
*/
IFX_int_t IFXOS_SocketRecvFromIpV6(
   IFXOS_socket_t socFd,
   IFX_char_t *pBuffer,
   IFX_int_t bufSize_byte,
   IFXOS_sockAddr6_t *pSocAddr6)
{
#ifdef CONFIG_LANTIQ_IFXOS_SOCKET_IPV6
   struct msghdr msg;
   struct iovec iov;
   mm_segment_t old_fs;
   int ret;

   iov.iov_base = pBuffer;
   iov.iov_len = (unsigned int) bufSize_byte;

   msg.msg_name = (void *) pSocAddr6;
   msg.msg_namelen = sizeof(IFXOS_sockAddr6_t);
   msg.msg_control = IFX_NULL;
   msg.msg_controllen = 0;
   msg.msg_flags = 0;
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;

   /* Modify address limitation which is used if user space is calling
      kernel space, otherwise sock_recvmsg() will fail.*/
   old_fs = get_fs();
   set_fs(KERNEL_DS);

   ret = sock_recvmsg ((struct socket *) socFd, &msg, bufSize_byte, 0);
   set_fs(old_fs);

   return ret;
#else
   return IFX_ERROR;
#endif
}

/**
   LINUX Kernel - Sends data to UDP IP V6 socket.

\par Implementation
   -  via "sock_sendmsg"

\param
   socFd        specifies the socket. Value has to be greater or equal zero
\param
   pBuffer      specifies the pointer to a buffer where the data will be copied
\param
   bufSize_byte specifies the size in byte of the buffer 'pBuffer'
\param
   pSocAddr6     specifies a pointer to the IFXOS_sockAddr_t structure

\return
   Returns the number of sent bytes. Returns a negative value if an error
   occurred
*/
IFX_int_t IFXOS_SocketSendToIpV6(
   IFXOS_socket_t socFd,
   IFX_char_t *pBuffer,
   IFX_int_t bufSize_byte,
   IFXOS_sockAddr6_t *pSocAddr6)
   {
#ifdef CONFIG_LANTIQ_IFXOS_SOCKET_IPV6
   struct msghdr msg;
   struct iovec iov;
   mm_segment_t old_fs;
   int ret;

   iov.iov_base = pBuffer;
   iov.iov_len = (unsigned int) bufSize_byte;

   msg.msg_name = (void *) pSocAddr6;
   msg.msg_namelen = sizeof(IFXOS_sockAddr6_t);
   msg.msg_control = IFX_NULL;
   msg.msg_controllen = 0;
   msg.msg_flags = MSG_DONTWAIT;
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;

   /* Modify address limitation which is used if user space is calling
   kernel space, otherwise sock_sendmsg() will fail.*/
   old_fs = get_fs();
   set_fs(KERNEL_DS);
   ret = sock_sendmsg((struct socket *) socFd, &msg, bufSize_byte);
   set_fs(old_fs);

   return ret;
#else
   return IFX_ERROR;
#endif
}

/**
   LINUX Kernel - Assigns a local address to a TCP/IPv6, UDP/IPv6 or raw socket.

\par Implementation
   -  via "bind"

\param
   socFd     specifies the socket should be bind to the address
             Value has to be greater or equal zero
\param
   pSocAddr6  specifies a pointer to the IFXOS_sockAddr6_t structure

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR if operation failed
*/
IFX_int_t IFXOS_SocketBindIpV6(
   IFXOS_socket_t socFd,
   IFXOS_sockAddr6_t *pSocAddr6)
{
#ifdef CONFIG_LANTIQ_IFXOS_SOCKET_IPV6
   IFX_int_t ret;

   ret = ((struct socket *)socFd)->ops->bind(
         (struct socket *)socFd,
         (struct sockaddr*) pSocAddr6,
         sizeof(IFXOS_sockAddr6_t));
   if (ret != 0)
      {return IFX_ERROR;}

   return IFX_SUCCESS;
#else
   return IFX_ERROR;
#endif
}

/** @} */

EXPORT_SYMBOL(IFXOS_SocketInit);
EXPORT_SYMBOL(IFXOS_SocketCleanup);
EXPORT_SYMBOL(IFXOS_SocketCreate);
EXPORT_SYMBOL(IFXOS_SocketClose);
EXPORT_SYMBOL(IFXOS_SocketSendTo);
EXPORT_SYMBOL(IFXOS_SocketRecvFrom);
EXPORT_SYMBOL(IFXOS_SocketBind);
EXPORT_SYMBOL(IFXOS_SocketAton);
#ifdef CONFIG_LANTIQ_IFXOS_SOCKET_IPV6
EXPORT_SYMBOL(IFXOS_SocketCreateIpV6);
EXPORT_SYMBOL(IFXOS_SocketSendToIpV6);
EXPORT_SYMBOL(IFXOS_SocketRecvFromIpV6);
EXPORT_SYMBOL(IFXOS_SocketBindIpV6);
#endif

/******************************************************************************
 * Copyright 2019 ETC Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************
 * This file is a part of lwpa. For more information, go to:
 * https://github.com/ETCLabs/lwpa
 ******************************************************************************/

#include "lwpa/socket.h"
#include "lwpa/private/socket.h"

#include <string.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include "os_error.h"

/*************************** Private constants *******************************/

#define POLL_CONTEXT_ARR_CHUNK_SIZE 10

/****************************** Private types ********************************/

/* A struct to track sockets being polled by the lwpa_poll() API */
typedef struct LwpaPollSocket
{
  // 'sock' must always remain as the first member in the struct to facilitate an LwpaRbTree lookup
  // shortcut
  lwpa_socket_t sock;
  lwpa_poll_events_t events;
  void* user_data;
} LwpaPollSocket;

/***************************** Private macros ********************************/

#define LWPA_FD_ZERO(setptr) \
  FD_ZERO(&(setptr)->set);   \
  (setptr)->count = 0

#define LWPA_FD_SET(sock, setptr) \
  FD_SET(sock, &(setptr)->set);   \
  (setptr)->count++

#define LWPA_FD_CLEAR(sock, setptr) \
  FD_CLR(sock, &(setptr)->set);     \
  (setptr)->count--

#define LWPA_FD_ISSET(sock, setptr) FD_ISSET(sock, &(setptr)->set)

/**************************** Private variables ******************************/

/* clang-format off */

#define LWPA_NUM_SHUT 3
static const int shutmap[LWPA_NUM_SHUT] =
{
  SD_RECEIVE,
  SD_SEND,
  SD_BOTH
};

#define LWPA_NUM_AF 3
static const int sfmap[LWPA_NUM_AF] =
{
  AF_UNSPEC,
  AF_INET,
  AF_INET6
};

#define LWPA_NUM_TYPE 2
static const int stmap[LWPA_NUM_TYPE] =
{
  SOCK_STREAM,
  SOCK_DGRAM
};

#define LWPA_NUM_AIF 8
static const int aiflagmap[LWPA_NUM_AIF] =
{
  0,
  AI_PASSIVE,
  AI_CANONNAME,
  AI_PASSIVE | AI_CANONNAME,
  AI_NUMERICHOST,
  AI_PASSIVE | AI_NUMERICHOST,
  AI_CANONNAME | AI_NUMERICHOST,
  AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST
};

static const int aifammap[LWPA_NUM_AF] =
{
  AF_UNSPEC,
  AF_INET,
  AF_INET6
};

#define LWPA_NUM_IPPROTO 6
static const int aiprotmap[LWPA_NUM_IPPROTO] =
{
  0,
  IPPROTO_IP,
  IPPROTO_ICMPV6,
  IPPROTO_IPV6,
  IPPROTO_TCP,
  IPPROTO_UDP
};

/* clang-format on */

/*********************** Private function prototypes *************************/

static int setsockopt_socket(lwpa_socket_t id, int option_name, const void* option_value, size_t option_len);
static int setsockopt_ip(lwpa_socket_t id, int option_name, const void* option_value, size_t option_len);
static int setsockopt_ip6(lwpa_socket_t id, int option_name, const void* option_value, size_t option_len);

// Helper functions for the lwpa_poll API
static void set_in_fd_sets(LwpaPollContext* context, const LwpaPollSocket* sock);
static void clear_in_fd_sets(LwpaPollContext* context, const LwpaPollSocket* sock);
static lwpa_error_t handle_select_result(LwpaPollContext* context, LwpaPollEvent* event, const LwpaPollFdSet* readfds,
                                         const LwpaPollFdSet* writefds, const LwpaPollFdSet* exceptfds);

static int poll_socket_compare(const LwpaRbTree* tree, const LwpaRbNode* node_a, const LwpaRbNode* node_b);
static LwpaRbNode* poll_socket_alloc();
static void poll_socket_free(LwpaRbNode* node);

/*************************** Function definitions ****************************/

#if !defined(LWPA_BUILDING_MOCK_LIB)

lwpa_error_t lwpa_socket_init()
{
  WSADATA wsdata;
  WORD wsver = MAKEWORD(2, 2);
  int startup_res = WSAStartup(wsver, &wsdata);
  if (startup_res != 0)
  {
    return err_winsock_to_lwpa(startup_res);
  }
  return kLwpaErrOk;
}

void lwpa_socket_deinit()
{
  WSACleanup();
}

lwpa_error_t lwpa_accept(lwpa_socket_t id, LwpaSockaddr* address, lwpa_socket_t* conn_sock)
{
  struct sockaddr_storage ss;
  int sa_size = sizeof ss;
  SOCKET res;

  if (!conn_sock)
    return kLwpaErrInvalid;

  res = accept(id, (struct sockaddr*)&ss, &sa_size);

  if (res != INVALID_SOCKET)
  {
    if (address && !sockaddr_os_to_lwpa((lwpa_os_sockaddr_t*)&ss, address))
    {
      closesocket(res);
      return kLwpaErrSys;
    }
    *conn_sock = res;
    return kLwpaErrOk;
  }
  return err_winsock_to_lwpa(WSAGetLastError());
}

lwpa_error_t lwpa_bind(lwpa_socket_t id, const LwpaSockaddr* address)
{
  struct sockaddr_storage ss;
  size_t sa_size;
  int res;

  if (!address)
    return kLwpaErrInvalid;

  sa_size = sockaddr_lwpa_to_os(address, (lwpa_os_sockaddr_t*)&ss);
  if (sa_size == 0)
    return kLwpaErrInvalid;

  res = bind(id, (struct sockaddr*)&ss, (int)sa_size);
  return (res == 0 ? kLwpaErrOk : err_winsock_to_lwpa(WSAGetLastError()));
}

lwpa_error_t lwpa_close(lwpa_socket_t id)
{
  int res = closesocket(id);
  return (res == 0 ? kLwpaErrOk : err_winsock_to_lwpa(WSAGetLastError()));
}

lwpa_error_t lwpa_connect(lwpa_socket_t id, const LwpaSockaddr* address)
{
  struct sockaddr_storage ss;
  size_t sa_size;
  int res;

  if (!address)
    return kLwpaErrInvalid;

  sa_size = sockaddr_lwpa_to_os(address, (lwpa_os_sockaddr_t*)&ss);
  if (sa_size == 0)
    return kLwpaErrInvalid;

  res = connect(id, (struct sockaddr*)&ss, (int)sa_size);
  return (res == 0 ? kLwpaErrOk : err_winsock_to_lwpa(WSAGetLastError()));
}

lwpa_error_t lwpa_getpeername(lwpa_socket_t id, LwpaSockaddr* address)
{
  /* TODO */
  (void)id;
  (void)address;
  return kLwpaErrNotImpl;
}

lwpa_error_t lwpa_getsockname(lwpa_socket_t id, LwpaSockaddr* address)
{
  int res;
  struct sockaddr_storage ss;
  socklen_t size = sizeof ss;

  if (!address)
    return kLwpaErrInvalid;

  res = getsockname(id, (struct sockaddr*)&ss, &size);
  if (res == 0)
  {
    if (!sockaddr_os_to_lwpa((lwpa_os_sockaddr_t*)&ss, address))
      return kLwpaErrSys;
    return kLwpaErrOk;
  }
  return err_winsock_to_lwpa(WSAGetLastError());
}

lwpa_error_t lwpa_getsockopt(lwpa_socket_t id, int level, int option_name, void* option_value, size_t* option_len)
{
  /* TODO */
  (void)id;
  (void)level;
  (void)option_name;
  (void)option_value;
  (void)option_len;
  return kLwpaErrNotImpl;
}

lwpa_error_t lwpa_listen(lwpa_socket_t id, int backlog)
{
  int res = listen(id, backlog);
  return (res == 0 ? kLwpaErrOk : err_winsock_to_lwpa(WSAGetLastError()));
}

int lwpa_recv(lwpa_socket_t id, void* buffer, size_t length, int flags)
{
  int res;
  int impl_flags = (flags & LWPA_MSG_PEEK) ? MSG_PEEK : 0;

  if (!buffer)
    return kLwpaErrInvalid;

  res = recv(id, buffer, (int)length, impl_flags);
  return (res >= 0 ? res : (int)err_winsock_to_lwpa(WSAGetLastError()));
}

int lwpa_recvfrom(lwpa_socket_t id, void* buffer, size_t length, int flags, LwpaSockaddr* address)
{
  int res;
  int impl_flags = (flags & LWPA_MSG_PEEK) ? MSG_PEEK : 0;
  struct sockaddr_storage fromaddr;
  socklen_t fromlen = sizeof fromaddr;

  if (!buffer)
    return (int)kLwpaErrInvalid;

  res = recvfrom(id, buffer, (int)length, impl_flags, (struct sockaddr*)&fromaddr, &fromlen);

  if (res >= 0)
  {
    if (address && fromlen > 0)
    {
      if (!sockaddr_os_to_lwpa((lwpa_os_sockaddr_t*)&fromaddr, address))
        return kLwpaErrSys;
    }
    return res;
  }
  return (int)err_winsock_to_lwpa(WSAGetLastError());
}

int lwpa_send(lwpa_socket_t id, const void* message, size_t length, int flags)
{
  int res;
  (void)flags;

  if (!message)
    return kLwpaErrInvalid;

  res = send(id, message, (int)length, 0);
  return (res >= 0 ? res : (int)err_winsock_to_lwpa(WSAGetLastError()));
}

int lwpa_sendto(lwpa_socket_t id, const void* message, size_t length, int flags, const LwpaSockaddr* dest_addr)
{
  int res = -1;
  size_t ss_size;
  struct sockaddr_storage ss;
  (void)flags;

  if (!dest_addr || !message)
    return (int)kLwpaErrInvalid;

  if ((ss_size = sockaddr_lwpa_to_os(dest_addr, (lwpa_os_sockaddr_t*)&ss)) > 0)
    res = sendto(id, message, (int)length, 0, (struct sockaddr*)&ss, (int)ss_size);

  return (res >= 0 ? res : (int)err_winsock_to_lwpa(WSAGetLastError()));
}

lwpa_error_t lwpa_setsockopt(lwpa_socket_t id, int level, int option_name, const void* option_value, size_t option_len)
{
  int res = -1;

  if (!option_value)
    return kLwpaErrInvalid;

  // TODO this OS implementation could be simplified by use of socket option lookup arrays.
  switch (level)
  {
    case LWPA_SOL_SOCKET:
      res = setsockopt_socket(id, option_name, option_value, option_len);
      break;
    case LWPA_IPPROTO_IP:
      res = setsockopt_ip(id, option_name, option_value, option_len);
      break;
    case LWPA_IPPROTO_IPV6:
      res = setsockopt_ip6(id, option_name, option_value, option_len);
      break;
    default:
      return kLwpaErrInvalid;
  }
  return (res == 0 ? kLwpaErrOk : err_winsock_to_lwpa(WSAGetLastError()));
}

int setsockopt_socket(lwpa_socket_t id, int option_name, const void* option_value, size_t option_len)
{
  switch (option_name)
  {
    case LWPA_SO_RCVBUF:
      return setsockopt(id, SOL_SOCKET, SO_RCVBUF, option_value, (int)option_len);
    case LWPA_SO_SNDBUF:
      return setsockopt(id, SOL_SOCKET, SO_SNDBUF, option_value, (int)option_len);
    case LWPA_SO_RCVTIMEO:
      if (option_len == sizeof(int))
      {
        DWORD val = (DWORD) * (int*)option_value;
        return setsockopt(id, SOL_SOCKET, SO_RCVTIMEO, (char*)&val, sizeof val);
      }
      break;
    case LWPA_SO_SNDTIMEO:
      if (option_len == sizeof(int))
      {
        DWORD val = (DWORD) * (int*)option_value;
        return setsockopt(id, SOL_SOCKET, SO_SNDTIMEO, (char*)&val, sizeof val);
      }
      break;
    case LWPA_SO_REUSEADDR:
      if (option_len == sizeof(int))
      {
        BOOL val = (BOOL) * (int*)option_value;
        return setsockopt(id, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof val);
      }
      break;
    case LWPA_SO_BROADCAST:
      if (option_len == sizeof(int))
      {
        BOOL val = (BOOL) * (int*)option_value;
        return setsockopt(id, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof val);
      }
      break;
    case LWPA_SO_KEEPALIVE:
      if (option_len == sizeof(int))
      {
        BOOL val = (BOOL) * (int*)option_value;
        return setsockopt(id, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof val);
      }
      break;
    case LWPA_SO_LINGER:
      if (option_len == sizeof(LwpaLinger))
      {
        LwpaLinger* ll = (LwpaLinger*)option_value;
        struct linger val;
        val.l_onoff = (u_short)ll->onoff;
        val.l_linger = (u_short)ll->linger;
        return setsockopt(id, SOL_SOCKET, SO_LINGER, (char*)&val, sizeof val);
      }
      break;
    case LWPA_SO_ERROR:     /* Set not supported */
    case LWPA_SO_REUSEPORT: /* Not supported on this OS. */
    case LWPA_SO_TYPE:      /* Not supported */
    default:
      break;
  }
  // If we got here, something was invalid. Set errno accordingly
  WSASetLastError(WSAEINVAL);
  return -1;
}

int setsockopt_ip(lwpa_socket_t id, int option_name, const void* option_value, size_t option_len)
{
  switch (option_name)
  {
    case LWPA_IP_TTL:
      if (option_len == sizeof(int))
      {
        DWORD val = (DWORD) * (int*)option_value;
        return setsockopt(id, IPPROTO_IP, IP_TTL, (char*)&val, sizeof val);
      }
      break;
    case LWPA_IP_ADD_MEMBERSHIP:
      if (option_len == sizeof(LwpaMreq))
      {
        LwpaMreq* amreq = (LwpaMreq*)option_value;
        if (LWPA_IP_IS_V4(&amreq->group))
        {
          struct ip_mreq val;
          val.imr_multiaddr.s_addr = htonl(LWPA_IP_V4_ADDRESS(&amreq->group));
          val.imr_interface.s_addr = htonl(LWPA_IP_V4_ADDRESS(&amreq->netint));
          return setsockopt(id, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&val, sizeof val);
        }
      }
      break;
    case LWPA_IP_DROP_MEMBERSHIP:
      if (option_len == sizeof(LwpaMreq))
      {
        LwpaMreq* amreq = (LwpaMreq*)option_value;
        if (LWPA_IP_IS_V4(&amreq->group))
        {
          struct ip_mreq val;
          val.imr_multiaddr.s_addr = htonl(LWPA_IP_V4_ADDRESS(&amreq->group));
          val.imr_interface.s_addr = htonl(LWPA_IP_V4_ADDRESS(&amreq->netint));
          return setsockopt(id, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&val, sizeof val);
        }
      }
      break;
    case LWPA_MCAST_JOIN_GROUP:
      if (option_len == sizeof(LwpaGroupReq))
      {
        LwpaGroupReq* greq = (LwpaGroupReq*)option_value;
        if (LWPA_IP_IS_V4(&greq->group) && greq->ifindex >= 0)
        {
          struct group_req val;
          val.gr_interface = (ULONG)greq->ifindex;

          memset(&val.gr_group, 0, sizeof val.gr_group);
          struct sockaddr_in* sin = (struct sockaddr_in*)&val.gr_group;
          sin->sin_family = AF_INET;
          sin->sin_addr.s_addr = htonl(LWPA_IP_V4_ADDRESS(&greq->group));
          return setsockopt(id, IPPROTO_IP, MCAST_JOIN_GROUP, (char*)&val, sizeof val);
        }
      }
      break;
    case LWPA_MCAST_LEAVE_GROUP:
      if (option_len == sizeof(LwpaGroupReq))
      {
        LwpaGroupReq* greq = (LwpaGroupReq*)option_value;
        if (LWPA_IP_IS_V4(&greq->group) && greq->ifindex >= 0)
        {
          struct group_req val;
          val.gr_interface = (ULONG)greq->ifindex;

          memset(&val.gr_group, 0, sizeof val.gr_group);
          struct sockaddr_in* sin = (struct sockaddr_in*)&val.gr_group;
          sin->sin_family = AF_INET;
          sin->sin_addr.s_addr = htonl(LWPA_IP_V4_ADDRESS(&greq->group));
          return setsockopt(id, IPPROTO_IP, MCAST_LEAVE_GROUP, (char*)&val, sizeof val);
        }
      }
      break;
    case LWPA_IP_MULTICAST_IF:
      if (option_len == sizeof(unsigned int))
      {
        unsigned int ifindex = *(unsigned int*)option_value;
        // On Windows, this socket option takes either an IPv4 address or an interface index in
        // network byte order. We use the latter option.
        DWORD val = htonl(ifindex);
        return setsockopt(id, IPPROTO_IP, IP_MULTICAST_IF, (char*)&val, sizeof val);
      }
      break;
    case LWPA_IP_MULTICAST_TTL:
      if (option_len == sizeof(int))
      {
        DWORD val = (DWORD) * (int*)option_value;
        return setsockopt(id, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&val, sizeof val);
      }
      break;
    case LWPA_IP_MULTICAST_LOOP:
      if (option_len == sizeof(int))
      {
        DWORD val = (DWORD) * (int*)option_value;
        return setsockopt(id, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&val, sizeof val);
      }
      break;
    default:
      break;
  }
  // If we got here, something was invalid. Set errno accordingly
  WSASetLastError(WSAEINVAL);
  return -1;
}

int setsockopt_ip6(lwpa_socket_t id, int option_name, const void* option_value, size_t option_len)
{
  switch (option_name)
  {
    case LWPA_MCAST_JOIN_GROUP:
      if (option_len == sizeof(LwpaGroupReq))
      {
        LwpaGroupReq* greq = (LwpaGroupReq*)option_value;
        if (LWPA_IP_IS_V6(&greq->group) && greq->ifindex >= 0)
        {
          struct group_req val;
          val.gr_interface = (ULONG)greq->ifindex;

          memset(&val.gr_group, 0, sizeof val.gr_group);
          struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&val.gr_group;
          sin6->sin6_family = AF_INET6;
          memcpy(sin6->sin6_addr.s6_addr, LWPA_IP_V6_ADDRESS(&greq->group), LWPA_IPV6_BYTES);
          return setsockopt(id, IPPROTO_IPV6, MCAST_JOIN_GROUP, (char*)&val, sizeof val);
        }
      }
      break;
    case LWPA_MCAST_LEAVE_GROUP:
      if (option_len == sizeof(LwpaGroupReq))
      {
        LwpaGroupReq* greq = (LwpaGroupReq*)option_value;
        if (LWPA_IP_IS_V6(&greq->group) && greq->ifindex >= 0)
        {
          struct group_req val;
          val.gr_interface = (ULONG)greq->ifindex;

          memset(&val.gr_group, 0, sizeof val.gr_group);
          struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&val.gr_group;
          sin6->sin6_family = AF_INET6;
          memcpy(sin6->sin6_addr.s6_addr, LWPA_IP_V6_ADDRESS(&greq->group), LWPA_IPV6_BYTES);
          return setsockopt(id, IPPROTO_IPV6, MCAST_LEAVE_GROUP, (char*)&val, sizeof val);
        }
      }
      break;
    case LWPA_IP_MULTICAST_IF:
      if (option_len == sizeof(unsigned int))
      {
        DWORD val = (DWORD) * (unsigned int*)option_value;
        return setsockopt(id, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char*)&val, sizeof val);
      }
      break;
    case LWPA_IP_MULTICAST_TTL:
      if (option_len == sizeof(int))
      {
        DWORD val = (DWORD) * (int*)option_value;
        return setsockopt(id, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char*)&val, sizeof val);
      }
      break;
    case LWPA_IP_MULTICAST_LOOP:
      if (option_len == sizeof(int))
      {
        DWORD val = (DWORD) * (int*)option_value;
        return setsockopt(id, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char*)&val, sizeof val);
      }
      break;
    case LWPA_IPV6_V6ONLY:
      if (option_len == sizeof(int))
      {
        DWORD val = (DWORD) * (int*)option_value;
        return setsockopt(id, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&val, sizeof val);
      }
      break;
    default: /* Other IPv6 options TODO on windows. */
      break;
  }
  // If we got here, something was invalid. Set errno accordingly
  WSASetLastError(WSAEINVAL);
  return -1;
}

lwpa_error_t lwpa_shutdown(lwpa_socket_t id, int how)
{
  if (how >= 0 && how < LWPA_NUM_SHUT)
  {
    int res = shutdown(id, shutmap[how]);
    return (res == 0 ? kLwpaErrOk : err_winsock_to_lwpa(WSAGetLastError()));
  }
  return kLwpaErrInvalid;
}

lwpa_error_t lwpa_socket(unsigned int family, unsigned int type, lwpa_socket_t* id)
{
  if (id)
  {
    if (family < LWPA_NUM_AF && type < LWPA_NUM_TYPE)
    {
      SOCKET sock = socket(sfmap[family], stmap[type], 0);
      if (sock != INVALID_SOCKET)
      {
        *id = sock;
        return kLwpaErrOk;
      }
      else
      {
        *id = LWPA_SOCKET_INVALID;
        return err_winsock_to_lwpa(WSAGetLastError());
      }
    }
    else
    {
      *id = LWPA_SOCKET_INVALID;
    }
  }
  return kLwpaErrInvalid;
}

lwpa_error_t lwpa_setblocking(lwpa_socket_t id, bool blocking)
{
  unsigned long val = (blocking ? 0 : 1);
  int res = ioctlsocket(id, FIONBIO, &val);
  return (res == 0 ? kLwpaErrOk : err_winsock_to_lwpa(WSAGetLastError()));
}

lwpa_error_t lwpa_getblocking(lwpa_socket_t id, bool* blocking)
{
  *blocking = false;
  return kLwpaErrOk;
}

lwpa_error_t lwpa_poll_context_init(LwpaPollContext* context)
{
  if (!context)
    return kLwpaErrInvalid;

  if (!lwpa_mutex_create(&context->lock))
    return kLwpaErrSys;

  lwpa_rbtree_init(&context->sockets, poll_socket_compare, poll_socket_alloc, poll_socket_free);
  LWPA_FD_ZERO(&context->readfds);
  LWPA_FD_ZERO(&context->writefds);
  LWPA_FD_ZERO(&context->exceptfds);
  context->valid = true;
  return kLwpaErrOk;
}

void lwpa_poll_context_deinit(LwpaPollContext* context)
{
  if (!context || !context->valid)
    return;

  lwpa_rbtree_clear(&context->sockets);
  lwpa_mutex_destroy(&context->lock);
  context->valid = false;
}

void set_in_fd_sets(LwpaPollContext* context, const LwpaPollSocket* sock_desc)
{
  if (sock_desc->events & LWPA_POLL_IN)
  {
    LWPA_FD_SET(sock_desc->sock, &context->readfds);
  }
  if (sock_desc->events & (LWPA_POLL_OUT | LWPA_POLL_CONNECT))
  {
    LWPA_FD_SET(sock_desc->sock, &context->writefds);
  }
  if (sock_desc->events & (LWPA_POLL_OOB | LWPA_POLL_CONNECT))
  {
    LWPA_FD_SET(sock_desc->sock, &context->exceptfds);
  }
}

void clear_in_fd_sets(LwpaPollContext* context, const LwpaPollSocket* sock_desc)
{
  if (sock_desc->events & LWPA_POLL_IN)
  {
    LWPA_FD_CLEAR(sock_desc->sock, &context->readfds);
  }
  if (sock_desc->events & (LWPA_POLL_OUT | LWPA_POLL_CONNECT))
  {
    LWPA_FD_CLEAR(sock_desc->sock, &context->writefds);
  }
  if (sock_desc->events & (LWPA_POLL_OOB | LWPA_POLL_CONNECT))
  {
    LWPA_FD_CLEAR(sock_desc->sock, &context->exceptfds);
  }
}

lwpa_error_t lwpa_poll_add_socket(LwpaPollContext* context, lwpa_socket_t socket, lwpa_poll_events_t events,
                                  void* user_data)
{
  if (!context || !context->valid || socket == LWPA_SOCKET_INVALID || !(events & LWPA_POLL_VALID_INPUT_EVENT_MASK))
    return kLwpaErrInvalid;

  lwpa_error_t res = kLwpaErrSys;
  if (lwpa_mutex_take(&context->lock))
  {
    if (lwpa_rbtree_size(&context->sockets) >= LWPA_SOCKET_MAX_POLL_SIZE)
    {
      res = kLwpaErrNoMem;
    }
    else
    {
      LwpaPollSocket* new_sock = (LwpaPollSocket*)malloc(sizeof(LwpaPollSocket));
      if (new_sock)
      {
        new_sock->sock = socket;
        new_sock->events = events;
        new_sock->user_data = user_data;
        res = lwpa_rbtree_insert(&context->sockets, new_sock);
        if (res == kLwpaErrOk)
        {
          set_in_fd_sets(context, new_sock);
        }
        else
        {
          free(new_sock);
        }
      }
      else
      {
        res = kLwpaErrNoMem;
      }
    }
    lwpa_mutex_give(&context->lock);
  }
  return res;
}

lwpa_error_t lwpa_poll_modify_socket(LwpaPollContext* context, lwpa_socket_t socket, lwpa_poll_events_t new_events,
                                     void* new_user_data)
{
  if (!context || !context->valid || socket == LWPA_SOCKET_INVALID || !(new_events & LWPA_POLL_VALID_INPUT_EVENT_MASK))
    return kLwpaErrInvalid;

  lwpa_error_t res = kLwpaErrSys;
  if (lwpa_mutex_take(&context->lock))
  {
    LwpaPollSocket* sock_desc = (LwpaPollSocket*)lwpa_rbtree_find(&context->sockets, &socket);
    if (sock_desc)
    {
      clear_in_fd_sets(context, sock_desc);
      sock_desc->events = new_events;
      sock_desc->user_data = new_user_data;
      set_in_fd_sets(context, sock_desc);
      res = kLwpaErrOk;
    }
    else
    {
      res = kLwpaErrNotFound;
    }
    lwpa_mutex_give(&context->lock);
  }
  return res;
}

void lwpa_poll_remove_socket(LwpaPollContext* context, lwpa_socket_t socket)
{
  if (!context || !context->valid || socket == LWPA_SOCKET_INVALID)
    return;

  if (lwpa_mutex_take(&context->lock))
  {
    LwpaPollSocket* sock_desc = (LwpaPollSocket*)lwpa_rbtree_find(&context->sockets, &socket);
    if (sock_desc)
    {
      clear_in_fd_sets(context, sock_desc);
      lwpa_rbtree_remove(&context->sockets, sock_desc);
    }
    lwpa_mutex_give(&context->lock);
  }
}

lwpa_error_t lwpa_poll_wait(LwpaPollContext* context, LwpaPollEvent* event, int timeout_ms)
{
  if (!context || !context->valid || !event)
    return kLwpaErrInvalid;

  // Get the sets of sockets that we will select on.
  LwpaPollFdSet readfds, writefds, exceptfds;
  LWPA_FD_ZERO(&readfds);
  LWPA_FD_ZERO(&writefds);
  LWPA_FD_ZERO(&exceptfds);
  if (lwpa_mutex_take(&context->lock))
  {
    if (lwpa_rbtree_size(&context->sockets) > 0)
    {
      readfds = context->readfds;
      writefds = context->writefds;
      exceptfds = context->exceptfds;
    }
    lwpa_mutex_give(&context->lock);
  }

  // No valid sockets are currently added to the context.
  if (!readfds.count && !writefds.count && !exceptfds.count)
    return kLwpaErrNoSockets;

  struct timeval os_timeout;
  if (timeout_ms == 0)
  {
    os_timeout.tv_sec = 0;
    os_timeout.tv_usec = 0;
  }
  else if (timeout_ms != LWPA_WAIT_FOREVER)
  {
    os_timeout.tv_sec = timeout_ms / 1000;
    os_timeout.tv_usec = (timeout_ms % 1000) * 1000;
  }

  int sel_res = select(0, readfds.count ? &readfds.set : NULL, writefds.count ? &writefds.set : NULL,
                       exceptfds.count ? &exceptfds.set : NULL, timeout_ms == LWPA_WAIT_FOREVER ? NULL : &os_timeout);

  if (sel_res < 0)
  {
    return err_winsock_to_lwpa(WSAGetLastError());
  }
  else if (sel_res == 0)
  {
    return kLwpaErrTimedOut;
  }
  else
  {
    lwpa_error_t res = kLwpaErrSys;
    if (context->valid && lwpa_mutex_take(&context->lock))
    {
      res = handle_select_result(context, event, &readfds, &writefds, &exceptfds);
      lwpa_mutex_give(&context->lock);
    }
    return res;
  }
}

lwpa_error_t handle_select_result(LwpaPollContext* context, LwpaPollEvent* event, const LwpaPollFdSet* readfds,
                                  const LwpaPollFdSet* writefds, const LwpaPollFdSet* exceptfds)
{
  // Init the event data.
  event->socket = LWPA_SOCKET_INVALID;
  event->events = 0;
  event->err = kLwpaErrOk;

  // The default return; if we don't find any sockets set that we passed to select(), something has
  // gone wrong.
  lwpa_error_t res = kLwpaErrSys;

  LwpaRbIter iter;
  lwpa_rbiter_init(&iter);
  for (LwpaPollSocket* sock_desc = (LwpaPollSocket*)lwpa_rbiter_first(&iter, &context->sockets); sock_desc;
       sock_desc = (LwpaPollSocket*)lwpa_rbiter_next(&iter))
  {
    if (sock_desc->sock == LWPA_SOCKET_INVALID)
      continue;

    if (LWPA_FD_ISSET(sock_desc->sock, readfds) || LWPA_FD_ISSET(sock_desc->sock, writefds) ||
        LWPA_FD_ISSET(sock_desc->sock, exceptfds))
    {
      res = kLwpaErrOk;
      event->socket = sock_desc->sock;
      event->user_data = sock_desc->user_data;

      /* Check for errors */
      int error;
      int error_size = sizeof(error);
      if (getsockopt(sock_desc->sock, SOL_SOCKET, SO_ERROR, (char*)&error, &error_size) == 0)
      {
        if (error != 0)
        {
          event->events |= LWPA_POLL_ERR;
          event->err = err_winsock_to_lwpa(error);
        }
      }
      else
      {
        res = err_winsock_to_lwpa(WSAGetLastError());
        break;
      }
      if (LWPA_FD_ISSET(sock_desc->sock, readfds))
      {
        if (sock_desc->events & LWPA_POLL_IN)
          event->events |= LWPA_POLL_IN;
      }
      if (LWPA_FD_ISSET(sock_desc->sock, writefds))
      {
        if (sock_desc->events & LWPA_POLL_CONNECT)
          event->events |= LWPA_POLL_CONNECT;
        else if (sock_desc->events & LWPA_POLL_OUT)
          event->events |= LWPA_POLL_OUT;
      }
      if (LWPA_FD_ISSET(sock_desc->sock, exceptfds))
      {
        if (sock_desc->events & LWPA_POLL_CONNECT)
          event->events |= LWPA_POLL_CONNECT;  // Async connect errors are handled above
        else if (sock_desc->events & LWPA_POLL_OOB)
          event->events |= LWPA_POLL_OOB;
      }
      break;  // We handle one event at a time.
    }
  }
  return res;
}

int poll_socket_compare(const LwpaRbTree* tree, const LwpaRbNode* node_a, const LwpaRbNode* node_b)
{
  (void)tree;

  LwpaPollSocket* a = (LwpaPollSocket*)node_a->value;
  LwpaPollSocket* b = (LwpaPollSocket*)node_b->value;

  return (a->sock > b->sock) - (a->sock < b->sock);
}

LwpaRbNode* poll_socket_alloc()
{
  return (LwpaRbNode*)malloc(sizeof(LwpaRbNode));
}

void poll_socket_free(LwpaRbNode* node)
{
  if (node)
  {
    free(node->value);
    free(node);
  }
}

lwpa_error_t lwpa_getaddrinfo(const char* hostname, const char* service, const LwpaAddrinfo* hints,
                              LwpaAddrinfo* result)
{
  int res;
  struct addrinfo* pf_res;
  struct addrinfo pf_hints;

  if ((!hostname && !service) || !result)
    return kLwpaErrInvalid;

  memset(&pf_hints, 0, sizeof pf_hints);
  if (hints)
  {
    pf_hints.ai_flags = (hints->ai_flags < LWPA_NUM_AIF) ? aiflagmap[hints->ai_flags] : 0;
    pf_hints.ai_family = (hints->ai_family < LWPA_NUM_AF) ? aifammap[hints->ai_family] : AF_UNSPEC;
    pf_hints.ai_socktype = (hints->ai_socktype < LWPA_NUM_TYPE) ? stmap[hints->ai_socktype] : 0;
    pf_hints.ai_protocol = (hints->ai_protocol < LWPA_NUM_IPPROTO) ? aiprotmap[hints->ai_protocol] : 0;
  }

  res = getaddrinfo(hostname, service, hints ? &pf_hints : NULL, &pf_res);
  if (res == 0)
  {
    result->pd[0] = pf_res;
    result->pd[1] = pf_res;
    if (!lwpa_nextaddr(result))
      res = -1;
  }
  return (res == 0 ? kLwpaErrOk : err_winsock_to_lwpa(res));
}

bool lwpa_nextaddr(LwpaAddrinfo* ai)
{
  if (ai && ai->pd[1])
  {
    struct addrinfo* pf_ai = (struct addrinfo*)ai->pd[1];
    ai->ai_flags = 0;
    if (!sockaddr_os_to_lwpa((lwpa_os_sockaddr_t*)pf_ai->ai_addr, &ai->ai_addr))
      return false;

    /* Can't use reverse maps, because we have no guarantee of the numeric values of the OS
     * constants. Ugh. */
    if (pf_ai->ai_family == AF_INET)
      ai->ai_family = LWPA_AF_INET;
    else if (pf_ai->ai_family == AF_INET6)
      ai->ai_family = LWPA_AF_INET6;
    else
      ai->ai_family = LWPA_AF_UNSPEC;

    if (pf_ai->ai_socktype == SOCK_DGRAM)
      ai->ai_socktype = LWPA_DGRAM;
    else if (pf_ai->ai_socktype == SOCK_STREAM)
      ai->ai_socktype = LWPA_STREAM;
    else
      ai->ai_socktype = 0;

    if (pf_ai->ai_protocol == IPPROTO_UDP)
      ai->ai_protocol = LWPA_IPPROTO_UDP;
    else if (pf_ai->ai_protocol == IPPROTO_TCP)
      ai->ai_protocol = LWPA_IPPROTO_TCP;
    else
      ai->ai_protocol = 0;

    ai->ai_canonname = pf_ai->ai_canonname;
    ai->pd[1] = pf_ai->ai_next;

    return true;
  }
  return false;
}

void lwpa_freeaddrinfo(LwpaAddrinfo* ai)
{
  if (ai)
    freeaddrinfo((struct addrinfo*)ai->pd[0]);
}

#endif /* !defined(LWPA_BUILDING_MOCK_LIB) */

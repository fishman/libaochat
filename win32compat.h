#ifndef __WIN32COMPAT_H
#define __WIN32COMPAT_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <wincrypt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* yawn....... */
#ifndef EWOULDBLOCK
	#define EWOULDBLOCK             WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
	#define EINPROGRESS             WSAEINPROGRESS
#endif
#ifndef EALREADY
	#define EALREADY                WSAEALREADY
#endif
#ifndef ENOBUFS
	#define ENOBUFS                 WSAENOBUFS
#endif

/* connect.c(14) : warning C4047: '=' : 'unsigned char *' differs in levels of indirection from 'int' */
#pragma warning(disable:4047)
/* connect.c(123) : warning C4133: 'function' : incompatible types - from 'int *' to 'char *' */
#pragma warning(disable:4133)

#endif

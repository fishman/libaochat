/* libaochat -- Anarchy Online Chat Library
   Copyright (c) 2003, 2004 Andreas Allerdahl <dinkles@tty0.org>.
   All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA
*/
#include "aochat.h"
#include <fcntl.h>


struct __aoc_pref _aoc_pref =
{
	0,	/* AOC_PREF_DEBUG */
	0,	/* AOC_PREF_BWLIMIT */
	0,	/* AOC_PREF_PKTLIMIT */
	AOC_PUBLIC_KEY_1,
	AOC_PUBLIC_KEY_2,
	0,	/* AOC_PREF_FASTRND */
	0	/* AOC_PREF_MAXQSIZE */
};


void aocSetPref(int prefid, void *value)
{
	switch(prefid)
	{
	case AOC_PREF_DEBUG:
		_aoc_pref.debug = (int)value;
		break;
	case AOC_PREF_BWLIMIT:
		_aoc_pref.bwlimit = (int)value;
		break;
	case AOC_PREF_PKTLIMIT:
		_aoc_pref.pktlimit = (int)value;
		break;
	case AOC_PREF_PKEY1:
		_aoc_pref.pkey1 = (char *)value;
		break;
	case AOC_PREF_PKEY2:
		_aoc_pref.pkey2 = (char *)value;
		break;
	case AOC_PREF_FASTRND:
		_aoc_pref.fastrnd = (int)value;
		break;
	case AOC_PREF_MAXQSIZE:
		_aoc_pref.maxqueuesize = (int)value;
	}
}


char *aocServerPacketName(int type)
{
	static char buf[32];

	switch(type)
	{
	case AOC_SRV_LOGIN_SEED:			return "LOGIN_SEED";
	case AOC_SRV_LOGIN_OK:			return "LOGIN_OK";
	case AOC_SRV_LOGIN_ERROR:		return "LOGIN_ERROR";
	case AOC_SRV_LOGIN_CHARLIST:		return "LOGIN_CHARLIST";
	case AOC_SRV_CLIENT_UNKNOWN:		return "CLIENT_UNKNOWN";
	case AOC_SRV_CLIENT_NAME:		return "CLIENT_NAME";
	case AOC_SRV_LOOKUP_RESULT:		return "LOOKUP_RESULT";
	case AOC_SRV_PRIVATE_MSG:		return "PRIVATE_MSG";
	case AOC_SRV_VICINITY_MSG:		return "VICINITY_MSG";
	case AOC_SRV_ANONVICINITY_MSG:	return "ANONVICINITY_MSG";
	case AOC_SRV_SYSTEM_MSG:			return "SYSTEM_MSG";
	case AOC_SRV_CHAT_NOTICE:			return "CHAT_NOTICE";
	case AOC_SRV_BUDDY_STATUS:		return "BUDDY_STATUS";
	case AOC_SRV_BUDDY_REMOVED:		return "BUDDY_REMOVED";
	case AOC_SRV_PRIVGRP_INVITE:		return "PRIVGRP_INVITE";
	case AOC_SRV_PRIVGRP_KICK:		return "PRIVGRP_KICK";
	case AOC_SRV_PRIVGRP_PART:		return "PRIVGRP_PART";
	case AOC_SRV_PRIVGRP_CLIJOIN:	return "PRIVGRP_CLIJOIN";
	case AOC_SRV_PRIVGRP_CLIPART:	return "PRIVGRP_CLIPART";
	case AOC_SRV_PRIVGRP_MSG:		return "PRIVGRP_MSG";
	case AOC_SRV_GROUP_JOIN:			return "GROUP_INFO";
	case AOC_SRV_GROUP_PART:			return "GROUP_PART";
	case AOC_SRV_GROUP_MSG:			return "GROUP_MSG";
	case AOC_SRV_PONG:				return "PONG";
	case AOC_SRV_FORWARD:			return "FORWARD";
	case AOC_SRV_AMD_MUX_INFO:		return "AMD_MUX_INFO";
	default:
#ifdef HAVE_SNPRINTF
		snprintf(buf, sizeof(buf), "UNKNOWN_%d", type);
#else
		sprintf(buf, "UNKNOWN_%d", type);
#endif
		return buf;
	}
}


char *aocClientPacketName(int type)
{
	static char buf[32];

	switch(type)
	{
	case AOC_CLI_LOGIN_RESPONSE:		return "LOGIN_RESPONSE";
	case AOC_CLI_LOGIN_SELCHAR:		return "LOGIN_SELCHAR";
	case AOC_CLI_NAME_LOOKUP:		return "NAME_LOOKUP";
	case AOC_CLI_PRIVATE_MSG:		return "PRIVATE_MSG";
	case AOC_CLI_BUDDY_ADD:			return "BUDDY_ADD";
	case AOC_CLI_BUDDY_REMOVE:		return "BUDDY_REMOVE";
	case AOC_CLI_ONLINE_STATUS:		return "ONLINE_STATUS";
	case AOC_CLI_PRIVGRP_INVITE:		return "PRIVGRP_INVITE";
	case AOC_CLI_PRIVGRP_KICK:		return "PRIVGRP_KICK";
	case AOC_CLI_PRIVGRP_JOIN:		return "PRIVGRP_JOIN";
	case AOC_CLI_PRIVGRP_KICKALL:	return "PRIVGRP_KICKALL";
	case AOC_CLI_PRIVGRP_MSG:		return "PRIVGRP_MSG";
	case AOC_CLI_GROUP_DATASET:		return "GROUP_DATASET";
	case AOC_CLI_GROUP_MESSAGE:		return "GROUP_MESSAGE";
	case AOC_CLI_GROUP_CLIMODE:		return "GROUP_CLIMODE";
	case AOC_CLI_CLIMODE_GET:		return "CLIMODE_GET";
	case AOC_CLI_CLIMODE_SET:		return "CLIMODE_SET";
	case AOC_CLI_PING:				return "PING";
	case AOC_CLI_CHAT_COMMAND:		return "CHAT_COMMAND";
	default:
#ifdef HAVE_SNPRINTF
		snprintf(buf, sizeof(buf), "UNKNOWN_%d", type);
#else
		sprintf(buf, "UNKNOWN_%d", type);
#endif
		return buf;
	}
}


int aocMsgArraySize(const aocMessage *msg, int aidx)
{
	int n;

	if(msg == NULL) return -1;

	for(n=0; n<msg->argc; n++)
	{
		if(msg->argt[n] != AOC_TYPE_ARRAYSIZE)
			continue;

		if(aidx-- == 0)
			return AOC_WRD(msg->argv[n]);

		n += AOC_WRD(msg->argv[n]);
	}

	return -1;
}


void *aocMsgArrayValue(const aocMessage *msg, int aidx, int idx, int *argt, int *argl)
{
	int n;

	if(msg == NULL) return NULL;

	for(n=0; n<msg->argc; n++)
	{
		if(msg->argt[n] != AOC_TYPE_ARRAYSIZE)
			continue;

		if(aidx-- == 0)
		{
			if(idx >= AOC_WRD(msg->argv[n]))
				return NULL;

			if(argt != NULL) *argt = msg->argt[n+idx+1];
			if(argl != NULL) *argl = msg->argl[n+idx+1];

			return msg->argv[n+idx+1];
		}

		n += AOC_WRD(msg->argv[n]);
	}

	return NULL;	
}


void aocLowerCase(char *str)
{
	if(str == NULL) return;

	while(*str)
	{
		if(*str >= 'A' && *str <= 'Z')
			*str += ('a' - 'A');
		str++;
	}

	return;
}


char *aocNameLowerCase(const char *str)
{
	static char buf[128];
	int i;
	
	if(str == NULL) return NULL;
	
	for(i=0; i<127; i++)
	{
		if(str[i] >= 'A' && str[i] <= 'Z')
			buf[i] = str[i] + ('a' - 'A');
		else
			buf[i] = str[i];

		if(str[i] == 0)
			break;
	}
	buf[127] = 0;

	return buf;
}


char *aocNameTrim(const char *str)
{
	static char buf[128];
	char *end;

	if(str == NULL) return NULL;

	/* left trim */
	while(*str == ' ') str++;

	strncpy(buf, str, 127);
	buf[127] = 0;

	if(buf[0] != 0)
	{
		/* right trim */
		end = buf + strlen(buf);
		while(end > buf && *--end == ' ');
		end[1] = 0;
	}

	return buf;
}


struct sockaddr_in *aocMakeAddr(const char *host, int port)
{
	static struct sockaddr_in addr;
	struct in_addr *raddr;

	raddr = aocResolveHost(host);
	if(!raddr)
		return NULL;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = raddr->s_addr;

	return &addr;
}


void aocSocketSetAsync(int s)
{
#ifdef _WIN32
	u_long arg = 1;

	ioctlsocket(s, FIONBIO, &arg);
#else
	int flags = 0;

	flags = fcntl(s, F_GETFL);
	flags |= O_NONBLOCK;

	fcntl(s, F_SETFL, flags);
#endif
	return;
}


struct in_addr *aocResolveHost(const char *host)
{
	static struct in_addr addr;
	struct hostent *he;

	if(host == NULL) return NULL;

	memset(&addr, 0, sizeof(addr));

	/* check if the host is just an IP (eg "1.2.3.4") */
#ifdef HAVE_INET_ATON
	if(inet_aton(host, &addr))
		return &addr;
#else
	addr.s_addr = inet_addr(host);
	if(addr.s_addr != -1)
		return &addr;
#endif
	/* resolve the hostname */
	if(!(he = gethostbyname(host)))
		return NULL;

	memcpy(&addr.s_addr, he->h_addr_list[0], 4);

	return &addr;
}


void *aocMemDup(const void *ptr, int len)
{
	void *_ptr;

	if(ptr == NULL) return NULL;
	if(len == 0) return NULL;

	_ptr = malloc(len);
	if(_ptr != NULL)
		memcpy(_ptr, ptr, len);

	return _ptr;
}


unsigned char *aocGetColorRGB(int color)
{
	static unsigned char ao_color_map[42][3] =
	{
		/* colors 0 -> 34 */
		{0x00, 0xff, 0x00}, {0x30, 0xd2, 0xff}, {0x30, 0xd2, 0xff},
		{0xff, 0xff, 0xc9}, {0x00, 0xff, 0xff}, {0xff, 0xff, 0x45},
		{0x00, 0xa6, 0x51}, {0x63, 0xe6, 0x89}, {0x0f, 0xf2, 0x0b},
		{0xff, 0x00, 0x99}, {0x22, 0x99, 0xff}, {0x00, 0x00, 0x00},
		{0xff, 0x00, 0x00}, {0x00, 0xf0, 0x00}, {0x00, 0x00, 0xff},
		{0xff, 0xff, 0xff}, {0xff, 0xff, 0x00}, {0xcc, 0xaa, 0x44},
		{0xdd, 0xdd, 0x44}, {0x00, 0xdd, 0x44}, {0x66, 0xaa, 0x66},
		{0xff, 0xff, 0xff}, {0x9a, 0xd5, 0xd9}, {0xff, 0x00, 0x00},
		{0xff, 0x00, 0x00}, {0xd9, 0xd9, 0xd2}, {0x99, 0x99, 0x26},
		{0xff, 0x77, 0x18}, {0x8c, 0xb6, 0xff}, {0xff, 0xff, 0x00},
		{0xff, 0xe3, 0xa1}, {0x00, 0xee, 0x00}, {0xee, 0xee, 0xee},
		{0xcc, 0xcc, 0xcc}, {0xff, 0x63, 0xff},
		/* colors 50 -> 55 (index 35 -> 40) */
		{0xff, 0x8c, 0xfc}, {0xff, 0xff, 0xff}, {0xff, 0x61, 0xa6},
		{0x66, 0x99, 0xff}, {0x66, 0xff, 0x99}, {0x00, 0xf0, 0x00},
		/* color 80 (index 41) */
		{0xee, 0xee, 0xee}
	};

	/* map colors 80,90 to 40,41 */
	if(color == 80)
		color = 41;
	else if(color >= 50 && color <= 55)
		color -= 15;
	else if(color < 0 || color > 34)
		return NULL;

	return ao_color_map[color];
}


unsigned char *aocStripStyles(unsigned char *str, int len, int alloc)
{
	unsigned char *res;
	int i, p = 0;

	if(str == NULL) return NULL;

	if(len == -1)
		len = strlen(str);

	res = alloc ? malloc(len+1) : str;

	for(i=0; i<len; i++)
	{
		if(	str[i] == AOC_STYLE_COLOR && len - i >= 2 &&
			aocGetColorRGB(str[i+1]) != NULL )
		{
			i++;
			/* unicode crap?? ugh..
			if(str[i+1] >= 192)
				i++;
			if(str[i+1] >= 223)
				i++;*/
		}
		else if(str[i] != AOC_STYLE_UL_START && str[i] != AOC_STYLE_UL_END)
		{
			res[p++] = str[i];
		}
	}

	if(p < len || alloc)
		res[p] = 0;

	return res;
}


void aocMakeWndBlob(unsigned char *blob, uint32_t len, int is_tell)
{
	static uint32_t id = 0;

	aocMakeBlob(blob, len, 50000, id++, 0, is_tell);
}


void aocMakeItemBlob(unsigned char *blob, uint32_t low_id, uint32_t high_id, uint32_t item_ql, int is_tell)
{
	aocMakeBlob(blob, 0, low_id, high_id, item_ql, is_tell);
}


void aocMakeBlob(unsigned char *blob, uint32_t blob_len, uint32_t low_id, uint32_t high_id, uint32_t item_ql, int is_tell)
{
	blob_len = htonl(blob_len);
	low_id = htonl(low_id);
	high_id = htonl(high_id);
	item_ql = htonl(item_ql);

	if(blob == NULL) return;

	/* prefix with null chars; 2 for private tells, 1 for everything else. */
	if(is_tell)
		*blob++ = 0;
	*blob++ = 0;

	/* Low ID */
	memcpy(blob, &low_id, 4);
	blob += 4;

	/* High ID */
	memcpy(blob, &high_id, 4);
	blob += 4;

	/* Item QL */
	memcpy(blob, &item_ql, 4);
	blob += 4;

	/* Text length */
	memcpy(blob, &blob_len, 4);

	return;
}


int aocDecodeBlob(unsigned char *blob, int blob_len, int is_tell, uint32_t *low_id, uint32_t *high_id, uint32_t *item_ql, uint32_t *str_len, unsigned char **str)
{
	int baselen = 17 + (is_tell ? 1 : 0);

	if(blob == NULL) return 0;

	if(blob_len < baselen)
		return 0;

	/* private tells are prefixed with 2 null chars */
	if(is_tell) blob++;


	if(*blob != 0)
		return 0;

	blob++;

	/* Low ID */
	*low_id = ntohl(*(uint32_t *)blob);

	/* High ID */
	*high_id = ntohl(*(uint32_t *)(blob+4));

	/* Item QL */
	*item_ql = ntohl(*(uint32_t *)(blob+8));

	/* Text length */
	*str_len = ntohl(*(uint32_t *)(blob+12));

	*str = blob + 16;

	if(*str_len > blob_len - baselen)
		return 0;	/* *str_len = blob_len - baselen; */

	return 1;
}


#ifdef _WIN32

/* TODO: move to win32 compat */
int gettimeofday(struct timeval *tv, void *moo)
{
	FILETIME ft;
	ULARGE_INTEGER t;

	GetSystemTimeAsFileTime(&ft);
	t.LowPart = ft.dwLowDateTime;
	t.HighPart = ft.dwHighDateTime;

	/* FIXME: tv->tv_sec is incorrect */
	tv->tv_sec = (t.QuadPart - 100000000) / 10000000;
	tv->tv_usec = (t.QuadPart / 10) % 1000000;

	return 0;
}

#endif


/* time/cpu-speed based "pseudo" random number generator */
static void moo_trand(unsigned char *buf, int len)
{
	struct timeval ptv, ntv;
	static long moo = 0x12345678;
	static int mod = 8000, init = 0;
	long diff;
	int a, b;

	if(!init)
	{
		/* first generates 32 bytes of data to seed moo
		   prevents timing attacks.. */
		char crap[32];
		init = 1;
		moo_trand(crap, 16);
		gettimeofday(&ptv, NULL);
		srand(ptv.tv_sec * ptv.tv_usec + ptv.tv_usec);
	}


	for(a=0; a<len; a++)
	{
		gettimeofday(&ptv, NULL);
		moo *= ptv.tv_usec + 1;

		/* cpu speed dependant delay */
		if(mod < 2) mod = 2;
		for(b=0; b<(mod+moo%mod); b++);

		gettimeofday(&ntv, NULL);
		diff = AOC_TV_UDIFF(&ntv, &ptv);

		moo += diff;

		if(diff < 1024)
		{
			mod += moo & 0xFF;
			a--;
			continue;
		}
		mod -= moo  & 0xFF;

		buf[a] = (moo + mod + rand()) & 0xFF;
	}

	return;
}


#ifdef _WIN32
void aocRandom(unsigned char *buf, int len)
{
	HCRYPTPROV hProv = 0;
	int ok = 1;

	/* try to use windows built-in crypto functions (win95+) */
	if(CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0))
	{
		if(!CryptGenRandom(hProv, len, buf))
			ok = 0;
		else
			CryptReleaseContext(hProv, 0);
	}
	else
	{
		ok = 0;
	}

	if(!ok)
		moo_trand(buf, len);

	return;
}
#else
void aocRandom(unsigned char *buf, int len)
{
	FILE *fd = NULL;
	int l;


	if(buf == NULL) return;

	/*	/dev/random can sometimes be extremely slow (eg. when the
		entropy pool is empty), so an option to disable the use of
		it is needed..
	*/
	if(!_aoc_pref.fastrnd)
		fd = fopen("/dev/random", "r");

	if(fd != NULL)
	{
		l = fread(buf, 1, len, fd);
		fclose(fd);
		if(l != len)
			fd = NULL;
	}

	if(fd == NULL && buf)
	{
		/* try to use /dev/urandom (ok) */
		fd = fopen("/dev/urandom", "r");
		if(fd != NULL)
		{
			l = fread(buf, 1, len, fd);
			fclose(fd);
			if(l != len)
				fd = NULL;
		}
	}

	if(fd == NULL && buf)
		moo_trand(buf, len);

	return;
}
#endif


void aocDebugMsg(const char *fmt, ...)
{
	va_list va;

	fprintf(stderr, "AOCDBG: ");
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");

	return;
}


void aocFree(void *ptr)
{
	if(ptr != NULL)
		free(ptr);
}

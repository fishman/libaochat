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
#include <stdarg.h>
#include "aochat.h"


int aocPushWord(aocPacket *p, uint16_t val)
{
	if(p->len + 4 + 2 > p->size)
	{
		unsigned char *newptr;

		newptr = realloc(p->data, p->size + 128);
		if(newptr == NULL)
			return 0;

		p->size += 128;
		p->data = newptr;
	}

	val = htons(val);

	memcpy(p->data + 4 + p->len, &val, 2);
	p->len += 2;

	return 1;
}


int aocPushInteger(aocPacket *p, uint32_t val)
{
	if(p->len + 4 + 4 > p->size)
	{
		unsigned char *newptr;

		newptr = realloc(p->data, p->size + 128);
		if(newptr == NULL)
			return 0;

		p->size += 128;
		p->data = newptr;
	}

	val = htonl(val);

	memcpy(p->data + 4 + p->len, &val, 4);
	p->len += 4;

	return 1;
}


int aocPushGroupId(aocPacket *p, const unsigned char *grpid)
{
	if(grpid == NULL) return 0;

	if(p->len + 4 + 5 > p->size)
	{
		unsigned char *newptr;

		newptr = realloc(p->data, p->size + 128);
		if(newptr == NULL)
			return 0;

		p->size += 128;
		p->data = newptr;
	}

	memcpy(p->data + 4 + p->len, grpid, 5);
	p->len += 5;

	return 1;
}


int aocPushString(aocPacket *p, const char *str, int len)
{
	if(str == NULL) return 0;

	if(len == -1)
		len = strlen(str);

	if(p->len + 4 + 2 + len > p->size)
	{
		unsigned char *newptr;
		int alen;

		alen = (128 + len + 2) / 128 * 128;

		newptr = realloc(p->data, p->size + alen);
		if(newptr == NULL)
			return 0;

		p->size += alen;
		p->data = newptr;
	}

	p->data[p->len+4+0] = len / 256;
	p->data[p->len+4+1] = len % 256;
	memcpy(p->data + 4 + 2 + p->len, str, len);
	p->len += 2+len;

	return 1;
}


static uint16_t *aocPopWord(aocPacket *p)
{
	uint16_t *_ptr, _val;

	if(p->pos + 2 > p->len)
		return NULL;

	p->pos += 2;

	_ptr = (uint16_t *)(p->data + 4 + p->pos - 2);
	_val = ntohs(*_ptr);
	memcpy(_ptr, &_val, 2);	/* works on sparc :P */

	return _ptr;	
}


static uint32_t *aocPopInteger(aocPacket *p)
{
	uint32_t *_ptr, _val;

	if(p->pos + 4 > p->len)
		return NULL;

	p->pos += 4;

	_ptr = (uint32_t *)(p->data + 4 + p->pos - 4);
	_val = ntohl(*_ptr);
	memcpy(_ptr, &_val, 4);	/* works on sparc :P */

	return _ptr;
}


static char *aocPopString(aocPacket *p, int *_len)
{
	int len;

	if(p->pos + 2 > p->len)
		return NULL;

	len = AOC_WORDPTR(p->data + 4 + p->pos);	/* string length */

	if(p->pos + 2 + len > p->len)
		return NULL;

	if(_len)
		*_len = len;

	/* move the string to make room for 2 NULL-terminate chars
	   (the string length word will be overwritten..) */
	memmove(p->data + 4 + p->pos, p->data + 4 + p->pos + 2, len);
	p->data[4 + p->pos + len + 0] = 0;
	p->data[4 + p->pos + len + 1] = 0;

	p->pos += 2 + len;

	return (unsigned char *)(p->data + 4 + p->pos - len - 2);
}


static unsigned char *aocPopGroupId(aocPacket *p)
{
	if(p->pos + 5 > p->len)
		return NULL;

	p->pos += 5;

	return (unsigned char *)(p->data + 4 + p->pos - 5);
}


static unsigned char *aocPopRaw(aocPacket *p, int *_len)
{
	unsigned char *ret;

	if(p->pos >= p->len)
		return NULL;

	if(_len)
		*_len = p->len - p->pos;

	ret = (unsigned char *)p->data + 4 + p->pos;

	p->pos = p->len;

	return ret;
}


int aocPacketInit(aocPacket *p, int type)
{
	p->data = malloc(128);
	if(p->data == NULL)
		return 0;

	p->size = 128;
	p->len = 0;
	p->pos = 0;
	p->type = type;

	return 1;
}


/* sends a aocPacket, queues it if necessary. */
int aocPacketSend(aocConnection *aoc, aocPacket *p)
{
	int r;


	if(aoc == NULL)
		return -1;

	if(aoc->status != AOC_STAT_CONNECTED)
		return -1;

	/* write packet type */
	p->data[0] = p->type / 256;
	p->data[1] = p->type % 256;

	/* write packet length */
	p->data[2] = p->len / 256;
	p->data[3] = p->len % 256;

	/* correct packet size */
	p->len += 4;

	MAYBE_DEBUG
		aocDebugMsg("Sending packet type %d(AOC_CLI_%s) length %d.",
			p->type, aocClientPacketName(p->type), p->len);

	if(!aocSendQueueIsEmpty(aoc) ||
		(aoc->bw_limit > 0 && aoc->bw_remain == 0) ||
		(aoc->pkt_limit > 0 && aoc->pkt_remain == 0))
	{
		/*	queue is not empty ||
			no bandwidth left ||
			pkt limit reached
			.. queue entire packet */
		if(!aocSendQueuePacket(aoc, p, 0))
			goto exit_disconnect;
		set_sock_errno(0);
		return p->len;
	}

	if(aoc->bw_limit > 0 && aoc->bw_remain < p->len)
		/* not enough bandwidth to send the whole packet */
		r = send(aoc->socket, p->data, aoc->bw_remain, 0);
	else
		/* enough bandwidth to send the whole packet */
		r = send(aoc->socket, p->data, p->len, 0);

	if(r > 0 && r < p->len)
	{
		/* partial packet sent */
		aoc->bw_remain -= r;
		if(!aocSendQueuePacket(aoc, p, r))
			goto exit_disconnect;
		return p->len;
	}
	else if(r == -1 && (sock_errno == EAGAIN || sock_errno == EWOULDBLOCK ||
			sock_errno == ENOBUFS
#ifndef _WIN32
					|| sock_errno == EINTR
#endif
			))
	{
		/* nothing was sent due to some weird error, queue the packet */
		if(!aocSendQueuePacket(aoc, p, 0))
			goto exit_disconnect;
		set_sock_errno(0);
		return p->len;
	}

	free(p->data);

	if(r == -1 || r == 0)
	{
		set_sock_errno(0);
		goto exit_disconnect;
	}
	else
	{
		if(aoc->bw_limit > 0)
			aoc->bw_remain -= r;
		if(aoc->pkt_limit > 0)
			aoc->pkt_remain--;
	}

	return r;

exit_disconnect:;
	aoc->status = AOC_STAT_DISCONNECTED;
	aocEventAdd(aoc, AOC_EVENT_DISCONNECT, (void *)sock_errno);
	return -1;
}


int aocPacketArrayUnpack(aocPacket *p, aocMessage *m, int type)
{
	void *newptr;
	uint16_t *len;
	int i, n;

	/* get size of array */
	if( !(len = aocPopWord(p)) )
		return -1;

	n = *len + 1;


	m->argc += n;

	newptr = realloc(m->argt, sizeof(int)	* m->argc);
	if(newptr == NULL) return -1;
	m->argt = newptr;

	newptr = realloc(m->argl, sizeof(int)	* m->argc);
	if(newptr == NULL) return -1;
	m->argl = newptr;

	newptr = realloc(m->argv, sizeof(void *)	* m->argc);
	if(newptr == NULL) return -1;
	m->argv = newptr;

	/* array size */
	m->argt[m->argc-n] = AOC_TYPE_ARRAYSIZE;
	m->argl[m->argc-n] = 2;
	m->argv[m->argc-n] = (void *)len;

	for(i=(m->argc-n+1); i<m->argc; i++)
	{
		m->argt[i] = type;
		switch(type)
		{
		case AOC_TYPE_GROUPID:
			m->argl[i] = 5;
			m->argv[i] = (void *)aocPopGroupId(p);
			break;

		case AOC_TYPE_INTEGER:
			m->argl[i] = 4;
			m->argv[i] = (void *)aocPopInteger(p);
			break;

		case AOC_TYPE_STRING:
			m->argv[i] = (void *)aocPopString(p, &m->argl[i]);
			break;

		case AOC_TYPE_WORD:
			m->argl[i] = 4;
			m->argv[i] = (void *)aocPopInteger(p);
			break;
		}

		if(m->argv[i] == NULL)
			return -1;
	}

	return n;
}


int aocPacketUnpack(aocPacket *p, aocMessage *m, int type)
{
	void *newptr;

	newptr = realloc(m->argt, sizeof(int)	* (m->argc + 1));
	if(newptr == NULL) return -1;
	m->argt = newptr;

	newptr = realloc(m->argl, sizeof(int)	* (m->argc + 1));
	if(newptr == NULL) return -1;
	m->argl = newptr;

	newptr = realloc(m->argv, sizeof(void *)	* (m->argc + 1));
	if(newptr == NULL) return -1;
	m->argv = newptr;


	m->argt[ m->argc ] = type;
	switch(type)
	{
	case AOC_TYPE_GROUPID:
		m->argl[ m->argc ] = 5;
		m->argv[ m->argc ] = (void *)aocPopGroupId(p);
		break;

	case AOC_TYPE_INTEGER:
		m->argl[ m->argc ] = 4;
		m->argv[ m->argc ] = (void *)aocPopInteger(p);
		break;

	case AOC_TYPE_STRING:
		m->argv[ m->argc ] = (void *)aocPopString(p, &m->argl[ m->argc ]);
		break;

	case AOC_TYPE_WORD:
		m->argl[ m->argc ] = 2;
		m->argv[ m->argc ] = (void *)aocPopWord(p);
		break;

	case AOC_TYPE_RAW:
		m->argv[ m->argc ] = (void *)aocPopRaw(p, &m->argl[ m->argc ]);
		break;
	}


	if(m->argv[ m->argc ] == NULL)
		return 0;

	m->argc++;

	return 1;
}


/*
	fmt:
	g - group id
	i - 32bit unsigned integer
	l - string, convert to lowercase
	s - string
	w - 16bit unsigned integer
	r - raw binary data (all remaining data)
	G - array of g's
	I - array of i's
	L - array of l's
	S - array of s's
	W - array of w's
	R - can't be implemented

	inspired by aochat.php (from auno)
*/
int aocPacketDecode(aocPacket *p, aocMessage *m, const char *fmt)
{
	int err = 0, n;

	m->argc = 0;
	m->argt = NULL;
	m->argl = NULL;
	m->argv = NULL;

	while(*fmt)
	{
		switch(*fmt)
		{
		case 'g':
			if(!aocPacketUnpack(p, m, AOC_TYPE_GROUPID))
				err = 1;
			break;

		case 'i':
			if(!aocPacketUnpack(p, m, AOC_TYPE_INTEGER))
				err = 1;
			break;

		case 'l':
		case 's':
			if(!aocPacketUnpack(p, m, AOC_TYPE_STRING))
				err = 1;
			else if(*fmt == 'l')
				aocLowerCase((char *)m->argv[m->argc - 1]);
			break;

		case 'w':
			if(!aocPacketUnpack(p, m, AOC_TYPE_WORD))
				err = 1;
			break;

		case 'r':
			break;

		case 'G':
			if(aocPacketArrayUnpack(p, m, AOC_TYPE_GROUPID) == -1)
				err = 1;
			break;

		case 'I':
			if(aocPacketArrayUnpack(p, m, AOC_TYPE_INTEGER) == -1)
				err = 1;
			break;

		case 'L':
		case 'S':
			if((n = aocPacketArrayUnpack(p, m, AOC_TYPE_STRING)) == -1)
				err = 1;
			else if(*fmt == 'L')
			{
				n++;
				while(--n)
					aocLowerCase((char *)m->argv[m->argc - n]);
			}
			break;

		case 'W':
			if(aocPacketArrayUnpack(p, m, AOC_TYPE_WORD) == -1)
				err = 1;
			break;

		}

		if(err)
		{
			free(m->argt);
			free(m->argl);
			free(m->argv);
			m->argc = 0;
			m->argt = NULL;
			m->argl = NULL;
			m->argv = NULL;

			return -1;
		}

		fmt++;
	}

	return m->argc;
}


int aocPacketParse(aocConnection *aoc, unsigned char *buf, int len)
{
	aocMessage *m;
	aocPacket _p, *p = &_p;
	int err = 0;


	/* set up a "fake" packet so we can work with the aocPop* functions */
	p->len = len - 4;
	p->pos = 0;
	p->type = AOC_WORDPTR(buf);

	/* dupe packet data */
	p->data = aocMemDup(buf, len);
	if(p->data == NULL)
		return 0;

	m = malloc(sizeof(aocMessage));
	if(m == NULL)
	{
		free(p->data);
		return 0;
	}
	m->type = p->type;
	m->data = p->data;

	switch(p->type)
	{
	case AOC_SRV_LOGIN_SEED:		/* [string] */
		err = aocPacketDecode(p, m, "s");
		break;

	case AOC_SRV_LOGIN_OK:		/* - */
		err = aocPacketDecode(p, m, "");
		break;

	case AOC_SRV_LOGIN_ERROR:	/* [string] */
		err = aocPacketDecode(p, m, "s");
		break;

	case AOC_SRV_LOGIN_CHARLIST:		/* {[int]} {[string]} {[int]} {[int]} */
		err = aocPacketDecode(p, m, "ILII");
		break;

	case AOC_SRV_CLIENT_UNKNOWN:	/* [int] */
		err = aocPacketDecode(p, m, "i");
		break;

	case AOC_SRV_CLIENT_NAME:		/* [int] [string] */
		err = aocPacketDecode(p, m, "il");
		if(err != -1) aocMsgQueueLookupCallback(aoc, m);
		break;

	case AOC_SRV_LOOKUP_RESULT:	/* [int] [string] */
		err = aocPacketDecode(p, m, "il");
		if(err != -1) aocMsgQueueLookupCallback(aoc, m);
		break;

	case AOC_SRV_PRIVATE_MSG:		/* [int] [string] [string] */
		err = aocPacketDecode(p, m, "iss");
		break;

	case AOC_SRV_VICINITY_MSG:	/* [int] [string] [string] */
		err = aocPacketDecode(p, m, "iss");
		break;

	case AOC_SRV_ANONVICINITY_MSG:	/* [string] [string] [string] */
		err = aocPacketDecode(p, m, "lss");
		break;

	case AOC_SRV_SYSTEM_MSG:		/* [string] */
		err = aocPacketDecode(p, m, "s");
		break;

	case AOC_SRV_BUDDY_STATUS:		/* [int] [int] [string] */
		err = aocPacketDecode(p, m, "iis");
		break;

	case AOC_SRV_BUDDY_REMOVED:	/* [int] */
		err = aocPacketDecode(p, m, "i");
		break;

	case AOC_SRV_PRIVGRP_INVITE:	/* [int] */
		err = aocPacketDecode(p, m, "i");
		break;

	case AOC_SRV_PRIVGRP_KICK:		/* [int] */
		err = aocPacketDecode(p, m, "i");
		break;

	case AOC_SRV_PRIVGRP_PART:		/* [int] */
		err = aocPacketDecode(p, m, "i");
		break;

	case AOC_SRV_PRIVGRP_CLIJOIN:	/* [int] [int] */
		err = aocPacketDecode(p, m, "ii");
		break;

	case AOC_SRV_PRIVGRP_CLIPART:	/* [int] [int] */
		err = aocPacketDecode(p, m, "ii");
		break;

	case AOC_SRV_PRIVGRP_MSG:		/* [int] [int] [string] [string] */
		err = aocPacketDecode(p, m, "iiss");
		break;

	case AOC_SRV_GROUP_JOIN:		/* [grp] [string] [word] [word] [string] */
		err = aocPacketDecode(p, m, "gswws");
		break;

	case AOC_SRV_GROUP_PART:		/* [grp] */
		err = aocPacketDecode(p, m, "g");
		break;

	case AOC_SRV_GROUP_MSG:		/* [grp] [int] [string] [string] */
		err = aocPacketDecode(p, m, "giss");
		break;

	case AOC_SRV_PONG:			/* [string] */
		err = aocPacketDecode(p, m, "s");
		break;

	case AOC_SRV_FORWARD:		/* [raw] */
		err = aocPacketDecode(p, m, "r");
		break;

	case AOC_SRV_AMD_MUX_INFO:	/* {[int]} {[int]} {[int]} */
		err = aocPacketDecode(p, m, "III");
		break;

	default:
		/* unknown/unhandled message */
		m->argc = 0;
		m->argt = NULL;
		m->argl = NULL;
		m->argv = NULL;
		aocEventAdd(aoc, AOC_EVENT_UNHANDLED, (void *)m);

		MAYBE_DEBUG
			aocDebugMsg("Received unknown packet type %d length %d.",
				p->type, p->len + 4);
		return 0;
	}


	if(err == -1)
	{
		/* decode failed */
		free(m->data);
		m->data = aocMemDup(buf, len);
		if(m->data == NULL)
			return 0;

		aocEventAdd(aoc, AOC_EVENT_UNHANDLED, (void *)m);

		MAYBE_DEBUG
			aocDebugMsg("Received invalid packet type %d(AOC_SRV_%s) length %d.",
				p->type, aocServerPacketName(p->type), p->len);
		return 0;
	}

	aocEventAdd(aoc, AOC_EVENT_MESSAGE, (void *)m);

	MAYBE_DEBUG
		aocDebugMsg("Received packet type %d(AOC_SRV_%s) length %d (%d bytes not decoded).",
			p->type, aocServerPacketName(p->type), p->len, p->len - p->pos);

	return 1;
}


int aocPacketRead(aocConnection *aoc)
{
	unsigned char *buf;
	int n, plen;

	if(aoc == NULL)
		return -1;
	if(aoc->status != AOC_STAT_CONNECTED)
		return -1;

	while(1)
	{
		n = recv(aoc->socket,
			aoc->read_buf + aoc->read_buf_len,
			aoc->read_buf_size - aoc->read_buf_len,
			0);

		if(n == 0)
		{
			aoc->status = AOC_STAT_DISCONNECTED;
			aocEventAdd(aoc, AOC_EVENT_DISCONNECT, (void *)0);
			return 0;
		}
		else if(n < 0)
		{
			if(sock_errno == EWOULDBLOCK
#ifndef _WIN32
					|| sock_errno == EINTR
#endif
			)
				break;

			aoc->status = AOC_STAT_DISCONNECTED;
			aocEventAdd(aoc, AOC_EVENT_DISCONNECT, (n == 0) ? (void *)0 : (void *)sock_errno);
			return 0;
		}

		aoc->read_buf_len += n;

		if(aoc->read_buf_len < 4)
			return 1;

		plen = AOC_WORDPTR(aoc->read_buf+2) + 4;

		if(aoc->read_buf_len == aoc->read_buf_size &&
			plen > aoc->read_buf_size)
		{
			void *newptr;

			/*	* recv() filled up our buffer, so there
				  is most likely more data to read
				* packet is larger than our buffer

				resize the buffer, and call recv() again */
			newptr = realloc(aoc->read_buf, aoc->read_buf_size + plen);
			if(newptr == NULL)
				return 1;

			aoc->read_buf_size += plen;
			aoc->read_buf = newptr;
			/* call recv() again */
			continue;
		}

		break;
	}

	/* parse any packets we have received */
	buf = aoc->read_buf;
	while(aoc->read_buf_len >= 4)
	{
		plen = AOC_WORDPTR(buf+2) + 4;

		if(aoc->read_buf_len < plen)
			break;

		aocPacketParse(aoc, buf, plen);

		aoc->read_buf_len -= plen;
		buf += plen;
	}

	if(aoc->read_buf != buf && aoc->read_buf_len > 0)
		memmove(aoc->read_buf, buf, aoc->read_buf_len);

	if(aoc->read_buf_size != AOC_READ_BUF_SIZE && aoc->read_buf_len < AOC_READ_BUF_SIZE)
	{
		aoc->read_buf_size = AOC_READ_BUF_SIZE;
		aoc->read_buf = realloc(aoc->read_buf, AOC_READ_BUF_SIZE);
	}

	return 1;
}


int aocSendLoginResponse(aocConnection *aoc, uint32_t _zero, const char *name, const char *key)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_LOGIN_RESPONSE);
	aocPushInteger(&p, _zero);
	aocPushString(&p, name, -1);
	aocPushString(&p, key, -1);

	return aocPacketSend(aoc, &p);
}


int aocSendLoginSelectChar(aocConnection *aoc, uint32_t user_id)
{
	aocPacket p;

	if(aoc->selchar_done)
		return -1;
	aoc->selchar_done = 1;

	aocPacketInit(&p, AOC_CLI_LOGIN_SELCHAR);
	aocPushInteger(&p, user_id);

	return aocPacketSend(aoc, &p);
}


int aocSendNameLookup(aocConnection *aoc, const char *name)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_NAME_LOOKUP);
	aocPushString(&p, aocNameTrim(name), -1);

	return aocPacketSend(aoc, &p);
}


int aocSendPrivateMessage(aocConnection *aoc, uint32_t user_id, const char *text, int text_len, const unsigned char *blob, int blob_len)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_PRIVATE_MSG);
	aocPushInteger(&p, user_id);
	aocPushString(&p, text, text_len);
	aocPushString(&p, blob, blob_len);

	return aocPacketSend(aoc, &p);
}


int aocSendBuddyAdd(aocConnection *aoc, uint32_t user_id, const unsigned char *blob, int blob_len)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_BUDDY_ADD);
	aocPushInteger(&p, user_id);
	aocPushString(&p, blob, blob_len);

	return aocPacketSend(aoc, &p);
}


int aocSendBuddyRemove(aocConnection *aoc, uint32_t user_id)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_BUDDY_REMOVE);
	aocPushInteger(&p, user_id);

	return aocPacketSend(aoc, &p);
}


int aocSendOnlineStatus(aocConnection *aoc, uint32_t status)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_ONLINE_STATUS);
	aocPushInteger(&p, status);

	return aocPacketSend(aoc, &p);
}


int aocSendPrivateGroupInvite(aocConnection *aoc, uint32_t user_id)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_PRIVGRP_INVITE);
	aocPushInteger(&p, user_id);

	return aocPacketSend(aoc, &p);
}


int aocSendPrivateGroupKick(aocConnection *aoc, uint32_t user_id)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_PRIVGRP_KICK);
	aocPushInteger(&p, user_id);

	return aocPacketSend(aoc, &p);
}


int aocSendPrivateGroupJoin(aocConnection *aoc, uint32_t user_id)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_PRIVGRP_JOIN);
	aocPushInteger(&p, user_id);

	return aocPacketSend(aoc, &p);
}


int aocSendPrivateGroupKickAll(aocConnection *aoc)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_PRIVGRP_KICKALL);

	return aocPacketSend(aoc, &p);
}


int aocSendPrivateGroupMessage(aocConnection *aoc, uint32_t user_id, const char *text, int text_len, const unsigned char *blob, int blob_len)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_PRIVGRP_MSG);
	aocPushInteger(&p, user_id);
	aocPushString(&p, text, text_len);
	aocPushString(&p, blob, blob_len);

	return aocPacketSend(aoc, &p);
}


int aocSendGroupDataset(aocConnection *aoc, const unsigned char *group_id, uint16_t flags, uint32_t _unknown)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_GROUP_DATASET);
	aocPushGroupId(&p, group_id);
	aocPushWord(&p, flags);
	aocPushInteger(&p, _unknown);

	return aocPacketSend(aoc, &p);
}


int aocSendGroupMessage(aocConnection *aoc, const unsigned char *group_id, const char *text, int text_len, const unsigned char *blob, int blob_len)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_GROUP_MESSAGE);
	aocPushGroupId(&p, group_id);
	aocPushString(&p, text, text_len);
	aocPushString(&p, blob, blob_len);

	return aocPacketSend(aoc, &p);
}


int aocSendGroupClimode(aocConnection *aoc, const unsigned char *group_id, uint32_t _unknown1, uint32_t _unknown2, uint32_t _unknown3, uint32_t _unknown4)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_GROUP_CLIMODE);
	aocPushGroupId(&p, group_id);
	aocPushInteger(&p, _unknown1);
	aocPushInteger(&p, _unknown2);
	aocPushInteger(&p, _unknown3);
	aocPushInteger(&p, _unknown4);

	return aocPacketSend(aoc, &p);
}


int aocSendPing(aocConnection *aoc, const unsigned char *blob, int blob_len)
{
	aocPacket p;

	aocPacketInit(&p, AOC_CLI_PING);
	aocPushString(&p, blob, blob_len);

	return aocPacketSend(aoc, &p);
}


int aocSendChatCommand(aocConnection *aoc, const char *command)
{
	const char *ptr, *pptr;
	aocPacket p;
	int asize = 0;


	aocPacketInit(&p, AOC_CLI_CHAT_COMMAND);
	aocPushWord(&p, 0);	/* array size */

	ptr = command;
	while(1)
	{
		/* skip junk spaces */
		while(*ptr == ' ')
			ptr++;

		pptr = ptr;

		/* find next space */
		ptr = strchr(pptr, ' ');
		if(ptr == NULL)
		{
			if(*pptr != 0)
			{
				asize++;
				aocPushString(&p, pptr, strlen(pptr));
			}
			break;
		}

		asize++;
		aocPushString(&p, pptr, ptr - pptr);
	}

	/* hack - change array size */
	asize = htons(asize);
	memcpy(p.data + 4, &asize, 2);

	return aocPacketSend(aoc, &p);
}


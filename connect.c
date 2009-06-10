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


aocConnection *aocInit(aocConnection *aoc)
{
	if(aoc == NULL)
		aoc = malloc(sizeof(aocConnection));

	if(aoc == NULL)
		return NULL;

	memset(aoc, 0, sizeof(aocConnection));

	aoc->socket = -1;
	aoc->status = AOC_STAT_DISCONNECTED;

	aoc->read_buf_size = AOC_READ_BUF_SIZE;
	aoc->read_buf = malloc(AOC_READ_BUF_SIZE);
	if(aoc->read_buf == NULL)
	{
		free(aoc);
		return NULL;
	}

	aoc->bw_limit = _aoc_pref.bwlimit;
	aoc->bw_remain = _aoc_pref.bwlimit;

	aoc->pkt_limit = _aoc_pref.pktlimit;
	aoc->pkt_remain = _aoc_pref.pktlimit;

	if(aoc->bw_limit > 0 || aoc->pkt_limit > 0)
	{
		struct timeval tv;

		gettimeofday(&tv, NULL);
		if(tv.tv_usec > 0)
		{
			tv.tv_sec++;
			tv.tv_usec = 0;
		}
		aocTimerNew(aoc, AOC_ITIMER_LIMITER, 1, 0, &tv);
	}

	return aoc;
}


int aocConnect(aocConnection *aoc, const struct sockaddr_in *addr)
{
	if(aoc == NULL) return 0;
	if(addr == NULL) return 0;

	aoc->socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(aoc->socket == -1)
		return 0;

	aocSocketSetAsync(aoc->socket);

	if(connect(aoc->socket, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1)
	{
		if(! (sock_errno == EINPROGRESS || sock_errno == EWOULDBLOCK))
			return 0;
	}

	aoc->status = AOC_STAT_CONNECTING;

	return 1;
}


void aocDisconnect(aocConnection *aoc)
{
	if(aoc == NULL) return;

	if(aoc->socket != -1)
#ifdef _WIN32
		closesocket(aoc->socket);
#else
		close(aoc->socket);
#endif
	aoc->socket = -1;
	aoc->status = AOC_STAT_DISCONNECTED;

	if(aoc->read_buf != NULL)
		free(aoc->read_buf);
	aoc->read_buf = NULL;

	aocSendQueueEmpty(aoc);
	aocEventDestroyByAoc(aoc);
	aocTimerDestroyByAoc(aoc);
	aocMsgQueueDestroy(aoc);

	return;
}


int aocPollQuery(aocConnection *aoc)
{
	int flags = 0;

	if(aoc == NULL) return 0;

	if(aoc->status == AOC_STAT_CONNECTED)
		flags |= AOC_POLL_READ;

	if(aoc->status == AOC_STAT_CONNECTING)
	{
		flags |= AOC_POLL_WRITE;
	}
	else if(!aocSendQueueIsEmpty(aoc))
	{
		if(aoc->bw_limit > 0 && aoc->bw_remain == 0)
			return flags;	/* no bandwidth left */
		if(aoc->pkt_limit > 0 && aoc->pkt_remain == 0)
			return flags;	/* can't send more packets */

		flags |= AOC_POLL_WRITE;
	}

	return flags;
}


void aocPollCanRead(aocConnection *aoc)
{
	if(aoc == NULL) return;

	aocPacketRead(aoc);
	return;
}


void aocPollCanWrite(aocConnection *aoc)
{
	if(aoc == NULL) return;

	if(aoc->status == AOC_STAT_CONNECTING)
	{
		/*socklen_t len = sizeof(int);*/
		unsigned int val = 0, len = sizeof(int);

		if(getsockopt(aoc->socket, SOL_SOCKET, SO_ERROR, &val, &len) == -1)
		{
			aocEventAdd(aoc, AOC_EVENT_CONNFAIL, (void *)0);
			return;
		}

		if(val != 0)
		{
			aocEventAdd(aoc, AOC_EVENT_CONNFAIL, (void *)val);
			return;
		}

		aoc->status = AOC_STAT_CONNECTED;
		aocEventAdd(aoc, AOC_EVENT_CONNECT, (void *)0);
	}
	else
	{
		if(!aocSendQueuePoll(aoc))
			aocEventAdd(aoc, AOC_EVENT_DISCONNECT, (void *)sock_errno);
	}
	return;
}


/* This poll function is very inefficient, but it works well
   since most applications use very few connections...
   so, um.. yeah. it's a pretty fucking excellent poll function. */
int aocPollArray(long sec, long usec, int num, aocConnection **c)
{
	fd_set fd_read, fd_write;
	struct timeval tv;
	int high = -1, nw = 0, n, i, flags = 0;

	if(c == NULL)
		return 1;

	tv.tv_sec = sec;
	tv.tv_usec = usec;

	aocTimerPoll();
	aocTimerMaxTv(&tv);

	FD_ZERO(&fd_read);
	FD_ZERO(&fd_write);
	for(i=0; i<num; i++)
	{
		if(c[i] == NULL)
			continue;

		flags = aocPollQuery(c[i]);

		if(flags & AOC_POLL_READ)
		{
			FD_SET(c[i]->socket, &fd_read);

			if(c[i]->socket > high)
				high = c[i]->socket;
		}

		if(flags & AOC_POLL_WRITE)
		{
			FD_SET(c[i]->socket, &fd_write);
			nw++;

			if(c[i]->socket > high)
				high = c[i]->socket;
		}
	}

	if(nw > 0)
		n = select(high+1, &fd_read, &fd_write, NULL, &tv);
	else
		n = select(high+1, &fd_read, NULL, NULL, &tv);

	if(n < 0)
		return 0;

	for(i=0; i<num; i++)
	{
		if(c[i] == NULL)
			continue;

		if(FD_ISSET(c[i]->socket, &fd_read))
			aocPollCanRead(c[i]);
		if(FD_ISSET(c[i]->socket, &fd_write))
			aocPollCanWrite(c[i]);
	}

	return 1;
}


/* cheese, anyone? */
int aocPollVarArg(long sec, long usec, int num, ...)
{
	aocConnection **c;
	va_list va;
	int i, r;

	c = malloc(sizeof(void *) * num);
	if(c == NULL)
		return 0;

	va_start(va, num);
	for(i=0; i<num; i++)
		c[i] = va_arg(va, aocConnection *);
	va_end(va);

	r = aocPollArray(sec, usec, num, c);
	free(c);

	return r;
}


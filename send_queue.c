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


int aocSendQueuePacket(aocConnection *aoc, aocPacket *p, int sent)
{
	aocSendQueue *q;

	aoc->send_queue_size += p->len - sent;
	if(aoc->send_queue_max > 0 &&
		aoc->send_queue_size > aoc->send_queue_max)
	{
		set_sock_errno(0);
		return 0;
	}

	q = malloc(sizeof(aocSendQueue));
	if(q == NULL)
		return 0;

	if(aoc->send_queue == NULL)
	{
		aoc->send_queue = q;
		aoc->send_queue_last = q;
	}
	else
	{
		aoc->send_queue_last->next = q;
		aoc->send_queue_last = q;
	}

	q->next = NULL;
	q->len = p->len;
	q->packet = p->data;
	q->sent = sent;

	MAYBE_DEBUG
		aocDebugMsg("Queue packet: length %d, sent %d, queuesize %d, max %d.",
			q->len, q->sent, aoc->send_queue_size, aoc->send_queue_max);

	return 1;
}


int aocSendQueuePoll(aocConnection *aoc)
{
	aocSendQueue *q, *tmp;
	int r;

	q = aoc->send_queue;
	while(q != NULL)
	{
		if(aoc->bw_limit > 0 && aoc->bw_remain == 0)
			break;	/* no more bandwidth */

		if(aoc->pkt_limit > 0 && aoc->pkt_remain == 0)
			break;	/* packet limit reached */

		if( aoc->bw_limit > 0 && aoc->bw_remain < (q->len - q->sent) )
			r = send(aoc->socket, q->packet + q->sent, aoc->bw_remain, 0);
		else
			r = send(aoc->socket, q->packet + q->sent, q->len - q->sent, 0);

		if(r >= 0 && r < (q->len - q->sent))
		{
			/* partial packet sent, decrease bandwidth counter and queue size */
			if(aoc->bw_limit > 0)
				aoc->bw_remain -= r;
			aoc->send_queue_size -= r;
			q->sent += r;
			return 1;
		}
		else if(r < 0)
		{
			if(sock_errno == EAGAIN || sock_errno == EWOULDBLOCK ||
				sock_errno == ENOBUFS
#ifndef _WIN32
				|| sock_errno == EINTR
#endif
			)
			{
				/* could not send (no fatal error though) */
				return 1;
			}
			/* fatal send error */
			aocSendQueueEmpty(aoc);
			return 0;
		}

		/* packet sent, decrease bandwidth counter and packet counter */
		if(aoc->bw_limit > 0)
			aoc->bw_remain -= r;

		if(aoc->pkt_limit > 0)
			aoc->pkt_remain--;

		aoc->send_queue_size -= r;

		/* remove packet from queue */
		tmp = q->next;
		free(q->packet);
		free(q);
		q = aoc->send_queue = tmp;
		if(q == NULL)
			aoc->send_queue_last = NULL;
	}

	return 1;
}


void aocSendQueueEmpty(aocConnection *aoc)
{
	aocSendQueue *q, *tmp;

	q = aoc->send_queue;
	while(q)
	{
		tmp = q->next;
		free(q->packet);
		free(q);
		q = tmp;
	}

	aoc->send_queue = NULL;
	aoc->send_queue_last = NULL;
	aoc->send_queue_size = 0;
}


int aocSendQueueIsEmpty(aocConnection *aoc)
{
	return aoc->send_queue ? 0 : 1;
}


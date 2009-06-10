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

/*	Fairly decent timer functions.
	Delete / polling is O(1)
	Average insert time is O(ceil(n/2)) */


/* timer list, sorted by aocTimer->tv */
aocTimer *_aoc_tlist_start = NULL;
aocTimer *_aoc_tlist_end = NULL;


/* move this elsewhere? naaah. */
void aocTimerInternalCallback(aocTimer *timer)
{
	switch(timer->id)
	{
	case AOC_ITIMER_LIMITER:
		if(timer->aoc->status != AOC_STAT_CONNECTED)
			break;

		if(timer->aoc->bw_remain == 0 || timer->aoc->pkt_remain == 0)
		{
			/* connection used up all bandwidth.. try sending more */
			timer->aoc->bw_remain = timer->aoc->bw_limit;
			timer->aoc->pkt_remain = timer->aoc->pkt_limit;
			aocPollCanWrite(timer->aoc);

			/* hmm.. */
			/*if(!aocSendQueueIsEmpty(timer->aoc) &&
				(	(timer->aoc->bw_limit > 0 && timer->aoc->bw_remain != 0) ||
					(timer->aoc->pkt_limit > 0 && timer->aoc->pkt_remain != 0)	))
			{
				aocDebugMsg("POLL UPDATE REQUIRED!");
				aocEventAdd(timer->aoc, AOC_EVENT_POLLUPD, NULL);
			}*/
		}
		else
		{
			/* give connection more bandwidth */
			timer->aoc->bw_remain = timer->aoc->bw_limit;
			timer->aoc->pkt_remain = timer->aoc->pkt_limit;
		}
		break;

	case AOC_ITIMER_MSGQUEUE:
		aocMsgQueueTick(timer->aoc, 1);
		break;
	}
}


/* removes a timer from the linked list */
static void aocTimerUnchain(aocTimer *timer)
{
	if(timer->next != NULL)
		timer->next->prev = timer->prev;
	else
		_aoc_tlist_end = timer->prev;

	if(timer->prev != NULL)
		timer->prev->next = timer->next;
	else
		_aoc_tlist_start = timer->next;
}


/* inserts a timer in the linked list */
static void aocTimerChain(aocTimer *where, aocTimer *timer)
{
	if(where == NULL)
	{
		timer->prev = NULL;
		timer->next = _aoc_tlist_start;

		if(_aoc_tlist_start != NULL)
			_aoc_tlist_start->prev = timer;
		_aoc_tlist_start = timer;

		if(_aoc_tlist_end == NULL)
			_aoc_tlist_end = timer;
	}
	else
	{
		if(where->next == NULL)
			_aoc_tlist_end = timer;

		timer->prev = where;
		timer->next = where->next;
		where->next = timer;

		if(timer->next)
			timer->next->prev = timer;
	}
}


void aocTimerMaxTv(struct timeval *tv)
{
	struct timeval now;

	if(_aoc_tlist_start == NULL)
		return;			/* no active timers */

	gettimeofday(&now, NULL);

	AOC_TV_ADD(tv, &now);	/* offset tv with current time */

	if(AOC_TV_CMP(tv, >, &_aoc_tlist_start->tv))
		AOC_TV_SET(tv, &_aoc_tlist_start->tv);

	AOC_TV_SUB(tv, &now);	/* revert offset */

	if(tv->tv_sec < 0)
		tv->tv_sec = 0;
	if(tv->tv_usec < 0)
		tv->tv_usec = 0;

	return;
}


int aocTimerMax(int timeout)
{
	struct timeval now;
	int diff;

	if(_aoc_tlist_start == NULL)
		return timeout;

	gettimeofday(&now, NULL);

	diff = AOC_TV_UDIFF(&_aoc_tlist_start->tv, &now) / 1000;

	if(timeout > diff)
		return diff;

	return timeout;
}


/* inserts a timer in the timer queue (sorted) */
void aocTimerInsertSort(aocTimer *timer, const struct timeval *now)
{
	aocTimer *where = NULL;


	AOC_TV_ADD(&timer->tv, &timer->interval);

	/* handle timer underruns.. (slow) */
	timer->underruns = 0;
	while(AOC_TV_CMP(now, >, &timer->tv))
	{
		AOC_TV_ADD(&timer->tv, &timer->interval);
		timer->underruns++;
	}

	if(_aoc_tlist_start)
	{
		aocTimer *t;
		int dl, dh;

		dl = abs(AOC_TV_UDIFF(&timer->tv, &_aoc_tlist_start->tv));
		dh = abs(AOC_TV_UDIFF(&timer->tv, &_aoc_tlist_end->tv));

		/* pick closest end */
		if(dl > dh)
		{
			/* end to start search */
			t = _aoc_tlist_end;
			while(t)
			{
				where = t;
				if(AOC_TV_CMP(&t->tv, <, &timer->tv)) break;
				t = t->prev;
			}
		}
		else
		{
			/* start to end search */
			t = _aoc_tlist_start;
			while(t)
			{
				if(AOC_TV_CMP(&t->tv, >, &timer->tv)) break;
				where = t;
				t = t->next;
			}
		}
	}

	aocTimerChain(where, timer);

	return;
}


void aocTimerPoll()
{
	struct timeval now;
	aocTimer *timer;


	if(_aoc_tlist_start == NULL)
		return;			/* no active timers */

	gettimeofday(&now, NULL);

	while(1)
	{
		if(AOC_TV_CMP(&now, <, &_aoc_tlist_start->tv))
			break;

		if(_aoc_tlist_start->id >= 0)
		{
			aocEventAdd(_aoc_tlist_start->aoc, AOC_EVENT_TIMER, (void *)_aoc_tlist_start);
		}
		else
		{
			/* internal timer */
			timer = _aoc_tlist_start;
			aocTimerInternalCallback(_aoc_tlist_start);
			if(_aoc_tlist_start == NULL)
				break;
			if(_aoc_tlist_start != timer)
				continue;
		}

		timer = _aoc_tlist_start;
		aocTimerUnchain(_aoc_tlist_start);
		aocTimerInsertSort(timer, &now);
	}
}


aocTimer *aocTimerNew(aocConnection *aoc, int id, long sec, long usec, const struct timeval *tv)
{
	aocTimer *tnew;

	if(sec < 0)
		return NULL;
	if(usec < 0 || usec > 999999)
		return NULL;
	if(sec == 0 && usec < 50000)
		return NULL;

	tnew = malloc(sizeof(aocTimer));
	if(tnew == NULL)
		return NULL;

	tnew->aoc = aoc;
	tnew->id = id;
	tnew->underruns = 0;
	tnew->interval.tv_sec = sec;
	tnew->interval.tv_usec = (usec / 50000) * 50000;

	if(tv == NULL)
	{
		gettimeofday(&tnew->tv, NULL);
		tnew->tv.tv_usec = ((tnew->tv.tv_usec + 25000) / 50000) * 50000;
	}
	else
	{
		AOC_TV_SET(&tnew->tv, tv);
	}

	tnew->prev = NULL;
	tnew->next = NULL;
	aocTimerInsertSort(tnew, &tnew->tv);

	return tnew;
}


void aocTimerDestroy(aocTimer *timer)
{
	aocTimerUnchain(timer);
	free(timer);

	return;
}


void aocTimerDestroyByAoc(aocConnection *aoc)
{
	aocTimer *timer, *timer_next;

	timer = _aoc_tlist_start;
	while(timer != NULL)
	{
		timer_next = timer->next;

		if(timer->aoc == aoc)
			aocTimerDestroy(timer);

		timer = timer_next;
	}

	return;
}

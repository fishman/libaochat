/* libaochat -- Anarchy Online Chat Library
   Copyright (c) 2003 Andreas Allerdahl <dinkles@tty0.org>.
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


aocEvent *_aoc_event_start = NULL;
aocEvent *_aoc_event_end = NULL;


aocEvent *aocEventGet()
{
	aocEvent *e;

	if(_aoc_event_start == NULL)
		return NULL;

	e = _aoc_event_start;

	_aoc_event_start = _aoc_event_start->next;
	if(_aoc_event_start == NULL)
		_aoc_event_end = NULL;

	return e;
}


void aocEventDestroy(aocEvent *e)
{
	if(e == NULL) return;

	if(	(e->type == AOC_EVENT_MESSAGE ||
		 e->type == AOC_EVENT_UNHANDLED)
		&& e->data != NULL)
	{
		aocMessage *msg = (aocMessage *)e->data;

		if(msg->data)
			free(msg->data);
		if(msg->argt)
			free(msg->argt);
		if(msg->argl)
			free(msg->argl);
		if(msg->argv)
			free(msg->argv);
		free(msg);
	}

	free(e);

	return;
}


int aocEventAdd(aocConnection *aoc, int type, void *data)
{
	aocEvent *ev;

	ev = malloc(sizeof(aocEvent));
	if(ev == NULL)
		return 0;

	if(_aoc_event_start == NULL)
	{
		_aoc_event_start = ev;
		_aoc_event_end = ev;
	}
	else
	{
		_aoc_event_end->next = ev;
		_aoc_event_end = ev;
	}

	ev->next = NULL;
	ev->aoc = aoc;
	ev->type = type;
	ev->data = data;

	return 1;
}


void aocEventDestroyByAoc(aocConnection *aoc)
{
	aocEvent *prev = NULL, *next, *ev;

	ev = _aoc_event_start;
	while(ev)
	{
		next = ev->next;

		if(ev->aoc == aoc)
		{
			if(prev == NULL)		/* same as.. if(ev == _aoc_event_start) */
			{
				_aoc_event_start = next;
				if(_aoc_event_start == NULL)
					_aoc_event_end = NULL;
			}
			else
			{
				prev->next = next;
			}

			aocEventDestroy(ev);
		}
		else
		{
			prev = ev;
		}

		ev = next;
	}

	return;
}


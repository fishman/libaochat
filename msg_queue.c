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


/* called internally from packet.c */
void aocMsgQueueLookupCallback(aocConnection *aoc, aocMessage *msg)
{
	static uint32_t unique = 0;

	if(aoc->mq.namelist == NULL)
		return;

	if(AOC_INT(msg->argv[0]) != AOC_INVALID_UID)
		return;

	/* unknown charname, add to bad names list */
	if(unique == AOC_INVALID_UID)
		unique++;
	aocNameListInsert(aoc->mq.badlist, unique++, AOC_STR(msg->argv[1]), NULL);

	aocMsgQueueTick(aoc, 0);
}


void aocMsgQueueTick(aocConnection *aoc, int from_timer)
{
	uint32_t uid;
	int qid;


	if(!aoc->selchar_done)
		return;	/* not logged in yet */

	if(from_timer)
	{
		if(aoc->mq.burst > 0)
			aoc->mq.burst--;
	}

	/* send out tells/grpmsgs */
	for(qid=0; qid<2; qid++)
	{
		while(aoc->mq.burst < 3)
		{
			aocMQMsg *msg = aoc->mq.msg_start[qid];
			aocMQMsg *next;

			if(msg == NULL)
				break;

			next = msg->next;

			switch(msg->type)
			{
			case 0:
				aocSendPrivateMessage(aoc, msg->dest.uid, msg->text, msg->text_len, msg->blob, msg->blob_len);
				break;

			case 1:
				aocSendGroupMessage(aoc, msg->dest.group_id, msg->text, msg->text_len, msg->blob, msg->blob_len);
				break;

			case 2:
				uid = aocNameListLookupByName(aoc->mq.namelist, msg->dest.name, NULL);
				if(uid != AOC_INVALID_UID)
					aocSendPrivateMessage(aoc, uid, msg->text, msg->text_len, msg->blob, msg->blob_len);
				else if(aocNameListLookupByName(aoc->mq.badlist, msg->dest.name, NULL) == AOC_INVALID_UID)
					return;	/* user id not known yet */
			}

			aoc->mq.burst++;

			free(msg->text);
			free(msg->blob);
			free(msg);

			aoc->mq.msg_start[qid] = next;
			if(next == NULL)
				aoc->mq.msg_end[qid] = NULL;
		}
	}


	if(aoc->mq.burst == 0 && aoc->mq.msg_start[0] == NULL && aoc->mq.msg_start[1] == NULL)
	{
		/* the burst has recharged, and there are no more queued messages */
		if(aoc->mq.timer != NULL)
			aocTimerDestroy(aoc->mq.timer);
		aoc->mq.timer = NULL;

		aocNameListDestroy(aoc->mq.badlist);
		aoc->mq.badlist = aocNameListNew(128);
	}

	return;
}


void aocMsgQueueSetNameList(aocConnection *aoc, aocNameList *namelist)
{
	aoc->mq.namelist = namelist;
	if(aoc->mq.badlist == NULL)
		aoc->mq.badlist = aocNameListNew(128);
}


int aocMsgQueueInsert(aocConnection *aoc, aocMQMsg *msg, int prio,
		const char *text, int text_len, const unsigned char *blob, int blob_len)
{
	int qid;


	if(text_len == -1) text_len = strlen(text);
	if(blob_len == -1) blob_len = strlen(blob);

	msg->text = aocMemDup(text, text_len+1);
	msg->text_len = text_len;
	if(msg->text == NULL)
		goto tq_add_error;

	msg->blob = aocMemDup(blob, blob_len+1);
	msg->blob_len = blob_len;
	if(msg->blob == NULL)
		goto tq_add_error;


	/* start timer if it's not active */
	if(aoc->mq.timer == NULL)
	{
		aoc->mq.timer = aocTimerNew(aoc, AOC_ITIMER_MSGQUEUE, 2, 0, NULL);
		if(aoc->mq.timer == NULL)
			goto tq_add_error;
	}
	
	/* insert into queue */
	qid = (prio == AOC_PRIO_HIGH) ? 0 : 1;

	if(aoc->mq.msg_start[qid] == NULL)
		aoc->mq.msg_start[qid] = msg;
	else
		aoc->mq.msg_end[qid]->next = msg;
	aoc->mq.msg_end[qid] = msg;
	msg->next = NULL;

	/* tick */
	aocMsgQueueTick(aoc, 0);

	return 1;

tq_add_error:;
	if(msg->blob != NULL)	free(msg->blob);
	if(msg->text != NULL)	free(msg->text);
	if(msg != NULL)			free(msg);

	return 0;
}


int aocMsgQueueTellUID(aocConnection *aoc, uint32_t uid, int prio, 
	const char *text, int text_len, const unsigned char *blob, int blob_len)
{
	aocMQMsg *msg;

	msg = malloc(sizeof(aocMQMsg));
	if(msg == NULL)
		return 0;

	msg->type = 0;
	msg->dest.uid = uid;

	return aocMsgQueueInsert(aoc, msg, prio, text, text_len, blob, blob_len);
}


int aocMsgQueueGroup(aocConnection *aoc, const unsigned char *group_id, int prio, 
	const char *text, int text_len, const unsigned char *blob, int blob_len)
{
	aocMQMsg *msg;

	msg = malloc(sizeof(aocMQMsg));
	if(msg == NULL)
		return 0;

	msg->type = 0;
	memcpy(msg->dest.group_id, group_id, 5);

	return aocMsgQueueInsert(aoc, msg, prio, text, text_len, blob, blob_len);
}


int aocMsgQueueTell(aocConnection *aoc, const char *name, int prio, 
	const char *text, int text_len, const unsigned char *blob, int blob_len)
{
	aocMQMsg *msg;
	uint32_t uid;

	msg = malloc(sizeof(aocMQMsg));
	if(msg == NULL)
		return 0;

	msg->type = 2;

	uid = aocNameListLookupByName(aoc->mq.namelist, name, NULL);
	if(uid == AOC_INVALID_UID)
		aocSendNameLookup(aoc, name);

	strncpy(msg->dest.name, aocNameLowerCase(name), AOC_MAX_NAME_LEN);
	msg->dest.name[AOC_MAX_NAME_LEN] = 0;

	return aocMsgQueueInsert(aoc, msg, prio, text, text_len, blob, blob_len);
}


void aocMsgQueueDestroy(aocConnection *aoc)
{
	int qid;

	for(qid=0; qid<2; qid++)
	{
		while(aoc->mq.msg_start[qid])
		{
			aocMQMsg *msg = aoc->mq.msg_start[qid];
			aocMQMsg *next = msg->next;

			if(msg->text != NULL)
				free(msg->text);
			if(msg->blob != NULL)
				free(msg->blob);
			free(msg);

			aoc->mq.msg_start[qid] = next;
		}
	}

	if(aoc->mq.timer != NULL)
		aocTimerDestroy(aoc->mq.timer);

	if(aoc->mq.badlist != NULL)
		aocNameListDestroy(aoc->mq.badlist);
}

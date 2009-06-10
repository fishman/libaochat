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
#ifndef _AOCHAT_H
#define _AOCHAT_H


#ifdef __COMPILING_LIB
#ifdef _WIN32
	#include "win32autoconf.h"
	#include "win32compat.h"

	#define sock_errno				WSAGetLastError()
	#define set_sock_errno(v)		WSASetLastError(v)
#else
	#include "autoconf.h"
	#ifdef HAVE_UNISTD_H
		#include <unistd.h>
	#endif
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <errno.h>
	#include <time.h>
	#include <sys/time.h>
	#include <netinet/in_systm.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <stdarg.h>

	#ifndef HAVE_SYS_TYPES_H
		/* TODO: use autoconf SIZEOF_* */
		typedef unsigned long uint32_t;
		typedef unsigned short uint16_t;
	#endif

	#define sock_errno			errno
	#define set_sock_errno(v)	errno = v
#endif
#endif


#ifdef __cplusplus
	extern "C" {
#endif

#ifdef _WIN32
	typedef unsigned long uint32_t;
	typedef unsigned short uint16_t;
#endif



#define AOC_TV_UDIFF(a, b) \
	( ((a)->tv_sec - (b)->tv_sec) * 1000000 + \
	  ((a)->tv_usec - (b)->tv_usec) )

#define AOC_TV_CMP(a, cmp, b) \
	( (a)->tv_sec  cmp  (b)->tv_sec || \
	  ((a)->tv_sec == (b)->tv_sec && \
	   (a)->tv_usec  cmp  (b)->tv_usec) )

#define AOC_TV_ADD(res, a) \
	(res)->tv_sec += (a)->tv_sec; \
	(res)->tv_usec += (a)->tv_usec; \
	if((res)->tv_usec > 1000000) { \
		(res)->tv_sec++; \
		(res)->tv_usec -= 1000000; \
	}

#define AOC_TV_SUB(res, a) \
	(res)->tv_sec -= (a)->tv_sec; \
	(res)->tv_usec -= (a)->tv_usec; \
	if((res)->tv_usec < 0) { \
		(res)->tv_sec--; \
		(res)->tv_usec += 1000000; \
	}

#define AOC_TV_SET(res, a) \
	(res)->tv_sec = (a)->tv_sec; \
	(res)->tv_usec = (a)->tv_usec;


#define AOC_WRD(a)	(*(uint16_t *)a)
#define AOC_INT(a)	(*(uint32_t *)a)
#define AOC_STR(a)	((char *)a)
#define AOC_GRP(a)	((unsigned char *)a)

#define AOC_COLOR_GREEN		0
#define AOC_COLOR_CYAN		4
#define AOC_COLOR_BLACK		11
#define AOC_COLOR_RED		12
#define AOC_COLOR_BLUE		14
#define AOC_COLOR_WHITE		15
#define AOC_COLOR_YELLOW	16

#define AOC_STYLE_COLOR		0x10
#define AOC_STYLE_UL_START	0x11
#define AOC_STYLE_UL_END	0x12

#define AOC_PREF_DEBUG		0
#define AOC_PREF_BWLIMIT	1
#define AOC_PREF_PKTLIMIT	2
#define AOC_PREF_PKEY1		3
#define AOC_PREF_PKEY2		4
#define AOC_PREF_FASTRND	5
#define AOC_PREF_MAXQSIZE	6

#define AOC_PRIO_HIGH		0
#define AOC_PRIO_LOW		1

#define AOC_PUBLIC_KEY_1 \
		"eca2e8c85d863dcdc26a429a71a9815ad052f6139669dd659f98ae159d313d13" \
		"c6bf2838e10a69b6478b64a24bd054ba8248e8fa778703b418408249440b2c1e" \
		"dd28853e240d8a7e49540b76d120d3b1ad2878b1b99490eb4a2a5e84caa8a91c" \
		"ecbdb1aa7c816e8be343246f80c637abc653b893fd91686cf8d32d6cfe5f2a6f"

#define AOC_PUBLIC_KEY_2 \
		"9c32cc23d559ca90fc31be72df817d0e124769e809f936bc14360ff4bed758f2" \
		"60a0d596584eacbbc2b88bdd410416163e11dbf62173393fbc0c6fefb2d855f1" \
		"a03dec8e9f105bbad91b3437d8eb73fe2f44159597aa4053cf788d2f9d7012fb" \
		"8d7c4ce3876f7d6cd5d0c31754f4cd96166708641958de54a6def5657b9f2e92"

#define AOC_SERVER_RK1			"chat.d1.funcom.com"
#define AOC_SERVER_RK2			"chat.d2.funcom.com"
#define AOC_SERVER_RK3			"chat.d3.funcom.com"
#define AOC_SERVER_TEST			"chat.dt.funcom.com"

#define AOC_INVALID_UID			0xFFFFFFFFUL
#define AOC_MAX_NAME_LEN		15

#define AOC_GROUP_UNMUTE		0x0000
#define AOC_GROUP_MUTE			0x0101

#define AOC_BUDDY_TEMPORARY		"\0"	/* Temporary buddy */
#define AOC_BUDDY_PERMANENT		"\1"	/* Permanent buddy */


#define AOC_STAT_DISCONNECTED	0
#define AOC_STAT_CONNECTING		1
#define AOC_STAT_CONNECTED		2

#define AOC_POLL_READ			1
#define AOC_POLL_WRITE			2

#define AOC_EVENT_CONNECT		-1	/* event->data is undefined */
#define AOC_EVENT_CONNFAIL		-2	/* (int)event->data is errno */
#define AOC_EVENT_DISCONNECT	-3	/* (int)event->data is errno */
#define AOC_EVENT_MESSAGE		-4	/* (aocMessage *)event->data is an aocMessage struct */
#define AOC_EVENT_UNHANDLED		-5	/* (aocMessage *)event->data is an aocMessage struct */
#define AOC_EVENT_TIMER			-6	/* (int)event->data is timer id */

#define AOC_TYPE_WORD			0
#define AOC_TYPE_INTEGER		1
#define AOC_TYPE_STRING			2
#define AOC_TYPE_GROUPID		3
#define AOC_TYPE_ARRAYSIZE		4	/* AOC_TYPE_WORD */
#define AOC_TYPE_RAW			5

#define AOC_STACK_LIFO			0	/* last in, first out */
#define AOC_STACK_FIFO			1	/* first in, first out */


/* Chat Protocol constants */
#define AOC_SRV_LOGIN_SEED			0		/* [string Seed] */
#define AOC_SRV_LOGIN_OK			5		/* - */
#define AOC_SRV_LOGIN_ERROR			6		/* [string Message] */
#define AOC_SRV_LOGIN_CHARLIST		7		/* {[int UserID]} {[string Name]} {[int Level]} {[int Online]} */
#define AOC_SRV_CLIENT_UNKNOWN		10		/* [int UserID] */
#define AOC_SRV_CLIENT_NAME			20		/* [int UserID] [string Name] */
#define AOC_SRV_LOOKUP_RESULT		21		/* [int UserID] [string Name] */
#define AOC_SRV_PRIVATE_MSG			30		/* [int UserID] [string Text] [string Blob] */
#define AOC_SRV_VICINITY_MSG		34		/* [int UserID] [string Text] [string Blob] */
#define AOC_SRV_ANONVICINITY_MSG	35		/* [string] [string Text] [string Blob] */
#define AOC_SRV_SYSTEM_MSG			36		/* [string Text] */
#define AOC_SRV_CHAT_NOTICE			37		/* [int] [int] [int] [string] */
#define AOC_SRV_BUDDY_STATUS		40		/* [int UserID] [int Online] [string Status] */
#define AOC_SRV_BUDDY_REMOVED		41		/* [int UserID] */
#define AOC_SRV_PRIVGRP_INVITE		50		/* [int UserID] */
#define AOC_SRV_PRIVGRP_KICK		51		/* [int UserID] */
#define AOC_SRV_PRIVGRP_PART		53		/* [int UserID] */
#define AOC_SRV_PRIVGRP_CLIJOIN		55		/* [int UserID] [int UserID] */
#define AOC_SRV_PRIVGRP_CLIPART		56		/* [int UserID] [int UserID] */
#define AOC_SRV_PRIVGRP_MSG			57		/* [int UserID] [int UserID] [string Text] [string Blob] */
#define AOC_SRV_GROUP_INFO			60		/* [grp GroupID] [string GroupName] [word Mute] [word ?] [string ?] */
#define AOC_SRV_GROUP_PART			61		/* [grp GroupID] */
#define AOC_SRV_GROUP_MSG			65		/* [grp GroupID] [int UserID] [string Text] [string Blob] */
#define AOC_SRV_PONG				100		/* [string Whatever] */
#define AOC_SRV_FORWARD				110		/* [int ?] [raw Data] */
#define AOC_SRV_AMD_MUX_INFO		1100	/* {[int ?]} {[int ?]} {[int ?]} */

#define AOC_CLI_LOGIN_RESPONSE		2		/* [int ?] [string Username] [string Key] */
#define AOC_CLI_LOGIN_SELCHAR		3		/* [int UserID] */
#define AOC_CLI_NAME_LOOKUP			21		/* [string Name] */
#define AOC_CLI_PRIVATE_MSG			30		/* [int UserID] [string Text] [string Blob] */
#define AOC_CLI_BUDDY_ADD			40		/* [int UserID] [string Status] */
#define AOC_CLI_BUDDY_REMOVE		41		/* [int UserID] */
#define AOC_CLI_ONLINE_STATUS		42		/* [int ?] */
#define AOC_CLI_PRIVGRP_INVITE		50		/* [int UserID] */
#define AOC_CLI_PRIVGRP_KICK		51		/* [int UserID] */
#define AOC_CLI_PRIVGRP_JOIN		52		/* [int UserID] */
#define AOC_CLI_PRIVGRP_KICKALL		54		/* - */
#define AOC_CLI_PRIVGRP_MSG			57		/* [int UserID] [string Text] [string Blob] */
#define AOC_CLI_GROUP_DATASET		64		/* [grp GroupID] [word Mute] [int ?] */
#define AOC_CLI_GROUP_MESSAGE		65		/* [grp GroupID] [string text] [string Blob] */
#define AOC_CLI_GROUP_CLIMODE		66		/* [grp GroupID] [int ?] [int ?] [int ?] [int ?] sent when zoning in game? */
#define AOC_CLI_CLIMODE_GET			70		/* [int ?] [grp GroupID] */
#define AOC_CLI_CLIMODE_SET			71		/* [int ?] [int ?] [int ?] [int ?] */
#define AOC_CLI_PING				100		/* [string Whatever] */
#define AOC_CLI_CHAT_COMMAND		120		/* {[string Command] [string Value]...} */

/* Backwards compatibility with 1.0.x */
#define AOC_SRV_MSG_PRIVATE			AOC_SRV_PRIVATE_MSG
#define AOC_SRV_MSG_VICINITY		AOC_SRV_VICINITY_MSG
#define AOC_SRV_MSG_ANONVICINITY	AOC_SRV_ANONVICINITY_MSG
#define AOC_SRV_MSG_SYSTEM			AOC_SRV_SYSTEM_MSG
#define AOC_SRV_GROUP_JOIN			AOC_SRV_GROUP_INFO
#define AOC_CLI_MSG_PRIVATE			AOC_CLI_PRIVATE_MSG


typedef struct _aocConnection	aocConnection;
typedef struct _aocMessage	aocMessage;
typedef struct _aocEvent		aocEvent;
typedef struct _aocPacket		aocPacket;
typedef struct _aocSendQueue	aocSendQueue;
typedef struct _aocHashNode	aocHashNode;
typedef struct _aocHashTable	aocHashTable;
typedef struct _aocHashTable	aocNameList;	/* alias for aocHashTable */
typedef struct _aocGroupList	aocGroupList;
typedef struct _aocStack		aocStack;
typedef struct _aocTimer		aocTimer;
typedef struct _aocMsgQueue	aocMsgQueue;
typedef struct _aocMQMsg		aocMQMsg;



/* msg_queue.c */
struct _aocMsgQueue
{
	aocMQMsg	*msg_start[2];	/* first node in queue prio[HIGH,LOW] */
	aocMQMsg	*msg_end[2];		/* last node in queue prio[HIGH,LOW] */
	aocNameList	*namelist;		/* name list hash table */
	aocNameList	*badlist;		/* list of charnames that don't exist */
	int			burst;			/* burst */
	aocTimer	*timer;
};

struct _aocMQMsg
{
	aocMQMsg *next;
	int type;		/* 0 = uid tell, 1 = group msg, 2 = name tell */
	union {
		uint32_t uid;
		unsigned char group_id[5];
		char name[AOC_MAX_NAME_LEN+1];
	} dest;
	int text_len;
	char *text;
	int blob_len;
	unsigned char *blob;
};


/* connect.c */
struct _aocConnection
{
	int				socket;				/* socket file descriptor */
	int				status;				/* connection status (one of AOC_STAT_*) */
	int				selchar_done;		/* whether an AOC_CLI_LOGIN_SELCHAR packet has been sent */

	int				read_buf_size;		/* size of read_buf */
	int				read_buf_len;		/* amount of data in read_buf */
	unsigned char		*read_buf;			/* recv buffer */

	aocSendQueue	*send_queue;		/* send queue list */
	aocSendQueue	*send_queue_last;	/* last node in send queue list */
	int				send_queue_max;		/* max # of bytes in queue */
	int				send_queue_size;	/* # of bytes in queue */

	int				bw_limit;			/* outgoing bandwidth limit in bytes/sec */
	int				bw_remain;			/* bandwidth left to use (this second) */

	int				pkt_limit;			/* outgoing packet limit in bytes/sec */
	int				pkt_remain;			/* packets left to send (this second) */

	aocMsgQueue		mq;

	union {
		void *ptr;
		int integer;
		uint32_t u32;
	} udata;
};


/* message.c */
struct _aocMessage
{
	int		type;		/* type of packet */
	int		argc;		/* argument count */
	int		*argt;		/* argument types */
	int		*argl;		/* argument length */
	void		**argv;		/* argument values */
	void		*data;		/* packet data (might be modified, read docs) */
};

struct _aocEvent
{
	aocEvent		*next;	/* next event */
	aocConnection	*aoc;	/* connection (can be NULL) */
	int				type;	/* type of event */
	void				*data;	/* data, depends on event type */
};


/* packet.c */
struct _aocPacket
{
	int			size;		/* malloc()'ed size */
	int			pos;		/* position in data (used by the aocPop* functions) */
	int			len;		/* length of data */
	int			type;		/* packet type */
	unsigned char	*data;		/* packet header (4 bytes) and packet body */
};


/* send_queue.c */
struct _aocSendQueue
{
	aocSendQueue	*next;		/* next queued packet */
	int				len;		/* length of data */
	int				sent;		/* bytes sent */
	unsigned char		*packet;	/* data */
};


/* list.c */
struct _aocHashNode
{
	aocHashNode	*next;		/* next node */
	uint32_t	key;		/* key */
	int			which;		/* key generated from uid(0) or name(1)? */
	uint32_t	uid;
	char			name[AOC_MAX_NAME_LEN+1];
	void			*data;		/* user data */
};

struct _aocHashTable
{
	uint32_t	size;		/* size of node array */
	void		**node;		/* node array */
	int		count;		/* number of nodes (items) */
};

struct _aocGroupList
{
	aocGroupList	*next;				/* next group in list */
	unsigned char		group_id[5];			/* group id */
	char 			group_name[32];		/* group name */
};

struct _aocStack
{
	int	type;
	int	si;		/* stack idx (used in FIFO stacks) */
	int	sp;		/* stack ptr */
	int	ss;		/* stack size */
	void	**ptr;
};


/* timer.c */
struct _aocTimer
{
	aocTimer		*prev;			/* previous timer in chain */
	aocTimer		*next;			/* next timer in chain */
	aocConnection	*aoc;			/* associated connection (can be NULL) */
	int				id;				/* user defined timer ID */
	int				underruns;		/* # of underruns */
	struct timeval	interval;		/* interval */
	struct timeval	tv;				/* next trigger time */
};



/* connect.c */
aocConnection *aocInit(aocConnection *aoc);
int aocConnect(aocConnection *aoc, const struct sockaddr_in *addr);
void aocDisconnect(aocConnection *aoc);
void aocPollCanRead(aocConnection *aoc);
void aocPollCanWrite(aocConnection *aoc);
int aocPollQuery(aocConnection *aoc);
int aocPollArray(long sec, long usec, int num, aocConnection **c);
int aocPollVarArg(long sec, long usec, int num, ...);

/* message.c */
aocEvent *aocEventGet();
int aocEventAdd(aocConnection *aoc, int type, void *data);
void aocEventDestroy(aocEvent *ev);

/* misc.c */
struct sockaddr_in *aocMakeAddr(const char *host, int port);
void aocSocketSetAsync(int s);
struct in_addr *aocResolveHost(const char *host);
void aocLowerCase(char *str);
char *aocNameLowerCase(const char *str);
char *aocNameTrim(const char *str);
void *aocMemDup(const void *ptr, int len);
void aocRandom(unsigned char *buf, int len);
void aocSetPref(int prefid, void *value);
unsigned char *aocGetColorRGB(int color);
char *aocServerPacketName(int type);
char *aocClientPacketName(int type);
int aocMsgArraySize(const aocMessage *msg, int aidx);
void *aocMsgArrayValue(const aocMessage *msg, int aidx, int idx, int *argt, int *argl);
unsigned char *aocStripStyles(unsigned char *str, int len, int alloc);
void aocMakeWndBlob(unsigned char *blob, uint32_t len, int is_tell);
void aocMakeItemBlob(unsigned char *blob, uint32_t low_id, uint32_t high_id, uint32_t item_ql, int is_tell);
void aocMakeBlob(unsigned char *blob, uint32_t blob_len, uint32_t low_id, uint32_t high_id, uint32_t item_ql, int is_tell);
int aocDecodeBlob(unsigned char *blob, int blob_len, int is_tell, uint32_t *low_id, uint32_t *high_id, uint32_t *item_ql, uint32_t *str_len, unsigned char **str);
void aocFree(void *ptr);

/* packet.c */
int aocPushWord(aocPacket *p, uint16_t val);
int aocPushInteger(aocPacket *p, uint32_t val);
int aocPushGroupId(aocPacket *p, const unsigned char *grpid);
int aocPushString(aocPacket *p, const char *str, int len);
int aocPacketInit(aocPacket *p, int type);
int aocPacketSend(aocConnection *c, aocPacket *p);
int aocSendLoginResponse(aocConnection *c, uint32_t _zero, const char *name, const char *key);
int aocSendLoginSelectChar(aocConnection *c, uint32_t user_id);
int aocSendNameLookup(aocConnection *c, const char *name);
int aocSendPrivateMessage(aocConnection *c, uint32_t user_id, const char *text, int text_len, const unsigned char *blob, int blob_len);
int aocSendBuddyAdd(aocConnection *c, uint32_t user_id, const unsigned char *blob, int blob_len);
int aocSendBuddyRemove(aocConnection *c, uint32_t user_id);
int aocSendOnlineStatus(aocConnection *c, uint32_t status);
int aocSendPrivateGroupInvite(aocConnection *c, uint32_t user_id);
int aocSendPrivateGroupKick(aocConnection *c, uint32_t user_id);
int aocSendPrivateGroupJoin(aocConnection *c, uint32_t user_id);
int aocSendPrivateGroupKickAll(aocConnection *c);
int aocSendPrivateGroupMessage(aocConnection *c, uint32_t user_id, const char *text, int text_len, const unsigned char *blob, int blob_len);
int aocSendGroupDataset(aocConnection *c, const unsigned char *group_id, uint16_t flags, uint32_t _unknown);
int aocSendGroupMessage(aocConnection *c, const unsigned char *group_id, const char *text, int text_len, const unsigned char *blob, int blob_len);
int aocSendGroupClimode(aocConnection *c, const unsigned char *group_id, uint32_t _unknown1, uint32_t _unknown2, uint32_t _unknown3, uint32_t _unknown4);
int aocSendPing(aocConnection *c, const unsigned char *blob, int blob_len);
int aocSendChatCommand(aocConnection *c, const char *command);

/* list.c */
aocHashTable *aocNameListNew(int size);
void aocNameListDestroy(aocHashTable *table);
int aocNameListInsert(aocHashTable *table, uint32_t uid, const char *name, void *data);
void *aocNameListDeleteByName(aocHashTable *table, const char *name);
void *aocNameListDeleteByUID(aocHashTable *table, uint32_t uid);
uint32_t aocNameListLookupByName(aocHashTable *table, const char *name, void **data);
char *aocNameListLookupByUID(aocHashTable *table, uint32_t uid, void **data);
void *aocNameListSetDataByName(aocHashTable *table, const char *name, void *data);
void *aocNameListSetDataByUID(aocHashTable *table, uint32_t uid, void *data);
aocHashNode *aocNameListWalk(aocHashTable *table, aocHashNode *node);
aocGroupList *aocGroupAdd(aocGroupList *list, const unsigned char *group_id, const char *name);
aocGroupList *aocGroupDelete(aocGroupList *list, const unsigned char *group_id);
aocGroupList *aocGroupDeleteAll(aocGroupList *list);
char *aocGroupLookupByGID(aocGroupList *list, const unsigned char *group_id);
unsigned char *aocGroupLookupByName(aocGroupList *list, const char *name);
aocStack *aocStackNew(int type);
void aocStackDestroy(aocStack *stack);
int aocStackPush(aocStack *stack, void *ptr);
void *aocStackPop(aocStack *stack);
void *aocStackPeek(const aocStack *stack);
void aocStackPoke(aocStack *stack, void *ptr);

/* keyex.c */
char *aocKeyexGenerateKey(const char *servkey, const char *username, const char *password);

/* timer.c */
void aocTimerMaxTv(struct timeval *tv);
int aocTimerMax(int timeout);
void aocTimerPoll();
aocTimer *aocTimerNew(aocConnection *aoc, int id, long sec, long usec, const struct timeval *tv);
void aocTimerDestroy(aocTimer *timer);

/* tell_queue.c */
void aocMsgQueueSetNameList(aocConnection *aoc, aocNameList *namelist);
int aocMsgQueueTellUID(aocConnection *aoc, uint32_t uid, int prio,  const char *text, int text_len, const unsigned char *blob, int blob_len);
int aocMsgQueueGroup(aocConnection *aoc, const unsigned char *group_id, int prio,  const char *text, int text_len, const unsigned char *blob, int blob_len);
int aocMsgQueueTell(aocConnection *aoc, const char *name, int prio, const char *text, int text_len, const unsigned char *blob, int blob_len);


#ifdef __cplusplus
	}
#endif


#ifdef __COMPILING_LIB
	#define AOC_STACK_CHNK			64	/* size of malloc() chunks */
	#define AOC_WORDPTR(a)	(*(unsigned char *)(a)*256 + *(unsigned char *)(a+1))

	#define AOC_ITIMER_LIMITER	-1	/* bandwidth/packet limit timer */
	#define AOC_ITIMER_MSGQUEUE	-2	/* buddy/tell/grpmsg queue timer */

	#define AOC_READ_BUF_SIZE	4096

	#define MAYBE_DEBUG if(_aoc_pref.debug)

	struct __aoc_pref
	{
		int debug;
		int bwlimit;
		int pktlimit;
		char *pkey1;
		char *pkey2;
		int fastrnd;
		int maxqueuesize;
	};

	void aocEventDestroyByAoc(aocConnection *aoc);
	int aocPacketRead(aocConnection *aoc);
	int aocSendQueuePacket(aocConnection *aoc, aocPacket *p, int sent);
	int aocSendQueuePoll(aocConnection *aoc);
	void aocSendQueueEmpty(aocConnection *aoc);
	int aocSendQueueIsEmpty(aocConnection *aoc);
	void aocTimerPushBack(aocTimer *timer, const struct timeval *now, int unchain);
	void aocTimerDestroyByAoc(aocConnection *aoc);
	void aocDebugMsg(const char *fmt, ...);
	void aocMsgQueueLookupCallback(aocConnection *aoc, aocMessage *msg);
	void aocMsgQueueTick(aocConnection *aoc, int from_timer);
	void aocMsgQueueDestroy(aocConnection *aoc);
	int aocMsgQueueInsert(aocConnection *aoc, aocMQMsg *msg, int prio,
			const char *text, int text_len, const unsigned char *blob, int blob_len);

	extern struct __aoc_pref _aoc_pref;
#endif

#endif

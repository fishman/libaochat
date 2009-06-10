#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winsock.h>
#else
	#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
	#include <sys/time.h>
	#include <netinet/in_systm.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
#endif
#include <aochat.h>


#define TIMER_PING	0


int			running = 1;
aocNameList	*namelist;

char			conf_username[64];
char			conf_password[64];
char			conf_charname[AOC_MAX_NAME_LEN+1];
uint32_t	my_user_id = AOC_INVALID_UID;


/* Wrapper for aocNameListLookupByUID() */
char *GetCharName(uint32_t uid)
{
	char *charname;

	charname = aocNameListLookupByUID(namelist, uid, NULL);
	if(charname == NULL)
		return "unknown";

	return charname;
}


/* Takes care of timer events */
void HandleTimer(aocConnection *aoc, int id)
{
	switch(id)
	{
	case TIMER_PING:
		aocSendPing(aoc, "Moo.", -1);
		break;
	}
}


/* Takes care of incoming packets */
void HandleMessage(aocConnection *aoc, aocMessage *msg)
{
	char *charname, *pgrp_name, *grp_name;
	unsigned char *text, *blob;
	int text_len, blob_len;
	uint32_t uid, pgrp_uid;
	unsigned char *group_id;
	


	switch(msg->type)
	{
	/*
		This is the first packet you receive after establishing a connection
		to the chat server. The message contains a 32 byte human readable hex
		string, which you should pass to the aocKeyexGenerateKey() function to
		generate a login key, and then reply with aocSendLoginResponse().

		Packet contents:
			msg->argv[0] = (String) LoginSeed
	*/
	case AOC_SRV_LOGIN_SEED:
		{
			char *key;

			/* Generate a valid login key. */
			key = aocKeyexGenerateKey(AOC_STR(msg->argv[0]), conf_username, conf_username);
			if(key == NULL)
			{
				printf("Could not generate login key.\n");
				running = 0;
				break;
			}

			/* Send authenticate packet */
			aocSendLoginResponse(aoc, 0, conf_username, key);

			aocFree(key);
		}
		break;


	/*
		This packet is received is your username or password is incorrect.
		It contains a human readable error string. The server will disconnect
		you after sending this message.

		Packet contents:
			msg->argv[0] = (String) ErrorMessage
	*/
	case AOC_SRV_LOGIN_ERROR:
		printf("Server said: %s\n", AOC_STR(msg->argv[0]));
		running = 0;
		break;


	/*
		This packet is received after you have successfully authenticated
		yourself with aocSendLoginResponse(). The packet contains a list
		of all characters on the account. You should reply with
		aocSendLoginSelectChar().

		Packet contents: (4 arrays)
			msg->argv[...] = (Integer) UserID
			msg->argv[...] = (String)  Charname
			msg->argv[...] = (Integer) Level
			msg->argv[...] = (Integer) Online Status (0 or 1)

			All 4 arrays have the same size in this packet.
			(Though you should probably handle cases where they
			 have different sizes... as the following code does)
	*/
	case AOC_SRV_LOGIN_CHARLIST:
		{
			int nchars, i;

			/* The number of characters (taken from the length of the first array) */
			nchars = aocMsgArraySize(msg, 0);

			/* Decode the packet. */
			for(i=0; i<nchars; i++)
			{
				void *user_id, *charname, *level, *status;

				/* Get values from all 4 arrays */
				user_id		= aocMsgArrayValue(msg, 0, i, NULL, NULL);
				charname	= aocMsgArrayValue(msg, 1, i, NULL, NULL);
				level		= aocMsgArrayValue(msg, 2, i, NULL, NULL);
				status		= aocMsgArrayValue(msg, 3, i, NULL, NULL);

				if(charname == NULL)
					continue;

				/* Compare char name with conf_charname */
				if(strcmp(AOC_STR(charname), aocNameLowerCase(conf_charname)) != 0)
					continue;

				printf("Logging in as %s... (UserID %d, Level %d, Status %d)\n",
					AOC_STR(charname),
					user_id	? AOC_INT(user_id)	: (int)AOC_INVALID_UID,
					level	? AOC_INT(level)		: 0,
					status	? AOC_INT(status)	: 0);

				aocSendLoginSelectChar(aoc, AOC_INT(user_id));
				my_user_id = AOC_INT(user_id);
				break;
			}

			if(my_user_id == AOC_INVALID_UID)
			{
				printf("No character named '%s' was found.\n", ao_char);
				running = 0;
			}
		}
		break;


	/*
		This packet is received after you have selected a character.

		Packet contents: None
	*/
	case AOC_SRV_LOGIN_OK:
		printf("Logged in!\n");
		break;


	case AOC_SRV_CLIENT_UNKNOWN:
		uid = AOC_INT(msg->argv[0]);

		printf("UserID %d does not exist.\n", uid);
		break;


	case AOC_SRV_CLIENT_NAME:
		uid			= AOC_INT(msg->argv[0]);
		charname	= AOC_STR(msg->argv[1]);

		aocNameListInsert(namelist, uid, charname, NULL);
		break;


	case AOC_SRV_LOOKUP_RESULT:
		uid = AOC_INT(msg->argv[0]);
		charname = AOC_STR(msg->argv[1]);

		if(uid == AOC_INVALID_UID)
		{
			printf("Character '%s' does not exist.\n", charname);
			break;
		}

		/* Add the charname / user id to the name list */
		aocNameListInsert(namelist, uid, charname, NULL);
		break;


	case AOC_SRV_PRIVATE_MSG:
		uid			= AOC_INT(msg->argv[0]);
		charname	= GetCharName(uid);
		text		= AOC_STR(msg->argv[1]);
		text_len	= msg->argl[1];
		blob		= AOC_STR(msg->argv[2]);
		blob_len	= msg->argl[2];

		/* Remove color styles, etc.. */
		aocStripStyles(text, text_len, 0);

		/* Remove unprintable characters */
		for(i=0; i<text_len; i++)
			if(text[i] < 32) text[i] = '.';

		printf("[%s]: %s\n", charname, text);
		break;


	/*
		To receive this packet you must be in-game, hence you will
		never receive it with libaochat.

		Packet contents:
			msg->argv[0] = (Integer) UserID
			msg->argv[1] = (String) Text
			msg->argv[2] = (String) Blob
	*/
	case AOC_SRV_VICINITY_MSG:
		break;


	/*
		This is usually used for broadcast messages, such as information about
		upcoming downtimes, when people "ding" 220, etc...

		Packet contents:
			msg->argv[0] = (String) Sender
			msg->argv[1] = (String) Text
			msg->argv[2] = (String) Blob
	*/
	case AOC_SRV_ANONVICINITY_MSG:
		break;


	/*
		System message.

		Packet contents:
			msg->argv[0] = (String) Message
	*/
	case AOC_SRV_SYSTEM_MSG:
		printf("System message: %s\n", AOC_STR(msg->argv[0]));
		break;


	/*
		This packet contains information about buddy status and type.
		You will receive it when you log on, when one of your buddies change
		status, and when you add/change a buddy with aocSendBuddyAdd().
		It is sent once per buddy. :)

		Packet contents:
			msg->argv[0] = (Integer) UserID
			msg->argv[1] = (Integer) Status (0 = Offline, 1 = Online)
			msg->argv[2] = (String) Buddy type ('\0' = Temp, '\1' = Perm)
							(AOC_BUDDY_TEMPORARY, AOC_BUDDY_PERMANENT)
	*/
	case AOC_SRV_BUDDY_STATUS:
		printf("%s buddy %s changed his/her status to %s.\n",
				AOC_STR(msg->argv[2])[0] ? "Permanent" : "Temporary",
				GetCharName(uid),
				AOC_INT(msg->argv[1]) ? "online" : "offline" );
		break;


	/*
		This packet is sent in reply to buddy remove packets.

		Packet contents:
			msg->argv[0] = (Integer) UserID
	*/
	case AOC_SRV_BUDDY_REMOVED:
		uid = AOC_INT(msg->argv[0]);

		printf("%s was removed from your buddy list.\n", GetCharName(uid));
		break;


	/*
		This packet is sent when you receive an invite to someone's
		private group.

		Packet contents:
			msg->argv[0] = (Integer) UserID (aka PrivateGroupID)
	*/
	case AOC_SRV_PRIVGRP_INVITE:
		break;


	/*
		This packet is sent when you have been kicked from someone's
		private group.

		Packet contents:
			msg->argv[0] = (Integer) UserID (aka PrivateGroupID)
	*/
	case AOC_SRV_PRIVGRP_KICK:
		break;


	/*
		This packet is sent when you have left someone's private group.
		Note however that there is no way to actually part a group.
		I guess funcom's programmers have a weird sense of humor...

		Packet contents:
			msg->argv[0] = (Integer) UserID (aka PrivateGroupID)
	*/
	case AOC_SRV_PRIVGRP_PART:
		break;


	/*
		This packet is sent when someone joins a private group that you
		are currently in.

		Packet contents:
			msg->argv[0] = (Integer) UserID (aka PrivateGroupID)
			msg->argv[1] = (Integer) UserID
	*/
	case AOC_SRV_PRIVGRP_CLIJOIN:
		break;


	/*
		This packet is sent when someone leaves a private group that you
		are currently in. (ie. was kicked by the private group owner)

		Packet contents:
			msg->argv[0] = (Integer) UserID (aka PrivateGroupID)
			msg->argv[1] = (Integer) UserID
	*/
	case AOC_SRV_PRIVGRP_CLIPART:
		break;


	/*
		Private group message.

		Packet contents:
			msg->argv[0] = (Integer) UserID (aka PrivateGroupID)
			msg->argv[1] = (Integer) UserID
			msg->argv[2] = (String) Text
			msg->argv[3] = (String) Blob
	*/
	case AOC_SRV_PRIVGRP_MSG:
		break;


	case AOC_SRV_GROUP_INFO:
		break;


	case AOC_SRV_GROUP_PART:
		break;


	case AOC_SRV_GROUP_MSG:
		break;


	/*
		This packet is sent in reply to pings.

		Packet contents:
			msg->argv[0] = (String) UserData
	*/
	case AOC_SRV_PONG:
		printf("Pong!\n");
		break;


	/*
		This packet is sent to you if someone types "/cc info yourcharname" in game.

		Packet contents:
			msg->argv[0] = (Raw) Binary Data
	*/
	case AOC_SRV_FORWARD:
		break;


	/*
		Unknown.
	*/
	case AOC_SRV_AMD_MUX_INFO:
		break;

	}

	return;
}


int main(int argc, char **argv)
{
	aocConnection *aoc;
	aocEvent *event;
	struct sockaddr_in *addr;


#ifdef _WIN32
	struct WSAData wd;

	/* On windows we need to initialize WinSock */
	WSAStartup(0x0101, &wd);
#endif

	/* Look up IPv4 address for rimor's chat server */
	printf("Resolving %s...\n", AOC_SERVER_RK2);
	addr = aocMakeAddr(AOC_SERVER_RK2, 7012);
	if(addr == NULL)
	{
		printf("DNS Lookup Failure.\n");
		return 1;
	}

	/* Allocate and initialize a new connection */
	aoc = aocInit(NULL);

	/* Create a hash list to map charnames to user ids, and vice versa */
	namelist = aocNameListNew(2048);

	/* Give our hash list to the message queue */
	aocMsgQueueSetNameList(aoc, namelist);

	/* Connect to chat server. */
	if(!aocConnect(aoc, addr))
	{
		printf("Fatal socket error.\n");
		return 2;
	}

	/* Create a ping timer with a 60 second interval */
	aocTimerNew(aoc, TIMER_PING, 60, 0, NULL);

	/* The main loop */
	while(running)
	{
		/* Poll the connection */
		aocPollVarArg(60, 0, 1, aoc);

		/* Process events */
		while((event = aocEventGet()))
		{
			switch(event->type)
			{
			case AOC_EVENT_CONNECT:
				printf("Connected to chat server!\n");
				break;

			case AOC_EVENT_CONNFAIL:
				printf("Connection failed. (%s)\n",
						strerror((int)event->data));
				running = 0;
				break;

			case AOC_EVENT_DISCONNECT:
				printf("Disconnected from chat server. (%s)\n",
						strerror((int)event->data));
				running = 0;
				break;

			case AOC_EVENT_MESSAGE:
				HandleMessage(event->aoc, (aocMessage *)event->data);
				break;

			case AOC_EVENT_TIMER:
				HandleTimer(event->aoc, (int)((aocTimer *)event->data)->id);
				break;
			}

			/* Destroy the event */
			aocEventDestroy(event);
		}
	}

	/* Disconnect and deinitialize the connection */
	aocDisconnect(aoc);
	aocFree(aoc);

	return 0;
}

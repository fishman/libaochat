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
#include "../aochat.h"


int running = 1;
char *ao_user, *ao_pass, *ao_char;
uint32_t my_uid = AOC_INVALID_UID;
aocNameList *namelist;


char *get_char_name(uint32_t uid)
{
	char *charname;

	charname = aocNameListLookupByUID(namelist, uid, NULL);
	if(charname == NULL)
		return "unknown";
	return charname;
}


void handle_message(aocConnection *aoc, aocMessage *msg)
{
	char *charname;
	uint32_t uid;

	switch(msg->type)
	{
	case AOC_SRV_LOGIN_SEED:
		{
			char *key = aocKeyexGenerateKey(AOC_STR(msg->argv[0]), ao_user, ao_pass);

			if(key == NULL)
			{
				printf("Could not generate login key.\n");
				running = 0;
				break;
			}

			aocSendLoginResponse(aoc, 0, ao_user, key);

			aocFree(key);
		}
		break;


	case AOC_SRV_LOGIN_OK:
		printf("Logged in.\n");
		break;


	case AOC_SRV_LOGIN_CHARLIST:
		{
			int nchars, i;

			nchars = aocMsgArraySize(msg, 0);

			for(i=0; i<nchars; i++)
			{
				void *user_id, *charname, *level;

				charname = aocMsgArrayValue(msg, 1, i, NULL, NULL);
				if(charname == NULL || strcmp(AOC_STR(charname), aocNameLowerCase(ao_char)) != 0)
					continue;

				user_id  = aocMsgArrayValue(msg, 0, i, NULL, NULL);
				level    = aocMsgArrayValue(msg, 2, i, NULL, NULL);

				printf("Logging in as %s... (UserID %d, Level %d)\n",
					AOC_STR(charname),
					user_id  ? AOC_INT(user_id)  : (int)AOC_INVALID_UID,
					level    ? AOC_INT(level)    : 0);

				aocSendLoginSelectChar(aoc, AOC_INT(user_id));
				my_uid = AOC_INT(user_id);
				break;
			}
			if(i == nchars)
			{
				printf("No character named '%s' was found.\n", ao_char);
				running = 0;
			}
		}
		break;


	case AOC_SRV_LOGIN_ERROR:
		printf("Login error:\n  ");
		puts(AOC_STR(msg->argv[0]));
		break;


	case AOC_SRV_CLIENT_NAME:
	case AOC_SRV_LOOKUP_RESULT:
		uid = AOC_INT(msg->argv[0]);
		charname = AOC_STR(msg->argv[1]);

		aocNameListInsert(namelist, uid, charname, NULL);
		printf("Player %s has user id %d\n", charname, uid);
		break;


	case AOC_SRV_PRIVGRP_INVITE:
		uid = AOC_INT(msg->argv[0]);
		if(uid == my_uid) break;

		printf("Joined %s's private group!\n", get_char_name(uid));
		aocSendPrivateGroupJoin(aoc, uid);
		aocSendPrivateGroupMessage(aoc, uid, "Hi! OmGz =)))", -1, "", 0);
		break;


	case AOC_SRV_PRIVGRP_PART:
	case AOC_SRV_PRIVGRP_KICK:
		printf("Left %s's private group!\n", get_char_name(AOC_INT(msg->argv[0])));
		break;

	case AOC_SRV_PRIVGRP_CLIPART:
	case AOC_SRV_PRIVGRP_CLIJOIN:
		{
			char *group_name;
			char buf[256];
			int len;

			if(my_uid == AOC_INT(msg->argv[1]))
				break;

			uid = AOC_INT(msg->argv[0]);
			group_name = get_char_name(uid);
			charname = get_char_name(AOC_INT(msg->argv[1]));

			printf("Player %s %s %s's group!",
					charname,
					msg->type == AOC_SRV_PRIVGRP_CLIPART ? "left" : "joined",
					group_name);

			len = sprintf(buf, "%s %s %s's group %s",
					charname,
					msg->type == AOC_SRV_PRIVGRP_CLIPART ? "left" : "joined",
					group_name,
					msg->type == AOC_SRV_PRIVGRP_CLIPART ? "=((" : "=))) OMGz! Hi!");

			aocSendPrivateGroupMessage(aoc, uid, buf, len, "", 0);
		}
		break;


	case AOC_SRV_PRIVGRP_MSG:
		{
			char *group_name;
			char *str;
			int i;

			group_name = get_char_name(AOC_INT(msg->argv[0]));
			charname = get_char_name(AOC_INT(msg->argv[1]));

			/* filter out nasty characters */
			str = AOC_STR(msg->argv[2]);
			for(i=0; i<msg->argl[2]; i++)
				if(str[i] < 32) str[i] = '.';

			printf("[%s] %s: %s\n", group_name, charname, str);
		}
		break;


	case AOC_SRV_MSG_PRIVATE:
		{
			char *str;
			int i;

			uid = AOC_INT(msg->argv[0]);
			if(my_uid != uid)
				aocMsgQueueTellUID(aoc, uid, AOC_PRIO_HIGH,
						AOC_STR(msg->argv[1]), msg->argl[1],
						AOC_STR(msg->argv[2]), msg->argl[2]);

			/* filter out nasty characters */
			str = AOC_STR(msg->argv[1]);
			for(i=0; i<msg->argl[1]; i++)
				if(str[i] < 32) str[i] = '.';

			printf("[%s]: %s\n", get_char_name(uid), str);
		}
		break;


	case AOC_SRV_MSG_SYSTEM:
		printf("System message:\n  ");
		puts(AOC_STR(msg->argv[0]));
		break;


	case AOC_SRV_GROUP_JOIN:
		{
			unsigned char *g = AOC_GRP(msg->argv[0]);

			printf("Joined group '%s' grpid %02x%02x%02x%02x%02x flags %04x:%08x\n",
				AOC_STR(msg->argv[1]), g[0], g[1], g[2], g[3], g[4],
				AOC_WRD(msg->argv[2]), AOC_INT(msg->argv[3]));
		}
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

	WSAStartup(0x0101, &wd);
#endif

	if(argc != 4)
	{
		printf("Usage: charlist [username] [password] [charname]\n");
		return 1;
	}

	ao_user = argv[1];
	ao_pass = argv[2];
	ao_char = argv[3];

	/* Look up IPv4 address for rimor's chat server */
	printf("Resolving %s...\n", AOC_SERVER_RK3);
	addr = aocMakeAddr(AOC_SERVER_RK3, 7013);
	if(addr == NULL)
	{
		printf("DNS Lookup Failure.\n");
		return 1;
	}

	/* Allocate and initialize a new connection */
	aoc = aocInit(NULL);

	/* Initialize the built-in tell queue */
	namelist = aocNameListNew(256);

	aocMsgQueueSetNameList(aoc, namelist);

	/* Connect to chat server */
	if(!aocConnect(aoc, addr))
	{
		printf("Fatal socket error.\n");
		return 1;
	}

	/* The main loop */
	while(running)
	{
		/* Poll the connection */
		aocPollVarArg(1, 0, 1, aoc);

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
				handle_message(event->aoc, (aocMessage *)event->data);
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

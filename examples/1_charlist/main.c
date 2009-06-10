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
	#include <unistd.h>
	#include <netinet/in_systm.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
#endif
#include <aochat.h>


int running = 1;
char *ao_user, *ao_pass;
aocNameList *namelist;


void handle_message(aocConnection *aoc, aocMessage *msg)
{
	switch(msg->type)
	{
	case AOC_SRV_LOGIN_SEED:
		{
			char *key = aocKeyexGenerateKey(AOC_STR(msg->argv[0]), ao_user, ao_pass);

			if(key == NULL)
			{
				/* This will never happen */
				printf("Could not generate login key.\n");
				running = 0;
				break;
			}

			aocSendLoginResponse(aoc, 0, ao_user, key);

			aocFree(key);
		}
		break;


	case AOC_SRV_LOGIN_CHARLIST:
		{
			int nchars, i;

			nchars = aocMsgArraySize(msg, 0);

			printf("%d characters available:\n", nchars);
			for(i=0; i<nchars; i++)
			{
				void *user_id, *charname, *level, *online;

				user_id  = aocMsgArrayValue(msg, 0, i, NULL, NULL);
				charname = aocMsgArrayValue(msg, 1, i, NULL, NULL);
				level    = aocMsgArrayValue(msg, 2, i, NULL, NULL);
				online   = aocMsgArrayValue(msg, 3, i, NULL,NULL);

				printf("%10d %15s %10d %10d\n",
					user_id  ? (int)AOC_INT(user_id)  : (int)AOC_INVALID_UID,
					charname ? AOC_STR(charname)      : "-",
					level    ? (int)AOC_INT(level)    : 0,
					online   ? (int)AOC_INT(online)   : -1);
			}
			running = 0;
		}
		break;


	case AOC_SRV_LOGIN_ERROR:
		puts(AOC_STR(msg->argv[0]));
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

	if(argc != 3)
	{
		printf("Usage: charlist [username] [password]\n");
		return 1;
	}

	ao_user = argv[1];
	ao_pass = argv[2];

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


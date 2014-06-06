#include "stdio.h"
#include "winsock2.h"
#include <errno.h>

void DisplayError(char *text)
{
	int   dwLastError = GetLastError();
	TCHAR   lpBuffer[256];
	if(dwLastError != 0)    // Don't want to see a "operation done successfully" error ;-)
	{
    	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,                 // Its a system error
                     NULL,                                      // No string to be formatted needed
                     dwLastError,                               // Hey Windows: Please explain this error!
                     MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),  // Do it in the standard language
                     lpBuffer,              // Put the message here
                     255,                     // Number of bytes to store the message
                     NULL);
		printf("%s: %d (%d) - %s\n", text, dwLastError, errno, (char*)lpBuffer);
	}
}

int WorkClient(int sock);

//TODO: добавить множество клиентов (заменить client_sock на список)

void main()
{
	WSADATA data;
	int on = 1, number, cnt=0;
	char buff[1024], str[1024];
	SOCKET sock, client_sock = 0;
	DWORD dw = 1;
	int len;
	struct sockaddr_in addr;
	int sel_res;
	struct fd_set read_s, work;
	struct timeval time_out;
	int i;

	printf("Server with non blocked sockets\n");

	if( WSAStartup(0x0202, &data) != 0)
	{
		DisplayError("WSAStartip");
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if( sock == INVALID_SOCKET )
	{
		DisplayError("socket");
	}

	if( setsockopt(sock, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on)) == SOCKET_ERROR )
	{
		DisplayError("Set reusable socket");
	}

	ioctlsocket(sock, FIONBIO, &dw);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(555);
	addr.sin_addr.s_addr = 0;

	if( bind(sock, (struct sockaddr*) &addr, sizeof(addr)) != 0 )
	{
		DisplayError("bind");
	}

	if( listen(sock, 0x100) != 0 )
	{
		DisplayError("listen");
	}

	time_out.tv_sec = 0;
	time_out.tv_usec = 500000; //Таймаут 0.5 секунды.
	FD_ZERO (&work);
	while( 1)
	{
		switch(cnt % 4)
		{
		case 0: printf("-"); break;
		case 1: printf("\\"); break;
		case 2: printf("|"); break;
		case 3: printf("/"); break;
		}
		printf("\r");
		cnt++;

		FD_ZERO (&read_s);

		FD_SET (sock, &read_s);
		//копируем все новые сокеты
		for(i=0; i<work.fd_count; i++)
			FD_SET(work.fd_array[i], &read_s);

		sel_res =  select(0, &read_s, NULL, NULL, &time_out); 

		if( (sel_res != 0) && !(FD_ISSET (sock, &read_s) )) //кто-то стучится, но не для подключения
		{
			for( i=0; i< read_s.fd_count; i++)
			{
				if( WorkClient(read_s.fd_array[i]) == -1 )
				{
					printf("disconnect\n");
					client_sock = 0;
					FD_CLR(read_s.fd_array[i], &work); //удаляем отключившегося из рабочих сокетов					
				}
			}
		}

		if ((sel_res!=0) && (FD_ISSET (sock, &read_s)) ) //кто-то хочет подключиться
		{
			client_sock = accept(sock, NULL, NULL);
			
			if( client_sock != SOCKET_ERROR )
			{
				setsockopt( client_sock, SOL_SOCKET, SO_RCVTIMEO, ( const char * ) &time_out, sizeof( time_out ) );
                setsockopt( client_sock, SOL_SOCKET, SO_SNDTIMEO, ( const char * ) &time_out, sizeof( time_out ) );
				if( ioctlsocket(client_sock, FIONBIO, &dw) == SOCKET_ERROR )
				{
					DisplayError("client_socket set nonBlock");
					break;
				}
				send(client_sock, "test", 4, 0);
				printf("connected");
				FD_SET(client_sock, &work);
			}
		}
	}

	getchar();
}

//обработать клиента
int WorkClient(int sock)
{
	int len, number;
	char buff[1024];
	int e;
	while(1)
	{
		len = recv(sock, buff, 1024, 0);
		if( len != SOCKET_ERROR || len == 0 || errno == 0)
			break;
		DisplayError("WorkClient");
	}

	if( len > 0)
	{
		buff[len] = 0;
		printf("received: %s ... ", buff);

		//формируем ответ
		number = atoi(buff);
		sprintf(buff, "%f", number/2.0);
		send(sock, buff, strlen(buff), 0);
		printf("answer: %s\n", buff);
		return 1;
	}
	else
	{
		return len; //тут либо -1(дисконнект), либо 0(не помню что)
	}
}
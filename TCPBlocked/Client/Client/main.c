#include "stdio.h"
#include "string.h"
#include "winsock2.h"

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
		printf("%s: %d - %s\n", text, dwLastError, (char*)lpBuffer);
	}
}

void main()
{
	char command[1000], buff[1000];
	WSADATA data;
	SOCKET sock;
	struct sockaddr_in addr;
	int cnt;

	printf("Client\n");

	printf("Connecting\n");

	if( WSAStartup(0x0202, &data) != 0 )
	{
		DisplayError("WSAStartup");
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if( sock == INVALID_SOCKET )
	{
		DisplayError("socket");
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(555);

	if( inet_addr("127.0.0.1")  != INADDR_NONE )
	{
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	}
	else
	{
		printf("fail");
	}

	if( connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0)
	{
		DisplayError("connect");
	}
	else
	{
		printf("Waiting permission...");
		cnt = recv(sock, &buff[0], 1000, 0);
		buff[cnt] = 0;
		printf("%s\n", buff);
	}

	printf("connected to 127.0.0.1:555\n");
	while (1)
	{
		gets(command);

		if( strcmp("quit", command) == 0 ) break;

		send(sock, command, strlen(command), 0);

		cnt = recv(sock, &buff[0], 1000, 0);

		buff[cnt] = 0;
		printf("%s\n", buff);
	}

	closesocket(sock);
	WSACleanup();

}
#include "stdio.h"
#include "winsock2.h"
#include "windows.h"

#define sHELLO "You are connected\r\n"

void DisplayError(char *text)
{
	int   dwLastError = GetLastError();
	TCHAR   lpBuffer[256];
	if(dwLastError != 0)    // Don't want to see a "operation done successfully" error ;-)
    	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,                 // Its a system error
                     NULL,                                      // No string to be formatted needed
                     dwLastError,                               // Hey Windows: Please explain this error!
                     MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),  // Do it in the standard language
                     lpBuffer,              // Put the message here
                     255,                     // Number of bytes to store the message
                     NULL);
	printf("%s: %d - %s\n", text, dwLastError, (char*)lpBuffer);
}

DWORD WINAPI ClientThread(LPVOID client_socket);

void main()
{
	WSADATA d;
	SOCKET sock, client_sock;
	struct sockaddr_in addr, client_addr;	
	int size = sizeof(client_addr);
	DWORD threadID;

	printf("Server starting...\n");	

	if( WSAStartup(0x0202, &d) != 0 )
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
	addr.sin_addr.s_addr = 0;

	if( bind(sock, (struct sockaddr*) &addr, sizeof(addr)) != 0 )
	{
		DisplayError("bind");
	}

	if( listen(sock, 0x100) != 0 )
	{
		DisplayError("listen");
	}

	while( 1)
	{
		client_sock =  accept(sock, (struct sockaddr *)&client_addr, &size);

		CreateThread(NULL, NULL, ClientThread, &client_sock, NULL, &threadID);

		printf("connected!");
	}

	printf("Work of server is completed");
	getchar();
}


DWORD WINAPI ClientThread(LPVOID client_socket)
{
	SOCKET sock = *(SOCKET*)client_socket;
	int bytes, number;
	char buff[1000], str[1000];
	send(sock, sHELLO, sizeof(sHELLO),0);

	while(1)
	{
		bytes = recv(sock, &buff[0], sizeof(buff), 0);

		if( bytes == 0 )
		{
			printf("disconnected");
			break;
		}

		if( bytes == SOCKET_ERROR )
		{
			DisplayError("Client");

			break;
		}

		buff[bytes] = 0;
		printf("received: %s ... ", buff);

		//формируем ответ
		number = atoi(buff);
		sprintf(str, "%f", number/2.0);
		send(sock, str, strlen(str), 0);
		printf("answer: %s\n", str);
	}

	closesocket(sock);
}
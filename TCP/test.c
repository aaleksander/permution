#include "winsock2.h"
#include "stdio.h"

void DisplayError(char *text)
{
	int   dwLastError = GetLastError();
	char   lpBuffer[256];
	if(dwLastError != 0)    // Don't want to see a "operation done successfully" error ;-)
    	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,                 // It┬┤s a system error
                     NULL,                                      // No string to be formatted needed
                     dwLastError,                               // Hey Windows: Please explain this error!
                     MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),  // Do it in the standard language
                     lpBuffer,              // Put the message here
                     255,                     // Number of bytes to store the message
                     NULL);
    printf("%s: %d - %s\n", text, dwLastError, lpBuffer);
}

DWORD WINAPI SexToClient(LPVOID client_socket);

void main()
{
	WSADATA lpWSAData;
	SOCKET sock;

	if( WSAStartup(0x0202, &lpWSAData) )
	{
		DisplayError("WSAStartup: ");
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr;
	addr.sin_family=AF_INET;
	addr.sin_port=htons(555);
	addr.sin_addr.s_addr=0; // слушаем все IP с указанного порта

	if( bind(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0 )
	{
		DisplayError("bind: ");
	}

	if( listen(sock, 1) != 0 )
	{
		DisplayError("listen: ");
	}


 	printf("Waiting connections:\n");
 
 	//Шаг 5 извлекаем сообщение из очереди
 	SOCKET client_socket; // сокет для клиента
 	struct sockaddr_in client_addr; // адрес клиента (заполняется системой)
 
 	// функции accept необходимо передать размер структуры
 	int client_addr_size=sizeof(client_addr);
 	int nclients = 0;
 	// цикл извлечения запросов на подключение из очереди
 	while((client_socket=accept(sock, (struct sockaddr *) &client_addr, &client_addr_size)))
 	{
  		nclients++; // увеличиваем счетчик подключившихся клиентов
		// пытаемся получить имя хоста
		HOSTENT *hst;
		hst=gethostbyaddr((char *) &client_addr.sin_addr.s_addr,4,AF_INET);
		// вывод сведений о клиенте
		printf("+%s [%s] new connect!\n",(hst)?hst->h_name:"",inet_ntoa(client_addr.sin_addr));
		//PRINTNUSERS
		// Вызов нового потока для обслужвания клиента
		// Да, для этого рекомендуется использовать _beginthreadex
		// но, поскольку никаких вызов функций стандартной Си библиотеки
		// поток не делает, можно обойтись и CreateThread
		DWORD thID;

	  	CreateThread(NULL,0,SexToClient,&client_socket,0,&thID);
 	}

	printf("%d\n", sock);
}

// Эта функция создается в отдельном потоке
// и обсуживает очередного подключившегося клиента независимо от остальных
DWORD WINAPI SexToClient(LPVOID client_socket)
{
	SOCKET my_sock;
	my_sock=((SOCKET *) client_socket)[0];
	char buff[20*1024];

	// отправляем клиенту приветствие 
	send(my_sock, "You are connected", 6,0);
	// цикл эхо-сервера: прием строки от клиента и возвращение ее клиенту
	int bytes_recv;
	while((bytes_recv=recv(my_sock,&buff[0],sizeof(buff),0)) && bytes_recv !=SOCKET_ERROR)
	{
		printf("%s\n", buff);
		send(my_sock,&buff[0],bytes_recv,0);
	}

	// если мы здесь, то произошел выход из цикла по причине 
	// возращения функцией recv ошибки - соединение с клиентом разорвано

	//nclients--; // уменьшаем счетчик активных клиентов
	printf("disconnect\n");
	// закрываем сокет
	closesocket(my_sock);
	return 0;
}

//ТЗ: сделать сервер, который будет ждать подключение пользователей.
//сервер должен сообщать о подключении пользователя, об его отключении, 
//и о том, что пользователь передал сообщение

#include <windows.h>
#include "stdio.h"
#include "common.h"


void main()
{
   	HANDLE port;
	const int READ_TIME = 100;
	OVERLAPPED sync = {0};
	int reuslt = 0;
	unsigned long wait = 0, read = 0, state = 0;

   	port = CreateFile("\\\\.\\COM2", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
   		0/*FILE_FLAG_OVERLAPPED*/, NULL);
   	if (port == INVALID_HANDLE_VALUE) {
      	printf("I cant open COM2: %d", GetLastError());
      	DisplayError("I cant open COM2");
   	}

	//настраиваем порт
   	DCB info;
    memset(&info, 0, sizeof(info));
    info.DCBlength = sizeof(info);

    if ( !GetCommState(port, &info))    
        DisplayError("GetCommState");

    if (!BuildCommDCB("baud=9600 parity=n data=8 stop=1", &info))
        DisplayError("BuildCommDCB");

    if (!SetCommState(port, &info))
        DisplayError("SetComState");

	COMMTIMEOUTS timeouts;
// set short timeouts on the comm port.
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.ReadTotalTimeoutConstant = 1;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = 1;
    if (!SetCommTimeouts(port, &timeouts))
        DisplayError("SetCommTimeouts");

	DWORD iSize, res;
	char sReceivedChar;
	while (1)
	{
		ReadFile(port, &sReceivedChar, 1, &iSize, 0);
		if ( iSize )
		{
			printf ("%d", sReceivedChar);
			WriteFile(port, "ok", 2, &res, NULL);
		}
	}

   	CloseHandle(port);
}
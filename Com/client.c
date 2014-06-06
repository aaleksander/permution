#include <windows.h>
#include "stdio.h"

void DisplayError()
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
    printf("%d: %s\n", dwLastError, lpBuffer);
}

void main()
{
   	HANDLE port;
	const int READ_TIME = 100;
	OVERLAPPED sync = {0};
	int reuslt = 0;
	unsigned long wait = 0, read = 0, state = 0;

   	port = CreateFile("\\\\.\\COM3", GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 
   		0/*FILE_FLAG_OVERLAPPED*/, NULL);
   	if (port == INVALID_HANDLE_VALUE) {
      	printf("I cant open COM3: %d", GetLastError());
      	DisplayError();
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

   	DWORD res;
	DWORD iSize;
	char sReceivedChar[3] ;

   	WriteFile(port, "2", 1, &res, NULL);		
	while (1)
	{
		ReadFile(port, &sReceivedChar, 2, &iSize, 0);
		if ( iSize )
		{
			sReceivedChar[iSize] = 0;
			printf ("%d: %s", iSize, sReceivedChar);
		 	break;
		}
	}

   	CloseHandle(port);
}
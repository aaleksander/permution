#include <windows.h>
#include "stdio.h"
#include "common.h"


//небольшой чат. В качестве параметро указывать свой порт и порт адресата




int main(int argc, char* argv[])
{

   	if (argc < 1)// если передаем аргументы, то argc будет больше 1(в зависимости от кол-ва аргументов)
   	{
        printf("primer: chat.exe 2, gde '2' - nomer com-porta ");
   	}

   	printf("Connecting to COM%s...", argv[1]);

   	char portName[10];
	sprintf(portName, "\\\\.\\COM%s", argv[1]);

   	HANDLE port;
	const int READ_TIME = 100;
	OVERLAPPED sync = {0};
	int reuslt = 0;
	unsigned long wait = 0, read = 0, state = 0;

   	port = CreateFile("\\\\.\\COM2", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
   		FILE_FLAG_OVERLAPPED, NULL);
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

    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.ReadTotalTimeoutConstant = 1;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = 1;
    if (!SetCommTimeouts(port, &timeouts))
        DisplayError("SetCommTimeouts");

    sync.hEvent = CreateEvent(NULL, 1, 0, NULL);
	DWORD iSize, res;
	char sReceivedChar;

	/* Устанавливаем маску на события порта */
	if(SetCommMask(port, EV_RXCHAR)) {		
		/* Связываем порт и объект синхронизации*/
		WaitCommEvent(port, &state, &sync);
		/* Начинаем ожидание данных*/	
		wait = WaitForSingleObject(sync.hEvent, READ_TIME);
		/* Данные получены */		
		if(wait == WAIT_OBJECT_0) {
			printf("222222");
			/* Начинаем чтение данных */
			//ReadFile(port, dst, size, &read, &sync);
			ReadFile(port, &sReceivedChar, 1, &iSize, 0);
			/* Ждем завершения операции чтения */
			wait = WaitForSingleObject(sync.hEvent, READ_TIME);
			/* Если все успешно завершено, узнаем какой объем данных прочитан */
			if(wait == WAIT_OBJECT_0) 
				if(GetOverlappedResult(port, &sync, &read, FALSE)) 
					printf("ok");
			//		reuslt = read;
		}
	}

   	printf("exiting\n");
    return 0;
}
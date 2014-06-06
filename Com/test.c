#include "stdio.h"
#include "windows.h"

void DisplayError()
{
	int   dwLastError = GetLastError();
	char   lpBuffer[256];
	if(dwLastError != 0)    // Don't want to see a "operation done successfully" error ;-)
    	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,                 // It´s a system error
                     NULL,                                      // No string to be formatted needed
                     dwLastError,                               // Hey Windows: Please explain this error!
                     MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),  // Do it in the standard language
                     lpBuffer,              // Put the message here
                     255,                     // Number of bytes to store the message
                     NULL);
    printf("%d: %s\n", dwLastError, lpBuffer);
}

HANDLE NewFile(char *fileName)
{	//открываем файл либо пересоздает
	//возвращает декриптор файла
	HANDLE hFile;

   	hFile = CreateFile(
		fileName,     // address of name of the communications device
      	GENERIC_WRITE,          // access (read-write) mode
      	0,                  // share mode
      	NULL,               // address of security descriptor
      	CREATE_NEW,      // how to create
      	FILE_ATTRIBUTE_NORMAL,                  // file attributes
      	NULL                // handle of file with attributes to copy
   	);

   	if( hFile == INVALID_HANDLE_VALUE ) 
   	{
   		int err = GetLastError();
   		switch(err)
   		{
   			case 80: 
   				DeleteFile(fileName);
			   	hFile = CreateFile(
					fileName,     // address of name of the communications device
			      	GENERIC_WRITE,          // access (read-write) mode
			      	0,                  // share mode
			      	NULL,               // address of security descriptor
			      	CREATE_NEW,      // how to create
			      	FILE_ATTRIBUTE_NORMAL,                  // file attributes
			      	NULL                // handle of file with attributes to copy
			   	);
   				break;
   		}
         if( hFile == INVALID_HANDLE_VALUE )
         {//ничего не получилось
            DisplayError();
         }
   	}

	return hFile;
}


main()
{
	HANDLE hFile;
   LPDWORD res;
   OVERLAPPED osReader = {0};
   osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	printf("Start\n");

	printf("Openning file... ");

	hFile = NewFile("test.tmp");

 	printf("result:\t%d\n", hFile);

   WriteFile(
      hFile,                    // дескриптор файла
      "ddddd",                // буфер данных
      5,     // число байтов для записи
      res,  // число записанных байтов
      NULL        // асинхронный буфер
   );

  	CloseHandle(hFile);

  	printf("Press any key...");
	char ch = getch();
}



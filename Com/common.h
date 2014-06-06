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
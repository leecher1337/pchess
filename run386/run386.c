/*
 * Power Chess RUN386.EXE launcher and pipe relay.
 * This process is needed so that Power Chess can run on newer
 * NT-based operating systems like Windows XP etc.
 *
 * This is necessary, because PIPE-I/O does not work properly, when
 * directly attaching to the NTVDM process with PCHESS.EXE
 * Please note that you also need to patch CreateProcess flags in 
 * PCHESS.EXE
 *
 * leecher@dose.0wnz.at
 * 11/2021
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Normally implemeted in Shell-API, but we want to save space
static LPTSTR PathGetArgs (LPTSTR pszPath)
{
    if( *pszPath== '"' )
    {
		pszPath++;
        while(*pszPath&& (*pszPath!= '"')) pszPath++;
        if(*pszPath== '"') pszPath++;
    }
    else    
    {
        while ( *pszPath> ' ' ) pszPath++;
    }

    while ( *pszPath&& (*pszPath<= ' ') ) pszPath++;
	return pszPath;
}


static DWORD WINAPI ReaderThread(LPVOID lpParams)
{
	HANDLE *hHandles = (HANDLE*)lpParams;
	DWORD dwRead;
	char buf[1];

	while (ReadFile(hHandles[0], buf, sizeof(buf), &dwRead, NULL))
		WriteFile(hHandles[1], buf, dwRead, &dwRead, NULL);
	return 0;
}

static DWORD WINAPI WriterThread(LPVOID lpParams)
{
	HANDLE *hHandles = (HANDLE*)lpParams;
	DWORD dwRead;
	char buf[1024];

	while (ReadFile(hHandles[0], buf, sizeof(buf), &dwRead, NULL))
		WriteFile(hHandles[1], buf, dwRead, &dwRead, NULL);
	return 0;
}


void mainCRTStartup(void)
{
	HANDLE hStdInPipeRead , hStdInPipeWrite;
	HANDLE hStdOutPipeRead, hStdOutPipeWrite;
	HANDLE hStdErrPipeRead, hStdErrPipeWrite;
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	char *pszCmd = PathGetArgs (GetCommandLine()), *p, szCmdLine[MAX_PATH], szAppName[MAX_PATH];
	HANDLE hStdOut[2], hStdIn[2], hStdErr[2];
	HANDLE hReaderThread, hWriterThread, hErrThread;
	DWORD dwExitCode;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	
	// Parse commandline
	p = szCmdLine;
	p += GetSystemDirectory(p, sizeof(szCmdLine));
	lstrcpy(p, "\\cmd.exe /c run386d");
	lstrcpyn(szAppName, szCmdLine, p-szCmdLine+9);
	p += 19;
	lstrcpy(p, pszCmd-1);

	// Create needed pipes
    if (!CreatePipe(&hStdInPipeRead, &hStdInPipeWrite, &sa, 0) ||
		!CreatePipe(&hStdOutPipeRead, &hStdOutPipeWrite, &sa, 0) ||
		!CreatePipe(&hStdErrPipeRead, &hStdErrPipeWrite, &sa, 0))
		ExitProcess(-1);
    
	// Connect pipes to child
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdError = hStdErrPipeWrite;
    si.hStdOutput = hStdOutPipeWrite;
    si.hStdInput = hStdInPipeRead;

	// Start child process
    if (!CreateProcess(szAppName, szCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
		ExitProcess(-1);

	// Close unused pipe ends
	CloseHandle(hStdInPipeRead);
	CloseHandle(hStdOutPipeWrite);
	CloseHandle(hStdErrPipeWrite);

	// Start pipe relay threads (sorry for ugly threading, but was unable to do it with overlapped I/O)
	hStdOut[0] = hStdOutPipeRead;
	hStdOut[1] = GetStdHandle(STD_OUTPUT_HANDLE);
	hReaderThread = CreateThread(NULL, 0, ReaderThread, &hStdOut, 0, NULL);

	hStdErr[0] = hStdErrPipeRead;
	hStdErr[1] = GetStdHandle(STD_ERROR_HANDLE);
	hErrThread = CreateThread(NULL, 0, ReaderThread, &hStdErr, 0, NULL);

	hStdIn[0] = GetStdHandle(STD_INPUT_HANDLE);
	hStdIn[1] = hStdInPipeWrite;
	hWriterThread = CreateThread(NULL, 0, WriterThread, &hStdIn, 0, NULL);

	// Wait for process to terminate
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Terminate replay threads
	TerminateThread(hReaderThread, 0);
	TerminateThread(hWriterThread, 0);
	TerminateThread(hErrThread, 0);

    // Clean up and exit.
    CloseHandle(hStdOutPipeRead);
	CloseHandle(hStdErrPipeRead);
    CloseHandle(hStdInPipeWrite);

    GetExitCodeProcess(pi.hProcess, &dwExitCode);

	ExitProcess(dwExitCode);
}
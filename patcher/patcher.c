#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "resource.h"

static void print_ok(char *pszString)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (hConsole, &csbi);
	SetConsoleTextAttribute (hConsole, (WORD)(FOREGROUND_INTENSITY | FOREGROUND_GREEN));
	printf (pszString);
	SetConsoleTextAttribute (hConsole, csbi.wAttributes);
}


BOOL ShowModuleError(LPSTR pszModError, LPSTR pszError)
{
	LPTSTR lpMsgBuf;
	DWORD err = GetLastError();
	HANDLE hConsole;
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo (hConsole, &csbi);
	SetConsoleTextAttribute (hConsole, (WORD)(FOREGROUND_INTENSITY | FOREGROUND_RED));
    fprintf (stderr, "%s: %s: 0x%08X: %s\n",
        pszModError, pszError, err, lpMsgBuf);
	SetConsoleTextAttribute (hConsole, csbi.wAttributes);
	LocalFree( lpMsgBuf );
	return FALSE;
}

BOOL ShowError(LPSTR pszError)
{
	return ShowModuleError("Failed patching file", pszError);
}

static BYTE *Match (BYTE *lpMem, DWORD dwSize, const BYTE *lpSig, DWORD cbSig, DWORD step)
{
	BYTE *lpOffset, *lpEnd;
	DWORD i;

	for (lpOffset=lpMem, lpEnd=lpMem+dwSize-cbSig; lpOffset<lpEnd; lpOffset+=step)
	{
		for (i=0; i<cbSig; i++)
			if (lpSig[i]!=0x99 && lpOffset[i]!=lpSig[i]) break;
		if (i==cbSig) return lpOffset;
	}
	return NULL;
}

static BOOL PatchFile(char *pszFile)
{
	HANDLE hFile;
	BOOL bRet=FALSE;
	BYTE Sig[] = {0x50, 0x6A, 0x00, 0x68, 0x99, 0x08, 0x00, 0x99, 0x6A, 0x01};
	PBYTE lpSig, lpMem;

	printf ("Checking %s....", pszFile);
	hFile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwSize;
		HANDLE hMap;

		dwSize = GetFileSize(hFile, NULL);
		if (hMap = CreateFileMapping (hFile, NULL, PAGE_READWRITE | SEC_COMMIT, 0, 0, NULL))
		{
			if (lpMem = MapViewOfFile (hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0))
			{
				if (lpSig = Match(lpMem, dwSize, Sig, sizeof(Sig), 1))
				{
					if (lpSig[4] == 0x00 && lpSig[7] == 0x08)
						print_ok("Already patched\n");
					else
					{
						lpSig[4] = 0x00;
						lpSig[7] = 0x08;
						print_ok("Patch applied\n");
					}
					bRet = TRUE;
				}
				else
					ShowError("Cannot find signature in file");

				UnmapViewOfFile (lpMem);
			}
			else
				ShowError("Cannot map view of file");
			CloseHandle (hMap);
		}
		else
			ShowError("Cannot map view of file");
		CloseHandle(hFile);
	}
	else
		ShowError("Cannot open file for reading");
	
	return bRet;
}

static BOOL DropDLL(char *pszFile)
{
    HRSRC hResID;
    HGLOBAL hRes;
    LPVOID pRes;
	HANDLE hResFile, hInstance = GetModuleHandle(NULL);
	BOOL bRet;
	DWORD dwWritten, dwResSize;
    
	if (!(hResID = FindResource(hInstance, MAKEINTRESOURCE(IDR_RUN386), "DLL")) ||
		!(hRes = LoadResource(hInstance, hResID)) ||
		!(pRes = LockResource(hRes))) return FALSE;  

    dwResSize = SizeofResource(hInstance, hResID);
    if ((hResFile = CreateFile(pszFile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL)) != INVALID_HANDLE_VALUE)
	{
		bRet = WriteFile(hResFile, pRes, dwResSize, &dwWritten, NULL);
		CloseHandle(hResFile);
		return bRet;
    }

    return FALSE;
}


int main(int argc, char **argv)
{
	DWORD dwOffset;
	char szPath[MAX_PATH], *p;

	printf ("Powerchess Win32 patcher\n(c)oded by leecher@dose.0wnz.at, 2021\n\n");

	if (!(dwOffset = GetPrivateProfileString("PowChess", "InstallPath", "C:\\SIERRA\\POWCHESS", szPath, sizeof(szPath), "sierra.ini")))
	{
		ShowModuleError("Error", "Cannot find Powerchess installation path in sierra.inf");
		return -1;
	}
	p=szPath+dwOffset;
	*p++='\\';
	
	strcpy(p, "PCHESS.EXE");
	if (GetFileAttributes(szPath) == 0xFFFFFFFF)
	{
		ShowModuleError("Cannot find Powerchess in installation path", szPath);
		return -1;
	}
	PatchFile(szPath);

	printf ("Installing RUN386.EXE wrapper...");
	strcpy(p, "RUN386D.EXE");
	if (GetFileAttributes(szPath) == 0xFFFFFFFF)
	{
		char szDest[MAX_PATH];

		strcpy(szDest, szPath);
		strcpy(p, "RUN386.EXE");
		if (!MoveFile(szPath, szDest))
		{
			ShowModuleError("Error", "Cannot rename RUN386.EXE");
			return -1;
		}
		if (!DropDLL(szPath))
		{
			ShowModuleError("Error", "Cannot drop wrapper");
			MoveFile(szDest, szPath);
			return -1;
		}
		print_ok("Installed\n");
	} else print_ok("Already installed.\n");


	printf ("Press any key to exit...");
	getch();

	return 0;
}

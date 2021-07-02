#include <stdio.h>
#include <tchar.h>
#include <Windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
	
	WCHAR AbsolutePathA[MAX_PATH];
	WCHAR AbsolutePathB[MAX_PATH];
	WCHAR OutputFile[MAX_PATH];
	ZeroMemory(AbsolutePathA, MAX_PATH);
	ZeroMemory(AbsolutePathB, MAX_PATH);
	ZeroMemory(OutputFile, MAX_PATH);
	GetCurrentDirectory(MAX_PATH, AbsolutePathA);
	GetCurrentDirectory(MAX_PATH, AbsolutePathB);
	WCHAR *ProgramB = argv[1];
	WCHAR ProgramA[MAX_PATH] = L"\\SelectMyParent.exe";
	memcpy(AbsolutePathA + lstrlenW(AbsolutePathA), ProgramA, lstrlenW(ProgramA)*2);
	AbsolutePathB[lstrlenW(AbsolutePathB)] = '\\';
	memcpy(AbsolutePathB + lstrlenW(AbsolutePathB), ProgramB, lstrlenW(ProgramB)*2);
	memcpy(OutputFile, AbsolutePathB, lstrlenW(AbsolutePathB)*2);
	memcpy(OutputFile + lstrlenW(AbsolutePathB) - 4, L"_protected.exe", lstrlenW(L"_protected.exe") * 2);
	wprintf(L"%s\r\n", OutputFile);

	HANDLE hAbsolutePathA = CreateFile(AbsolutePathA, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hAbsolutePathA)
	{
		wprintf(L"Open hAbsolutePathA Error : %d\r\n", GetLastError());
		return 0;
	}

	HANDLE hAbsolutePathB = CreateFile(AbsolutePathB, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hAbsolutePathB)
	{
		wprintf(L"Open hAbsolutePathB Error : %d\r\n", GetLastError());
		return 0;
	}

	HANDLE hOutputFile = CreateFile(OutputFile, GENERIC_ALL, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(INVALID_HANDLE_VALUE == hOutputFile)
	{
		wprintf(L"Create OutputFile Error : %d\r\n", GetLastError());
		return 0;
	}

	DWORD ASize = GetFileSize(hAbsolutePathA, NULL);
	DWORD BSize = GetFileSize(hAbsolutePathB, NULL);

	PBYTE AFileBuff = (PBYTE)malloc(ASize);
	PBYTE BFileBuff = (PBYTE)malloc(BSize);

	DWORD readSizeA = 0, readSizeB = 0;
	if (!ReadFile(hAbsolutePathA, AFileBuff, ASize, &readSizeA, NULL)) 
	{
		wprintf(L"ReadFile A Error : %d\r\n", GetLastError());
		return 0;
	}
	
	if(!ReadFile(hAbsolutePathB, BFileBuff, BSize, &readSizeB, NULL))
	{
		wprintf(L"ReadFile B Error : %d\r\n", GetLastError());
		return 0;
	}

	DWORD OutputFileSize = 0;
	if(!WriteFile(hOutputFile, AFileBuff,readSizeA,&OutputFileSize,NULL))
	{
		wprintf(L"WriteFile A Error : %d\r\n", GetLastError());
		return 0;
	}

	for (size_t i = 0; i < readSizeB; i++)
	{
		BFileBuff[i] ^= 0xCC;
	}

	if (!WriteFile(hOutputFile, BFileBuff, readSizeB, &OutputFileSize, NULL))
	{
		wprintf(L"WriteFile B Error : %d\r\n", GetLastError());
		return 0;
	}

	if (!WriteFile(hOutputFile, &ASize, 4, &OutputFileSize, NULL)) 
	{
		wprintf(L"WriteFile A Length Error : %d\r\n", GetLastError());
		return 0;
	}

	CloseHandle(hAbsolutePathA);
	CloseHandle(hAbsolutePathB);
	CloseHandle(hOutputFile);

	return 0;
}
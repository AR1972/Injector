//
/* This program is free software. It comes without any warranty, to
* the extent permitted by applicable law. You can redistribute it
* and/or modify it under the terms of the Do What The Fuck You Want
* To Public License, Version 2, as published by Sam Hocevar. See
* http://sam.zoy.org/wtfpl/COPYING for more details. */
//
#include "stdafx.h"
using namespace std;
#define MAX_SLPSTRING 32

int _tmain(int argc, _TCHAR* argv[])
{
	ofstream outfile;
	int ret = 1;
	UCHAR Key[16] = {};
	wchar_t EntryName[24] = {};
	wchar_t Description[] = L"Injector\0";
	wchar_t WindSLIC[] = L"WindSLIC\0";
	wchar_t BootFile[] = L"\\BOOTX64.EFI\0";
	ULONG BootEntryLength = 0;
	HRSRC hResource;
	HGLOBAL hgResource;
	size_t WindSlicSize;
	ULONG EntryId;
	UCHAR* WindSlicBuffer;
	UCHAR* BootEntry;
	wchar_t* UId = NULL;
	wchar_t* DosDevice = NULL;
	wchar_t* Directory = NULL;
	wchar_t* OldDirectory = NULL;
	wchar_t* FilePath = NULL;
	wchar_t* BootEntryPath = NULL;
	//
	if (!InitLib(TRUE)) {
		wprintf_s(L"lib initialization error\n");
		return 1;
	}
	if (!isEfi()) {
		wprintf_s(L"system is not EFI\n");
		return 2;
	}
	//
	try {
		DosDevice = new wchar_t[8];
		Directory = new wchar_t[MAX_PATH];
		OldDirectory = new wchar_t[MAX_PATH];
		FilePath = new wchar_t[MAX_PATH];
		BootEntryPath = new wchar_t[MAX_PATH];
		UId = new wchar_t[sizeof(GUID) * 2 + 4]();
		GetFreeLetter (DosDevice);
		if(UniqueId(&UId) == ERROR_OUTOFMEMORY) {
			throw bad_alloc();
		}
		wsprintf(Directory, L"%s\\EFI\\%s\0", DosDevice, UId);
		wsprintf(OldDirectory, L"%s\\EFI\\WINDSLIC\0", DosDevice);
		wsprintf(FilePath, L"%s%s\0", Directory, BootFile);
		wsprintf(BootEntryPath, L"\\EFI\\%s%s\0", UId, BootFile);
		// get modify firmware variable privilage.
		AcquirePrivilage();
		// install WindSLIC to ESP.
		MountEsp(DosDevice);
		DeleteRecursive(OldDirectory);
		while (DeleteRecursive(Directory) != 0) {
			Sleep(100);
		}
		if (argv[1] != NULL && wcscmp(argv[1], L"/u\0") == 0) {
			wprintf_s(L"uninstalling Injector\n");
			EfiDeleteDescription(Description);
			EfiDeleteDescription(WindSLIC);
			EfiBootmgrAddFirst();
			goto exit;
		}
		if (CreateDirectory(Directory, NULL)) {
			wprintf_s(L"installing Injector to: %s\n", FilePath);
			// get ChainLoader resource.
			hResource = FindResource(NULL, MAKEINTRESOURCE(101), L"RAW");
			WindSlicSize = SizeofResource(NULL, hResource);
			hgResource = LoadResource(NULL, hResource);
			WindSlicBuffer = new UCHAR[WindSlicSize];
			memcpy(WindSlicBuffer, LockResource(hgResource), WindSlicSize);
			outfile.open(FilePath, ios_base::binary);
			if (outfile.is_open()) {
				outfile.write((char*) WindSlicBuffer, WindSlicSize);
				outfile.close();
			}
			delete[] WindSlicBuffer;
		}
		else {
			wprintf_s(L"create directory failed: %X\n", GetLastError());
			EfiDeleteDescription(Description);
			EfiDeleteDescription(WindSLIC);
			EfiBootmgrAddFirst();
			goto exit;
		}
		// remove any ChainLoader boot entries.
		EfiDeleteDescription(Description);
		EfiDeleteDescription(WindSLIC);
		EfiBootmgrAddFirst();
		// add ChainLoader boot entry.
		EntryId = EfiFreeBootEntry();
		if (EntryId == -1) {
			wprintf_s(L"find free boot entry failed");
		}
		else {
			wsprintf(EntryName, L"Boot%04d", EntryId);
			if(EfiCreateBootEntry(NULL, &BootEntryLength, Description, BootEntryPath) == ERROR_OUTOFMEMORY) {
				throw bad_alloc();
			}
			BootEntry = new UCHAR[BootEntryLength];
			switch(EfiCreateBootEntry(BootEntry, &BootEntryLength, Description, BootEntryPath)) {
			case ERROR_SUCCESS:
				wprintf_s(L"adding Injector boot entry: %s\n", EntryName);
				SetFirmwareEnvironmentVariable(EntryName, EfiGuid, BootEntry, BootEntryLength);
				EfiBootOrderAddFirst(EntryId);
				break;
			case ERROR_OUTOFMEMORY:
				throw bad_alloc();
				break;
			}
			ret = ERROR_SUCCESS;
			delete[] BootEntry;
		}
exit:
		UnmountEsp(DosDevice);
		delete[] UId;
		delete[] BootEntryPath;
		delete[] FilePath;
		delete[] Directory;
		delete[] OldDirectory;
		delete[] DosDevice;
		InitLib(FALSE);
		return ret;
	}
	catch (bad_alloc) { 
		if(NULL != UId) {delete[] UId;}
		if(NULL != Directory) {delete[] Directory;}
		if(NULL != OldDirectory) {delete[] OldDirectory;}
		if(NULL != DosDevice) {delete[] DosDevice;}
		if(NULL != FilePath) {delete[] FilePath;}
		if(NULL != BootEntryPath) {delete[] BootEntryPath;}
		InitLib(FALSE);
		return ERROR_OUTOFMEMORY;
	}
}


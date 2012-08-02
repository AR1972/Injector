//
/* This program is free software. It comes without any warranty, to
* the extent permitted by applicable law. You can redistribute it
* and/or modify it under the terms of the Do What The Fuck You Want
* To Public License, Version 2, as published by Sam Hocevar. See
* http://sam.zoy.org/wtfpl/COPYING for more details. */
//
#include <Uefi.h>
#include "Lib.h"
#include "Main.h"
#include "Common.h"
//
#define SLP_INJECT 1
#define PATCH_TABLES 1
//
EFI_STATUS
	LegacyUnlock (
	VOID
	)
{
	EFI_STATUS Status = EFI_PROTOCOL_ERROR;
	EFI_LEGACY_REGION_PROTOCOL* LegacyRegionProtocol;
	UINT32 Granularity = 0;
	if(BS->LocateProtocol(&LegacyRegionGuid, NULL, (VOID**) &LegacyRegionProtocol) == EFI_SUCCESS) {
		if (LegacyRegionProtocol != NULL) {
			Status = LegacyRegionProtocol->UnLock(LegacyRegionProtocol, 0xF0000, 0xFFFF, &Granularity);
		}
	}
	return Status;
}
//
EFI_STATUS
	LegacyLock (
	VOID
	)
{
	EFI_STATUS Status = EFI_PROTOCOL_ERROR;
	EFI_LEGACY_REGION_PROTOCOL* LegacyRegionProtocol;
	UINT32 Granularity = 0;
	if(BS->LocateProtocol(&LegacyRegionGuid, NULL, (VOID**) &LegacyRegionProtocol) == EFI_SUCCESS) {
		if (LegacyRegionProtocol != NULL) {
			Status = LegacyRegionProtocol->Lock(LegacyRegionProtocol, 0xF0000, 0xFFFF, &Granularity);
		}
	}
	return Status;
}
//
EFI_STATUS
	Main(
	VOID
	)
{
	EFI_ACPI_SUPPORT_PROTOCOL *AcpiSupportProtocol;
	EFI_LEGACY_BIOS_PROTOCOL *LegacyBiosProtocol;
	UINT64 Handel = 0;
	UINTN DataSize = 0;
	EFI_TPL OldTpl;
	EFI_STATUS Status = EFI_UNSUPPORTED;
	Rsdp20Tbl_t *RsdpTable = 0;
	Rsdp20Tbl_t *LegacyRsdpTable = 0;
	RsdtTbl_t *RsdtTable = 0;
	XsdtTbl_t *XsdtTable = 0;
	SlicTbl_t *SlicTable = 0;
	AcpiTbl_t *AcpiTable = 0;
	VOID *LegacyAddress = 0;
	UINT8 SlpString[0x20] = { 0 };
	UINTN i = 0;

	//=========================================================================//
	// code starts                                                             //
	//=========================================================================//

	SlicTable = (SlicTbl_t *)SLIC;

	//=========================================================================//
	// add Marker and Public Key to empty SLIC                                 //
	//=========================================================================//

	DataSize = sizeof(Marker_t);
	if (RS->GetVariable(OaMarkerName, &OaMarkerGuid, 0, &DataSize, &SlicTable->Marker) != EFI_SUCCESS ||
		DataSize != sizeof(Marker_t)) {
			return EFI_NOT_FOUND;
	}
	DataSize = sizeof(PublicKey_t);
	if (RS->GetVariable(OaPublicKeyName, &OaPublicKeyGuid, 0, &DataSize, &SlicTable->PublicKey) != EFI_SUCCESS ||
		DataSize != sizeof(PublicKey_t)) {
			return EFI_NOT_FOUND;
	}

	//=========================================================================//
	// copy OemId, OemTableId from Marker to SLIC ACPI header                  //
	//=========================================================================//

	BS->CopyMem(SlicTable->Header.OemId, SlicTable->Marker.OemId, 6);
	BS->CopyMem(SlicTable->Header.OemTableId, SlicTable->Marker.OemTableId, 8);

	//=========================================================================//
	// add SLIC to ACPI tables                                                 //
	//=========================================================================//

	if(BS->LocateProtocol(&AcpiProtocolGuid, NULL, (VOID **) &AcpiSupportProtocol) == EFI_SUCCESS) {
		Status = AcpiSupportProtocol->SetAcpiTable(AcpiSupportProtocol, (VOID *)SLIC, TRUE,
			EFI_ACPI_TABLE_VERSION_1_0B|EFI_ACPI_TABLE_VERSION_2_0|EFI_ACPI_TABLE_VERSION_3_0, &Handel);
	}

	if (Status != EFI_SUCCESS) {
		return Status;
	}

#if SLP_INJECT == 1

	//=========================================================================//
	// add SLP 1.0 string to legacy region                                     //
	//=========================================================================//

	DataSize = sizeof(SlpString);
	if (RS->GetVariable(OaSlpName, &OaSlpGuid, 0, &DataSize, SlpString) == EFI_SUCCESS) {
		if (BS->LocateProtocol(&LegacyBiosGuid, NULL, (VOID **)&LegacyBiosProtocol) == EFI_SUCCESS) {
			if (LegacyBiosProtocol->GetLegacyRegion(LegacyBiosProtocol, sizeof(SlpString), 1, 2, &LegacyAddress) == EFI_SUCCESS) {
				Status = LegacyBiosProtocol->CopyLegacyRegion(LegacyBiosProtocol, DataSize, LegacyAddress, SlpString);
			}
		}
	}
#endif

	//=========================================================================//
	// find ACPI tables                                                        //
	//=========================================================================//

	OldTpl = BS->RaiseTPL(TPL_HIGH_LEVEL);
	Status = GetSystemConfigurationTable (&EfiAcpi20TableGuid, (VOID **)&RsdpTable);
	if (EFI_ERROR (Status)) {
		Status = GetSystemConfigurationTable (&EfiAcpiTableGuid, (VOID **)&RsdpTable);
	}
	if (Status == EFI_SUCCESS) {
		if (RsdpTable->Revision == 0) {
			RsdtTable = (RsdtTbl_t *) RsdpTable->RSDTAddress;
		}
		else if (RsdpTable->Revision == 2) {
			RsdtTable = (RsdtTbl_t *) RsdpTable->RSDTAddress;
			XsdtTable = (XsdtTbl_t *) RsdpTable->XSDTAddress;
		}
		else {
			return EFI_UNSUPPORTED;
		}
	}
	else {
		return EFI_NOT_FOUND;
	}

	//=========================================================================//
	// copy SLIC OemId, OemTableId to RSDP, RSDT, XSDT                         //
	//=========================================================================//

#if PATCH_TABLES == 1
	DataSize = (RsdtTable->Header.Length - sizeof(AcpiHeader_t)) << 2;
	for(i = 0 ; i < DataSize; i++) {
		AcpiTable = (AcpiTbl_t *) RsdtTable->Entry[i];
		if (AcpiTable != NULL) {
			if (CompareMem(AcpiTable->Header.OemId, RsdtTable->Header.OemId, 6) == 0 &&
				CompareMem(AcpiTable->Header.OemTableId, RsdtTable->Header.OemTableId, 8) == 0) {
					BS->CopyMem(AcpiTable->Header.OemId, SlicTable->Header.OemId, 6);
					BS->CopyMem(AcpiTable->Header.OemTableId, SlicTable->Header.OemTableId, 8);
					AcpiTable->Header.Checksum = 0;
					AcpiTable->Header.Checksum = ComputeChecksum((UINT8 *) AcpiTable, AcpiTable->Header.Length);
			}
		}
	}
#endif

	BS->CopyMem(RsdtTable->Header.OemId, SlicTable->Header.OemId, 6);
	BS->CopyMem(RsdtTable->Header.OemTableId, SlicTable->Header.OemTableId, 8);
	RsdtTable->Header.Checksum = 0;
	RsdtTable->Header.Checksum = ComputeChecksum((UINT8 *) RsdtTable, RsdtTable->Header.Length);
	BS->CopyMem(RsdpTable->OemId, SlicTable->Header.OemId, 6);
	RsdpTable->Checksum = 0;
	RsdpTable->Checksum = ComputeChecksum((UINT8 *) RsdpTable, 0x14);
	if(RsdpTable->Revision == 2) {

#if PATCH_TABLES == 1
		DataSize = (XsdtTable->Header.Length - sizeof(AcpiHeader_t)) << 3;
		for(i = 0 ; i < DataSize; i++) {
			AcpiTable = (AcpiTbl_t *) (XsdtTable->Entry[i] & 0xFFFFFFFF);
			if (AcpiTable != NULL) {
				if (CompareMem(AcpiTable->Header.OemId, XsdtTable->Header.OemId, 6) == 0 &&
					CompareMem(AcpiTable->Header.OemTableId, XsdtTable->Header.OemTableId, 8) == 0) {
						BS->CopyMem(AcpiTable->Header.OemId, SlicTable->Header.OemId, 6);
						BS->CopyMem(AcpiTable->Header.OemTableId, SlicTable->Header.OemTableId, 8);
						AcpiTable->Header.Checksum = 0;
						AcpiTable->Header.Checksum = ComputeChecksum((UINT8 *) AcpiTable, AcpiTable->Header.Length);
				}
			}
		}
#endif

		RsdpTable->ExtendedChecksum = 0;
		RsdpTable->ExtendedChecksum = ComputeChecksum((UINT8 *) RsdpTable, RsdpTable->Length);
		BS->CopyMem(XsdtTable->Header.OemId, SlicTable->Header.OemId, 6);
		BS->CopyMem(XsdtTable->Header.OemTableId, SlicTable->Header.OemTableId, 8);
		XsdtTable->Header.Checksum = 0;
		XsdtTable->Header.Checksum = ComputeChecksum((UINT8 *) XsdtTable, XsdtTable->Header.Length);
	}

	//=========================================================================//
	// copy OemId to RSDP in legacy region                                     //
	//=========================================================================//

	LegacyRsdpTable = (Rsdp20Tbl_t *) FindAcpiRsdPtr();
	if (LegacyRsdpTable != NULL && LegacyUnlock() == EFI_SUCCESS)
	{
		BS->CopyMem(LegacyRsdpTable->OemId, SlicTable->Header.OemId, 6);
		LegacyRsdpTable->RSDTAddress = RsdpTable->RSDTAddress;
		LegacyRsdpTable->Checksum = 0;
		LegacyRsdpTable->Checksum = ComputeChecksum((UINT8 *) LegacyRsdpTable, 0x14);
		if(LegacyRsdpTable->Revision == 2) {
			LegacyRsdpTable->XSDTAddress = RsdpTable->XSDTAddress;
			LegacyRsdpTable->ExtendedChecksum = 0;
			LegacyRsdpTable->ExtendedChecksum = ComputeChecksum((UINT8 *) LegacyRsdpTable, LegacyRsdpTable->Length);
		}
		LegacyLock();
	}
	BS->RestoreTPL(OldTpl);
	return Status;
}
//
EFI_STATUS
	EntryPoint (
	EFI_HANDLE ImageHandle,
	EFI_SYSTEM_TABLE* SystemTable
	)
{
	EFI_STATUS Status = 0;
	UINTN i = 0;
	EFI_FILE_IO_INTERFACE *Vol = 0;
	EFI_FILE_HANDLE RootFs = 0;
	EFI_FILE_HANDLE FileHandle = 0;
	EFI_HANDLE* Search = 0;
	EFI_HANDLE DeviceHandle = 0;
	UINTN Size = 0;
	VOID* BootmgrBuffer = 0;
	EFI_FILE_INFO* FileInfoBuffer = 0;
	EFI_HANDLE BootmgrHandle = NULL;
	EFI_LOADED_IMAGE *BootmgrLoadedImage = 0;
	EFI_LOADED_IMAGE *LoadedImage = 0;
	CHAR16 *BOOTMGFW = L"\\EFI\\Microsoft\\BOOT\\BOOTMGFW.EFI";
	CHAR16 *BOOTMGFW_BAK = L"\\EFI\\Microsoft\\BOOT\\BOOTMGFW.BAK";
	EFI_DEVICE_PATH_PROTOCOL* DevPath = 0;

	ST = SystemTable;
	BS = ST->BootServices;
	RS = ST->RuntimeServices;

	Status = Main();

	//=========================================================================//
	// get device handle for the loaded (this) image                           //
	//=========================================================================//

	Status = BS->HandleProtocol(ImageHandle, &gLoadedImageProtocol, (VOID **) &LoadedImage);
	if (EFI_ERROR (Status)) {
		return Status;
	}
	DeviceHandle = LoadedImage->DeviceHandle;

	//=========================================================================//
	// get file io interface for device image was loaded from                  //
	//=========================================================================//

	Status = BS->HandleProtocol(DeviceHandle, &gFileSystemProtocol, (VOID **) &Vol);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	//=========================================================================//
	// open file system root for the device image was loaded from              //
	//=========================================================================//

	Status = Vol->OpenVolume(Vol, &RootFs);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	//=========================================================================//
	// try to open bootmgfw on file system that image was loaded from          //
	//=========================================================================//

	//=========================================================================//
	// look for BOOTMGFW.BAK first to support BOOTMGFW.EFI replacement         //
	// install method.                                                         //
	//=========================================================================//

	Status = RootFs->Open(RootFs, &FileHandle, BOOTMGFW_BAK, EFI_FILE_MODE_READ, 0);
	if (Status == EFI_NOT_FOUND) {
		/* if BOOTMGFW.BAK not found search for BOOTMGFW.EFI */
		Status = RootFs->Open(RootFs, &FileHandle, BOOTMGFW, EFI_FILE_MODE_READ, 0);
		if (EFI_ERROR (Status)) {
			RootFs->Close(RootFs);
			switch(Status) {
			case EFI_NOT_FOUND:

				//=========================================================================//
				// failed to find bootmgfw on same device, look for it on other devices.   //
				// get array of device handle's that bootmgfw might be installed on        //
				// first get size of array                                                 //
				//=========================================================================//

				Size = 0;
				Status = BS->LocateHandle(ByProtocol, &gFileSystemProtocol, NULL, &Size, 0);
				if(Status == EFI_BUFFER_TOO_SMALL) {
					/* allocate memory for array */
					Search = (EFI_HANDLE *) AllocatePool(Size);
				}
				if(Search) {
					/* get the array */
					Status = BS->LocateHandle(ByProtocol, &gFileSystemProtocol, NULL, &Size, Search);
					/* loop through handle's open each file system & try to open bootmgfw */
					if(Status == EFI_SUCCESS) {
						for(i = 0; i < Size / sizeof(EFI_HANDLE); i++) {
							/* we already know bootmgfw is not in the same device as the loaded image, skip */
							if(Search[i] == DeviceHandle) {
								continue;
							}
							/* get file io interface */
							Status = BS->HandleProtocol(Search[i], &gFileSystemProtocol, (VOID **) &Vol);
							if(EFI_ERROR (Status)) {
								continue;
							}
							/* open file system root on the device */
							Status = Vol->OpenVolume(Vol, &RootFs);
							if(EFI_ERROR (Status)) {
								continue;
							}
							/* try to open bootmgfw on the file system */
							Status = RootFs->Open(RootFs, &FileHandle, BOOTMGFW, EFI_FILE_MODE_READ, 0);
							if(Status == EFI_SUCCESS) {
								/* found it, set DeviceHandle & break the loop */
								DeviceHandle = Search[i];
								break;
							}
							/* clean up for next pass */
							RootFs->Close(RootFs);
						}
						/* free array of device handles, if EFI_SUCCESS break */
						/* the switch/case else fall through to the error */
						BS->FreePool(Search);
						if(Status == EFI_SUCCESS) {
							break;
						}
					}
				}
			default:
				return Status;
			}
		}
	}

	//============================================================================//
	// RootFs is open, FileHandle is open, DeviceHandle is set                    //
	// get size of bootmgfw.efi by retriving an EFI_FILE_INFO                     //
	// first get the size of the STRUCT and allocate memory                       //
	//============================================================================//

	Size = 0;
	Status = FileHandle->GetInfo(FileHandle, &gFileInfo, &Size, NULL);
	if(Status == EFI_BUFFER_TOO_SMALL) {
		/* allocate memory for EFI_FILE_INFO */
		FileInfoBuffer = (EFI_FILE_INFO *) AllocatePool(Size);
	} 
	else {
		FileHandle->Close(FileHandle);
		RootFs->Close(RootFs);
		return Status;
	}

	//=========================================================================//
	// get EFI_FILE_INFO for bootmgfw.efi                                      //
	//=========================================================================//

	Status = FileHandle->GetInfo(FileHandle, &gFileInfo, &Size, FileInfoBuffer);
	if(EFI_ERROR(Status)) {
		FileHandle->Close(FileHandle);
		RootFs->Close(RootFs);
		return Status;
	}

	//=========================================================================//
	// get size of bootmgfw.efi                                                //
	//=========================================================================//

	Size = FileInfoBuffer->FileSize;

	//=========================================================================//
	// free EFI_FILE_INFO buffer                                               //
	//=========================================================================//

	BS->FreePool(FileInfoBuffer);

	//=========================================================================//
	// allocate memory for bootmgfw.efi                                        //
	//=========================================================================//

	BootmgrBuffer = AllocatePool(Size);
	if (!BootmgrBuffer) {
		BS->FreePool(BootmgrBuffer);
		FileHandle->Close(FileHandle);
		RootFs->Close(RootFs);
		return Status;
	}

	//=========================================================================//
	// read bootmgfw.efi into buffer                                           //
	//=========================================================================//

	Status = FileHandle->Read(FileHandle, &Size, BootmgrBuffer);
	if (EFI_ERROR (Status)) {
		BS->FreePool(BootmgrBuffer);
		FileHandle->Close(FileHandle);
		RootFs->Close(RootFs);
		return Status;
	}

	//=========================================================================//
	// close handle for bootmgfw.efi                                           //
	//=========================================================================//

	Status = FileHandle->Close(FileHandle);
	if (EFI_ERROR (Status)) {
	}

	//=========================================================================//
	// close handle for file system root                                       //
	//=========================================================================//

	Status = RootFs->Close(RootFs);
	if (EFI_ERROR (Status)) {
	}

	//=========================================================================//
	// load bootmgfw.efi from buffer to execution space                        //
	//=========================================================================//

	Status = BS->LoadImage(FALSE, ImageHandle, 0, BootmgrBuffer, Size, &BootmgrHandle);
	if (EFI_ERROR (Status)) {
		BS->FreePool(BootmgrBuffer);
		return Status;
	}
	BS->FreePool(BootmgrBuffer);

	//=========================================================================//
	// set bootmgfw.efi start variables                                        //
	//=========================================================================//

	Status = BS->HandleProtocol(BootmgrHandle, &gLoadedImageProtocol, (VOID **) &BootmgrLoadedImage);
	if (EFI_ERROR (Status)) {
		return Status;
	}
	BootmgrLoadedImage->DeviceHandle = DeviceHandle;
	BootmgrLoadedImage->ParentHandle = NULL;
	BootmgrLoadedImage->FilePath = FileDevicePath(DeviceHandle, BOOTMGFW);

	//=========================================================================//
	// start bootmgfw.efi execution                                            //
	//=========================================================================//

	Status = BS->StartImage(BootmgrHandle, 0, 0);

	//============================================================================//
	// should never get here show error                                           //
	//============================================================================//

	if (BootmgrHandle != NULL) {
		Status = BS->UnloadImage(BootmgrHandle);
	}
	return Status;
}
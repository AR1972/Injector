#ifndef LIB_H
#define LIB_H
#include <Uefi.h>
//
INTN
CompareMem (
    IN VOID		*Dest,
    IN VOID		*Src,
    IN UINTN	len
    );
//
VOID *
AllocatePool (
    IN UINTN	Size
    );
//
VOID *
AllocateZeroPool (
    IN UINTN	Size
    );
//
UINTN
StrSize (
    IN CHAR16	*s1
    );
//
EFI_DEVICE_PATH *
FileDevicePath (
    IN EFI_HANDLE	Device  OPTIONAL,
    IN CHAR16		*FileName
    );
//
VOID *
	FindAcpiRsdPtr (
	VOID
	);
//
//
UINT8
	ComputeChecksum (
	UINT8	*Buffer,
	UINT32	Length
	);
//
INTN
CompareGuid (
    IN EFI_GUID     *Guid1,
    IN EFI_GUID     *Guid2
    );
//
EFI_STATUS
	GetSystemConfigurationTable (
	IN EFI_GUID *TableGuid,
	IN OUT VOID **Table
	);
//
#endif
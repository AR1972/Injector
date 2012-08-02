#include "Lib.h"
#include "Common.h"
#define ACPI_RSDP_SIG		0x2052545020445352		// "RSD PTR "
#define EBDA_BASE_ADDRESS 0x40E
//
INTN
CompareMem (
    IN VOID     *Dest,
    IN VOID     *Src,
    IN UINTN    len
    )
{
    CHAR8    *d, *s;

    d = (CHAR8*)Dest;
    s = (CHAR8*)Src;
    while (len--) {
        if (*d != *s) {
            return *d - *s;
        }

        d += 1;
        s += 1;
    }

    return 0;
}
//
VOID *
AllocatePool (
    IN UINTN                Size
    )
{
    EFI_STATUS              Status;
    VOID                    *p;

    Status = BS->AllocatePool (EfiBootServicesData, Size, &p);
    if (EFI_ERROR(Status)) {
        p = NULL;
    }
    return p;
}
//
VOID *
AllocateZeroPool (
    IN UINTN                Size
    )
{
    VOID                    *p;

     BS->AllocatePool (EfiBootServicesData, Size, &p);
    if (p) {
        BS->SetMem (p, Size, 0);
    }

    return p;
}
//
UINTN
StrSize (
    IN CHAR16   *s1
    )
// string size
{
    UINTN        len;
    
    for (len=0; *s1; s1+=1, len+=1) ;
    return (len + 1) * sizeof(CHAR16);
}
//
VOID *
	FindAcpiRsdPtr (
	VOID
	)
	/*
	Description:
		finds the RSDP in low memory
	*/
{
	UINTN Address;
	UINTN Index;
	//
	// First Seach 0x0e0000 - 0x0fffff for RSD Ptr
	//
	for (Address = 0xe0000; Address < 0xfffff; Address += 0x10) {
		if (*(UINT64 *)(Address) == ACPI_RSDP_SIG) {
			return (VOID *)Address;
		}
	}
	//
	// Search EBDA
	//
	Address = (*(UINT16 *)(UINTN)(EBDA_BASE_ADDRESS)) << 4;
	for (Index = 0; Index < 0x400 ; Index += 16) {
		if (*(UINT64 *)(Address + Index) == ACPI_RSDP_SIG) {
			return (VOID *)Address;
		}
	}
	return NULL;
}
//
UINT8
	ComputeChecksum (
	UINT8	*Buffer,
	UINT32	Length
	)
	/*
	Description:
		Compute byte checksum on buffer of given length.

	Arguments:
		Buffer		- Pointer to buffer to compute checksum
		Length		- Number of bytes to checksum

	Returns:
		Checksum	- Checksum of buffer
	*/
{
	UINT8	Checksum = 0 ;
	while(Length--)
	{
		Checksum += *Buffer;
		Buffer++;
	}
	return -Checksum ;
}
//
INTN
CompareGuid (
    IN EFI_GUID     *Guid1,
    IN EFI_GUID     *Guid2
    )
/*++

Routine Description:

    Compares to GUIDs

Arguments:

    Guid1       - guid to compare
    Guid2       - guid to compare

Returns:
    = 0     if Guid1 == Guid2

--*/
{
    INT32       *g1, *g2, r;

    //
    // Compare 32 bits at a time
    //

    g1 = (INT32 *) Guid1;
    g2 = (INT32 *) Guid2;

    r  = g1[0] - g2[0];
    r |= g1[1] - g2[1];
    r |= g1[2] - g2[2];
    r |= g1[3] - g2[3];

    return r;
}
//
//
EFI_STATUS
	GetSystemConfigurationTable (
	IN EFI_GUID *TableGuid,
	IN OUT VOID **Table
	)
	/*
	Description:
		Function returns a system configuration table that is stored in the 
		EFI System Table based on the provided GUID.

	Arguments:
		TableGuid        - A pointer to the table's GUID type.

		Table            - On exit, a pointer to a system configuration table.

	Returns:

		EFI_SUCCESS      - A configuration table matching TableGuid was found

		EFI_NOT_FOUND    - A configuration table matching TableGuid was not found
	*/
{
	UINTN Index;
	//
	for (Index = 0; Index < ST->NumberOfTableEntries; Index++) {
		if (CompareGuid (TableGuid, &(ST->ConfigurationTable[Index].VendorGuid)) == 0) {
			*Table = ST->ConfigurationTable[Index].VendorTable;
			return EFI_SUCCESS;
		}
	}
	return EFI_NOT_FOUND;
}
//
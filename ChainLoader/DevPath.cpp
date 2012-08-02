#include <uefi.h>
#include "Lib.h"
#include "Common.h"
//
#define EFI_DP_TYPE_MASK                    0x7F
#define IsDevicePathEndSubType(a)   ( (a)->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE )
#define DevicePathType(a)           ( ((a)->Type) & EFI_DP_TYPE_MASK )
#define IsDevicePathEnd(a)          ( IsDevicePathEndType(a) && IsDevicePathEndSubType(a) )
#define IsDevicePathEndType(a)      ( DevicePathType(a) == END_DEVICE_PATH_TYPE )
#define DevicePathSubType(a)        ( (a)->SubType )
#define DevicePathNodeLength(a)     ( ((a)->Length[0]) | ((a)->Length[1] << 8) )
#define NextDevicePathNode(a)       ( (EFI_DEVICE_PATH *) ( ((UINT8 *) (a)) + DevicePathNodeLength(a)))
#define SetDevicePathNodeLength(a,l) {                  \
            (a)->Length[0] = (UINT8) (l);               \
            (a)->Length[1] = (UINT8) ((l) >> 8);        \
            }

#define SetDevicePathEndNode(a)  {                      \
            (a)->Type = END_DEVICE_PATH_TYPE;           \
            (a)->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;     \
            (a)->Length[0] = sizeof(EFI_DEVICE_PATH);   \
            (a)->Length[1] = 0;                         \
            }
#define END_DEVICE_PATH_LENGTH               (sizeof (EFI_DEVICE_PATH_PROTOCOL))
#define  ASSERT(a)
#define EFI_FIELD_OFFSET(TYPE,Field) ((UINTN)(&(((TYPE *) 0)->Field)))
//
EFI_DEVICE_PATH *
AppendDevicePath (
    IN EFI_DEVICE_PATH  *Src1,
    IN EFI_DEVICE_PATH  *Src2
    );
//
EFI_DEVICE_PATH *
DevicePathFromHandle (
    IN EFI_HANDLE       Handle
    );
//
EFI_DEVICE_PATH *
DuplicateDevicePath (
    IN EFI_DEVICE_PATH  *DevPath
    );
//
UINTN
DevicePathInstanceCount (
    IN EFI_DEVICE_PATH      *DevicePath
    );
//
UINTN
DevicePathSize (
    IN EFI_DEVICE_PATH  *DevPath
    );
//
EFI_DEVICE_PATH *
DevicePathInstance (
    IN OUT EFI_DEVICE_PATH  **DevicePath,
    OUT UINTN               *Size
    );
//
EFI_GUID DevicePathProtocol       = DEVICE_PATH_PROTOCOL;
EFI_DEVICE_PATH EndDevicePath[] = {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, END_DEVICE_PATH_LENGTH, 0
};
//
EFI_DEVICE_PATH EndInstanceDevicePath[] = {
    END_DEVICE_PATH_TYPE, END_INSTANCE_DEVICE_PATH_SUBTYPE, END_DEVICE_PATH_LENGTH, 0
};
//
EFI_DEVICE_PATH *
FileDevicePath (
    IN EFI_HANDLE       Device  OPTIONAL,
    IN CHAR16           *FileName
    )
/*++

    N.B. Results are allocated from pool.  The caller must FreePool
    the resulting device path structure

--*/
{
    UINTN                   Size;
    FILEPATH_DEVICE_PATH    *FilePath;
    EFI_DEVICE_PATH         *Eop, *DevicePath;    

    Size = StrSize(FileName);
    FilePath = (FILEPATH_DEVICE_PATH *) AllocateZeroPool (Size + SIZE_OF_FILEPATH_DEVICE_PATH + sizeof(EFI_DEVICE_PATH));
    DevicePath = (EFI_DEVICE_PATH *) NULL;

    if (FilePath) {

        //
        // Build a file path
        //

        FilePath->Header.Type = MEDIA_DEVICE_PATH;
        FilePath->Header.SubType = MEDIA_FILEPATH_DP;
        SetDevicePathNodeLength (&FilePath->Header, Size + SIZE_OF_FILEPATH_DEVICE_PATH);
        BS->CopyMem (FilePath->PathName, FileName, Size);
        Eop = NextDevicePathNode(&FilePath->Header);
        SetDevicePathEndNode(Eop);

        //
        // Append file path to device's device path
        //

        DevicePath = (EFI_DEVICE_PATH *) FilePath;
        if (Device) {
            DevicePath = AppendDevicePath (
                            DevicePathFromHandle(Device),
                            DevicePath
                            );

            BS->FreePool(FilePath);
        }
    }

    return DevicePath;
}
//
EFI_DEVICE_PATH *
DevicePathFromHandle (
    IN EFI_HANDLE       Handle
    )
{
    EFI_STATUS          Status;
    EFI_DEVICE_PATH     *DevicePath;

    Status = BS->HandleProtocol (Handle, &DevicePathProtocol, (VOID*)&DevicePath);
    if (EFI_ERROR(Status)) {
        DevicePath = (EFI_DEVICE_PATH *) NULL;
    }

    return DevicePath;
}
//
EFI_DEVICE_PATH *
AppendDevicePath (
    IN EFI_DEVICE_PATH  *Src1,
    IN EFI_DEVICE_PATH  *Src2
    )
// Src1 may have multiple "instances" and each instance is appended
// Src2 is appended to each instance is Src1.  (E.g., it's possible
// to append a new instance to the complete device path by passing 
// it in Src2)
{
    UINTN               Src1Size, Src1Inst, Src2Size, Size;
    EFI_DEVICE_PATH     *Dst, *Inst;
    UINT8               *DstPos;

    //
    // If there's only 1 path, just duplicate it
    //

    if (!Src1) {
        ASSERT (!IsDevicePathUnpacked (Src2));
        return DuplicateDevicePath (Src2);
    }

    if (!Src2) {
        ASSERT (!IsDevicePathUnpacked (Src1));
        return DuplicateDevicePath (Src1);
    }

    //
    // Verify we're not working with unpacked paths
    //

//    ASSERT (!IsDevicePathUnpacked (Src1));
//    ASSERT (!IsDevicePathUnpacked (Src2));

    //
    // Append Src2 to every instance in Src1
    //

    Src1Size = DevicePathSize(Src1);
    Src1Inst = DevicePathInstanceCount(Src1);
    Src2Size = DevicePathSize(Src2);
    Size = Src1Size * Src1Inst + Src2Size;
    
    Dst = (EFI_DEVICE_PATH *) AllocatePool (Size);
    if (Dst) {
        DstPos = (UINT8 *) Dst;

        //
        // Copy all device path instances
        //

        while (Inst = DevicePathInstance (&Src1, &Size)) {

            BS->CopyMem(DstPos, Inst, Size);
            DstPos += Size;

            BS->CopyMem(DstPos, Src2, Src2Size);
            DstPos += Src2Size;

            BS->CopyMem(DstPos, EndInstanceDevicePath, sizeof(EFI_DEVICE_PATH));
            DstPos += sizeof(EFI_DEVICE_PATH);
        }

        // Change last end marker
        DstPos -= sizeof(EFI_DEVICE_PATH);
        BS->CopyMem(DstPos, EndDevicePath, sizeof(EFI_DEVICE_PATH));
    }

    return Dst;
}
//
UINTN
DevicePathInstanceCount (
    IN EFI_DEVICE_PATH      *DevicePath
    )
{
    UINTN       Count, Size;

    Count = 0;
    while (DevicePathInstance(&DevicePath, &Size)) {
        Count += 1;
    }

    return Count;
}
//
EFI_DEVICE_PATH *
DevicePathInstance (
    IN OUT EFI_DEVICE_PATH  **DevicePath,
    OUT UINTN               *Size
    )
{
    EFI_DEVICE_PATH         *Start, *Next, *DevPath;
    UINTN                   Count;

    DevPath = *DevicePath;
    Start = DevPath;

    if (!DevPath) {
        return (EFI_DEVICE_PATH *) NULL;
    }

    //
    // Check for end of device path type
    //    

    for (Count = 0; ; Count++) {
        Next = NextDevicePathNode(DevPath);

        if (IsDevicePathEndType(DevPath)) {
            break;
        }

        if (Count > 01000) {
            //
            // BugBug: Debug code to catch bogus device paths
            //
            //DumpHex (0, 0, ((UINT8 *) DevPath) - ((UINT8 *) Start), Start);
            break;
        }

        DevPath = Next;
    }

    ASSERT (DevicePathSubType(DevPath) == END_ENTIRE_DEVICE_PATH_SUBTYPE ||
            DevicePathSubType(DevPath) == END_INSTANCE_DEVICE_PATH_SUBTYPE);

    //
    // Set next position
    //

    if (DevicePathSubType(DevPath) == END_ENTIRE_DEVICE_PATH_SUBTYPE) {
        Next = (EFI_DEVICE_PATH *) NULL;
    }

    *DevicePath = Next;

    //
    // Return size and start of device path instance
    //

    *Size = ((UINT8 *) DevPath) - ((UINT8 *) Start);
    return Start;
}
//
UINTN
DevicePathSize (
    IN EFI_DEVICE_PATH  *DevPath
    )
{
    EFI_DEVICE_PATH     *Start;

    //
    // Search for the end of the device path structure
    //    

    Start = DevPath;
    while (!IsDevicePathEnd(DevPath)) {
        DevPath = NextDevicePathNode(DevPath);
    }

    //
    // Compute the size
    //

    return ((UINTN) DevPath - (UINTN) Start) + sizeof(EFI_DEVICE_PATH);
}
//
EFI_DEVICE_PATH *
DuplicateDevicePath (
    IN EFI_DEVICE_PATH  *DevPath
    )
{
    EFI_DEVICE_PATH     *NewDevPath;
    UINTN               Size;    

    //
    // Check parameter
    //

    if( DevPath == NULL )
    {
        return (EFI_DEVICE_PATH *) NULL;
    }


    //
    // Compute the size
    //

    Size = DevicePathSize (DevPath);

    //
    // Make a copy
    //

    NewDevPath = (EFI_DEVICE_PATH *) AllocatePool (Size);
    if (NewDevPath) {
        BS->CopyMem (NewDevPath, DevPath, Size);
    }

    return NewDevPath;
}
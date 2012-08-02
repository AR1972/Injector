//
/* This program is free software. It comes without any warranty, to
* the extent permitted by applicable law. You can redistribute it
* and/or modify it under the terms of the Do What The Fuck You Want
* To Public License, Version 2, as published by Sam Hocevar. See
* http://sam.zoy.org/wtfpl/COPYING for more details. */
//
// {9CEB005D-273B-4231-A670-F3C86CCFA9AF}
#define OA_MARKER { 0x9ceb005d, 0x273b, 0x4231, 0xa6, 0x70, 0xf3, 0xc8, 0x6c, 0xcf, 0xa9, 0xaf }
// {F12D1538-809F-44B6-870D-6FEA5DA5D6F3}
#define OA_PUBLIC_KEY { 0xf12d1538, 0x809f, 0x44b6, 0x87, 0xd, 0x6f, 0xea, 0x5d, 0xa5, 0xd6, 0xf3 }
// {4002F4A3-4302-431C-B1B7-6510B37126E1} 
#define OA_SLP10 { 0x4002f4a3, 0x4302, 0x431c, 0xb1, 0xb7, 0x65, 0x10, 0xb3, 0x71, 0x26, 0xe1 }
//
#pragma pack(1)
typedef struct {        
    UINT32		Signature;			/* unique signature */
    UINT32		Length;				/* length of the table */
    UINT8		Revision;			/* current rev is 1 */
    UINT8		Checksum;			/* entire table must sum zero */
    UINT8		OEMId[6];			/* string that identifies OEM */
    UINT8		OEMTableId[8];		/* manufacturer model ID */
    UINT32		OEMRevision;		/* OEM revsion of this table */
    UINT8		CreatorId[4];		/* string that identifies creator */
    UINT32		CreatorRevision;	/* creator rev of this table */
} AcpiHeader_t;
//
// SLIC
//
typedef struct {
	UINT32 Ukn0;
	UINT32 Length;
	UINT32 Ukn1;
	UINT8 OemId[6];
	UINT8 OemTableId[8];
	UINT8 CreatorId[8];
	UINT32 Version;
	UINT64 Ukn2;
	UINT64 Ukn3;
	UINT8 Data[128];
} Marker_t;
//
typedef struct {
	UINT32 Ukn0;
	UINT32 Length;
	UINT32 Ukn1;
	UINT32 Ukn2;
	UINT8 Type[4];
	UINT32 Ukn4;
	UINT32 Ukn5;
	UINT8 Data[128];
} PublicKey_t;
//
typedef struct {
	AcpiHeader_t Header;
	PublicKey_t PublicKey;
	Marker_t Marker;
} SlicTbl_t;
#pragma pack()
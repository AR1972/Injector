#include <Windows.h>
//
typedef int (WINAPI *SLOpen)(HANDLE Ukn);
typedef int (WINAPI *SLClose)(HANDLE Ukn);
typedef int (WINAPI *SLInstallLicense)(HANDLE Ukn, ULONG CertLength, UCHAR* Cert, GUID* pPkeyId);
typedef int (WINAPI *SLInstallProofOfPurchase)(HANDLE Ukn, wchar_t* PKeyType, wchar_t* PKeyIn, ULONG PKeyDataSize, wchar_t* PKeyData, GUID* PKeyId);
typedef int (WINAPI *SLFireEvent)(HANDLE Ukn, wchar_t* Event, GUID* Slid);
wchar_t* PKeyType = L"msft:rm/algorithm/pkey/2005";
wchar_t* SlEvent = L"msft:rm/event/licensingstatechanged\0";
char* eloadresource = "load resource failed";
#define WINDOWS_SLID {0x55c92734, 0xd682, 0x4d71, 0x98, 0x3e, 0xd6, 0xec, 0x3f, 0x16, 0x05, 0x9f};
char* success = "success";
char* fail = "fail";
//
unsigned int strlen(const char *f)
{
	INT i=0;
	while(*f++) i++;
	return i;
}
//
void printf(char * fmtstr, ...)
{
	DWORD dwRet;
	CHAR buffer[256];
	va_list v1;
	va_start(v1,fmtstr);
	wvsprintf(buffer,fmtstr,v1);
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buffer, strlen(buffer), &dwRet, 0);
	va_end(v1);
}
//
BOOL GetKey(wchar_t** ProductKey)
{
	DWORD dwVersion = NULL;
	DWORD dwPInfo = NULL;
	DWORD dwMajorVersion = NULL;
	DWORD dwMinorVersion = NULL;
	dwVersion = GetVersion();
	dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
	GetProductInfo(dwMajorVersion, dwMinorVersion, 0, 0, &dwPInfo);
	switch(dwPInfo) {
	case PRODUCT_ULTIMATE:
		*ProductKey = L"2Y4WT-DHTBF-Q6MMK-KYK6X-VKM6G";
		break;
	case PRODUCT_HOME_BASIC:
		*ProductKey = L"89G97-VYHYT-Y6G8H-PJXV6-77GQM";
		break;
	case PRODUCT_HOME_PREMIUM:
		*ProductKey = L"2QDBX-9T8HR-2QWT6-HCQXJ-9YQTR";
		break;
	case PRODUCT_PROFESSIONAL:
		*ProductKey = L"2WCJK-R8B4Y-CWRF2-TRJKB-PV9HW";
		break;
	case PRODUCT_STARTER:
		*ProductKey = L"6K6WB-X73TD-KG794-FJYHG-YCJVG";
		break;
	case PRODUCT_STANDARD_SERVER:
	case PRODUCT_STANDARD_SERVER_CORE:
	case PRODUCT_STANDARD_SERVER_CORE_V:
	case PRODUCT_STANDARD_SERVER_V:
		*ProductKey = L"D7TCH-6P8JP-KRG4P-VJKYY-P9GFF";
		break;
	default:
		*ProductKey = NULL;
		return FALSE;
	}
	return TRUE;
}
//
int Supported(VOID)
{
	DWORD dwPInfo = NULL;
	DWORD dwVersion = NULL;
	DWORD dwMajorVersion = NULL;
	DWORD dwMinorVersion = NULL;
	dwVersion = GetVersion();
	dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
	if(dwMajorVersion != 6 || dwMinorVersion != 1) {
		return 1;
	}
	GetProductInfo(dwMajorVersion, dwMinorVersion, 0, 0, &dwPInfo);
	switch(dwPInfo) {
	case PRODUCT_ULTIMATE:
	case PRODUCT_HOME_BASIC:
	case PRODUCT_HOME_PREMIUM:
	case PRODUCT_PROFESSIONAL:
	case PRODUCT_STARTER:
	case PRODUCT_STANDARD_SERVER:
	case PRODUCT_STANDARD_SERVER_CORE:
	case PRODUCT_STANDARD_SERVER_CORE_V:
	case PRODUCT_STANDARD_SERVER_V:
		break;
	default:
		return 2;
	}
	return 0;
}
//
int main()
{
	ULONG Status = 0;
	SLOpen pSLOpen = NULL;
	SLClose pSLClose = NULL;
	SLInstallLicense pSLInstallLicense = NULL;
	SLInstallProofOfPurchase pSLInstallProofOfPurchase = NULL;
	SLFireEvent pSLFireEvent = NULL;
	HMODULE hSlc = NULL;
	HANDLE hSl = NULL;
	HRSRC hResource = NULL;
	HGLOBAL hgResource = NULL;
	ULONG CertSize = NULL;
	GUID pKeyId = GUID_NULL;
	GUID WindowsSlid = WINDOWS_SLID;
	UCHAR* Cert = NULL;
	wchar_t* PKeyIn;
	switch(Supported()) {
	case 0:
		break;
	case 1:
		printf("unsupported OS");
		return 1;
	case 2:
		printf("unsupported product");
		return 1;
	default:
		return 1;
	}
	hResource = FindResource(NULL, MAKEINTRESOURCE(101), "RAW");
	if(hResource) {
		CertSize = SizeofResource(NULL, hResource);
		hgResource = LoadResource(NULL, hResource);
	}
	else {
		printf("%s", eloadresource);
		return 1;
	}
	if(hgResource) {
		Cert = (UCHAR*)LockResource(hgResource);
	}
	else {
		printf("%s", eloadresource);
		return 1;
	}
	hSlc = LoadLibrary("slc.dll");
	if (hSlc) {
		// get pointers
		pSLOpen = (SLOpen) GetProcAddress(hSlc, "SLOpen");
		pSLClose = (SLClose) GetProcAddress(hSlc, "SLClose");
		pSLInstallLicense = (SLInstallLicense) GetProcAddress(hSlc, "SLInstallLicense");
		pSLInstallProofOfPurchase = (SLInstallProofOfPurchase) GetProcAddress(hSlc, "SLInstallProofOfPurchase");
		pSLFireEvent = (SLFireEvent) GetProcAddress(hSlc, "SLFireEvent");
		//
		if(pSLOpen(&hSl) == ERROR_SUCCESS) {
			printf("installing certificate: ");
			if(pSLInstallLicense(hSl, CertSize, Cert, &pKeyId) == ERROR_SUCCESS) {
				printf("%s\n", success);
			}
			else {
				printf("%s\n", fail);
				Status++;
			}
			if(GetKey(&PKeyIn)) {
				printf("installing key: ");
				if(pSLInstallProofOfPurchase(hSl, PKeyType, PKeyIn, 0, 0 , &pKeyId) == ERROR_SUCCESS) {
					printf("%s\n", success);
				}
				else {
					printf("%s\n", fail);
					Status++;
				}
			}
			else {
				printf("couldn't find product key");
				Status++;
			}
			Status = pSLFireEvent(hSl, SlEvent, &WindowsSlid);
			pSLOpen = NULL;
			pSLInstallLicense = NULL;
			pSLInstallProofOfPurchase = NULL;
			pSLFireEvent = NULL;
			Status = pSLClose(hSl);
			pSLClose = NULL;
			hSl = NULL;
		}
		FreeLibrary(hSlc);
		hSlc = NULL;
	}
	else {
		printf("failed to load slc.dll");
		Status = 1;
	}
	ExitProcess(Status);
}
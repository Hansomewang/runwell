#ifndef UTILS_GBUTF8_INCLUDED
#define UTILS_GBUTF8_INCLUDED

// UTF8 and UniCode conversion process one code only, they are intermidiate conversion 
// UTF8 -> UniCode -> GBK
// GBK -> UniCode -> UTF8

int LoadCodeTable();		// load GBK, UniCode table. 
void UnloadCodeTable();	// unload code table and release memory

// convert UTF-8 <--> UniCode (one wchar_t only)
int UnicodeToUTF8(wchar_t Unicode, char* pUTF8);
int UTF8ToUnicode(const char* pUTF_8, wchar_t* pUnicode);

// GBK code to Unicode conversion (one wchar_t only)
wchar_t GBKtoUNICODE(const char *GBKCode);
wchar_t UNICODEtoGBK(wchar_t unicode);

// following two functions all return number of bytes on output buffer (utf8 or gb)
// which are all null terminated.
// caller is responsible to ensure enough space in output buffers.

int GBKToUTF8(const char* pmbcGBK, int nGBKLen, char *utf8);
int UTF8ToGBK(const char* pUTF8,int nutf8Len, char *pmbcGB);

// GBK and UniCode string convert
// Used in File Name created in FATFS which should be in Unicode
// strgbk2uc: return number of wchar_t (not include null terminate).
//            if uc is null, function calcuate number of wchar_t takes when convert.
int strgbktouc(const char *gbk, wchar_t *uc);
int structogbk(wchar_t *uc, int nwchar, char *gbk);

#endif

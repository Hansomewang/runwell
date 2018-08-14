#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dbg_printf.h"

#define GBKTOUC_TBL		"_gbk2uc.dat"
#define UCTOGBK_TBL		"_uc2gbk.dat"

#define GBK1_BEGIN		0xa1			// lower byte is 0xA1 ~ 0xFE
#define GBK1_END			0xa9
#define GBK1_LBB			0xA1
#define GBK1_LBE			0xFE

#define GBK2_BEGIN		0xb0			// lower byte is 0xA1 ~ 0xFE
#define GBK2_END			0xf7
#define GBK2_LBB			0xA1	
#define GBK2_LBE			0xFE

#define GBK3_BEGIN		0x81			// lower byte is 0x40 ~ 0xFE
#define GBK3_END			0xa0
#define GBK3_LBB			0x40
#define GBK3_LBE			0xFE

#define GBK4_BEGIN		0xaa			// lower byte is 0x40 ~ 0xA0
#define GBK4_END			0xfe
#define GBK4_LBB			0x40
#define GBK4_LBE			0xA0

static int CharInSection[] = {
		0x34e,		// GBK1  (0xa9-0xa1_+1)*(0xfe-0xa1+1)
		0x1a70,		// GBK2 (GB2312) (0xf7-0xb0+1)*(0xfe-0xa1+1)
		0x17e0,		// GBK3 (0xa0-0x81+1)*(0xfe-0x40+1)
		0x2035,		// GBK4 (0xfe-0xaa+1)*(0xa0-0x40+1)
	}; 
static int nPageSize[] = {			// number of characters in a code page
		0x5e,
		0x5e,
		0x20,
		0x61,
	};

static int nPageRange[][2] = {
		{GBK1_BEGIN, GBK1_END}, // {0xa1, 0xa9},
		{GBK2_BEGIN, GBK2_END},	// {0xb0, 0xf7},
		{GBK3_BEGIN, GBK3_END}, // {0x81, 0xa0},
		{GBK4_BEGIN, GBK4_END}, // {0xaa, 0xf0},
	};
static int nCodeRange[][2] = {
		{GBK1_LBB, GBK1_LBE},
		{GBK2_LBB, GBK2_LBE},
		{GBK3_LBB, GBK3_LBE},
		{GBK4_LBB, GBK4_LBE},
	};
	

typedef struct {
	unsigned short gbk;
	unsigned short uc;
} GBKUCPair;

int CodeCompairFxc( const void *entry1, const void *entry2 )
{
	GBKUCPair *code1 = (GBKUCPair *)entry1;
	GBKUCPair *code2 = (GBKUCPair *)entry2;
	return (int)(code1->uc - code2->uc);
}

static GBKUCPair *CodePair=NULL;		// UniCode to GBK array
static unsigned short	 *gbkArray=NULL;		// GBK to UniCode array
int nGBK2UC=0;		// GBK to UC array entries
int nUC2GBK=0;		// UC to GBK array entry

// code1 is high byte (page code), code2 is low bytes (code in page)
static int get_gbk_index( unsigned char code1, unsigned char code2 )
{
	int i, sect, index=-1;	
	// find code section
	for(sect=0; sect<4; sect++)
	{
		if ( nPageRange[sect][0] <= code1 && code1 <= nPageRange[sect][1] &&
			   nCodeRange[sect][0] <= code2 && code2 <= nCodeRange[sect][1] )
		{
			index = 0;
			for(i=0; i<sect; i++ )
				index += CharInSection[i];
			index += (code1-nPageRange[sect][0])*nPageSize[sect] + code2 - nCodeRange[sect][0];
			break;
		}
	}
	return (index < nGBK2UC) ? index : -1;
}

int LoadCodeTable()
{
	int fd;
	struct stat st;
	
	if ( (fd = open(GBKTOUC_TBL, O_RDONLY))==-1 )
		PRINTF("GBK to UniCode code table "GBKTOUC_TBL" not found.\n");
	else
	{
		if ( fstat(fd,&st)==0 )
		{
			nGBK2UC = st.st_size / 2;
			gbkArray = (unsigned short *)malloc(	nGBK2UC * 2 );
			read( fd, gbkArray, nGBK2UC * 2 );
			PRINTF1("GBK to UniCode code table loaded. code entries=%d\n", nGBK2UC ); 
		}
		close(fd);
	}
	if ( (fd = open(UCTOGBK_TBL, O_RDONLY))==-1 )
		PRINTF("UniCode to GBK code table "UCTOGBK_TBL" not found.\n");
	else
	{
		if ( fstat(fd,&st)==0 )
		{
			nUC2GBK = st.st_size / sizeof(GBKUCPair);
			CodePair = (GBKUCPair *)malloc(	nUC2GBK * sizeof(GBKUCPair) );
			read( fd, CodePair, st.st_size );
			PRINTF1("UniCode to GBK code table loaded. code entries=%d\n", nUC2GBK ); 
		}
		close(fd);
	}	
	return nUC2GBK+nGBK2UC==0 ? -1 : 0;
}

void UnloadCodeTable()
{
	if ( gbkArray )
		free(gbkArray);
	nGBK2UC = 0;
	if ( CodePair )
		free( CodePair );
	nUC2GBK = 0;
}
 

// 返回值 1，2，3, 4 就是UTF8的编码字节数
int UnicodeToUTF8(wchar_t UniCode, char* pUTF8)  
{  
	if ( UniCode <= 0x007f )
	{
		pUTF8[0] = UniCode & 0x00ff;
		return 1;
	}
	else if ( 0x80 <= UniCode && UniCode <= 0x07ff )
	{
		pUTF8[0] = (char)(((UniCode & 0x07c0) >> 6) | 0x00c0);		// 110xxxxx 
		pUTF8[1] = (char)((UniCode & 0x003f) | 0x0080);						// 10xxxxxx
		return 2;
	}
	else if ( 0x800 <= UniCode && UniCode <= 0xffff )
	{
    pUTF8[0] = (char)(((UniCode & 0xf000) >> 12) | 0x00e0);		// 1110xxxx
    pUTF8[1] = (char)(((UniCode & 0x0fc0) >> 6) | 0x0080); 		// 10xxxxxx
    pUTF8[2] = (char)((UniCode & 0x003f) | 0x0080);  					// 10xxxxxx
    return 3;
  }
/*  只有 UCS-4 编码会有4字节的UniCode，目前只会有UCS-2
  else if ( 0x10000 <= UniCode && UniCode <= 0x10ffff )
  {
    pUTF8[0] = (char)(((UniCode & 0x1c0000) >> 18) | 0x00f0);
    pUTF8[1] = (char)(((UniCode & 0x03f000) >> 12) | 0x0080); 
    pUTF8[2] = (char)(((UniCode & 0x000fc0) >> 6) | 0x0080);  
    pUTF8[3] = (char)((UniCode & 0x00003f) | 0x0080);  
    return 4;
  }
  */
  return 0;
}  
  
int UTF8ToUnicode(const char* pUTF_8, wchar_t* pUnicode)  
{  
    if ( (pUTF_8[0] & 0x80)==0 )
    {
    	*pUnicode = (wchar_t)pUTF_8[0];
    	return 1;
    } 
    else if ( (pUTF_8[0] & 0xe0)==0xc0 && (pUTF_8[1] & 0xc0)==0x80)		// 110xxxxx 10xxxxxx - 2 byte codeing (0x0080 ~ 0x07ff)
    {
    	*pUnicode = (((wchar_t)(pUTF_8[0] & 0x1f)) << 6) | 
    							((wchar_t)(pUTF_8[1] & 0x3f));
    	return 2;
    }
    else if ( (pUTF_8[0] & 0xf0)==0xe0 && (pUTF_8[1] & 0xc0)==0x80 && (pUTF_8[2] & 0xc0)==0x80 )		// 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx  - 3 byte codeing (0x0800 ~ 0xffff)
    {
    	*pUnicode = (((wchar_t)(pUTF_8[0] & 0x0f)) << 12) | 
    							(((wchar_t)(pUTF_8[1] & 0x3f)) << 6) |
    							((wchar_t)(pUTF_8[2] & 0x3f));
    	return 3;
    }
/*    
    else if ( pUTF_8[0] & 0xf1)==0xf0 )			// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  - 4 byte codeing
    {
    	*pUnicode = (((wchar_t)(pUTF_8[0] & 0x07)) << 18) | 
    							(((wchar_t)(pUTF_8[1] & 0x3f)) << 12) |
    							(((wchar_t)(pUTF_8[2] & 0x3f)) << 6) |
    							((wchar_t)(pUTF_8[3] & 0x3f));
    }
   */
   return 0;
}
  
// 单个GBK字符编码转换为UniCode编码
wchar_t GBKtoUNICODE(const char *GBKCode)  
{
	int index;
	index = get_gbk_index( GBKCode[0], GBKCode[1] );
	if ( index != -1 )
		return gbkArray[index];
	return 0; 	// Unicode space
}  
  
//  单个UniCode字符转换为GBK字符编码
wchar_t UNICODEtoGBK(wchar_t unicode)  
{  
#if 1
	GBKUCPair key, *code_found;
	
	if ( nUC2GBK > 0 )
	{
		key.uc = unicode;
		key.gbk = 0;		// irrelavent
		code_found = bsearch(&key, CodePair, nUC2GBK, sizeof(GBKUCPair), CodeCompairFxc );
		if ( code_found != NULL )
			return code_found->gbk;
	}
#else
	int i;
	for(i=0; i<nUC2GBK; i++)
		if ( unicode==CodePair[i].uc )
			return CodePair[i].gbk;
#endif	
	return 0;		// two ASCII space
}  

// GBK 字串 转换为 UTF8 字串
//
int GBKToUTF8(const char* pmbcGB, int nGBLen, char *utf8)
{  
    int i = 0, j = 0;  
    while(i < nGBLen)  
    {
      if( (pmbcGB[i] & 0x80)==0 )  	// ASCII
      {  
        utf8[j++] = pmbcGB[i++];  
      }  
      else  // Hanzi
      {  
        wchar_t wcsUniCode;  
				int k;
				if ( (wcsUniCode = GBKtoUNICODE(pmbcGB+i)) != 0 )
				{
	        k = UnicodeToUTF8(wcsUniCode, utf8+j);
	        PRINTF4("GB code=%02X %02X, UniCode=%d, UTF8 (%d bytes)=%02X %02X %02X\n",
	        		pmbcGB[i], pmbcGB[i+1], wcsUniCode, k, utf8[j], utf8[j+1], utf8[j+2] );
	        j += k;  
	        i += 2;
	      }
	      else
	      {
	      	PRINTF4("GBKToUTF8 - invalid GBK code %02x %02x.\n", pmbcGB[i], pmbcGB[i+1] );
	      	i += 2;
	      	utf8[j++] = '?';
	      	utf8[j++] = '?';
	      }
      }  
  	}  
  	utf8[j] = '\0';
  	return j;  
}  
  
// UTF8 字串 转换为 GBK 字串  
//
int UTF8ToGBK(const char* pUTF8,int nutf8Len, char *pmbcGB)  
{  
    int i = 0, j=0;  
    while(i < nutf8Len)  
    {  
      if( (pUTF8[i] & 0x80)==0 )  
      {  
          pmbcGB[j++] = pUTF8[i++];  
      }  
      else  
      {  
        wchar_t wcsUniCode,gbcode;
        int k;  
        if ( (k=UTF8ToUnicode(pUTF8+i, &wcsUniCode)) > 0)
        {
        	if ( (gbcode = UNICODEtoGBK(wcsUniCode)) != 0 )
        	{
	        	pmbcGB[j] = gbcode >> 8;
	        	pmbcGB[j+1] = gbcode & 0xff;
					}
					else
					{
	        	pmbcGB[j] = '?';
	        	pmbcGB[j+1] = '?';
	        	PRINTF3("Unicode to GBK Convert error: UTF8=%02x %02x %02x, UC=%04x, GBK code=%04X(%02x %02x)\n",
	        			pUTF8[i], pUTF8[i+1], pUTF8[i+2], 
	        			wcsUniCode, gbcode, 
	        			pmbcGB[j], pmbcGB[j+1] );
					}        	
        	i += k;  
        	j += 2;	
        }
        else
        {
        	PRINTF3("invalid UTF-8 code: 0x%02x 0x%02x 0x%02x.\n", pUTF8[i], pUTF8[i+1],pUTF8[i+2] );
        	// skip one utf-8 byte
        	i++;
        }
      }  
	  }  
	  pmbcGB[j] = '\0';
	  return j;  
}  

int strgbktouc(const char *gbk, wchar_t *uc)
{
	int nwchar=0;
	if ( gbk )
	{
		while( *gbk )
		{
			if ( *gbk & 0x80 )
			{
				if ( uc )
					*(uc++) = GBKtoUNICODE(gbk);
				gbk += 2;
			}
			else	// ASCII
			{
				if ( uc )
					*(uc++) = (wchar_t)*gbk;
				gbk++;
			}
			nwchar++;
		}
	}
	*uc = 0;
	return nwchar;
}

int structogbk(wchar_t *uc, int nwchar, char *gbk)
{
	int i;
	char *gbk0 = gbk;
	
	for(i=0; i<nwchar; i++ )
	{
		if ( uc[i] < 0x80 )
			*(gbk++) = (char)uc[i];
		else
		{
			wchar_t gbkcode = UNICODEtoGBK(uc[i]);
			if ( gbkcode != 0 )
			{
				*(gbk++) = (char)(gbkcode >> 8) & 0xff;
				*(gbk++) = (char)(gbkcode & 0xff);
			}
			else
			{
				*(gbk++) = '?';
				*(gbk++) = '?';
			}
		}
	}
	*gbk = '\0';
	return gbk - gbk0;
}

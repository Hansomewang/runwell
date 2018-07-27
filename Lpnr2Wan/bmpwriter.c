/*
 *   This souce file is used to create Microsoft BMP image file. It support bpp 1, 4, 8, 16, 24 and 32, however, data compression is not implemented.
 *   Author: Thomas Chang
 *  Date: 2012-12-10
 * 
 *  Note:
 *  	1. Ideally, BMP output shall be designed as STREAM like a 'FILE *' in C. But we need output to a buffer, so I designed it this way (There is no
 *	    FILE stream in DSP). However, it is very easy to be modified to suit your need.
 *      2. Data in BMP header is stored little-endian sequence. This code assume that CPU is also little-endian architrcture (Intel, TI DSP are all little-endian)
 *	    If your target machine in big-endian, you have to handle this issue. I could have put some compiler directives and macros to handle both cases,
 *	    finally I just let it be.
 *
 *  to compile with a test code. use:
 *  > gcc -DENABLE_TESTBMP bmpwriter.c -obmpwrite
 * then exectute it
 * > ./bmpwriter >test1.bmp
 * or with arguments
 * >./bmpwriter -w94 -h32 -otest2.bmp
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bmpwriter.h"
#ifndef linux
#include <io.h>
#endif

//#define _CRT_SECURE_NO_WARNINGS 		// disable the annoying warning 4996

typedef short		Int16;
typedef  int			Int32;
typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned int	Uint32;

#define MAX_WIDTH	1920

#ifdef linux
#define O_BINARY		0
#define _S_IREAD		0444
#define _S_IWRITE		0200
#endif

typedef struct _bmp_header {
	char		file_header[2];		// 'BM'
	Uint32	file_size;			// size in bytes from begin of file
	Uint32	reserved;			// must be 0
	Uint32	data_offset;		// bitmap image data offset relative to begin of file		
	Uint32	header_size;		// size of header (include color palette)
	Uint32	img_width;		// number of pixels
	Uint32	img_height;		// number of pixels
	Uint16	plane;			// must be 1 
	Uint16	bpp;				// bits per pixels (1, 4, 8, 16, 24, 32)
	Uint32	compress;		// 0: not compressed, 1: RLE-8, 2: RLE-4, 3: bit-files
	Uint32	data_size;		// bitmap data size (in bytes)
	Uint32	ppm_horiz;		// horizontal resolution (pixels-per-meter)
	Uint32	ppm_vert;		// vertical resolution
	Uint32	num_colors;		// number of colors used (2 for bpp=1, 16 for bpp=4, 256 for bpp=8,...)
	Uint32	imp_colors;		// number of important colors (can be equal to num_colors which means all colors are important)
} BMPHeader;

int BMP_FileSize(int w, int h, int bpp)
{
	int row_size = (bpp * w + 31) / 32 * 4;		// BMP row size is always round up to a number of multiple of 4 bytes (32 bits)
	int file_size = 54 + row_size * h;
	// for bpp less than 8, we need to write color pallete
	if ( bpp <= 8 )
		file_size += 4 * (1 << bpp);
	return file_size;
}

BMPImage *BMP_Create( int width, int height, int bpp )
{
	BMPImage *imgBMP = (BMPImage *)malloc( sizeof(BMPImage) );
	imgBMP->bpp = bpp;
	imgBMP->width = width;
	imgBMP->height = height;
	imgBMP->num_palette = (bpp <= 8) ? (1 << bpp) : 0;
	if ( imgBMP->num_palette > 0 )
		imgBMP->palette = (Palette *)malloc( sizeof(Palette) * imgBMP->num_palette );
	else
		imgBMP->palette = NULL;
	return imgBMP;
}

#define MOVEINT32(s,n) \
	*(s++) = (Uint8)(n & 0x000000ff); \
	*(s++) = (Uint8)( (n & 0x0000ff00) >> 8); \
	*(s++) = (Uint8)( (n & 0x00ff0000) >> 16); \
	*(s++) = (Uint8)( (n & 0xff000000) >> 24); 

#define MOVEINT16(s,n) \
	*(s++) = (Uint8)(n & 0x00ff); \
	*(s++) = (Uint8)( (n & 0xff00) >> 8); 
	
void BMP_fillHeader(BMPHeader *hdr, int imgWidth, int imgHeight, int bpp )
{
	hdr->file_header[0] = 'B';
	hdr->file_header[1] = 'M';
	hdr->header_size = 0x28; // sizeof( BMPHeader ) + (1 << bpp) * sizeof(Palette);
	hdr->data_size = (bpp * imgWidth * imgHeight) / 8;
	hdr->data_offset = 0x36;
	if ( bpp <= 8 )		// bpp > 8 is direct color w/o color palette.
		hdr->data_offset +=  4 * (1 << bpp);
	hdr->file_size =  BMP_FileSize( imgWidth, imgHeight, bpp );
	hdr->reserved = 0;
	hdr->img_width = imgWidth;
	hdr->img_height = imgHeight;
	hdr->plane = 1;
	hdr->bpp = bpp;
	hdr->compress = 0;						// no compressed BMP.
	hdr->ppm_horiz = hdr->ppm_vert = 0; // 11811;		// this is 300 dpi
	hdr->num_colors = hdr->imp_colors = (bpp<=8 ) ? 1 << bpp : 0;
}
	
char *BMP_moveHeader( char *stream, int imgWidth, int imgHeight, int bpp )
{
	BMPHeader hdr;
	
	BMP_fillHeader( &hdr, imgWidth, imgHeight, bpp );
	
	// do not use memcpy( stream, &hdr, sizeof(hdr) ) as structure member is not compact. Compiler will generate padding bytes to make every entry
	// start at 32-bits boundary
	*(stream++) = hdr.file_header[0];
	*(stream++) = hdr.file_header[1];
	MOVEINT32( stream, hdr.file_size );
	MOVEINT32( stream, hdr.reserved );
	MOVEINT32( stream, hdr.data_offset );
	MOVEINT32( stream, hdr.header_size );
	MOVEINT32( stream, hdr.img_width );
	MOVEINT32( stream, hdr.img_height );
	MOVEINT16( stream, hdr.plane );
	MOVEINT16( stream, hdr.bpp );
	MOVEINT32( stream, hdr.compress );
	MOVEINT32( stream, hdr.data_size );
	MOVEINT32( stream, hdr.ppm_horiz );
	MOVEINT32( stream, hdr.ppm_vert );
	MOVEINT32( stream, hdr.num_colors );
	MOVEINT32( stream, hdr.imp_colors );
	
/*	
	*((Uint32 *)(stream)) = hdr.file_size;		stream += 4;
	*((Uint32 *)(stream)) = hdr.reserved;		stream += 4;
	*((Uint32 *)(stream)) = hdr.data_offset;		stream += 4;
	*((Uint32 *)(stream)) = hdr.header_size;		stream += 4;
	*((Uint32 *)(stream)) = hdr.img_width;		stream += 4;
	*((Uint32 *)(stream)) = hdr.img_height;		stream += 4;
	*((Uint16 *)(stream)) = hdr.plane;			stream += 2;
	*((Uint16 *)(stream)) = hdr.bpp;			stream += 2;
	*((Uint32 *)(stream)) = hdr.compress;		stream += 4;
	*((Uint32 *)(stream)) = hdr.data_size;		stream += 4;
	*((Uint32 *)(stream)) = hdr.ppm_horiz;		stream += 4;
	*((Uint32 *)(stream)) = hdr.ppm_vert;		stream += 4;
	*((Uint32 *)(stream)) = hdr.num_colors;		stream += 4;
	*((Uint32 *)(stream)) = hdr.imp_colors;		stream += 4;
*/
	return stream;
}

int BMP_writeHeader( int fd, int imgWidth, int imgHeight, int bpp )
{
	BMPHeader hdr;
	
	BMP_fillHeader( &hdr, imgWidth, imgHeight, bpp );
	
	// do not use memcpy( stream, &hdr, sizeof(hdr) ) as structure member is not compact. Compiler will generate padding bytes to make every entry
	// start at 32-bits boundary
	write( fd, hdr.file_header, 2);
	write( fd, &hdr.file_size, 4 );
	write( fd, &hdr.reserved, 4 );
	write( fd, &hdr.data_offset, 4 );
	write( fd, &hdr.header_size, 4 );
	write( fd, &hdr.img_width, 4 );
	write( fd, &hdr.img_height, 4 );
	write( fd, &hdr.plane, 2 );
	write( fd, &hdr.bpp, 2 );
	write( fd, &hdr.compress, 4 );
	write( fd, &hdr.data_size, 4 );
	write( fd, &hdr.ppm_horiz, 4 );
	write( fd, &hdr.ppm_vert, 4 );
	write( fd, &hdr.num_colors, 4 );
	write( fd, &hdr.imp_colors, 4 );
	return 0;
}

char *BMP_movePalette( char *stream, int ncolor, Palette *palette )
{
	int	i;
	for( i=0; i<ncolor; i++ )
	{
		MOVEINT32( stream, palette[i].bgr );
	}
	return stream;
}

int BMP_writePalette( int fd, int ncolor, Palette *palette )
{
	int	i;
	for( i=0; i<ncolor; i++ )
	{
		write( fd, &palette[i].bgr, 4 );
	}
	return 0;
}

int BMP_writeGrayPalette( int fd )
{
	unsigned int  palette[256];
	int i;
	
	palette[0] = 0;
	for( i=1; i<256; i++ )
		palette[i] = palette[i-1] + 0x00010101;
	return write( fd, palette, sizeof(palette) );
}

int BMP_writeBinaryPalette( int fd )
{
	unsigned int palette[2] = { 0,  0x00ffffff };
	return write( fd, palette, sizeof(palette) );
}

char *BMP_moveImageLine( char *stream, char *image, int width, int bpp, int swapRB )
{
	char *line_start = stream;
	int depth = bpp >= 8 ? bpp / 8 : 1;
	int i, left;
		
	if ( bpp>=24 && swapRB )
	{
		for ( ; width--; )
		{
			*(stream++) = image[2];
			*(stream++) = image[1];
			*(stream++) = image[0];
			if ( bpp==32 )
				*(stream++) = image[3];
			image += depth;
		}
	}
	else if ( bpp >= 8 )	// gray scale, BGR565, RGB24, RGB32
	{
		memcpy( stream, image, depth * width );
		stream += depth * width;
	}
	else if ( bpp == 4 )
	{
		width >>= 1;
		for( ; width-- ;  image += 2 )
			*(stream++) = (image[0] & 0x0f) + ((image[1] & 0x0f) << 4);
	}
	else if ( bpp== 1 )
	{
		Uint8 outByte;
		
		left = width % 8;
		width >>= 3;
		for( ; width--;  )
		{
			outByte = 0;
			for( i=0; i<8; i++ )
			{
				outByte <<= 1;
				outByte |= *(image++) != 0;
			}
			*(stream++) = outByte;
		}
		if ( left )
		{
			outByte = 0;
			for( i=0; i<left; i++ )
			{
				outByte <<= 1;
				outByte |= *(image++) != 0;
			}
			outByte <<= (8-left);
			*(stream++) = outByte;
		}
	}
	// check do we need to pad bytes on end of data row. (BMP image line must be roundup to a multiple of 4 bytes number
	left = (stream - line_start) % 4;
	if ( left )
	{
		left = 4 - left;
		for( ; left--; )  
			*(stream++) = 0;
	}
	return stream;
}
int BMP_writeImageLine( int fd, char *image, int width, int bpp, int swapRB )
{
	off_t  line_start = lseek( fd, 0, SEEK_CUR );
	int depth = bpp >= 8 ? bpp / 8 : 1;
	int i, left;
		
	if ( bpp>=24 && swapRB )
	{
		char lineBuf[MAX_WIDTH*4];
		char *ptr = lineBuf;
		int   w = width;
		for ( ; w--; )
		{
			*(ptr++) = image[2];
			*(ptr++) = image[1];
			*(ptr++) = image[0];
			if ( bpp==32 )
				*(ptr++) = image[3];
			image += depth;
		}
		write( fd, lineBuf, depth * width );
	}
	else if ( bpp >= 8 )	// gray scale, BGR565, RGB24, RGB32
	{
		write( fd, image, depth * width );
	}
	else if ( bpp == 4 )
	{
		short dualpix;
		width >>= 1;
		for( ; width-- ;  image += 2 )
		{
			dualpix = (image[0] & 0x0f) + ((image[1] & 0x0f) << 4);
			write( fd, &dualpix, 2 );
		}
	}
	else if ( bpp== 1 )
	{
		char lineBuf[MAX_WIDTH/8];
		int  w;
		Uint8 outByte;
		left = width % 8;
		for( w=0; w<width/8; w++  )
		{
			outByte = 0;
			for( i=0; i<8; i++ )
			{
				outByte <<= 1;
				outByte |= *(image++) != 0;
			}
			lineBuf[w] = outByte;
		}
		// last byte
		if ( left )
		{
			outByte = 0;
			for( i=0; i<left; i++ )
			{
				outByte <<= 1;
				outByte |= *(image++) != 0;
			}
			outByte <<= (8-left);
			lineBuf[w++] = outByte;
		}
		write( fd, lineBuf, w ); 
	}
	// check do we need to pad bytes on end of data row. (BMP image line must be roundup to a multiple of 4 bytes number)
	left = (lseek( fd, 0, SEEK_CUR ) - line_start) % 4;
	if ( left )
	{
		Uint8 pad[4] = {0,0,0,0};
		left = 4 - left;
		write( fd, pad, left );
	}
	return 0;
}
/*
 * Note: 
 * char *image is the start of image data
 * for bpp == 1, 4, 8, one byte is one pixel value in source image
 * for bpp==16 two bytes for one pixel value
 * for bpp==24 3 bytes for one pixel value
 * for bpp==32 4 bytes for one pixel value
 * linestride is offset to address begining of next image line (source image)
 * 
 */
char *BMP_moveImageFrame( char *stream, char *image, int width, int height, int bpp, int linestride, int swapRB )
{
	int  depth = bpp >= 8 ? bpp / 8 : 1;
//	int  line;
	char *lineptr;
	
	if ( linestride == 0 ) linestride = width * depth;
	if ( linestride < width*depth )  return stream;

	// BMP image data is stored in up-side-down sequence which mean. first row stored on end of file.
	lineptr = image + height*linestride;
	for( ; height--;   )
	{
		lineptr -= linestride;
		stream = BMP_moveImageLine( stream, lineptr, width, bpp, swapRB );
	}
	return stream;
}
int BMP_writeImageFrame( int fd, char *image, int width, int height, int bpp, int linestride, int swapRB )
{
	int  depth = bpp >= 8 ? bpp / 8 : 1;
//	int  line;
	char *lineptr;
	
	if ( linestride == 0 ) linestride = width * depth;
	if ( linestride < width*depth )  return -1;

	// BMP image data is stored in up-side-down sequence which mean. first row stored on end of file.
	lineptr = image + height*linestride;
	for( ; height--;   )
	{
		lineptr -= linestride;
		BMP_writeImageLine( fd, lineptr, width, bpp, swapRB );
	}
	return 0;
}

void BMP_planerized( BMPImage *imgBMP,  char *r_ptr, char *g_ptr, char *b_ptr )
{
	int  depth = imgBMP->bpp >= 8 ? imgBMP->bpp / 8 : 1;
	int linestride = depth * imgBMP->width;
	char *lineptr;
	int height = imgBMP->height;

	if ( depth < 3 ) return;		// only 3 or 4 can be planerized
	// BMP image data is stored in up-side-down sequence which mean. first row stored on end of file.
	lineptr = (char *)(imgBMP->imageData + imgBMP->height*linestride);
	for( ; height--;   )
	{
		int   w = imgBMP->width;
		char *image;
		lineptr -= linestride;
		 image = lineptr;
		for ( ; w--; )
		{
			*(r_ptr++) = image[2];
			*(g_ptr++) = image[1];
			*(b_ptr++) = image[0];
			image += depth;
		}
	}
}

void BMP_compose( BMPImage *imgBMP,  char *r_ptr, char *g_ptr, char *b_ptr )
{
	int  depth = imgBMP->bpp >= 8 ? imgBMP->bpp / 8 : 1;
	int linestride = depth * imgBMP->width;		// BMP raster line size must be a number of multipe of 4 bytes.
	char *lineptr;
	int height = imgBMP->height;

	if ( depth < 3 ) return;		// only 3 or 4 can be planerized
	// BMP image data is stored in up-side-down sequence which mean. first row stored on end of file.
	lineptr = (char *)(imgBMP->imageData + imgBMP->height*linestride);
	for( ; height--;   )
	{
		int   w = imgBMP->width;
		char *image;
		lineptr -= linestride;
		image  = lineptr;
		for ( ; w--; )
		{
			image[2] = *(r_ptr++);
			image[1] = *(g_ptr++) ;
			image[0] = *(b_ptr++);
			if ( depth==4 )
				image[3] = 0;
			image += depth;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    R E A D
static int bmp_ReadHeader( int fd, BMPHeader *hdr )
{
	read( fd, hdr->file_header, 2 );
	read( fd, &hdr->file_size, 4 );
	read( fd, &hdr->reserved, 4 );
	read( fd, &hdr->data_offset, 4 );
	read( fd, &hdr->header_size, 4 );
	read( fd, &hdr->img_width, 4 );
	read( fd, &hdr->img_height, 4 );
	read( fd, &hdr->plane, 2 );
	read( fd, &hdr->bpp, 2 );
	read( fd, &hdr->compress, 4 );
	read( fd, &hdr->data_size, 4 );
	read( fd, &hdr->ppm_horiz, 4 );
	read( fd, &hdr->ppm_vert, 4 );
	read( fd, &hdr->num_colors, 4 );
	read( fd, &hdr->imp_colors, 4 );
	return hdr->file_header[0]=='B' && hdr->file_header[1]=='M' && hdr->header_size==0x28;
}

static int bmp_ReadBody( int fd, BMPImage *imgBMP )
{
	unsigned char *lineBuf;
	unsigned char *ptr;
	int i, j, k, lineStep, rowStride, nPixDepth;

	// read image data
	lineStep = (( (imgBMP->bpp*imgBMP->width + 7) / 8 + 3) >> 2) << 2;	// line step in file is length of a multiplier of 4-bytes
	lineBuf = (unsigned char *)malloc( lineStep );
	nPixDepth = (imgBMP->bpp <= 8 ? 1 : imgBMP->bpp/8);
	rowStride = nPixDepth * imgBMP->width;								// row stride in image buffer (continues no padding)
	imgBMP->imageSize = rowStride * imgBMP->height;
	imgBMP->imageData = malloc( imgBMP->imageSize );
	// BMP data is stored in upside-down sequence. Top most image row at end of file.
	ptr = imgBMP->imageData + imgBMP->height * rowStride;
	for( i=0; i<imgBMP->height; i++ )
	{
		ptr -= rowStride;
		if ( (j=read( fd, lineBuf, lineStep )) != lineStep )
		{
//			const char *estr = strerror( errno );
//			long ofs = lseek( fd, 0, SEEK_CUR );		// for debugger to know the file read position
			break;
		}
		if ( imgBMP->bpp >= 8 )
			memcpy( ptr, lineBuf, rowStride );
		else if ( imgBMP->bpp == 4 )
		{
			for( j=0, k=0; j<imgBMP->width; j+=2, k++ )
			{
				ptr[j] = lineBuf[k] & 0x0f;
				ptr[j+1] = (lineBuf[k] & 0xf0) >> 4;
			}
		}
		else if ( imgBMP->bpp==1 )
		{
			int n;
			for( j=0,k=0,n=0; j<imgBMP->width; j++ )
			{
				ptr[j] = lineBuf[n] & (0x80 >> (k++) ) ? 255 : 0;
				k %= 8;
				if ( !k ) n++;
			}
		}
	}
	free( lineBuf );
	return i == imgBMP->height;
}

BMPImage* BMP_LoadFromFile( const char* strFile )
{
	BMPHeader	bmpHeader;
	BMPImage	*imgBMP;

	int fd;
	
	if ( (fd=open( strFile, O_RDONLY|O_BINARY )) == -1 )
	{
		perror( strFile );
		return NULL;
	}
	if ( !bmp_ReadHeader( fd, &bmpHeader ) )
	{
		fprintf(stderr,"BMP header error!\n");
		return NULL;
	}
	imgBMP = BMP_Create(bmpHeader.img_width, bmpHeader.img_height, bmpHeader.bpp );
	if ( imgBMP->num_palette > 0 && imgBMP->palette!=NULL )
		read( fd, imgBMP->palette, sizeof(Palette)*imgBMP->num_palette );
	if ( !bmp_ReadBody( fd, imgBMP ) )
	{
		fprintf( stderr, "BMP body read error!\n");
		BMP_destroy( &imgBMP );
	}
	close( fd );
	return imgBMP;
}

static int STREAM2INT( unsigned char *stream)
{
	int nval = 0;
	int i;
	for( i=3; i>=0; i-- )
	{
		nval <<= 8;
		nval |= stream[i];
	}
	return nval;
}

static short STREAM2SHORT( unsigned char *stream)
{
	short nval = 0;
	nval = stream[1];
	nval <<= 8;
	nval |= stream[0];
	return nval;
}

static const char *bmp_MoveHeader( unsigned char *stream, BMPHeader *hdr )
{
	hdr->file_header[0] = (char)*(stream++);
	hdr->file_header[1] = (char)*(stream++);
	hdr->file_size = (Uint32)STREAM2INT(stream);		stream += 4;
	hdr->reserved = (Uint32)STREAM2INT(stream);		stream += 4;
	hdr->data_offset = (Uint32)STREAM2INT(stream);	stream += 4;
	hdr->header_size = (Uint32)STREAM2INT(stream);	stream += 4;
	hdr->img_width = (Uint32)STREAM2INT(stream);	stream += 4;
	hdr->img_height = (Uint32)STREAM2INT(stream);		stream += 4;
	hdr->plane = (Uint16)STREAM2SHORT(stream);		stream += 2;
	hdr->bpp =  (Uint16)STREAM2SHORT(stream);		stream += 2;
	hdr->compress = (Uint32)STREAM2INT(stream);		stream += 4;
	hdr->data_size = (Uint32)STREAM2INT(stream);		stream += 4;
	hdr->ppm_horiz = (Uint32)STREAM2INT(stream);	stream += 4;
	hdr->ppm_vert = (Uint32)STREAM2INT(stream);		stream += 4;
	hdr->num_colors = (Uint32)STREAM2INT(stream);	stream += 4;
	hdr->imp_colors = (Uint32)STREAM2INT(stream);	stream += 4;
	return (hdr->file_header[0]=='B' && hdr->file_header[1]=='M' && hdr->header_size==0x28) ?
		stream : NULL;
}

static const char *bmp_MoveBody( const char *stream, BMPImage *imgBMP )
{
	unsigned char *lineBuf;
	unsigned char *ptr;
	int i, j, k, lineStep, rowStride, nPixDepth;

	// read image data
	lineStep = (( (imgBMP->bpp*imgBMP->width + 7) / 8 + 3) >> 2) << 2;	// line step in file is length of a multiplier of 4-bytes
	lineBuf = malloc( lineStep );
	nPixDepth = (imgBMP->bpp <= 8 ? 1 : imgBMP->bpp/8);
	rowStride = nPixDepth * imgBMP->width;								// row stride in image buffer (continues no padding)
	imgBMP->imageSize = rowStride * imgBMP->height;
	imgBMP->imageData = malloc( imgBMP->imageSize );
	// BMP data is stored in upside-down sequence. Top most image row at end of file.
	ptr = imgBMP->imageData + imgBMP->height * rowStride;
	for( i=0; i<imgBMP->height; i++ )
	{
		ptr -= rowStride;
		memcpy( lineBuf, stream, lineStep );
		stream += lineStep;
		if ( imgBMP->bpp >= 8 )
			memcpy( ptr, lineBuf, rowStride );
		else if ( imgBMP->bpp == 4 )
		{
			for( j=0, k=0; j<imgBMP->width; j+=2, k++ )
			{
				ptr[j] = lineBuf[k] & 0x0f;
				ptr[j+1] = (lineBuf[k] & 0xf0) >> 4;
			}
		}
		else if ( imgBMP->bpp==1 )
		{
			int n;
			for( j=0,k=0,n=0; j<imgBMP->width; j++ )
			{
				ptr[j] = lineBuf[n] & (0x80 >> (k++) ) ? 255 : 0;
				k %= 8;
				if ( !k ) n++;
			}
		}
	}
	free( lineBuf );
	return stream;
}

BMPImage* BMP_LoadFromStream( const char* stream )
{
	BMPHeader	bmpHeader;
	BMPImage	*imgBMP;

	if ( (stream = bmp_MoveHeader( (char *)stream, &bmpHeader )) == NULL )
	{
		fprintf(stderr,"BMP header error!\n");
		return NULL;
	}
	imgBMP = BMP_Create( bmpHeader.img_width, bmpHeader.img_height, bmpHeader.bpp );
	if ( imgBMP->num_palette > 0 && imgBMP->palette != NULL )
	{
		memcpy( imgBMP->palette, stream, sizeof(Palette)*imgBMP->num_palette );
		stream += sizeof(Palette)*imgBMP->num_palette;
	}
	bmp_MoveBody( stream, imgBMP );
	return imgBMP;
}

void BMP_destroy( BMPImage **imgBMP )
{
	BMPImage *bmp = *imgBMP;
	
	if ( bmp )
	{
		free( bmp->imageData );
		if ( bmp->num_palette > 0 )
			free( bmp->palette );
		free( bmp );
		*imgBMP = NULL;
	}
}

int BMP_WriteToFile( BMPImage * bmp, const char *path )
{
	int fd = open( path, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, _S_IREAD | _S_IWRITE );
	if ( fd == -1 )
		return -1;
	BMP_writeHeader( fd, bmp->width, bmp->height, bmp->bpp );
	if ( bmp->bpp <= 8 )
		BMP_writePalette(fd, bmp->num_palette, bmp->palette );
	BMP_writeImageFrame(fd, (char *)bmp->imageData, bmp->width, bmp->height, bmp->bpp, 0, 0 );
	close(fd);
	return BMP_FileSize( bmp->width, bmp->height, bmp->bpp );
}

int BMP_WriteToMem( BMPImage * bmp, char *buf )
{
	char *ptr = buf;
	ptr = BMP_moveHeader( ptr, bmp->width, bmp->height, bmp->bpp );
	if ( bmp->num_palette > 0 )
		ptr = BMP_movePalette( ptr, bmp->num_palette, bmp->palette );
	ptr = BMP_moveImageFrame( ptr, (char *)bmp->imageData, bmp->width, bmp->height, bmp->bpp, 0, 0 );
	return (int)(ptr - buf);
}

void BMP_ExportRawBinaryImage(BMPImage *imgBMP, unsigned char *binImage )
{
	unsigned char *ptrbpp1, *ptrbpp8;
	int i, j, k, bit, bpl;
	
	if ( imgBMP->bpp > 8 ) return;
	bpl = (imgBMP->width + 7) / 8;
	ptrbpp8 = imgBMP->imageData;
	ptrbpp1 = binImage;
	for( i=0; i<imgBMP->height; i++ )
	{
		for(j=0; j<imgBMP->width; j++)
		{
			k = j / 8;
			bit = j % 8;
			if ( ptrbpp8[j] != 0 )
				ptrbpp1[k] |= (1 << (7 - bit));
			else
				ptrbpp1[k] &= ~(1 << (7 - bit));
		}
		ptrbpp8 += imgBMP->width;
		ptrbpp1 += bpl;
	}
}

// this is turn the data from 280 bytes to 80*28 bytes 
// but here is not handle the palatte 
BMPImage *BMP_ImportRawBinaryImage(unsigned char *binImage, int width, int height )
{
	unsigned char *ptrbpp1, *ptrbpp8;
	int i, j, k, bit, bpl;
	BMPImage *imgBMP = malloc(sizeof(BMPImage));

	memset(imgBMP,0,sizeof(imgBMP));
	imgBMP->bpp  = 1;
	imgBMP->width = width;
	imgBMP->height = height;
	imgBMP->imageSize = width * height;
	imgBMP->imageData = malloc(imgBMP->imageSize);
//  we need to add the palette to show the bmp picture	
	imgBMP->num_palette = 2;
    Palette *palette =(Palette *) malloc(sizeof(Palette)*2);
	Palette *test = palette;
	test->bgr = 0x00000000;
	test++;
	test->bgr = 0x00ffffff;
	imgBMP->palette =palette;

	ptrbpp8 = imgBMP->imageData;
	ptrbpp1 = (unsigned char *)binImage;
	bpl = (imgBMP->width + 7) / 8;
	for( i=0; i<imgBMP->height; i++ )
	{
		for(j=0; j<imgBMP->width; j++)
		{
			k = j / 8;
			bit = j % 8;
			if ( (ptrbpp1[k] & (1 << bit)) != 0 )
				ptrbpp8[j] = 0xff;
			else
				ptrbpp8[j] = 0;
		}
		ptrbpp8 += imgBMP->width;
		ptrbpp1 += bpl;
	}

	return imgBMP;
}

BMPImage *BMP_CropRect(BMPImage *srcBMP, int x, int y, int width, int height )
{
	BMPImage *cropBMP = NULL;
	int i, nPixDepth, rowStride, srcStride;
	unsigned char *srcPtr, *dstPtr;

	if ( x<0 || x+width > srcBMP->width || y<0 || y+height > srcBMP->height )
		return NULL;
	cropBMP = (BMPImage *)malloc( sizeof(BMPImage) );
	cropBMP->width = width;
	cropBMP->height = height;
	cropBMP->bpp = srcBMP->bpp;
	cropBMP->num_palette = srcBMP->num_palette;
	if ( srcBMP->num_palette > 0 )
	{
		cropBMP->palette = (Palette *)malloc( sizeof(Palette) * srcBMP->num_palette );
		memcpy(cropBMP->palette, srcBMP->palette, sizeof(Palette) * srcBMP->num_palette );
	}
	nPixDepth = (srcBMP->bpp <= 8 ? 1 : srcBMP->bpp/8);
	rowStride = nPixDepth * width;								// row stride in image buffer (continues no padding)
	srcStride = nPixDepth * srcBMP->width;
	cropBMP->imageSize = rowStride * height;
	cropBMP->imageData = (unsigned char *)malloc( cropBMP->imageSize );
	srcPtr = srcBMP->imageData + (y*srcBMP->width + x) * nPixDepth;
	dstPtr = cropBMP->imageData;
	for(i=0; i<height; i++)
	{
		memcpy( dstPtr, srcPtr, rowStride );
		srcPtr += srcStride;
		dstPtr += rowStride;
	}
	return cropBMP;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_TESTBMP

main(int argc, char * const argv[] )
{
	int imgWidth=64, imgHeight=64;
	int lineWidth = 64;
	int bpp=1;
	char pattern = 0xcc;
	char *imageData;
	char *bmpStream;
	char *ptr;
	int nFileSize;
	Palette bw_palette[2] = { 0, 0x00ffffff };
	int i, j, nb=0;
	const char *outFile=NULL;
	int  c;

	// parse arguments
	for (;;) 
	{
		c = getopt(argc, argv, "w:h:o:");
		if (-1 == c)		// end of options
			break;
		if ( optarg == NULL )
		{
			printf("missing parameter for option %c\n", c );
			return 0;
		}
		switch (c) 	
		{
		case 'w':
			imgWidth = atoi( optarg );
			break;
		case 'h':
			imgHeight = atoi( optarg );
			break;
		case 'o':	
			outFile = optarg;
			break;
		}
	}
	lineWidth = ((imgWidth + 7) / 8) * 8;
	imageData = malloc( lineWidth * imgHeight );
	bmpStream = malloc( BMP_FileSize(imgWidth, imgHeight, bpp) );
	
	// generate image source data
	for( i=0; i<imgHeight; i++ )
	{
		ptr = imageData + i *lineWidth;
		nb = 0;
		for( j=0; j<imgWidth; j++ )
		{
			*(ptr++) = (pattern >> nb) & 0x01;
			if ( ++nb == 8 ) nb = 0;
		}
	}
	
	// generate BMP stream
	ptr = bmpStream;
	ptr = BMP_writeHeader( ptr, imgWidth, imgHeight, bpp );
	ptr = BMP_writePalette( ptr, 2, bw_palette );
	ptr = BMP_writeImageFrame( ptr, imageData, imgWidth, imgHeight, 1, lineWidth );
	nFileSize = ptr - bmpStream;
	
	if ( nFileSize != BMP_FileSize(imgWidth, imgHeight, bpp) )
	{
		fprintf(stderr, "Generated stream length incorrect. Expected %d bytes while output is %d bytes\n", 
				BMP_FileSize(imgWidth, imgHeight, bpp), nFileSize );
	}
	else
	{
		if ( outFile != NULL )
		{
			FILE *fout = fopen( outFile, "wb" );
			if ( fout == NULL )
			{
				perror( outFile );
			}
			else
			{
				fwrite( bmpStream, 1, nFileSize, fout );
				fclose( fout );
			}
		}
		else
		{
			// output to stdout
			write( 1, bmpStream, nFileSize );
		}
	}
	
	free( imageData );
	free( bmpStream );
}

#endif	// #ifdef ENABLE_TESTCODE

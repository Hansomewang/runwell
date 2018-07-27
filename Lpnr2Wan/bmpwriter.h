#ifndef _BMPWRITER_INCL
#define _BMPWRITER_INCL

typedef union _Palette 
{
	unsigned int	bgr;
	unsigned char	comp[4];			// in B G R F sequence (F is filled with 0)
} Palette;

typedef struct _bmp_image {
	int	width;								// image width
	int	height;								// image height
	int bpp;									// pixel depth of image
	int imageSize;						// size of image data in number of bytes
	int	num_palette;						// number of palettes
	Palette	*palette;						// palette of the image
	unsigned char *imageData;		// data of image (compact no padding, top to down, left to right order
												// for bpp < 8, still one byte store one pixel's value to simple the operation
} BMPImage;

#ifdef __cplusplus
extern "C"
{
#endif

extern int BMP_FileSize(int w, int h, int bpp);
extern BMPImage *BMP_Create( int width, int height, int bpp );

#define BMP_HeaderSize( bpp )	( 54 + ( bpp<=8 ? 4 * (1 << bpp) : 0) )	// include color palette
#define BMP_BodySize(w,h,bpp)	( (( (bpp) * (w) + 31) / 32 * 4) * (h) )

#define isBMPHeader( img )		( img[0]=='B' && img[1]=='M' )

/*
 *  return value of following functions are the same. Always the address of next byte to output to BMP byte stream.
 */
extern char *BMP_moveHeader( char *stream, int imgWidth, int imgHeight, int bpp );
extern char *BMP_movePalette( char *stream, int nColor, Palette *palette );
extern int BMP_writeHeader( int fd, int imgWidth, int imgHeight, int bpp );
extern int BMP_writePalette( int fd, int nColor, Palette *palette );
extern int BMP_writeGrayPalette( int fd );
extern int BMP_writeBinaryPalette( int fd );
extern int BMP_WriteToFile( BMPImage * bmp, const char *path );
extern int BMP_WriteToMem( BMPImage * bmp, char *buf );

/* Note: BMP_writeImageFrame - output entire image
 * stream is output BMP stream
 * image is the start of image data
 * width, height is resolution of image
 * for bpp == 1, 4, 8, one byte is one pixel value in source image. color palette is required
 * for bpp==16 two bytes for one pixel value (RGB16). color palette is not required,
 * for bpp==24 3 bytes for one pixel value. color palette is not required.
 * for bpp==32 4 bytes for one pixel value. color palette is not required.
 * linestride is offset to the address of next image line (source image). linestride==0 means
 *     no padding bytes on end of each row (image data are continuous). 
 *     addr. of row_n + linestride is addr. of row_n+1
 * swapRB: TRUE to swap the 1'st and 3'rd byte of each pixel (only meaningful for bpp 24 or 32)
 *
 * Special Note: we still use 1 byte to store 1 pixel for bpp 1 and 4. This is for our convenience on binary image process.
 * Maybe I will change it as standard way. But for now just let it be this way.
 */
extern char *BMP_moveImageFrame( char *stream, char *image, int width, int height, int bpp, int linestride, int swapRB );
extern int BMP_writeImageFrame( int fd, char *image, int width, int height, int bpp, int linestride, int swapRB );

/*
 * output one image row to BMP output stream. As the matter of factor, BMP_writeImageFrame invoke BMP_writeImageLine
 * line by line to output entire image frame.
 */
extern char *BMP_moveImageLine( char *stream, char *image, int width, int bpp, int swapRB );
extern int BMP_writeImageLine( int fd, char *image, int width, int bpp, int swapRB );

// load image from file
/*
 * read a image from data file or convert BMP stream to image frame
 */
extern BMPImage* BMP_LoadFromFile( const char* strFile );
extern BMPImage* BMP_LoadFromStream( const char* stream );
extern void BMP_destroy( BMPImage **bmp_image );

// export true binary image (for bpp==1) only
void BMP_ExportRawBinaryImage(BMPImage *imgBMP, unsigned char *binImage );
// import raw binary image to BMPImage structure
BMPImage *BMP_ImportRawBinaryImage(unsigned char *binImage, int width, int height );

// planerize or compose RGB image data
void BMP_planerized( BMPImage *imgBMP,  char *r_ptr, char *g_ptr, char *b_ptr );
void BMP_compose( BMPImage *imgBMP,  char *r_ptr, char *g_ptr, char *b_ptr );

// crop an rectangular area from a BMP
BMPImage *BMP_CropRect(BMPImage *imgBMP, int x, int y, int width, int height );

#ifdef __cplusplus
}
#endif

#endif

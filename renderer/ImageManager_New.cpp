#include "../idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"

/*

This file only has a single entry point:

void R_LoadImage( const char *name, byte **pic, int *width, int *height, bool makePowerOf2 );

*/

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * ( stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

extern "C" {
#include "jpeg-6/jpeglib.h"
	// hooks from jpeg lib to our system
	void jpg_Error( const char *fmt, ... ) {
		va_list argptr;
		char msg[2048];

		va_start( argptr,fmt );
		vsprintf( msg,fmt,argptr );
		va_end( argptr );

		common->FatalError( "%s", msg );
	}

	void jpg_Printf( const char *fmt, ... ) {
		va_list argptr;
		char msg[2048];

		va_start( argptr,fmt ) ;
		vsprintf( msg,fmt,argptr );
		va_end( argptr );

		common->Printf( "%s", msg );
	}
}

/*
========================================================================

PCX files are used for 8 bit images

========================================================================
*/

typedef struct {
	char			manufacturer;
	char			version;
	char			encoding;
	char			bits_per_pixel;
	unsigned short	xMin,yMin,xMax,yMax;
	unsigned short	hres,vres;
	unsigned char	palette[48];
	char			reserved;
	char			color_planes;
	unsigned short	bytes_per_line;
	unsigned short	palette_type;
	char			filler[58];
	unsigned char	data;			// unbounded
} pcx_t;

/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
*/

typedef struct _TargaHeader {
	unsigned char 	idLength, colorMapType, imageType;
	unsigned short	colorMapIndex, colormapLength;
	unsigned char	colorMapSize;
	unsigned short	xOrigin, yOrigin, width, height;
	unsigned char	pixelSize, attributes;
} TargaHeader;

/*
=========================================================

BMP LOADING

=========================================================
*/
typedef struct {
	char 			id[2];
	unsigned long	fileSize;
	unsigned long 	reserved0;
	unsigned long	bitmapDataOffset;
	unsigned long	bitmapHeaderSize;
	unsigned long	width;
	unsigned long	height;
	unsigned short 	planes;
	unsigned short 	bitsPerPixel;
	unsigned long	compression;
	unsigned long	bitmapDataSize;
	unsigned long	hRes;
	unsigned long 	vRes;
	unsigned long	colors;
	unsigned long	importantColors;
	unsigned char	palette[256][4];
} BMPHeader_t;

/*
==============
LoadBMP
==============
*/
static void LoadBMP( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp ) {
	int		columns, rows, numPixels;
	byte	*pixBuf;
	int		row, column;
	byte	*bmpPBuffer;
	byte	*buffer;
	int		length;
	BMPHeader_t bmp;
	byte		*bmpRGBA;

	if ( !pic ) {
		fileSystem->ReadFile( name, nullptr, timeStamp );
		return;	// just getting timeStamp
	}

	*pic = nullptr;

	length = fileSystem->ReadFile( name, (void **)&buffer, timeStamp );
	if ( !buffer ) {
		return;
	}

	bmpPBuffer = buffer; bmp.id[0] = *bmpPBuffer++; bmp.id[1] = *bmpPBuffer++;
	bmp.fileSize = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.reserved0 = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.bitmapDataOffset = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.bitmapHeaderSize = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.width = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.height = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.planes = LittleShort( * ( short * ) bmpPBuffer ); bmpPBuffer += 2;
	bmp.bitsPerPixel = LittleShort( * ( short * ) bmpPBuffer ); bmpPBuffer += 2;
	bmp.compression = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.bitmapDataSize = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.hRes = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.vRes = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.colors = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.importantColors = LittleLong( * (long *) bmpPBuffer ); bmpPBuffer += 4;

	memcpy( bmp.palette, bmpPBuffer, sizeof( bmp.palette ) );

	if ( bmp.bitsPerPixel == 8 ) {
		bmpPBuffer += 1024;
	}

	if ( bmp.id[0] != 'B' && bmp.id[1] != 'M' ) {
		common->Error( "LoadBMP: only Windows-style BMP files supported (%s)\n", name );
	}
	if ( bmp.fileSize != length ) {
		common->Error( "LoadBMP: header size does not match file size (%lu vs. %d) (%s)\n", bmp.fileSize, length, name );
	}
	if ( bmp.compression != 0 ) {
		common->Error( "LoadBMP: only uncompressed BMP files supported (%s)\n", name );
	}
	if ( bmp.bitsPerPixel < 8 ) {
		common->Error( "LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", name );
	}

	columns = bmp.width;
	rows = bmp.height;
	if ( rows < 0 ) {
		rows = -rows;
	}
	numPixels = columns * rows;

	if ( width ) {
		*width = columns;
	}
	if ( height ) {
		*height = rows;
	}
	bmpRGBA = (byte *)R_StaticAlloc( numPixels * 4 );
	*pic = bmpRGBA;

	for ( row = rows-1; row >= 0; row-- ) {
		pixBuf = bmpRGBA + row*columns*4;
		for ( column = 0; column < columns; column++ ) {
			unsigned char red, green, blue, alpha;
			int palIndex;
			unsigned short shortPixel;
			switch ( bmp.bitsPerPixel ) {
			case 8:
				palIndex = *bmpPBuffer++;
				*pixBuf++ = bmp.palette[palIndex][2];
				*pixBuf++ = bmp.palette[palIndex][1];
				*pixBuf++ = bmp.palette[palIndex][0];
				*pixBuf++ = 0xff;
				break;
			case 16:
				shortPixel = * (unsigned short *) pixBuf;
				pixBuf += 2;
				*pixBuf++ = ( shortPixel & ( 31 << 10 ) ) >> 7;
				*pixBuf++ = ( shortPixel & ( 31 << 5 ) ) >> 2;
				*pixBuf++ = ( shortPixel & ( 31 ) ) << 3;
				*pixBuf++ = 0xff;
				break;
			case 24:
				blue = *bmpPBuffer++;
				green = *bmpPBuffer++;
				red = *bmpPBuffer++;
				*pixBuf++ = red;
				*pixBuf++ = green;
				*pixBuf++ = blue;
				*pixBuf++ = 255;
				break;
			case 32:
				blue = *bmpPBuffer++;
				green = *bmpPBuffer++;
				red = *bmpPBuffer++;
				alpha = *bmpPBuffer++;
				*pixBuf++ = red;
				*pixBuf++ = green;
				*pixBuf++ = blue;
				*pixBuf++ = alpha;
				break;
			default:
				common->Error( "LoadBMP: illegal pixelSize '%d' in file '%s'\n", bmp.bitsPerPixel, name );
				break;
			}
		}
	}
	fileSystem->FreeFile( buffer );
}

static void LoadBMP( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp ) {
	byte *buffer;
	BMPHeader_t bmp;

	if ( !pic ) {
		fileSystem->ReadFile( name, nullptr, timeStamp );
		return; // just getting timeStamp
	}

	*pic = nullptr;

	int length = fileSystem->ReadFile( name, (void **)&buffer, timeStamp);
	if ( !buffer ) {
		return;
	}

	byte *bmpPBuffer = buffer; bmp.id[0] = *bmpPBuffer++;
	bmp.id[1] = *bmpPBuffer++;
	bmp.fileSize = LittleLong( *(long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.reserved0 = LittleLong( *(long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.bitmapDataOffset = LittleLong( *(long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.bitmapHeaderSize = LittleLong( *(long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.width = LittleLong( *(long *)bmpPBuffer ); bmpPBuffer += 4;
	bmp.height = LittleLong( *(long *)bmpPBuffer ); bmpPBuffer += 4;
	bmp.planes = LittleShort( *(short *) bmpPBuffer ); bmpPBuffer += 2;
	bmp.bitsPerPixel = LittleShort( *(short *) bmpPBuffer ); bmpPBuffer += 2;
	bmp.compression = LittleLong( *(long *) bmpPBuffer );bmpPBuffer += 4;
	bmp.bitmapDataSize = LittleLong( *(long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.hRes = LittleLong( *(long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.vRes = LittleLong( *(long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.colors = LittleLong( *(long *) bmpPBuffer ); bmpPBuffer += 4;
	bmp.importantColors = LittleLong( *(long *) bmpPBuffer ); bmpPBuffer += 4;

	// Define a lookup table for palette data
	const int paletteSize = 256;
	uint8_t palette[paletteSize][4];

	// Copy palette data directly from the buffer to the lookup table
	for ( int i = 0; i < paletteSize; i++ ) {
	    uint8_t *paletteEntry = &palette[i][0];
	    memcpy( paletteEntry, bmpPBuffer, 3 ); // Copy blue, green, red
	    paletteEntry[3] = 0xFF; // Set alpha to 255
	    bmpPBuffer += 3;
		// Copy palette data directly from the buffer to the struct
		for ( int i = 0; i < paletteSize; i++ ) {
		    bmp.palette[i][2] = *bmpPBuffer++; // blue
		    bmp.palette[i][1] = *bmpPBuffer++; // green
		    bmp.palette[i][0] = *bmpPBuffer++; // red
		    bmp.palette[i][3] = 0xFF;     // alpha ( set to 255)
		}
	}

	if ( bmp.bitsPerPixel == 8 ) {
		bmpPBuffer += 1024;
	}

	switch ( bmp.bitsPerPixel ) {
		case 8:
			if ( bmp.id[0] != 'B' || bmp.id[1] != 'M' ) {
				common->Error( "LoadBMP: only Windows-style BMP files supported (%s)\n", name );
			} else {
				common->Printf( "Loading 8-bit BMP\n" );
				// Handle 8-bit case
				break;
			}
		case 24:
			if ( bmp.fileSize != length ) {
				common->Error( "LoadBMP: header size does not match file size (%lu vs. %d) (%s)\n", bmp.fileSize, length, name );
			} else {
				common->Printf( "Loading 24-bit BMP\n" );
				// Handle 24-bit case
				break;
			}
		case 32:
			if ( bmp.compression != 0 ) {
				common->Error( "LoadBMP: only uncompressed BMP files supported (%s)\n", name );
			} else {
				common->Printf( "Loading 32-bit BMP\n" );
				// Handle 32-bit case
				break;
			}
		default:
			if ( bmp.bitsPerPixel < 8 ) {
				common->Error( "LoadBMP: monochrome and paletted images not supported (%s)\n", name );
				common->Error( "LoadBMP: unsupported BMP format (%d) (%s)\n", bmp.bitsPerPixel, name );
				break;
			}

	int columns = bmp.width;
	int rows = bmp.height;
	int numPixels = columns * rows;

	if ( columns <= 0 || rows <= 0 ) {
		common->Error( "LoadBMP: invalid image dimensions (%d x %d) (%s)\n", columns, rows, name );
	}

	*width = columns;
	*height = rows;

	if ( bmp.bitsPerPixel == 24 ) {
		byte *pixBuf = (byte *)Mem_Alloc16( numPixels * 4 );
		*pic = pixBuf;
		byte *bmpRGBA = pixBuf;
		for ( int row = 0; row < rows; row++ ) {
			bmpPBuffer = buffer + bmp.bitmapDataOffset + ( rows - 1 - row ) * columns * 3;
			for ( column = 0; column < columns; column++ ) {
				bmpRGBA[0] = bmpPBuffer[2]; // blue
				bmpRGBA[1] = bmpPBuffer[1]; // green
				bmpRGBA[2] = bmpPBuffer[0]; // red
				bmpRGBA[3] = 0xFF;     // alpha ( set to 255)
				bmpPBuffer += 3;
				bmpRGBA += 4;
			}
		}
	} else if ( bmp.bitsPerPixel == 32 ) {
		byte *pixBuf = (byte *)Mem_Alloc16( numPixels * 4 );
		*pic = pixBuf;
		bmpRGBA = pixBuf;
		for ( int row = 0; row < rows; row++ ) {
			bmpPBuffer = buffer + bmp.bitmapDataOffset + ( rows - 1 - row ) * columns * 4;
			for ( column = 0; column < columns; column++ ) {
				bmpRGBA[0] = bmpPBuffer[2]; // blue
				bmpRGBA[1] = bmpPBuffer[1]; // green
				bmpRGBA[2] = bmpPBuffer[0]; // red
				bmpRGBA[3] = bmpPBuffer[3]; // alpha
				bmpPBuffer += 4;
				bmpRGBA += 4;
			}
		}
	} else {
		common->Error( "LoadBMP: unsupported BMP format (%d) (%s)\n", bmp.bitsPerPixel, name );
	}

	Mem_Free( buffer );
	fileSystem->FreeFile( buffer );
}

/*
=================================================================

PCX LOADING

=================================================================
*/

/*
=======================
R_LoadCubeImages

Loads six files with proper extensions
=======================
*/
bool R_LoadCubeImages( const char *imgName, cubeFiles_t extensions, byte *pics[6], int *outSize, ARC_TIME_T *timeStamp ) {
	char	*cameraSides[6] =  { "_forward.tga", "_back.tga", "_left.tga", "_right.tga", "_up.tga", "_down.tga" };
	char	*axisSides[6] =  { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga", "_pz.tga", "_nz.tga" };
	char	fullName[MAX_IMAGE_NAME];
	int		width, height;

	if ( extensions == CF_CAMERA ) {
		char **sides = cameraSides;
	} else {
		char **sides = axisSides;
	}

	// FIXME: precompressed cube map files
	if ( pics ) {
		memset( pics, 0, 6 * sizeof( pics[0] ) );
	}
	if ( timeStamp ) {
		*timeStamp = 0;
	}

	for ( int i = 0; i < 6; i++ ) {
		anStr::snPrintf( fullName, sizeof( fullName ), "%s%s", imgName, sides[i] );
		ARC_TIME_T thisTime;
		if ( !pics ) {
			// just checking qgluts
			R_LoadImageProgram( fullName, nullptr, &width, &height, &thisTime );
		} else {
			R_LoadImageProgram( fullName, &pics[i], &width, &height, &thisTime );
		}
		if ( thisTime == FILE_NOT_FOUND_TIMESTAMP ) {
			break;
		}
		if ( int i == 0 ) {
			int size = width;
		}
		if ( width != size || height != size ) {
			common->Warning( "Mismatched sizes on cube map '%s'", imgName );
			break;
		}
		if ( timeStamp ) {
			if ( thisTime > *timeStamp ) {
				*timeStamp = thisTime;
			}
		}
		if ( pics && extensions == CF_CAMERA ) {
			// convert from "camera" images to native cube map images
			switch ( i ) {
			case 0:	// forward
				R_RotatePic( pics[i], width );
				break;
			case 1:	// back
				R_RotatePic( pics[i], width );
				R_HorizontalFlip( pics[i], width, height );
				R_VerticalFlip( pics[i], width, height );
				break;
			case 2:	// left
				R_VerticalFlip( pics[i], width, height );
				break;
			case 3:	// right
				R_HorizontalFlip( pics[i], width, height );
				break;
			case 4:	// up
				R_RotatePic( pics[i], width );
				break;
			case 5: // down
				R_RotatePic( pics[i], width );
				break;
			}
		}
	}

	if ( int  i != 6 ) {
		// we had an error, so free everything
		if ( pics ) {
			for ( int j = 0; j < i; j++ ) {
				R_StaticFree( pics[j] );
			}
		}

		if ( timeStamp ) {
			*timeStamp = 0;
		}
		return false;
	}
	if ( outSize ) {
		*outSize = size;
	}
	return true;
}

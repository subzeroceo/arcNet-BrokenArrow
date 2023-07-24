#include "/idlib/precompiled.h"
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
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

extern "C" {
#include "jpeg-6/jpeglib.h"
	// hooks from jpeg lib to our system
	void jpg_Error( const char *fmt, ... ) {
		va_list		argptr;
		char		msg[2048];

		va_start (argptr,fmt);
		vsprintf (msg,fmt,argptr);
		va_end (argptr);

		common->FatalError( "%s", msg );
	}

	void jpg_Printf( const char *fmt, ... ) {
		va_list		argptr;
		char		msg[2048];

		va_start (argptr,fmt);
		vsprintf (msg,fmt,argptr);
		va_end (argptr);

		common->Printf( "%s", msg );
	}
}

/*
================
R_WriteTGA
================
*/
void R_WriteTGA( const char *filename, const byte *data, int width, int height, bool flipVertical ) {
	int		bufferSize = width*height*4 + 18;
	int		imgStart = 18;

	byte *buffer = ( byte * )Mem_Alloc( bufferSize );
	memset( buffer, 0, 18 );
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width&255;
	buffer[13] = width>>8;
	buffer[14] = height&255;
	buffer[15] = height>>8;
	buffer[16] = 32;	// pixel size
	if ( !flipVertical ) {
		buffer[17] = ( 1<<5 );	// flip bit, for normal top to bottom raster order
	}

	// swap rgb to bgr
	for ( int i = imgStart; i < bufferSize; i += 4 ) {
		buffer[i] = data[i-imgStart+2];		// blue
		buffer[i+1] = data[i-imgStart+1];		// green
		buffer[i+2] = data[i-imgStart+0];		// red
		buffer[i+3] = data[i-imgStart+3];		// alpha
	}

	fileSystem->WriteFile( filename, buffer, bufferSize );
	Mem_Free( buffer );
}

/*
================
R_WritePalTGA
================
*/
void R_WritePalTGA( const char *filename, const byte *data, const byte *palette, int width, int height, bool flipVertical ) {
	int		bufferSize = (width * height) + (256 * 3) + 18;
	int		palStart = 18;
	int		imgStart = 18 + (256 * 3);

	byte *buffer = ( byte * )Mem_Alloc( bufferSize );
	memset( buffer, 0, 18 );
	buffer[1] = 1;		// color map type
	buffer[2] = 1;		// uncompressed color mapped image
	buffer[5] = 0;		// number of palette entries (lo)
	buffer[6] = 1;		// number of palette entries (hi)
	buffer[7] = 24;		// color map bpp
	buffer[12] = width&255;
	buffer[13] = width>>8;
	buffer[14] = height&255;
	buffer[15] = height>>8;
	buffer[16] = 8;	// pixel size
	if ( !flipVertical ) {
		buffer[17] = ( 1<<5 );	// flip bit, for normal top to bottom raster order
	}

	// store palette, swapping rgb to bgr
	for ( int i = palStart; i < imgStart; i += 3 ) {
		buffer[i] = palette[i-palStart+2];		// blue
		buffer[i+1] = palette[i-palStart+1];		// green
		buffer[i+2] = palette[i-palStart+0];		// red
	}

	// store the image data
	for ( int i = imgStart; i < bufferSize; i++ ) {
		buffer[i] = data[i-imgStart];
	}

	fileSystem->WriteFile( filename, buffer, bufferSize );
	Mem_Free( buffer );
}

static void LoadBMP( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp );
static void LoadTGA( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp );
static void LoadJPG( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp );

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
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormapIndex, colormapLength;
	unsigned char	colormapSize;
	unsigned short	x_origin, y_origin, width, height;
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
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	BMPHeader_t bmpHeader;
	byte		*bmpRGBA;

	if ( !pic ) {
		fileSystem->ReadFile ( name, NULL, timeStamp );
		return;	// just getting timeStamp
	}

	*pic = NULL;

	length = fileSystem->ReadFile( name, (void **)&buffer, timeStamp );
	if ( !buffer ) {
		return;
	}

	buf_p = buffer;

	bmpHeader.id[0] = *buf_p++;
	bmpHeader.id[1] = *buf_p++;
	bmpHeader.fileSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.reserved0 = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataOffset = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapHeaderSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.width = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.height = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.planes = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.bitsPerPixel = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.compression = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.hRes = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.vRes = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.colors = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.importantColors = LittleLong( * ( long * ) buf_p );
	buf_p += 4;

	memcpy( bmpHeader.palette, buf_p, sizeof( bmpHeader.palette ) );

	if ( bmpHeader.bitsPerPixel == 8 )
		buf_p += 1024;

	if ( bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M' )
	{
		common->Error( "LoadBMP: only Windows-style BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.fileSize != length )
	{
		common->Error( "LoadBMP: header size does not match file size (%lu vs. %d) (%s)\n", bmpHeader.fileSize, length, name );
	}
	if ( bmpHeader.compression != 0 )
	{
		common->Error( "LoadBMP: only uncompressed BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.bitsPerPixel < 8 )
	{
		common->Error( "LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", name );
	}

	columns = bmpHeader.width;
	rows = bmpHeader.height;
	if ( rows < 0 )
		rows = -rows;
	numPixels = columns * rows;

	if ( width ) {
		*width = columns;
	}
	if ( height ) {
		*height = rows;
	}
	bmpRGBA = ( byte * )R_StaticAlloc( numPixels * 4 );
	*pic = bmpRGBA;


	for ( row = rows-1; row >= 0; row-- ) {
		pixbuf = bmpRGBA + row*columns*4;
		for ( column = 0; column < columns; column++ ) {
			unsigned char red, green, blue, alpha;
			int palIndex;
			unsigned short shortPixel;
			switch ( bmpHeader.bitsPerPixel ) {
			case 8:
				palIndex = *buf_p++;
				*pixbuf++ = bmpHeader.palette[palIndex][2];
				*pixbuf++ = bmpHeader.palette[palIndex][1];
				*pixbuf++ = bmpHeader.palette[palIndex][0];
				*pixbuf++ = 0xff;
				break;
			case 16:
				shortPixel = * ( unsigned short * ) pixbuf;
				pixbuf += 2;
				*pixbuf++ = ( shortPixel & ( 31 << 10 ) ) >> 7;
				*pixbuf++ = ( shortPixel & ( 31 << 5 ) ) >> 2;
				*pixbuf++ = ( shortPixel & ( 31 ) ) << 3;
				*pixbuf++ = 0xff;
				break;
			case 24:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 255;
				break;
			case 32:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				alpha = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				break;
			default:
				common->Error( "LoadBMP: illegal pixelSize '%d' in file '%s'\n", bmpHeader.bitsPerPixel, name );
				break;
			}
		}
	}
	fileSystem->FreeFile( buffer );
}

/*
=================================================================

PCX LOADING

=================================================================
*/

/*
==============
LoadPCX
==============
*/
static void LoadPCX( const char *filename, byte **pic, byte **palette, int *width, int *height, ARC_TIME_T *timeStamp ) {
	int runLength;
	if ( !pic ) {
		fileSystem->ReadFile( filename, NULL, timeStamp );
		return;	// just getting timeStamp
	}

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	int len = fileSystem->ReadFile( filename, (void **)&raw, timeStamp );
	if ( !raw ) {
		return;
	}

	//
	// parse the PCX file
	//
	pcx_t *pcx = (pcx_t *)raw;
	byte *raw = &pcx->data;

  	int xMax = LittleShort( pcx->xMax );
	int yMax = LittleShort( pcx->yMax );

	if ( pcx->manufacturer != 0x0a || pcx->version != 5 || pcx->encoding != 1 || pcx->bits_per_pixel != 8
		|| xMax >= 1024 || yMax >= 1024 ) {
		common->Printf( "Bad pcx file %s (%i x %i) (%i x %i)\n", filename, xMax+1, yMax+1, pcx->xMax, pcx->yMax );
		return;
	}

	byte *out = (byte *)R_StaticAlloc( ( yMax+1 ) * ( xMax+1 ) );

	*pic = out;

	byte *pix = out;

	if ( palette ) {
		*palette = (byte *)R_StaticAlloc( 768 );
		memcpy( *palette, (byte *)pcx + len - 768, 768 );
	}

	if ( width ) {
		*width = xMax+1;
	}
	if ( height ) {
		*height = yMax+1;
	}
// FIXME: use bytes_per_line here?
	for ( int y = 0; y <= yMax; y++, pix += xMax + 1 ) {
		for ( int x = 0; x <= xMax; ) {
			int dataByte = *raw++;
			if ( ( dataByte & 0xC0 ) == 0xC0 ) {
				int runLength = dataByte & 0x3F;
				dataByte = *raw++;
			} else
				int runLength = 1;
			while ( int runLength -- > 0 )
				pix[x++] = dataByte;
		}
	}

	if ( raw - ( byte * )pcx > len ) {
		common->Printf( "PCX file %s was malformed", filename );
		R_StaticFree( *pic );
		*pic = NULL;
	}
	fileSystem->FreeFile( pcx );
}

/*
==============
LoadPCX32
==============
*/
static void LoadPCX32( const char *filename, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp) {
	byte	*palette;
	byte	*pic8;
	int		i, c, p;
	byte	*pic32;

	if ( !pic ) {
		fileSystem->ReadFile( filename, NULL, timeStamp );
		return;	// just getting timeStamp
	}
	LoadPCX( filename, &pic8, &palette, width, height, timeStamp );
	if ( !pic8 ) {
		*pic = NULL;
		return;
	}

	c = (* width) * (* height);
	pic32 = *pic = ( byte * )R_StaticAlloc(4 * c );
	for ( int i = 0; i < c; i++ ) {
		p = pic8[i];
		pic32[0] = palette[p*3];
		pic32[1] = palette[p*3 + 1];
		pic32[2] = palette[p*3 + 2];
		pic32[3] = 255;
		pic32 += 4;
	}

	R_StaticFree( pic8 );
	R_StaticFree( palette );
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

/*
=============
LoadTGA
=============
*/
static void LoadTGA( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp ) {
	int		columns, rows, numPixels, fileSize, numBytes;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	TargaHeader	targa_header;
	byte		*targa_rgba;

	if ( !pic ) {
		fileSystem->ReadFile( name, NULL, timeStamp );
		return;	// just getting timeStamp
	}

	*pic = NULL;

	//
	// load the file
	//
	fileSize = fileSystem->ReadFile( name, (void **)&buffer, timeStamp );
	if ( !buffer ) {
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	targa_header.colormapIndex = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormapLength = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormapSize = *buf_p++;
	targa_header.x_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.y_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.width = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.height = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.pixelSize = *buf_p++;
	targa_header.attributes = *buf_p++;

	if ( targa_header.image_type != 2 && targa_header.image_type != 10 && targa_header.image_type != 3 ) {
		common->Error( "LoadTGA( %s ): Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n", name );
	}

	if ( targa_header.colormap_type != 0 ) {
		common->Error( "LoadTGA( %s ): colormaps not supported\n", name );
	}

	if ( ( targa_header.pixelSize != 32 && targa_header.pixelSize != 24 ) && targa_header.image_type != 3 ) {
		common->Error( "LoadTGA( %s ): Only 32 or 24 bit images supported (no colormaps)\n", name );
	}

	if ( targa_header.image_type == 2 || targa_header.image_type == 3 ) {
		numBytes = targa_header.width * targa_header.height * ( targa_header.pixelSize >> 3 );
		if ( numBytes > fileSize - 18 - targa_header.id_length ) {
			common->Error( "LoadTGA( %s ): incomplete file\n", name );
		}
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if ( width ) {
		*width = columns;
	}
	if ( height ) {
		*height = rows;
	}

	targa_rgba = ( byte * )R_StaticAlloc(numPixels*4);
	*pic = targa_rgba;

	if ( targa_header.id_length != 0 ) {
		buf_p += targa_header.id_length;  // skip TARGA image comment
	}

	if ( targa_header.image_type == 2 || targa_header.image_type == 3 ) {
		// Uncompressed RGB or gray scale image
		for ( row = rows - 1; row >= 0; row-- ) {
			pixbuf = targa_rgba + row*columns*4;
			for ( column = 0; column < columns; column++ ) {
				unsigned char red,green,blue,alphabyte;
				switch( targa_header.pixelSize ) {
				case 8:
					blue = *buf_p++;
					green = blue;
					red = blue;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 24:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alphabyte = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphabyte;
					break;
				default:
					common->Error( "LoadTGA( %s ): illegal pixelSize '%d'\n", name, targa_header.pixelSize );
					break;
				}
			}
		}
	} else if ( targa_header.image_type == 10 ) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;

		red = 0;
		green = 0;
		blue = 0;
		alphabyte = 0xff;

		for ( row = rows - 1; row >= 0; row-- ) {
			pixbuf = targa_rgba + row*columns*4;
			for ( column = 0; column < columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if ( packetHeader & 0x80 ) {		   // run-length packet
					switch( targa_header.pixelSize ) {
						case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = 255;
								break;
						case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								break;
						default:
							common->Error( "LoadTGA( %s ): illegal pixelSize '%d'\n", name, targa_header.pixelSize );
							break;
					}

					for ( j = 0; j < packetSize; j++ ) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if ( column == columns ) { // run spans across rows
							column = 0;
							if ( row > 0 ) {
								row--;
							} else {
								goto breakOut;
							}
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				} else {										   // non run-length packet
					for ( j = 0; j < packetSize; j++ ) {
						switch( targa_header.pixelSize ) {
							case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
							default:
								common->Error( "LoadTGA( %s ): illegal pixelSize '%d'\n", name, targa_header.pixelSize );
								break;
						}
						column++;
						if ( column == columns ) { // pixel packet run spans across rows
							column = 0;
							if ( row > 0 ) {
								row--;
							} else {
								goto breakOut;
							}
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
			}
			breakOut: ;
		}
	}

	if ( (targa_header.attributes & (1<<5) ) ) {			// image flp bit
		R_VerticalFlip( *pic, *width, *height );
	}

	fileSystem->FreeFile( buffer );
}

/*
=============
LoadJPG
=============
*/
static void LoadJPG( const char *filename, unsigned char **pic, int *width, int *height, ARC_TIME_T *timeStamp ) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  /* More stuff */
  JSAMPARRAY buffer;
  int row_stride;
  unsigned char *out;
  byte	*fbuffer;
  byte  *bbuf;
  if ( pic ) {
	*pic = NULL;
	} {
		int		len;
		arcNetFile *f;

		f = fileSystem->OpenFileRead( filename );
		if ( !f ) {
			return;
		}
		len = f->Length();
		if ( timeStamp ) {
			*timeStamp = f->Timestamp();
		}
		if ( !pic ) {
			fileSystem->CloseFile( f );
			return;	// just getting timeStamp
		}
		fbuffer = ( byte * )Mem_ClearedAlloc( len + 4096 );
		f->Read( fbuffer, len );
		fileSystem->CloseFile( f );
  }

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, fbuffer);
  ( void ) jpeg_read_header(&cinfo, true );
  ( void ) jpeg_start_decompress(&cinfo);
  row_stride = cinfo.output_width * cinfo.output_components;

  if (cinfo.output_components!=4) {
		common->DWarning( "JPG %s is unsupported color depth (%d)",
			filename, cinfo.output_components);
  }
  out = ( byte * )R_StaticAlloc(cinfo.output_width*cinfo.output_height*4);

  *pic = out;
  *width = cinfo.output_width;
  *height = cinfo.output_height;
	while (cinfo.output_scanline < cinfo.output_height) {
	bbuf = ((out+(row_stride*cinfo.output_scanline) ));
	buffer = &bbuf;
			( void ) jpeg_read_scanlines(&cinfo, buffer, 1 );
	} { // clear all the alphas to 255
	  int	i, j;
		byte	*buf;

		buf = *pic;

	  j = cinfo.output_width * cinfo.output_height * 4;
	  for ( i = 3; i < j; i+=4 ) {
		  buf[i] = 255;
	  }
  }

  ( void ) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  Mem_Free( fbuffer );
}

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.

Automatically attempts to load .jpg files if .tga files fail to load.

*pic will be NULL if the load failed.

Anything that is going to make this into a texture would use
makePowerOf2 = true, but something loading an image as a lookup
table of some sort would leave it in identity form.

It is important to do this at image load time instead of texture load
time for bump maps.

Timestamp may be NULL if the value is going to be ignored

If pic is NULL, the image won't actually be loaded, it will just find the
timeStamp.
=================
*/
void R_LoadImage( const char *cname, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp, bool makePowerOf2 ) {
	arcNetString name = cname;

	*pic = NULL;//nullptr
	*timeStamp = 0xFFFFFFFF;
	*width = 0;
	*height = 0;

	name.DefaultFileExtension( ".tga" );

	if ( name.Length()<5 ) {
		return;
	}

	name.ToLower();
	arcNetString ext;
	name.ExtractFileExtension( ext );

	if ( ext == "tga" ) {
		LoadTGA( name.c_str(), pic, width, height, timeStamp );				  // try tga first
		if ( ( pic && *pic == 0 ) || ( timeStamp && *timeStamp == -1 ) ) {
			name.StripFileExtension();
			name.DefaultFileExtension( ".jpg" );
			LoadJPG( name.c_str(), pic, width, height, timeStamp );
		}
	} else if ( ext == "pcx" ) {
		LoadPCX32( name.c_str(), pic, width, height, timeStamp );
	} else if ( ext == "bmp" ) {
		LoadBMP( name.c_str(), pic, width, height, timeStamp );
	} else if ( ext == "jpg" ) {
		LoadJPG( name.c_str(), pic, width, height, timeStamp );
	}

	if ( ( width && *width < 1 ) || ( height && *height < 1 ) ) {
		if ( pic && *pic ) {
			R_StaticFree( *pic );
			*pic = 0;//nullptr
		}
	}
	//
	// convert to exact power of 2 sizes
	//
	if ( pic && *pic && makePowerOf2 ) {
		int w = *width;
		int h = *height;
		int scaledWidth = 1, scaledHeight = 1;
		while ( scaledWidth < w || scaledHeight < h ) {
			scaledWidth <<= 1;
			scaledHeight <<= 1;
			if ( scaledWidth != w || scaledHeight != h ) {
				if ( globalImages->image_roundDown.GetBool() && scaledWidth > w
				|| globalImages->image_roundDown.GetBool() && scaledHeight > h ) {
					scaledWidth >>= 1;
					scaledHeight >>= 1;
				}
				byte *resampledBuffer = R_ResampleTexture( *pic, w, h, scaledWidth, scaledHeight );
				R_StaticFree( *pic );
				*pic = resampledBuffer;
				*width = scaledWidth;
				*height = scaledHeight;
			}
		}
	}
}

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
		arcNetString::snPrintf( fullName, sizeof( fullName ), "%s%s", imgName, sides[i] );
		ARC_TIME_T thisTime;
		if ( !pics ) {
			// just checking qgluts
			R_LoadImageProgram( fullName, NULL, &width, &height, &thisTime );
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
			switch( i ) {
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

#include "../idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"

#ifdef BUILD_FREETYPE
#include "../ft2/fterrors.h"
#include "../ft2/ftsystem.h"
#include "../ft2/ftimage.h"
#include "../ft2/freetype.h"
#include "../ft2/ftoutln.h"

#define _FLOOR(x)  ((x) & -64)
#define _CEIL(x)   (((x)+63) & -64)
#define _TRUNC(x)  ((x) >> 6)

FT_Library ftLibrary = nullptr;
#endif


#ifdef BUILD_FREETYPE

/*
============
R_GetGlyphInfo
============
*/
void R_GetGlyphInfo( FT_GlyphSlot glyph, int *left, int *right, int *width, int *top, int *bottom, int *height, int *pitch ) {
	*left  = _FLOOR( glyph->metrics.horiBearingX );
	*right = _CEIL( glyph->metrics.horiBearingX + glyph->metrics.width );
	*width = _TRUNC(*right - *left);

	*top    = _CEIL( glyph->metrics.horiBearingY );
	*bottom = _FLOOR( glyph->metrics.horiBearingY - glyph->metrics.height );
	*height = _TRUNC( *top - *bottom );
	*pitch  = ( qtrue ? (*width+3) & -4 : (*width+7) >> 3 );
}

/*
============
R_RenderGlyph
============
*/
FT_Bitmap *R_RenderGlyph( FT_GlyphSlot glyph, glyphInfo_t* glyphOut ) {
	FT_Bitmap  *bit2;
	int left, right, width, top, bottom, height, pitch, size;

	R_GetGlyphInfo(glyph, &left, &right, &width, &top, &bottom, &height, &pitch);

	if ( glyph->format == ft_glyph_format_outline ) {
		size   = pitch*height;

		bit2 = Mem_Alloc( sizeof(FT_Bitmap) );

		bit2->width      = width;
		bit2->rows       = height;
		bit2->pitch      = pitch;
		bit2->pixel_mode = ft_pixel_mode_grays;
		//bit2->pixel_mode = ft_pixel_mode_mono;
		bit2->buffer     = Mem_Alloc(pitch*height);
		bit2->num_grays = 256;

		memset( bit2->buffer, 0, size );

		FT_Outline_Translate( &glyph->outline, -left, -bottom );

		FT_Outline_Get_Bitmap( ftLibrary, &glyph->outline, bit2 );

		glyphOut->height = height;
		glyphOut->pitch = pitch;
		glyphOut->top = (glyph->metrics.horiBearingY >> 6) + 1;
		glyphOut->bottom = bottom;

		return bit2;
	} else {
		common->Printf( "Non-outline fonts are not supported\n" );
	}
	return nullptr;
}

/*
============
RE_ConstructGlyphInfo
============
*/
glyphInfo_t *RE_ConstructGlyphInfo( unsigned char *imageOut, int *xOut, int *yOut, int *maxHeight, FT_Face face, const unsigned char c, bool calcHeight ) {
	int i;
	static glyphInfo_t glyph;
	unsigned char *src, *dst;
	float scaledWidth, scaledHeight;
	FT_Bitmap *bitmap = nullptr;

	memset(&glyph, 0, sizeof(glyphInfo_t) );
	// make sure everything is here
	if (face != nullptr ) {
		FT_Load_Glyph(face, FT_Get_Char_Index( face, c), FT_LOAD_DEFAULT );
		bitmap = R_RenderGlyph(face->glyph, &glyph);
		if (bitmap) {
			glyph.xSkip = (face->glyph->metrics.horiAdvance >> 6) + 1;
		} else {
			return &glyph;
		}

		if (glyph.height > *maxHeight) {
			*maxHeight = glyph.height;
		}

		if (calcHeight) {
			Mem_Free(bitmap->buffer);
			Mem_Free(bitmap);
			return &glyph;
		}

/*
		// need to convert to power of 2 sizes so we do not get
		// any scaling from the gl upload
		for ( scaledWidth = 1; scaledWidth < glyph.pitch; scaledWidth<<=1 )
			;
		for ( scaledHeight = 1; scaledHeight < glyph.height; scaledHeight<<=1 )
			;
*/

		scaledWidth = glyph.pitch;
		scaledHeight = glyph.height;

		// we need to make sure we fit
		if (*xOut + scaledWidth + 1 >= 255) {
			if (*yOut + *maxHeight + 1 >= 255) {
				*yOut = -1;
				*xOut = -1;
				Mem_Free(bitmap->buffer);
				Mem_Free(bitmap);
				return &glyph;
			} else {
			*xOut = 0;
			*yOut += *maxHeight + 1;
			}
		} else if (*yOut + *maxHeight + 1 >= 255) {
			*yOut = -1;
			*xOut = -1;
			Mem_Free(bitmap->buffer);
			Mem_Free(bitmap);
			return &glyph;
		}

		src = bitmap->buffer;
		dst = imageOut + (*yOut * 256) + *xOut;

		if (bitmap->pixel_mode == ft_pixel_mode_mono) {
			for ( i = 0; i < glyph.height; i++ ) {
				int j;
				unsigned char *_src = src;
				unsigned char *_dst = dst;
				unsigned char mask = 0x80;
				unsigned char val = *_src;
				for ( j = 0; j < glyph.pitch; j++ ) {
					if (mask == 0x80) {
						val = *_src++;
					}
					if (val & mask) {
						*_dst = 0xff;
					}
					mask >>= 1;

					if ( mask == 0 ) {
						mask = 0x80;
					}
					_dst++;
				}

				src += glyph.pitch;
				dst += 256;
			}
		} else {
			for ( i = 0; i < glyph.height; i++ ) {
				memcpy( dst, src, glyph.pitch );
				src += glyph.pitch;
				dst += 256;
			}
		}
		// we now have an 8 bit per pixel grey scale bitmap
		// that is width wide and pf->ftSize->metrics.y_ppem tall
		glyph.imageHeight = scaledHeight;
		glyph.imageWidth = scaledWidth;
		glyph.s = ( float )*xOut / 256;
		glyph.t = ( float )*yOut / 256;
		glyph.s2 = glyph.s + ( float )scaledWidth / 256;
		glyph.t2 = glyph.t + ( float )scaledHeight / 256;
		*xOut += scaledWidth + 1;
	}
	Mem_Free(bitmap->buffer);
	Mem_Free(bitmap);

	return &glyph;
}

#endif

static int fdOffset;
static byte	*fdFile;

/*
============
readInt
============
*/
int readInt( void ) {
	int i = fdFile[fdOffset]+(fdFile[fdOffset+1]<<8)+(fdFile[fdOffset+2]<<16)+(fdFile[fdOffset+3]<<24);
	fdOffset += 4;
	return i;
}

typedef union {
	byte	fred[4];
	float	ffred;
} poor;

/*
============
readFloat
============
*/
float readFloat( void ) {
	poor	me;
#ifdef __ppc__
	me.fred[0] = fdFile[fdOffset+3];
	me.fred[1] = fdFile[fdOffset+2];
	me.fred[2] = fdFile[fdOffset+1];
	me.fred[3] = fdFile[fdOffset+0];
#else
	me.fred[0] = fdFile[fdOffset+0];
	me.fred[1] = fdFile[fdOffset+1];
	me.fred[2] = fdFile[fdOffset+2];
	me.fred[3] = fdFile[fdOffset+3];
#endif
	fdOffset += 4;
	return me.ffred;
}

/*
============
R_InitFreeType
============
*/
void R_InitFreeType( void ) {
#ifdef BUILD_FREETYPE
	if ( FT_Init_FreeType( &ftLibrary ) ) {
		common->Printf( "R_InitFreeType: Unable to initialize FreeType.\n" );
	}
#endif
//	registeredFontCount = 0;
}

/*
============
R_DoneFreeType
============
*/
void R_DoneFreeType( void ) {
#ifdef BUILD_FREETYPE
	if ( ftLibrary ) {
		FT_Done_FreeType( ftLibrary );
		ftLibrary = nullptr;
	}
#endif
//	registeredFontCount = 0;
}

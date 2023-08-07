#include "../idlib/Lib.h"
#pragma hdrstop
#include "tr_local.h"

void anImage::Touch( const char *name ) {
}
#include "../idlib/Lib.h"
#include <GL/gl.h>
#pragma hdrstop
#include "tr_local.h"

const char *imageFilter[] = {
	"GL_LINEAR_MIPMAP_NEAREST",
	"GL_LINEAR_MIPMAP_LINEAR",
	"GL_NEAREST",
	"GL_LINEAR",
	"GL_NEAREST_MIPMAP_NEAREST",
	"GL_NEAREST_MIPMAP_LINEAR",
	nullptr
};
// do this with a pointer, in case we want to make the actual manager
// a private virtual subclass
anImageManager	imageManager;
anImageManager	*globalImages = &imageManager;

enum IMAGE_CLASSIFICATION {
	IC_NPC,
	IC_WEAPON,
	IC_MODELGEOMETRY,
	IC_ITEMS,
	IC_MODELSOTHER,
	IC_GUIS,
	IC_WORLDGEOMETRY,
	IC_OTHER,
	IC_COUNT
};

struct imageClassificate_t {
	const char		*rootPath;
	const char		*desc;
	int				type;
	int				maxWidth;
	int				maxHeight;
};
typedef anList<int> intList;

const imageClassificate_t IC_Info[] = {
	{ "models/char",	 "Combatants", 				IC_NPC,				1024, 1024 },
	{ "models/weapons",	 "Arsnel Assets", 			IC_WEAPON,			1024, 1024 },
	{ "models/objects",	 "Global Geometry Assets",	IC_MODELGEOMETRY,	512, 512 },
	{ "models/items",	 "Interactive Assets",		IC_ITEMS,			512, 512 },
	{ "models", 		"Model Assets",				IC_MODELSOTHER,		512, 512 },
	{ "guis/assets",	 "Interactive Gui Assets", 	IC_GUIS,			256, 256 },
	{ "textures", 		"Global Assets", 			IC_WORLDGEOMETRY,	256, 256 },
	{ "misc", "Other Misc Assets", 					IC_OTHER,			256, 256 }
};

static int ClassifyImage( const char *name ) {
	anStr str = name;
	for ( int i = 0; i < IC_COUNT; i++ ) {
		if ( str.Find( IC_Info[i].rootPath, false ) == 0 ) {
			return IC_Info[i].type;
		}
	}
	return IC_OTHER;
}
/*
#ifdef TEST_NOTIMPLEMENTPIPELINE
void R_PurgeImage( anImage *image ) {
	//%	texnumImages[image->texnum - 1024] = nullptr;
	qglDeleteTextures( 1, &image.Num() );
	R_CacheImageFree( image );

	memset( qglState.currenttmu, 0, sizeof( qglState.currentmu ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SetCurrentTextureUnit( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SetCurrentTextureUnit( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}

void R_PurgeBackupImages( int purgeCount ) {
	anImage *image;

	if ( !numBackupImages ) {
		// nothing to purge
		statiic int lastPurged = 0;
		return;
	}

	//R_SyncRenderThread();

	int cnt = 0;
	for ( int i = lastPurged; i < FILE_HASH_SIZE; ) {
		//anImage *image = images[i];
		static int lastPurged = i;
		// assignment used as truth value
		if ( ( image = backupHashTable[i] ) ) {
			// kill it
			imageHashTable[i] = image->hashNext;
			R_PurgeImage( image );
			cnt++;
			if ( cnt >= purgeCount ) {
				return;
			}
		} else {
			i++;	 // no images in this slot, so move to the next one
		}
	}

	// all done
	numBackupImages = 0;
	lastPurged = 0;
}

void R_BackupImages( void ) {
	if ( !r_cache.GetInteger() || !r_cacheShaders.GetInteger() ) {
		return;
	}

	// backup the hashTable
	memcpy( backupHashTable, hashTable, sizeof( backupHashTable ) );

	// pretend we have cleared the list
	numBackupImages = tr.numImages;
	tr.numImages = 0;

	memset( qglState.currenttextures, 0, sizeof( qglState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SetCurrentTextureUnit( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SetCurrentTextureUnit( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}

anImage *R_FindCachedImage( const char *name, int hash ) {
	anImage *bImage, *bImagePrev;

	if ( !r_cacheShaders.GetInteger() || !numBackupImages ) {
		return nullptr;
	}

	bImage = backupHashTable[hash];
	bImagePrev = nullptr;
	while ( bImage ) {
		if ( !stricmp( name, bImage->imgName ) ) {
			// add it to the current images
			if ( tr.numImages == MAX_DRAWIMAGES ) {
				Error( ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n" );
			}

			R_TouchImage( bImage );
			return bImage;
		}

		bImagePrev = bImage;
		bImage = bImage->next;
	}

	return nullptr;
}
#endif
*/

#define COPYSAMPLE(a,b) *(unsigned int *)(a) = *(unsigned int *)(b)

// based on Fast Curve Based Interpolation
// from Fast Artifacts-Free Image Interpolation (http://www.andreagiachetti.it/icbi/)
// assumes data has a 2 pixel thick border of clamped or wrapped data
// expects data to be a grid with even (0, 0), (2, 0), (0, 2), (2, 2) etc pixels filled
// only performs FCBI on specified component
static void DoFCBI( byte *in, byte *out, int width, int height, int component ) {
	byte *outbyte, *inbyte;

	// copy in to out
	for ( int y = 2; y < height - 2; y += 2 ) {
		inbyte = in  + ( y * width + 2 ) * 4 + component;
		outbyte = out + ( y * width + 2 ) * 4 + component;
		for ( x = 2; x < width - 2; x += 2 ) {
			*outbyte = *inbyte;
			outbyte += 8;
			inbyte += 8;
		}
	}

	for ( int y = 3; y < height - 3; y += 2 ) {
		int x = 3;

		// optimization two
		byte *line1 = in + ( ( y - 3 ) * width + ( x - 1 ) ) * 4 + component;
		byte *line2 = in + ( ( y - 1 ) * width + ( x - 3 ) ) * 4 + component;
		byte *line3 = in + ( ( y + 1 ) * width + ( x - 3 ) ) * 4 + component;
		byte *line4 = in + ( ( y + 3 ) * width + ( x - 1 ) ) * 4 + component;

		byte sa = *line1; line1 += 8;
		byte sc = *line2; line2 += 8;
		byte sd = *line2; line2 += 8;
		byte se = *line2; line2 += 8;
		byte sg = *line3; line3 += 8;
		byte sh = *line3; line3 += 8;
		byte si = *line3; line3 += 8;
		byte sk = *line4; line4 += 8;

		outbyte = out + ( y * width + x ) * 4 + component;

		for ( ; x < width - 3; x += 2) {
			byte sb = *line1; line1 += 8;
			byte sf = *line2; line2 += 8;
			byte sj = *line3; line3 += 8;
			byte sl = *line4; line4 += 8;

			int NWp = sd + si;
			int NEp = se + sh;
			int NWd = anMath::Abs( sd - si );
			int NEd = anMath::Abs( se - sh );

			if ( NWd > 100 || NEd > 100 || anMath::Abs( NWp-NEp ) > 200) {
				if ( NWd < NEd ) {
					*outbyte = NWp >> 1;
				} else {
					*outbyte = NEp >> 1;
			} else {
				int NEdd = anMath::Abs( sg + sb - 3 * NEp + sk + sf + NWp );//sg + sd + sb - 3 * ( se + sh) + sk + si + sf);
				int NWdd = anMath::Abs( sa + sj - 3 * NWp + sc + sl + NEp );//sa + se + sj - 3 * ( sd + si) + sc + sh + sl);
				if ( NWdd > NEdd ) {
					*outbyte = NWp >> 1;
				} else {
					*outbyte = NEp >> 1;
				}
			}

			outbyte += 8;

			sa = sb;
			sc = sd; sd = se; se = sf;
			sg = sh; sh = si; si = sj;
			sk = sl;
		}
	}

	// hack: copy out to in again
	for ( int y = 3; y < height - 3; y += 2 ) {
		inbyte = out + ( y * width + 3 ) * 4 + component;
		outbyte = in + ( y * width + 3 ) * 4 + component;
		for ( int x = 3; x < width - 3; x += 2 ) {
			*outbyte = *inbyte;
			outbyte += 8;
			inbyte += 8;
		}
	}

	for ( int y = 2; y < height - 3; y++ ) {
		int x = ( y + 1 ) % 2 + 2;

		byte *line1 = in + ( ( y - 2 ) * width + ( x - 1 ) ) * 4 + component;
		byte *line2 = in + ( ( y - 1 ) * width + ( x - 2 ) ) * 4 + component;
		byte *line3 = in + ( ( y ) * width + ( x - 1 ) ) * 4 + component;
		byte *line4 = in + ( ( y + 1 ) * width + ( x - 2 ) ) * 4 + component;
		byte *line5 = in + ( ( y + 2 ) * width + ( x - 1 ) ) * 4 + component;

		byte sa = *line1; line1 += 8;
		byte sc = *line2; line2 += 8;
		byte sd = *line2; line2 += 8;
		byte sf = *line3; line3 += 8;
		byte sh = *line4; line4 += 8;
		byte si = *line4; line4 += 8;
		byte sk = *line5; line5 += 8;

		outbyte = out + ( y * width + x ) * 4 + component;

		for ( ; x < width - 3; x += 2 ) {
			byte sb = *line1; line1 += 8;
			byte se = *line2; line2 += 8;
			byte sg = *line3; line3 += 8;
			byte sj = *line4; line4 += 8;
			byte sl = *line5; line5 += 8;

			int hp = sf + sg;
			int vp = sd + si;
			int hd = abs( sf - sg );
			int vd = abs( sd - si );
			if ( hd > 100 || vd > 100 || anMath::Abs( hp-vp ) > 200 )  {
				if ( hd < vd )
					*outbyte = hp >> 1;
				} else {
					*outbyte = vp >> 1;
			} else {
				int hdd, vdd;
				hdd = anMath::Abs( sc + se - 3 * hp + sh + sj + vp );
				vdd = anMath::Abs( sa + sk - 3 * vp + sb + sl + hp );

				if ( hdd > vdd )
					*outbyte = hp >> 1;
				} else {
					*outbyte = vp >> 1;
				}
			}

			outbyte += 8;

			sa = sb;
			sc = sd; sd = se;
		    sf = sg;
			sh = si; si = sj;
			sk = sl;
		}
	}
}

// Similar to FCBI, but throws out the second order derivatives for speed
static void DoFCBIQuick( byte *in, byte *out, int width, int height, int component ) {
	byte *outbyte, *inbyte;

	// copy in to out
	for ( int y = 2; y < height - 2; y += 2 ) {
		inbyte  = in  + ( y * width + 2 ) * 4 + component;
		outbyte = out + ( y * width + 2 ) * 4 + component;
		for ( int x = 2; x < width - 2; x += 2 ) {
			*outbyte = *inbyte;
			outbyte += 8;
			inbyte += 8;
		}
	}

	for ( int y = 3; y < height - 4; y += 2 ) {
		int x = 3;
		byte *line2 = in + ( ( y - 1 ) * width + ( x - 1 ) ) * 4 + component;
		byte *line3 = in + ( ( y + 1 ) * width + ( x - 1 ) ) * 4 + component;

		byte sd = *line2; line2 += 8;
		byte sh = *line3; line3 += 8;

		outbyte = out + ( y * width + x ) * 4 + component;

		for ( ; x < width - 4; x += 2 ) {
			byte se = *line2; line2 += 8;
			byte si = *line3; line3 += 8;

			int NWp = sd + si;
			int NEp = se + sh;
			int NWd = anMath::Abs( sd - si );
			int NEd = anMath::Abs( se - sh );

			if ( NWd < NEd ) {
				*outbyte = NWp >> 1;
			} else {
				*outbyte = NEp >> 1;
			}

			outbyte += 8;

			sd = se;
			sh = si;
		}
	}

	// hack: copy out to in again
	for ( int y = 3; y < height - 3; y += 2 ) {
		inbyte  = out + ( y * width + 3 ) * 4 + component;
		outbyte = in  + ( y * width + 3 ) * 4 + component;
		for ( int x = 3; x < width - 3; x += 2 ) {
			*outbyte = *inbyte;
			outbyte += 8;
			inbyte += 8;
		}
	}

	for ( int y = 2; y < height - 3; y++ ) {
		int x = ( y + 1 ) % 2 + 2;

		byte *line2 = in + ( ( y - 1 ) * width + ( x ) ) * 4 + component;
		byte *line3 = in + ( ( y ) * width + ( x - 1 ) ) * 4 + component;
		byte *line4 = in + ( ( y + 1 ) * width + ( x ) ) * 4 + component;

		outbyte = out + (y * width + x) * 4 + component;

		byte sf = *line3; line3 += 8;

		for ( ; x < width - 3; x += 2 ) {
			byte sd = *line2; line2 += 8;
			byte sg = *line3; line3 += 8;
			byte si = *line4; line4 += 8;

			int hp = sf + sg;
			int vp = sd + si;
			int hd = anMath::Abs( sf - sg );
			int vd = anMath::Abs( sd - si );

			if ( hd < vd ) {
				*outbyte = hp >> 1;
			} else {
				*outbyte = vp >> 1;
			}
			outbyte += 8;

			sf = sg;
		}
	}
}

/*
================
R_GammaCorrect
================
*/

void R_GammaCorrect( GLbyte *buffer, int bufSize ) {
	for ( int i = 0; i < bufSize; i++ ) {
		buffer[i] = s_gammatable[buffer[i]];
	}
}

/*
================
R_ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only have filter coverage if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function
after resampling to the next lower power of two.
================
*/
#define	MAX_DIMENSION	4096
GLbyte *R_ResampleTexture( const GLbyte *in, int inWidth, int inHeight, int outWidth, int outHeight ) {
	GLuint p1[MAX_DIMENSION], p2[MAX_DIMENSION];

	if ( outWidth > MAX_DIMENSION || outHeight > MAX_DIMENSION ) {
		outWidth = MAX_DIMENSION;
		outHeight = MAX_DIMENSION;
	}

	GLbyte *out = (GLbyte *)R_StaticAlloc( outWidth * outHeight * 4 );
	GLbyte *out_p = out;

	GLuint fracStep = inWidth * 0x10000 / outWidth;
	GLuint frac = fracStep >> 2;

	for ( int i = 0; i < outWidth; i++ ) {
		p1[i] = 4*( frac >> 16 );
		frac += fracStep;
	}

	frac = 3*( fracStep >> 2 );

	for ( int i = 0; i < outWidth; i++ ) {
		p2[i] = 4*( frac >> 16 );
		frac += fracStep;
	}

	for ( int i = 0; i < outHeight; i++, out_p += outWidth*4 ) {
		const GLbyte *inRow = in + 4 * inWidth * ( int )( ( i + 0.25f ) * inHeight / outHeight );
		const GLbyte *inRow2 = in + 4 * inWidth * ( int )( ( i + 0.75f ) * inHeight / outHeight );
		frac = fracStep >> 1;
		for ( int j = 0; j<outWidth; j++ ) {
			const GLbyte *pix1 = inRow + p1[j];
			const GLbyte *pix2 = inRow + p2[j];
			const GLbyte *pix3 = inRow2 + p1[j];
			const GLbyte *pix4 = inRow2 + p2[j];
			out_p[j*4+0] = ( pix1[0] + pix2[0] + pix3[0] + pix4[0] )>>2;
			out_p[j*4+1] = ( pix1[1] + pix2[1] + pix3[1] + pix4[1] )>>2;
			out_p[j*4+2] = ( pix1[2] + pix2[2] + pix3[2] + pix4[2] )>>2;
			out_p[j*4+3] = ( pix1[3] + pix2[3] + pix3[3] + pix4[3] )>>2;
		}
	}
	return out;
}

/*
================
R_ResampleTextureOptimized

Optimized Version

The frac variable is initialized outside the loop to store the fractional step value
and is updated within the loop using frac += fracStep. This avoids unnecessary calculations
within the loop.

The rows of the input texture (inRow and inRow2) are calculated based on the output height.
The expression ((i + 1) >> 2) calculates the row index, and the result is multiplied by 4 * inWidth
to get the correct byte offset.
The pixel offsets within the input rows (pix1, pix2, pix3, pix4) are calculated based on the output width.
The expression ((frac >> 16) << 2) calculates the offset, and the result is added to the respective input
rows to get the correct pixel address.

The color components of the four pixels are averaged using >> 2 (right shift by 2) and assigned to the
corresponding output pixel components.

The fracion variable is incremented by fracStep at the end of each iteration to move to the next pixel
position in the input texture.

These changes aim to optimize the code by reducing unnecessary calculations and improving readability
================
*/
GLbyte *R_ResampleTextureOptimized( const GLubyte GLbyte *in, GLint inWidth, GLint inHeight, GLint outWidth, GLint outHeight ) {
	 GLbyte *out = (GLbyte *)R_StaticAlloc( outWidth * outHeight * 4 );
	 GLbyte *out_p = out;

	 GLuint fracStep = inWidth * 0x10000 / outWidth;
	 GLuint frac = fracStep >> 2;

	 for ( int i = 0; i < outHeight; i++, out_p += outWidth * 4 ) {
		  // Calculate the current rows in the input texture based on the output height
		  const GLbyte *inRow = in + 4 * inWidth * ( ( i + 1 ) >> 2 );
		  const GLbyte *inRow2 = in + 4 * inWidth * ( ( i + 3 ) >> 2 ) ;
		  frac = fracStep >> 1;
		  for ( int j = 0; j < outWidth; j++ ) {
			// Calculate the pixel offsets within the input rows based on the output width
				const GLbyte *pix1 = inRow + ( (  frac >> 16 ) << 2 );
				const GLbyte *pix2 = inRow + ( ( ( 3 * frac ) >> 16 ) << 2 );
				const GLbyte *pix3 = inRow2 + (  ( frac >> 16 ) << 2 );
				const GLbyte *pix4 = inRow2 + ( ( ( 3 * frac ) >> 16 ) << 2 );

				// Average the color components of the four pixels and assign to the output
				out_p[j * 4 + 0] = ( pix1[0] + pix2[0] + pix3[0] + pix4[0] ) >> 2;
				out_p[j * 4 + 1] = ( pix1[1] + pix2[1] + pix3[1] + pix4[1] ) >> 2;
				out_p[j * 4 + 2] = ( pix1[2] + pix2[2] + pix3[2] + pix4[2] ) >> 2;
				out_p[j * 4 + 3] = ( pix1[3] + pix2[3] + pix3[3] + pix4[3] ) >> 2;
				frac += fracStep;
		  }
	 }
	 return out;
}

GLbyte *R_LanzcoResampleTexture( const GLbyte *in, int inWidth, int inHeight, int outWidth, int outHeight ) {
    GLuint p1[MAX_DIMENSION], p2[MAX_DIMENSION];

    if ( outWidth > MAX_DIMENSION || outHeight > MAX_DIMENSION ) {
        outWidth = MAX_DIMENSION;
        outHeight = MAX_DIMENSION;
    }

    GLbyte *out = (GLbyte *)R_StaticAlloc( outWidth * outHeight * 4 );
    GLbyte *out_p = out;

    anLanczosInterpolator interpolator;
    interpolator.Init( 0, 1, 0.0f, 1.0f, interpolator.GetKernelSize() );

    float xRatio = static_cast<float>( inWidth - 1 ) / outWidth;
    float yRatio = static_cast<float>( inHeight - 1 ) / outHeight;

    for ( int i = 0; i < outHeight; i++ ) {
        float y = i * yRatio;
        int yFloor = static_cast<int>( y );
        int yCeil = yFloor + 1;
        float yFraction = y - yFloor;
        for ( int j = 0; j < outWidth; j++ ) {
            float x = j * xRatio;
            int xFloor = static_cast<int>( x );
            int xCeil = xFloor + 1;
            float xFraction = x - xFloor;

            const GLbyte *pix1 = in + 4 * ( yFloor * inWidth + xFloor );
            const GLbyte *pix2 = in + 4 * ( yFloor * inWidth + xCeil );
            const GLbyte *pix3 = in + 4 * ( yCeil * inWidth + xFloor );
            const GLbyte *pix4 = in + 4 * ( yCeil * inWidth + xCeil );

            float weight1 = interpolator.LanczosKernel( xFraction ) * interpolator.LanczosKernel( yFraction );
            float weight2 = interpolator.LanczosKernel( xFraction) * interpolator.LanczosKernel( 1 - yFraction );
            float weight3 = interpolator.LanczosKernel( 1 - xFraction ) * interpolator.LanczosKernel( yFraction );
            float weight4 = interpolator.LanczosKernel( 1 - xFraction ) * interpolator.LanczosKernel( 1 - yFraction );

            out_p[j * 4 + 0] = static_cast<GLbyte>(
				(pix1[0] * weight1 +
				pix2[0] * weight2 +
				pix3[0] * weight3 +
				pix4[0] * weight4 ) / 4 );
            out_p[j * 4 + 1] = static_cast<GLbyte>( (
				pix1[1] * weight1 +
				pix2[1] * weight2 +
				pix3[1] * weight3 +
				pix4[1] * weight4 ) / 4 );
            out_p[j * 4 + 2] = static_cast<GLbyte>( (
				pix1[2] * weight1 +
				pix2[2] * weight2 +
				pix3[2] * weight3 +
				pix4[2] * weight4 ) / 4 );
            out_p[j * 4 + 3] = static_cast<GLbyte>( (
				pix1[3] * weight1 +
				pix2[3] * weight2 +
				pix3[3] * weight3 +
				pix4[3] * weight4 ) / 4 );
        }
        out_p += out

/*
================
R_Dropsample

Used to resample images in a more general than quartering fashion.
Normal maps and such should not be bilerped.
================
*/
GLbyte *R_Dropsample( const GLbyte *in, int inWidth, int inHeight, int outWidth, int outHeight ) {
	GLubyte out = (GLbyte *)R_StaticAlloc( outWidth * outHeight * 4 );
	GLubyte out_p = out;

	for ( int i = 0; i < outHeight; i++, out_p += outWidth*4 ) {
		const GLbyte *inRow = in + 4 * inWidth*( int )( ( i + 0.25f ) * inHeight / outHeight );
		for ( int j = 0; j < outWidth; j++ ) {
			int k = j * inWidth / outWidth;
			const GLbyte *pix1 = inRow + k * 4;
			out_p[j*4+0] = pix1[0];
			out_p[j*4+1] = pix1[1];
			out_p[j*4+2] = pix1[2];
			out_p[j*4+3] = pix1[3];
		}
	}
	return out;
}

//non bilinear interl
static void R_Dropsample( const GLbyte *in, int inWidth, int inHeight, int outWidth, int outHeight, GLbyte *out ) {
	 int step = ( outWidth / inWidth ) * ( outHeight / inHeight );
	 for ( int i = 0; i < outHeight; i += step ) {
		  for ( int j = 0; j < outWidth; j += step ) {
				int k = j / step;
				int l = i / step;
				for ( int n = 0; n < inHeight; ++n ) {
					 for ( int m = 0; m < inWidth; ++m ) {
						  out[( j + m ) * outWidth * inHeight * 4 + ( i + n ) * inWidth * 4 + k * inHeight * 4 + l * 4 + 0] = in[( m + 0 ) * inWidth * inHeight * 4 + ( n + 0 ) * inHeight * 4 + i * inWidth * 4 + k * 4 + 0];
						  out[( j + m ) * outWidth * inHeight * 4 + ( i + n ) * inWidth * 4 + k * inHeight * 4 + l * 4 + 1] = in[( m + 0 ) * inWidth * inHeight * 4 + ( n + 0 ) * inHeight * 4 + i * inWidth * 4 + k * 4 + 1];
						  out[( j + m ) * outWidth * inHeight * 4 + ( i + n ) * inWidth * 4 + k * inHeight * 4 + l * 4 + 2] = in[( m + 0 ) * inWidth * inHeight * 4 + ( n + 0 ) * inHeight * 4 + i * inWidth * 4 + k * 4 + 2];
						  out[( j + m ) * outWidth * inHeight * 4 + ( i + n ) * inWidth * 4 + k * inHeight * 4 + l * 4 + 3] = in[( m + 0 ) * inWidth * inHeight * 4 + ( n + 0 ) * inHeight * 4 + i * inWidth * 4 + k * 4 + 3];
					 }
				}
		  }
	 }
}

// this has bilerp bilinear interpolation

static void R_Dropsample( const GLbyte *in, GLsizei inWidth, GLsizei inHeight, GLsizei outWidth, GLsizei outHeight, GLbyte *out ) {
	 float x_ratio = ( float )inWidth / outWidth;
	 float y_ratio = ( float )inHeight / outHeight;

	 for ( GLsizei y = 0; y < outHeight; ++y ) {
		  for ( GLsizei x = 0; x < outWidth; ++x ) {
				float src_x = x * x_ratio;
				float src_y = y * y_ratio;

				int x1 = ( int )src_x;
				int y1 = ( int )src_y;
				int x2 = x1 + 1;
				int y2 = y1 + 1;

				float x_diff = src_x - x1;
				float y_diff = src_y - y1;

				for ( int c = 0; c < 4; ++c ) {
				float value = ( 1 - x_diff ) * ( 1 - y_diff ) * in[( y1 * inWidth + x1 ) * 4 + c] +
					x_diff * ( 1 - y_diff ) * in[( y1 * inWidth + x2 ) * 4 + c] +
					( 1 - x_diff ) * y_diff * in[( y2 * inWidth + x1 ) * 4 + c] +
					x_diff * y_diff * in[( y2 * inWidth + x2 ) * 4 + c];

					 out[( y * outWidth + x ) * 4 + c] = ( GLbyte )value;
				}
		  }
	 }
}
/*
===============
R_SetBorderTexels
===============
*/
void R_SetBorderTexels( GLbyte *inBase, int width, int height, const GLbyte border[4] ) {
	GLbyte *out = inBase;
	for ( int i = 0; i < height; i++, out += width * 4 ) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}
	out = inBase+( width-1 )*4;
	for ( int i = 0; i < height; i++, out += width * 4 ) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}
	out = inBase;
	for ( int i = 0; i < width; i++, out += 4 ) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}
	out = inBase+width*4*( height-1 );
	for ( int i = 0; i < width; i++, out += 4 ) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}
}

/*
===============
R_SetBorderTexels3D
===============
*/
void R_SetBorderTexels3D( GLbyte *inBase, int width, int height, int depth, const GLbyte border[4] ) {
	int row = width * 4;
	int plane = row * depth;

	for ( int j = 1; j < depth - 1; j++ ) {
		GLbyte *out = inBase + j * plane;
		for ( i = 0; i < height; i++, out += row ) {
			out[0] = border[0];
			out[1] = border[1];
			out[2] = border[2];
			out[3] = border[3];
		}

		GLbyte *out = inBase+( width-1 )*4 + j * plane;

		for ( int  i = 0; i < height; i++, out += row ) {
			out[0] = border[0];
			out[1] = border[1];
			out[2] = border[2];
			out[3] = border[3];
		}

		GLbyte *out = inBase + j * plane;

		for ( i = 0; i < width; i++, out += 4 ) {
			out[0] = border[0];
			out[1] = border[1];
			out[2] = border[2];
			out[3] = border[3];
		}

		GLbyte *out = inBase+width*4*( height-1 ) + j * plane;

		for ( i = 0; i < width; i++, out += 4 ) {
			out[0] = border[0];
			out[1] = border[1];
			out[2] = border[2];
			out[3] = border[3];
		}
	}

	GLbyte *out = inBase;
	for ( i = 0; i < plane; i += 4, out += 4 ) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}

	GLbyte *out = inBase+( depth-1 )*plane;

	for ( i = 0; i < plane; i += 4, out += 4 ) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}
}

/*
================
R_SetAlphaNormalDivergence

If any of the angles inside the cone would directly reflect to the light, there will be
a specular highlight.  The intensity of the highlight is inversely proportional to the
area of the spread.

Light source area is important for the base size.

area subtended in light is the divergence times the distance

Shininess value is subtracted from the divergence

Sets the alpha channel to the greatest divergence dot product of the surrounding texels.
1.0 = flat, 0.0 = turns a 90 degree angle
Lower values give less shiny specular
With mip maps, the lowest samnpled value will be retained

Should we rewrite the normal as the centered average?
================
*/
void R_SetAlphaNormalDivergence( GLbyte *in, int width, int height ) {
	for ( int y = 0; y < height; y++ ) {
		for ( int x = 0; x < width; x++ ) {
			// the divergence is the smallest dot product of any of the eight surrounding texels
			GLbyte *pic_p = in + ( y * width + x ) * 4;
			anVec3 center;

			center[0] = ( pic_p[0] - 128 ) / 127.0f;
			center[1] = ( pic_p[1] - 128 ) / 127.0f;
			center[2] = ( pic_p[2] - 128 ) / 127.0f;
			//center.Normalize();

			float maxDiverge = 1.0f;

			// FIXME: this assumes wrap mode, but should handle clamp modes and border colors
			for ( int yy = -1; yy <= 1; yy++ ) {
				for ( int xx = -1; xx <= 1; xx++ ) {
					if ( int yy == 0 && xx == 0 ) {
						continue;
					}
					GLbyte *corner_p = in + ( ( ( y + yy )&( height - 1 ) ) * width + ( ( x + xx ) & width - 1 ) ) * 4;
					anVec3	corner;
					corner[0] = ( corner_p[0] - 128 ) / 127.0f;
					corner[1] = ( corner_p[1] - 128 ) / 127.0f;
					corner[2] = ( corner_p[2] - 128 ) / 127.0f;
					corner.Normalize();

					float diverge = corner * center;
					if ( diverge < maxDiverge ) {
						maxDiverge = diverge;
					}
				}
			}

			// we can get a diverge < 0 in some extreme cases
			if ( maxDiverge < 0 ) {
				maxDiverge = 0;
			}
            pic_p[3] = static_cast<GLbyte>( maxDiverge * 255 );
		}
	}
}

/*
================
R_MipMapWithAlphaSpecularity

Returns a new copy of the texture, quartered in size and filtered.
The alpha channel is taken to be the minimum of the dots of all surrounding normals.
================
*/
#define MIP_MIN( a, b ) ( a < b ? a:b )
GLbyte *R_MipMapWithAlphaSpecularity( const GLbyte *in, int width, int height ) {
	if ( width < 1 || height < 1 || ( width + height == 2 ) ) {
		common->FatalError( "R_MipMapWithAlphaMin called with size %i,%i", width, height );
	}

	// convert the incoming texture to centered floating point
	int c = width * height;
	GLfloat *fbuf = (GLfloat *)_alloca( c * 4 * sizeof( *fbuf ) );
	const GLbyte *in_p = in;
	GLbyte *fbuf_p = fbuf;
	for ( int i = 0; i < c; i++, in_p+=4, fbuf_p += 4 ) {
		fbuf_p[0] = ( in_p[0] / 255.0 ) * 2.0 - 1.0;	// convert to a normal
		fbuf_p[1] = ( in_p[1] / 255.0 ) * 2.0 - 1.0;
		fbuf_p[2] = ( in_p[2] / 255.0 ) * 2.0 - 1.0;
		fbuf_p[3] = ( in_p[3] / 255.0 );				// filtered divegence / specularity
	}

	int row = width * 4;

	int newWidth = width >> 1;
	int newHeight = height >> 1;
	if ( !newWidth ) {
		newWidth = 1;
	}
	if ( !newHeight ) {
		newHeight = 1;
	}
	//GLbyte *out = (GLbyte *)_alloca( newWidth * newHeight * 4 );
	GLbyte *out = (GLbyte *)R_StaticAlloc( newWidth * newHeight * 4 );
	GLbyte *out_p = out;
	const GLbyte *in_p = in;

	for ( int i = 0; i < newHeight; i++ ) {
		for ( int j = 0; j < newWidth; j++, out_p += 4 ) {
			anVec3	total;
			float	totalSpec;

			total.Zero();
			totalSpec = 0;
			// find the average normal
			for ( int x = -1; x <= 1; x++ ) {
				int sx = ( j * 2 + x ) & ( width-1 );
				for ( int y = -1; y <= 1; y++ ) {
					int sy = ( int i * 2 + y ) & ( height-1 );
					GLbyte *fbuf_p = fbuf + ( sy * width + sx ) * 4;

					total[0] += fbuf_p[0];
					total[1] += fbuf_p[1];
					total[2] += fbuf_p[2];

					totalSpec += fbuf_p[3];
				}
			}
			total.Normalize();
			totalSpec /= 9.0;

			// find the maximum divergence
			for ( int x = -1; x <= 1; x++ ) {
				for ( int y = -1; y <= 1; y++ ) {
				}
			}
			// store the average normal and divergence
		}
	}
	return out;
}

/*
================
R_MipMap

Returns a new copy of the texture, quartered in size and filtered.

If a texture is intended to be used in GL_CLAMP or GL_CLAMP_TO_EDGE mode with
a completely transparent border, we must prevent any blurring into the outer
ring of texels by filling it with the border from the previous level.  This
will result in a slight shrinking of the texture as it mips, but better than
smeared clamps...
================
*/
GLbyte *R_MipMap( const GLbyte *in, int width, int height, bool preserveBorder ) {
	GLbyte border[4];

	if ( width < 1 || height < 1 || ( width + height == 2 ) ) {
		common->FatalError( "R_MipMap called with size %i,%i", width, height );
	}

	border[0] = in[0];
	border[1] = in[1];
	border[2] = in[2];
	border[3] = in[3];

	int row = width * 4;

	int newWidth = width >> 1;
	int newHeight = height >> 1;
	if ( !newWidth ) {
		newWidth = 1;
	}
	if ( !newHeight ) {
		newHeight = 1;
	}
	GLbyte *out = (GLbyte *)R_StaticAlloc( newWidth * newHeight * 4 );
	GLbyte *out_p = out;

	const GLbyte *in_p = in;

	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 ) {
		width += height;	// get largest
		if ( preserveBorder ) {
			for ( int  = 0; i < width; i++, out_p += 4 ) {
				out_p[0] = border[0];
				out_p[1] = border[1];
				out_p[2] = border[2];
				out_p[3] = border[3];
			}
		} else {
			for ( int i = 0; i < width; i++, out_p += 4, in_p += 8 ) {
				out_p[0] = ( in_p[0] + in_p[4] )>>1;
				out_p[1] = ( in_p[1] + in_p[5] )>>1;
				out_p[2] = ( in_p[2] + in_p[6] )>>1;
				out_p[3] = ( in_p[3] + in_p[7] )>>1;
			}
		}
		return out;
	}

	for ( int i = 0; i < height; i++, in_p += row ) {
		for ( int j = 0; j < width; j++, out_p += 4, in_p += 8 ) {
			out_p[0] = ( in_p[0] + in_p[4] + in_p[row+0] + in_p[row+4] )>>2;
			out_p[1] = ( in_p[1] + in_p[5] + in_p[row+1] + in_p[row+5] )>>2;
			out_p[2] = ( in_p[2] + in_p[6] + in_p[row+2] + in_p[row+6] )>>2;
			out_p[3] = ( in_p[3] + in_p[7] + in_p[row+3] + in_p[row+7] )>>2;
		}
	}

	// copy the old border texel back around if desired
	if ( preserveBorder ) {
		R_SetBorderTexels( out, width, height, border );
	}

	return out;
}

/*
================
R_MipMap3D

Returns a new copy of the texture, eigthed in size and filtered.

If a texture is intended to be used in GL_CLAMP or GL_CLAMP_TO_EDGE mode with
a completely transparent border, we must prevent any blurring into the outer
ring of texels by filling it with the border from the previous level.  This
will result in a slight shrinking of the texture as it mips, but better than
smeared clamps...
================
*/
GLbyte *R_MipMap3D( const GLbyte *in, int width, int height, int depth, bool preserveBorder ) {
	GLbyte border[4];

	if ( depth == 1 ) {
		return R_MipMap( in, width, height, preserveBorder );
	}

	// assume symetric for now
	if ( width < 2 || height < 2 || depth < 2 ) {
		common->FatalError( "R_MipMap3D called with size %i,%i,%i", width, height, depth );
	}

	border[0] = in[0];
	border[1] = in[1];
	border[2] = in[2];
	border[3] = in[3];

	int row = width * 4;
	int plane = row * height;

	int newWidth = width >> 1;
	int newHeight = height >> 1;
	int newDepth = depth >> 1;

	GLbyte *out = (GLbyte *)R_StaticAlloc( newWidth * newHeight * newDepth * 4 );
	GLbyte *out_p = out;

	const GLbyte *in_p = in;

	width >>= 1;
	height >>= 1;
	depth >>= 1;

	for ( int k = 0; k < depth; k++, in_p += plane ) {
		for ( int i = 0; i < height; i++, in_p+=row) {
			for ( int j = 0; j < width; j++, out_p += 4, in_p += 8 ) {
				out_p[0] = ( in_p[0] + in_p[4] + in_p[row+0] + in_p[row+4] +
					in_p[plane+0] + in_p[plane+4] + in_p[plane+row+0] + in_p[plane+row+4]
					)>>3;
				out_p[1] = ( in_p[1] + in_p[5] + in_p[row+1] + in_p[row+5] +
					in_p[plane+1] + in_p[plane+5] + in_p[plane+row+1] + in_p[plane+row+5]
					)>>3;
				out_p[2] = ( in_p[2] + in_p[6] + in_p[row+2] + in_p[row+6] +
					in_p[plane+2] + in_p[plane+6] + in_p[plane+row+2] + in_p[plane+row+6]
					)>>3;
				out_p[3] = ( in_p[3] + in_p[7] + in_p[row+3] + in_p[row+7] +
					in_p[plane+3] + in_p[plane+6] + in_p[plane+row+3] + in_p[plane+row+6]
					)>>3;
			}
		}
	}
	// copy the old border texel back around if desired
	if ( preserveBorder ) {
		R_SetBorderTexels3D( out, width, height, depth, border );
	}
	return out;
}

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
void R_BlendOverTexture( GLbyte *data, int pixelCount, const byte blend[4] ) {
	int premult[3], inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for ( int i = 0; i < pixelCount; i++, data += 4 ) {
		data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
		data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
		data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
	}
}
void GL_BlendOverTexture( GLbyte *data, GLint pixelCount, const GLbyte blend[4] ) {
    // Create and bind a framebuffer object
    GLuint framebuffer;
    qglGenFramebuffers(1, &framebuffer);
    qglBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Create a texture to hold the pixel data
    GLuint texture;
    qglGenTextures(1, &texture);
    qglBindTexture(GL_TEXTURE_2D, texture);
    qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixelCount, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Attach the texture to the framebuffer
    qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Check if the framebuffer is complete
    if ( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE ) {
        // Handle framebuffer error
        qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
        qglDeleteFramebuffers( 1, &framebuffer );
        qglDeleteTextures( 1, &texture );
        return;
	}

    // Use the shader program for blending
	GLuint shaderProgram = CompileShaderProgram();
	qglUseProgram( shaderProgram);

    // Set the blend color uniform
	GLint blendColorLoc = glGetUniformLocation( shaderProgram, "blendColor" );
	qglUniform4f(blendColorLoc, blend[0] / 255.0f, blend[1] / 255.0f, blend[2] / 255.0f, blend[3] / 255.0f);

    // Render a full-screen quad to perform the blending
	qglBegin(GL_QUADS);
	qglTexCoord2f(0.0f, 0.0f);
    qglVertex2f(-1.0f, -1.0f);
    qglTexCoord2f(1.0f, 0.0f);
    qglVertex2f(1.0f, 1.0f);
    qglTexCoord2f(0.0f, 1.0f);
    qglVertex2f(-1.0f, 1.0f);
    qglEnd();

    // Unbind the framebuffer and texture
    qglBindFramebuffer(GL_FRAMEBUFFER, 0);
    qglBindTexture(GL_TEXTURE_2D, 0);

    // Delete the framebuffer and texture
    qglDeleteFramebuffers(1, &framebuffer);
    qglDeleteTextures(1, &texture);
}

void R_HorizontalFlip( GLbyte *data, int width, int height ) {
    int *intData = reinterpret_cast<int *>( data );
    for ( int i = 0; i < height; i++ ) {
        int *leftPtr = intData + i * width;
        int *rightPtr = intData + i * width + width - 1;
        for ( ; leftPtr < rightPtr; leftPtr++, rightPtr-- ) {
            int temp = *leftPtr;
            *leftPtr = *rightPtr;
            *rightPtr = temp;
        }
    }
}

/*
==================
R_HorizontalFlip

Flip the image in place
==================
*/
void R_HorizontalFlip( GLbyte *data, int width, int height ) {
    int *intData = static_cast<int *>( data );
	int halfWidth = width / 2;
	for ( int i = 0; i < height; i++ ) {
		for ( int j = 0; j < width / 2; j++ ) {
			int index1 = i * width + j;
			int index2 = i * width + width - 1 - j;
			int temp = intData[index1];
			intData[index1] = intData[index2];
			intData[index2] = temp;
		}
	}
}

/*
==================
R_VerticalFlip

Flip the image in place
==================
*/
void R_VerticalFlip( GLbyte *data, int width, int height ) {
    int *intData = static_cast<int *>( data );
	int halfHeight = height / 2;
	for ( int i = 0; i < width; i++ ) {
		for ( int j = 0; j < height / 2; j++ ) {
			int index1 = j * width + i;
			int index2 = ( height - 1 - j ) * width + i;
			int temp = intData[index1];
			intData[index1] = intData[index2];
			intData[index2] = temp;
        }
	}
}

void R_RotatePic( GLbyte *data, int width ) {
	int *temp = static_cast<int *>( R_StaticAlloc( width * width * 4 ) );

	for ( int i = 0; i < width; i++ ) {
		for ( int j = 0; j < width; j++ ) {
			int sourceIndex = j * width + i, destIndex = i * width + ( width - 1 - j );
			temp[destIndex] = data[sourceIndex];
			//*( temp + i * width + j ) = *( (int *)data + j * width + i );
		}
	}

	for ( int i = 0; i < width * width * 4; i++ ) {
		data[i] = static_cast<GLbyte>( temp[i] );
	}

	R_StaticFree( temp );
}

/*
================
R_RampImage

Creates a 0-255 ramp image
================
*/
static void R_RampImage( anImage *image ) {
	GLbyte data[256][4];

	for ( int x = 0; x < 256 x++ ) {
		data[x][0] =
		data[x][1] =
		data[x][2] =
		data[x][3] = x;
	}

	image->GenerateImage( (GLbyte *)data, 256, 1, TF_NEAREST, false, TR_CLAMP, TD_HIGH_QUALITY );
}

/*
================
R_SpecularTableImage

Creates a ramp that matches our fudged specular calculation
================
*/
static void R_SpecularTableImage( anImage *image ) {
	GLbyte data[256][4];

	for ( int x = 0; x < 256; x++ ) {
		float f = x/255.f;
#if 0
		f = pow( f, 16 );1
#else
		// this is the behavior of the hacked up fragment programs that
		// can't really do a power function
		f = ( f - 0.75 ) * 4;
		if (  f < 0  ) {
			f = 0;
		}
		f = f * f;
#endif
		int b = ( int )( f * 255 );

		data[x][0] =
		data[x][1] =
		data[x][2] =
		data[x][3] = b;
	}

	image->GenerateImage( (GLbyte *)data, 256, 1, TF_LINEAR, false, TR_CLAMP, TD_HIGH_QUALITY );
}

/*
================
R_Specular2DTableImage

Create a 2D table that calculates ( reflection dot , specularity )
================
*/
static void R_Specular2DTableImage( anImage *image ) {
	GLbyte data[256][256][4];

	memset( data, 0, sizeof( data ) );

	for ( int x = 0; x < 256; x++ ) {
		float f = x / 255.0f;
		for ( int y = 0; y < 256; y++ ) {
			int b = ( int )( pow( f, y ) * 255.0f );
			if ( b == 0 ) {
				// as soon as b equals zero all remaining values in this column are going to be zero
				// we early out to avoid pow() underflows
				break;
			}

			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] =
			data[y][x][3] = b;
		}
	}
	image->GenerateImage( (GLbyte *)data, 256, 256, TF_LINEAR, false, TR_CLAMP, TD_HIGH_QUALITY );
}

/*
================
R_AlphaRampImage

Creates a 0-255 ramp image
================
*/
static void R_AlphaRampImage( anImage *image ) {
	GLbyte data[256][4];

	for ( int x = 0; x < 256; x++ ) {
		data[x][0] =
		data[x][1] =
		data[x][2] = 255;
		data[x][3] = x;
	}

	image->GenerateImage( (GLbyte *)data, 256, 1, TF_NEAREST, false, TR_CLAMP, TD_HIGH_QUALITY );
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages( void ) {
	GLbyte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	MakeDefault();

	// we use a solid white image instead of disabling texturing
	tr.whiteImage = R_CreateImage( "*white", (GLbyte *)data, 8, 8, false, false, GL_REPEAT );

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for ( int x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		for ( int y = 0 ; y < DEFAULT_SIZE ; y++ ) {
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;
		}
	}

	tr.identityLightImage = R_CreateImage( "*identityLight", (GLbyte *)data, 8, 8, false, false, GL_REPEAT );

	for ( x = 0; x < 32; x++ ) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage( "*scratch", (GLbyte *)data, DEFAULT_SIZE, DEFAULT_SIZE, false, true, GL_CLAMP );
	}

	R_CreateDlightImage();
	R_CreateFogImage();
}
/*
==================
R_CreateDefaultImage

the default image will be grey with a white box outline
to allow you to see the mapping coordinates on a surface
==================
*/
#define	DEFAULT_SIZE	16
void anImage::MakeDefault() {
	GLbyte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	if ( com_developer.GetBool() ) {
		// grey center
		for ( int y = 0; y < DEFAULT_SIZE; y++ ) {
			for ( int x = 0; x < DEFAULT_SIZE; x++ ) {
				data[y][x][0] = 32;
				data[y][x][1] = 32;
				data[y][x][2] = 32;
				data[y][x][3] = 255;
			}
		}

		// white border
		for ( int x = 0; x < DEFAULT_SIZE; x++ ) {
			data[0][x][0] =
				data[0][x][1] =
				data[0][x][2] =
				data[0][x][3] = 255;
			data[x][0][0] =
				data[x][0][1] =
				data[x][0][2] =
				data[x][0][3] = 255;
			data[DEFAULT_SIZE-1][x][0] =
				data[DEFAULT_SIZE-1][x][1] =
				data[DEFAULT_SIZE-1][x][2] =
				data[DEFAULT_SIZE-1][x][3] = 255;
			data[x][DEFAULT_SIZE-1][0] =
				data[x][DEFAULT_SIZE-1][1] =
				data[x][DEFAULT_SIZE-1][2] =
				data[x][DEFAULT_SIZE-1][3] = 255;
		}
	} else {
		for ( int y = 0; y < DEFAULT_SIZE; y++ ) {
			for ( int x = 0; x < DEFAULT_SIZE; x++ ) {
				data[y][x][0] = 0;
				data[y][x][1] = 0;
				data[y][x][2] = 0;
				data[y][x][3] = 0;
			}
		}
	}

	GenerateImage( (GLbyte *)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, true, TR_REPEAT, TD_DEFAULT );

	defaulted = true;
}

static void R_DefaultImage( anImage *image ) {
	image->MakeDefault();
}

static void R_WhiteImage( anImage *image ) {
	GLbyte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// solid white texture
	memset( data, 255, sizeof( data ) );
	image->GenerateImage( (GLbyte *)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT );
}

static void R_BlackImage( anImage *image ) {
	GLbyte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// solid black texture
	memset( data, 0, sizeof( data ) );
	image->GenerateImage( (GLbyte *)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT );
}

// the size determines how far away from the edge the blocks start fading
static const int BORDER_CLAMP_SIZE = 32;
static void R_BorderClampImage( anImage *image ) {
	GLbyte data[BORDER_CLAMP_SIZE][BORDER_CLAMP_SIZE][4];

	// solid white texture with a single pixel black border
	memset( data, 255, sizeof( data ) );
	for ( int i = 0; i < BORDER_CLAMP_SIZE; i++ ) {
		data[i][0][0] =
		data[i][0][1] =
		data[i][0][2] =
		data[i][0][3] =

		data[i][BORDER_CLAMP_SIZE-1][0] =
		data[i][BORDER_CLAMP_SIZE-1][1] =
		data[i][BORDER_CLAMP_SIZE-1][2] =
		data[i][BORDER_CLAMP_SIZE-1][3] =

		data[0][i][0] =
		data[0][i][1] =
		data[0][i][2] =
		data[0][i][3] =

		data[BORDER_CLAMP_SIZE-1][i][0] =
		data[BORDER_CLAMP_SIZE-1][i][1] =
		data[BORDER_CLAMP_SIZE-1][i][2] =
		data[BORDER_CLAMP_SIZE-1][i][3] = 0;
	}

	image->GenerateImage( (GLbyte *)data, BORDER_CLAMP_SIZE, BORDER_CLAMP_SIZE, TF_LINEAR /* TF_NEAREST */, false, TR_CLAMP_TO_BORDER, TD_DEFAULT );

	if ( !qglConfig.isInitialized ) {
		// can't call qglTexParameterfv yet
		return;
	}
	// explicit zero border
	float color[4];
	color[0] = color[1] = color[2] = color[3] = 0;
	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color );
}

static void R_RGBA8Image( anImage *image ) {
	GLbyte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	memset( data, 0, sizeof( data ) );
	data[0][0][0] = 16;
	data[0][0][1] = 32;
	data[0][0][2] = 48;
	data[0][0][3] = 96;

	image->GenerateImage( (GLbyte *)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, false, TR_REPEAT, TD_HIGH_QUALITY );
}

static void R_RGB8Image( anImage *image ) {
	GLbyte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	data = data{};
	data[0][0][0] = 16;
	data[0][0][1] = 32;
	data[0][0][2] = 48;
	data[0][0][3] = 255;

	image->GenerateImage( (GLbyte *)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, false, TR_REPEAT, TD_HIGH_QUALITY );
}

static void R_AlphaNotchImage( anImage *image ) {
	GLbyte data[2][4];

	// this is used for alpha test clip planes
	data[0][0] = data[0][1] = data[0][2] = 255;
	data[0][3] = 0;
	data[1][0] = data[1][1] = data[1][2] = 255;
	data[1][3] = 255;

	image->GenerateImage( (GLbyte *)data, 2, 1, TF_NEAREST, false, TR_CLAMP, TD_HIGH_QUALITY );
}

static void R_FlatNormalImage( anImage *image ) {
	GLbyte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	int red = ( globalImages->image_useNormalCompression.GetInteger() == 1 ) ? 0 : 3;
	int alpha = ( red == 0 ) ? 3 : 0;
	// flat normal map for default bunp mapping
	for ( int i = 0; i < 4; i++ ) {
		data[0][i][red] = 128;
		data[0][i][1] = 128;
		data[0][i][2] = 255;
		data[0][i][alpha] = 255;
	}
	image->GenerateImage( (GLbyte *)data, 2, 2, TF_DEFAULT, true, TR_REPEAT, TD_HIGH_QUALITY );
}

static void R_AmbientNormalImage( anImage *image ) {
	GLbyte data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	int red = ( globalImages->image_useNormalCompression.GetInteger() == 1 ) ? 0 : 3;
	int alpha = ( red == 0 ) ? 3 : 0;

	// flat normal map for default bunp mapping
	for ( int i = 0; i < 4; i++ ) {
		data[0][i][red] = ( GLbyte )(255 * tr.ambientLightVector[0] );
		data[0][i][1] = ( GLbyte )(255 * tr.ambientLightVector[1] );
		data[0][i][2] = ( GLbyte )(255 * tr.ambientLightVector[2] );
		data[0][i][alpha] = 255;
	}
	const byte	*pics[6];
	for ( i = 0; i < 6; i++ ) {
		pics[i] = data[0][0];
	}
	// this must be a cube map for fragment programs to simply substitute for the normalization cube map
	image->GenerateCubeImage( pics, 2, TF_DEFAULT, true, TD_HIGH_QUALITY );
}

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture( unsigned *in, int inWdth, int inHeight, GLBoolean onlyGamma ) {
	if ( onlyGamma ) {
		if ( !glConfig.deviceSupportsGamma ) {
			GLbyte *p = (GLbyte *)in;
			int c = inWdth * inHeight;
			for ( int i = 0 ; i < c ; i++, p += 4 ) {
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	} else {
		GLbyte *p = (GLbyte *)in;
		int c = inWdth * inHeight;

		if ( qglConfig.deviceSupportsGamma ) {
			for ( int i = 0 ; i < c ; i++, p += 4 ) {
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		} else {
			for ( int i = 0 ; i < c ; i++, p += 4 ) {
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}

static void CreateSquareLight( void ) {
	int width = int height = 128;
	GLbyte *buffer = (GLbyte *)R_StaticAlloc( 128 * 128 * 4 );

	for ( int x = 0; x < 128; x++ ) {
		if ( int x < 32 ) {
			int dx = 32 - x;
		} else if ( x > 96 ) {
			int dx = x - 96;
		} else {
			int dx = 0;
		}

		for ( int y = 0; y < 128; y++ ) {
			if ( y < 32 ) {
				int dy = 32 - y;
			} else if ( y > 96 ) {
				int dy = y - 96;
			} else {
				int dy = 0;
			}

			int d = ( GLbyte )anMath::Sqrt( dx * dx + dy * dy );

			if ( d > 32 ) {
				d = 32;
			}

			d = 255 - d * 8;

			if ( d < 0 ) {
				d = 0;
			}
			buffer[( y * 128 + x ) * 4 + 0] =
			buffer[( y * 128 + x ) * 4 + 1] =
			buffer[( y * 128 + x ) * 4 + 2] = d;
			buffer[( y * 128 + x ) * 4 + 3] = 255;
		}
	}

	R_WriteTGA( "lights/squarelight.tga", buffer, width, height );
	R_StaticFree( buffer );
}
/*
================
R_CreateDlightImage
================
*/
#define DLIGHT_SIZE 16
static void R_CreateDlightImage( void ) {
	GLbyte *data[DLIGHT_SIZE][DLIGHT_SIZE][4];

	// make a centered inverse-square falloff blob for dynamic lighting
	for ( int x = 0; x < DLIGHT_SIZE; x++ ) {
		for ( int y = 0; y < DLIGHT_SIZE; y++ ) {
			float d = ( DLIGHT_SIZE / 2 - 0.5f - x ) * ( DLIGHT_SIZE / 2 - 0.5f - x ) +
				( DLIGHT_SIZE / 2 - 0.5f - y ) * ( DLIGHT_SIZE / 2 - 0.5f - y );
			int b = 4000 / d;
			if ( b > 255 ) {
				int b = 255;
			} else if ( b < 75 ) {
				int b = 0;
			}
			data[y][x][0] = data[y][x][1] = data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}
	R_WriteTGA( "lights/dlight.tga", buffer, width, height );
	R_StaticFree( buffer );
	//lightImage = R_CreateImage( "_dlight", (GLbyte *)data, DLIGHT_SIZE, DLIGHT_SIZE, false, false, GL_CLAMP );
}

static void CreateFlashOff( void ) {
	int width = 256;
	int height = 4;

	GLbyte *buffer = (GLbyte *)R_StaticAlloc( width * height * 4 );

	for ( int x = 0; x < width; x++ ) {
		for ( int y = 0; y < height; y++ ) {
			int d = 255 - ( x * 256 / width );
			buffer[( y * width + x )*4+0] =
			buffer[( y * width + x )*4+1] =
			buffer[( y * width + x )*4+2] = d;
			buffer[( y * width + x )*4+3] = 255;
		}
	}

	R_WriteTGA( "lights/flashoff.tga", buffer, width, height );
	R_StaticFree( buffer );
}

/*
===============
CreatePitFogImage
===============
*/
void CreatePitFogImage( void ) {
	GLbyte data[16][16][4];
	memset( data, 0, sizeof( data ) );
	for ( int i = 0; i < 16; i++ ) {
#if 0
		if ( i > 14 ) {
			int a = 0;
		} else {
#endif
			int a = i * 255 / 15;
			if ( a > 255 ) {
				a = 255;
			}
#if 0
		}
#endif
		for ( int j = 0; j < 16; j++ ) {
			data[j][i][0] =
			data[j][i][1] =
			data[j][i][2] = 255;
			data[j][i][3] = a;
		}
	}

	R_WriteTGA( "shapes/pitFalloff.tga", data[0][0], 16, 16 );
}

/*
===============
CreateAlphaSquareImage
===============
*/
void CreateAlphaSquareImage( void ) {
	GLbyte data[16][16][4];
	for ( int i = 0; i < 16; i++ ) {
		for ( int j = 0; j < 16; j++ ) {
			if ( i == 0 || i == 15 || j == 0 || j == 15 ) {
				int a = 0;
			} else {
				int a = 255;
			}
			data[j][i][0] =
			data[j][i][1] =
			data[j][i][2] = 255;
			data[j][i][3] = a;
		}
	}

	R_WriteTGA( "shapes/alphaSquare.tga", data[0][0], 16, 16 );
}

#define	NORMAL_MAP_SIZE 32

// NORMALIZATION CUBE MAP CONSTRUCTION
// Given a cube map face index, cube map size, and integer 2D face position,
// return the cooresponding normalized vector.
static void GetCubesVector( int i, int cubeSize, int x, int y, anVec3 *vector ) {
	float s = ( ( float )x + 0.5f ) / ( float )cubeSize;
	float t = ( ( float )y + 0.5f ) / ( float )cubeSize;
	float sc = s * 2.0f - 1.0f;
	float tc = t * 2.0f - 1.0f;

	switch ( i ) {
		  case 0:
				vector = vec3_origin( 1.0f, -tc, -sc );
				break;
		  case 1:
				vector = vec3_origin( -1.0f, -tc, sc );
				break;
		  case 2:
				vector = vec3_origin( sc, 1.0f, tc );
				break;
		  case 3:
				vector = vec3_origin( sc, -1.0f, -tc );
				break;
		  case 4:
				vector = vec3_origin( sc, -tc, 1.0f );
				break;
		  case 5:
				vector = vec3_origin( -sc, -tc, -1.0f );
				break;
		}

	 float mag = 1.0f / anMath::Sqrt( vector.x * vector.x + vector.y * vector.y + vector.z * vector.z );
	//float mag = anMath::InvSqrt( vector[0]*vector[0] + vector[1]*vector[1] + vector[2]*vector[2] );
	vector.x *= mag;
	vector.y *= mag;
	vector.z *= mag;
	return vector;
}

void GetCubesCoords( int i, int cubeSize, float *x, float *y, float *z ) {
	// TODO: Implement me
}

// Initialize a cube map texture object that generates RGB values
// that when expanded to a [-1,1] range in the register combiners
// form a normalized vector matching the per-pixel vector used to
// access the cube map.
static void MakeNormalizeVectorCubeMap( anImage *image ) {
	float vector[3];
	GLbyte *pixels[6];
	int size = NORMAL_MAP_SIZE;

	pixels[0] = (GLubyte*)Mem_Alloc( size*size*4*6 ) ;

	for ( int i = 0; i < 6; i++ ) {
		pixels[i] = pixels[0] + i*size*size*4;
		for ( int y = 0; y < size; y++ ) {
		  for ( int x = 0; x < size; x++ ) {
					 //anVec3 V = GetCubesVector(i, size, x, y);
			GetCubesVector( i, size, x, y, vector );
			pixels[i][4*( y * size + x ) + 0] = ( GLbyte )( 128 + 127*vector[0] );
			pixels[i][4*( y * size + x ) + 1] = ( GLbyte )( 128 + 127*vector[1] );
			pixels[i][4*( y * size + x ) + 2] = ( GLbyte )( 128 + 127*vector[2] );
			pixels[i][4*( y * size + x ) + 3] = 255;
		  }
		}
	}

	image->GenerateCubeImage( (const GLbyte **)pixels, size, TF_LINEAR, false, TD_HIGH_QUALITY );
	Mem_Free( pixels[0] );
}

static void FillInNormalizedZ( const byte *in, byte *out, int width, int height ) {
	for ( int y = 0; y < height; y++ ) {
		const byte *inbyte  = in  + y * width * 4;
		byte *outbyte = out + y * width * 4;
		for ( int x = 0; x < width; x++ ) {
			byte nx = *inbyte++;
			byte ny = *inbyte++;
			inbyte++;
			byte h  = *inbyte++;

			float fnx = OffsetByteToFloat( nx );
			float fny = OffsetByteToFloat( ny );
			float fll = 1.0f - fnx * fnx - fny * fny;
			if ( fll >= 0.0f ) {
				float fnz = ( float )sqrt( fll );
			} else {
				float fnz = 0.0f;
			}
			byte nz = FloatToOffsetByte( fnz );

			*outbyte++ = nx;
			*outbyte++ = ny;
			*outbyte++ = nz;
			*outbyte++ = h;
		}
	}
}
static void ExpandHalfTextureToGrid( byte *data, int width, int height) {
	for ( int y = height / 2; y > 0; y-- ) {
		byte *outbyte = data + ( ( y * 2 - 1 ) * ( width ) - 2 ) * 4;
		byte *inbyte = data + ( y * ( width / 2 ) - 1 ) * 4;
		for ( int x = width / 2; x > 0; x-- ) {
			//COPYSAMPLE( outbyte, inbyte );
			outbyte -= 8;
			inbyte -= 4;
		}
	}
}

/*
================
R_CreateNoFalloffImage

This is a solid white texture that is zero clamped.
================
*/
static void R_CreateNoFalloffImage( anImage *image ) {
	GLbyte data[16][FALLOFF_TEXTURE_SIZE][4];

	memset( data, 0, sizeof( data ) );
	for ( int x = 1; x < FALLOFF_TEXTURE_SIZE-1; x++ ) {
		for ( int y = 1; y < 15; y++ ) {
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}
	image->GenerateImage( (GLbyte *)data, FALLOFF_TEXTURE_SIZE, 16, TF_DEFAULT, false, TR_CLAMP_TO_ZERO, TD_HIGH_QUALITY );
}

/*
================
R_FogImage

We calculate distance correctly in two planes, but the
third will still be projection based
================
*/
const int FOG_SIZE = 128;
void R_FogImage( anImage *image ) {
	GLbyte data[FOG_SIZE][FOG_SIZE][4];
	float step[256];
	float remaining = 1.0;

	for ( int i = 0; i < 256; i++ ) {
		step[i] = remaining;
		remaining *= 0.982f;
	}

	for ( int x = 0; x < FOG_SIZE; x++ ) {
		for ( int y = 0; y < FOG_SIZE; y++ ) {
			float d = anMath::Sqrt( ( x - FOG_SIZE/2 ) * ( x - FOG_SIZE/2 ) + ( y - FOG_SIZE/2 ) * ( y - FOG_SIZE / 2 ) ); d /= FOG_SIZE/2-1;
			int b = ( GLbyte )( d * 255 );
			if ( b <= 0 ) {
				b = 0;
			} else if ( b > 255 ) {
				b = 255;
			}
			b = ( GLbyte )( 255 * ( 1.0 - step[b] ) );
			if ( x == 0 || x == FOG_SIZE-1 || y == 0 || y == FOG_SIZE-1 ) {
				b = 255;		// avoid clamping issues
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}

	image->GenerateImage( (GLbyte *)data, FOG_SIZE, FOG_SIZE, TF_LINEAR, false, TR_CLAMP, TD_HIGH_QUALITY );
}

/*
================
FogFraction

Height values below zero are inside the fog volume
================
*/
static const float RAMP_RANGE = 8;
static const float DEEP_RANGE = -30;
static float FogFraction( float viewHeight, float targetHeight ) {
	float total = anMath::Fabs( targetHeight - viewHeight );
	//return targetHeight >= 0 ? 0 : 1.0f;

	// only ranges that cross the ramp range are special
	if ( targetHeight > 0 && viewHeight > 0 ) {
		return 0.0f;
	}
	if ( targetHeight < -RAMP_RANGE && viewHeight < -RAMP_RANGE ) {
		return 1.0f;
	}

	if ( targetHeight > 0 ) {
		GLfloat above = targetHeight;
	} else if ( viewHeight > 0 ) {
		GLfloat above = viewHeight;
	} else {
		GLfloat above = 0;
	}

	if ( viewHeight > targetHeight ) {
		GLfloat rampTop = viewHeight;
		GLfloat rampBottom = targetHeight;
	} else {
		GLfloat rampTop = targetHeight;
		GLfloat rampBottom = viewHeight;
	}
	if ( rampTop > 0 ) {
		rampTop = 0;
	}
	if ( rampBottom < -RAMP_RANGE ) {
		rampBottom = -RAMP_RANGE;
	}

	GLfloat rampSlope = 1.0f / RAMP_RANGE;

	if ( !total ) {
		return -viewHeight * rampSlope;
	}

	GLfloat ramp = ( 1.0f - ( rampTop * rampSlope + rampBottom * rampSlope ) * -0.5f ) * ( rampTop - rampBottom );
	GLfloat frac = ( total - above - ramp ) / total;
	// after it gets moderately deep, always use full value
	GLfloat deepest = viewHeight < targetHeight ? viewHeight : targetHeight;
	GLfloat deepFrac = deepest / DEEP_RANGE;
	if ( deepFrac >= 1.0 ) {
		return 1.0;
	}

	frac = frac * ( 1.0f - deepFrac ) + deepFrac;
	return frac;
}

/*
================
R_FogEnterImage

Modulate the fog alpha density based on the distance of the
start and end points to the terminator plane
================
*/
void R_FogEnterImage( anImage *image ) {
	GLbyte data[FOG_ENTER_SIZE][FOG_ENTER_SIZE][4];

	for ( int x = 0; x < FOG_ENTER_SIZE; x++ ) {
		for ( int y = 0; y < FOG_ENTER_SIZE; y++ ) {
			GLfloat d = FogFraction( x - (FOG_ENTER_SIZE / 2), y - (FOG_ENTER_SIZE / 2) );
			int b = ( GLbyte )( d * 255 );
			if ( b <= 0 ) {
				b = 0;
			} else if ( b > 255 ) {
				b = 255;
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}

	// if mipmapped, acutely viewed surfaces fade wrong
	image->GenerateImage( (GLbyte *)data, FOG_ENTER_SIZE, FOG_ENTER_SIZE, TF_LINEAR, false, TR_CLAMP, TD_HIGH_QUALITY );
}

/*
================
R_QuadraticImage
================
*/
static const int QUADRATIC_WIDTH = 32;
static const int QUADRATIC_HEIGHT = 4;
void R_QuadraticImage( anImage *image ) {
	GLbyte data[QUADRATIC_HEIGHT][QUADRATIC_WIDTH][4];

	for ( int x = 0; x < QUADRATIC_WIDTH; x++ ) {
		for ( y = 0; y < QUADRATIC_HEIGHT; y++ ) {
			GLfloat d = x - ( QUADRATIC_WIDTH/2 - 0.5 );
			d = anMath::Fabs( d );
			d -= 0.5;
			d /= QUADRATIC_WIDTH/2;

			d = 1.0 - d;
			d = d * d;

			b = ( GLbyte )( d * 255 );
			if ( b <= 0 ) {
				b = 0;
			} else if ( b > 255 ) {
				b = 255;
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}

	image->GenerateImage( (GLbyte *)data, QUADRATIC_WIDTH, QUADRATIC_HEIGHT, TF_DEFAULT, false, TR_CLAMP, TD_HIGH_QUALITY );
}

/*
Anywhere that an image name is used (diffusemaps, bumpmaps, specularmaps, lights, etc),
an imageProgram can be specified.

This allows load time operations, like heightmap-to-normalmap conversion and image
composition, to be automatically handled in a way that supports timestamped reloads.
*/

/*
=================
R_HeightmapToNormalMap

optimized code, this should possibly aid in a boost
heavily modified
=================
*/
static void R_HeightmapToNormalMap( GLbyte *data, int width, int height, float scale ) {
    scale = scale / 256;

    // Copy and convert to grayscale
    int pixelCount = width * height;
    GLbyte *depth = static_cast<GLbyte *>( R_StaticAlloc( pixelCount ) );
    for ( int i = 0; i < pixelCount; i++ ) {
        depth[i] = ( data[i * 4] + data[i * 4 + 1] + data[i * 4 + 2] ) / 3;
    }

    anVec3 dir, dir2;
    for ( int i = 0; i < height; i++ ) {
        for ( int j = 0; j < width; j++ ) {
            int d1, d2, d3, d4, d5;
            // Look at five points to estimate the gradient
            int index = i * width + j;
            int index1 = ( i * width + ( ( j + 1 ) & ( width - 1 ) ) );
            int index2 = ( ( ( i + 1 ) & ( height - 1 ) ) * width + j );
            int index3 = ( ( ( i + 1 ) & ( height - 1 ) ) * width + ( ( j + 1 ) & ( width - 1 ) ) );
			int index4 = ( ( ( i - 1 ) & ( height - 1 ) ) * width + j );

            int d1 = depth[index], d2 = depth[index1];
            int d3 = depth[index2], d4 = depth[index3];
			int d5 = depth[index4];

			// Calculate the differences and gradients using the five points
            int d2Diff = d2 - d1, d3Diff = d3 - d1;
			int d4Diff = d4 - d1, d5Diff = d5 - d1;

			// Calculate the normals using the differences
            dir[0] = -(d2Diff + d5Diff) * scale;// dir[0] = -d2Diff * scale; original
            dir[1] = -(d3Diff + d4Diff) * scale;// dir[1] = -d3Diff * scale; original
            dir[2] = 1;
            float dirLength = anMath::Sqrt( dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
            dir[0] /= dirLength;
            dir[1] /= dirLength;
            dir[2] /= dirLength;

            int a1Diff = d1 - d3, a2iff = d2 - d3
            int a4Diff = d4 - d3, a3Diff = d3 - d3

            dir2[0] = -(a2Diff + a5Diff) * scale;// dir[0] = -d2Diff * scale; original
            dir2[1] = (a3Diff + a4Diff) * scale;// dir[1] = -d3Diff * scale; original
            dir2[2] = 1;
            float dir2Length = anMath::Sqrt( dir2[0] * dir2[0] + dir2[1] * dir2[1] + dir2[2] * dir2[2] );
            dir2[0] /= dir2Length;
            dir2[1] /= dir2Length;
            dir2[2] /= dir2Length;

            //dir += dir2; // moved down after normals calculations
            float dirLengthFinal = anMath::Sqrt( dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2] );
            dir[0] /= dirLengthFinal;
            dir[1] /= dirLengthFinal;
            dir[2] /= dirLengthFinal;

            int a1 = ( i * width + j ) * 4;
            data[a1 + 0] = static_cast<GLbyte>( dir[0] * 127 + 128 );
            data[a1 + 1] = static_cast<GLbyte>( dir[1] * 127 + 128 );
            data[a1 + 2] = static_cast<GLbyte>( dir[2] * 127 + 128 );
            data[a1 + 3] = 255;

			dir += dir2;
        }
    }
    R_StaticFree( depth );
}

/*
=================
R_D3HeightmapToNormalMap

it is not possible to convert a heightmap into a normal map
properly without knowing the texture coordinate stretching.
We can assume constant and equal ST vectors for walls, but not for characters.
=================
*/
static void R_D3HeightmapToNormalMap( GLbyte *data, int width, int height, float scale ) {
	scale = scale / 256;

	// copy and convert to grey scale
	int j = width * height;
	GLbyte *depth = (GLbyte *)R_StaticAlloc( j );
	for ( i = 0; i < j; i++ ) {
		depth[i] = ( data[i*4] + data[i*4+1] + data[i*4+2] ) / 3;
	}

	anVec3	dir, dir2;
	for ( int i = 0; i < height; i++ ) {
		for ( int j = 0; j < width; j++ ) {
			int d3, d4;

			// FIXME: look at five points?

			// look at three points to estimate the gradient
			int a1 = d1 = depth[ ( i * width + j ) ];
			int a2 = d2 = depth[ ( i * width + ( ( j + 1 ) & ( width - 1 ) ) ) ];
			int a3 = d3 = depth[ ( ( ( i + 1 ) & ( height - 1 ) ) * width + j ) ];
			int a4 = d4 = depth[ ( ( ( i + 1 ) & ( height - 1 ) ) * width + ( ( j + 1 ) & ( width - 1 ) ) ) ];

			int d2 -= d1;
			int d3 -= d1;

			dir[0] = -d2 * scale;
			dir[1] = -d3 * scale;
			dir[2] = 1;
			dir.NormalizeFast();

			a1 -= a3;
			a4 -= a3;

			dir2[0] = -a4 * scale;
			dir2[1] = a1 * scale;
			dir2[2] = 1;
			dir2.NormalizeFast();

			dir += dir2;
			dir.NormalizeFast();

			a1 = ( i * width + j ) * 4;
			data[ a1 + 0 ] = ( GLbyte )( dir[0] * 127 + 128 );
			data[ a1 + 1 ] = ( GLbyte )( dir[1] * 127 + 128 );
			data[ a1 + 2 ] = ( GLbyte )( dir[2] * 127 + 128 );
			data[ a1 + 3 ] = 255;
		}
	}
	R_StaticFree( depth );
}

const static int JITTER_SIZE = 128;
static void R_CreateJitterImage16( anImage *image ) {
	static GLbyte *data[JITTER_SIZE][JITTER_SIZE * 16][4];

	for ( int i = 0; i < JITTER_SIZE; i++ ) {
		for ( int s = 0; s < 16; s++ ) {
			int sOfs = 64 * ( s & 3 );
			int tOfs = 64 * ( ( s >> 2 ) & 3 );
			for ( int j = 0 ; j < JITTER_SIZE ; j++ ) {
				data[i][s * JITTER_SIZE + j][0] = ( rand() & 63 ) | sOfs;
				data[i][s * JITTER_SIZE + j][1] = ( rand() & 63 ) | tOfs;
				data[i][s * JITTER_SIZE + j][2] = rand();
				data[i][s * JITTER_SIZE + j][3] = 0;
			}
		}
	}

	image->GenerateImage( (GLbyte *)data, JITTER_SIZE * 16, JITTER_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
}
static void R_CreateJitterImage4( anImage *image ) {
	GLbyte *data[JITTER_SIZE][JITTER_SIZE * 4][4];

	for ( int i = 0; i < JITTER_SIZE ; i++ ) {
		for ( int s = 0; s < 4; s++ ) {
			int sOfs = 128 * ( s & 1 );
			int tOfs = 128 * ( ( s >> 1 ) & 1 );
			for ( int j = 0; j < JITTER_SIZE; j++ ) {
				data[i][s * JITTER_SIZE + j][0] = ( rand() & 127 ) | sOfs;
				data[i][s * JITTER_SIZE + j][1] = ( rand() & 127 ) | tOfs;
				data[i][s * JITTER_SIZE + j][2] = rand();
				data[i][s * JITTER_SIZE + j][3] = 0;
			}
		}
	}

	image->GenerateImage( (GLbyte *)data, JITTER_SIZE * 4, JITTER_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
}

static void R_CreateJitterImage1( anImage *image ) {
	GLbyte data[JITTER_SIZE][JITTER_SIZE][4];

	for ( int i = 0; i < JITTER_SIZE; i++ ) {
		for ( int j = 0; j < JITTER_SIZE; j++ ) {
			data[i][j][0] = rand();
			data[i][j][1] = rand();
			data[i][j][2] = rand();
			data[i][j][3] = 0;
		}
	}

	image->GenerateImage( (GLbyte *)data, JITTER_SIZE, JITTER_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
}

/*
=================
R_ImageScale

This provides a Lanczos Interpolation to scale an image, its simplified and assumes
the image has RGBA channels, But we will improve on this later
the rgba is fine as is.
=================
*/
static void R_ImageScale( GLbyte *data, int width, int height, float scale[4] ) {
    int c = width * height * 4;
    float invScaleX = 1.0f / scale[0];
    float invScaleY = 1.0f / scale[1];

    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x+ + ) {
            for ( int channel = 0; channel < 4; channel++ ) {
                float accum = 0.0f;
                float weightSum = 0.0f;
                for ( int j = y - 2; j <= y + 2; j++ ) {
                    for ( int i = x - 2; i <= x + 2; i++ ) {
                        if ( i >= 0 && i < width && j >= 0 && j < height ) {
                            float u = ( i + 0.5f ) * invScaleX;
                            float v = ( j + 0.5f ) * invScaleY;
                            float weight = LanczosKernel( u - x ) * LanczosKernel( v - y );

                            accum += data[( j * width + i ) * 4 + channel] * weight;
                            weightSum += weight;
                        }
                    }
                }

                data[( y * width + x ) * 4 + channel] = static_cast<GLbyte>( accum / weightSum );
            }
        }
    }
}
/*
=================
R_ImageScale
=================
*/
static void R_ImageScaleStandard( GLbyte *data, int width, int height, float scale[4] ) {
	int c = width * height * 4;

	for ( int i = 0; i < c; i++ ) {
		int j = ( GLbyte )( data[i] * scale[i&3] );
		if ( int j < 0 ) {
			int j = 0;
		} else if ( j > 255 ) {
			int j = 255;
		}
		data[i] = j;
	}
}

/*
=================
R_InvertAlpha
=================
*/
static void R_InvertAlpha( GLbyte *data, int width, int height ) {
	int c = width * height* 4;
	for ( int  i = 0; i < c; i += 4 ) {
		data[i+3] = 255 - data[i+3];
	}
}

/*
=================
R_InvertColor
=================
*/
static void R_InvertColor( GLbyte *data, int width, int height ) {
	int c = width * height* 4;
	for ( int i = 0; i < c; i += 4 ) {
		data[i+0] = 255 - data[i+0];
		data[i+1] = 255 - data[i+1];
		data[i+2] = 255 - data[i+2];
	}
}

/*
===================
RB_RGBAtoNormal

uses a sobel filter to change a texture to a normal map
===================
*/
static void RB_RGBAtoNormal( const GLbyte *in, GLbyte *out, GLint width, GLint height, GLboolean clampToEdge ) {}
	// convert to heightmap, storing in alpha
	// same as converting to Y in YCoCg
	int max = 1;
	for ( y = 0; y < height; y++ ) {
		const byte *inbyte  = in  + y * width * 4;
		byte *outbyte = out + y * width * 4 + 3;
		for ( x = 0; x < width; x++ ) {
			byte result = ( inbyte[0] >> 2 ) + ( inbyte[1] >> 1 ) + ( inbyte[2] >> 2 );
			result = result * result / 255; // Make linear
			*outbyte = result;
			max = anMath::Max( max, *outbyte );
			outbyte += 4;
			inbyte  += 4;
		}
	}

	// level out heights
	if ( max < 255 ) {
		for ( y = 0; y < height; y++ ) {
			byte *outbyte = out + y * width * 4 + 3;
			for ( x = 0; x < width; x++ ) {
				*outbyte = *outbyte + ( 255 - max );
				outbyte += 4;
			}
		}
	}
	// now run sobel filter over height values to generate X and Y
	// then normalize
	for ( GLint y = 0; y < height; y++ ) {
		byte *outbyte = out + y * width * 4;
		for ( x = 0; x < width; x++ ) {
			byte s[9];
			int x2 i;
			anVec3 normal;
			GLint i = 0;
			for ( int y2 = -1; y2 <= 1; y2++ ) {
				int src_y = y + y2;
				if ( clampToEdge ) {
					src_y = anMath::Clamp( src_y, 0, height - 1 );
				} else {
					src_y = ( src_y + height ) % height;
				}

				for ( x2 = -1; x2 <= 1; x2++ ) {
					int src_x = x + x2;

					if ( clampToEdge ) {
						src_x = anMath::Clamp( src_x, 0, width - 1);
					} else {
						src_x = ( src_x + width ) % width;
					}

					s[i++] = *( out + ( src_y * width + src_x ) * 4 + 3 );
				}
			}

			normal[0] = s[0] - s[2]+ 2 * s[3] - 2 * s[5] + s[6] - s[8];
			normal[1] = s[0] + 2 * s[1] + s[2] - s[6] - 2 * s[7] - s[8];
			normal[2] = s[4] * 4;

			if ( !normal.Normalize( normal ) ) {
				normal.Set( 0, 0, 1 );
			}

			*outbyte++ = FloatToOffsetByte( normal[0] );
			*outbyte++ = FloatToOffsetByte( normal[1] );
			*outbyte++ = FloatToOffsetByte( normal[2] );
			outbyte++;
		}
	}
}

/*
===================
R_AddNormalMaps
===================
*/
static void R_AddNormalMaps( GLbyte *data1, int width1, int height1, GLbyte *data2, int width2, int height2 ) {
	// resample pic2 to the same size as pic1
	if ( width2 != width1 || height2 != height1 ) {
		GLbyte *newMap = R_Dropsample( data2, width2, height2, width1, height1 );
		data2 = newMap;
	} else {
		newMap = nullptr;
	}

	// add the normal change from the second and renormalize
	for ( int i = 0; i < height1; i++ ) {
		for ( int j = 0; j < width1; j++ ) {
			anVec3	n;

			GLbyte *d1 = data1 + ( i * width1 + j ) * 4;
			GLbyte *d2 = data2 + ( i * width1 + j ) * 4;

			n[0] = ( d1[0] - 128 ) / 127.0;
			n[1] = ( d1[1] - 128 ) / 127.0;
			n[2] = ( d1[2] - 128 ) / 127.0;

			// There are some normal maps that blend to 0,0,0 at the edges
			// this screws up compression, so we try to correct that here by instead fading it to 0,0,1
			float len = n.LengthFast();
			if ( len < 1.0f ) {
				n[2] = anMath::Sqrt(1.0 - (n[0]*n[0] ) - (n[1]*n[1] ) );
			}

			n[0] += ( d2[0] - 128 ) / 127.0;
			n[1] += ( d2[1] - 128 ) / 127.0;
			n.Normalize();

			d1[0] = ( GLbyte )( n[0] * 127 + 128 );
			d1[1] = ( GLbyte )( n[1] * 127 + 128 );
			d1[2] = ( GLbyte )( n[2] * 127 + 128 );
			d1[3] = 255;
		}
	}

	if ( newMap ) {
		R_StaticFree( newMap );
	}
}

/*
================
R_SmoothNormalMap
================
*/
static void R_SmoothNormalMap( GLbyte *data, int width, int height ) {
	static float factors[3][3] = {
		{ 1, 1, 1 },
		{ 1, 1, 1 },
		{ 1, 1, 1 }
	};
	GLbyte *orig = (GLbyte *)R_StaticAlloc( width * height * 4 );
	memcpy( orig, data, width * height * 4 );

	for ( int i = 0; i < width; i++ ) {
		for ( int j = 0; j < height; j++ ) {
			anVec3 normal = vec3_origin;
			for ( int k = -1; k < 2; k++ ) {
				for ( int l = -1; l < 2; l++ ) {
					GLbyte *in = orig + ( ( ( j + 1 )&( height-1 ) )*width + ( ( i+k )&( width-1 ) ) ) * 4;
					// ignore 000 and -1 -1 -1
					if ( in[0] == 0 && in[1] == 0 && in[2] == 0 ) {
						continue;
					}
					if ( in[0] == 128 && in[1] == 128 && in[2] == 128 ) {
						continue;
					}

					normal[0] += factors[k+1][l+1] * ( in[0] - 128 );
					normal[1] += factors[k+1][l+1] * ( in[1] - 128 );
					normal[2] += factors[k+1][l+1] * ( in[2] - 128 );
				}
			}
			normal.Normalize();
			GLbyte *out = data + ( j * width + i ) * 4;
			out[0] = ( GLbyte )(128 + 127 * normal[0] );
			out[1] = ( GLbyte )(128 + 127 * normal[1] );
			out[2] = ( GLbyte )(128 + 127 * normal[2] );
		}
	}
	R_StaticFree( orig );
}

/*
===================
R_ImageAdd
===================
*/
static void R_ImageAdd( GLbyte *data1, int width1, int height1, GLbyte *data2, int width2, int height2 ) {
	// resample pic2 to the same size as pic1
	if ( width2 != width1 || height2 != height1 ) {
		GLbyte *newMap = R_Dropsample( data2, width2, height2, width1, height1 );
		data2 = newMap;
	} else {
		newMap = nullptr;
	}

	int c = width1 * height1 * 4;

	for ( int i = 0; i < c; i++ ) {
		j = data1[i] + data2[i];
		if ( int j > 255 ) {
			j = 255;
		}
		data1[i] = j;
	}

	if ( GLbyte *newMap ) {
		R_StaticFree( newMap );
	}
}

// we build a canonical token form of the image program here
static char parseBuffer[MAX_IMAGE_NAME];

/*
===================
AppendToken
===================
*/
static void AppendToken( anToken &token ) {
	// add a leading space if not at the beginning
	if ( parseBuffer[0] ) {
		anStr::Append( parseBuffer, MAX_IMAGE_NAME, " " );
	}
	anStr::Append( parseBuffer, MAX_IMAGE_NAME, token.c_str() );
}

/*
===================
MatchAndAppendToken
===================
*/
static void MatchAndAppendToken( anLexer &src, const char *match ) {
	if ( !src.ExpectTokenString( match ) ) {
		return;
	}
	// a matched token won't need a leading space
	anStr::Append( parseBuffer, MAX_IMAGE_NAME, match );
}

/*
===================
R_ParseImageProgram_r

If pic is nullptr, the timeStamps will be filled in, but no image will be generated
If both pic and timeStamps are nullptr, it will just advance past it, which can be
used to parse an image program from a text stream.
===================
*/
static bool R_ParseImageProgram_r( anLexer &src, GLbyte **pic, int *width, int *height, ARC_TIME_T *timeStamps, textureDepth_t *depth ) {
	anToken token;
	ARC_TIME_T timestamp;

	src.ReadToken( &token );
	// Since all interaction shaders now assume YCoCG diffuse textures.  We replace all entries for the intrinsic
	// _black texture to the black texture on disk.  Doing this will cause a YCoCG compliant texture to be generated.
	// Without a YCoCG compliant black texture we will get color artifacts for any interaction
	// material that specifies the _black texture.
	if ( token == "_black" ) {
		token = "textures/black";
	}

	// also check for _white
	if ( token == "_white" ) {
		token = "guis/white";
	}
	AppendToken( token );

	if ( !token.Icmp( "heightMap" ) ) {
		MatchAndAppendToken( src, "( " );
		if ( !R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth ) ) {
			return false;
		}

		MatchAndAppendToken( src, "," );
		src.ReadToken( &token );
		AppendToken( token );
		float scale = token.GetFloatValue();

		// process it
		if ( pic ) {
			R_HeightmapToNormalMap( *pic, *width, *height, scale );
			if ( depth ) {
				*depth = TD_BUMP;
			}
		}

		MatchAndAppendToken( src, " )" );
		return true;
	}

	if ( !token.Icmp( "addNormals" ) ) {
		GLbyte *pic2;
		int	width2, height2;
		MatchAndAppendToken( src, "( " );
		if ( !R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth ) ) {
			return false;
		}
		MatchAndAppendToken( src, "," );
		if ( !R_ParseImageProgram_r( src, pic ? &pic2 : nullptr, &width2, &height2, timeStamps, depth ) ) {
			if ( pic ) {
				R_StaticFree( *pic );
				*pic = nullptr;
			}
			return false;
		}

		// process it
		if ( pic ) {
			R_AddNormalMaps( *pic, *width, *height, pic2, width2, height2 );
			R_StaticFree( pic2 );
			if ( depth ) {
				*depth = TD_BUMP;
			}
		}

		MatchAndAppendToken( src, " )" );
		return true;
	}

	if ( !token.Icmp( "smoothNormals" ) ) {
		MatchAndAppendToken( src, "( " );
		if ( !R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth ) ) {
			return false;
		}

		if ( pic ) {
			R_SmoothNormalMap( *pic, *width, *height );
			if ( depth ) {
				*depth = TD_BUMP;
			}
		}
		MatchAndAppendToken( src, " )" );
		return true;
	}

	if ( !token.Icmp( "add" ) ) {
		GLbyte *pic2;
		int	 width2, height2;
		MatchAndAppendToken( src, "( " );
		if ( !R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth ) ) {
			return false;
		}

		MatchAndAppendToken( src, "," );

		if ( !R_ParseImageProgram_r( src, pic ? &pic2 : nullptr, &width2, &height2, timeStamps, depth ) ) {
			if ( pic ) {
				R_StaticFree( *pic );
				*pic = nullptr;
			}
			return false;
		}

		// process it
		if ( pic ) {
			R_ImageAdd( *pic, *width, *height, pic2, width2, height2 );
			R_StaticFree( pic2 );
		}

		MatchAndAppendToken( src, " )" );
		return true;
	}

	if ( !token.Icmp( "scale" ) ) {
		float	scale[4];
		MatchAndAppendToken( src, "( " );
		R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth );
		for ( int i = 0; i < 4; i++ ) {
			MatchAndAppendToken( src, "," );
			src.ReadToken( &token );
			AppendToken( token );
			scale[i] = token.GetFloatValue();
		}

		// process it
		if ( pic ) {
			R_ImageScale( *pic, *width, *height, scale );
		}
		MatchAndAppendToken( src, " )" );
		return true;
	}

	if ( !token.Icmp( "invertAlpha" ) ) {
		MatchAndAppendToken( src, "( " );
		R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth );
		// process it
		if ( pic ) {
			R_InvertAlpha( *pic, *width, *height );
		}
		MatchAndAppendToken( src, " )" );
		return true;
	}

	if ( !token.Icmp( "invertColor" ) ) {
		MatchAndAppendToken( src, "( " );
		R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth );
		// process it
		if ( pic ) {
			R_InvertColor( *pic, *width, *height );
		}
		MatchAndAppendToken( src, " )" );
		return true;
	}

	if ( !token.Icmp( "makeIntensity" ) ) {
		MatchAndAppendToken( src, "( " );
		R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth );
		// copy red to green, blue, and alpha
		if ( pic ) {
			int c = *width * *height * 4;
			for ( int i = 0; i < c; i+=4 ) {
				( *pic )[i+1] =
				( *pic )[i+2] =
				( *pic )[i+3] = ( *pic )[i];
			}
		}
		MatchAndAppendToken( src, " )" );
		return true;
	}

	if ( !token.Icmp( "makeAlpha" ) ) {
		MatchAndAppendToken( src, "( " );
		R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth );
		// average RGB into alpha, then set RGB to white
		if ( pic ) {
			int c = *width * *height * 4;
			for ( int i = 0; i < c; i += 4 ) {
				( *pic )[i+3] = ( ( *pic )[i+0] + ( *pic )[i+1] + ( *pic )[i+2] ) / 3;
				( *pic )[i+0] = ( *pic )[i+1] = ( *pic )[i+2] = 255;
			}
		}
		MatchAndAppendToken( src, " )" );
		return true;
	}

	// if we are just parsing instead of loading or checking,
	// don't do the R_LoadImage
	if ( !timeStamps && !pic ) {
		return true;
	}

	// load it as an image
	R_LoadImage( token.c_str(), pic, width, height, &timestamp, true );

	if ( timestamp == -1 ) {
		return false;
	}

	// add this to the timestamp
	if ( timeStamps ) {
		if ( timestamp > *timeStamps ) {
			*timeStamps = timestamp;
		}
	}

	return true;
}

/*
===================
R_LoadImageProgram
===================
*/
void R_LoadImageProgram( const char *name, GLbyte **pic, int *width, int *height, ARC_TIME_T *timeStamps, textureDepth_t *depth ) {
	anLexer src.LoadMemory( name, strlen( name ), name );
	src.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
	parseBuffer[0] = 0;
	if ( timeStamps ) {
		*timeStamps = 0;
	}

	R_ParseImageProgram_r( src, pic, width, height, timeStamps, depth );

	src.FreeSource();
}

/*
===================
R_ParsePastImageProgram
===================
*/
const char *R_ParsePastImageProgram( anLexer &src ) {
	parseBuffer[0] = 0;
	R_ParseImageProgram_r( src, nullptr, nullptr, nullptr, nullptr, nullptr );
	return parseBuffer;
}

//=====================================================================

typedef struct {
	char *name;
	int	minimize, maximize;
} filterName_t;
static filterName_t textureFilters[] = {
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR},
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST}};

/*
===============
anImage::Reload
===============
*/
void anImage::Reload( bool checkPrecompressed, bool force ) {
	// always regenerate functional images
	if ( generatorFunction ) {
		common->DPrintf( "regenerating %s.\n", imgName.c_str() );
		generatorFunction( this );
		return;
	}

	// check file times
	if ( !force ) {
		ARC_TIME_T	current;
		if ( cubeFiles != CF_2D ) {
			R_LoadCubeImages( imgName, cubeFiles, nullptr, nullptr, &current );
		} else {
			// get the current values
			R_LoadImageProgram( imgName, nullptr, nullptr, nullptr, &current );
		}
		if ( current <= timestamp ) {
			return;
		}
	}

	common->DPrintf( "reloading %s.\n", imgName.c_str() );

	PurgeImage();

	// force no precompressed image check, which will cause it to be reloaded
	// from source, and another precompressed file generated.
	// Load is from the front end, so the back end must be synced
	ActuallyLoadImage( checkPrecompressed, false );
}

/*
===============
R_ReloadImages_f

Regenerate all images that came directly from files that have changed, so
any saved changes will show up in place.

New r_texturesize/r_texturedepth variables will take effect on reload

reloadImages <all>
===============
*/
void R_ReloadImages_f( const anCommandArgs &args ) {
	// this probably isn't necessary...
	globalImages->ChangeTextureFilter();

	bool all = false;
	bool checkPrecompressed = false;		// if we are doing this as a vid_restart, look for precompressed like normal

	if ( args.Argc() == 2 ) {
		if ( !anStr::Icmp( args.Argv(1 ), "all" ) ) {
			all = true;
		} else if ( !anStr::Icmp( args.Argv(1 ), "reload" ) ) {
			all = true;
			checkPrecompressed = true;
		} else {
			common->Printf( "USAGE: reloadImages <all>\n" );
			return;
		}
	}

	for ( int i = 0; i < globalImages->images.Num(); i++ ) {
		image = globalImages->images[i];
		anImage *image->Reload( checkPrecompressed, all );
	}

	//globalImages->ReloadImages( all );
}

typedef struct {
	anImage	*image;
	int		size;
} sortImage;

`/*
=======================
R_QSortImageSizes
=======================
*/
static int R_QSortImageSizes( const void *a, const void *b ) {
	const sortImage	*ea = ( sortImage *)a;
	const sortImage	*eb = ( sortImage *)b;

	if ( ea->size > eb->size ) {
		return -1;
	}
	if ( ea->size < eb->size ) {
		return 1;
	}
	return anStr::Icmp( ea->image->imgName, eb->image->imgName );
}

/*
=======================
R_QsortImageName
=======================
*/
static int R_QsortImageName( const void *a, const void *b ) {
	const sortImage	*ea, *eb;

	ea = ( sortImage *)a;
	eb = ( sortImage *)b;

	return anStr::Icmp( ea->image->GetName(), eb->image->GetName() );
}

/*
===============
R_ListImages_f
===============
*/
void R_ListImages_f( const anCommandArgs &args ) {
	int		count = 0, matchTag = 0;
	bool	uncompressedOnly = false, unloaded = false;
	bool	partial = false, cached = false;
	bool	uncached = false,failed = false;
	bool	touched = false, sorted = false;
	bool	duplicated = false, byClassification = false, overSized = false;

	if ( args.Argc() == 1 ) {
	} else if ( args.Argc() == 2 ) {
		if ( anStr::Icmp( args.Argv( 1 ), "uncompressed" ) == 0 ) {
			uncompressedOnly = true;
		} else if ( anStr::Icmp( args.Argv( 1 ), "sorted" ) == 0 ) {
			sorted = true;
		} else if ( anStr::Icmp( args.Argv( 1 ), "partial" ) == 0 ) {
			partial = true;
		} else if ( anStr::Icmp( args.Argv( 1 ), "unloaded" ) == 0 ) {
			unloaded = true;
		} else if ( anStr::Icmp( args.Argv( 1 ), "cached" ) == 0 ) {
			cached = true;
		} else if ( anStr::Icmp( args.Argv( 1 ), "uncached" ) == 0 ) {
			uncached = true;
		} else if ( anStr::Icmp( args.Argv( 1 ), "tagged" ) == 0 ) {
			matchTag = 1;
		} else if ( anStr::Icmp( args.Argv( 1 ), "duplicated" ) == 0 ) {
			duplicated = true;
		} else if ( anStr::Icmp( args.Argv( 1 ), "touched" ) == 0 ) {
			touched = true;
		} else if ( anStr::Icmp( args.Argv( 1 ), "classify" ) == 0 ) {
			byClassification = true;
			sorted = true;
		} else if ( anStr::Icmp( args.Argv( 1 ), "oversized" ) == 0 ) {
			byClassification = true;
			sorted = true;
			overSized = true;
		} else {
			failed = true;
		}
	} else {
		failed = true;
	}

	if ( failed ) {
		common->Printf( "usage: listImages [ sorted | partial | unloaded | cached | uncached | tagged | duplicated | touched | classify | showOverSized ]\n" );
		return;
	}

	const char *header = "		 -w-- -h-- filt -fmt-- wrap  size --name-------\n";
	common->Printf( "\n%s", header );

	int totalSize = 0;

	sortImage *sortedArray = ( sortImage *)alloca( sizeof( sortImage ) * globalImages->images.Num() );

	for ( int i = 0; i < globalImages->images.Num(); i++ ) {
		anImage *image = globalImages->images[i];
		if ( uncompressedOnly ) {
			if ( ( image->internalFormat >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT && image->internalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT )
				|| image->internalFormat == GL_COLOR_INDEX8_EXT ) {
				continue;
			}
		}

		if ( matchTag && image->classification != matchTag ) {
			continue;
		}
		if ( unloaded && image->texnum != anImage::TEXTURE_NOT_LOADED ) {
			continue;
		}
		if ( partial && !image->isPartialImage ) {
			continue;
		}
		if ( cached && ( !image->partialImage || image->texnum == anImage::TEXTURE_NOT_LOADED ) ) {
			continue;
		}
		if ( uncached && ( !image->partialImage || image->texnum != anImage::TEXTURE_NOT_LOADED ) ) {
			continue;
		}

		// only print duplicates (from mismatched wrap / clamp, etc)
		if ( duplicated ) {
			for ( int j = i + 1; j < globalImages->images.Num(); j++ ) {
				if ( anStr::Icmp( image->imgName, globalImages->images[ j ]->imgName ) == 0 ) {
					break;
				}
			}
			if ( j == globalImages->images.Num() ) {
				continue;
			}
		}

		// "listimages touched" will list only images bound since the last "listimages touched" call
		if ( touched ) {
			if ( image->bindCount == 0 ) {
				continue;
			}
			image->bindCount = 0;
		}

		if ( sorted ) {
			sortedArray[count].image = image;
			sortedArray[count].size = image->StorageSize();
		} else {
			common->Printf( "%4i:",	i );
			image->Print();
		}
		totalSize += image->StorageSize();
		count++;
	}

	if ( sorted ) {
		qsort( sortedArray, count, sizeof( sortImage ), R_QSortImageSizes );
		int partialSize = 0;
		for ( int i = 0; i < count; i++ ) {
			common->Printf( "%4i:",	i );
			sortedArray[i].image->Print();
			partialSize += sortedArray[i].image->StorageSize();
			if ( ( ( i + 1 ) % 10 ) == 0 ) {
				common->Printf( "-------- %5.1f of %5.1f megs --------\n", partialSize / ( 1024 * 1024.0 ), totalSize / ( 1024 * 1024.0 ) );
			}
		}
	}

	common->Printf( "%s", header );
	common->Printf( " %i images (%i total)\n", count, globalImages->images.Num() );
	common->Printf( " %5.1f total megabytes of images\n\n\n", totalSize / ( 1024 * 1024.0 ) );

	if ( byClassification ) {
		anList< int > classifications[IC_COUNT];
		for ( int i = 0; i < count; i++ ) {
			int cl = ClassifyImage( sortedArray[i].image->imgName );
			classifications[ cl ].Append( i );
		}

		for ( int i = 0; i < IC_COUNT; i++ ) {
			int partialSize = 0;
			anList<int> overSizedList;
			for ( j = 0; j < classifications[i].Num(); j++ ) {
				partialSize += sortedArray[ classifications[i][ j ] ].image->StorageSize();
				if ( overSized ) {
					if ( sortedArray[ classifications[i][ j ] ].image->uploadWidth > IC_Info[i].maxWidth && sortedArray[ classifications[i][ j ] ].image->uploadHeight > IC_Info[i].maxHeight ) {
						overSizedList.Append( classifications[i][ j ] );
					}
				}
			}
			common->Printf ( " Classification %s contains %i images using %5.1f megabytes\n", IC_Info[i].desc, classifications[i].Num(), partialSize / ( 1024*1024.0 ) );
			if ( overSized && overSizedList.Num() ) {
				common->Printf( "  The following images may be oversized\n" );
				for ( int j = 0; j < overSizedList.Num(); j++ ) {
					common->Printf( "	 " );
					sortedArray[ overSizedList[ j ] ].image->Print();
					common->Printf( "\n" );
				}
			}
		}
	}
}

int anImage::GetImageId( const char *name ) const {
	//GetName();
	//Printf( "ImageId [%s].\n", name );
	for ( int i = 0 ; i < images.Num() ; i++ ) {
		if ( !strcmp( name, images[i]->imgName ) ) {
			//Printf( "Found ImageId %d\n", i );
			return i;
		}
	}

	//Printf( "Image not found.\n" );
	return -1;
}

static void anImage::GetGeneratedName( anStr &_name, const textureUsage_t &_usage, const cubeFiles_t &_cube ) {
	anStringStatic< 64 > extension;

	_name.ExtractFileExtension( extension );
	_name.StripFileExtension();

	_name += va( "#__%02d%02d", (int)_usage, (int)_cube );
	if ( extension.Length() > 0 ) {
		_name.SetFileExtension( extension );
	}
/*	int hash = name.FileNameHash();
	for ( anImage *image = imageHashTable[hash]; image; image = image->hashNext ) {
		if ( !name.Icmp( image->imgName ) ) {
			// the white image can be used with any set of parms, but other mismatches are errors
			if ( name.Icmp( name, "*white" ) ) {
				if ( image->flags != flags ) {
					common->Printf( "WARNING: reused image %s with mixed flags (%i vs %i)\n", name, image->flags, flags );
					//return image;
				}
			}
			return image;
		}
	}*/
}

/*
===============
R_CombineCubeImages_f

Used to combine animations of six separate tga files into
a serials of 6x taller tga files, for preparation to roq compress
===============
*/
void R_CombineCubeImages_f( const anCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "usage: combineCubeImages <baseName>\n" );
		common->Printf( " combines basename[1-6][0001-9999].tga to basenameCM[0001-9999].tga\n" );
		common->Printf( " 1: forward 2:right 3:back 4:left 5:up 6:down\n" );
		return;
	}

	anStr baseName = args.Argv( 1 );
	common->SetRefreshOnPrint( true );
	char filename[MAX_IMAGE_NAME];
	GLbyte *pics[6];
	int width, height, orderRemap[6] = { 1,3,4,2,5,6 };

	for ( int frameNum = 1; frameNum < 10000; frameNum++ ) {
		for ( int side = 0; side < 6; side++ ) {
			sprintf( filename, "%s%i%04i.tga", baseName.c_str(), orderRemap[side], frameNum );
			common->Printf( "reading %s\n", filename );
			R_LoadImage( filename, &pics[side], &width, &height, nullptr, true );
			if ( !pics[side] ) {
				common->Printf( "not found.\n" );
				break;
			}

			// convert from "camera" images to native cube map images
			switch ( side ) {
			case 0:	// forward
				R_RotatePic( static_cast<GLbyte *>( pics[side] ), width );
				break;
			case 1:	// back
				R_RotatePic( static_cast<GLbyte *>( pics[side] ), width );
				R_HorizontalFlip( static_cast<GLbyte *>( pics[side] ), width, height );
				R_VerticalFlip( static_cast<GLbyte *>( pics[side] ), width, height );
				break;
			case 2:	// left
				R_VerticalFlip( static_cast<GLbyte *>( pics[side] ), width, height );
				break;
			case 3:	// right
				R_HorizontalFlip( static_cast<GLbyte *>( pics[side] ), width, height );
				break;
			case 4:	// up
				R_RotatePic( static_cast<GLbyte *>( pics[side] ), width );
				break;
			case 5: // down
				R_RotatePic( static_cast<GLbyte *>( pics[side] ), width );
				break;
			}
		}

		if ( int side != 6 ) {
			for ( int i = 0; i < side; side++ ) {
				Mem_Free( pics[side] );
			}
			break;
		}

		GLbyte *combined = static_cast<GLbyte *>( Mem_Alloc( width * height * 6  * 4 ) );
		for ( int side = 0; side < 6; side++ ) {
			memcpy( combined + width * height * 4 * side, pics[side], width * height * 4 );
			Mem_Free( pics[side] );
		}

		sprintf( filename, "%sCM%04i.tga", baseName.c_str(), frameNum );
		common->Printf( "writing %s\n", filename );
		R_WriteTGA( filename, combined, width, height*6 );
		Mem_Free( combined );
	}
	common->SetRefreshOnPrint( false );
}

/*
===============
R_ApplyCubeMapTransforms

transforms multiple ways, the images from a cube map,
as well as both Env map and Skybox systems.
===============
*/
void R_ApplyCubeMapTransforms( int iter, GLbyte *data, int size ) {
	if ( ( iter == 1 ) || ( iter == 2 ) ) {
		R_VerticalFlip( data, size, size );
	}

	if ( ( iter == 0 ) || ( iter == 1 ) || ( iter == 4 ) || ( iter == 5 ) ) {
		// not only rotates but also flips horitzontally
		R_RotatePic( data, size );
	}

	if ( iter == 1 ) {
		R_VerticalFlip( data, size, size );
	} else if ( iter == 3 ) {
		R_HorizontalFlip( data, size, size );
	}
}

static bool FormatIsDXT( int internalFormat ) {
	if ( internalFormat < GL_COMPRESSED_RGB_S3TC_DXT1_EXT || internalFormat > GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ) {
		return false;
	}
	return true;
}

int MakePowerOfTwo( int num ) {
	int		pot;
	for ( pot = 1; pot < num; pot <<= 1 ) {
	}
	return pot;
}

/*
================
BitsForInternalFormat

Used for determining memory utilization
================
*/
int anImage::BitsForInternalFormat( int internalFormat ) const {
	switch ( internalFormat ) {
	case GL_INTENSITY8:
	case 1:
		return 8;
	case 2:
	case GL_LUMINANCE8_ALPHA8:
		return 16;
	case 3:
		return 32;		// on some future hardware, this may actually be 24, but be conservative
	case 4:
		return 32;
	case GL_LUMINANCE8:
		return 8;
	case GL_ALPHA8:
		return 8;
	case GL_RGBA8:
		return 32;
	case GL_RGB8:
		return 32;		// on some future hardware, this may actually be 24, but be conservative
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		return 4;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return 4;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		return 8;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return 8;
	case GL_RGBA4:
		return 16;
	case GL_RGB5:
		return 16;
	case GL_COLOR_INDEX8_EXT:
		return 8;
	case GL_COLOR_INDEX:
		return 8;
	case GL_COMPRESSED_RGB_ARB:
		return 4;			// not sure
	case GL_COMPRESSED_RGBA_ARB:
		return 8;			// not sure
	default:
		common->Error( "R_BitsForInternalFormat: BAD FORMAT:%i", internalFormat );
	}
	return 0;
}

/*
========================
anImage::AllocImage
========================
*/
void anImage::AllocImage( textureFilter_t tf, textureRepeat_t tr ) {
	filter = tf;
	repeat = tr;
	opts = imgOpts;
	DeriveOpts();
	AllocImage();
}

/*
==================
UploadCompressedNormalMap

Create a 256 color palette to be used by compressed normal maps
==================
*/
void anImage::UploadCompressedNormalMap( int width, int height, const GLbyte *rgba, int mipLevel ) {
	int row = width < 4 ? 4 : width;	// OpenGL's pixel packing rule

	GLbyte *normals = (GLbyte *)_alloca( row * height );
	if ( !normals ) {
		common->Error( "R_UploadCompressedNormalMap: _alloca failed" );
	}

	const GLbyte *in = rgba;
	GLbyte *out = (GLbyte *)normals;

	for ( int i = 0; i < height; i++, out += row, in += width * 4 ) {
		for ( j = 0; j < width; j++ ) {
			int x = in[ j * 4 + 0 ];
			int y = in[ j * 4 + 1 ];
			int z = in[ j * 4 + 2 ];
			if ( x == 128 && y == 128 && z == 128 ) {
				// the "nullnormal" color
				int c = 255;
			} else {
				int c = ( globalImages->originalToCompressed[x] << 4 ) | globalImages->originalToCompressed[y];
				if ( c == 255 ) {
					c = 254;	// don't use the nullnormal color
				}
			}
			out[j] = c;
		}
	}

	if ( mipLevel == 0 ) {
		// Optionally write out the paletized normal map to a .tga
		if ( globalImages->image_writeNormalTGAPalletized.GetBool() ) {
			char filename[MAX_IMAGE_NAME];
			ImageProgramStringToCompressedFileName( imgName, filename );
			char *ext = strrchr(filename, '.');
			if ( ext ) {
				strcpy(ext, "_pal.tga" );
				R_WritePalTGA( filename, normals, globalImages->compressedPalette, width, height);
			}
		}
	}

	if ( qglConfig.isSharedTPalette ) {
		qglTexImage2D( GL_TEXTURE_2D, mipLevel, GL_COLOR_INDEX8_EXT, width, height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, normals );
	}
}

//=======================================================================

static byte	mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};

/*
===============
SelectInternalFormat

This may need to scan six cube map images
GL_RGBA8	GL_RGBA8UI		GL_RGBA8_SNORM
GL_RGBA16					GL_RGBA16_SNORM
GL_RGBA32	GL_RGBA32UI
GL_SIGNED_NORMALIZED
===============
*/
GLenum anImage::SelectInternalFormat( const GLbyte **dataPtrs, int numDataPtrs, int width, int height, textureDepth_t minimumDepth, bool *monochromeResult ) const {
	// determine if the rgb channels are all the same
	// and if either all rgb or all alpha are 255
	int c = width*height;
	int rgbDiffer = 0;
	int rgbaDiffer = 0;
	int rgbOr = 0;
	int rgbAnd = -1;
	int aOr = 0;
	int aAnd = -1;

	*monochromeResult = true;	// until shown otherwise

	for ( int side = 0; side < numDataPtrs; side++ ) {
		const GLbyte *scan = dataPtrs[side];
		for ( int i = 0; i < c; i++, scan += 4 ) {
			aOr |= scan[3];
			aAnd &= scan[3];

			int cor = scan[0] | scan[1] | scan[2];
			int cand = scan[0] & scan[1] & scan[2];

			// if rgb are all the same, the or and and will match
			rgbDiffer |= ( cor ^ cand );

			// our "isMonochrome" test is more lax than rgbDiffer,
			// allowing the values to be off by several units and
			// still use the NV20 mono path
			if ( *monochromeResult ) {
				if ( abs( scan[0] - scan[1] ) > 16 || abs( scan[0] - scan[2] ) > 16 ) {
						*monochromeResult = false;
					}
			}

			rgbOr |= cor;
			rgbAnd &= cand;

			cor |= scan[3];
			cand &= scan[3];

			rgbaDiffer |= ( cor ^ cand );
		}
	}

	// we assume that all 0 implies that the alpha channel isn't needed,
	// because some tools will spit out 32 bit images with a 0 alpha instead
	// of 255 alpha, but if the alpha actually is referenced, there will be
	// different behavior in the compressed vs uncompressed states.
	if ( aAnd == 255 || aOr == 0 ) {
		bool needAlpha = false;
	} else {
		bool needAlpha = true;
	}

	// catch normal maps first
	if ( minimumDepth == TD_BUMP ) {
		if ( globalImages->image_useCompression.GetBool() && globalImages->image_useNormalCompression.GetInteger() == 1 && qglConfig.isSharedTPalette ) {
			// image_useNormalCompression should only be set to 1 on nv_10 and nv_20 paths
			return GL_COLOR_INDEX8_EXT;
		} else if ( globalImages->image_useCompression.GetBool() && globalImages->image_useNormalCompression.GetInteger() && qglConfig.textureCompression ) {
			// image_useNormalCompression == 2 uses rxgb format which produces really good quality for medium settings
			return GL_COMPRESSED_RED_RGTC1;
		} else {
			// we always need the alpha channel for bump maps for swizzling
			return GL_RGBA8;
		}
	}

	// allow a complete override of image compression with a cvar
	if ( !globalImages->image_useCompression.GetBool() ) {
		minimumDepth = TD_HIGH_QUALITY;
	}

	if ( minimumDepth == TD_SPECULAR ) {
		// we are assuming that any alpha channel is unintentional
		if ( qglConfig.textureCompression ) {
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		} else {
			return GL_RGB5;
		}
	}

	if ( minimumDepth == TD_DIFFUSE ) {
		// we might intentionally have an alpha channel for alpha tested textures
		if ( qglConfig.textureCompression ) {
			if ( !needAlpha ) {
				return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			} else {
				return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			}
		} else if ( ( aAnd == 255 || aOr == 0 ) ) {
			return GL_RGB5;
		} else {
			return GL_RGBA4;
		}
	}

	// there will probably be some drivers that don't
	// correctly handle the intensity/alpha/luminance/luminance+alpha
	// formats, so provide a fallback that only uses the rgb/rgba formats
	if ( !globalImages->image_useAllFormats.GetBool() ) {
		// pretend rgb is varying and inconsistant, which
		// prevents any of the more compact forms
		rgbDiffer = 1;
		rgbaDiffer = 1;
		rgbAnd = 0;
	}

	// cases without alpha
	if ( !needAlpha ) {
		if ( minimumDepth == TD_HIGH_QUALITY ) {
			return GL_RGB8;			// four bytes
		}
		// GL_ARB_texture_compression_rgtc
		// GL_ARB_texture_compression_bptc for possible replacements?
		if ( qglConfig.textureCompression ) {
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;	// half byte
			//qglCompressedTexImage2D(); // implement me NOW!
		}

		return GL_RGB5;			// two bytes
	}

	// cases with alpha
	if ( !rgbaDiffer ) {
		if ( minimumDepth != TD_HIGH_QUALITY && qglConfig.textureCompression ) {
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;	// one byte
			//qglCompressedTexImage2D(); // implement me NOW!
		}

		return GL_INTENSITY8;	// single byte for all channels
	}

#if 0 //GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM GL_COMPRESSED_RGBA_BPTC_UNORM
	// we don't support alpha textures any more, because there
	// is a discrepancy in the definition of TEX_ENV_COMBINE that
	// causes them to be treated as 0 0 0 A, instead of 1 1 1 A as
	// normal texture modulation treats them
	if ( rgbAnd == 255 ) {
		return GL_ALPHA8;		// single byte, only alpha
	}
#endif

	if ( minimumDepth == TD_HIGH_QUALITY ) {
		return GL_RGBA8;	// four bytes
	}
	if ( qglConfig.textureCompression ) {
		return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;	// one byte
	}
	if ( !rgbDiffer ) {
		return GL_LUMINANCE8_ALPHA8;	// two bytes, max quality
	}
	return GL_RGBA4;	// two bytes
}

/*
==================
SetImageFilterAndRepeat
==================
*/
void anImage::SetImageFilterAndRepeat() const {
	// set the minimize / maximize filtering
	switch ( filter ) {
	case TF_DEFAULT:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter );
		break;
	case TF_LINEAR:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		break;
	case TF_NEAREST:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		break;
	case TF_LINEARNEAREST:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		break;
	default:
		common->FatalError( "R_CreateImage: bad texture filter" );
	}

	if ( qglConfig.useAnisotropyFilter ) {
		// only do aniso filtering on mip mapped images
		if ( filter == TF_DEFAULT ) {
			globalImages->textureAnisotropy = r_maxAnisotropicFiltering.GetInteger();
			if ( aniso > qglConfig.maxTextureAnisotropy || aniso globalImages->textureMaxFilter) {
				aniso = qglConfig.maxTextureAnisotropy;
			}
			if ( aniso < 0 ) {
				aniso = 0;
			}
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,  globalImages->textureAnisotropy  );
		} else {
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );
		}
	}
	if ( qglConfig.useTextureLODBias && ( usage != TD_FONT ) ) {
		// use a blurring LOD bias in combination with high anisotropy to fix our aliasing grate textures...
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS_EXT, r_lodBias.GetFloat() );
		//qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS_EXT, globalImages->textureLODBias );
	}

	// set the wrap/clamp modes
	switch ( repeat ) {
	case TR_REPEAT:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		break;
	case TR_MIRRORED_REPEAT:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
		break;
	case TR_CLAMP_TO_BORDER:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
		break;
	case TR_CLAMP_TO_ZERO:
		float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
		break;
	case TR_CLAMP_TO_ZERO_ALPHA:
			float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
			break;
	case TR_CLAMP:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
		break;
	case TR_CLAMP_X:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		//GL_MIRROR_CLAMP_TO_EDGE_ATI
		break;
	case TR_CLAMP_Y:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_Y );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_Y );
		break;
	case TR_MIRROR:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRROR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRROR );
		break;
	case TR_MIRROR_X:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRROR_X );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRROR_X );
		break;
	case TR_MIRROR_Y:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRROR_Y );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRROR_Y );
		break;
	case TR_MIRROR_CLAMP_ATI:
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRROR_CLAMP_ATI );
		break;
	default:
		common->FatalError( "R_CreateImage: bad texture repeat" );
	}
}

/*
================
anImage::Downsize
helper function that takes the current width/height and might make them smaller
================
*/
void anImage::GetDownsize( int &scaledWidth, int &scaledHeight ) const {
	int size = 0;

	// perform optional picmip operation to save texture memory
	if ( depth == TD_SPECULAR && globalImages->image_downSizeSpecular.GetInteger() ) {
		size = globalImages->image_downSizeSpecularLimit.GetInteger();
		if ( size == 0 ) {
			size = 64;
		}
	} else if ( depth == TD_BUMP && globalImages->image_downSizeBump.GetInteger() ) {
		size = globalImages->image_downSizeBumpLimit.GetInteger();
		if ( size == 0 ) {
			size = 64;
		}
	} else if ( ( allowDownSize || globalImages->image_forceDownSize.GetBool() ) && globalImages->image_downSize.GetInteger() ) {
		size = globalImages->image_downSizeLimit.GetInteger();
		if ( size == 0 ) {
			size = 256;
		}
	}

	if ( size > 0 ) {
		while ( scaledWidth > size || scaledHeight > size ) {
			if ( scaledWidth > 1 ) {
				scaledWidth >>= 1;
			}
			if ( scaledHeight > 1 ) {
				scaledHeight >>= 1;
			}
		}
	}

	// clamp to minimum size
	if ( scaledWidth < 1 ) {
		scaledWidth = 1;
	}
	if ( scaledHeight < 1 ) {
		scaledHeight = 1;
	}

	// clamp size to the hardware specific upper limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	// This causes a 512*256 texture to sample down to
	// 256*128 on a voodoo3, even though it could be 256*256
	while ( scaledWidth > qglConfig.maxImageSize || scaledHeight > qglConfig.maxImageSize ) {
		scaledWidth >>= 1;
		scaledHeight >>= 1;
	}
}

/*
================
GenerateImage
================
*/
void anImage::GenerateAlternateImage( const GLbyte *pic, int width, int height, textureFilter_t filterParm, textureRepeat_t repeatParm, textureUsage_t usageParm ) {
	PurgeImage();

	filter = filterParm;
	repeat = repeatParm;
	usage = usageParm;
	cubeFiles = CF_2D;

	opts.textureType = TT_2D;
	opts.width = width;
	opts.height = height;
	opts.numLevels = 0;
	DeriveOpts();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if ( !R_IsInitialized() ) {
		return;
	}

	anBinaryImage im( GetName() );
	im.Load2DFromMemory( width, height, pic, opts.numLevels, opts.format, opts.colorFormat, opts.gammaMips );

	AllocImage();

	for ( int i = 0; i < im.NumImages(); i++ ) {
		const binaryImg & img = im.GetImageHeader( i );
		const GLbyte * data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
}

/*
================
GenerateImage

The alpha channel bytes should be 255 if you don't
want the channel.

We need a material characteristic to ask for specific texture modes.
Designed limitations of flexibility:

No support for texture borders.
No support for texture border color.

No support for texture environment colors or GL_BLEND or GL_DECAL
texture environments, because the automatic optimization to single
or dual component textures makes those modes potentially undefined.

No non-power-of-two images.
No palettized textures.

There is no way to specify separate wrap/clamp values for S and T
There is no way to specify explicit mip map levels

================
*/
void anImage::GenerateImage( const GLbyte *pic, int width, int height, textureFilter_t filterParm, bool allowDownSizeParm, textureRepeat_t repeatParm, textureDepth_t depthParm ) {
	PurgeImage();

	filter = filterParm;
	allowDownSize = allowDownSizeParm;
	repeat = repeatParm;
	depth = depthParm;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if ( !qglConfig.isInitialized ) {
		return;
	}

	// don't let mip mapping smear the texture into the clamped border
	if ( repeat == TR_CLAMP_TO_ZERO ) {
		bool preserveBorder = true;
	} else {
		bool preserveBorder = false;
	}

	// make sure it is a power of 2
	int scaledWidth = MakePowerOfTwo( width );
	int scaledHeight = MakePowerOfTwo( height );

	if ( scaledWidth != width || scaledHeight != height ) {
		common->Error( "R_CreateImage: not a power of 2 image" );
	}

	// Optionally modify our width/height based on options/hardware
	GetDownsize( scaledWidth, scaledHeight );

	GLbyte *scaledBuffer = nullptr;

	// generate the texture number
	qglGenTextures( 1, &texnum );

	// select proper internal format before we resample
	internalFormat = SelectInternalFormat( &pic, 1, width, height, depth, &isMonochrome );

	// copy or resample data as appropriate for first MIP level
	if ( ( scaledWidth == width ) && ( scaledHeight == height ) ) {
		// we must copy even if unchanged, because the border zeroing
		// would otherwise modify const data
		scaledBuffer = (GLbyte *)R_StaticAlloc( sizeof( unsigned ) * scaledWidth * scaledHeight );
		memcpy ( scaledBuffer, pic, width*height*4);
	} else {
		// resample down as needed (FIXME: this doesn't seem like it resamples anymore!)
		// scaledBuffer = R_ResampleTexture( pic, width, height, width >>= 1, height >>= 1 );
		scaledBuffer = R_MipMap( pic, width, height, preserveBorder );
		width >>= 1;
		height >>= 1;
		if ( width < 1 ) {
			width = 1;
		}
		if ( height < 1 ) {
			height = 1;
		}

		while ( width > scaledWidth || height > scaledHeight ) {
			GLbyte *shrunk = R_MipMap( scaledBuffer, width, height, preserveBorder );
			R_StaticFree( scaledBuffer );
			scaledBuffer = shrunk;

			width >>= 1;
			height >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}

		// one might have shrunk down below the target size
		scaledWidth = width;
		scaledHeight = height;
	}

	uploadHeight = scaledHeight;
	uploadWidth = scaledWidth;
	type = TT_2D;

	// zero the border if desired, allowing clamped projection textures
	// even after picmip resampling or careless artists.
	if ( repeat == TR_CLAMP_TO_ZERO ) {
		byte	rgba[4];
		rgba[0] = rgba[1] = rgba[2] = 0;
		rgba[3] = 255;
		R_SetBorderTexels( (GLbyte *)scaledBuffer, width, height, rgba );
	}
	if ( repeat == TR_CLAMP_TO_ZERO_ALPHA ) {
		byte	rgba[4];
		rgba[0] = rgba[1] = rgba[2] = 255;
		rgba[3] = 0;
		R_SetBorderTexels( (GLbyte *)scaledBuffer, width, height, rgba );
	}

	if ( generatorFunction == nullptr && ( depth == TD_BUMP && globalImages->image_writeNormalTGA.GetBool() || depth != TD_BUMP && globalImages->image_writeTGA.GetBool() ) ) {
		// Optionally write out the texture to a .tga
		char filename[MAX_IMAGE_NAME];
		ImageProgramStringToCompressedFileName( imgName, filename );
		char *ext = strrchr(filename, '.');
		if ( ext ) {
			strcpy( ext, ".tga" );
			// swap the red/alpha for the write
/*			if ( depth == TD_BUMP ) {
				for ( int i = 0; i < scaledWidth * scaledHeight * 4; i += 4 ) {
					scaledBuffer[i] = scaledBuffer[ i + 3 ];
					scaledBuffer[ i + 3 ] = 0;
				}
			}*/
			R_WriteTGA( filename, scaledBuffer, scaledWidth, scaledHeight, false );

			// put it back
/*			if ( depth == TD_BUMP ) {
				for ( int i = 0; i < scaledWidth * scaledHeight * 4; i += 4 ) {
					scaledBuffer[ i + 3 ] = scaledBuffer[i];
					scaledBuffer[i] = 0;
				}
			}*/
		}
	}

	// swap the red and alpha for rxgb support
	// do this even on tga normal maps so we only have to use
	// one fragment program
	// if the image is precompressed ( either in palletized mode or true rxgb mode )
	// then it is loaded above and the swap never happens here
	if ( depth == TD_BUMP && globalImages->image_useNormalCompression.GetInteger() != 1 ) {
		for ( int i = 0; i < scaledWidth * scaledHeight * 4; i += 4 ) {
			scaledBuffer[ i + 3 ] = scaledBuffer[i];
			scaledBuffer[i] = 0;
		}
	}
	// upload the main image level
	Bind();

	if ( internalFormat == GL_COLOR_INDEX8_EXT ) {
/*		if ( depth == TD_BUMP ) {
			for ( int i = 0; i < scaledWidth * scaledHeight * 4; i += 4 ) {
				scaledBuffer[i] = scaledBuffer[ i + 3 ];
				scaledBuffer[ i + 3 ] = 0;
			}
		}*/
		UploadCompressedNormalMap( scaledWidth, scaledHeight, scaledBuffer, 0 );
	} else {
		qglTexImage2D( GL_TEXTURE_2D, 0, internalFormat, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
	}

	// create and upload the mip map levels, which we do in all cases, even if we don't think they are needed
	int miplevel = 0;
	while ( scaledWidth > 1 || scaledHeight > 1 ) {
		// preserve the border after mip map unless repeating
		shrunk = R_MipMap( scaledBuffer, scaledWidth, scaledHeight, preserveBorder );
		R_StaticFree( scaledBuffer );
		scaledBuffer = shrunk;

		scaledWidth >>= 1;
		scaledHeight >>= 1;
		if ( scaledWidth < 1 ) {
			scaledWidth = 1;
		}
		if ( scaledHeight < 1 ) {
			scaledHeight = 1;
		}
		miplevel++;

		// this is a visualization tool that shades each mip map
		// level with a different color so you can see the
		// rasterizer's texture level selection algorithm
		// Changing the color doesn't help with lumminance/alpha/intensity formats...
		if ( depth == TD_DIFFUSE && globalImages->image_colorMipLevels.GetBool() ) {
			R_BlendOverTexture( (GLbyte *)scaledBuffer, scaledWidth * scaledHeight, mipBlendColors[miplevel] );
		}

		// upload the mip map
		if ( internalFormat == GL_COLOR_INDEX8_EXT ) {
			UploadCompressedNormalMap( scaledWidth, scaledHeight, scaledBuffer, miplevel );
		} else {
			qglTexImage2D( GL_TEXTURE_2D, miplevel, internalFormat, scaledWidth, scaledHeight,
				0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
		}
	}

	if ( scaledBuffer != 0 ) {
		R_StaticFree( scaledBuffer );
	}

	SetImageFilterAndRepeat();

	// see if we messed anything up
	GL_CheckErrors();
}

/*
==================
Generate3DImage

	TR_REPEAT TR_MIRROR TR_MIRROR_X TR_MIRROR_Y
	TR_CLAMP TR_CLAMP_X TR_CLAMP_Y
	TR_CLAMP_TO_BORDER TR_CLAMP_TO_ZERO TR_CLAMP_TO_ZERO_ALPHA
TF_LINEAR
TF_NEAREST
TF_LINEARNEAREST
TF_DEFAULT
==================
*/
void anImage::Generate3DImage( const GLbyte *pic, int width, int height, int picDepth, textureFilter_t filterParm, bool allowDownSizeParm, textureRepeat_t repeatParm, textureDepth_t minDepthParm ) {
	PurgeImage();

	filter = filterParm;
	allowDownSize = allowDownSizeParm;
	repeat = repeatParm;
	depth = minDepthParm;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if ( !qglConfig.isInitialized ) {
		return;
	}

	// make sure it is a power of 2
	int scaledWidth = MakePowerOfTwo( width );
	int scaledHeight = MakePowerOfTwo( height );
	int scaled_depth = MakePowerOfTwo( picDepth );
	if ( scaledWidth != width || scaledHeight != height || scaled_depth != picDepth ) {
		common->Error( "R_Create3DImage: not a power of 2 image" );
	}

	// FIXME: allow picmip here

	// generate the texture number
	qglGenTextures( 1, &texnum );

	// select proper internal format before we resample
	// this function doesn't need to know it is 3D, so just make it very "tall"
	internalFormat = SelectInternalFormat( &pic, 1, width, height * picDepth, minDepthParm, &isMonochrome );
	uploadHeight = scaledHeight;
	uploadWidth = scaledWidth;
	uploadDepth = scaled_depth;

	type = TT_3D;

	// upload the main image level
	Bind();

	qglTexImage3D( GL_TEXTURE_3D, 0, internalFormat, scaledWidth, scaledHeight, scaled_depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic );

	// create and upload the mip map levels
	GLbyte *scaledBuffer = (GLbyte *)R_StaticAlloc( scaledWidth * scaledHeight * scaled_depth * 4 );
	memcpy( scaledBuffer, pic, scaledWidth * scaledHeight * scaled_depth * 4 );
	int miplevel = 0;
	while ( scaledWidth > 1 || scaledHeight > 1 || scaled_depth > 1 ) {
		// preserve the border after mip map unless repeating
		GLbyte *shrunk = R_MipMap3D( scaledBuffer, scaledWidth, scaledHeight, scaled_depth, (bool)(repeat != TR_REPEAT) );
		R_StaticFree( scaledBuffer );
		scaledBuffer = shrunk;

		scaledWidth >>= 1;
		scaledHeight >>= 1;
		scaled_depth >>= 1;
		if ( scaledWidth < 1 ) {
			scaledWidth = 1;
		}
		if ( scaledHeight < 1 ) {
			scaledHeight = 1;
		}
		if ( scaled_depth < 1 ) {
			scaled_depth = 1;
		}
		miplevel++;
		// upload the mip map
		qglTexImage3D( GL_TEXTURE_3D, miplevel, internalFormat, scaledWidth, scaledHeight, scaled_depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
	}
	R_StaticFree( scaledBuffer );

	// set the minimize / maximize filtering
	switch ( filter ) {
	case TF_DEFAULT:
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter );
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter );
		break;
	case TF_LINEAR:
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		break;
	case TF_NEAREST:
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		break;
	case TF_LINEARNEAREST:
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST );
	default:
		common->FatalError( "R_CreateImage: bad texture filter" );
	}

	// set the wrap/clamp modes
	switch ( repeat ) {
	case TR_REPEAT:
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT );
		break;
	case TR_MIRRORED_REPEAT:
	case TR_CLAMP_TO_BORDER:
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
		break;
	case TR_CLAMP_TO_ZERO:
	case TR_CLAMP_TO_ZERO_ALPHA:
	case TR_CLAMP:
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
		break;
	default:
		common->FatalError( "R_CreateImage: bad texture repeat" );
	}

	Mem_Free();
	// see if we messed anything up
	GL_CheckErrors();
}

/*
====================
GenerateCubeImage

Non-square cube sides are not allowed
====================
*/
void anImage::GenerateCubeImage( const GLbyte *pic[6], int size, textureFilter_t filterParm, bool allowDownSizeParm, textureDepth_t depthParm ) {
	PurgeImage();

	filter = filterParm;
	allowDownSize = allowDownSizeParm;
	depth = depthParm;

	type = TT_CUBIC;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if ( !qglConfig.isInitialized ) {
		return;
	}

	if ( ! qglConfig.useCubeMap ) {
		return;
	}

	int width = int height = size;

	// generate the texture number
	qglGenTextures( 1, &texnum );

	// select proper internal format before we resample
	internalFormat = SelectInternalFormat( pic, 6, width, height, depth, &isMonochrome );

	// don't bother with downsample for now
	int scaledWidth = width;
	int scaledHeight = height;

	uploadHeight = scaledHeight;
	uploadWidth = scaledWidth;

	Bind();

	// no other clamp mode makes sense
	qglTexParameteri( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// set the minimize / maximize filtering
	switch ( filter ) {
	case TF_DEFAULT:
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter );
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter );
		break;
	case TF_LINEAR:
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		break;
	case TF_NEAREST:
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		break;
	case TF_LINEARNEAREST:
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameterf( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST );
		//qglTexParameteri( GL_TEXTURE_CUBE_MAP_EXT, GL_GENERATE_MIPMAP_SGIS,GL_TRUE );
		//qglTexParameteri( GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER,GL_NEAREST );

	default:
		common->FatalError( "R_CreateImage: bad texture filter" );
	}

	// upload the base level
	// FIXME: support GL_COLOR_INDEX8_EXT?
	for ( int i = 0; i < 6; i++ ) {
		qglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, internalFormat, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic[i] );
	}

	// create and upload the mip map levels
	GLbyte *shrunk[6];

	for ( int i = 0; i < 6; i++ ) {
		shrunk[i] = R_MipMap( pic[i], scaledWidth, scaledHeight, false );
	}

	int miplevel = 1;
	while ( scaledWidth > 1 ) {
		for ( i = 0; i < 6; i++ ) {
			qglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, miplevel, internalFormat, scaledWidth/2, scaledHeight/2, 0, GL_RGBA, GL_UNSIGNED_BYTE, shrunk[i] );
			if ( scaledWidth > 2 ) {
				GLbyte *shrunken = R_MipMap( shrunk[i], scaledWidth/2, scaledHeight/2, false );
			} else {
				GLbyte *shrunken = nullptr;
			}

			R_StaticFree( shrunk[i] );
			shrunk[i] = shrunken;
		}

		scaledWidth >>= 1;
		scaledHeight >>= 1;
		miplevel++;
	}

	// see if we messed anything up
	GL_CheckErrors();
}

/*
==================
NumLevelsForImageSize
==================
*/
int	anImage::NumLevelsForImageSize( int width, int height ) const {
	int	numLevels = 1;

	while ( width > 1 || height > 1 ) {
		numLevels++;
		width >>= 1;
		height >>= 1;
	}

	return numLevels;
}

int anImage::NumLevelsForImageSize( int width, int height, int internalFormat ) {
	int numLevels = 1;
	int minSize;

	switch ( internalFormat ) {
		case GL_RGBA8:
			minSize = 1;
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			minSize = 4;
			break;
		}

		while ( width > minSize || height > minSize ) {
			numLevels++;
			width >>= 1;
			height >>= 1;
	}

	return numLevels;
}

/*
==================
anImage::DataSizeForImageSize

FIXME: the switch statement is far from complete
==================
*/
int anImage::DataSizeForImageSize( int width, int height, int internalFormat ) {
	int numLevels = NumLevelsForImageSize( width, height, internalFormat );
	int dataSize = 0;

	for ( int i = 0; i < numLevels; i++ ) {
		switch ( internalFormat ) {
		case GL_RGBA8:
			dataSize += width * height * 4;
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			assert( width == height );
			dataSize += ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * 8;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			assert( width == height );
			dataSize += ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * 16;
			break;
		default:
			dataSize = -1;
		}
		width >>= 1;
		height >>= 1;
	}

	return dataSize;
}

/*
================
ImageProgramStringToFileCompressedFileName
================
*/
void anImage::ImageProgramStringToCompressedFileName( const char *imageProg, char *fileName ) const {
	strcpy( fileName, "dds/" );
	char *f = fileName + strlen( fileName );
	int depth = 0;

	// convert all illegal characters to underscores
	// this could conceivably produce a duplicated mapping, but we aren't going to worry about it
	for ( const char *s = imageProg; *s; s++ ) {
		if ( *s == '/' || *s == '\\' || *s == '(') {
			if ( depth < 4 ) {
				*f = '/';
				depth ++;
			} else {
				*f = ' ';
			}
			f++;
		} else if ( *s == '<' || *s == '>' || *s == ':' || *s == '|' || *s == '"' || *s == '.' ) {
			*f = '_';
			f++;
		} else if ( *s == ' ' && *(f-1 ) == '/' ) {	// ignore a space right after a slash
		} else if ( *s == ')' || *s == ',' ) {		// always ignore these
		} else {
			*f = *s;
			f++;
		}
	}
	*f++ = 0;
	strcat( fileName, ".dds" );
}

/*
================
WritePrecompressedImage

When we are happy with our source data, we can write out precompressed
versions of everything to speed future load times.
================
*/
void anImage::WritePrecompressedImage() {
	// Always write the precompressed image if we're making a build
	if ( !com_makingBuild.GetBool() ) {
		if ( !globalImages->image_writePrecompressedTextures.GetBool() || !globalImages->image_usePrecompressedTextures.GetBool() ) {
			return;
		}
	}

	if ( !qglConfig.isInitialized ) {
		return;
	}

	char filename[MAX_IMAGE_NAME];
	ImageProgramStringToCompressedFileName( imgName, filename );

	int numLevels = NumLevelsForImageSize( uploadWidth, uploadHeight );
	if ( numLevels > MAX_TEXTURE_LEVELS ) {
		common->Warning( "WritePrecompressedImage: level > MAX_TEXTURE_LEVELS for image %s", filename );
		return;
	}

	// glGetTexImage only supports a small subset of all the available internal formats
	// We have to use BGRA because DDS is a windows based format
	int altInternalFormat = 0;
	int bitSize = 0;
	switch ( internalFormat ) {
		case GL_COLOR_INDEX8_EXT:
		case GL_COLOR_INDEX:
			// this will not work with dds viewers but we need it in this format to save disk
			// load speed ( i.e. size )
			altInternalFormat = GL_COLOR_INDEX;
			bitSize = 24;
		break;
		case 1:
		case GL_INTENSITY8:
		case GL_LUMINANCE8:
		case 3:
		case GL_RGB8:
			altInternalFormat = GL_BGR_EXT;
			bitSize = 24;
		break;
		case GL_LUMINANCE8_ALPHA8:
		case 4:
		case GL_RGBA8:
			altInternalFormat = GL_BGRA_EXT;
			bitSize = 32;
		break;
		case GL_ALPHA8:
			altInternalFormat = GL_ALPHA;
			bitSize = 8;
		break;
		default:
			if ( FormatIsDXT( internalFormat ) ) {
				altInternalFormat = internalFormat;
			} else {
				common->Warning( "Unknown or unsupported format for %s", filename);
				return;
			}
	}

	if ( globalImages->image_useOffLineCompression.GetBool() && FormatIsDXT( altInternalFormat ) ) {
		anStr outFile = fileSystem->RelativePathToOSPath( filename, "fs_basepath" );
		anStr inFile = outFile;
		inFile.StripFileExtension();
		inFile.SetFileExtension( "tga" );
		anStr format;
		if ( depth == TD_BUMP ) {
			format = "RXGB +red 0.0 +green 0.5 +blue 0.5";
		} else {
			switch ( altInternalFormat ) {
				case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
					format = "DXT1";
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
					format = "DXT1 -alpha_threshold";
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
					format = "DXT3";
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
					format = "DXT5";
					break;
			}
		}
		globalImages->AddDDSCommand( va( "z:/compressor/thecompressor -convert \"%s\" \"%s\" %s -mipmaps\n", inFile.c_str(), outFile.c_str(), format.c_str() ) );
		return;
	}

	ddsFileProperties_t header;
	memset( &header, 0, sizeof(header) );
	header.dwSize = sizeof(header);
	header.dwFlags = DDSF_CAPS | DDSF_PIXELFORMAT | DDSF_WIDTH | DDSF_HEIGHT;
	header.dwHeight = uploadHeight;
	header.dwWidth = uploadWidth;

	// hack in our monochrome flag for the NV20 optimization
	if ( isMonochrome ) {
		header.dwFlags |= DDSF_ID_MONOCHROME;
	}

	if ( FormatIsDXT( altInternalFormat ) ) {
		// size (in bytes) of the compressed base image
		header.dwFlags |= DDSF_LINEARSIZE;
		header.dwPitchOrLinearSize = ( ( uploadWidth + 3 ) / 4 ) * ( ( uploadHeight + 3 ) / 4 )*
			(altInternalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);
	} else {
		// 4 Byte aligned line width (from nv_dds)
		header.dwFlags |= DDSF_PITCH;
		header.dwPitchOrLinearSize = ( ( uploadWidth * bitSize + 31 ) & -32 ) >> 3;
	}

	header.dwCaps1 = DDSF_TEXTURE;

	if ( numLevels > 1 ) {
		header.dwMipMapCount = numLevels;
		header.dwFlags |= DDSF_MIPMAPCOUNT;
		header.dwCaps1 |= DDSF_MIPMAP | DDSF_COMPLEX;
	}

	header.ddspf.dwSize = sizeof( header.ddspf );
	if ( FormatIsDXT( altInternalFormat ) ) {
		header.ddspf.dwFlags = DDSF_FOURCC;
		switch ( altInternalFormat ) {
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			header.ddspf.dwFourCC = DDS_MAKEFOURCC('D','X','T','1');
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			header.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
			header.ddspf.dwFourCC = DDS_MAKEFOURCC('D','X','T','1');
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			header.ddspf.dwFourCC = DDS_MAKEFOURCC('D','X','T','3');
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			header.ddspf.dwFourCC = DDS_MAKEFOURCC('D','X','T','5');
			break;
		}
	} else {
		header.ddspf.dwFlags = ( internalFormat == GL_COLOR_INDEX8_EXT ) ? DDSF_RGB | DDSF_ID_INDEXCOLOR : DDSF_RGB;
		header.ddspf.dwRGBBitCount = bitSize;
		switch ( altInternalFormat ) {
		case GL_BGRA_EXT:
		case GL_LUMINANCE_ALPHA:
			header.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
			header.ddspf.dwABitMask = 0xFF000000;
			// Fall through
		case GL_BGR_EXT:
		case GL_LUMINANCE:
		case GL_COLOR_INDEX:
			header.ddspf.dwRBitMask = 0x00FF0000;
			header.ddspf.dwGBitMask = 0x0000FF00;
			header.ddspf.dwBBitMask = 0x000000FF;
			break;
		case GL_ALPHA:
			header.ddspf.dwFlags = DDSF_ALPHAPIXELS;
			header.ddspf.dwABitMask = 0xFF000000;
			break;
		default:
			common->Warning( "Unknown or unsupported format for %s", filename );
			return;
		}
	}

	anFile *f = fileSystem->OpenFileWrite( filename );
	if ( f == nullptr ) {
		common->Warning( "Could not open %s trying to write precompressed image", filename );
		return;
	}
	common->Printf( "Writing precompressed image: %s\n", filename );

	f->Write( "DDS ", 4 );
	f->Write( &header, sizeof(header) );

	// bind to the image so we can read back the contents
	Bind();

	qglPixelStorei( GL_PACK_ALIGNMENT, 1 );	// otherwise small rows get padded to 32 bits

	int uw = uploadWidth;
	int uh = uploadHeight;

	// Will be allocated first time through the loop
	GLbyte *data = nullptr;

	for ( int level = 0; level < numLevels; level++ ) {
		int size = 0;
		if ( FormatIsDXT( altInternalFormat ) ) {
			size = ( ( uw + 3 ) / 4 ) * ( ( uh + 3 ) / 4 ) *
				( altInternalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16 );
		} else {
			size = uw * uh * (bitSize / 8);
		}

		if ( data == nullptr ) {
			data = (GLbyte *)R_StaticAlloc( size );
		}

		if ( FormatIsDXT( altInternalFormat ) ) {
			qglGetCompressedTexImageARB( GL_TEXTURE_2D, level, data );
		} else {
			qglGetTexImage( GL_TEXTURE_2D, level, altInternalFormat, GL_UNSIGNED_BYTE, data );
		}

		f->Write( data, size );

		uw /= 2;
		uh /= 2;
		if ( uw < 1 ) {
			uw = 1;
		}
		if ( uh < 1 ) {
			uh = 1;
		}
	}

	if ( data != nullptr ) {
		R_StaticFree( data );
	}

	fileSystem->CloseFile( f );
}

/*
================
ShouldImageBePartialCached

Returns true if there is a precompressed image, and it is large enough
to be worth caching
================
*/
bool anImage::ShouldImageBePartialCached() {
	if ( !qglConfig.textureCompression || !globalImages->image_useCache.GetBool() ) {
		return false;
	}

	// the allowDownSize flag does double-duty as don't-partial-load
	if ( !allowDownSize ) {
		return false;
	}

	if ( globalImages->image_cacheMinK.GetInteger() <= 0 ) {
		return false;
	}

	// if we are doing a copyFiles, make sure the original images are referenced
	if ( fileSystem->PerformingCopyFiles() ) {
		return false;
	}

	char	filename[MAX_IMAGE_NAME];
	ImageProgramStringToCompressedFileName( imgName, filename );

	// get the file timestamp
	fileSystem->ReadFile( filename, nullptr, &timestamp );

	if ( timestamp == FILE_NOT_FOUND_TIMESTAMP ) {
		return false;
	}

	// open it and get the file size
	anFile *f = fileSystem->OpenFileRead( filename );
	if ( !f ) {
		return false;
	}

	int	len = f->Length();
	fileSystem->CloseFile( f );

	if ( len <= globalImages->image_cacheMinK.GetInteger() * 1024 ) {
		return false;
	}

	// we do want to do a partial load
	return true;
}

/*
================
CheckPrecompressedImage

If fullLoad is false, only the small mip levels of the image will be loaded
================
*/
bool anImage::CheckPrecompressedImage( bool fullLoad ) {
	if ( !qglConfig.isInitialized || !qglConfig.textureCompression ) {
		return false;
	}

#if 1 //  disabled ) - Allow grabbing of DDS's from original Doom pak files
	// if we are doing a copyFiles, make sure the original images are referenced
	if ( fileSystem->PerformingCopyFiles() ) {
		return false;
	}
#endif

	if ( depth == TD_BUMP && globalImages->image_useNormalCompression.GetInteger() != 2 ) {
		return false;
	}

	// god i love last minute hacks :-)
	if ( com_machineSpec.GetInteger() >= 1 && com_videoRam.GetInteger() >= 128 && imgName.Icmpn( "lights/", 7 ) == 0 ) {
		return false;
	}

	char filename[MAX_IMAGE_NAME];
	ImageProgramStringToCompressedFileName( imgName, filename );

	// get the file timestamp
	ARC_TIME_T precompTimestamp;
	fileSystem->ReadFile( filename, nullptr, &precompTimestamp );

	if ( precompTimestamp == FILE_NOT_FOUND_TIMESTAMP ) {
		return false;
	}

	if ( !generatorFunction && timestamp != FILE_NOT_FOUND_TIMESTAMP ) {
		if ( precompTimestamp < timestamp ) {
			// The image has changed after being precompressed
			return false;
		}
	}

	timestamp = precompTimestamp;

	// open it and just read the header
	anFile *f = fileSystem->OpenFileRead( filename );
	if ( !f ) {
		return false;
	}

	int	len = f->Length();
	if ( len < sizeof( ddsFileProperties_t ) ) {
		fileSystem->CloseFile( f );
		return false;
	}

	if ( !fullLoad && len > globalImages->image_cacheMinK.GetInteger() * 1024 ) {
		len = globalImages->image_cacheMinK.GetInteger() * 1024;
	}

	GLbyte *data = (GLbyte *)R_StaticAlloc( len );

	f->Read( data, len );
	fileSystem->CloseFile( f );

	unsigned long magic = LittleLong( *(unsigned long *)data );
	ddsFileProperties_t	*_header = (ddsFileProperties_t *)( data + 4);
	int ddspf_dwFlags = LittleLong( _header->ddspf.dwFlags );

	if ( magic != DDS_MAKEFOURCC('D', 'D', 'S', ' ') ) {
		common->Printf( "CheckPrecompressedImage( %s ): magic != 'DDS '\n", imgName.c_str() );
		R_StaticFree( data );
		return false;
	}

	// if we don't support color index textures, we must load the full image
	// should we just expand the 256 color image to 32 bit for upload?
	if ( ddspf_dwFlags & DDSF_ID_INDEXCOLOR && !qglConfig.isSharedTPalette ) {
		R_StaticFree( data );
		return false;
	}

	// upload all the levels
	UploadPrecompressedImage( data, len );

	R_StaticFree( data );

	return true;
}

/*
===================
UploadPrecompressedImage

This can be called by the front end during nromal loading,
or by the backend after a background read of the file
has completed
===================
*/
void anImage::UploadPrecompressedImage( GLbyte *data, int len ) {
	ddsFileProperties_t	*header = (ddsFileProperties_t *)( data + 4 );

	// ( not byte swapping dwReserved1 dwReserved2 )
	header->dwSize = LittleLong( header->dwSize );
	header->dwFlags = LittleLong( header->dwFlags );
	header->dwHeight = LittleLong( header->dwHeight );
	header->dwWidth = LittleLong( header->dwWidth );
	header->dwPitchOrLinearSize = LittleLong( header->dwPitchOrLinearSize );
	header->dwDepth = LittleLong( header->dwDepth );
	header->dwMipMapCount = LittleLong( header->dwMipMapCount );
	header->dwCaps1 = LittleLong( header->dwCaps1 );
	header->dwCaps2 = LittleLong( header->dwCaps2 );

	header->ddspf.dwSize = LittleLong( header->ddspf.dwSize );
	header->ddspf.dwFlags = LittleLong( header->ddspf.dwFlags );
	header->ddspf.dwFourCC = LittleLong( header->ddspf.dwFourCC );
	header->ddspf.dwRGBBitCount = LittleLong( header->ddspf.dwRGBBitCount );
	header->ddspf.dwRBitMask = LittleLong( header->ddspf.dwRBitMask );
	header->ddspf.dwGBitMask = LittleLong( header->ddspf.dwGBitMask );
	header->ddspf.dwBBitMask = LittleLong( header->ddspf.dwBBitMask );
	header->ddspf.dwABitMask = LittleLong( header->ddspf.dwABitMask );

	// generate the texture number
	qglGenTextures( 1, &texnum );

	int externalFormat = 0;
	precompressedFile = true;
	uploadWidth = header->dwWidth;
	uploadHeight = header->dwHeight;

	 if ( header->ddspf.dwFlags & DDSF_FOURCC ) {
		  switch ( header->ddspf.dwFourCC ) {
		  case DDS_MAKEFOURCC( 'D', 'X', 'T', '1' ):
			if ( header->ddspf.dwFlags & DDSF_ALPHAPIXELS ) {
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			} else {
				internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			}
				break;
		  case DDS_MAKEFOURCC( 'D', 'X', 'T', '3' ):
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
		  case DDS_MAKEFOURCC( 'D', 'X', 'T', '5' ):
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
		case DDS_MAKEFOURCC( 'R', 'X', 'G', 'B' ):
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		  default:
				common->Warning( "Invalid compressed internal format\n" );
				return;
		  }
	 } else if ( ( header->ddspf.dwFlags & DDSF_RGBA ) && header->ddspf.dwRGBBitCount == 32 ) {
		externalFormat = GL_BGRA_EXT;
		internalFormat = GL_RGBA8;
	 } else if ( ( header->ddspf.dwFlags & DDSF_RGB ) && header->ddspf.dwRGBBitCount == 32 ) {
		  externalFormat = GL_BGRA_EXT;
		internalFormat = GL_RGBA8;
	 } else if ( ( header->ddspf.dwFlags & DDSF_RGB ) && header->ddspf.dwRGBBitCount == 24 ) {
		if ( header->ddspf.dwFlags & DDSF_ID_INDEXCOLOR ) {
			externalFormat = GL_COLOR_INDEX;
			internalFormat = GL_COLOR_INDEX8_EXT;
		} else {
			externalFormat = GL_BGR_EXT;
			internalFormat = GL_RGB8;
		}
	} else if ( header->ddspf.dwRGBBitCount == 8 ) {
		externalFormat = GL_ALPHA;
		internalFormat = GL_ALPHA8;
	} else {
		common->Warning( "Invalid uncompressed internal format\n" );
		return;
	}

	// we need the monochrome flag for the NV20 optimized path
	if ( header->dwFlags & DDSF_ID_MONOCHROME ) {
		isMonochrome = true;
	}

	type = TT_2D;			// FIXME: we may want to support pre-compressed cube maps in the future

	Bind();

	int numMipmaps = 1;
	if ( header->dwFlags & DDSF_MIPMAPCOUNT ) {
		numMipmaps = header->dwMipMapCount;
	}

	int uw = uploadWidth;
	int uh = uploadHeight;

	// We may skip some mip maps if we are downsizing
	int skipMip = 0;
	GetDownsize( uploadWidth, uploadHeight );

	GLbyte *imagedata = data + sizeof( ddsFileProperties_t ) + 4;

	for ( int i = 0; i < numMipmaps; i++ ) {
		int size = 0;
		if ( FormatIsDXT( internalFormat ) ) {
			size = ( ( uw + 3 ) / 4 ) * ( ( uh + 3 ) / 4 ) *
				( internalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16 );
		} else {
			size = uw * uh * ( header->ddspf.dwRGBBitCount / 8 );
		}

		if ( uw > uploadWidth || uh > uploadHeight ) {
			skipMip++;
		} else {
			if ( FormatIsDXT( internalFormat ) ) {
				qglCompressedTexImage2DARB( GL_TEXTURE_2D, i - skipMip, internalFormat, uw, uh, 0, size, imagedata );
			} else {
				qglTexImage2D( GL_TEXTURE_2D, i - skipMip, internalFormat, uw, uh, 0, externalFormat, GL_UNSIGNED_BYTE, imagedata );
			}
		}

		imagedata += size;
		uw /= 2;
		uh /= 2;
		if ( uw < 1 ) {
			uw = 1;
		}
		if ( uh < 1 ) {
			uh = 1;
		}
	}
	SetImageFilterAndRepeat();
}

/*
===============
ActuallyLoadImage

Absolutely every image goes through this path
On exit, the anImage will have a valid OpenGL texture number that can be bound
===============
*/
void anImage::ActuallyLoadImage( bool checkForPrecompressed, bool fromBackEnd ) {
	int width, height;
	GLbyte *pic;
	//Byte pic = nullptr;

	//if ( imgName.equals( "guis/assets/splash/launch" ) ) {
	//	return;
	//}

	// this is the ONLY place generatorFunction will ever be called
	if ( generatorFunction ) {
		generatorFunction( this );
		return;
	}

	// if we are a partial image, we are only going to load from a compressed file
	if ( isPartialImage ) {
		if ( CheckPrecompressedImage( false ) ) {
			return;
		}
		// this is an error -- the partial image failed to load
		MakeDefault();
		return;
	}

	//
	// load the image from disk
	//
	if ( cubeFiles != CF_2D ) {
		GLbyte *pics[6];
		// we don't check for pre-compressed cube images currently
		R_LoadCubeImages( imgName, cubeFiles, pics, &width, &timestamp );
		if ( pics[0] == nullptr ) {
			common->Warning( "Couldn't load cube image: %s", imgName.c_str() );
			MakeDefault();
			return;
		}

		GenerateCubeImage( (const GLbyte **)pics, width, filter, allowDownSize, depth );
		precompressedFile = false;

		for ( int i = 0; i < 6; i++ ) {
			if ( pics[i] ) {
				R_StaticFree( pics[i] );
			}
		}
	} else {
		// see if we have a pre-generated image file that is
		// already image processed and compressed
		if ( checkForPrecompressed && globalImages->image_usePrecompressedTextures.GetBool() ) {
			if ( CheckPrecompressedImage( true ) ) {
				// we got the precompressed image
				return;
			}
			// fall through to load the normal image
		}

		R_LoadImageProgram( imgName, &pic, &width, &height, &timestamp, &depth );

		if ( pic == nullptr ) {
			common->Warning( "Couldn't load image: %s", imgName.c_str() );
			MakeDefault();
			return;
		}
/*		// swap the red and alpha for rxgb support
		// do this even on tga normal maps so we only have to use
		// one fragment program
		// if the image is precompressed ( either in palletized mode or true rxgb mode )
		// then it is loaded above and the swap never happens here
		if ( depth == TD_BUMP && globalImages->image_useNormalCompression.GetInteger() != 1 ) {
			for ( int i = 0; i < width * height * 4; i += 4 ) {
				pic[ i + 3 ] = pic[i];
				pic[i] = 0;
			}
		}*/
		// build a hash for checking duplicate image files
		// NOTE: takes about 10% of image load times (SD)
		// may not be strictly necessary, but some code uses it, so let's leave it in
		imageHash = MD4_BlockChecksum( pic, width * height * 4 );

		GenerateImage( pic, width, height, filter, allowDownSize, repeat, depth );
		timestamp = timestamp;
		precompressedFile = false;

		R_StaticFree( pic );

		// write out the precompressed version of this file if needed
		WritePrecompressedImage();
	}
}

//=========================================================================================================

/*
========================
anImage::ReadImage
========================
*/
void anImage::ReadImage( int x, int y, int width, int height, int slice, byte *buffer) {
	Bind();

	if ( opts.textureType == TT_CUBIC ) {
		qglGetTexImage( GL_TEXTURE_CUBE_MAP_POSITIVE_X + slice, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer );
	} else if ( opts.textureType == TT_2D ) {
		qglGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer );
	} else {
		common->FatalError( "Image::ReadImage: Unsupported texture type!" );
	}

	globalImages->BindNull();
}

/*
========================
anImage::AllocImage

Every image will pass through this function. Allocates all the necessary MipMap levels for the
Image, but doesn't put anything in them.

This should not be done during normal game-play, if you can avoid it.
========================
*/
void anImage::AllocImage() {
	GL_CheckErrors();
	PurgeImage();

	anScopedCriticalSection lock( com_loadScreenMutex );
	anScopedLoadContext scopedContext;

	switch ( opts.format ) {
	case FMT_RGBA8:
		internalFormat = GL_RGBA8;
		dataFormat = GL_RGBA;
		dataType = GL_UNSIGNED_BYTE;
		break;
	case FMT_XRGB8:
		internalFormat = GL_RGB;
		dataFormat = GL_RGBA;
		dataType = GL_UNSIGNED_BYTE;
		break;
	case FMT_RGB565:
		internalFormat = GL_RGB;
		dataFormat = GL_RGB;
		dataType = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case FMT_RGBA16:
		internalFormat = GL_RGB16UI;
		dataFormat = GL_RGBA_INTEGER;
		dataType = GL_UNSIGNED_SHORT;
		break;
	case FMT_RGBAF16:
		internalFormat = GL_RGBA16F;
		dataFormat = GL_RGBA;
		dataType = GL_HALF_FLOAT;
		break;
	case FMT_RG16:
		internalFormat = GL_RG16UI;
		dataFormat = GL_RG_INTEGER;
		dataType = GL_UNSIGNED_SHORT;
		break;
	case FMT_RG32:
		internalFormat = GL_RG32F;
		dataFormat = GL_RG;
		dataType = GL_FLOAT;
		break;
	case FMT_R32:
		internalFormat = GL_R32F;
		dataFormat = GL_RED;
		dataType = GL_FLOAT;
		break;
	case FMT_DEPTH32:
		internalFormat = GL_DEPTH_COMPONENT32F;
		dataFormat = GL_DEPTH_COMPONENT;
		dataType = GL_FLOAT;
		break;
	case FMT_ALPHA:
#if defined( USE_CORE_PROFILE )
		internalFormat = GL_R8;
		dataFormat = GL_RED;
#else
		internalFormat = GL_ALPHA8;
		dataFormat = GL_ALPHA;
#endif
		dataType = GL_UNSIGNED_BYTE;
		break;
	case FMT_L8A8:
#if defined( USE_CORE_PROFILE )
		internalFormat = GL_RG8;
		dataFormat = GL_RG;
#else
		internalFormat = GL_LUMINANCE8_ALPHA8;
		dataFormat = GL_LUMINANCE_ALPHA;
#endif
		dataType = GL_UNSIGNED_BYTE;
		break;
	case FMT_LUM8:
#if defined( USE_CORE_PROFILE )
		internalFormat = GL_R8;
		dataFormat = GL_RED;
#else
		internalFormat = GL_LUMINANCE8;
		dataFormat = GL_LUMINANCE;
#endif
		dataType = GL_UNSIGNED_BYTE;
		break;
	case FMT_INT8:
#if defined( USE_CORE_PROFILE )
		internalFormat = GL_R8;
		dataFormat = GL_RED;
#else
		internalFormat = GL_INTENSITY8;
		dataFormat = GL_LUMINANCE;
#endif
		dataType = GL_UNSIGNED_BYTE;
		break;
	case FMT_DXT1:
		internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		dataFormat = GL_RGBA;
		dataType = GL_UNSIGNED_BYTE;
		break;
	case FMT_DXT5:
		internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		dataFormat = GL_RGBA;
		dataType = GL_UNSIGNED_BYTE;
		break;
	case FMT_DEPTH:
		internalFormat = GL_DEPTH_COMPONENT;
		dataFormat = GL_DEPTH_COMPONENT;
		dataType = GL_UNSIGNED_BYTE;
		break;
	case FMT_DEPTH_STENCIL:
		internalFormat = GL_DEPTH24_STENCIL8;
		dataFormat = GL_DEPTH_STENCIL;
		dataType = GL_UNSIGNED_INT_24_8;
		break;
	case FMT_X16:
		internalFormat = GL_INTENSITY16;
		dataFormat = GL_LUMINANCE;
		dataType = GL_UNSIGNED_SHORT;
		break;
	case FMT_Y16_X16:
		internalFormat = GL_LUMINANCE16_ALPHA16;
		dataFormat = GL_LUMINANCE_ALPHA;
		dataType = GL_UNSIGNED_SHORT;
		break;
	default:
		idLib::Error( "Unhandled image format %d in %s\n", opts.format, GetName() );
	}

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if ( !renderSystem->IsOpenGLRunning() ) {
		return;
	}

	// generate the texture number
	qglGenTextures( 1, (GLuint *)&texnum );
	assert( texnum != TEXTURE_NOT_LOADED );

	//----------------------------------------------------
	// allocate all the mip levels with nullptr data
	//----------------------------------------------------

	int numSides;
	int target;
	int uploadTarget;
	if ( opts.textureType == TT_2D ) {
		if (opts.numMSAASamples == 0 ) {
			target = uploadTarget = GL_TEXTURE_2D;
		} else {
			target = uploadTarget = GL_TEXTURE_2D_MULTISAMPLE;
		}
		numSides = 1;
	} else if ( opts.textureType == TT_CUBIC ) {
		target = GL_TEXTURE_CUBE_MAP_EXT;
		uploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT;
		numSides = 6;
	} else {
		assert( !"opts.textureType" );
		target = uploadTarget = GL_TEXTURE_2D;
		numSides = 1;
	}

	qglBindTexture( target, texnum );

	for ( int side = 0; side < numSides; side++ ) {
		int w = opts.width;
		int h = opts.height;
		if ( opts.textureType == TT_CUBIC ) {
			h = w;
		}
		for ( int level = 0; level < opts.numLevels; level++ ) {
			// clear out any previous error
			GL_CheckErrors();
			if ( IsCompressed() ) {
				int compressedSize = ( ( ( w+3 )/4 ) * ( ( h+3 )/4 ) * int64_t( 16 ) * BitsForFormat( opts.format ) ) / 8;
				// Even though the OpenGL specification allows the 'data' pointer to be nullptr, for some
				// drivers we actually need to upload data to get it to allocate the texture.
				// However, on 32-bit systems we may fail to allocate a large block of memory for large
				// textures. We handle this case by using HeapAlloc directly and allowing the allocation
				// to fail in which case we simply pass down nullptr to glCompressedTexImage2D and hope for the best.
				// As of 2011-10-6 using NVIDIA hardware and drivers we have to allocate the memory with HeapAlloc
				// with the exact size otherwise large image allocation (for instance for physical page textures)
				// may fail on Vista 32-bit.
				void * data = HeapAlloc( GetProcessHeap(), 0, compressedSize );
				glCompressedTexImage2DARB( uploadTarget+side, level, internalFormat, w, h, 0, compressedSize, data );
				if ( data != nullptr ) {
					HeapFree( GetProcessHeap(), 0, data );
				}
			} else {
				if ( opts.numMSAASamples == 0 ) {
					qglTexImage2D( uploadTarget + side, level, internalFormat, w, h, 0, dataFormat, dataType, nullptr );
				} else {
					if (opts.textureType == TT_2D ) {
						qglTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, opts.numMSAASamples, internalFormat, w, h, false );
					} else {
						common->FatalError( "Unsupported MSAA texture type!\n" );
					}
				}
			}

			GL_CheckErrors();

			w = Max( 1, w >> 1 );
			h = Max( 1, h >> 1 );
		}
	}

	qglTexParameteri( target, GL_TEXTURE_MAX_LEVEL, opts.numLevels - 1 );

	// see if we messed anything up
	GL_CheckErrors();

	SetTexParameters();

	GL_CheckErrors();
}

/*
========================
anImage::Resize
========================
*/
void anImage::Resize( int width, int height ) {
	if ( opts.width == width && opts.height == height ) {
		return;
	}
	opts.width = width;
	opts.height = height;
	AllocImage();
}

/*
===============
PurgeImage
===============
*/
void anImage::PurgeImage() {
	anScopedCriticalSection lock( com_loadScreenMutex );
	anScopedLoadContext scopedContext;
	if ( texnum != TEXTURE_NOT_LOADED ) {
		qglDeleteTextures( 1, (GLuint *)&texnum );	// this should be the ONLY place it is ever called!
		texnum = TEXTURE_NOT_LOADED;
	}
	// clear all the current binding caches, so the next bind will do a real one
	for ( int i = 0; i < MAX_MULTITEXTURE_UNITS; i++ ) {
		backEnd.qglState.tmu[i].current2DMap = -1;//= TEXTURE_NOT_LOADED;
		backEnd.qglState.tmu[i].current3DMap = -1;//= TEXTURE_NOT_LOADED;
		backEnd.qglState.tmu[i].currentCubeMap = -1;//= TEXTURE_NOT_LOADED;
	}
/*	qglDeleteTextures( 1, &image->texnum );

	R_CacheImageFree( image );

	memset( qglState.currenttmu, 0, sizeof( qglState.currenttmu ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SetCurrentTextureUnit( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SetCurrentTextureUnit( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}*/
}

/*
==============
Bind

Automatically enables 2D mapping, cube mapping, or 3D texturing if needed
==============
*/
void anImage::Bind() {
	if ( tr.logFile ) {
		RB_LogComment( "anImage::Bind( %s )\n", imgName.c_str() );
		ActuallyLoadImage( true );
	}

	// if this is an image that we are caching, move it to the front of the LRU chain
	if ( partialImage ) {
		if ( cacheUsageNext ) {
			// unlink from old position
			cacheUsageNext->cacheUsagePrev = cacheUsagePrev;
			cacheUsagePrev->cacheUsageNext = cacheUsageNext;
		}
		// link in at the head of the list
		cacheUsageNext = globalImages->cacheLRU.cacheUsageNext;
		cacheUsagePrev = &globalImages->cacheLRU;

		cacheUsageNext->cacheUsagePrev = this;
		cacheUsagePrev->cacheUsageNext = this;
	}

	// load the image if necessary (FIXME: not SMP safe!)
	if ( texnum == TEXTURE_NOT_LOADED ) {
		if ( !IsLoaded() && partialImage ) {
			// if we have a partial image, go ahead and use that
			this->partialImage->Bind();
			// start a background load of the full thing if it isn't already in the queue
			if ( !backgroundLoadInProgress ) {
				StartBackgroundImageLoad();
			}
			return;
		}

		// load the image on demand here, which isn't our normal game operating mode
		ActuallyLoadImage( true, true );	// check for precompressed, load is from back end
	}

	// bump our statistic counters
	frameUsed = backEnd.frameCount;
	bindCount++;

	tmu_t *tmu = &backEnd.qglState.tmu[backEnd.qglState.currenttmu];

	// enable or disable apropriate texture modes
	if ( tmu->textureType != type && ( backEnd.qglState.currenttmu < qglConfig.maxImageUnits ) ) {
		if ( tmu->textureType == TT_CUBIC ) {
			qglDisable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
		} else if ( tmu->textureType == TT_3D ) {
			qglDisable( GL_TEXTURE_3D );
		} else if ( tmu->textureType == TT_2D ) {
			qglDisable( GL_TEXTURE_2D );
		//} else if ( tmu->textureType == TT_RECT ) {
			//qglDisable( GL_TEXTURE_BINDING_RECTANGLE_ARB );
		}
		if ( type == TT_CUBIC ) {
			qglEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
		} else if ( type == TT_3D ) {
			qglEnable( GL_TEXTURE_3D );
		} else if ( type == TT_2D ) {
			qglEnable( GL_TEXTURE_2D );
		}
		tmu->textureType = type;
	}

	// bind the texture
	if ( type == TT_2D ) {
		if ( tmu->current2DMap != texnum ) {
			tmu->current2DMap = texnum;
			qglBindTexture( GL_TEXTURE_2D, texnum );
		}
	} else if ( type == TT_CUBIC ) {
		if ( tmu->currentCubeMap != texnum ) {
			tmu->currentCubeMap = texnum;
			qglBindTexture( GL_TEXTURE_CUBE_MAP_SEAMLESS, texnum );
		}
	} else if ( type == TT_3D ) {
		if ( tmu->current3DMap != texnum ) {
			tmu->current3DMap = texnum;
			qglBindTexture( GL_TEXTURE_3D, texnum );
		}
	}

	if ( com_purgeAll.GetBool() ) {
		GLclampf priority = 1.0f;
		qglPrioritizeTextures( 1, &texnum, &priority );
	}
}

/*
==============
BindFragment

Fragment programs explicitly say which type of map they want, so we don't need to
do any enable / disable changes
==============
*/
void anImage::BindFragment() {
	if ( tr.logFile ) {
		RB_LogComment( "anImage::BindFragment %s )\n", imgName.c_str() );
	}

	// if this is an image that we are caching, move it to the front of the LRU chain
	if ( partialImage ) {
		if ( cacheUsageNext ) {
			// unlink from old position
			cacheUsageNext->cacheUsagePrev = cacheUsagePrev;
			cacheUsagePrev->cacheUsageNext = cacheUsageNext;
		}
		// link in at the head of the list
		cacheUsageNext = globalImages->cacheLRU.cacheUsageNext;
		cacheUsagePrev = &globalImages->cacheLRU;

		cacheUsageNext->cacheUsagePrev = this;
		cacheUsagePrev->cacheUsageNext = this;
	}

	// load the image if necessary (FIXME: not SMP safe!)
	if ( texnum == TEXTURE_NOT_LOADED ) {
		if ( partialImage ) {
			// if we have a partial image, go ahead and use that
			this->partialImage->BindFragment();
			// start a background load of the full thing if it isn't already in the queue
			if ( !backgroundLoadInProgress ) {
				StartBackgroundImageLoad();
			}
			return;
		}

		// load the image on demand here, which isn't our normal game operating mode
		ActuallyLoadImage( true, true );	// check for precompressed, load is from back end
	}

	// bump our statistic counters
	frameUsed = backEnd.frameCount;
	bindCount++;

	// bind the texture
	if ( type == TT_2D ) {
		qglBindTexture( GL_TEXTURE_2D, texnum );
	} else if ( type == TT_RECT ) {
		qglBindTexture( GL_TEXTURE_RECTANGLE, texnum );
	} else if ( type == TT_CUBIC ) {
		qglBindTexture( GL_TEXTURE_CUBE_MAP_SEAMLESS, texnum );
	} else if ( type == TT_3D ) {
		qglBindTexture( GL_TEXTURE_3D, texnum );
	}
}

/*
====================
CopyFramebuffer
====================
*/
void anImage::CopyFramebuffer( int x, int y, int imageWidth, int imageHeight, bool useOversizedBuffer ) {
	Bind();

	if ( cvarSystem->GetCVarBool( "g_lowresFullscreen" ) ) {
		imageWidth = 512;
		imageHeight = 512;
	}

	// if the size isn't a power of 2, the image must be increased in size
	int potWidth = MakePowerOfTwo( imageWidth );
	int potHeight = MakePowerOfTwo( imageHeight );

	GetDownsize( imageWidth, imageHeight );
	GetDownsize( potWidth, potHeight );

	qglReadBuffer( GL_BACK );

	// only resize if the current dimensions can't hold it at all,
	// otherwise subview renderings could thrash this
	if ( ( useOversizedBuffer && ( uploadWidth < potWidth || uploadHeight < potHeight ) )
		|| ( !useOversizedBuffer && ( uploadWidth != potWidth || uploadHeight != potHeight ) ) ) {
		uploadWidth = potWidth;
		uploadHeight = potHeight;
		if ( potWidth == imageWidth && potHeight == imageHeight ) {
			qglCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, x, y, imageWidth, imageHeight, 0 );
		} else {
			// we need to create a dummy image with power of two dimensions,
			// then do a qglCopyTexSubImage2D of the data we want
			// this might be a 16+ meg allocation, which could fail on _alloca
			GLbyte *junk = (GLbyte *)Mem_Alloc( potWidth * potHeight * 4 );
			memset( junk, 0, potWidth * potHeight * 4 );		//!@#
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, potWidth, potHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, junk );
			Mem_Free( junk );

			qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );
		}
	} else {
		// otherwise, just subimage upload it so that drivers can tell we are going to be changing
		// it and don't try and do a texture compression or some other silliness
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );
	}

	// if the image isn't a full power of two, duplicate an extra row and/or column to fix bilerps
	if ( imageWidth != potWidth ) {
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, imageWidth, 0, x+imageWidth-1, y, 1, imageHeight );
	}
	if ( imageHeight != potHeight ) {
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, imageHeight, x, y+imageHeight-1, imageWidth, 1 );
	}

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	backEnd.c_copyFrameBuffer++;
}

/*
====================
CopyDepthbuffer

This should just be part of copyFramebuffer once we have a proper image type field
====================
*/
void anImage::CopyDepthbuffer( int x, int y, int imageWidth, int imageHeight ) {
	Bind();

	// if the size isn't a power of 2, the image must be increased in size
	int potWidth = MakePowerOfTwo( imageWidth );
	int potHeight = MakePowerOfTwo( imageHeight );

	if ( uploadWidth != potWidth || uploadHeight != potHeight ) {
		uploadWidth = potWidth;
		uploadHeight = potHeight;
		if ( potWidth == imageWidth && potHeight == imageHeight ) {
			qglCopyTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, x, y, imageWidth, imageHeight, 0 );
		} else {
			// we need to create a dummy image with power of two dimensions,
			// then do a qglCopyTexSubImage2D of the data we want
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, potWidth, potHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr );
			qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );
		}
	} else {
		// otherwise, just subimage upload it so that drivers can tell we are going to be changing
		// it and don't try and do a texture compression or some other silliness
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );
	}

//	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
//	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

/*
=============
RB_UploadScratchImage

if rows = cols * 6, assume it is a cube map animation
=============
*/
void anImage::UploadScratch( const GLbyte *data, int cols, int rows ) {
	// if rows = cols * 6, assume it is a cube map animation
	if ( rows == cols * 6 ) {
		if ( type != TT_CUBIC ) {
			type = TT_CUBIC;
			uploadWidth = -1;	// for a non-sub upload
		}

		Bind();

		rows /= 6;
		// if the scratchImage isn't in the format we want, specify it as a new texture
		if ( cols != uploadWidth || rows != uploadHeight ) {
			uploadWidth = cols;
			uploadHeight = rows;

			// upload the base level
			for ( int i = 0; i < 6; i++ ) {
				qglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + cols*rows*4*i );
			}
		} else {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			for ( int i = 0; i < 6; i++ ) {
				qglTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, 0, 0, cols, rows,
					GL_RGBA, GL_UNSIGNED_BYTE, data + cols*rows*4*i );
			}
		}
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		// no other clamp mode makes sense
		qglTexParameteri( GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameteri( GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	} else {
		// otherwise, it is a 2D image
		if ( type != TT_2D ) {
			type = TT_2D;
			uploadWidth = -1;	// for a non-sub upload
		}

		Bind();

		// if the scratchImage isn't in the format we want, specify it as a new texture
		if ( cols != uploadWidth || rows != uploadHeight ) {
			uploadWidth = cols;
			uploadHeight = rows;
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		} else {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		// these probably should be clamp, but we have a lot of issues with editor
		// geometry coming out with texcoords slightly off one side, resulting in
		// a smear across the entire polygon
#if 1
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
#else
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
#endif
	}
}

void anImage::SetClassification( int tag ) {
	classification = tag;
}

/*
==================
StorageSize
==================
*/
int anImage::StorageSize() const {
	if ( texnum == TEXTURE_NOT_LOADED ) {
		return 0;
	}

	switch ( type ) {
	default:
	case TT_2D:
		int baseSize = uploadWidth*uploadHeight;
		break;
	case TT_3D:
		int baseSize = uploadWidth*uploadHeight*uploadDepth;
		break;
	case TT_CUBIC:
		int baseSize = 6 * uploadWidth*uploadHeight;
		break;
	}

	baseSize *= BitsForInternalFormat( internalFormat );
	baseSize /= 8;

	// account for mip mapping
	baseSize = baseSize * 4 / 3;

	return baseSize;
}

/*
==================
Print
==================
*/
void anImage::Print( bool csv ) const {
	if ( precompressedFile ) {
		common->Printf( "P" );
	} else if ( generatorFunction ) {
		common->Printf( "F" );
	} else {
		common->Printf( " " );
	}

	switch ( type ) {
	case TT_2D:
		common->Printf( " " );
		break;
	case TT_3D:
		common->Printf( "3" );
		break;
	case TT_CUBIC:
		common->Printf( "C" );
		break;
	case TT_RECT:
		common->Printf( "R" );
		break;
	default:
		common->Printf( "<BAD TYPE:%i>", type );
		break;
	}
	if ( csv ) {
		common->Printf( "%4i %4i ",	uploadWidth, uploadHeight );
	}
	switch ( filter ) {
	case TF_DEFAULT:
		common->Printf( "dflt " );
		break;
	case TF_LINEAR:
		common->Printf( "linr " );
		break;
	case TF_NEAREST:
		common->Printf( "nrst " );
		break;
		case TF_LINEAR_NEAREST:
		common->Printf( "linrnrst " );
		break;
	default:
		common->Printf( "<BAD FILTER:%i>", filter );
		break;
	}

	switch ( internalFormat ) {
	case GL_INTENSITY8:
	case 1:
		common->Printf( "I	  " );
		break;
	case 2:
	case GL_LUMINANCE8_ALPHA8:
		common->Printf( "LA	 " );
		break;
	case 3:
		common->Printf( "RGB	" );
		break;
	case 4:
		common->Printf( "RGBA  " );
		break;
	case GL_LUMINANCE8:
		common->Printf( "L	  " );
		break;
	case GL_ALPHA8:
		common->Printf( "A	  " );
		break;
	case GL_RGBA8:
		common->Printf( "RGBA8 " );
		break;
	case GL_RGB8:
		common->Printf( "RGB8  " );
		break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		common->Printf( "DXT1  " );
		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		common->Printf( "DXT1A " );
		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		common->Printf( "DXT3  " );
		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		common->Printf( "DXT5  " );
		break;
	case GL_RGBA4:
		common->Printf( "RGBA4 " );
		break;
	case GL_RGB5:
		common->Printf( "RGB5  " );
		break;
	case GL_COLOR_INDEX8_EXT:
		common->Printf( "CI8	" );
		break;
	case GL_COLOR_INDEX:
		common->Printf( "CI	 " );
		break;
	case GL_COMPRESSED_RGB_ARB:
		common->Printf( "RGBC  " );
		break;
	case GL_COMPRESSED_RGBA_ARB:
		common->Printf( "RGBAC " );
		break;
	case 0:
		common->Printf( "		" );
		break;
	default:
		common->Printf( "<BAD FORMAT:%i>", internalFormat );
		break;
	}

	switch ( repeat ) {
	case TR_REPEAT:
		common->Printf( "repeat" );
		break;
	case TR_MIRRORED_REPEAT:
		common->Printf( "mrrpt " );
		break;
	case TR_CLAMP_TO_ZERO:
		common->Printf( "zero " );
		break;
	case TR_CLAMP_TO_ZERO_ALPHA:
		common->Printf( "a0 " );
		break;
	case TR_CLAMP:
		common->Printf( "clamp" );
		break;
	case TR_CLAMP_X:
	case TR_CLAMP_Y:
	default:
		common->Printf( "<BAD REPEAT:%i>", repeat );
		break;
	}

	if ( csv ) {
		common->Printf( "%4ik,%s\n", StorageSize() / 1024, imgName.c_str() );
	} else {
		common->Printf( "%4ik  %s\n", StorageSize() / 1024, imgName.c_str() );
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

	byte *buffer = (byte *)Mem_Alloc( bufferSize );
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
	int		imgStart = 18 + ( 256 * 3 );

    //byte *buffer = new byte[bufferSize];
	byte *buffer = (byte *)Mem_Alloc( bufferSize );
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
	Mem_Free( buffer );// delete[] buffer;
}

static void LoadBMP( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp );
static void LoadTGA( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp );
static void LoadJPG( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp );


#include <./usr/include/GL/glew.h>
#include <./usr/include/GL/gl.h>
#include <GLFW/glfw3.h>
#include </usr/include/GL/internal/glcore.h>
#include "../renderer/GLIncludes/qgl.h"
#include "../framework/FileSystem.h"
#include "../renderer/RenderSystem.h"
#include "../renderer/RenderWorld.h"
#include "Material.h"
#include "ImageManager.h"
/*
====================================================================

IMAGE

anImage have a one to one correspondance with OpenGL textures.

No texture is ever used that does not have a corresponding anImage.

no code outside this unit should call any of these OpenGL functions:

qglGenTextures
qglDeleteTextures
qglBindTexture

qglTexParameter

qglTexImage
qglTexSubImage

qglCopyTexImage
qglCopyTexSubImage

qglEnable( GL_TEXTURE_* )
qglDisable( GL_TEXTURE_* )


GLboolean: Represents a boolean value (GL_TRUE or GL_FALSE).

GLsizei: Represents a size or count of objects or elements.
GLbyte: Represents a signed 8-bit integer.
GLubyte: Represents an unsigned 8-bit integer.

GLshort: Represents a signed 16-bit integer.
GLushort: Represents an unsigned 16-bit integer. I.E. unsigned short

GLint: Represents a signed 32-bit integer.
GLuint: Represents an unsigned 32-bit integer.

GLfloat: Represents a single-precision floating-point value.
GLdouble: Represents a double-precision floating-point value.

GLenum: Represents an enumerated type or constant.

typedef byte				GLubyte;
typedef float				GLfloat;
typedef unsigned char		GLubyte8;		// 8-bit unsigned integer
typedef unsigned short		GLuint16;		// 16-bit unsigned integer
typedef unsigned int		GLuint32;		// 32-bit unsigned integer
typedef unsigned long long	GLuint64;		// 64-bit unsigned integer

typedef signed char			GLbyte8;		// 8-bit signed integer
typedef signed short		GLshort16;		// 16-bit signed integer
typedef signed int			GLint32;		// 32-bit signed integer
typedef signed long long	GLint64;		// 64-bit signed integer

typedef float				GLfloat32;		// 32-bit floating-point number
typedef double				GLfloat64;		// 64-bit floating-point number

These types are commonly used in OpenGL function signatures and parameter definitions to specify
the types of arguments expected by the OpenGL API.
It's important to note that the actual size and range of these types may vary depending on the
platform and OpenGL implementation.


NOTE: TODO: FUTURE IMPLEMENTATIONS:
ome of the OpenGL extensions for LOD Bias in version 4.6 include:

GLARBtexturefilteranisotropic:
This extension provides anisotropic texture filtering,
which can help improve the quality of texture sampling, especially when textures are viewed at oblique angles.

GLARBsampler_objects:
This extension allows for the creation of sampler objects,
which can be used to store and manage texture sampling state, including LOD bias.

GLARBtexture_lod:
This extension introduces additional control over the level of detail (LOD) calculation for textures, including LOD bias.
GLARBbindless_texture:
This extension allows for the use of bindless textures,
which can improve texture sampling performance and provide more flexibility in LOD bias settings.
GLARBtexturequerylevels:
This extension provides a way to query the number of mip levels in a texture,
which can be useful for LOD bias calculations.
====================================================================
*/

class anCVar;
class idMegaTexture;

typedef enum {
	IS_UNLOADED,	// no gl texture number
	IS_PARTIAL,		// has a texture number and the low mip levels loaded
	IS_LOADED		// has a texture number and the full mip hierarchy
} imageState_t;

static const int	MAX_TEXTURE_LEVELS = 14;

// surface description flags
const unsigned long DDSF_CAPS           = 0x00000001l;
const unsigned long DDSF_HEIGHT         = 0x00000002l;
const unsigned long DDSF_WIDTH          = 0x00000004l;
const unsigned long DDSF_PITCH          = 0x00000008l;
const unsigned long DDSF_PIXELFORMAT    = 0x00001000l;
const unsigned long DDSF_MIPMAPCOUNT    = 0x00020000l;
const unsigned long DDSF_LINEARSIZE     = 0x00080000l;
const unsigned long DDSF_DEPTH          = 0x00800000l;

// pixel format flags
const unsigned long DDSF_ALPHAPIXELS    = 0x00000001l;
const unsigned long DDSF_FOURCC         = 0x00000004l;
const unsigned long DDSF_RGB            = 0x00000040l;
const unsigned long DDSF_RGBA           = 0x00000041l;

// our extended flags
const unsigned long DDSF_ID_INDEXCOLOR	= 0x10000000l;
const unsigned long DDSF_ID_MONOCHROME	= 0x20000000l;

// dwCaps1 flags
const unsigned long DDSF_COMPLEX         = 0x00000008l;
const unsigned long DDSF_TEXTURE         = 0x00001000l;
const unsigned long DDSF_MIPMAP          = 0x00400000l;

#define DDS_MAKEFOURCC( a, b, c, d ) ( ( a ) | ( ( b ) << 8 ) | ( ( c ) << 16 ) | ( ( d ) << 24 ) )

// renaming the struct and mergring the two structs together ddsFilePixelFormat and
// ddsFileHeader
// i see no need in them being in different structures when they call the same functions
// and they are the same file format
typedef struct {
    unsigned long		dwSize;
    unsigned long		dwFlags;
    unsigned long		dwHeight, dwWidth;
    unsigned long		dwFourCC, dwPitchOrLinearSize;
    unsigned long		dwDepth, dwMipMapCount;
    unsigned long		dwRGBBitCount;
	unsigned long		dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;
    GLuint64			ddspf; // ddsFilePixelFormat
    unsigned long		dwCaps1, dwCaps2;
    unsigned long		dwReserved1[11], dwReserved2[3];
} ddsFileProperties_t;

//static const mipmapState_t defaultMipmapState = { {0,0,0,0}, {0,0,0,0}, mipmapState_t::MT_NONE };

// increasing numeric values imply more information is stored
typedef enum {
	TD_SPECULAR,			// may be compressed, and always zeros the alpha channel
	TD_DIFFUSE,				// may be compressed
	TD_DEFAULT,				// will use compressed formats when possible
	TD_BUMP,				// may be compressed with 8 bit lookup
	TD_HIGH_QUALITY			// either 32 bit or a component format, no loss at all
} textureDepth_t;

typedef enum {
	TT_DISABLED,
	TT_2D,
	TT_3D,
	TT_CUBIC,
	TT_CUBE_INFT,
	TT_RECT
} textureType_t;

typedef enum {
	CF_2D,			// not a cube map
	CF_NATIVE,		// _px, _nx, _py, etc, directly sent to GL
	CF_CAMERA,		// _forward, _back, etc, rotated and flipped as needed before sending to GL
	CF_HALFSPHERE	// Half-Spherical projection map resampled to cubemap at load time
} cubeFiles_t;

#define	MAX_IMAGE_NAME	256

class anImage {
public:
				anImage();

	const char	*GetName() const { return imgName; }

	// Makes this image active on the current GL texture unit.
	// automatically enables or disables cube mapping or texture3D
	// May perform file loading if the image was not preloaded.
	// May start a background image read.
	void		Bind();

	// for use with fragment programs, doesn't change any enable2D/3D/cube states
	void		BindFragment();

	// used by callback functions to specify the actual data
	// data goes from the bottom to the top line of the image, as OpenGL expects it
	// These perform an implicit Bind() on the current texture unit
	// FIXME: should we implement cinematics this way, instead of with explicit calls?
	void		GenerateImage( const GLubyte *pic, int width, int height, textureFilter_t filter, bool allowDownSize, textureRepeat_t repeat, textureDepth_t depth );
	void		Generate3DImage( const GLubyte *pic, int width, int height, int depth, textureFilter_t filter, bool allowDownSize, textureRepeat_t repeat, textureDepth_t minDepth );
	void		GenerateCubeImage( const GLubyte *pic[6], int size, textureFilter_t filter, bool allowDownSize, textureDepth_t depth );

	void		CopyFramebuffer( int x, int y, int width, int height );
	void		CopyOverSizedFramebuffer( int x, int y, int width, int height, bool useOversizedBuffer );
	void		CopyDepthbuffer( int x, int y, int width, int height );

	void		UploadScratch( const GLubyte *pic, int width, int height );

	// Copy data from one image over to the other
	//void				CopyFromImage( anImage *img );
	//void				CopyFromImageCube( anImage *img );

	//void				Download( GLubyte **pixels, int *width, int *height );

	// just for resource tracking
	void			SetClassification( int tag );

	// estimates size of the GL image based on dimensions and storage type
	int				StorageSize() const;

	// print a one line summary of the image
	void			Print() const;

	// check for changed timestamp on disk and reload if necessary
	//virtual void		Reload( bool checkPrecompressed, bool force );

	int				GetImageId( const char *name ) const;
//==========================================================

	void			GetDownsize( int &scaledWidth, int &scaledHeight ) const;
	void			MakeDefault();	// fill with a grid pattern

	void			FromParameters( int width, int height, int internalFormat, textureType_t type, textureFilter_t filter, textureRepeat_t repeat );

	void			SetImageFilterAndRepeat() const;
	bool			ShouldImageBePartialCached();

	void			WritePrecompressedImage();
	bool			CheckPrecompressedImage( bool fullLoad );
	void			UploadPrecompressedImage( GLubyte *data, int len );

	void			ActuallyLoadImage( bool checkForPrecompressed, bool fromBackEnd );
	void			StartBackgroundImageLoad();

	int				BitsForInternalFormat( int internalFormat ) const;

	void			UploadCompressedNormalMap( int width, int height, const GLubyte *rgba, int mipLevel );

	GLenum			SelectInternalFormat( const GLubyte **dataPtrs, int numDataPtrs, int width, int height, textureDepth_t minimumDepth, bool *monochromeResult ) const;

	void			ImageProgramStringToCompressedFileName( const char *imageProg, char *fileName ) const;
	int				NumLevelsForImageSize( int width, int height ) const;
	int				NumLevelsForImageSize( int width, int height, int internalFormat ) const;
//===========================================================

	void			AllocImage( textureFilter_t filter, textureRepeat_t repeat );

	// Deletes the texture object, but leaves the structure so it can be reloaded
	// or resized.
	void			PurgeImage();

	// z is 0 for 2D textures, 0 - 5 for cube maps, and 0 - uploadDepth for 3D textures. Only
	// one plane at a time of 3D textures can be uploaded. The data is assumed to be correct for
	// the format, either bytes, halfFloats, floats, or DXT compressed. The data is assumed to
	// be in OpenGL RGBA format, the consoles may have to reorganize. pixelPitch is only needed
	// when updating from a source subrect. Width, height, and dest* are always in pixels, so
	// they must be a multiple of four for dxt data.
	void			SubImageUpload( int mipLevel, int destX, int destY, int destZ, int width, int height, const void *data, int pixelPitch = 0 ) const;

	// SetPixel is assumed to be a fast memory write on consoles, degenerating to a
	// SubImageUpload on PCs.  Used to update the page mapping images.
	// We could remove this now, because the consoles don't use the intermediate page mapping
	// textures now that they can pack everything into the virtual page table images.
	void			SetPixel( int mipLevel, int x, int y, const void *data, int dataSize );

	// some scratch images are dynamically resized based on the display window size.  This
	// simply purges the image and recreates it if the sizes are different, so it should not be
	// done under any normal circumstances, and probably not at all on consoles.
	void			Resize( int width, int height );

	bool			IsCompressed() const { return ( opts.format == FMT_DXT1 || opts.format == FMT_DXT5 ); }

	void			SetTexParameters();	// update aniso and trilinear

	bool			IsLoaded() const { return texnum != TEXTURE_NOT_LOADED; }

	static void			GetGeneratedName( anString &_name, const textureUsage_t &_usage, const cubeFiles_t &_cube );

private:
	friend class anImageManager;

	void				AllocImage();
//==========================================================
	// data commonly accessed is grouped here
	static const int 		TEXTURE_NOT_LOADED = -1;
	GLuint					texnum;					// gl texture binding, will be TEXTURE_NOT_LOADED if not loaded
	textureType_t			type;
	int						frameUsed;				// for texture usage in frame statistics
	int						bindCount;				// incremented each bind

	// LOD information
	bool					distanceLod;			// we are to far away, a lower res version can be used just fine
	float					smallestDistanceSeen;	// The smallest distance seen so far for the lod parameter
	int						frameOfDistance;		// The frame the lod was last set

	// background loading information
	anImage					*partialImage;			// shrunken, space-saving version
	bool					isPartialImage;			// true if this is pointed to by another image
	bool					backgroundLoadInProgress;	// true if another thread is reading the complete d3t file
	backgroundDownload_t	bgl;
	anImage *				bglNext;				// linked from tr.backgroundImageLoads

	// parameters that define this image
	anString				imgName;				// game path, including extension (except for cube maps), may be an image program
	void					(*generatorFunction)( anImage *image );	// NULL for files

	bool					fromBackEnd;			// this is the image loaded from the back end
	bool					fromParams;
	bool					allowDownSize;			// this also doubles as a don't-partially-load flag

	mipmapState_t			mipmapState;			// Mipmap level coloring
	int						numMipLevels, picMipOfs, picMipMin;

	float					anisotropy; // FIXME: what happend to min and max?

	textureFilter_t			filter;
	textureRepeat_t			repeat;
	textureDepth_t			depth;
	cubeFiles_t				cubeFiles;				// determines the naming and flipping conventions for the six images

	float					minLod;
	float					maxLod;

	bool					referencedOutsideLevelLoad;
	bool					levelLoadReferenced;	// for determining if it needs to be purged
	bool					precompressedFile;		// true when it was loaded from a .d3t file
	bool					defaulted;				// true if the default image was generated because a file couldn't be loaded
	bool					isMonochrome;			// so the NV20 path can use a reduced pass count
	ARC_TIME_T				timestamp;				// the most recent of all images used in creation, for reloadImages command

	int						imageHash;				// for identical-image checking

	int						classification;			// just for resource profiling

	// data for listImages
	//int					sourceWidth, sourceHeight;				// after power of two, before downsample
	int						uploadWidth, uploadHeight, uploadDepth;	// after power of two, downsample, and MAX_TEXTURE_SIZE
	int						internalFormat;

	anImage 				*cacheUsagePrev, *cacheUsageNext;	// for dynamic cache purging of old images

	anImage 				*hashNext;				// for hash chains to speed lookup

	int						refCount;				// overall ref count
};

ARC_INLINE anImage::anImage() {
	texnum = TEXTURE_NOT_LOADED;
	partialImage = NULL;
	type = TT_DISABLED;
	isPartialImage = false;
	frameUsed = 0;
	classification = 0;
	backgroundLoadInProgress = false;
	bgl.opcode = DLTYPE_FILE;
	bgl.f = NULL;
	bglNext = NULL;
	imgName[0] = '\0';
	generatorFunction = NULL;
	fromBackEnd = false;
	fromParams = false;
	allowDownSize = false;
	filter = TF_DEFAULT;
	repeat = TR_REPEAT;
	depth = TD_DEFAULT;
	cubeFiles = CF_2D;
	referencedOutsideLevelLoad = false;
	levelLoadReferenced = false;
	precompressedFile = false;
	defaulted = false;
	timestamp = 0;
	bindCount = 0;
	//sourceWidth = sourceHeight;
	uploadWidth = uploadHeight = uploadDepth = 0;
	internalFormat = 0;
	cacheUsagePrev = cacheUsageNext = NULL;
	hashNext = NULL;
	isMonochrome = false;
	refCount = 0;
}

// data is RGBA
void	R_WriteTGA( const char *filename, const GLubyte *data, int width, int height, bool flipVertical = false );
// data is an 8 bit index into palette, which is RGB (no A)
void	R_WritePalTGA( const char *filename, const GLubyte *data, const GLubyte *palette, int width, int height, bool flipVertical = false );
// data is in top-to-bottom raster order unless flipVertical is set

int MakePowerOfTwo( int num );

#define CELL_GCM_INVALID_PITCH		64

/*
================================================
anRenderImage holds both the color and depth images that are made
resident on the video hardware.
================================================
*/
class anRenderImage {
public:
							anRenderImage();
							~anRenderImage();

	ARC_INLINE int			GetWidth() const { return ( colorImage != NULL ) ? colorImage->GetUploadWidth() : depthImage->GetUploadWidth(); }
	ARC_INLINE int			GetHeight() const { return ( colorImage != NULL ) ? colorImage->GetUploadHeight() : depthImage->GetUploadHeight(); }

	ARC_INLINE anImage *	GetColorImage() const { return colorImage; }
	ARC_INLINE anImage *	GetDepthImage() const { return depthImage; }


	void					Resize( int width, int height );

	void					MakeCurrent( int level = 0, int side = 0 );

private:
	anImage *			colorImage;
	anImage *			depthImage;
	int					targetWidth;
	int					targetHeight;
};

/*
====================================================================

IMAGEPROCESS

FIXME: make an "imageBlock" type to hold byte*,width,height?
====================================================================
*/

GLubyte *R_Dropsample( const GLubyte *in, int inWidth, int inHeight, int outWidth, int outHeight );
GLubyte *R_ResampleTextureOptimized( const GLubyte byte *in, GLint inWidth, GLint inHeight, GLint outWidth, GLint outHeight );
GLubyte *R_ResampleTexture( const GLubyte *in, int inWidth, int inHeight, int outWidth, int outHeight );
GLubyte *R_MipMapWithAlphaSpecularity( const GLubyte *in, int width, int height );
GLubyte *R_MipMap( const GLubyte *in, int width, int height, bool preserveBorder );
GLubyte *R_MipMap3D( const GLubyte *in, int width, int height, int depth, bool preserveBorder );

// these operate in-place on the provided pixels
void R_SetBorderTexels( GLubyte *inBase, int width, int height, const GLubyte border[4] );
void R_SetBorderTexels3D( GLubyte *inBase, int width, int height, int depth, const GLubyte border[4] );
void R_BlendOverTexture( GLubyte *data, int pixelCount, const GLubyte blend[4] );
void R_HorizontalFlip( GLubyte *data, int width, int height );
void R_VerticalFlip( GLubyte *data, int width, int height );
void R_RotatePic( GLubyte *data, int width );

/*
====================================================================

IMAGEFILES

====================================================================
*/

void R_LoadImage( const char *name, GLubyte **pic, int *width, int *height, ARC_TIME_T *timestamp, bool makePowerOf2 );
// pic is in top to bottom raster format
bool R_LoadCubeImages( const char *cname, cubeFiles_t extensions, GLubyte *pic[6], int *size, ARC_TIME_T *timestamp );

/*
====================================================================

IMAGEPROGRAM

====================================================================
*/

void R_LoadImageProgram( const char *name, GLubyte **pic, int *width, int *height, ARC_TIME_T *timestamp, textureDepth_t *depth = NULL );
const char *R_ParsePastImageProgram( anLexer &src );


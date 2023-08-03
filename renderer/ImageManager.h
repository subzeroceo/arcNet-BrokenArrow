#include <./usr/include/GL/glew.h>
#include <./usr/include/GL/gl.h>
#include <GLFW/glfw3.h>
#include </usr/include/GL/internal/glcore.h>
#include "../renderer/GLIncludes/qgl.h"
#include "../framework/FileSystem.h"
#include "../renderer/RenderSystem.h"
#include "../renderer/RenderWorld.h"

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


ideas/examples

enum SurfaceFlag {
    SF_NONE = 0,
    SFLAG_ALPHA = 1 << 0,          // The image has an alpha channel
    SFLAG_TRANSPARENT = 1 << 1,    // The image has transparent pixels
    SFLAG_FLIP_HORIZONTAL = 1 << 2, // The image should be flipped horizontally
    SFLAG_FLIP_VERTICAL = 1 << 3   // The image should be flipped vertically
};

enum SurfaceDescriptionFlag {
    SURF_DESC_FLAG_NONE = 0,
    SURF_DESC_FLAG_HDR = 1 << 0,       // The image is in high dynamic range format
    SURF_DESC_FLAG_NORMAL_MAP = 1 << 1, // The image is a normal map
    SURF_DESC_FLAG_CUBEMAP = 1 << 2    // The image is a cubemap
};

enum ImageLevel {
    IL_BASE = 0,        // The base level of the image
    IL_MIPMAP_1 = 1,    // The first level of the image's mipmaps
    IL_MIPMAP_2 = 2,    // The second level of the image's mipmaps
    // ... additional mip levels
    IL_MAX = 10        // The maximum level of the image's mipmaps
};

struct ImageHeader {
    int width;                    // The width of the image
    int height;                   // The height of the image
    int numChannels;              // The number of color channels in the image
    SurfaceFlag surfaceFlags;     // Flags indicating special properties of the image surface
    SurfaceDescriptionFlag descFlags; // Flags describing additional properties of the image
    PixelFormatFlag pixelFormatFlags; // Flags indicating the pixel format of the image
    ImageLevel maxMipLevel;       // The maximum level of mipmaps in the image
};
====================================================================
*/

/*
================================================================================================

This is where the Binary image headers go that are also included by external tools such as the cloud.

================================================================================================
*/

// These structures are used for memory mapping bimage files, but
// not for the normal loading, so be careful making changes.
// Values are big endien to reduce effort on consoles.
#define BIMAGE_VERSION 10
#define BIMAGE_MAGIC (unsigned int)( ('B'<<0)|('I'<<8)|('M'<<16)|(BIMAGE_VERSION<<24) )
#pragma pack( push, 1 )
struct binaryImage {
	int		level;
	int		destZ;
	int		width;
	int		height;
	int		dataSize; // dataSize bytes follow
	ARC_TIME_T	sourceFileTime;
	int		headerMagic;
	int		textureType;
	int		format;
	int		colorFormat;
	int		numLevels;
};
#pragma pack( pop )
//ARC_STATIC_ASSERT( sizeof( binaryImage ) == ( sizeof( bimageanImage ) + sizeof( bimageFile_t ) ) );

struct mipmapState_t {
	enum colorType_e {
		MT_NONE,
		MT_DEFAULT,
		MT_WATER,
		MT_COLORLEVELS
	};

	bool operator == ( const mipmapState_t &a ) {
		return !memcmp( this, &a, sizeof( mipmapState_t ) );
	}

	bool operator != ( const mipmapState_t &a ) {
		return !operator ==( a );
	}

	float		color[4];	// Color to blend to
	float		blend[4];	// Blend factor for every channel
	colorType_e	colorType;
};

static const mipmapState_t defaultMipmapState = { {0,0,0,0}, {0,0,0,0}, mipmapState_t::MT_NONE };


/*
================================================
The internal *Texture Format Types*, ::textureFormat_t, are:
================================================
*/

enum PixelFormatFlag {
    PF_FLAG_NONE = 0,
    PF_FLAG_RGB = 1 << 0,       // The image uses RGB pixel format
    PF_FLAG_RGBA = 1 << 1,      // The image uses RGBA pixel format
    PF_FLAG_BGR = 1 << 2,       // The image uses BGR pixel format
    PF_FLAG_BGRA = 1 << 3       // The image uses BGRA pixel format
};

enum textureFormat_t {
	FMT_NONE,
	FMT_RGBA8,			// 32 bpp
	FMT_XRGB8,			// 32 bpp
	// Alpha channel only
	// Alpha ends up being the same as L8A8 in our current implementation, because straight
	// alpha gives 0 for color, but we want 1.
	FMT_ALPHA,
	// Luminance replicates the value across RGB with a constant A of 255
	// Intensity replicates the value across RGBA
	FMT_L8A8,			// 16 bpp
	FMT_LUM8,			//  8 bpp
	FMT_INT8,			//  8 bpp
	// Compressed texture formats
	FMT_DXT1,			// 4 bpp
	FMT_DXT5,			// 8 bpp
	// Depth buffer format
	FMT_DEPTH,			// 24 bpp
	FMT_X16,			// 16 bpp
	FMT_Y16_X16,		// 32 bpp
	FMT_RGB565,			// 16 bpp
	FMT_BGR8,			// 24 bpp
	FMT_BGRA8			// 32 bpp
	FMT_DEPTH_STENCIL,
	FMT_RGBA16,			// 16 bit bpp
	FMT_RGBAF16,
	FMT_RG16,
	FMT_RG32,
	FMT_R32,
	FMT_DEPTH32,
};

/*
================================================
DXT5 color formats
================================================
*/
enum textureColor_t {
	CFM_DEFAULT,			// RGBA
	CFM_NORMAL_DXT5,		// XY format and use the fast DXT5 compressor
	CFM_YCOCG_DXT5,			// convert RGBA to CoCg_Y format
	CFM_GREEN_ALPHA			// Copy the alpha channel to green
};


class anImageManager {
public:public:
	anImageManager() {
		insideLevelLoad = false;
		preloadingMapImages = false;
	}

	void				Init();
	void				Shutdown();

  	bool				IsInitialized() const { return (images.Num() != 0); }
	static anImage *	ParseImage( anParser &src, const imageParams_t &defaultParms = defaultImageParms);
	// If the exact combination of parameters has been asked for already, an existing
	// image will be returned, otherwise a new image will be created.
	// Be careful not to use the same image file with different filter / repeat / etc parameters
	// if possible, because it will cause a second copy to be loaded.
	// If the load fails for any reason, the image will be filled in with the default
	// grid pattern.
	// Will automatically resample non-power-of-two images and execute image programs if needed.
	anImage *			ImageFromFile( const char *name, textureFilter_t filter, bool allowDownSize, textureRepeat_t repeat, textureDepth_t depth, cubeFiles_t cubeMap = CF_2D );

	// look for a loaded image, whatever the parameters
	anImage *			GetImage( const char *name ) const;

	// look for a loaded image, whatever the parameters
	anImage *			GetImageWithParameters( const char *name, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage, cubeFiles_t cubeMap ) const;

	// The callback will be issued immediately, and later if images are reloaded or vid_restart
	// The callback function should call one of the anImage::Generate* functions to fill in the data
	anImage *			ImageFromFunction( const char *name, void (*generatorFunction)( anImage *image ) );
	idMegaTexture *		MegaTextureFromFile( const char *name );

	// called once a frame to allow any background loads that have been completed
	// to turn into textures.
	void				CompleteBackgroundImageLoads();
	// scratch images are for internal renderer use.  ScratchImage names should always begin with an underscore
	anImage *			ScratchImage( const char *name, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage );

	// returns the number of bytes of image data bound in the previous frame
	int					SumOfUsedImages();

	// called each frame to allow some cvars to automatically force changes
	void				CheckCvars();

	// purges all megatexture and heightmaps before a vid_restart
 	void PurgeAllMegaTextures();

	// purges all the images before a vid_restart
	void				PurgeAllImages();

	// reloads all apropriate images after a vid_restart
	void				ReloadAllImages();
	// unbind all textures from all texture units
	void				UnbindAll();

	// disable the active texture unit
	void				BindNull();

	// Mark all file based images as currently unused,
	// but don't free anything.  Calls to ImageFromFile() will
	// either mark the image as used, or create a new image without
	// loading the actual data.
	// Called only by renderSystem::BeginLevelLoad
	void				BeginLevelLoad();

	// Free all images marked as unused, and load all images that are necessary.
	// This architecture prevents us from having the union of two level's
	// worth of data present at one time.
	// Called only by renderSystem::EndLevelLoad
	void				EndLevelLoad();

	void				SetInsideLevelLoad( bool value ) { insideLevelLoad = value; }

  // During level load, there is the requirement to load textures at two points.
  // Once after the level loading GUI has been loaded, and once in EndLevelLoad.
	void				LoadPendingImages( bool updatePacifier = true, const bool & mapPreload );

	// used to clear and then write the dds conversion batch file
	void				StartBuild();
	void				FinishBuild( bool removeDups = false );
	void				AddDDSCommand( const char *cmd );

	void				PrintMemInfo( MemInfo_t *mi );

	// cvars
	static anCVarSystem		image_roundDown;			// round bad sizes down to nearest power of two
	static anCVarSystem		image_colorMipLevels;		// development aid to see texture mip usage
	static anCVarSystem		image_downSize;				// controls texture downsampling
	static anCVarSystem		image_useCompression;		// 0 = force everything to high quality
	static anCVarSystem		image_filter;				// changes texture filtering on mipmapped images
	static anCVarSystem		image_anisotropy;			// set the maximum texture anisotropy if available
	static anCVarSystem		image_lodbias;				// change lod bias on mipmapped images
	static anCVarSystem		image_useAllFormats;		// allow alpha/intensity/luminance/luminance+alpha
	static anCVarSystem		image_usePrecompressedTextures;	// use .dds files if present
	static anCVarSystem		image_writePrecompressedTextures; // write .dds files if necessary
	static anCVarSystem		image_writeNormalTGA;		// debug tool to write out .tgas of the final normal maps
	static anCVarSystem		image_writeNormalTGAPalletized;		// debug tool to write out palletized versions of the final normal maps
	static anCVarSystem		image_writeTGA;				// debug tool to write out .tgas of the non normal maps
	static anCVarSystem		image_useNormalCompression;	// 1 = use 256 color compression for normal maps if available, 2 = use rxgb compression
	static anCVarSystem		image_useOffLineCompression; // will write a batch file with commands for the offline compression
	static anCVarSystem		image_preload;				// if 0, dynamically load all images
	static anCVarSystem		image_cacheMinK;			// maximum K of precompressed files to read at specification time,
													// the remainder will be dynamically cached
	static anCVarSystem		image_cacheMegs;			// maximum bytes set aside for temporary loading of full-sized precompressed images
	static anCVarSystem		image_useCache;				// 1 = do background load image caching
	static anCVarSystem		image_showBackgroundLoads;	// 1 = print number of outstanding background loads
	static anCVarSystem		image_forceDownSize;		// allows the ability to force a downsize
	static anCVarSystem		image_downSizeSpecular;		// downsize specular
	static anCVarSystem		image_downSizeSpecularLimit;// downsize specular limit
	static anCVarSystem		image_downSizeBump;			// downsize bump maps
	static anCVarSystem		image_downSizeBumpLimit;	// downsize bump limit
	static anCVarSystem		image_ignoreHighQuality;	// ignore high quality on materials
	static anCVarSystem		image_downSizeLimit;		// downsize diffuse limit

	// built-in images
	anImage *			defaultImage;
	anImage *			flatNormalMap;				// 128 128 255 in all pixels
	anImage *			ambientNormalMap;			// tr.ambientLightVector encoded in all pixels
	anImage *			rampImage;					// 0-255 in RGBA in S
	anImage *			alphaRampImage;				// 0-255 in alpha, 255 in RGB
	anImage *			alphaNotchImage;			// 2x1 texture with just 1110 and 1111 with point sampling
	anImage *			whiteImage;					// full of 0xff
	anImage *			blackImage;					// full of 0x00
	anImage *			normalCubeMapImage;			// cube map to normalize STR into RGB
	anImage *			noFalloffImage;				// all 255, but zero clamped
	anImage *			fogImage;					// increasing alpha is denser fog
	anImage *			fogEnterImage;				// adjust fogImage alpha based on terminator plane
	anImage *			cinematicImage;
	anImage *			scratchImage;
	anImage *			scratchImage2;
	anImage *			accumImage;
	anImage *			coronaImage;
	anImage *			currentRenderImage;			// for SS_POST_PROCESS shaders
	anImage *			currentDepthImage;				// for motion blur
	anImage *			scratchCubeMapImage;
	anImage *			specularTableImage;			// 1D intensity texture with our specular function
	anImage *			specular2DTableImage;		// 2D intensity texture with our specular function with variable specularity
	anImage *			borderClampImage;			// white inside, black outside
	anImage *			postProcessBuffer[2];
	//--------------------------------------------------------

	anImage *			AllocImage( const char *name );
	void				SetNormalPalette();
	void				ChangeTextureFilter();

	anList<anImage*>	images;
	anStringList		ddsList;
	anHashIndex			ddsHash;

	bool				insideLevelLoad;			// don't actually load images now

	GLubyte				originalToCompressed[256];	// maps normal maps to 8 bit textures
	GLubyte				compressedPalette[768];		// the palette that normal maps use

	// default filter modes for images
	GLenum				textureMinFilter;
	GLenum				textureMaxFilter;
	float				textureAnisotropy;
	float				textureLODBias;

	anImage *			imageHashTable[FILE_HASH_SIZE]; // backupHashTable for image Hash table

	anImage *			backgroundImageLoads;		// chain of images that have background file loads active
	anImage				cacheLRU;					// head/tail of doubly linked list
	int					totalCachedImageSize;		// for determining when something should be purged

	int					numActiveBackgroundImageLoads;
	const static int MAX_BACKGROUND_IMAGE_LOADS = 8;
};

extern anImageManager	*globalImages;		// pointer to global list for the rest of the system

/*
================================================
anImageOpts hold parameters for texture operations.
================================================
*/

class anImageOpts {
public:
struct imageParams_t {
	textureFilter_t	tf;
	textureRepeat_t	trp;
	textureDepth_t	td;
	cubeFiles_t		cubeMap;
	mipmapState_t	mipState;
	bool			allowPicmip;
	int				picmipofs;
	int				picMipMin;
	float			anisotropy;
	bool			partialLoad;
	float			minLod;
	float			maxLod;

	imageParams_t::imageParams_t() { Clear(); }
	imageParams_t::imageParams_t( textureFilter_t _filter, bool _allowDownSize, int _picmipofs, float _anisotropy,
		textureRepeat_t _repeat, textureDepth_t _depth, cubeFiles_t _cubeMap = CF_2D, mipmapState_t _mipmapState = defaultMipmapState ) {
		Clear();

		tf = _filter;
		allowPicmip = _allowDownSize;
		picmipofs = _picmipofs;
		anisotropy = _anisotropy;
		trp = _repeat;
		td = _depth;
		cubeMap = _cubeMap;
		mipState = _mipmapState;
	}

	void Clear() {
		tf = TF_DEFAULT;
		trp = TR_REPEAT;
		td = TD_DEFAULT;
		cubeMap = CF_2D;
		mipState = defaultMipmapState;
		allowPicmip = true;
		picmipofs = 0;
		partialLoad = false;
		anisotropy = -1.f;
		minLod = 0.f;
		maxLod = 1000.f;
		picMipMin = -10;
	}
}; static const imageParams_t defaultImageParms;

	anImageOpts();

	bool	operator==( const anImageOpts &opts );

	//---------------------------------------------------
	// these determine the physical memory size and layout
	//---------------------------------------------------

	textureType_t		textureType;
	textureFormat_t		format;
	textureColor_t		colorFormat;
	int					width;
	int					height;			// not needed for cube maps
	int					numLevels;		// if 0, will be 1 for NEAREST / LINEAR filters, otherwise based on size
	bool				gammaMips;		// if true, mips will be generated with gamma correction
	bool				readback;		// 360 specific - cpu reads back from this texture, so allocate with cached memory
};

/*
========================
anImageOpts::anImageOpts
========================
*/
ARC_INLINE anImageOpts::anImageOpts() {
	format		= FMT_NONE;
	colorFormat	= CFM_DEFAULT;
	width		= 0;
	height		= 0;
	numLevels	= 0;
	textureType	= TT_2D;
	gammaMips	= false;
	readback	= false;
	Clear();
};

/*
========================
anImageOpts::operator
========================
*/
ARC_INLINE bool anImageOpts::operator==( const anImageOpts & opts ) {
	return ( memcmp( this, &opts, sizeof(* this) ) == 0 );
}

class anImageGenFunctorBase {
public:
  virtual ~anImageGenFunctorBase( void ) {}
  virtual void operator()( class anImage *image ) const = 0;
};

template <class T>
class anImageGenFunctor : public anImageGenFunctorBase {
public:
typedef void (T::*func_t)( class anImage *image );

void Init( T *generatorClass, func_t imageGenerator ) {
	this->generatorClass = generatorClass;
	this->imageGenerator = imageGenerator;
}

virtual void operator()( class anImage *image ) const {
	(*generatorClass.*imageGenerator)( image );
}

private:
	T *generatorClass;
	func_t imageGenerator;
};

class anImageGenFunctorGlobal : public anImageGenFunctorBase {
public:
typedef void (*func_t)( class anImage *image );
	anImageGenFunctorGlobal( func_t imageGenerator ) {
	this->imageGenerator = imageGenerator;
}

virtual void operator()( class anImage *image ) const {
	imageGenerator( image );
}

protected:
  func_t imageGenerator;
};

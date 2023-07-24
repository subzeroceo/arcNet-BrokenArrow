class arcMaterial;
class ARCImage;
class arcVec4;
class arcDrawVert;
class arcPlane;
class arcVec3;
class arcNetString;

#include <gl/gl.h>
#include "../GLIncludes/qgl.h"
#include "Model_public.h"

/*
===============================================================================

	ARCRenderSystem is responsible for managing the screen, which can have
	multiple ARCRenderWorld and 2D drawing done on it.

The GLARBmultitexture extension has been superseded by several versions of OpenGL. Here are some of the subsequent versions that incorporate the functionality provided by GLARBmultitexture:

Here is a summary of the major versions of OpenGL:
OpenGL 1.0: The initial release of OpenGL.

OpenGL 1.3: Introduced multitexture functionality as a core feature of OpenGL,
eliminating the need for the GLARBmultitexture extension. Therefore no equivalent extension needed.
OpenGL versions beyond OpenGL 1.3 incorporate the functionality provided by the GLARBmultitexture extension.
Here are some of the newer versions of OpenGL along with their multitexture equivalent:

OpenGL 1.4: This version improved and expanded upon the multitexture capabilities introduced in OpenGL 1.3.
No specific multitexture equivalent, but it further improved and expanded upon the multitexture capabilities introduced in OpenGL 1.3.

OpenGL 1.5: This version introduced support for vertex buffer objects (VBOs) and advanced vertex array functionality,
enhancing the efficiency of multitexture operations.
 No specific multitexture equivalent, but it introduced vertex buffer objects (VBOs) and enhanced vertex array functionality,
 which are often used in conjunction with multitexture operations.

OpenGL 2.0: This version introduced programmable shaders (vertex and fragment shaders)
and advanced rendering capabilities. It provided more flexibility and control over multitexture operations.
Introduced programmable shaders (vertex and fragment shaders) which can be utilized for multitexture operations.

OpenGL 3.0: Introduced a major overhaul of the OpenGL pipeline and deprecated some older features.
and onwards: The multitexture functionality is integrated into the core OpenGL API, so there are no specific multitexture extensions in these versions.
It's important to note that multitexture operations can be achieved using various techniques and features provided by different versions of OpenGL,
such as combining programmable shaders, texture units, and texture binding.

OpenGL 4.0: Added support for tessellation shaders and compute shaders.
OpenGL 4.5: Introduced support for direct state access and enhanced debugging capabilities.

OpenGL 4.6: The newest version of OpenGL, as of my knowledge, is OpenGL 4.6.
It is the latest major release and includes various enhancements, performance optimizations and additional features.
It's worth noting that OpenGL versions are backward compatible, meaning that newer versions include all the
functionality of the previous versions. However, older features and extensions may be deprecated or removed
in newer versions.

===============================================================================
*/

typedef enum gpuVendor_t {
	VEN_NVIDIA,
	VEN_AMD64VEGA,
	VEN_ATI,
	VEN_AMD64,
	VEN_AMD32,
	VEN_INTEL,
}gpuVendor_t;

// Contains variables specific to the OpenGL configuration being run right now.
// These are constant once the OpenGL subsystem is initialized.
typedef struct qglConfig_t {
	const char			*rendererOutput;
	const char			*vendorOutput;
	const char			*versionOutput;
	const char			*qglExtStrOutput;
	const char			*qwglExtStrOutput;

	gpuVendor_t			gpuVendor;

	float				qglVersion;				// atof( versionOutput )

	GLuint				colorBits, depthBits, stencilBits, alphaBits;
	//GLuint				depthMaskBits, redBits, greenBits, blueBits;

	bool				useSRGBFramebuffer;

	bool				useVertexBufferObject;
	bool				useVertexArrayObject;
	GLuint				global_vao;
	bool				drawElementsBaseVertex;

	int					maxImageSize;			// queried from GL
	int					maxImageUnits;
	int					maxImageCoords;
	int					maxTextureImageUnits;

	float				maxTextureAnisotropy;
	bool				useAnisotropyFilter;	// GL_EXT_texture_filter_anisotropic

	// remove and upgrade clientside GL extensions to
	bool				isMultiTexture;			// GL_ARB_multitexture: qglMultiTexCoord2fARB, qglMultiTexCoord2fvARB, qglActiveTextureARB, qglClientActiveTextureARB
	bool				textureCompression;		// GL_EXT_texture_compression_s3tc probably needs to be reviewed, see what can replace it in 4.6

	bool				directStateAccess;

	bool				useTextureLODBias;		// GL_EXT_texture_lod
	bool				textureEnvAdd;			// GL_ARB_texture_env_add
	bool				textureEnvCombine;		// GL_ARB_texture_env_combine
	bool				regCombiners;			// GL_NV_register_combiners

	bool				useSeamlessCubeMap;
	bool				useCubeMap;				// GL_ARB_texture_cube_map

	bool				envDot3;				// GL_ARB_texture_env_dot3
	bool				3DImagesActive;			// for GL_EXT_texture3D
	bool				isSharedTPalette;		// for GL_EXT_shared_texture_palette

	bool				isImageNonPO2;			// GL_ARB_texture_non_power_of_two
	bool				depthBoundsTest;
	bool				debugOutputAvailable;
	bool				arbVertBuffObject;
	bool				ARBVPAvailable;
	bool				ARBFPAvailable;

	bool				isDoubleEdgeStencil;	// GL_EXT_stencil_two_side
	//bool				ATI_separateStencil // GL_ATI_separate_stencil
	bool				atiDoubleEdgeStencil;
	bool				atiPixelFormatFloat;	// pixel format float
	bool				ARBPixelFormatFloat;	// pixel format float ARBPixelFormatFloat
	// TODO: tomorrow?
	bool				isRyzen5VegaAvailable;
	bool				isRyzenCoreCompatable;
	// ati r200 extensions
	bool				atiShaderFragmentOn;

	int					maxVertexAttribs;
	int					maxProgramLocalParms;
	int					maxProgramEnvParms;

	//bool				fragmentProgramOn;
	//bool				enableGLsl;

	bool				nvFloatBuffer;
	bool				uniformBufferEnabled;

	bool				isTimerQueryActive;
	//bool				occlusionQueryAvailable;
	bool				debugOutput;
	bool				swapControlTear;
	bool				isSynchronized;

	int					vidWidth, vidHeight;	// passed to R_BeginFrame

	int					displayFrequency;

	bool				isFullscreen;
	int					multiSamples;

	bool				NV30Path;
	bool				NV20Path;
	bool				NV10Path;
	bool				R200Path;
	bool				ARB2Path;
	bool				CgPath;

	bool				isInitialized;
	//bool				backendInitialized;

	float				pixelAspect;

	bool				isSmpAvailable;
	int					isSmpActive;
	int					rearmSmp;
} qglConfig_t;

struct setBufferCommand_t;

// font support
const int GLYPH_START			= 0;
const int GLYPH_END				= 255;
const int GLYPH_CHARSTART		= 32;
const int GLYPH_CHAREND			= 127;
const int GLYPHS_PER_FONT		= GLYPH_END - GLYPH_START + 1;

typedef struct {
	int					height;			// number of scan lines
	int					top;			// top of glyph in buffer
	int					bottom;			// bottom of glyph in buffer
	int					pitch;			// width for copying
	int					xSkip;			// x adjustment
	int					imageWidth;		// width of actual image
	int					imageHeight;	// height of actual image
	float				s;				// x offset in image where glyph starts
	float				t;				// y offset in image where glyph starts
	float				s2;
	float				t2;
	const arcMaterial *	glyph;			// shader with the glyph
	char				shaderName[32];
} glyphInfo_t;

typedef struct {
	glyphInfo_t			glyphs[GLYPHS_PER_FONT];
	float				glyphScale;
	char				name[64];
} fontInfo_t;

typedef struct {
	fontInfo_t			fontInfoS;
	fontInfo_t			fontInfoM;
	fontInfo_t			fontInfoXL;
	int					maxHeight;
	int					maxWidth;
	int					maxHeightS;
	int					maxWidthS;
	int					maxHeightM;
	int					maxWidthM;
	int					maxHeighXL;
	int					maxWidthXL;
	char				name[64];
} fontInfoEx_t;

const int SMALLCHAR_WIDTH		= 8;
const int SMALLCHAR_HEIGHT		= 16;
const int BIGCHAR_WIDTH			= 16;
const int BIGCHAR_HEIGHT		= 16;

// all drawing is done to a 640 x 480 virtual screen size
// and will be automatically scaled to the real resolution
const int SCREEN_WIDTH			= 640;
const int SCREEN_HEIGHT			= 480;

// NOTE: im thinking this will replace the Rendersystem games resolutions(s).
//#define	RENDER_WIDTH			1280
//#define RENDER_HEIGHT			720

class ARCRenderWorld;
class ARCRenderSystem {
public:

	virtual					~ARCRenderSystem() {}

	// set up cvars and basic data structures, but don't
	// init OpenGL, so it can also be used for dedicated servers
	virtual void			Init( void ) = 0;

	// only called before quitting
	virtual void			Shutdown( void ) = 0;

	//virtual void			ResetGuiModels() = 0;
	virtual void			FlushLevelImages( void ) = 0;

	virtual void			InitOpenGL( void ) = 0;

	virtual void			ShutdownOpenGL( void ) = 0;

	virtual bool			IsOpenGLRunning( void ) const = 0;

	virtual bool			IsFullScreen( void ) const = 0;
	virtual int				GetScreenWidth() const = 0;
	virtual int				GetScreenHeight() const = 0;
	virtual int				GetWidth() const { return GetScreenWidth(); }
	virtual int				GetHeight() const { return GetScreenHeight(); }

	// return w/h of a single pixel. This will be 1.0 for normal cases.
	// A side-by-side stereo 3D frame will have a pixel aspect of 0.5.
	// A top-and-bottom stereo 3D frame will have a pixel aspect of 2.0
	virtual float			GetPixelAspect() const = 0;

	// GetWidth() / GetHeight() return the size of a single eye
	// view, which may be replicated twice in a stereo display
	//virtual stereo3DMode_t	GetStereo3DMode() const = 0;
	//virtual bool			IsStereoScopicRenderingSupported() const = 0;
	//virtual stereo3DMode_t	GetStereoScopicRenderingMode() const = 0;
	//virtual void			EnableStereoScopicRendering( const stereo3DMode_t mode ) const = 0;
	virtual bool			HasQuadBufferSupport() const = 0;

	// allocate a renderWorld to be used for drawing
	virtual ARCRenderWorld	*AllocRenderWorld( void ) = 0;
	virtual	void			FreeRenderWorld( ARCRenderWorld * rw ) = 0;

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	virtual void			BeginLevelLoad( void ) = 0;
	virtual void			EndLevelLoad( void ) = 0;
	//virtual void			Preload( const PreloadManifest &manifest, const char *mapName ) = 0;
	//virtual void			LoadLevelImages() = 0;

	virtual	void			ExportMD5R( bool compressed ) = 0;
	virtual void			CopyPrimBatchTriangles( arcDrawVert *destDrawVerts, qglIndex_t *destIndices, void *primBatchMesh, void *silTraceVerts ) = 0;
	//virtual void			CopyPrimBatchTriangles( arcDrawVert *destDrawVerts, qglIndex_t *destIndices, aRcMesh *primBatchMesh, const aRcSilTraceVertT *silTraceVerts ) = 0;
	// font support
	virtual bool			RegisterFont( const char *fontName, fontInfoEx_t &font ) = 0;

	// GUI drawing just involves shader parameter setting and axial image subsections
	virtual void			SetColor( const arcVec4 &rgba ) = 0;
	virtual void			SetColor2( const arcVec4 &rgba ) = 0;
	virtual void			SetColor4( float r, float g, float b, float a ) = 0;
	virtual uint32			GetColor2() = 0;

	virtual void			DrawFilled( const arcVec4 & color, float x, float y, float w, float h ) = 0;
	virtual void			DrawStretchPic( const arcDrawVert *verts, const qglIndex_t *indexes, int vertCount, int indexCount, const arcMaterial *material, bool clip = true, float min_x = 0.0f, float min_y = 0.0f, float max_x = 640.0f, float max_y = 480.0f ) = 0;
	virtual void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const arcMaterial *material ) = 0;

	virtual void			DrawStretchTri ( arcVec2 p1, arcVec2 p2, arcVec2 p3, arcVec2 t1, arcVec2 t2, arcVec2 t3, const arcMaterial *material ) = 0;
	virtual arcDrawVert		*AllocTris( int numVerts, const triIndex_t * indexes, int numIndexes, const arcMaterial * material, const stereoDepthType_t stereoType = STEREO_DEPTH_TYPE_NONE ) = 0;

	virtual void			GlobalToNormalizedDeviceCoordinates( const arcVec3 &global, arcVec3 &ndc ) = 0;

	virtual void			GetGLSettings( int& width, int& height ) = 0;
	virtual void			PrintMemInfo( MemInfo_t *mi ) = 0;

	virtual void			DrawSmallChar( int x, int y, int ch, const arcMaterial *material ) = 0;
	virtual void			DrawSmallStringExt( int x, int y, const char *string, const arcVec4 &setColor, bool forceColor, const arcMaterial *material ) = 0;
	virtual void			DrawBigChar( int x, int y, int ch, const arcMaterial *material ) = 0;
	virtual void			DrawBigStringExt( int x, int y, const char *string, const arcVec4 &setColor, bool forceColor, const arcMaterial *material ) = 0;

	// FIXME: add an interface for arbitrary point/texcoord drawing

	// a frame cam consist of 2D drawing and potentially multiple 3D scenes
	// window sizes are needed to convert SCREEN_WIDTH / SCREEN_HEIGHT values
	virtual void			BeginFrame( int winWidth, int winHeight ) = 0;

	virtual	void			RenderLightFrustum( const struct renderLight_s &renderLight, arcPlane lightFrustum[6] ) = 0;
	virtual	void			LightProjectionMatrix( const arcVec3 &origin, const arcPlane &rearPlane, arcVec4 mat[4] ) = 0;
	virtual void			ToggleSmpFrame( void ) = 0;
	virtual bool			IsSMPEnabled( void ) = 0;

	// Performs final closeout of any gui models being defined.
	//
	// Waits for the previous GPU rendering to complete and vsync.
	//
	// Returns the head of the linked command list that was just closed off.
	//
	// Returns timing information from the previous frame.
	//
	// After this is called, new command buffers can be built up in parallel
	// with the rendering of the closed off command buffers by RenderCommandBuffers()
	virtual const setBufferCommand_t *SwapCommandBuffers( uint64 *frontEndMicroSec, uint64 *backEndMicroSec, uint64 *shadowMicroSec, uint64 *gpuMicroSec ) = 0;

	// SwapCommandBuffers operation can be split in two parts for non-smp rendering
	// where the GPU is idled intentionally for minimal latency.
	virtual void			SwapCommandBuffers_FinishRendering( uint64 *frontEndMicroSec, uint64 *backEndMicroSec, uint64 *shadowMicroSec, uint64 *gpuMicroSec ) = 0;
	virtual const setBufferCommand_t *SwapCommandBuffers_FinishCommandBuffers() = 0;

	// issues GPU commands to render a built up list of command buffers returned
	// by SwapCommandBuffers().  No references should be made to the current frameData,
	// so new scenes and GUIs can be built up in parallel with the rendering.
	virtual void			RenderCommandBuffers( const setBufferCommand_t *commandBuffers ) = 0;

	// if the pointers are not NULL, timing info will be returned
	virtual void			EndFrame( int *frontEndMsec, int *backEndMsec, int *numVerts = NULL, int *numIndexes = NULL ) = 0;

	// aviDemo uses this.
	// Will automatically tile render large screen shots if necessary
	// Samples is the number of jittered frames for anti-aliasing
	// If ref == NULL, session->updateScreen will be used
	// This will perform swapbuffers, so it is NOT an approppriate way to
	// generate image files that happen during gameplay, as for savegame
	// markers.  Use WriteRender() instead.
	virtual void			TakeScreenshot( int width, int height, const char *fileName, int samples, struct renderView_s *ref ) = 0;

	// the render output can be cropped down to a subset of the real screen, as
	// for save-game reviews and split-screen multiplayer.  Users of the renderer
	// will not know the actual pixel size of the area they are rendering to

	// the x,y,width,height values are in virtual SCREEN_WIDTH / SCREEN_HEIGHT coordinates

	// to render to a texture, first set the crop size with makePowerOfTwo = true,
	// then perform all desired rendering, then capture to an image
	// if the specified physical dimensions are larger than the current cropped region, they will be cut down to fit
	virtual void			CropRenderSize( int width, int height, bool makePowerOfTwo = false, bool forceDimensions = false ) = 0;
	virtual void			CaptureRenderToImage( const char *imageName ) = 0;
	// fixAlpha will set all the alpha channel values to 0xff, which allows screen captures
	// to use the default tga loading code without having dimmed down areas in many places
	virtual void			CaptureRenderToFile( const char *fileName, bool fixAlpha = false ) = 0;
	virtual void			UnCrop() = 0;
	virtual void			GetCardCaps( bool &oldCard, bool &nv10or20 ) = 0;

	// the image has to be already loaded ( most straightforward way would be through a FindMaterial )
	// texture filter / mipmapping / repeat won't be modified by the upload
	// returns false if the image wasn't found
	virtual bool			UploadImage( const char *imageName, const byte *data, int width, int height ) = 0;

	virtual	void			BindImage( textureType_t target, GLuint image ) = 0;
	virtual void			SetGLState( const GLint stateVector ) = 0;
	virtual void			SetGLTexEnv( int env ) = 0;
	virtual	void			SelectTextureUnit( int unit ) = 0;
	virtual void			SetDefaultGLState( void ) = 0;
	virtual void			SetGL2D( void ) = 0;
	virtual void			SetCull( int cullType ) = 0;

	virtual const glconfig_t &GLConfig() const = 0;\

	// consoles switch stereo 3D eye views each 60 hz frame
	virtual int				GetFrameCount() const = 0;
	virtual int				GetDoubleBufferIndex( void ) = 0;
}; extern ARCRenderSystem	*renderSystem;

//
// functions mainly intended for editor and dmap integration
//

// returns the frustum planes in world space
void R_RenderLightFrustum( const struct renderLight_s &renderLight, arcPlane lightFrustum[6] );

// for use by dmap to do the carving-on-light-boundaries and for the editor for display
//void LightProjectionMatrix( const arcVec3 &origin, const arcPlane &rearPlane, arcVec4 mat[4] );

// used by the view shot taker
void R_ScreenshotFilename( int &lastNumber, const char *base, arcNetString &fileName );

ARC_INLINE void	GL_Scissor( const ARCScreenRect & rect ) { GL_Scissor( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 ); }
ARC_INLINE void	GL_Viewport( const ARCScreenRect & rect ) { GL_Viewport( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 ); }
ARC_INLINE void	GL_ViewportAndScissor( int x, int y, int w, int h ) { GL_Viewport( x, y, w, h ); GL_Scissor( x, y, w, h ); }
ARC_INLINE void	GL_ViewportAndScissor( const ARCScreenRect& rect ) { GL_Viewport( rect ); GL_Scissor( rect ); }

#include "../idlib/Lib.h"

// vertex cache calls should only be made by the front end

// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
const int SMP_FRAMES =				1;

const int VERTCACHE_INDEX_MEMORY_PER_FRAME = 31 * 1024 * 1024;
const int VERTCACHE_VERTEX_MEMORY_PER_FRAME = 31 * 1024 * 1024;

const int VERTCACHE_NUM_FRAMES = 2;	// just incase someone forgets uses this or the other its ok :)
const int NUM_VERTEX_FRAMES = 2;

typedef uint64 vertCacheHandle_t;

// there are a lot more static indexes than vertexes, because interactions are just new
// index lists that reference existing vertexes
const int STATIC_INDEX_MEMORY = 31 * 1024 * 1024;
const int STATIC_VERTEX_MEMORY = 31 * 1024 * 1024;	// make sure it fits in VERTCACHE_OFFSET_MASK!

// vertCacheHandle_t packs size, offset, and frame number into 64 bits
typedef uint64 vertCacheHandle_t;
const int VERTCACHE_STATIC = 1;					// in the static set, not the per-frame set
const int VERTCACHE_SIZE_SHIFT = 1;
const int VERTCACHE_SIZE_MASK = 0x7fffff;		// 8 megs
const int VERTCACHE_OFFSET_SHIFT = 24;
const int VERTCACHE_OFFSET_MASK = 0x1ffffff;	// 32 megs
const int VERTCACHE_FRAME_SHIFT = 49;
const int VERTCACHE_FRAME_MASK = 0x7fff;		// 15 bits = 32k frames to wrap around

const int VERTEX_CACHE_ALIGN		= 32;
const int INDEX_CACHE_ALIGN			= 16;

typedef enum {
	TAG_FREE,
	TAG_USED,
	TAG_FIXED,		// for the temp buffers
	TAG_TEMP		// in frame temp area, not static area
} vertBlockTag_t;

typedef struct vertCache_s {
	GLuint					vbo;
	void					*virtMem;			// only one of vbo / virtMem will be set
	bool					indexBuffer;		// holds indexes instead of vertexes

	int						offset;
	int						size;				// may be larger than the amount asked for, due
										// to round up and minimum fragment sizes
	int						tag;				// a tag of 0 is a free block
	struct vertCache_s		**user;				// will be set to zero when purged
	struct vertCache_s 		*next, *prev;	// may be on the static list or one of the frame lists
	int						frameUsed;			// it can't be purged if near the current frame
} vertCache_t;

typedef struct srfVert_t {
    float					xyz[3];  // Position coordinates (x, y, z)
    float					st[2];   // Texture coordinates ( s, t)
};

class anVertexCache {
public:
	void			Init();
	void			Shutdown();

	// just for gfxinfo printing
	bool			IsFast();

	// called when vertex programs are enabled or disabled, because
	// the cached data is no longer valid
	void			PurgeAll();

	// call on loading a new map
	void			FreeStaticData();/////////////

	// Tries to allocate space for the given data in fast vertex
	// memory, and copies it over.
	// Alloc does NOT do a touch, which allows purging of things
	// created at level load time even if a frame hasn't passed yet.
	// These allocations can be purged, which will zero the pointer.
	void			Alloc( void *data, int bytes, vertCache_t **buffer, bool indexBuffer = false );
	vertCacheHandle_t	AllocVertex( const void * data, int bytes ) { return ActuallyAlloc( frameData[listNum], data, bytes, CACHE_VERTEX );}
	vertCacheHandle_t	AllocIndex( const void * data, int bytes ) { return ActuallyAlloc( frameData[listNum], data, bytes, CACHE_INDEX );}

	static vertCacheHandle_t AllocStaticVertex( const void *data, int bytes );
	static vertCacheHandle_t AllocStaticIndex( const void *data, int bytes );

	// This will be a real pointer with memory,
	// but it will be an int offset cast to a pointer of ARB_vertex_buffer_object
	void *			Position( vertCache_t *buffer );

	// if r_useIndexBuffers is enabled, but you need to draw something without
	// an indexCache, this must be called to reset GL_ELEMENT_ARRAY_BUFFER_ARB
	void			UnbindIndex();

	// automatically freed at the end of the next frame
	// used for specular texture coordinates and gui drawing, which
	// will change every frame.
	// will return nullptr if the vertex cache is completely full
	// As with Position(), this may not actually be a pointer you can access.
	vertCache_t	*	AllocFrameTemp( void *data, int bytes );

	// notes that a buffer is used this frame, so it can't be purged
	// out from under the GPU
	void			Touch( vertCache_t *buffer );

	// this block won't have to zero a buffer pointer when it is purged,
	// but it must still wait for the frames to pass, in case the GPU
	// is still referencing it
	void			Free( vertCache_t *buffer );

	// updates the counter for determining which temp space to use
	// and which blocks can be purged
	// Also prints debugging info when enabled
	void			EndFrame();

	// listVertexCache calls this
	void			List();

private:
	void			InitMemoryBlocks( int size );
	void			ActuallyFree( vertCache_t *block );

	static anCVarSystem	r_showVertexCache;
	static anCVarSystem	r_vertexBufferMegs;

	int				staticCountTotal;
	int				staticAllocTotal;		// for end of frame purging

	int				staticAllocThisFrame;	// debug counter
	int				staticCountThisFrame;
	int				dynamicAllocThisFrame;
	int				dynamicCountThisFrame;

	int				currentFrame;			// for purgable block tracking
	int				listNum;				// currentFrame % NUM_VERTEX_FRAMES, determines which tempBuffers to use

	bool			virtualMemory;			// not fast stuff

	bool			allocatingTempBuffer;	// force GL_STREAM_DRAW_ARB

	vertCache_t		*tempBuffers[NUM_VERTEX_FRAMES];		// allocated at startup
	bool			tempOverflow;			// had to alloc a temp in static memory

	anBlockAlloc<vertCache_t,1024>	headerAllocator;

	vertCache_t		freeStaticHeaders;		// head of doubly linked list
	vertCache_t		freeDynamicHeaders;		// head of doubly linked list
	vertCache_t		dynamicHeaders;			// head of doubly linked list
	vertCache_t		deferredFreeList;		// head of doubly linked list
	vertCache_t		staticHeaders;			// head of doubly linked list in MRU order,
											// staticHeaders.next is most recently used
	vertCache_t		freeDynamicHeaders;

	anRenderCache staticData;
	anRenderCache frameData[VERTCACHE_NUM_FRAMES];
	int				frameBytes;				// for each of NUM_VERTEX_FRAMES frames
};

extern	anVertexCache	vertexCache;


/*
=============================================================

RENDERER BACK END COMMAND QUEUE

moved from tr_local.h

do we want to add these back?
RC_SET_COLOR
RC_COLORMASK
RC_CLEARDEPTH
RC_CLEARCOLOR

=============================================================
*/

typedef enum {
	RC_NOP,		//RC_END_OF_LIST ??
	RC_DRAW_VIEW,
	RC_DRAW_VIEW_GUI,
	RC_POST_PROCESS,	// HDR post processing AKA bloom.
	RC_SET_BUFFER,
	RC_COPY_RENDER,
	RC_SWAP_BUFFERS		// can't just assume swap at end of list because
						// of forced list submission before syncs
} renderCommand_t;

// draw SurfsCommand_t empty Command_t copy RenderCommand_t all combined into one structure now
// should help with memory management and performance as well as organizing.
typedef struct {
	renderCommand_t	commandId, *next;
} setBufferCommand_t;

// this is the inital allocation for max number of drawsurfs
// in a given view, but it will automatically grow if needed
const int				INITIAL_DRAWSURFS = 0x4000;

// a request for frame memory will never fail
// (until malloc fails), but it may force the
// allocation of a new memory block that will
// be discontinuous with the existing memory
// all of the information needed by the back end must be
// contained in a frameData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine (OBSOLETE: this capability has been removed)
typedef struct {
	renderCommand_t		*rNext;
	renderCommand_t		commandId, *next;
	GLenum				buffer;
	GLint				frameCount; // was int
	GLint				x, y, imageWidth, imageHeight;
	anImage *			image;
	int					cubeFace;	// when copying to a cubeMap
	viewDef_t			*viewDef;
	int					size;
	int					used;
	int					poop;		// so that base is 16 byte aligned
	byte				base[4];	// dynamically allocated as [size]

	setBufferCommand_t *renderCommand; // already merged setBuffercommand_t leaving for one call the next just temporarily.
	// one or more blocks of memory for all frame
	// temporary allocations
	frameData_t	*		memory;

	// alloc will point somewhere into the memory chain
	frameData_t	*		alloc;

	srfTriangles_t		*firstDeferredFreeTriSurf;
	srfTriangles_t		*lastDeferredFreeTriSurf;

	int					memoryHighwater;	// max used on any frame

	// the currently building command list
	// commands can be inserted at the front if needed, as for required
	// dynamically generated textures
	setBufferCommand_t		*cmdHead, *cmdTail;		// may be of other command type based on commandId
} frameData_t;
extern frameData_t		*frameData;

class anRenderCache {
public:
	void					    Init();

						~anRenderCache() {DeleteFrameBuffer();DeleteVertexBuffer();DeleteIndexBuffer();DeleteVertexArrayObject();}

	void				R_InitFrameData( void );

	void		InitFrameBuffer();
	void		InitVertexBuffer();
	void		InitIndexBuffer();
	void		InitVertexArrayObject();

	void				AllocVertexBuffer( GLuint size );

	void *				GetAPIObject() const { return base; }
	int					GetSize() const { return ( size ); }

	void		BindFrameBuffer();
	void		BindVertexBuffer();
	void		BindIndexBuffer();
	void		BindVertexArrayObject();

	void		UnbindFrameBuffer();
	void		UnbindVertexBuffer();
	void		UnbindIndexBuffer();
	void		UnbindVertexArrayObject();

	void		ActuallyFreeVertexBuffers();

	void		DeleteFrameBuffer();
	void		DeleteVertexBuffer();
	void		DeleteIndexBuffer();
	void		DeleteVertexArrayObject();

	void				ShutdownFrameData( void );

	//void				Swap()
	// we can now begin to get this stuff out of tr_backend and tr_render slowly get this more organized.
	// we we need execute buffers and the others for backend rendering stuff moved in here.
	void		SwapBuffers();


	int 				CountFrameData( void );
	void 				ToggleSmpFrame( void );
	void				*FrameAlloc( int bytes );
	void 				*ClearedFrameAlloc( int bytes );
	void				FrameFree( void *data );

	void 				*StaticAlloc( int bytes );		// just malloc with error checking
	void 				*ClearedStaticAlloc( int bytes );	// with memset
	void 				StaticFree( void *data );

	void		RenderView( viewDef_t *parms );
	void		RenderBuffers();
	void		ClearWithoutFreeing();
private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	void				*base;
	GLuint				buffer;
    frameData_t			frameBuffer;
    frameData_t			vertexBuffer;
    GLuint				indexBuffer;
    GLuint				vertexArrayObject;

	char            name[MAX_QPATH];

	int             index;

	uint32_t        colorBuffers[16];
	int             colorFormat;
	struct image_s  *colorImage[16];

	uint32_t        depthBuffer;
	int             depthFormat;

	uint32_t        stencilBuffer;
	int             stencilFormat;

	uint32_t        packedDepthStencilBuffer;
	int             packedDepthStencilFormat;

	int             width;
	int             height;
};

extern anRenderCache *renderCache;


class ImprovedVertexCache {
private:
    struct vertCacheBlock {
		vertCacheBlock** user;
		vertCacheBlock* next;
		vertCacheBlock* prev;
		vertBlockTag_t tag;
        void *virtMem;
        int offset;
        int tags;
        int frameUsed;
        bool indexBuffer;
        bool virtMemDirty;
        int size;
        void *usr;
        GLuint vbo;
};


    vertCacheBlock *staticHeaders;
    vertCacheBlock *freeStaticHeaders;
    vertCacheBlock *freeDynamicHeaders;
    bool virtualMemory;
    int currentFrame;
    int listNum;
    int frameBytes;

public:
    ImprovedVertexCache() {
        // Initialize member variables and allocate initial cache blocks
        staticHeaders = new vertCacheBlock();
        freeStaticHeaders = new vertCacheBlock();
        freeDynamicHeaders = new vertCacheBlock();
        virtualMemory = false;
        currentFrame = 0;
        listNum = 0;
        frameBytes = 0;
    }

    ~ImprovedVertexCache() {
        // Clean up allocated cache blocks and any other resources
        delete staticHeaders;
        delete freeStaticHeaders;
        delete freeDynamicHeaders;
    }

    void List() {
        // Implementation of the List() function goes here
        // You can refer to the previous implementation and modify it as needed
        // Don't forget to update the function signature if needed
    }

        // This function should allocate a temporary cache block with the given size and return its handle
    vertCacheBlock *CreateTempVbo( int bytes, bool indexBuffer ) {
        // Implementation of the CreateTempVbo() function goes here
        // You can refer to the previous implementation and modify it as needed
        // Don't forget to update the function signature and return type if needed
        // You'll also need to handle memory allocation and deallocation for the cache blocks
        // using appropriate memory management mechanisms for the Doom 3 engine
	}

    int GetNextListNum() const {
        // Implementation of the GetNextListNum() function goes here
        // You can refer to the previous implementation and modify it as needed
        // Don't forget to update the function signature and return type if needed
    }

    bool IsFast() {
        // Implementation of the IsFast() function goes here
        // You can refer to the previous implementation and modify it as needed
        // Don't forget to update the function signature and return type if needed
    }
};


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

typedef struct {
	int			width;
	int			height;
	bool		fullScreen;
	bool		stereo;
	int			displayHz;
	int			multiSamples;
	float		pixelAspect;		// pixel width / height, should be 1.0 in most cases
	bool		fullscreenAvail;
} glimpParms_t;

bool		GLimp_Init( glimpParms_t parms );
// If the desired mode can't be set satisfactorily, false will be returned.
// The renderer will then reset the glimpParms to "safe mode" of 640x480
// fullscreen and try again.  If that also fails, the error will be fatal.

bool		GLimp_SetScreenParms( glimpParms_t parms );
// will set up gl up with the new parms

void		GLimp_Shutdown( void );
// Destroys the rendering context, closes the window, resets the resolution,
// and resets the gamma ramps.

void		GLimp_SwapBuffers( void );
// Calls the system specific swapbuffers routine, and may also perform
// other system specific cvar checks that happen every frame.
// This will not be called if 'r_drawBuffer GL_FRONT'

void		GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] );
// Sets the hardware gamma ramps for gamma and brightness adjustment.
// These are now taken as 16 bit values, so we can take full advantage
// of dacs with >8 bits of precision

bool		GLimp_SpawnRenderThread( void (*function)( void ) );
// Returns false if the system only has a single processor

void *		GLimp_BackEndSleep( void );
void		GLimp_FrontEndSleep( void );
void		GLimp_WakeBackEnd( void *data );
// these functions implement the dual processor syncronization

void		GLimp_ActivateContext( void );
void		GLimp_DeactivateContext( void );
// These are used for managing SMP handoffs of the OpenGL context
// between threads, and as a performance tunining aid.  Setting
// 'r_skipRenderContext 1' will call GLimp_DeactivateContext() before
// the 3D rendering code, and GLimp_ActivateContext() afterwards.  On
// most OpenGL implementations, this will result in all OpenGL calls
// being immediate returns, which lets us guage how much time is
// being spent inside OpenGL.

void		GLimp_EnableLogging( bool enable );


void R_LockSurfaceScene( viewDef_t *parms );
void R_ClearCommandChain( void );
void R_AddDrawViewCmd( viewDef_t *parms );

void R_ReloadGuis_f( const anCommandArgs &args );
void R_ListGuis_f( const anCommandArgs &args );

void *R_GetCommandBuffer( int bytes );

// this allows a global override of all materials
bool R_GlobalShaderOverride( const anMaterial **shader );

// this does various checks before calling the anDeclSkin
const anMaterial *R_RemapShaderBySkin( const anMaterial *shader, const anDeclSkin *customSkin, const anMaterial *customShader );
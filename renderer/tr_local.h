#ifndef __TR_LOCAL_H__
#define __TR_LOCAL_H__

#include "Image.h"

class anRenderWorldLocal;
class anMaterial;
class anDeclMaterial;

const int FALLOFF_TEXTURE_SIZE =	64;
const float	DEFAULT_FOG_DISTANCE =	500.0f;
const int FOG_ENTER_SIZE =			64;
const float FOG_ENTER = ( FOG_ENTER_SIZE+1.0f )/( FOG_ENTER_SIZE*2 );

// picky to get the bilerp correct at terminator
// anScreenRect gets carried around with each drawSurf, so it makes sense
// to keep it compact, instead of just using the anBounds class
class anScreenRect {
public:
	short		x1, y1, x2, y2;							// inclusive pixel bounds inside viewport
    float       zMin, zmax;								// for depth bounds test

	void		Clear();								// clear to backwards values
	void		AddPoint( float x, float y );			// adds a point
	void		Expand();								// expand by one pixel each way to fix roundoffs
	void		Intersect( const anScreenRect &rect );
	void		Union( const anScreenRect &rect );
	bool		Equals( const anScreenRect &rect ) const;
	bool		IsEmpty() const;

	anScreenRect RectFromViewFrustumBounds( const anBounds &bounds );
	anScreenRect CalcIntersectionBounds( const viewDef_t *viewDef );
};

anScreenRect R_ScreenRectFromViewFrustumBounds( const anBounds &bounds );
void R_ShowColoredScreenRect( const anScreenRect &rect, int colorIndex );

void TransposeGLMatrix( const float in[16], float out[16] );
void InvertByTranspose( const float a[16], float r[16] );
void FullInvert( const float a[16], float r[16] );

void FinishStageTexture( const textureStage_t *texture, const drawSurf_t *surf );
void AxisToModelMatrix( const anMat3 &axis, const anVec3 &origin, float modelMatrix[16] );

// note that many of these assume a normalized matrix, and will not work with scaled axis
void GlobalPointToLocal( const float modelMatrix[16], const anVec3 &in, anVec3 &out );
void GlobalVectorToLocal( const float modelMatrix[16], const anVec3 &in, anVec3 &out );
void GlobalPlaneToLocal( const float modelMatrix[16], const anPlane &in, anPlane &out );
void PointTimesMatrix( const float modelMatrix[16], const anVec4 &in, anVec4 &out );
void LocalPointToGlobal( const float modelMatrix[16], const anVec3 &in, anVec3 &out );
void LocalVectorToGlobal( const float modelMatrix[16], const anVec3 &in, anVec3 &out );
void LocalPlaneToGlobal( const float modelMatrix[16], const anPlane &in, anPlane &out );
void TransformModelToClip( const anVec3 &src, const float *modelMatrix, const float *projectionMatrix, anPlane &eye, anPlane &dst );
void TransformEyeZToWin( float srcZ, const float *projectionMatrix, float &dstZ );
void GL_MultMatrixAligned( const float *a, const float *b, float *out );
static float CalcSplit( float n, float f, float i, float m );

/*
==============================================================================

SURFACES

==============================================================================
*/

#include "ModelDecal.h"
#include "ModelOverlay.h"
#include "Interaction.h"

// drawSurf_t structures command the back end to render surfaces
// a given srfTriangles_t may be used with multiple viewEntity_t,
// as when viewed in a subview or multiple viewport render, or
// with multiple shaders when skinned, or, possibly with multiple
// lights, although currently each lighting interaction creates
// unique srfTriangles_t

// drawSurf_t are always allocated and freed every frame, they are never cached
static const int	DSF_VIEW_INSIDE_SHADOW	= 1;

typedef struct drawSurf_s {
	const srfTriangles_t	*geo;

	GLint					numIndexes;
	vertCacheHandle_t		indexCache;			// triIndex_t
	vertCacheHandle_t		ambientCache;		// anDrawVertex
	vertCacheHandle_t		shadowCache;		// anShadowCache / anShadowCache
	vertCacheHandle_t		jointCache;			// anJointMat

	const struct viewEntity_s *space;
	const anMaterial		*material;			// may be nullptr for shadow volumes
	GLint					ExtraGLStateBits;	// Extra GL state |'d with material->stage[].drawStateBits

	float					sort;				// material->sort, modified by gui / entity sort offsets
	const float				*shaderRegisters;	// evaluated and adjusted for referenceShaders

	const struct drawSurf_s	*nextOnLight;		// viewLight chains

	anScreenRect			scissorRect;		// for scissor clipping, local inside renderView viewport
	int						dsFlags;			// DSF_VIEW_INSIDE_SHADOW, etc

	struct vertCache_s		*dynamicTexCoords;	// float * in vertex cache memory
	// specular directions for non vertex program cards, skybox texcoords, etc

	GLint					renderZFail;
	//volatile shadowVolumeState_t shadowVolumeState;
} drawSurf_t;

typedef struct {
	int			numPlanes;		// this is always 6 for now
	anPlane	planes[6];
	// positive sides facing inward
	// plane 5 is always the plane the projection is going to, the
	// other planes are just clip planes
	// all planes are in global coordinates

	bool	makeClippedPlanes;
	// a projected light with a single frustum needs to make sil planes
	// from triangles that clip against side planes, but a point light
	// that has adjacent frustums doesn't need to
} shadowFrustum_t;

// areas have references to hold all the lights and entities in them
typedef struct areaReference_s {
	struct areaReference_s *areaNext;				// chain in the area
	struct areaReference_s *areaPrev;
	struct areaReference_s *ownerNext;				// chain on either the entityDef or lightDef
	anRenderEntityLocal *	entity;					// only one of entity / light will be non-nullptr
	anRenderLightsLocal *	light;					// only one of entity / light will be non-nullptr
	struct portalArea_s	*	area;					// so owners can find all the areas they are in
} areaReference_t;

// anRenderLight should become the new public interface replacing the arcNetHandle_t to light defs in the anRenderWorld interface
class anRenderLight {
public:
	virtual					~anRenderLight() {}

	virtual void			FreeRenderLight() = 0;
	virtual void			UpdateRenderLight( const renderLight_t *re, bool forceUpdate = false ) = 0;
	virtual void			GetRenderLight( renderLight_t *re ) = 0;
	virtual void			ForceUpdate() = 0;
	virtual int				GetIndex() = 0;
};

// anRenderEntity should become the new public interface replacing the arcNetHandle_t to entity defs in the anRenderWorld interface
class anRenderEntity {
public:
	virtual					~anRenderEntity() {}

	virtual void			FreeRenderEntity() = 0;
	virtual void			UpdateRenderEntity( const renderEntity_t *re, bool forceUpdate = false ) = 0;
	virtual void			GetRenderEntity( renderEntity_t *re ) = 0;
	virtual void			ForceUpdate() = 0;
	virtual int				GetIndex() = 0;

	// overlays are extra polygons that deform with animating models for blood and damage marks
	virtual void			ProjectOverlay( const anPlane localTextureAxis[2], const anMaterial *material ) = 0;
	virtual void			RemoveDecals() = 0;
};

class anRenderLightsLocal : public anRenderLight {
public:
							anRenderLightsLocal();

	virtual void			FreeRenderLight();
	virtual void			UpdateRenderLight( const renderLight_t *re, bool forceUpdate = false );
	virtual void			GetRenderLight( renderLight_t *re );
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	renderLight_t			parms;					// specification

	bool					lightHasMoved;			// the light has changed its position since it was
													// first added, so the prelight model is not valid

	float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin

	anRenderWorldLocal *	world;
	int						index;					// in world lightdefs

	int						areaNum;				// if not -1, we may be able to cull all the light's
													// interactions if !viewDef->connectedAreas[areaNum]

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
													// and should go in the dynamic frame memory, or kept
													// in the cached memory
	bool					archived;				// for demo writing


	// derived information
	anPlane					lightProject[4];

	const anMaterial		*lightShader;			// guaranteed to be valid, even if parms.shader isn't
	anImage					*falloffImage;

	anVec3					globalLightOrigin;		// accounting for lightCenter and parallel


	anPlane					frustum[6];				// in global space, positive side facing out, last two are front/back
	anWinding *				frustumWindings[6];		// used for culling
	srfTriangles_t *		frustumTris;			// triangulated frustumWindings[]

	int						numShadowFrustums;		// one for projected lights, usually six for point lights
	shadowFrustum_t			shadowFrustums[6];

	int						viewCount;				// if == tr.viewCount, the light is on the viewDef->viewLights list
	struct viewLight_s *	viewLight;

	areaReference_t *		references;				// each area the light is present in will have a lightRef
	anInteraction *		firstInteraction;		// doubly linked list
	anInteraction *		lastInteraction;

	struct doublePortal_s *	foggedPortals;
};

class anRenderEntityLocal : public anRenderEntity {
public:
							anRenderEntityLocal();

	virtual void			FreeRenderEntity();
	virtual void			UpdateRenderEntity( const renderEntity_t *re, bool forceUpdate = false );
	virtual void			GetRenderEntity( renderEntity_t *re );
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	// overlays are extra polygons that deform with animating models for blood and damage marks
	virtual void			ProjectOverlay( const anPlane localTextureAxis[2], const anMaterial *material );
	virtual void			RemoveDecals();

	renderEntity_t			parms;

	float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin

	anRenderWorldLocal		*world;
	int						index;					// in world entityDefs

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
													// and should go in the dynamic frame memory, or kept
													// in the cached memory
	bool					archived;				// for demo writing

	anRenderModel			*dynamicModel;			// if parms.model->IsDynamicModel(), this is the generated data
	int						dynamicModelFrameCount;	// continuously animating dynamic models will recreate
													// dynamicModel if this doesn't == tr.viewCount
	anRenderModel			*cachedDynamicModel;

	anBounds				referenceBounds;		// the local bounds used to place entityRefs, either from parms or a model

	// a viewEntity_t is created whenever a anRenderEntityLocal is considered for inclusion
	// in a given view, even if it turns out to not be visible
	int						viewCount;				// if tr.viewCount == viewCount, viewEntity is valid,
													// but the entity may still be off screen
	struct viewEntity_s		*viewEntity;				// in frame temporary memory

	int						visibleCount;
	// if tr.viewCount == visibleCount, at least one ambient
	// surface has actually been added by R_AddAmbientDrawsurfs
	// note that an entity could still be in the view frustum and not be visible due
	// to portal passing

	anRenderModelDecal *	decals;					// chain of decals that have been projected on this model
	anRenderModelOverlay *	overlay;				// blood overlays on animated models

	areaReference_t *		entityRefs;				// chain of all references
	anInteraction *		firstInteraction;		// doubly linked list
	anInteraction *		lastInteraction;

	bool					needsPortalSky;
};

// viewLights are allocated on the frame temporary stack memory
// a viewLight contains everything that the back end needs out of an anRenderLightsLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// a viewLight may exist even without any surfaces, and may be relevent for fogging,
// but should never exist if its volume does not intersect the view frustum
typedef struct viewLight_s {
	struct viewLight_s *	next;

	// back end should NOT reference the lightDef, because it can change when running SMP
	anRenderLightsLocal	*	lightDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewEntity_t was never actually
	// seen through any portals
	anScreenRect			scissorRect;

	// if the view isn't inside the light, we can use the non-reversed
	// shadow drawing, avoiding the draws of the front and rear caps
	bool					viewInsideLight;

	// true if globalLightOrigin is inside the view frustum, even if it may
	// be obscured by geometry.  This allows us to skip shadows from non-visible objects
	bool					viewSeesGlobalLightOrigin;

	// if !viewInsideLight, the corresponding bit for each of the shadowFrustum
	// projection planes that the view is on the negative side of will be set,
	// allowing us to skip drawing the projected caps of shadows if we can't see the face
	int						viewSeesShadowPlaneBits;

	anVec3					globalLightOrigin;			// global light origin used by backend
	anPlane					lightProject[4];			// light project used by backend
	anPlane					fogPlane;					// fog plane for backend fog volume rendering
	const srfTriangles_t *	frustumTris;				// light frustum for backend fog volume rendering
	const anMaterial *		lightShader;				// light shader used by backend
	const float *			shaderRegisters;			// shader registers used by backend
	anImage *				falloffImage;				// falloff image used by backend

	const struct drawSurf_s	*globalShadows;				// shadow everything
	const struct drawSurf_s	*localInteractions;			// don't get local shadows
	const struct drawSurf_s	*localShadows;				// don't shadow local Surfaces
	const struct drawSurf_s	*globalInteractions;		// get shadows from everything
	const struct drawSurf_s	*translucentInteractions;	// get shadows from everything
} viewLight_t;

// a viewEntity is created whenever a anRenderEntityLocal is considered for inclusion
// in the current view, but it may still turn out to be culled.
// viewEntity are allocated on the frame temporary stack memory
// a viewEntity contains everything that the back end needs out of a anRenderEntityLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// A single entityDef can generate multiple viewEntity_t in a single frame, as when seen in a mirror
typedef struct viewEntity_s {
	struct viewEntity_s		*next;

	// back end should NOT reference the entityDef, because it can change when running SMP
	anRenderEntityLocal	*entityDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewEntity_t was never actually
	// seen through any portals, but was created for shadow casting.
	// a viewEntity can have a non-empty scissorRect, meaning that an area
	// that it is in is visible, and still not be visible.
	anScreenRect			scissorRect;

	bool					primaryViewDepthHack;
	float					modelDepthHack;

	float					modelMatrix[16];		// local coords to global coords
	float					modelViewMatrix[16];	// local coords to eye coords
} viewEntity_t;

const int					MAX_CLIP_PLANES	= 1;	// we may expand this to six for some subview issues

// viewDefs are allocated on the frame temporary stack memory
typedef struct viewDef_s {
	// specified in the call to DrawScene()
	renderView_t			renderView;

	float					projectionMatrix[16];
	viewEntity_t			worldSpace;

	anRenderWorldLocal *	renderWorld;

	float					floatTime;

	anVec3					initialViewAreaOrigin;
	// Used to find the portalArea that view flooding will take place from.
	// for a normal view, the initialViewOrigin will be renderView.viewOrg,
	// but a mirror may put the projection origin outside
	// of any valid area, or in an unconnected area of the map, so the view
	// area must be based on a point just off the surface of the mirror / subview.
	// It may be possible to get a failed portal pass if the plane of the
	// mirror intersects a portal, and the initialViewAreaOrigin is on
	// a different side than the renderView.viewOrg is.

	bool					isSubview;				// true if this view is not the main view
	bool					isMirror;				// the portal is a mirror, invert the face culling
	bool					isXraySubview;

	bool					isEditor;

	int						numClipPlanes;			// mirrors will often use a single clip plane
	anPlane					clipPlanes[MAX_CLIP_PLANES];		// in world space, the positive side
												// of the plane is the visible side
	anScreenRect			viewport;				// in real pixels and proper Y flip

	anScreenRect			scissor;
	// for scissor clipping, local inside renderView viewport
	// subviews may only be rendering part of the main view
	// these are real physical pixel values, possibly scaled and offset from the
	// renderView x/y/width/height

	struct viewDef_s 		*superView;				// never go into an infinite subview loop
	struct drawSurf_s 		*subviewSurface;

	// drawSurfs are the visible surfaces of the viewEntities, sorted
	// by the material sort parameter
	drawSurf_t				**drawSurfs;				// we don't use an anList for this, because
	int						numDrawSurfs;			// it is allocated in frame temporary memory
	int						maxDrawSurfs;			// may be resized

	struct viewLight_s		*viewLights;			// chain of all viewLights effecting view
	struct viewEntity_s		*viewEntities;			// chain of all viewEntities effecting view, including off screen ones casting shadows
	// we use viewEntities as a check to see if a given view consists solely
	// of 2D rendering, which we can optimize in certain ways.  A 2D view will
	// not have any viewEntities

	anPlane					frustum[5];				// positive sides face outward, [4] is the front clip plane
	anFrustum				viewFrustum;

	int						areaNum;				// -1 = not in a valid area

	bool					*connectedAreas;
	// An array in frame temporary memory that lists if an area can be reached without
	// crossing a closed door.  This is used to avoid drawing interactions
	// when the light is behind a closed door.
} viewDef_t;

const int	MAX_CLIP_PLANES	= 1;				// we may expand this to six for some subview issues

// complex light / surface interactions are broken up into multiple passes of a
// simple interaction shader
typedef struct {
	const drawSurf_t		*surf;

	anImage					*lightImage;
	anImage					*lightFalloffImage;
	anImage					*bumpImage;
	anImage					*diffuseImage;
	anImage					*specularImage;

	anVec4					diffuseColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	anVec4					specularColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	stageVertexColor_t		vertexColor;	// applies to both diffuse and specular

	int						ambientLight;	// use tr.ambientNormalMap instead of normalization cube map
	// (not a bool just to avoid an uninitialized memory check of the pad region by valgrind)

	// these are loaded into the vertex program
	anVec4				localLightOrigin;
	anVec4				localViewOrigin;
	anVec4				lightProjection[4];	// in local coordinates, possibly with a texture matrix baked in
	anVec4				bumpMatrix[2];
	anVec4				diffuseMatrix[2];
	anVec4				specularMatrix[2];
} drawInteraction_t;

//====================================================

typedef struct {
	int					sphereCull, sphereClip, sphereCullOut;
	int					boxCull, boxCullOut;
	int					createInteractions;	// number of calls to anInteraction::CreateInteraction
	int					createLightTris;
	int					createShadowVolumes;
	int					generateMD5;
	int					entityDefCallbacks;
	int					allocCount, freeCount; // counts for R_StaticAllc/R_StaticFree
	int					visibleViewEntities;
	int					shadowViewEntities;
	int					viewLights;
	int					numViews;			// number of total views rendered
	int					deformedSurfs;		// anMD5Mesh::GenerateSurface
	int					deformedVerts;		// anMD5Mesh::GenerateSurface
	int					deformedIndexes;	// anMD5Mesh::GenerateSurface
	int					tangentIndexes;		// R_DeriveTangents()
	int					entityUpdates, lightUpdates, entityReferences, lightReferences;
	int					guiSurfaces;
	int					frontEndMsec;		// sum of time in all RE_RenderScene's in a frame
} performanceCounters_t;

typedef struct {
	GLint				current2DMap;
	GLint				current3DMap;
	GLint				currentCubeMap;
	GLint				currentSeamlessCubeMap;
	GLint				currentRect;
	GLint				texEnv;
	textureType_t		textureType;
} tmu_t;

const int MAX_MULTITEXTURE_UNITS =	8;
typedef struct {
	tmu_t				tmu[MAX_MULTITEXTURE_UNITS];
	int					currenttmu;
	GLuint				textures[NUM_TEXTURE_BUNDLES];	// honest this is the same as tmu
	GLenum				texUnit;	// and this too currenttmu

	GLuint				program;

	GLuint				drawFramebuffer;
	GLuint				readFramebuffer;
	GLuint				renderbuffer;

	int					faceCulling;

	vertexLayoutType_t	vertexLayout;
	unsigned int		currentVertexBuffer;
	unsigned int		currentIndexBuffer;

	float				polyOfsScale;
	float				polyOfsBias;

	GLuint				glStateBits;
	bool				forceGlState;		// the next GL_State will ignore glStateBits and set everything
} qglState_t;

typedef struct {
	int					surfCount;
	int					shaderCount;
	int					verts;
	int					indexes;		// one set per pass
	int					totalIndexes;	// counting all passes

	int					drawElements;
	int					drawIndexes;
	int					drawVerts;
	int					drawRefIndexes;
	int					drawRefVerts;

	int					shdwElements;
	int					shdwIndexes;
	int					shdwVerts;

	int					vboIndexes;
	float				overDraw;

	float				maxLightVal;	// for light scale
	int					mSec;			// total milliseconds for backend run
} backEndCounters_t;

// all state modified by the back end is separated
// from the front end state
typedef struct {
	int					frameCount;		// used to track all images used in a frame
	const viewDef_t		*viewDef;
	backEndCounters_t	pc;

	const viewEntity_t	*currentSpace;		// for detecting when a matrix must change
	anScreenRect		currentScissor;
	// for scissor clipping, local inside renderView viewport

	viewLight_t			*vLight;
	int					depthFunc;			// GLS_DEPTHFUNC_EQUAL, or GLS_DEPTHFUNC_LESS for translucent
	float				lightTxteMatrix[16];	// only if lightStage->texture.hasMatrix
	float				lightColor[4];		// evaluation of current light's color stage
	anVec4				lightColorVec;

	float				lightScale;			// Every light color calaculation will be multiplied by this,
											// which will guarantee that the result is < tr.backEndRendererMaxLight
											// A card with high dynamic range will have this set to 1.0
	float				overBright;			// The amount that all light interactions must be multiplied by
											// with post processing to get the desired total light level.
											// A high dynamic range card will have this set to 1.0.

	bool				currentRenderCopied	// true if any material has already referenced _currentRender

	arcRenderMatrices	prevMVP[2];			// world MVP from previous frame for motion blur, per-eye

	// surfaces used for code-based drawing
	drawSurf_t			unitSquareSurface;
	drawSurf_t			zeroOneCubeSurface;
	drawSurf_t			testImageSurface;
	// our OpenGL state deltas
	qglState_t			qglState;

	int					copyFrameBufferPC;
} backEndState_t;

const int MAX_GUI_SURFS	= 1024;		// default size of the drawSurfs list for guis, will
									// be automatically expanded as needed
typedef enum {
	BE_ARB,
	BE_NV10,
	BE_NV20,
	BE_R200,
	BE_ARB2,
	BE_BAD
} backEndName_t;

typedef struct {
	int		x, y, width, height;	// these are in physical, OpenGL Y-at-bottom pixels
} renderCrop_t;
static const int	MAX_RENDER_CROPS = 8;

// Most renderer globals are defined here.
// backend functions should never modify any of these fields,
// but may read fields that aren't dynamically modified
// by the frontend.
class anRenderSystemLocal : public anRenderSystem {
public:
	// external functions
	virtual void			Init( void );
	virtual void			Shutdown( void );
	virtual void			InitOpenGL( void );
	virtual void			ShutdownOpenGL( void );
	virtual bool			IsOpenGLRunning( void ) const;
	virtual bool			IsFullScreen( void ) const;
	virtual int				GetScreenWidth() const;
	virtual int				GetScreenHeight() const;
	virtual anRenderWorld	*AllocRenderWorld( void );
	virtual void			FreeRenderWorld( anRenderWorld *rw );
	virtual void			BeginLevelLoad( void );
	virtual void			EndLevelLoad( void );
	virtual bool			RegisterFont( const char *fontName, fontInfoEx_t &font );
	virtual void			SetColor( const anVec4 &rgba );
	virtual void			SetColor4( float r, float g, float b, float a );
	virtual void			DrawStretchPic( const anDrawVertex *verts, const qglIndex_t *indexes, int vertCount, int indexCount, const anMaterial *material, bool clip = true, float x = 0.0f, float y = 0.0f, float w = 640.0f, float h = 0.0f );
	virtual void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const anMaterial *material );

	virtual void			DrawStretchTri( anVec2 p1, anVec2 p2, anVec2 p3, anVec2 t1, anVec2 t2, anVec2 t3, const anMaterial *material );
	virtual void			GlobalToNormalizedDeviceCoordinates( const anVec3 &global, anVec3 &ndc );
	virtual void			GetGLSettings( int& width, int& height );
	virtual void			PrintMemInfo( MemInfo_t *mi );

	virtual void			DrawSmallChar( int x, int y, int ch, const anMaterial *material );
	virtual void			DrawSmallStringExt( int x, int y, const char *string, const anVec4 &setColor, bool forceColor, const anMaterial *material );
	virtual void			DrawBigChar( int x, int y, int ch, const anMaterial *material );
	virtual void			DrawBigStringExt( int x, int y, const char *string, const anVec4 &setColor, bool forceColor, const anMaterial *material );
	virtual void			BeginFrame( int winWidth, int winHeight );
	virtual void			EndFrame( int *frontEndMsec, int *backEndMsec );
	virtual void			TakeScreenshot( int width, int height, const char *fileName, int downSample, renderView_t *ref );
	virtual void			CropRenderSize( int width, int height, bool makePowerOfTwo = false, bool forceDimensions = false );
	virtual void			CaptureRenderToImage( const char *imageName );
	virtual void			CaptureRenderToFile( const char *fileName, bool fixAlpha );
	virtual void			UnCrop();
	virtual void			GetCardCaps( bool &oldCard, bool &nv10or20 );
	virtual bool			UploadImage( const char *imageName, const byte *data, int width, int height );

public:
	// internal functions
							anRenderSystemLocal( void );
							~anRenderSystemLocal( void );

	void					Clear( void );
	void					SetBackEndRenderer();			// sets tr.backEndRenderer based on cvars
	void					RenderViewToViewport( const renderView_t *renderView, anScreenRect *viewport );

public:
	// renderer globals
	bool					registered;		// cleared at shutdown, set at InitOpenGL

	bool					takeSnapshot;

	int						frameCount;		// incremented every frame
	int						viewCount;		// incremented every view (twice a scene if subviewed)
											// and every R_MarkFragments call

	int						staticAllocCount;	// running total of bytes allocated

	float					frameShaderTime;	// shader time for all non-world 2D rendering

	int						viewportOffset[2];	// for doing larger-than-window tiled renderings
	int						tiledViewport[2];

	// determines which back end to use, and if vertex programs are in use
	backEndName_t			backEndRenderer;
	bool					backEndRendererHasVertexPrograms;
	float					backEndRendererMaxLight;	// 1.0 for standard, unlimited for floats
														// determines how much overbrighting needs
														// to be done post-process

	anVec4					ambientLightVector;	// used for "ambient bump mapping"

	float					sortOffset;				// for determinist sorting of equal sort materials

	anList<anRenderWorldLocal*>worlds;

	anRenderWorldLocal		*primaryWorld;
	renderView_t			primaryRenderView;
	viewDef_t *				primaryView;
	// many console commands need to know which world they should operate on

	const anMaterial		*defaultMTL;
	anImage				*testImg;
	anImage				*ambientCubeImage;	// hack for testing dependent ambient lighting

	viewDef_t				*viewDef;

	performanceCounters_t	pc;					// performance counters
	setBufferCommand_t		lockSurfsCmd;	// use this when r_lockSurfaces = 1

	viewEntity_t			identitySpace;		// can use if we don't know viewDef->worldSpace is valid
	FILE					*logFile;			// for logging GL calls and frame breaks

	int						stencilIncr, stencilDecr;	// GL_INCR / INCR_WRAP_EXT, GL_DECR / GL_DECR_EXT

	renderCrop_t			renderCrops[MAX_RENDER_CROPS];
	int						currentRenderCrop;

	// GUI drawing variables for surface creation
	int						guiRecursionLevel;		// to prevent infinite overruns
	class anInteractiveGuiModel *guiModel;

	GLushort				gammaTable[256];	// brightness / gamma modify this
};

extern backEndState_t		backEnd;
extern anRenderSystemLocal	tr;
extern qglConfig_t			qglConfig;		// outside of TR since it shouldn't be cleared during ref re-init
extern anRenderCache	arcBuffer;
//
// cvars
//
extern anCVarSystem r_ext_vertex_array_range;

extern anCVarSystem r_glDriver;				// "opengl32", etc
extern anCVarSystem r_mode;					// video mode number
extern anCVarSystem r_displayRefresh;			// optional display refresh rate option for vid mode
extern anCVarSystem r_fullscreen;				// 0 = windowed, 1 = full screen
extern anCVarSystem r_multiSamples;			// number of antialiasing samples

extern anCVarSystem r_ignore;					// used for random debugging without defining new vars
extern anCVarSystem r_ignore2;				// used for random debugging without defining new vars
extern anCVarSystem r_znear;					// near Z clip plane

extern anCVarSystem r_finish;					// force a call to glFinish() every frame
extern anCVarSystem r_frontBuffer;			// draw to front buffer for debugging
extern anCVarSystem r_swapInterval;			// changes wglSwapIntarval
extern anCVarSystem r_offsetFactor;			// polygon offset parameter
extern anCVarSystem r_offsetUnits;			// polygon offset parameter
extern anCVarSystem r_singleTriangle;			// only draw a single triangle per primitive
extern anCVarSystem r_logFile;				// number of frames to emit GL logs
extern anCVarSystem r_clear;					// force screen clear every frame
extern anCVarSystem r_shadows;				// enable shadows
extern anCVarSystem r_subviewOnly;			// 1 = don't render main view, allowing subviews to be debugged
extern anCVarSystem r_lightScale;				// all light intensities are multiplied by this, which is normally 2
extern anCVarSystem r_flareSize;				// scale the flare deforms from the material def

extern anCVarSystem r_gamma;					// changes gamma tables
extern anCVarSystem r_brightness;				// changes gamma tables

extern anCVarSystem r_renderer;				// arb, nv10, nv20, r200, gl2, etc

extern anCVarSystem r_cgVertexProfile;		// arbvp1, vp20, vp30
extern anCVarSystem r_cgFragmentProfile;		// arbfp1, fp30

extern anCVarSystem r_checkBounds;			// compare all surface bounds with precalculated ones

extern anCVarSystem r_useNV20MonoLights;		// 1 = allow an interaction pass optimization
extern anCVarSystem r_useLightPortalFlow;	// 1 = do a more precise area reference determination
extern anCVarSystem r_useTripleTextureARB;	// 1 = cards with 3+ texture units do a two pass instead of three pass
extern anCVarSystem r_useShadowSurfaceScissor;// 1 = scissor shadows by the scissor rect of the interaction surfaces
extern anCVarSystem r_useConstantMaterials;	// 1 = use pre-calculated material registers if possible
extern anCVarSystem r_useInteractionTable;	// create a full entityDefs * lightDefs table to make finding interactions faster
extern anCVarSystem r_useNodeCommonChildren;	// stop pushing reference bounds early when possible
extern anCVarSystem r_useSilRemap;			// 1 = consider verts with the same XYZ, but different ST the same for shadows
extern anCVarSystem r_useCulling;			// 0 = none, 1 = sphere, 2 = sphere + box
extern anCVarSystem r_useLightCulling;		// 0 = none, 1 = box, 2 = exact clip of polyhedron faces
extern anCVarSystem r_useLightScissors;		// 1 = use custom scissor rectangle for each light
extern anCVarSystem r_useClippedLightScissors;// 0 = full screen when near clipped, 1 = exact when near clipped, 2 = exact always
extern anCVarSystem r_useEntityCulling;		// 0 = none, 1 = box
extern anCVarSystem r_useEntityScissors;		// 1 = use custom scissor rectangle for each entity
extern anCVarSystem r_useInteractionCulling;	// 1 = cull interactions
extern anCVarSystem r_useInteractionScissors;	// 1 = use a custom scissor rectangle for each interaction
extern anCVarSystem r_useFrustumFarDistance;	// if != 0 force the view frustum far distance to this distance
extern anCVarSystem r_useShadowCulling;		// try to cull shadows from partially visible lights
extern anCVarSystem r_usePreciseTriangleInteractions;	// 1 = do winding clipping to determine if each ambiguous tri should be lit
extern anCVarSystem r_useTurboShadow;			// 1 = use the infinite projection with W technique for dynamic shadows
extern anCVarSystem r_useExternalShadows;		// 1 = skip drawing caps when outside the light volume
extern anCVarSystem r_useOptimizedShadows;	// 1 = use the dmap generated static shadow volumes
extern anCVarSystem r_useShadowVertexProgram;	// 1 = do the shadow projection in the vertex program on capable cards
extern anCVarSystem r_useShadowProjectedCull;	// 1 = discard triangles outside light volume before shadowing
extern anCVarSystem r_useDeferredTangents;	// 1 = don't always calc tangents after deform
extern anCVarSystem r_useCachedDynamicModels;	// 1 = cache snapshots of dynamic models
extern anCVarSystem r_useTwoSidedStencil;		// 1 = do stencil shadows in one pass with different ops on each side
extern anCVarSystem r_useInfiniteFarZ;		// 1 = use the no-far-clip-plane trick
extern anCVarSystem r_useScissor;				// 1 = scissor clip as portals and lights are processed
extern anCVarSystem r_usePortals;				// 1 = use portals to perform area culling, otherwise draw everything
extern anCVarSystem r_useStateCaching;		// avoid redundant state changes in GL_*() calls
extern anCVarSystem r_useCombinerDisplayLists;// if 1, put all nvidia register combiner programming in display lists
extern anCVarSystem r_useVertexBuffers;		// if 0, don't use ARB_vertex_buffer_object for vertexes
extern anCVarSystem r_useIndexBuffers;		// if 0, don't use ARB_vertex_buffer_object for indexes
extern anCVarSystem r_useEntityCallbacks;		// if 0, issue the callback immediately at update time, rather than defering
extern anCVarSystem r_lightAllBackFaces;		// light all the back faces, even when they would be shadowed
extern anCVarSystem r_useDepthBoundsTest;     // use depth bounds test to reduce shadow fill

extern anCVarSystem r_skipPostProcess;		// skip all post-process renderings
extern anCVarSystem r_skipSuppress;			// ignore the per-view suppressions
extern anCVarSystem r_skipInteractions;		// skip all light/surface interaction drawing
extern anCVarSystem r_skipFrontEnd;			// bypasses all front end work, but 2D gui rendering still draws
extern anCVarSystem r_skipBackEnd;			// don't draw anything
extern anCVarSystem r_skipCopyTexture;		// do all rendering, but don't actually copyTexSubImage2D
extern anCVarSystem r_skipRender;				// skip 3D rendering, but pass 2D
extern anCVarSystem r_skipRenderContext;		// nullptr the rendering context during backend 3D rendering
extern anCVarSystem r_skipTranslucent;		// skip the translucent interaction rendering
extern anCVarSystem r_skipAmbient;			// bypasses all non-interaction drawing
extern anCVarSystem r_skipNewAmbient;			// bypasses all vertex/fragment program ambients
extern anCVarSystem r_skipBlendLights;		// skip all blend lights
extern anCVarSystem r_skipFogLights;			// skip all fog lights
extern anCVarSystem r_skipSubviews;			// 1 = don't render any mirrors / cameras / etc
extern anCVarSystem r_skipGuiShaders;			// 1 = don't render any gui elements on surfaces
extern anCVarSystem r_skipParticles;			// 1 = don't render any particles
extern anCVarSystem r_skipUpdates;			// 1 = don't accept any entity or light updates, making everything static
extern anCVarSystem r_skipDeforms;			// leave all deform materials in their original state
extern anCVarSystem r_skipDynamicTextures;	// don't dynamically create textures
extern anCVarSystem r_skipLightScale;			// don't do any post-interaction light scaling, makes things dim on low-dynamic range cards
extern anCVarSystem r_skipBump;				// uses a flat surface instead of the bump map
extern anCVarSystem r_skipSpecular;			// use black for specular
extern anCVarSystem r_skipDiffuse;			// use black for diffuse
extern anCVarSystem r_skipOverlays;			// skip overlay surfaces

extern anCVarSystem r_ignoreGLErrors;

extern anCVarSystem r_forceLoadImages;		// draw all images to screen after registration
extern anCVarSystem r_demonstrateBug;			// used during development to show IHV's their problems
extern anCVarSystem r_screenFraction;			// for testing fill rate, the resolution of the entire screen can be changed

extern anCVarSystem r_showUnsmoothedTangents;	// highlight geometry rendered with unsmoothed tangents
extern anCVarSystem r_showSilhouette;			// highlight edges that are casting shadow planes
extern anCVarSystem r_showVertexColor;		// draws all triangles with the solid vertex color
extern anCVarSystem r_showUpdates;			// report entity and light updates and ref counts
extern anCVarSystem r_showDynamic;			// report stats on dynamic surface generation
extern anCVarSystem r_showLightScale;			// report the scale factor applied to drawing for overbrights
extern anCVarSystem r_showIntensity;			// draw the screen colors based on intensity, red = 0, green = 128, blue = 255
extern anCVarSystem r_showDefs;				// report the number of modeDefs and lightDefs in view
extern anCVarSystem r_showTrace;				// show the intersection of an eye trace with the world
extern anCVarSystem r_showSmp;				// show which end (front or back) is blocking
extern anCVarSystem r_showDepth;				// display the contents of the depth buffer and the depth range
extern anCVarSystem r_showImages;				// draw all images to screen instead of rendering
extern anCVarSystem r_showTris;				// enables wireframe rendering of the world
extern anCVarSystem r_showSurfaceInfo;		// show surface material name under crosshair
extern anCVarSystem r_showNormals;			// draws wireframe normals
extern anCVarSystem r_showEdges;				// draw the sil edges
extern anCVarSystem r_showViewEntitys;		// displays the bounding boxes of all view models and optionally the index
extern anCVarSystem r_showTexturePolarity;	// shade triangles by texture area polarity
extern anCVarSystem r_showTangentSpace;		// shade triangles by tangent space
extern anCVarSystem r_showDominantTri;		// draw lines from vertexes to center of dominant triangles
extern anCVarSystem r_showTextureVectors;		// draw each triangles texture (tangent) vectors
extern anCVarSystem r_showLights;				// 1 = print light info, 2 = also draw volumes
extern anCVarSystem r_showLightCount;			// colors surfaces based on light count
extern anCVarSystem r_showShadows;			// visualize the stencil shadow volumes
extern anCVarSystem r_showShadowCount;		// colors screen based on shadow volume depth complexity
extern anCVarSystem r_showLightScissors;		// show light scissor rectangles
extern anCVarSystem r_showEntityScissors;		// show entity scissor rectangles
extern anCVarSystem r_showInteractionFrustums;// show a frustum for each interaction
extern anCVarSystem r_showInteractionScissors;// show screen rectangle which contains the interaction frustum
extern anCVarSystem r_showMemory;				// print frame memory utilization
extern anCVarSystem r_showCull;				// report sphere and box culling stats
extern anCVarSystem r_showInteractions;		// report interaction generation activity
extern anCVarSystem r_showSurfaces;			// report surface/light/shadow counts
extern anCVarSystem r_showPrimitives;			// report vertex/index/draw counts
extern anCVarSystem r_showPortals;			// draw portal outlines in color based on passed / not passed
extern anCVarSystem r_showAlloc;				// report alloc/free counts
extern anCVarSystem r_showSkel;				// draw the skeleton when model animates
extern anCVarSystem r_showOverDraw;			// show overdraw
extern anCVarSystem r_jointNameScale;			// size of joint names when r_showskel is set to 1
extern anCVarSystem r_jointNameOffset;		// offset of joint names when r_showskel is set to 1

extern anCVarSystem r_testGamma;				// draw a grid pattern to test gamma levels
extern anCVarSystem r_testStepGamma;			// draw a grid pattern to test gamma levels
extern anCVarSystem r_testGammaBias;			// draw a grid pattern to test gamma levels

extern anCVarSystem r_testARBProgram;			// experiment with vertex/fragment programs

extern anCVarSystem r_singleLight;			// suppress all but one light
extern anCVarSystem r_singleEntity;			// suppress all but one entity
extern anCVarSystem r_singleArea;				// only draw the portal area the view is actually in
extern anCVarSystem r_singleSurface;			// suppress all but one surface on each entity
extern anCVarSystem r_shadowPolygonOffset;	// bias value added to depth test for stencil shadow drawing
extern anCVarSystem r_shadowPolygonFactor;	// scale value for stencil shadow drawing

extern anCVarSystem r_jitter;				// randomly subpixel jitter the projection matrix
extern anCVarSystem r_lightSourceRadius;		// for soft-shadow sampling
extern anCVarSystem r_lockSurfaces;
extern anCVarSystem r_orderIndexes;			// perform index reorganization to optimize vertex use

extern anCVarSystem r_debugLineDepthTest;	// perform depth test on debug lines
extern anCVarSystem r_debugLineWidth;		// width of debug lines
extern anCVarSystem r_debugArrowStep;		// step size of arrow cone line rotation in degrees
extern anCVarSystem r_debugPolygonFilled;

extern anCVarSystem r_materialOverride;		// override all materials

extern anCVarSystem r_debugRenderToTexture;

/*
====================================================================

GL wrapper/helper functions

====================================================================
*/

void	GL_SetCurrentTextureUnit( int unit );
void	GL_CheckErrors( void );
void	GL_ClearStateDelta( void );
void	GL_State( int stateVector );
void	GL_TexEnv( int env );
void	GL_Cull( int cullType );

const int GLS_SRCBLEND_ZERO						= 0x00000001;
const int GLS_SRCBLEND_ONE						= 0x0;
const int GLS_SRCBLEND_DST_COLOR				= 0x00000003;
const int GLS_SRCBLEND_ONE_MINUS_DST_COLOR		= 0x00000004;
const int GLS_SRCBLEND_SRC_ALPHA				= 0x00000005;
const int GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		= 0x00000006;
const int GLS_SRCBLEND_DST_ALPHA				= 0x00000007;
const int GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		= 0x00000008;
const int GLS_SRCBLEND_ALPHA_SATURATE			= 0x00000009;
const int GLS_SRCBLEND_BITS						= 0x0000000f;

const int GLS_DSTBLEND_ZERO						= 0x0;
const int GLS_DSTBLEND_ONE						= 0x00000020;
const int GLS_DSTBLEND_SRC_COLOR				= 0x00000030;
const int GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		= 0x00000040;
const int GLS_DSTBLEND_SRC_ALPHA				= 0x00000050;
const int GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		= 0x00000060;
const int GLS_DSTBLEND_DST_ALPHA				= 0x00000070;
const int GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		= 0x00000080;
const int GLS_DSTBLEND_BITS						= 0x000000f0;


// these masks are the inverse, meaning when set the glColorMask value will be 0,
// preventing that channel from being written
const int GLS_DEPTHMASK							= 0x00000100;
const int GLS_REDMASK							= 0x00000200;
const int GLS_GREENMASK							= 0x00000400;
const int GLS_BLUEMASK							= 0x00000800;
const int GLS_ALPHAMASK							= 0x00001000;
const int GLS_COLORMASK							= ( GLS_REDMASK|GLS_GREENMASK|GLS_BLUEMASK);

const int GLS_POLYMODE_LINE						= 0x00002000;

const int GLS_DEPTHFUNC_ALWAYS					= 0x00010000;
const int GLS_DEPTHFUNC_EQUAL					= 0x00020000;
const int GLS_DEPTHFUNC_LESS					= 0x0;

const int GLS_ATEST_EQ_255						= 0x10000000;
const int GLS_ATEST_LT_128						= 0x20000000;
const int GLS_ATEST_GE_128						= 0x40000000;
const int GLS_ATEST_BITS						= 0x70000000;

const int GLS_DEFAULT							= GLS_DEPTHFUNC_ALWAYS;

void R_Init( void );
void R_InitOpenGL( void );

void R_DoneFreeType( void );

void R_SetColorMappings( void );

void R_ScreenShot_f( const anCommandArgs &args );
void R_StencilShot( void );

bool R_CheckExtension( char *name );

/*
====================================================================

MAIN

====================================================================
*/

// moved to GraphicsBuffer class type. temporarily shared with VertexCache.h/VertexCache.cpp possibly will be named RenderCache.h
//void R_RenderView( viewDef_t *parms );

// performs radius cull first, then corner cull
bool R_CullLocalBox( const anBounds &bounds, const float modelMatrix[16], int numPlanes, const anPlane *planes );
bool R_RadiusCullLocalBox( const anBounds &bounds, const float modelMatrix[16], int numPlanes, const anPlane *planes );
bool R_CornerCullLocalBox( const anBounds &bounds, const float modelMatrix[16], int numPlanes, const anPlane *planes );

void R_GlobalToNormalizedDeviceCoordinates( const anVec3 &global, anVec3 &ndc );

void R_SetViewMatrix( viewDef_t *viewDef );

/*
============================================================

LIGHT

============================================================
*/

void R_ListRenderLightDefs_f( const anCommandArgs &args );
void R_ListRenderEntityDefs_f( const anCommandArgs &args );

bool R_IssueEntityDefCallback( anRenderEntityLocal *def );
anRenderModel *R_EntityDefDynamicModel( anRenderEntityLocal *def );

viewEntity_t *R_SetEntityDefViewEntity( anRenderEntityLocal *def );
viewLight_t *R_SetLightDefViewLight( anRenderLightsLocal *def );

void R_AddDrawSurf( const srfTriangles_t *tri, const viewEntity_t *space, const renderEntity_t *renderEntity, const anMaterial *shader, const anScreenRect &scissor );

void R_LinkLightSurf( const drawSurf_t **link, const srfTriangles_t *tri, const viewEntity_t *space, const anRenderLightsLocal *light, const anMaterial *shader, const anScreenRect &scissor, bool viewInsideShadow );

bool R_CreateAmbientCache( srfTriangles_t *tri, bool needsLighting );
bool R_CreateLightingCache( const anRenderEntityLocal *ent, const anRenderLightsLocal *light, srfTriangles_t *tri );
void R_CreatePrivateShadowCache( srfTriangles_t *tri );
void R_CreateVertexProgramShadowCache( srfTriangles_t *tri );

/*
============================================================

LIGHTRUN

============================================================
*/

void R_RegenerateWorld_f( const anCommandArgs &args );

void R_ModulateLights_f( const anCommandArgs &args );
void R_SetLightProject( anPlane lightProject[4], const anVec3 origin, const anVec3 targetPoint, const anVec3 rightVector, const anVec3 upVector, const anVec3 start, const anVec3 stop );

void R_AddLightSurfaces( void );
void R_AddModelSurfaces( void );
void R_RemoveUnecessaryViewLights( void );

void R_FreeDerivedData( void );
void R_ReCreateWorldReferences( void );

void R_CreateEntityRefs( anRenderEntityLocal *def );
void R_CreateLightRefs( anRenderLightsLocal *light );

void R_DeriveLightData( anRenderLightsLocal *light );
void R_FreeLightDefDerivedData( anRenderLightsLocal *light );
void R_CheckForEntityDefsUsingModel( anRenderModel *model );

void R_ClearEntityDefDynamicModel( anRenderEntityLocal *def );
void R_FreeEntityDefDerivedData( anRenderEntityLocal *def, bool keepDecals, bool keepCachedDynamicModel );
void R_FreeEntityDefCachedDynamicModel( anRenderEntityLocal *def );
void R_FreeEntityDefDecals( anRenderEntityLocal *def );
void R_FreeEntityDefOverlay( anRenderEntityLocal *def );
void R_FreeEntityDefFadedDecals( anRenderEntityLocal *def, int time );

void R_CreateLightDefFogPortals( anRenderLightsLocal *ldef );

/*
============================================================

RENDER

============================================================
*/

void RB_EnterViewDepthHack();
void RB_EnterModelDepthHack( float depth );
void RB_LeaveDepthHack();
void RB_DrawElementsImmediate( const srfTriangles_t *tri );
void RB_RenderTriangleSurface( const srfTriangles_t *tri );
void RB_T_RenderTriangleSurface( const drawSurf_t *surf );
void RB_RenderDrawSurfListWithFunction( drawSurf_t **drawSurfs, int numDrawSurfs, void (*triFunc_)( const drawSurf_t *) );
void RB_RenderDrawSurfChainWithFunction( const drawSurf_t *drawSurfs, void (*triFunc_)( const drawSurf_t *) );
void RB_DrawShaderPasses( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_LoadShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture );
void RB_GetShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture, float matrix[16] );
void RB_CreateSingleDrawInteractions( const drawSurf_t *surf, void (*DrawInteraction)(const drawInteraction_t *) );

const materialStage_t *RB_SetLightTexture( const anRenderLightsLocal *light );

void RB_DrawView( const void *data );

void RB_DetermineLightScale( void );
void RB_STD_LightScale( void );
void RB_BeginDrawingView( void );

/*
============================================================

DRAW_STANDARD

============================================================
*/

void RB_DrawElementsWithCounters( const srfTriangles_t *tri );
void RB_DrawShadowElementsWithCounters( const srfTriangles_t *tri, int numIndexes );
void RB_STD_FillDepthBuffer( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_BindVariableStageImage( const textureStage_t *texture, const float *shaderRegisters );
void RB_BindStageTexture( const float *shaderRegisters, const textureStage_t *texture, const drawSurf_t *surf );
void RB_StencilShadowPass( const drawSurf_t *drawSurfs );
void RB_STD_DrawView( void );
void RB_STD_FogAllLights( void );
void RB_BakeTextureMatrixIntoTexgen( anPlane lightProject[3], const float textureMatrix[16] );

/*
============================================================

DRAW_*

============================================================
*/

void	RB_ARB_DrawInteractions( void );

void	R_R200_Init( void );
void	RB_R200_DrawInteractions( void );

void	R_NV10_Init( void );
void	RB_NV10_DrawInteractions( void );

void	R_NV20_Init( void );
void	RB_NV20_DrawInteractions( void );

void	R_ARB2_Init( void );
void	RB_ARB2_DrawInteractions( void );
void	R_ReloadARBPrograms_f( const anCommandArgs &args );
int		R_FindARBProgram( GLenum target, const char *program );

typedef enum {
	PROG_INVALID,
	VPROG_INTERACTION,
	VPROG_ENVIRONMENT,
	VPROG_BUMPY_ENVIRONMENT,
	VPROG_R200_INTERACTION,
	VPROG_STENCIL_SHADOW,
	VPROG_NV20_BUMP_AND_LIGHT,
	VPROG_NV20_DIFFUSE_COLOR,
	VPROG_NV20_SPECULAR_COLOR,
	VPROG_NV20_DIFFUSE_AND_SPECULAR_COLOR,
	VPROG_TEST,
	FPROG_INTERACTION,
	FPROG_ENVIRONMENT,
	FPROG_BUMPY_ENVIRONMENT,
	FPROG_TEST,
	VPROG_AMBIENT,
	FPROG_AMBIENT,
	VPROG_GLASSWARP,
	FPROG_GLASSWARP,
	PROG_USER
} program_t;

/*

  All vertex programs use the same constant register layout:

c[4]	localLightOrigin
c[5]	localViewOrigin
c[6]	lightProjection S
c[7]	lightProjection T
c[8]	lightProjection Q
c[9]	lightFalloff	S
c[10]	bumpMatrix S
c[11]	bumpMatrix T
c[12]	diffuseMatrix S
c[13]	diffuseMatrix T
c[14]	specularMatrix S
c[15]	specularMatrix T


c[20]	light falloff tq constant

// texture 0 was cube map
// texture 1 will be the per-surface bump map
// texture 2 will be the light falloff texture
// texture 3 will be the light projection texture
// texture 4 is the per-surface diffuse map
// texture 5 is the per-surface specular map
// texture 6 is the specular half angle cube map

*/

typedef enum {
	PP_LIGHT_ORIGIN = 4,
	PP_VIEW_ORIGIN,
	PP_LIGHT_PROJECT_S,
	PP_LIGHT_PROJECT_T,
	PP_LIGHT_PROJECT_Q,
	PP_LIGHT_FALLOFF_S,
	PP_BUMP_MATRIX_S,
	PP_BUMP_MATRIX_T,
	PP_DIFFUSE_MATRIX_S,
	PP_DIFFUSE_MATRIX_T,
	PP_SPECULAR_MATRIX_S,
	PP_SPECULAR_MATRIX_T,
	PP_COLOR_MODULATE,
	PP_COLOR_ADD,

	PP_LIGHT_FALLOFF_TQ = 20	// only for NV programs
} programParameter_t;


/*
============================================================

TR_STENCILSHADOWS moved to tr_shadowbounds.cpp

"facing" should have one more element than tri->numIndexes / 3, which should be set to 1

============================================================
*/

void R_MakeShadowFrustums( anRenderLightsLocal *def );

typedef enum {
	SG_DYNAMIC,		// use infinite projections
	SG_STATIC,		// clip to bounds
	SG_OFFLINE		// perform very time consuming optimizations
} shadowGen_t;

srfTriangles_t *R_CreateShadowVolume( const anRenderEntityLocal *ent, const srfTriangles_t *tri, const anRenderLightsLocal *light, shadowGen_t optimize, srfCullInfo_t &cullInfo );

/*
============================================================

TR_TURBOSHADOW

Fast, non-clipped overshoot shadow volumes

"facing" should have one more element than tri->numIndexes / 3, which should be set to 1
calling this function may modify "facing" based on culling

============================================================
*/

srfTriangles_t		*R_CreateVertexProgramTurboShadowVolume( const anRenderEntityLocal *ent, const srfTriangles_t *tri, const anRenderLightsLocal *light, srfCullInfo_t &cullInfo );
srfTriangles_t		*R_CreateTurboShadowVolume( const anRenderEntityLocal *ent, const srfTriangles_t *tri, const anRenderLightsLocal *light, srfCullInfo_t &cullInfo );

/*
============================================================

util/shadowopt3

dmap time optimization of shadow volumes, called from R_CreateShadowVolume

============================================================
*/

typedef struct {
	anVec3			*verts;			// includes both front and back projections, caller should free
	int				numVerts;
	qglIndex_t		*indexes;	// caller should free

	// indexes must be sorted frontCap, rearCap, silPlanes so the caps can be removed
	// when the viewer is in a position that they don't need to see them
	int				numFrontCapIndexes;
	int				numRearCapIndexes;
	int				numSilPlaneIndexes;
	int				totalIndexes;
} optimizedShadow_t;

optimizedShadow_t	SuperOptimizeOccluders( anVec4 *verts, qglIndex_t *indexes, int numIndexes, anPlane projectionPlane, anVec3 projectionOrigin );

void				CleanupOptimizedShadowTris( srfTriangles_t *tri );

/*
============================================================

TRISURF

============================================================
*/

#define USE_TRI_DATA_ALLOCATOR

void				R_InitTriSurfData( void );
void				R_ShutdownTriSurfData( void );
void				R_PurgeTriSurfData( frameData_t *frame );
void				R_ShowTriSurfMemory_f( const anCommandArgs &args );

srfTriangles_t *	R_AllocStaticTriSurf( void );
srfTriangles_t *	R_CopyStaticTriSurf( const srfTriangles_t *tri );
void				R_AllocStaticTriSurfVerts( srfTriangles_t *tri, int numVerts );
void				R_AllocStaticTriSurfIndexes( srfTriangles_t *tri, int numIndexes );
void				R_AllocStaticTriSurfShadowVerts( srfTriangles_t *tri, int numVerts );
void				R_AllocStaticTriSurfPlanes( srfTriangles_t *tri, int numIndexes );
void				R_ResizeStaticTriSurfVerts( srfTriangles_t *tri, int numVerts );
void				R_ResizeStaticTriSurfIndexes( srfTriangles_t *tri, int numIndexes );
void				R_ResizeStaticTriSurfShadowVerts( srfTriangles_t *tri, int numVerts );
void				R_ReferenceStaticTriSurfVerts( srfTriangles_t *tri, const srfTriangles_t *reference );
void				R_ReferenceStaticTriSurfIndexes( srfTriangles_t *tri, const srfTriangles_t *reference );
void				R_FreeStaticTriSurfSilIndexes( srfTriangles_t *tri );
void				R_FreeStaticTriSurf( srfTriangles_t *tri );
void				R_FreeStaticTriSurfVertexCaches( srfTriangles_t *tri );
void				R_ReallyFreeStaticTriSurf( srfTriangles_t *tri );
void				R_FreeDeferredTriSurfs( frameData_t *frame );
int					R_TriSurfMemory( const srfTriangles_t *tri );

void				R_BoundTriSurf( srfTriangles_t *tri );
void				R_RemoveDuplicatedTriangles( srfTriangles_t *tri );
void				R_CreateSilIndexes( srfTriangles_t *tri );
void				R_RemoveDegenerateTriangles( srfTriangles_t *tri );
void				R_RemoveUnusedVerts( srfTriangles_t *tri );
void				R_RangeCheckIndexes( const srfTriangles_t *tri );
void				R_CreateVertexNormals( srfTriangles_t *tri );	// also called by dmap
void				R_DeriveFacePlanes( srfTriangles_t *tri );		// also called by renderbump
void				R_CleanupTriangles( srfTriangles_t *tri, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents );
void				R_ReverseTriangles( srfTriangles_t *tri );

static void			R_ByteSwapTri( srfTriangles_t *tri );

// Only deals with vertexes and indexes, not silhouettes, planes, etc.
// Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
srfTriangles_t *	R_MergeSurfaceList( const srfTriangles_t **surfaces, int numSurfaces );
srfTriangles_t *	R_MergeTriangles( const srfTriangles_t *tri1, const srfTriangles_t *tri2 );

// if the deformed verts have significant enough texture coordinate changes to reverse the texture
// polarity of a triangle, the tangents will be incorrect
void				R_DeriveTangents( srfTriangles_t *tri, bool allocFacePlanes = true );

// deformable meshes precalculate as much as possible from a base frame, then generate
// complete srfTriangles_t from just a new set of vertexes
typedef struct deformInfo_s {
	int				numSourceVerts;

	// numOutputVerts may be smaller if the input had duplicated or degenerate triangles
	// it will often be larger if the input had mirrored texture seams that needed
	// to be busted for proper tangent spaces
	int				numOutputVerts;

	int				numMirroredVerts;
	int *			mirroredVerts;

	int				numIndexes;
	qglIndex_t *		indexes;

	qglIndex_t *		silIndexes;

	int				numDupVerts;
	int *			dupVerts;

	int				numSilEdges;
	silEdge_t *		silEdges;

	dominantTri_t *	dominantTris;
} deformInfo_t;


deformInfo_t *		R_BuildDeformInfo( int numVerts, const anDrawVertex *verts, int numIndexes, const int *indexes, bool useUnsmoothedTangents );
void				R_FreeDeformInfo( deformInfo_t *deformInfo );
int					R_DeformInfoMemoryUsed( deformInfo_t *deformInfo );

/*
============================================================

SUBVIEW

============================================================
*/

bool				R_PreciseCullSurface( const drawSurf_t *drawSurf, anBounds &ndcBounds );
bool				R_GenerateSubViews( void );

/*
============================================================

SCENE GENERATION

moved into class anRenderCache and in vertexcache.h temporarily. Most likely will be Vertex/RenderCache.h the name may change when its created
just future note and heads up
============================================================
*/

/*void				R_InitFrameData( void );
void				R_ShutdownFrameData( void );
int 				R_CountFrameData( void );
void 				R_ToggleSmpFrame( void );
void				*R_FrameAlloc( int bytes );
void 				*R_ClearedFrameAlloc( int bytes );
void				R_FrameFree( void *data );

void 				*R_StaticAlloc( int bytes );		// just malloc with error checking
void 				*R_ClearedStaticAlloc( int bytes );	// with memset
void 				R_StaticFree( void *data );*/


/*
=============================================================

RENDERER DEBUG TOOLS

=============================================================
*/

float RB_DrawTextLength( const char *text, float scale, int len );
void RB_AddDebugText( const char *text, const anVec3 &origin, float scale, const anVec4 &color, const anMat3 &viewAxis, const int align, const int lifetime, const bool depthTest );
void RB_ClearDebugText( int time );
void RB_AddDebugLine( const anVec4 &color, const anVec3 &start, const anVec3 &end, const int lifeTime, const bool depthTest );
void RB_ClearDebugLines( int time );
void RB_AddDebugPolygon( const anVec4 &color, const anWinding &winding, const int lifeTime, const bool depthTest );
void RB_ClearDebugPolygons( int time );
void RB_DrawBounds( const anBounds &bounds );
void RB_ShowLights( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_ShowLightCount( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_PolygonClear( void );
void RB_ScanStencilBuffer( void );
void RB_ShowDestinationAlpha( void );
void RB_ShowOverdraw( void );
void RB_RenderDebugTools( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_ShutdownDebugTools( void );

/*
=============================================================

TR_BACKEND

=============================================================
*/

void RB_SetDefaultGLState( void );
void RB_SetGL2D( void );

// write a comment to the r_logFile if it is enabled
void RB_LogComment( const char *comment, ... ) an_attribute((format(printf,1,2) ) );

void RB_ShowImages( void );

void RB_ExecuteBackEndCommands( const setBufferCommand_t *cmds );


/*
=============================================================

TR_GUISURF

=============================================================
*/

void R_SurfaceToTextureAxis( const srfTriangles_t *tri, anVec3 &origin, anVec3 axis[3] );
void R_RenderGuiSurf( anUserInterfaces *gui, drawSurf_t *drawSurf );

void R_OrderIndexes( int numIndexes, qglIndex_t *indexes );

void R_DeformDrawSurf( drawSurf_t *drawSurf );

srfTriangles_t *R_PolytopeSurface( int numPlanes, const anPlane *planes, anWinding **windings );

typedef struct {
	float		fraction;
	// only valid if fraction < 1.0
	anVec3		point;
	anVec3		normal;
	int			indexes[3];
} localTrace_t;

localTrace_t R_LocalTrace( const anVec3 &start, const anVec3 &end, const float radius, const srfTriangles_t *tri );
void RB_ShowTrace( drawSurf_t **drawSurfs, int numDrawSurfs );

anScreenRect R_CalcIntersectionScissor( const anRenderLightsLocal * lightDef, const anRenderEntityLocal * entityDef, const viewDef_t * viewDef );

#include "RenderWorld_local.h"
#include "GuiModel.h"
#include "VertexCache.h"

#endif

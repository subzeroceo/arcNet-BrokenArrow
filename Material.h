#ifndef __MATERIAL_H__
#define __MATERIAL_H__

/*
===============================================================================

        Material

===============================================================================
*/

class anImage;
class idCinematic;
class anUserInterfaces;
class idMegaTexture;
class anAudioSystem;

#include "/home/subzeroceo/ArC-NetSoftware-Projects/brokenarrow/renderer/GLIncludes/qgl.h"
#include "/home/subzeroceo/ArC-NetSoftware-Projects/brokenarrow/renderer/GLIncludes/qgl_linked.h"

#include "Model_local.h"

// moved from image.h for default parm
typedef enum {
  TF_LINEAR,
  TF_NEAREST,
  TF_LINEARNEAREST,
  TF_DEFAULT // use the user-specified r_textureFilter
} textureFilter_t;

typedef enum {
    TR_REPEAT,
	TR_MIRRORED_REPEAT,
    TR_CLAMP,
    TR_CLAMP_TO_BORDER, // this should replace TR_CLAMP_TO_ZERO and
                        // TR_CLAMP_TO_ZERO_ALPHA, but I don't want to risk
                        // changing it right now
    TR_CLAMP_TO_ZERO, // guarantee 0,0,0,255 edge for projected textures,
    // set AFTER image format selection
    TR_CLAMP_TO_ZERO_ALPHA, // guarantee 0 alpha edge for projected textures,
                           // set AFTER image format selection
    TR_CLAMP_X,
    TR_CLAMP_Y,
    TR_MIRROR,
    TR_MIRROR_X,
    TR_MIRROR_Y
} textureRepeat_t;

typedef struct {
  int stayTime;   // msec for no change
  int fadeTime;   // msec to fade vertex colors over
  float start[4]; // vertex color at spawn (possibly out of 0.0 - 1.0 range,
                  // will clamp after calc)
  float end[4];   // vertex color at fade-out (possibly out of 0.0 - 1.0 range,
                  // will clamp after calc)
} decalInfo_t;

typedef enum {
    DFRM_NONE,
    DFRM_SPRITE,
    DFRM_TUBE,
    DFRM_FLARE,
    DFRM_FLARE_2,
    DFRM_EXPAND,
    DFRM_MOVE,
    DFRM_EYEBALL,
    DFRM_PARTICLE,
    DFRM_PARTICLE2,
    DFRM_TURB,
    DFRM_GLOW,
	DFRM_CORONA,
	DFRM_RECTSPRITE
} deformSurf_t;

typedef enum {
    DI_STATIC,
    DI_SCRATCH, // video, screen wipe, etc
    DI_CUBE_RENDER,
    DI_MIRROR_RENDER,
    DI_XRAY_RENDER,
    DI_REFLECTION_RENDER,
    DI_REFRACTION_RENDER,
    DI_REMOTE_RENDER
} dynamicImage_t;

// note: keep opNames[] in sync with changes
typedef enum {
    OP_TYPE_ADD,
    OP_TYPE_SUBTRACT,
    OP_TYPE_MULTIPLY,
    OP_TYPE_DIVIDE,
    OP_TYPE_MOD,
    OP_TYPE_TABLE,
    OP_TYPE_GT,
    OP_TYPE_GE,
    OP_TYPE_LT,
    OP_TYPE_LE,
    OP_TYPE_EQ,
    OP_TYPE_NE,
    OP_TYPE_AND,
    OP_TYPE_OR,
    OP_TYPE_SOUND,
    OP_TYPE_LOAD
} expOpType_t;

typedef enum {
    EXP_REG_TIME,

    EXP_REG_PARM0,
    EXP_REG_PARM1,
    EXP_REG_PARM2,
    EXP_REG_PARM3,
    EXP_REG_PARM4,
    EXP_REG_PARM5,
    EXP_REG_PARM6,
    EXP_REG_PARM7,
    EXP_REG_PARM8,
    EXP_REG_PARM9,
    EXP_REG_PARM10,
    EXP_REG_PARM11,

    EXP_REG_GLOBAL0,
    EXP_REG_GLOBAL1,
    EXP_REG_GLOBAL2,
    EXP_REG_GLOBAL3,
    EXP_REG_GLOBAL4,
    EXP_REG_GLOBAL5,
    EXP_REG_GLOBAL6,
    EXP_REG_GLOBAL7,
	EXP_REG_VERTEX_RANDOMIZER,
    EXP_REG_NUMLIGHTS,
    EXP_REG_NUM_PREDEFINED
} expRegister_t;

typedef struct {
    expOpType_t 	opType;
    int				a, b, c;
} expOp_t;

typedef struct {
    int				registers[4];
} colorStage_t;

typedef enum {
  TG_EXPLICIT,
  TG_DIFFUSE_CUBE,
  TG_REFLECT_CUBE,
  TG_SKYBOX_CUBE,
  TG_WOBBLESKY_CUBE,
  TG_SCREEN, // screen aligned, for mirrorRenders and screen space temporaries
  TG_SCREEN2,
  TG_GLASSWARP
} texGen_t;

typedef struct {
    idCinematic *	cinematic;
    anImage *		image;
    texGen_t 		texgen;
    //effectsVertexProgram_t	program;
    bool 			hasMatrix;
    int 			matrix[2][3]; // we only allow a subset of the full projection matrix

    // dynamic image variables
    dynamicImage_t	dynamic;
    int				width, height;
    int				dynamicFrameCount;
} textureStage_t;

// the order BUMP / DIFFUSE / SPECULAR is necessary for interactions to draw
// correctly on low end cards
typedef enum {
    SL_AMBIENT, // execute after lighting
    SL_BUMP,
    SL_DIFFUSE,
    SL_SPECULAR,
   	SL_COVERAGE,
} stageLighting_t;

// cross-blended terrain textures need to modulate the color by
// the vertex color to smoothly blend between two textures
typedef enum {
    SVC_IGNORE,
    SVC_MODULATE,
   	SVC_MODULATE_ALPHA,
    SVC_INVERSE_MODULATE,
    SVC_INVERSE_MODULATE_ALPHA
} stageVertexColor_t;

static const int MAX_VERTEX_PARMS = 16;
static const int MAX_FRAGMENT_PARMS = 8;

// ummm oo0ps i forgot about the new shader stages struct, think a few others
// did as well. we can combine these two newShader Stage_t and material Stage_t
typedef struct {
    int					vertexProgram;

	int					m8d5rVertexProgram;

    int					numVertexParms;
    int					vertexParms[MAX_VERTEX_PARMS][4]; // evaluated register indexes

    int					fragmentProgram;

    int					numFragmentProgramImages;
    anImage *			fragmentProgramImages[MAX_FRAGMENT_IMAGES];

    idMegaTexture *		megaTexture; // handles all the binding and parameter setting
//} materialStage_t;
    int 				conditionRegister;    // if registers[conditionRegister] == 0, skip stage
    stageLighting_t 	lighting; // determines which passes interact with lights
    int 				drawStateBits;
    colorStage_t 		color;
    bool 				hasAlphaTest;
    int                 alphaTestRegister;
	bool				hasAlphaFunc;
	int					alphaTestMode;
    textureStage_t      texture;
    stageVertexColor_t  vertexColor;
    bool                ignoreAlphaTest;       // this stage should act as translucent, even
                                // if the surface is alpha tested
    float				privatePolygonOffset; // a per-stage polygon offset

    materialStage_t    *newStage; // vertex / fragment program based stage
} materialStage_t;

typedef enum {
	MC_BAD,
	MC_OPAQUE,			// completely fills the triangle, will have black drawn on fillDepthBuffer
	MC_PERFORATED,		// may have alpha tested holes
	MC_TRANSLUCENT		// blended with background
} materialCoverage_t;

typedef enum {
	SS_MIN = -10000,
  SS_SUBVIEW = -3, // mirrors, viewscreens, etc
  SS_GUI = -2,     // guis
  SS_BAD = -1,
  SS_OPAQUE,        // opaque
  SS_OPAQUEFIRST,   // opaque
  SS_OPAQUENEAR,    // used for materials with expensive shaders, such as the
                    // megaTexture
  SS_OPAQUENEAREST, // used for materials that definitely should be rendered
                    // 'last'

  SS_FAR_ATMOS,    // Translucent materials drawn before the atmosphere, this is
                   // usally not what we want
  SS_MEDIUM_ATMOS, // as translucent mats don't fill the z-buffer and thus get
                   // fogged like what's behind them
  SS_CLOSE_ATMOS,  // instead of for their own depth.
  SS_PORTAL_SKY,

  SS_DECAL, // scorch marks, etc.
  //
  //	Anything whith the following sort orders will need to be fogged
  //"manually" (i.e. by setting up the shader properly)
  //
  SS_ATMOSPHERE, // Not really used; just as a new phase
  SS_FAR,
  SS_MEDIUM, // normal translucent
  SS_CLOSE,
  SS_REFRACTABLE,    // Translucent, but drawn before refraction surfaces
  SS_REFRACTION,     // Stage using refraction screen copy to texture
  SS_ALMOST_NEAREST, // gun smoke puffs

  SS_NEAREST, // screen blood blobs

  SS_POST_PROCESS = 100, // after a screen copy to texture
  SS_MAX                // needs to be last
} materialSort_t;

typedef enum {
  CT_FRONT_SIDED,
  CT_BACK_SIDED,
  CT_TWO_SIDED,
  CT_INVALID
} cullType_t;

enum copyBuffer_t { CB_COLOR, CB_DEPTH };

// these don't effect per-material storage, so they can be very large
const int MAX_SHADER_STAGES = 256;
const int MAX_TEXGEN_REGISTERS = 4;
const int MAX_ENTITY_SHADER_PARMS = 12;

// material flags
typedef enum {
	MF_DEFAULTED = BIT(0),
	MF_POLYGONOFFSET = BIT(1),
	MF_NOSHADOWS = BIT(2),
	MF_FORCESHADOWS = BIT(3),
	MF_NOSELFSHADOW = BIT(4),
	MF_NOPORTALFOG = BIT(5), // this fog volume won't ever consider a portal fogged out
	MF_EDITOR_VISIBLE = BIT(6) // in use (visible) per editor
} materialFlags_t;

// contents flags, NOTE: make sure to keep the defines in doom_defs.script up to
// date with these!
typedef enum {
	CONTENTS_SOLID = BIT(0),        // an eye is never valid in a solid
	CONTENTS_OPAQUE = BIT(1),       // blocks visibility (for ai)
	CONTENTS_WATER = BIT(2),        // used for water
	CONTENTS_PLAYERCLIP = BIT(3),   // solid to players
	CONTENTS_MOVEABLECLIP = BIT(4), // solid to moveable entities
	CONTENTS_IKCLIP = BIT(5),       // solid to IK
	CONTENTS_BLOOD = BIT(6),        // used to detect blood decals
	CONTENTS_BODY = BIT(7),         // used for actors
	CONTENTS_PROJECTILE = BIT(8),   // used for projectiles
	CONTENTS_CORPSE = BIT(9),       // used for dead bodies
	CONTENTS_RENDERMODEL = BIT(10),                // used for render models for collision detection
	CONTENTS_TRIGGER = BIT(11), // used for triggers
	CONTENTS_AAS_SOLID = BIT(12), // solid for AAS
	CONTENTS_AAS_OBSTACLE = BIT(13), // used to compile an obstacle into AAS that can be enabled/disabled
	CONTENTS_FLASHLIGHT_TRIGGER = BIT(14), // used for triggers that are activated by the flashlight
	CONTENTS_SHADOWCOLLISION = BIT(15), // used for shadow collision

	CONTENTS_AAS_SOLID_PLAYER = BIT(24),
	CONTENTS_AAS_SOLID_VEHICLE = BIT(25),
	CONTENTS_AAS_CLUSTER_PORTAL = BIT(26),
	CONTENTS_AAS_OBSTACLE = BIT(27),

	// contents used by utils
	CONTENTS_AREAPORTAL = BIT(20), // portal separating renderer areas
	CONTENTS_NOCSG = BIT(21), // don't cut this brush with CSG operations in the editor

	CONTENTS_REMOVE_UTIL = ~(CONTENTS_AREAPORTAL | CONTENTS_NOCSG)
} contentsFlags_t;

// surface types
const int NUM_SURFACE_BITS = 4;
const int MAX_SURFACE_TYPES = 1 << NUM_SURFACE_BITS;

typedef enum {   // why twice to define the  same things?
    SURFTYPE_NONE, // default type
    SURFTYPE_METAL,
    SURFTYPE_STONE,
    SURFTYPE_FLESH,
    SURFTYPE_WOOD,
    SURFTYPE_CARDBOARD,
    SURFTYPE_LIQUID,
    SURFTYPE_GLASS,
    SURFTYPE_PLASTIC,
    SURFTYPE_RICOCHET,
    SURFTYPE_MUD,
    SURFTYPE_GRASS,
    SURFTYPE_SNOW,
    SURFTYPE_WATER,
    SURFTYPE_14,
    SURFTYPE_15
} surfTypes_t;

// surface flags
typedef enum {
	SURF_TYPE_BIT0 = BIT(0), // encodes the material type (metal, flesh, concrete, etc.)
	SURF_TYPE_BIT1 = BIT(1), // "
	SURF_TYPE_BIT2 = BIT(2), // "
	SURF_TYPE_BIT3 = BIT(3), // "
	SURF_TYPE_MASK = (1 << NUM_SURFACE_BITS) - 1,

	SURF_NODAMAGE = BIT(4),    // never give falling damage
	SURF_SLICK = BIT(5),       // effects game physics
	SURF_COLLISION = BIT(6),   // collision surface
	SURF_LADDER = BIT(7),      // player can climb up this surface
	SURF_NOIMPACT = BIT(8),    // don't make missile explosions
	SURF_NOSTEPS = BIT(9),     // no footstep sounds
	SURF_DISCRETE = BIT(10),   // not clipped or merged by utilities
	SURF_NOFRAGMENT = BIT(11), // dmap won't cut surface at each bsp boundary
	SURF_NULLNORMAL = BIT(12)  // renderbump will draw this surface as 0x80 0x80
	                            // 0x80, which won't collect light from any angle
} surfaceFlags_t;

class ARCSoundEmitter;

class anMaterial : public arcDecleration {
public:
	anMaterial();
	virtual ~anMaterial();

	virtual size_t Size( void ) const;
	virtual bool SetDefaultText( void );
	virtual const char *DefaultDefinition( void ) const;
	virtual bool Parse( const char *text, const int textLength );
	virtual void FreeData( void );
	virtual void Print( void ) const;

	bool Save( const char *fileName = NULL ) const;

  // returns the internal image name for stage 0, which can be used
  // for the renderer CaptureRenderToImage() call
  // I'm not really sure why this needs to be virtual...
  virtual const char *ImageName( void ) const;

  void ReloadImages( bool force ) const;

  // returns number of stages this material contains
  const int GetNumStages( void ) const { return numStages; }

  // get a specific stage
  const materialStage_t *GetStage( const int index ) const {
    assert( index >= 0 && index < numStages );
    return &stages[index];
  }

  // get the first bump map stage, or NULL if not present.
  // used for bumpy-specular
  const materialStage_t *GetBumpStage( void ) const;

  // returns true if the material will draw anything at all.  Triggers, portals,
  // etc, will not have anything to draw.  A not drawn surface can still
  // castShadow, which can be used to make a simplified shadow hull for a
  // complex object set as noShadow
  bool IsDrawn( void ) const {
    return ( numStages > 0 || entityGui != 0 || gui != NULL );
  }

  // returns true if the material will draw any non light interaction stages
  bool HasAmbient( void ) const { return ( numAmbientStages > 0 ); }

  // returns true if material has a gui
  bool HasGui( void ) const { return ( entityGui != 0 || gui != NULL ); }

  // returns true if the material will generate another view, either as
  // a mirror or dynamic rendered image
  bool HasSubview( void ) const { return hasSubview; }

  // returns true if the material will generate shadows, not making a
  // distinction between global and no-self shadows
  bool SurfaceCastsShadow( void ) const {
    return TestMaterialFlag( MF_FORCESHADOWS ) || !TestMaterialFlag( MF_NOSHADOWS );
  }

  // returns true if the material will generate interactions with fog/blend
  // lights All non-translucent surfaces receive fog unless they are explicitly
  // noFog
  bool ReceivesFog( void ) const {
    return ( IsDrawn() && !noFog && coverage != MC_TRANSLUCENT );
  }

  // returns true if the material will generate interactions with normal lights
  // Many special effect surfaces don't have any bump/diffuse/specular
  // stages, and don't interact with lights at all
  bool ReceivesLighting( void ) const { return numAmbientStages != numStages; }

  // returns true if the material should generate interactions on sides facing
  // away from light centers, as with noshadow and noselfshadow options
  bool ReceivesLightingOnBackSides( void ) const {
    return ( materialFlags & ( MF_NOSELFSHADOW | MF_NOSHADOWS ) ) != 0;
  }

  // Standard two-sided triangle rendering won't work with bump map lighting,
  // because the normal and tangent vectors won't be correct for the back sides.
  // When two sided lighting is desired. typically for alpha tested surfaces,
  // this is addressed by having CleanupModelSurfaces() create duplicates of all
  // the triangles with apropriate order reversal.
  bool ShouldCreateBackSides( void ) const { return shouldCreateBackSides; }

  // This surface has a different material on the backside
  const anMaterial *GetBackSideMaterial( void ) const {
    return backSideMaterial;
  }
  // characters and models that are created by a complete renderbump can use a
  // faster method of tangent and normal vector generation than surfaces which
  // have a flat renderbump wrapped over them.
  bool UseUnsmoothedTangents( void ) const { return unsmoothedTangents; }

  // by default, monsters can have blood overlays placed on them, but this can
  // be overrided on a per-material basis with the "noOverlays" material
  // command. This will always return false for translucent surfaces
  bool AllowOverlays( void ) const { return allowOverlays; }

  // MC_OPAQUE, MC_PERFORATED, or MC_TRANSLUCENT, for interaction list linking
  // and dmap flood filling The depth buffer will not be filled for
  // MC_TRANSLUCENT surfaces
  // FIXME: what do nodraw surfaces return?
  materialCoverage_t Coverage( void ) const { return coverage; }

  // returns true if this material takes precedence over other in coplanar cases
  bool HasHigherDmapPriority( const anMaterial &other) const {
    return ( IsDrawn() && !other.IsDrawn() ) || ( Coverage() < other.Coverage() );
  }

  // returns a anUserInterfaces if it has a global gui, or NULL if no gui
  anUserInterfaces *GlobalGui( void ) const { return gui; }

  // a discrete surface will never be merged with other surfaces by dmap, which
  // is necessary to prevent mutliple gui surfaces, mirrors, autosprites, and
  // some other special effects from being combined into a single surface guis,
  // merging sprites or other effects, mirrors and remote views are always
  // discrete
  bool IsDiscrete( void ) const {
    return ( entityGui || gui || deform != DFRM_NONE || sort == SS_SUBVIEW || ( surfaceFlags & SURF_DISCRETE ) != 0 );
  }

  // Normally, dmap chops each surface by every BSP boundary, then reoptimizes.
  // For gigantic polygons like sky boxes, this can cause a huge number of
  // planar triangles that make the optimizer take forever to turn back into a
  // single triangle.  The "noFragment" option causes dmap to only break the
  // polygons at area boundaries, instead of every BSP boundary.  This has the
  // negative effect of not automatically fixing up interpenetrations, so when
  // this is used, you should manually make the edges of your sky box exactly
  // meet, instead of poking into each other.
  bool NoFragment( void ) const { return ( surfaceFlags & SURF_NOFRAGMENT ) != 0; }

  // Some materials don't receive lighting, but still require full normals (and
  // sometimes tangents) to be calculated for per vertex/fragment effects. This
  // flag makes sure that the normals and tangents are generated at all times
  bool ForceTangentsCalculation( void ) const {
    return ( materialFlags & ( MF_FORCETANGENTS ) ) != 0;
  }

  //------------------------------------------------------------------
  // light shader specific functions, only called for light entities

  // lightshader option to fill with fog from viewer instead of light from
  // center
  bool IsFogLight() const { return fogLight; }

  // perform simple blending of the projection, instead of interacting with
  // bumps and textures
  bool IsBlendLight() const { return blendLight; }

  // an ambient light has non-directional bump mapping and no specular
  bool IsAmbientLight() const { return ambientLight; }

  // implicitly no-shadows lights (ambients, fogs, etc) will never cast shadows
  // but individual light entities can also override this value
  bool LightCastsShadows() const {
    return TestMaterialFlag( MF_FORCESHADOWS ) || ( !fogLight && !ambientLight && !blendLight && !TestMaterialFlag( MF_NOSHADOWS ) );
  }

  // fog lights, blend lights, ambient lights, etc will all have to have
  // interaction triangles generated for sides facing away from the light as
  // well as those facing towards the light.  It is debatable if noshadow lights
  // should effect back sides, making everything "noSelfShadow", but that would
  // make noshadow lights potentially slower than normal lights, which detracts
  // from their optimization ability, so they currently do not.
  bool LightEffectsBackSides() const {
    return fogLight || ambientLight || blendLight;
  }

  // NULL unless an image is explicitly specified in the shader with
  // "lightFalloffShader <image>"
anImage *LightFalloffImage() const {
    return lightFalloffImage;
}

  //------------------------------------------------------------------

  // returns the renderbump command line for this shader, or an empty string if
  // not present
const char *GetRenderBump() const { return renderBump; };

  // set specific material flag(s)
void SetMaterialFlag( const int flag ) const { materialFlags |= flag; }

  // clear specific material flag(s)
void ClearMaterialFlag( const int flag ) const { materialFlags &= ~flag; }

  // test for existance of specific material flag(s)
  bool TestMaterialFlag( const int flag ) const {
    return ( materialFlags & flag ) != 0;
}

  // get content flags
const int GetContentFlags( void ) const { return contentFlags; }

  // get surface flags
const int GetSurfaceFlags( void ) const { return surfaceFlags; }

  // gets name for surface type (stone, metal, flesh, etc.)
const surfTypes_t GetSurfaceType( void ) const {
    return static_cast<surfTypes_t>( surfaceFlags & SURF_TYPE_MASK );
}

  // get material description
const char *GetDescription( void ) const { return desc; }

  // get sort orderconst float GetSort( void ) const { return sort; }
  // this is only used by the gui system to force sorting order
  // on images referenced from tga's instead of materials.
  // this is done this way as there are 2000 tgas the guis use
void SetSort( float s ) const { sort = s; };

  // DFRM_NONE, DFRM_SPRITE, etc
deformSurf_t Deform( void ) const { return deform; }

  // flare size, expansion size, etc
const int GetDeformRegister( int index ) const {
    return deformRegisters[index];
}

  // particle system to emit from surface and table for turbulent
const arcDecleration *GetDeformDecl( void ) const { return deformDecl; }

  // currently a surface can only have one unique texgen for all the stages
  texGen_t Texgen() const;

  // wobble sky parms
const int *GetTexGenRegisters( void ) const { return texGenRegisters; }

  // get cull type
const cullType_t GetCullType( void ) const { return cullType; }

float GetEditorAlpha( void ) const { return editorAlpha; }

int GetEntityGui( void ) const { return entityGui; }

decalInfo_t GetDecalInfo( void ) const { return decalInfo; }

  // spectrums are used for "invisible writing" that can only be
  // illuminated by a light of matching spectrum
int Spectrum( void ) const { return spectrum; }

float GetPolygonOffset( void ) const { return polygonOffset; }

float GetSurfaceArea( void ) const { return surfaceArea; }
void AddToSurfaceArea( float area ) { surfaceArea += area; }

  //------------------------------------------------------------------

  // returns the length, in milliseconds, of the videoMap on this material,
  // or zero if it doesn't have one
  // int					CinematicLength( void ) const;
  // void				CloseCinematic( void ) const;
  // void				ResetCinematicTime( int time ) const;
  // void				UpdateCinematic( int time ) const;

  //------------------------------------------------------------------

  // gets an image for the editor to use
  anImage *GetEditorImage( void ) const;
  int GetImageWidth( void ) const;
  int GetImageHeight( void ) const;

  void SetGui( const char *_gui) const;

  // just for resource tracking
  void SetImageClassifications(int tag) const;

  //------------------------------------------------------------------

  // returns number of registers this material contains
  const int GetNumRegisters() const { return numRegisters; }

  // regs should point to a float array large enough to hold GetNumRegisters()
  // floats
  void EvaluateRegisters( float *regs, const float entityParms[MAX_ENTITY_SHADER_PARMS], const struct viewDef_s *view, ARCSoundEmitter *soundEmitter = NULL ) const;

  // if a material only uses constants (no entityParm or globalparm references),
  // this will return a pointer to an internal table, and EvaluateRegisters will
  // not need to be called.  If NULL is returned, EvaluateRegisters must be
  // used.
  const float *ConstantRegisters() const;

  bool SuppressInSubview() const { return suppressInSubview; };
  bool IsPortalSky() const { return portalSky; };
  void AddReference();

private:
  // parse the entire material
  void CommonInit();
  void ParseMaterial( anLexer &src);
  bool MatchToken( anLexer &src, const char *match );
  void ParseSort( anLexer &src);
  void ParseBlend( anLexer &src, materialStage_t *stage );
  void ParseVertexParm( anLexer &src, materialStage_t *newStage );
  void ParseFragmentMap( anLexer &src, materialStage_t *newStage );
  void ParseStage( anLexer &src, const textureRepeat_t trpDefault = TR_REPEAT );
  void ParseDeform( anLexer &src);
  void ParseDecalInfo( anLexer &src);
  bool CheckSurfaceParm(arcNetToken *token);
  int GetExpressionConstant(float f);
  int GetExpressionTemporary( void );
  expOp_t *GetExpressionOp( void );
  int EmitOp(int a, int b, expOpType_t opType );
  int ParseEmitOp( anLexer &src, int a, expOpType_t opType, int priority );
  int ParseTerm( anLexer &src );
  int ParseExpressionPriority( anLexer &src, int priority );
  int ParseExpression( anLexer &src );
  void ClearStage( materialStage_t *ss );
  int NameToSrcBlendMode( const anString &name );
  int NameToDstBlendMode( const anString &name );
  void MultiplyTextureMatrix( textureStage_t *ts,int registers[2][3] ); // FIXME: for some reason the const is bad for gcc and Mac
  void SortInteractionStages();
  void AddImplicitStages( const textureRepeat_t trpDefault = TR_REPEAT);
  void CheckForConstantRegisters();

private:
    anString desc;       // description
    anString renderBump; // renderbump command options, without the "renderbump"
                          // at the start

    anImage *lightFalloffImage;

    int entityGui; // draw a gui with the anUserInterfaces from the
                   // renderEntity_t non zero will draw gui, gui2, or gui3 from
                   // renderEnitty_t
    mutable anUserInterfaces *gui; // non-custom guis are shared by all users of a material

    bool noFog; // surface does not create fog interactions

    int spectrum; // for invisible writing, used for both lights and surfaces

    float polygonOffset;

    int contentFlags;          // content flags
    int surfaceFlags;          // surface flags
    mutable int materialFlags; // material flags

    decalInfo_t decalInfo;

    mutable float sort; // lower numbered shaders draw before higher numbered
    deformSurf_t deform;
    int deformRegisters[4]; // numeric parameter for deforms
    const arcDecleration *deformDecl; // for surface emitted particle deforms and tables

    int texGenRegisters[MAX_TEXGEN_REGISTERS]; // for wobbleSky

    materialCoverage_t coverage;
    cullType_t cullType; // CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
    bool shouldCreateBackSides;

    bool fogLight;
    bool blendLight;
    bool ambientLight;
    bool unsmoothedTangents;
    bool hasSubview; // mirror, remote render, etc
    bool allowOverlays;

    int numOps;
    expOp_t *ops; // evaluate to make expressionRegisters

    int numRegisters; //
    float *expressionRegisters;

    float *constantRegisters; // NULL if ops ever reference globalParms or
                              // entityParms

    int numStages;
    int numAmbientStages;

    materialStage_t *stages;

    struct mtrParsingData_s *pd; // only used during parsing

    float surfaceArea; // only for listSurfaceAreas

    // we defer loading of the editor image until it is asked for, so the game
    // doesn't load up all the invisible and uncompressed images. If editorImage
    // is NULL, it will atempt to load editorImageName, and set editorImage to
    // that or defaultImage
    anString editorImageName;
    mutable anImage *editorImage; // image used for non-shaded preview
    float editorAlpha;

    bool suppressInSubview;
    bool portalSky;
    int refCount;
};

typedef anList<const anMaterial *> anMatList;

#endif // !__MATERIAL_H__

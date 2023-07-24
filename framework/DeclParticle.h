#ifndef __DECLPARTICLE_H__
#define __DECLPARTICLE_H__

/*
===============================================================================

	arcDeclParticle

===============================================================================
*/

static const int MAX_PARTICLE_STAGES	= 32;

class idParticleParm {
public:
							idParticleParm() { table = NULL; from = to = 0.0f; }

	const arcDeclTable *		table;
	float					from;
	float					to;

	float					Eval( float frac, arcRandom &rand ) const;
	float					Integrate( float frac, arcRandom &rand ) const;
};


typedef enum {
	PDIST_RECT,				// ( sizeX sizeY sizeZ )
	PDIST_CYLINDER,			// ( sizeX sizeY sizeZ )
	PDIST_SPHERE			// ( sizeX sizeY sizeZ ringFraction )
							// a ringFraction of zero allows the entire sphere, 0.9 would only
							// allow the outer 10% of the sphere
} prtDistribution_t;

typedef enum {
	PDIR_CONE,				// parm0 is the solid cone angle
	PDIR_OUTWARD			// direction is relative to offset from origin, parm0 is an upward bias
} prtDirection_t;

typedef enum {
	PPATH_STANDARD,
	PPATH_HELIX,			// ( sizeX sizeY sizeZ radialSpeed climbSpeed )
	PPATH_FLIES,
	PPATH_ORBIT,
	PPATH_DRIP
} prtCustomPth_t;

typedef enum {
	POR_VIEW,
	POR_AIMED,				// angle and aspect are disregarded
	POR_X,
	POR_Y,
	POR_Z
} prtOrientation_t;

typedef struct renderEntity_s renderEntity_t;
typedef struct renderView_s renderView_t;

typedef struct {
	const renderEntity_t *	renderEnt;			// for shaderParms, etc
	const renderView_t *	renderView;
	int						index;				// particle number in the system
	float					frac;				// 0.0 to 1.0
	arcRandom				random;
	arcVec3					origin;				// dynamic smoke particles can have individual origins and axis
	arcMat3					axis;


	float					age;				// in seconds, calculated as fraction * stage->particleLife
	arcRandom				originalRandom;		// needed so aimed particles can reset the random for another origin calculation
	float					animationFrameFrac;	// set by ParticleTexCoords, used to make the cross faded version
} particleGen_t;


//
// single particle stage
//
class arcParticleStage {
public:
							arcParticleStage();
							~arcParticleStage() {}

	void					Default();
	int						NumQuadsPerParticle() const;	// includes trails and cross faded animations
	// returns the number of verts created, which will range from 0 to 4*NumQuadsPerParticle()
	int						CreateParticle( particleGen_t *g, arcDrawVert *verts ) const;

	void					ParticleOrigin( particleGen_t *g, arcVec3 &origin ) const;
	int						ParticleVerts( particleGen_t *g, const arcVec3 origin, arcDrawVert *verts ) const;
	void					ParticleTexCoords( particleGen_t *g, arcDrawVert *verts ) const;
	void					ParticleColors( particleGen_t *g, arcDrawVert *verts ) const;

	const char *			GetCustomPathName();
	const char *			GetCustomPathDesc();
	int						NumCustomPathParms();
	void					SetCustomPathType( const char *p );
	void					operator=( const arcParticleStage &src );


	//------------------------------

	const arcMaterial *		material;

	int						totalParticles;		// total number of particles, although some may be invisible at a given time
	float					cycles;				// allows things to oneShot ( 1 cycle ) or run for a set number of cycles
												// on a per stage basis

	int						cycleMsec;			// ( particleLife + deadTime ) in msec

	float					spawnBunching;		// 0.0 = all come out at first instant, 1.0 = evenly spaced over cycle time
	float					particleLife;		// total seconds of life for each particle
	float					timeOffset;			// time offset from system start for the first particle to spawn
	float					deadTime;			// time after particleLife before respawning

	//-------------------------------	// standard path parms

	prtDistribution_t		distributionType;
	float					distributionParms[4];

	prtDirection_t			directionType;
	float					directionParms[4];

	idParticleParm			speed;
	float					gravity;				// can be negative to float up
	bool					worldGravity;			// apply gravity in world space
	bool					randomDistribution;		// randomly orient the quad on emission ( defaults to true )
	bool					entityColor;			// force color from render entity ( fadeColor is still valid )

	//------------------------------	// custom path will completely replace the standard path calculations

	prtCustomPth_t			customPathType;		// use custom C code routines for determining the origin
	float					customPathParms[8];

	//--------------------------------

	arcVec3					offset;				// offset from origin to spawn all particles, also applies to customPath

	int						animationFrames;	// if > 1, subdivide the texture S axis into frames and crossfade
	float					animationRate;		// frames per second

	float					initialAngle;		// in degrees, random angle is used if zero ( default )
	idParticleParm			rotationSpeed;		// half the particles will have negative rotation speeds

	prtOrientation_t		orientation;	// view, aimed, or axis fixed
	float					orientationParms[4];

	idParticleParm			size;
	idParticleParm			aspect;				// greater than 1 makes the T axis longer

	arcVec4					color;
	arcVec4					fadeColor;			// either 0 0 0 0 for additive, or 1 1 1 0 for blended materials
	float					fadeInFraction;		// in 0.0 to 1.0 range
	float					fadeOutFraction;	// in 0.0 to 1.0 range
	float					fadeIndexFraction;	// in 0.0 to 1.0 range, causes later index smokes to be more faded

	bool					hidden;				// for editor use
	//-----------------------------------

	float					boundsExpansion;	// user tweak to fix poorly calculated bounds

	arcBounds				bounds;				// derived
};


//
// group of particle stages
//
class arcDeclParticle : public arcDecleration {
public:

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	bool					Save( const char *fileName = NULL );

	// Loaded instead of re-parsing, written if MD5 hash different
	bool					LoadBinary( arcNetFile * file, unsigned int checksum );
	void					WriteBinary( arcNetFile * file, unsigned int checksum );

	arcNetList<arcParticleStage *, TAG_LIBLIST_DECL>stages;
	arcBounds				bounds;
	float					depthHack;

private:
	bool					RebuildTextSource();
	void					GetStageBounds( arcParticleStage *stage );
	arcParticleStage *		ParseParticleStage( arcLexer &src );
	void					ParseParms( arcLexer &src, float *parms, int maxParms );
	void					ParseParametric( arcLexer &src, idParticleParm *parm );
	void					WriteStage( arcNetFile *f, arcParticleStage *stage );
	void					WriteParticleParm( arcNetFile *f, idParticleParm *parm, const char *name );
};

#endif /* !__DECLPARTICLE_H__ */

#include "/idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"
#include "Model_local.h"

static const char *parametricParticle_SnapshotName = "_ParametricParticle_Legacy_Snapshot_";

/*====================================================================================================
	integrating to our new .voxtex formatted volume textures


====================================================================================================*/

/*
====================
idRenderModelPrt_Legacy::idRenderModelPrt_Legacy
====================
*/
idRenderModelPrt::idRenderModelPrt() {
	particleSystem = nullptr;
}

/*
====================
idRenderModelPrt::InitFromFile
====================
*/
void idRenderModelPrt::InitFromFile( const char *fileName ) {
	name = fileName;
	particleSystem = static_cast<const anDeclParticle *>( declManager->FindType( DECL_PARTICLE, fileName ) );
}

/*
=================
idRenderModelPrt::TouchData
=================
*/
void idRenderModelPrt::TouchData( void ) {
	// Ensure our particle system is added to the list of referenced decls
	particleSystem = static_cast<const anDeclParticle *>( declManager->FindType( DECL_PARTICLE, name ) );
}

/*
====================
idRenderModelPrt::InstantiateDynamicModel
====================
*/
anRenderModel *idRenderModelPrt::InstantiateDynamicModel( const struct renderEntity_s *renderEntity, const struct viewDef_s *viewDef, anRenderModel *cachedModel ) {
	anModelStatic *staticModel;

	if ( cachedModel && !r_useCachedDynamicModels.GetBool() ) {
		delete cachedModel;
		cachedModel = nullptr;
	}

	// this may be triggered by a model trace or other non-view related source, to which we should look like an empty model
	if ( renderEntity == nullptr || viewDef == nullptr ) {
		delete cachedModel;
		return nullptr;
	}

	if ( r_skipParticles.GetBool() ) {
		delete cachedModel;
		return nullptr;
	}

/*	// if the entire system has faded out
	if ( renderEntity->shaderParms[SP_PARTICLE_STOPTIME] && viewDef->renderView.time * 0.001f >= renderEntity->shaderParms[SP_PARTICLE_STOPTIME] ) {
		delete cachedModel;
		return nullptr;
	}*/

	if ( cachedModel != nullptr ) {
		assert( dynamic_cast<anModelStatic *>( cachedModel ) != nullptr );
		assert( anStr::Icmp( cachedModel->Name(), parametricParticle_SnapshotName ) == 0 );

		staticModel = static_cast<anModelStatic *>( cachedModel );
	} else {
		staticModel = new anModelStatic;
		staticModel->InitEmpty( parametricParticle_SnapshotName );
	}

	particleGen_t g;

	g.renderEnt = renderEntity;
	g.renderView = &viewDef->renderView;
	g.origin.Zero();
	g.axis.Identity();

	for ( int stageNum = 0; stageNum < particleSystem->stages.Num(); stageNum++ ) {
		anParticleStage *stage = particleSystem->stages[stageNum];
		if ( !stage->material ) {
			continue;
		}
		if ( !stage->cycleMsec ) {
			continue;
		}
		if ( stage->hidden ) {		// just for gui particle editor use
			staticModel->DeleteSurfaceWithId( stageNum );
			continue;
		}

		anRandom steppingRandom, steppingRandom2;

		int stageAge = g.renderView->time + renderEntity->shaderParms[SS_TIMEOFFSET] * 1000 - stage->timeOffset * 1000;
		int	stageCycle = stageAge / stage->cycleMsec;
		int	inCycleTime = stageAge - stageCycle * stage->cycleMsec;

		// some particles will be in this cycle, some will be in the previous cycle
		steppingRandom.SetSeed( ( ( stageCycle << 10 ) & anRandom::MAX_RAND) ^ ( int )( renderEntity->shaderParms[SP_DIVERSITY] * anRandom::MAX_RAND )  );
		steppingRandom2.SetSeed( ( ( ( stageCycle-1 ) << 10 ) & anRandom::MAX_RAND) ^ ( int )( renderEntity->shaderParms[SP_DIVERSITY] * anRandom::MAX_RAND )  );

		int	count = stage->totalParticles * stage->NumQuadsPerParticle();

		int surfaceNum;
		modelSurface_t *surf;

		if ( staticModel->FindSurfaceWithId( stageNum, surfaceNum ) ) {
			surf = &staticModel->surfaces[surfaceNum];
			R_FreeStaticTriSurfVertexCaches( surf->geometry );
		} else {
			surf = &staticModel->surfaces.Alloc();
			surf->id = stageNum;
			surf->shader = stage->material;
			surf->geometry = R_AllocStaticTriSurf();
			R_AllocStaticTriSurfVerts( surf->geometry, 4 * count );
			R_AllocStaticTriSurfIndexes( surf->geometry, 6 * count );
			R_AllocStaticTriSurfPlanes( surf->geometry, 6 * count );
		}

		int numVerts = 0;
		anDrawVertex *verts = surf->geometry->verts;

		for ( int index = 0; index < stage->totalParticles; index++ ) {
			g.index = index;

			// bump the random
			steppingRandom.RandomInt();
			steppingRandom2.RandomInt();

			// calculate local age for this index
			int	bunchOffset = stage->particleLife * 1000 * stage->spawnBunching * index / stage->totalParticles;

			int particleAge = stageAge - bunchOffset;
			int	particleCycle = particleAge / stage->cycleMsec;
			if ( particleCycle < 0 ) {
				// before the particleSystem spawned
				continue;
			}
			if ( stage->cycles && particleCycle >= stage->cycles ) {
				// cycled systems will only run cycle times
				continue;
			}

			if ( particleCycle == stageCycle ) {
				g.random = steppingRandom;
			} else {
				g.random = steppingRandom2;
			}

			int	inCycleTime = particleAge - particleCycle * stage->cycleMsec;

			if ( renderEntity->shaderParms[SP_PARTICLE_STOPTIME] &&
				g.renderView->time - inCycleTime >= renderEntity->shaderParms[SP_PARTICLE_STOPTIME]*1000 ) {
				// don't fire any more particles
				continue;
			}

			// supress particles before or after the age clamp
			g.frac = ( float )inCycleTime / ( stage->particleLife * 1000 );
			if ( g.frac < 0.0f ) {
				// yet to be spawned
				continue;
			}
			if ( g.frac > 1.0f ) {
				// this particle is in the deadTime band
				continue;
			}

			// this is needed so aimed particles can calculate origins at different times
			g.originalRandom = g.random;

			g.age = g.frac * stage->particleLife;

			// if the particle doesn't get drawn because it is faded out or beyond a kill region, don't increment the verts
			numVerts += stage->CreateParticle( &g, verts + numVerts );
		}

		// numVerts must be a multiple of 4
		assert( ( numVerts & 3 ) == 0 && numVerts <= 4 * count );

		// build the indexes
		int	numIndexes = 0;
		qglIndex_t *indexes = surf->geometry->indexes;
		for ( int i = 0; i < numVerts; i += 4 ) {
			indexes[numIndexes+0] = i;
			indexes[numIndexes+1] = i+2;
			indexes[numIndexes+2] = i+3;
			indexes[numIndexes+3] = i;
			indexes[numIndexes+4] = i+3;
			indexes[numIndexes+5] = i+1;
			numIndexes += 6;
		}

		surf->geometry->tangentsCalculated = false;
		surf->geometry->facePlanesCalculated = false;
		surf->geometry->numVerts = numVerts;
		surf->geometry->numIndexes = numIndexes;
		surf->geometry->bounds = stage->bounds;		// just always draw the particles
	}

	return staticModel;
}

/*
===============
idRenderModelPrt::IsLoaded
===============
*/
bool idRenderModelPrt::IsLoaded() const {
	return true;
}

/*
====================
idRenderModelPrt::IsDynamicModel
====================
*/
dynamicModel_t idRenderModelPrt::IsDynamicModel() const {
	return DM_CONTINUOUS;
}

/*
====================
idRenderModelPrt::Bounds
====================
*/
anBounds idRenderModelPrt::Bounds( const struct renderEntity_s *ent ) const {
	return particleSystem->bounds;
}
/*
====================
idRenderModelPrt::DepthHack
====================
*/
int idRenderModelPrt::NumFrames() const {
	//return particleSystem->stages.Num();
	return numFrames;
}

/*
====================
idRenderModelPrt::DepthHack
====================
*/
float idRenderModelPrt::DepthHack() const {
	return particleSystem->depthHack;
}

/*
====================
idRenderModelPrt::Memory
====================
*/
int idRenderModelPrt::Memory() const {
	int total = 0;

	total += anModelStatic::Memory();

	if ( particleSystem ) {
		total += sizeof( *particleSystem );

		for ( int i = 0; i < particleSystem->stages.Num(); i++ ) {
			total += sizeof( particleSystem->stages[i] );
		}
	}

	return total;
}
	int						CreateParticle( particleGen_t *g, anDrawVertex *verts ) const;

void idRenderModelPrt::CreateParticle( float simScale, anMat3 axis, int size, anDrawVert *verts ) const {}
void idRenderModelPrt::LoadModel( void ) {}
void idRenderModelPrt::ProcessGeometryVolumes( int frameNum, anMat3 axis, modelSurface_t *surf ) {}

void idRenderModelPrt::Parse( anParser *src ) {}
void idRenderModelPrt::ParseSimulation( const char *fileName ) {}

/*===========================================================================*/
// A simple sprite model that always faces the view axis.
static const char *sprite_SnapshotName = "_sprite_Snapshot_";

/*
===============
idRenderModelBeam::IsDynamicModel
===============
*/
dynamicModel_t idRenderModelSprite::IsDynamicModel() const {
	return DM_CONTINUOUS;
}

/*
===============
idRenderModelBeam::IsLoaded
===============
*/
bool idRenderModelSprite::IsLoaded() const {
	return true;
}

/*
===============
idRenderModelSprite::InstantiateDynamicModel
===============
*/
anRenderModel *idRenderModelSprite::InstantiateDynamicModel( const struct renderEntity_s *renderEntity, const struct viewDef_s *viewDef, anRenderModel *cachedModel ) {
	anModelStatic *staticModel;
	srfTriangles_t *tri;
	modelSurface_t surf;

	if ( cachedModel && !r_useCachedDynamicModels.GetBool() ) {
		delete cachedModel;
		cachedModel = nullptr;
	}

	if ( renderEntity == nullptr || viewDef == nullptr ) {
		delete cachedModel;
		return nullptr;
	}

	if ( cachedModel != nullptr ) {
		assert( dynamic_cast<anModelStatic *>( cachedModel ) != nullptr );
		assert( anStr::Icmp( cachedModel->Name(), sprite_SnapshotName ) == 0 );

		staticModel = static_cast<anModelStatic *>( cachedModel );
		surf = *staticModel->Surface( 0 );
		tri = surf.geometry;
	} else {
		staticModel = new anModelStatic;
		staticModel->InitEmpty( sprite_SnapshotName );

		tri = R_AllocStaticTriSurf();
		R_AllocStaticTriSurfVerts( tri, 4 );
		R_AllocStaticTriSurfIndexes( tri, 6 );

		tri->verts[0].Clear();
		tri->verts[0].normal.Set( 1.0f, 0.0f, 0.0f );
		tri->verts[0].tangents[0].Set( 0.0f, 1.0f, 0.0f );
		tri->verts[0].tangents[1].Set( 0.0f, 0.0f, 1.0f );
		tri->verts[0].st[0] = 0.0f;
		tri->verts[0].st[1] = 0.0f;

		tri->verts[1].Clear();
		tri->verts[1].normal.Set( 1.0f, 0.0f, 0.0f );
		tri->verts[1].tangents[0].Set( 0.0f, 1.0f, 0.0f );
		tri->verts[1].tangents[1].Set( 0.0f, 0.0f, 1.0f );
		tri->verts[1].st[0] = 1.0f;
		tri->verts[1].st[1] = 0.0f;

		tri->verts[2].Clear();
		tri->verts[2].normal.Set( 1.0f, 0.0f, 0.0f );
		tri->verts[2].tangents[0].Set( 0.0f, 1.0f, 0.0f );
		tri->verts[2].tangents[1].Set( 0.0f, 0.0f, 1.0f );
		tri->verts[2].st[0] = 1.0f;
		tri->verts[2].st[1] = 1.0f;

		tri->verts[3].Clear();
		tri->verts[3].normal.Set( 1.0f, 0.0f, 0.0f );
		tri->verts[3].tangents[0].Set( 0.0f, 1.0f, 0.0f );
		tri->verts[3].tangents[1].Set( 0.0f, 0.0f, 1.0f );
		tri->verts[3].st[0] = 0.0f;
		tri->verts[3].st[1] = 1.0f;

		tri->indexes[0] = 0;
		tri->indexes[1] = 1;
		tri->indexes[2] = 3;
		tri->indexes[3] = 1;
		tri->indexes[ 4 ] = 2;
		tri->indexes[ 5 ] = 3;

		tri->numVerts = 4;
		tri->numIndexes = 6;

		surf.geometry = tri;
		surf.id = 0;
		surf.shader = tr.defaultMaterial;
		staticModel->AddSurface( surf );
	}

	int	red			= anMath::FtoiFast( renderEntity->shaderParms[ SP_RED ] * 255.0f );
	int green		= anMath::FtoiFast( renderEntity->shaderParms[ SP_GREEN ] * 255.0f );
	int	blue		= anMath::FtoiFast( renderEntity->shaderParms[ SS_BLUE ] * 255.0f );
	int	alpha		= anMath::FtoiFast( renderEntity->shaderParms[ SS_APLHA ] * 255.0f );

	anVec3 right	= anVec3( 0.0f, renderEntity->shaderParms[ SHADERPARM_SPRITE_WIDTH ] * 0.5f, 0.0f );
	anVec3 up		= anVec3( 0.0f, 0.0f, renderEntity->shaderParms[ SHADERPARM_SPRITE_HEIGHT ] * 0.5f );

	tri->verts[0].xyz = up + right;
	tri->verts[0].color[0] = red;
	tri->verts[0].color[1] = green;
	tri->verts[0].color[2] = blue;
	tri->verts[0].color[3] = alpha;

	tri->verts[1].xyz = up - right;
	tri->verts[1].color[0] = red;
	tri->verts[1].color[1] = green;
	tri->verts[1].color[2] = blue;
	tri->verts[1].color[3] = alpha;

	tri->verts[2].xyz = - right - up;
	tri->verts[2].color[0] = red;
	tri->verts[2].color[1] = green;
	tri->verts[2].color[2] = blue;
	tri->verts[2].color[3] = alpha;

	tri->verts[3].xyz = right - up;
	tri->verts[3].color[0] = red;
	tri->verts[3].color[1] = green;
	tri->verts[3].color[2] = blue;
	tri->verts[3].color[3] = alpha;

	R_BoundTriSurf( tri );

	staticModel->bounds = tri->bounds;

	return staticModel;
}

/*
===============
idRenderModelSprite::Bounds
===============
*/
anBounds idRenderModelSprite::Bounds( const struct renderEntity_s *renderEntity ) const {
	anBounds b;

	b.Zero();
	if ( renderEntity == nullptr ) {
		b.ExpandSelf( 8.0f );
	} else {
		b.ExpandSelf( Max( renderEntity->shaderParms[SHADERPARM_SPRITE_WIDTH], renderEntity->shaderParms[SHADERPARM_SPRITE_HEIGHT] ) * 0.5f );
	}
	return b;
}

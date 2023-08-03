#include "../..//idlib/Lib.h"
#pragma hdrstop

#include "dmap.h"

/*
  After parsing, there will be a list of entities that each has
  a list of primitives.

  Primitives are either brushes, triangle soups, or model references.

  Curves are tesselated to triangle soups at load time, but model
  references are
  Brushes will have

	brushes, each of which has a side definition.
*/

// private declarations
#define MAX_BUILD_SIDES		300

static	int		entityPrimitive;		// to track editor brush numbers
static	int		c_numMapPatches;
static	int		c_areaportals;

static	uEntity_t	*uEntity;

// brushes are parsed into a temporary array of sides,
// which will have duplicates removed before the final brush is allocated
static	uBrush_t	*buildBrush;

#define	NORMAL_EPSILON			0.00001f
#define	DIST_EPSILON			0.01f

/*
===========
FindFloatPlane
===========
*/
int FindFloatPlane( const anPlane &plane, bool *fixedDegeneracies ) {
	anPlane p = plane;
	bool fixed = p.FixDegeneracies( DIST_EPSILON );
	if ( fixed && fixedDegeneracies ) {
		*fixedDegeneracies = true;
	}
	return dmapGlobals.mapPlanes.FindPlane( p, NORMAL_EPSILON, DIST_EPSILON );
}

/*
===========
SetBrushContents

The contents on all sides of a brush should be the same
Sets contentsShader, contents, opaque
===========
*/
static void SetBrushContents( uBrush_t *b ) {
	int contents;
	side_t s = &b->sides[0];
	contents = s->material->GetContentFlags();

	b->contentShader = s->material;
	bool mixed = false;

	// a brush is only opaque if all sides are opaque
	b->opaque = true;

	for ( int i = 1; i<b->numsides; i++, s++ ) {
		s = &b->sides[i];
		if ( !s->material ) {
			continue;
		}

		int c2 = s->material->GetContentFlags();
		if ( c2 != contents ) {
			mixed = true;
			contents |= c2;
		}

		if ( s->material->Coverage() != MC_OPAQUE ) {
			b->opaque = false;
		}
	}

	if ( contents & CONTENTS_AREAPORTAL ) {
		c_areaportals++;
	}

	b->contents = contents;
}


//============================================================================

/*
===============
FreeBuildBrush
===============
*/
static void FreeBuildBrush( void ) {
	for ( int i = 0; i < buildBrush->numsides; i++ ) {
		if ( buildBrush->sides[i].winding ) {
			delete buildBrush->sides[i].winding;
		}
	}
	buildBrush->numsides = 0;
}

/*
===============
FinishBrush

Produces a final brush based on the buildBrush->sides array
and links it to the current entity
===============
*/
static uBrush_t *FinishBrush( void ) {
	uBrush_t	*b;
	primitive_t	*prim;

	// create windings for sides and bounds for brush
	if ( !CreateBrushWindings( buildBrush ) ) {
		// don't keep this brush
		FreeBuildBrush();
		return nullptr;
	}

	if ( buildBrush->contents & CONTENTS_AREAPORTAL ) {
		if (dmapGlobals.num_entities != 1 ) {
			common->Printf( "Entity %i, Brush %i: areaportals only allowed in world\n"
				,  dmapGlobals.num_entities - 1, entityPrimitive);
			FreeBuildBrush();
			return nullptr;
		}
	}

	// keep it
	b = CopyBrush( buildBrush );

	FreeBuildBrush();

	b->entitynum = dmapGlobals.num_entities-1;
	b->brushnum = entityPrimitive;

	b->original = b;

	prim = (primitive_t *)Mem_Alloc( sizeof( *prim ) );
	memset( prim, 0, sizeof( *prim ) );
	prim->next = uEntity->primitives;
	uEntity->primitives = prim;

	prim->brush = b;

	return b;
}

/*
================
AdjustEntityForOrigin
================
*/
static void AdjustEntityForOrigin( uEntity_t *ent ) {
	for ( primitive_t *prim = ent->primitives; prim; prim = prim->next ) {
		uBrush_t *b = prim->brush;
		if ( !b ) {
			continue;
		}
		for ( int i = 0; i < b->numsides; i++ ) {
			side_t *s = &b->sides[i];
			anPlane plane = dmapGlobals.mapPlanes[s->planenum];
			plane[3] += plane.Normal() * ent->origin;

			s->planenum = FindFloatPlane( plane );

			s->texVec.v[0][3] += DotProduct( ent->origin, s->texVec.v[0] );
			s->texVec.v[1][3] += DotProduct( ent->origin, s->texVec.v[1] );

			// remove any integral shift
			s->texVec.v[0][3] -= floor( s->texVec.v[0][3] );
			s->texVec.v[1][3] -= floor( s->texVec.v[1][3] );
		}
		CreateBrushWindings( b );
	}
}

/*
=================
RemoveDuplicateBrushPlanes

Returns false if the brush has a mirrored set of planes,
meaning it encloses no volume.
Also removes planes without any normal
=================
*/
static bool RemoveDuplicateBrushPlanes( uBrush_t *b ) {
	side_t *sides = b->sides;
	for ( int i = 1; i < b->numsides; i++ ) {
		// check for a degenerate plane
		if ( sides[i].planenum == -1 ) {
			common->Printf( "Entity %i, Brush %i: degenerate plane\n", b->entitynum, b->brushnum);
			// remove it
			for ( int k = i + 1; k < b->numsides; k++ ) {
				sides[k-1] = sides[k];
			}
			b->numsides--;
			i--;
			continue;
		}

		// check for duplication and mirroring
		for ( int j = 0; j < i; j++ ) {
			if ( sides[i].planenum == sides[j].planenum ) {
				common->Printf( "Entity %i, Brush %i: duplicate plane\n", b->entitynum, b->brushnum);
				// remove the second duplicate
				for ( k = i + 1; k < b->numsides; k++ ) {
					sides[k-1] = sides[k];
				}
				b->numsides--;
				i--;
				break;
			}

			if ( sides[i].planenum == (sides[j].planenum ^ 1 ) ) {
				// mirror plane, brush is invalid
				common->Printf( "Entity %i, Brush %i: mirrored plane\n", b->entitynum, b->brushnum);
				return false;
			}
		}
	}
	return true;
}

/*
=================
ParseBrush
=================
*/
static void ParseBrush( const anMapBrush *mapBrush, int primitiveNum ) {
	uBrush_t	*b;
	side_t		*s;
	const anMapBrushSides	*ms;
	int			i;
	bool		fixedDegeneracies = false;

	buildBrush->entitynum = dmapGlobals.num_entities-1;
	buildBrush->brushnum = entityPrimitive;
	buildBrush->numsides = mapBrush->GetNumSides();
	for ( i = 0; i < mapBrush->GetNumSides(); i++ ) {
		s = &buildBrush->sides[i];
		ms = mapBrush->GetSide( i );

		memset( s, 0, sizeof( *s ) );
		s->planenum = FindFloatPlane( ms->GetPlane(), &fixedDegeneracies );
		s->material = declManager->FindMaterial( ms->GetMaterial() );
		ms->GetTextureVectors( s->texVec.v );
		// remove any integral shift, which will help with grouping
		s->texVec.v[0][3] -= floor( s->texVec.v[0][3] );
		s->texVec.v[1][3] -= floor( s->texVec.v[1][3] );
	}

	// if there are mirrored planes, the entire brush is invalid
	if ( !RemoveDuplicateBrushPlanes( buildBrush ) ) {
		return;
	}

	// get the content for the entire brush
	SetBrushContents( buildBrush );

	b = FinishBrush();
	if ( !b ) {
		return;
	}

	if ( fixedDegeneracies && dmapGlobals.verboseentities ) {
		common->Warning( "brush %d has degenerate plane equations", primitiveNum );
	}
}

/*
================
ParseSurface
================
*/
static void ParseSurface( const anMapPatch *patch, const anSurface *surface, const anMaterial *material ) {
	int				i;
	mapTri_t		*tri;
	primitive_t *prim = static_cast<primitive_t *>( Mem_Alloc( prim ) );
	memset( prim, 0, sizeof( *prim ) );
	prim->next = uEntity->primitives;
	uEntity->primitives = prim;

	for ( i = 0; i < surface->GetNumIndexes(); i += 3 ) {
		tri = AllocTri();
		tri->v[2] = (* surface)[surface->GetIndexes()[i+0]];
		tri->v[1] = (* surface)[surface->GetIndexes()[i+2]];
		tri->v[0] = (* surface)[surface->GetIndexes()[i+1]];
		tri->material = material;
		tri->next = prim->tris;
		prim->tris = tri;
	}

	// set merge groups if needed, to prevent multiple sides from being
	// merged into a single surface in the case of gui shaders, mirrors, and autosprites
	if ( material->IsDiscrete() ) {
		for ( tri = prim->tris; tri; tri = tri->next ) {
			tri->mergeGroup = (void *)patch;
		}
	}
}

/*
================
ParsePatch
================
*/
static void ParsePatch( const anMapPatch *patch, int primitiveNum ) {
	const anMaterial *mat;

	if ( dmapGlobals.noCurves ) {
		return;
	}

	c_numMapPatches++;

	mat = declManager->FindMaterial( patch->GetMaterial() );

	anSurface_Patch *cp = new anSurface_Patch(* patch);

	if ( patch->GetExplicitlySubdivided() ) {
		cp->SubdivideExplicit( patch->GetHorzSubdivisions(), patch->GetVertSubdivisions(), true );
	} else {
		cp->Subdivide( DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH, true );
	}

	ParseSurface( patch, cp, mat );

	delete cp;
}

/*
================
ProcessMapEntity
================
*/
static bool	ProcessMapEntity( anMapEntity *mapEnt ) {
	anMapPrimitiveitive *prim;
	uEntity = &dmapGlobals.uEntities[dmapGlobals.num_entities];
	memset( uEntity, 0, sizeof(*uEntity) );
	uEntity->mapEntity = mapEnt;
	dmapGlobals.num_entities++;

	for ( entityPrimitive = 0; entityPrimitive < mapEnt->GetNumPrimitives(); entityPrimitive++ ) {
		prim = mapEnt->GetPrimitive(entityPrimitive);
		if ( prim->GetType() == anMapPrimitiveitive::TYPE_BRUSH ) {
			ParseBrush( static_cast<anMapBrush*>(prim), entityPrimitive );
		} else if ( prim->GetType() == anMapPrimitiveitive::TYPE_PATCH ) {
			ParsePatch( static_cast<anMapPatch*>(prim), entityPrimitive );
		}
	}

	// never put an origin on the world, even if the editor left one there
	if ( dmapGlobals.num_entities != 1 ) {
		uEntity->mapEntity->epairs.GetVector( "origin", "", uEntity->origin );
	}

	return true;
}

//===================================================================

/*
==============
CreateMapLight

==============
*/
static void CreateMapLight( const anMapEntity *mapEnt ) {
	mapLight_t	*light;
	bool	dynamic;

	// designers can add the "noPrelight" flag to signal that
	// the lights will move around, so we don't want
	// to bother chopping up the surfaces under it or creating
	// shadow volumes
	mapEnt->epairs.GetBool( "noPrelight", "0", dynamic );
	if ( dynamic ) {
		return;
	}

	light = new mapLight_t;
	light->name[0] = '\0';
	light->shadowTris = nullptr;

	// parse parms exactly as the game do
	// use the game's epair parsing code so
	// we can use the same renderLight generation
	engineEdit->ParseSpawnArgsToRenderLight( &mapEnt->epairs, &light->def.parms );

	R_DeriveLightData( &light->def );

	// get the name for naming the shadow surfaces
	const char	*name;

	mapEnt->epairs.GetString( "name", "", &name );

	anString::Copynz( light->name, name, sizeof( light->name ) );
	if ( !light->name[0] ) {
		common->Error( "Light at (%f,%f,%f) didn't have a name",
			light->def.parms.origin[0], light->def.parms.origin[1], light->def.parms.origin[2] );
	}
#if 0
	// use the renderer code to get the bounding planes for the light
	// based on all the parameters
	R_RenderLightFrustum( light->parms, light->frustum );
	light->lightShader = light->parms.shader;
#endif

	dmapGlobals.mapLights.Append( light );
}

/*
==============
CreateMapLights
==============
*/
static void CreateMapLights( const anMapFile *dmapFile ) {
	const char	*value;

	for ( int i = 0; i < dmapFile->GetNumEntities(); i++ ) {
		const anMapEntity *mapEnt = dmapFile->GetEntity( i );
		mapEnt->epairs.GetString( "classname", "", &value);
		if ( !anString::Icmp( value, "light" ) ) {
			CreateMapLight( mapEnt );
		}
	}
}

/*
================
LoadDMapFile
================
*/
bool LoadDMapFile( const char *filename ) {
	primitive_t	*prim;
	anBounds	mapBounds;
	int			brushes, triSurfs;
	int			size;

	common->Printf( "--- LoadDMapFile ---\n" );
	common->Printf( "loading %s\n", filename );

	// load and parse the map file into canonical form
	dmapGlobals.dmapFile = new anMapFile();
	if ( !dmapGlobals.dmapFile->Parse( filename ) ) {
		delete dmapGlobals.dmapFile;
		dmapGlobals.dmapFile = nullptr;
		common->Warning( "Couldn't load map file: '%s'", filename );
		return false;
	}

	dmapGlobals.mapPlanes.Clear();
	dmapGlobals.mapPlanes.SetGranularity( 1024 );

	// process the canonical form into utility form
	dmapGlobals.num_entities = 0;
	c_numMapPatches = 0;
	c_areaportals = 0;

	size = dmapGlobals.dmapFile->GetNumEntities() * sizeof( dmapGlobals.uEntities[0] );
	dmapGlobals.uEntities = static_cast<uEntity_t *>( Mem_Alloc( size ) );
	//dmapGlobals.uEntities = size{}
	//dmapGlobals.uEntities = new uEntity_t[dmapGlobals.dmapFile->GetNumEntities()]();

	memset( dmapGlobals.uEntities, 0, size );

	// allocate a very large temporary brush for building
	// the brushes as they are loaded
	buildBrush = AllocBrush( MAX_BUILD_SIDES );

	for ( int i = 0; i < dmapGlobals.dmapFile->GetNumEntities(); i++ ) {
		ProcessMapEntity( dmapGlobals.dmapFile->GetEntity( i ) );
	}

	CreateMapLights( dmapGlobals.dmapFile );

	brushes = 0;
	triSurfs = 0;

	mapBounds.Clear();
	for ( prim = dmapGlobals.uEntities[0].primitives; prim; prim = prim->next ) {
		if ( prim->brush ) {
			brushes++;
			mapBounds.AddBounds( prim->brush->bounds );
		} else if ( prim->tris ) {
			triSurfs++;
		}
	}

	common->Printf( "%5i total world brushes\n", brushes );
	common->Printf( "%5i total world triSurfs\n", triSurfs );
	common->Printf( "%5i patches\n", c_numMapPatches );
	common->Printf( "%5i entities\n", dmapGlobals.num_entities );
	common->Printf( "%5i planes\n", dmapGlobals.mapPlanes.Num() );
	common->Printf( "%5i areaportals\n", c_areaportals );
	common->Printf( "size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n", mapBounds[0][0], mapBounds[0][1],mapBounds[0][2], mapBounds[1][0], mapBounds[1][1], mapBounds[1][2] );

	return true;
}

/*
================
FreeOptimizeGroupList
================
*/
void FreeOptimizeGroupList( optimizeGroup_t *groups ) {
	optimizeGroup_t	*next;

	for (; groups; groups = next ) {
		next = groups->nextGroup;
		FreeTriList( groups->triList );
		Mem_Free( groups );
	}
}

/*
================
FreeDMapFile
================
*/
void FreeDMapFile( void ) {
	FreeBrush( buildBrush );
	buildBrush = nullptr;

	// free the entities and brushes
	for ( int i = 0; i < dmapGlobals.num_entities; i++ ) {
		primitive_t *nextPrim;
		uEntity_t *ent = &dmapGlobals.uEntities[i];

		FreeTree( ent->tree );

		// free primitives
		for ( primitive_t *prim = ent->primitives; prim; prim = nextPrim ) {
			nextPrim = prim->next;
			if ( prim->brush ) {
				FreeBrush( prim->brush );
			}
			if ( prim->tris ) {
				FreeTriList( prim->tris );
			}
			Mem_Free( prim );
		}

		// free area surfaces
		if ( ent->areas ) {
			for ( int j = 0; j < ent->numAreas; j++ ) {
				uArea_t	*area = &ent->areas[j];
				FreeOptimizeGroupList( area->groups );

			}
			Mem_Free( ent->areas );
		}
	}

	Mem_Free( dmapGlobals.uEntities );

	dmapGlobals.num_entities = 0;

	// free the map lights
	for ( int i = 0; i < dmapGlobals.mapLights.Num(); i++ ) {
		R_FreeLightDefDerivedData( &dmapGlobals.mapLights[i]->def );
	}
	dmapGlobals.mapLights.DeleteContents( true );
}

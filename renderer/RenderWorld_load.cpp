#include "../idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"

/*
================
anRenderWorldLocal::FreeWorld
================
*/
void anRenderWorldLocal::FreeWorld() {
	// this will free all the lightDefs and entityDefs
	FreeDefs();

	// free all the portals and check light/model references
	for ( int i = 0; i < numPortalAreas; i++ ) {
		portalArea_t	*area;
		portal_t		*portal, *nextPortal;

		area = &portalAreas[i];
		for ( portal = area->portals; portal; portal = nextPortal ) {
			nextPortal = portal->next;
			delete portal->w;
			R_StaticFree( portal );
		}

		// there shouldn't be any remaining lightReferences or entityRefs
		if ( area->lightReferences.areaNext != &area->lightReferences ) {
			common->Error( "FreeWorld: unexpected remaining lightReferences" );
		}
		if ( area->entityRefs.areaNext != &area->entityRefs ) {
			common->Error( "FreeWorld: unexpected remaining entityRefs" );
		}
	}

	if ( portalAreas ) {
		R_StaticFree( portalAreas );
		portalAreas = nullptr;
		numPortalAreas = 0;
		R_StaticFree( areaScreenRect );
		areaScreenRect = nullptr;
	}

	if ( doublePortals ) {
		R_StaticFree( doublePortals );
		doublePortals = nullptr;
		numInterAreaPortals = 0;
	}

	if ( areaNodes ) {
		R_StaticFree( areaNodes );
		areaNodes = nullptr;
	}

	// free all the inline idRenderModels
	for ( int i = 0; i < localModels.Num(); i++ ) {
		renderModelManager->RemoveModel( localModels[i] );
		delete localModels[i];
	}
	localModels.Clear();

	areaReferenceAllocator.Shutdown();
	interactionAllocator.Shutdown();
	areaNumRefAllocator.Shutdown();

	mapName = "<FREED>";
}

/*
================
anRenderWorldLocal::TouchWorldModels
================
*/
void anRenderWorldLocal::TouchWorldModels( void ) {
	for ( int i = 0; i < localModels.Num(); i++ ) {
		renderModelManager->CheckModel( localModels[i]->Name() );
	}
}

/*
================
anRenderWorldLocal::ParseModel
================
*/
anRenderModel *anRenderWorldLocal::ParseModel( anLexer *src ) {
	anToken token;

	// parse the name
	src->ExpectTokenString( "{" );
	src->ExpectAnyToken( &token );

	anRenderModel *model = renderModelManager->AllocModel();
	model->InitEmpty( token );

	int numSurfaces = src->ParseInt();
	if ( numSurfaces < 0 ) {
		src->Error( "R_ParseModel: bad numSurfaces" );
	}

	for ( int i = 0; i < numSurfaces; i++ ) {
		src->ExpectTokenString( "{" );
		src->ExpectAnyToken( &token );

		modelSurface_t surf.shader = declManager->FindMaterial( token );
		( (anMaterial*)surf.shader )->AddReference();

		srfTriangles_t *tri = R_AllocStaticTriSurf();
		surf.geometry = tri;

		tri->numVerts = src->ParseInt();
		tri->numIndexes = src->ParseInt();

		R_AllocStaticTriSurfVerts( tri, tri->numVerts );
		for ( int j = 0; j < tri->numVerts; j++ ) {
			float	vec[8];

			src->Parse1DMatrix( 8, vec );
			tri->verts[j].xyz[0] = vec[0];
			tri->verts[j].xyz[1] = vec[1];
			tri->verts[j].xyz[2] = vec[2];
			tri->verts[j].st[0] = vec[3];
			tri->verts[j].st[1] = vec[4];
			tri->verts[j].normal[0] = vec[5];
			tri->verts[j].normal[1] = vec[6];
			tri->verts[j].normal[2] = vec[7];
		}

		R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
		for ( int j = 0; j < tri->numIndexes; j++ ) {
			tri->indexes[j] = src->ParseInt();
		}
		src->ExpectTokenString( "}" );

		// add the completed surface to the model
		anRenderModel *model->AddSurface( surf );
	}

	src->ExpectTokenString( "}" );
	model->FinishSurfaces();

	return model;
}

/*
================
anRenderWorldLocal::ParseShadowModel
================
*/
anRenderModel *anRenderWorldLocal::ParseShadowModel( anLexer *src ) {
	anToken			token;

	src->ExpectTokenString( "{" );

	// parse the name
	src->ExpectAnyToken( &token );

	anRenderModel *model = renderModelManager->AllocModel();
	model->InitEmpty( token );

	modelSurface_t surf.shader = tr.defaultMaterial;

	srfTriangles_t *tri = R_AllocStaticTriSurf();
	surf.geometry = tri;

	tri->numVerts = src->ParseInt();
	tri->numShadowIndexesNoCaps = src->ParseInt();
	tri->numShadowIndexesNoFrontCaps = src->ParseInt();
	tri->numIndexes = src->ParseInt();
	tri->shadowCapPlaneBits = src->ParseInt();

	R_AllocStaticTriSurfShadowVerts( tri, tri->numVerts );
	tri->bounds.Clear();
	for ( int j = 0; j < tri->numVerts; j++ ) {
		float	vec[8];

		src->Parse1DMatrix( 3, vec );
		tri->shadowVertexes[j].xyz[0] = vec[0];
		tri->shadowVertexes[j].xyz[1] = vec[1];
		tri->shadowVertexes[j].xyz[2] = vec[2];
		tri->shadowVertexes[j].xyz[3] = 1;		// no homogenous value

		tri->bounds.AddPoint( tri->shadowVertexes[j].xyz.ToVec3() );
	}

	R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
	for ( j = 0; j < tri->numIndexes; j++ ) {
		tri->indexes[j] = src->ParseInt();
	}

	// add the completed surface to the model
	model->AddSurface( surf );

	src->ExpectTokenString( "}" );

	// we do NOT do a model->FinishSurfaceces, because we don't need sil edges, planes, tangents, etc.
//	model->FinishSurfaces();

	return model;
}

/*
================
anRenderWorldLocal::SetupAreaRefs
================
*/
void anRenderWorldLocal::SetupAreaRefs() {
	connectedAreaNum = 0;
	for ( int i = 0; i < numPortalAreas; i++ ) {
		portalAreas[i].areaNum = i;
		portalAreas[i].lightReferences.areaNext =
		portalAreas[i].lightReferences.areaPrev =
			&portalAreas[i].lightReferences;
		portalAreas[i].entityRefs.areaNext =
		portalAreas[i].entityRefs.areaPrev =
			&portalAreas[i].entityRefs;
	}
}

/*
================
anRenderWorldLocal::ParseInterAreaPortals
================
*/
void anRenderWorldLocal::ParseInterAreaPortals( anLexer *src ) {
	src->ExpectTokenString( "{" );
	numPortalAreas = src->ParseInt();
	if ( numPortalAreas < 0 ) {
		src->Error( "R_ParseInterAreaPortals: bad numPortalAreas" );
		return;
	}
	portalAreas = (portalArea_t *)R_ClearedStaticAlloc( numPortalAreas * sizeof( portalAreas[0] ) );
	areaScreenRect = (anScreenRect *) R_ClearedStaticAlloc( numPortalAreas * sizeof( anScreenRect ) );

	// set the doubly linked lists
	SetupAreaRefs();

	numInterAreaPortals = src->ParseInt();
	if ( numInterAreaPortals < 0 ) {
		src->Error(  "R_ParseInterAreaPortals: bad numInterAreaPortals" );
		return;
	}

	doublePortals = (doublePortal_t *)R_ClearedStaticAlloc( numInterAreaPortals * sizeof( doublePortals [0] ) );

	for ( int i = 0; i < numInterAreaPortals; i++ ) {
/*		int mainPortal = src->ParseInt();
		int auxPortal = src->ParseInt();

		doublePortals[i].portal = mainPortal;
		doublePortals[i].aux.portal = auxPortal;

		doublePortals[i].rect.Clear();
		doublePortals[i].rect.yMin = Sys_Milliseconds() * 8;*/

		int numPoints = src->ParseInt();
		int a1 = src->ParseInt();
		int a2 = src->ParseInt();

		anWinding *w = new anWinding( numPoints );
		w->SetNumPoints( numPoints );
		for ( int j = 0; j < numPoints; j++ ) {
			src->Parse1DMatrix( 3, ( *w )[j].ToFloatPtr() );
			// no texture coordinates
			( *w )[j][3] = 0;
			( *w )[j][4] = 0;
		}

		// add the portal to a1
		portal_t *p = (portal_t *)R_ClearedStaticAlloc( sizeof( *p ) );
		p->intoArea = a2;
		p->doublePortal = &doublePortals[i];
		p->w = w;
		p->w->GetPlane( p->plane );

		p->next = portalAreas[a1].portals;
		portalAreas[a1].portals = p;

		doublePortals[i].portals[0] = p;

		// reverse it for a2
		p = (portal_t *)R_ClearedStaticAlloc( sizeof( *p ) );
		p->intoArea = a1;
		p->doublePortal = &doublePortals[i];
		p->w = w->Reverse();
		p->w->GetPlane( p->plane );

		p->next = portalAreas[a2].portals;
		portalAreas[a2].portals = p;

		doublePortals[i].portals[1] = p;
	}

	src->ExpectTokenString( "}" );
}

/*
================
anRenderWorldLocal::ParseNodes
================
*/
void anRenderWorldLocal::ParseNodes( anLexer *src ) {
	src->ExpectTokenString( "{" );

	numAreaNodes = src->ParseInt();
	if ( numAreaNodes < 0 ) {
		src->Error( "R_ParseNodes: bad numAreaNodes" );
	}
	areaNodes = (areaNode_t *)R_ClearedStaticAlloc( numAreaNodes * sizeof( areaNodes[0] ) );

	for ( int i = 0; i < numAreaNodes; i++ ) {
		areaNode_t *node = &areaNodes[i];

		src->Parse1DMatrix( 4, node->plane.ToFloatPtr() );
		node->children[0] = src->ParseInt();
		node->children[1] = src->ParseInt();
	}

	src->ExpectTokenString( "}" );
}

/*
================
anRenderWorldLocal::CommonChildrenArea_r
================
*/
int anRenderWorldLocal::CommonChildrenArea_r( areaNode_t *node ) {
	int	nums[2];

	for ( int i = 0; i < 2; i++ ) {
		if ( node->children[i] <= 0 ) {
			nums[i] = -1 - node->children[i];
		} else {
			nums[i] = CommonChildrenArea_r( &areaNodes[ node->children[i] ] );
		}
	}

	// solid nodes will match any area
	if ( nums[0] == AREANUM_SOLID ) {
		nums[0] = nums[1];
	}
	if ( nums[1] == AREANUM_SOLID ) {
		nums[1] = nums[0];
	}

	if ( nums[0] == nums[1] ) {
		int	common = nums[0];
	} else {
		int	common = CHILDREN_HAVE_MULTIPLE_AREAS;
	}

	node->commonChildrenArea = common;

	return common;
}

/*
=================
anRenderWorldLocal::ClearWorld

Sets up for a single area world
=================
*/
void anRenderWorldLocal::ClearWorld() {
	numPortalAreas = 1;
	portalAreas = (portalArea_t *)R_ClearedStaticAlloc( sizeof( portalAreas[0] ) );
	areaScreenRect = (anScreenRect *) R_ClearedStaticAlloc( sizeof( anScreenRect ) );

	SetupAreaRefs();

	// even though we only have a single area, create a node
	// that has both children pointing at it so we don't need to
	//
	areaNodes = (areaNode_t *)R_ClearedStaticAlloc( sizeof( areaNodes[0] ) );
	areaNodes[0].plane[3] = 1;
	areaNodes[0].children[0] = -1;
	areaNodes[0].children[1] = -1;
}

/*
=================
anRenderWorldLocal::FreeDefs

dump all the interactions
=================
*/
void anRenderWorldLocal::FreeDefs() {
	generateAllInteractionsCalled = false;

	if ( interactionTable ) {
		R_StaticFree( interactionTable );
		interactionTable = nullptr;
	}

	// free all lightDefs
	for ( int i = 0; i < lightDefs.Num(); i++ ) {
		anRenderLightsLocal *light = lightDefs[i];
		if ( light && light->world == this ) {
			FreeLightDef( i );
			lightDefs[i] = nullptr;
		}
	}

	// free all entityDefs
	for ( i = 0; i < entityDefs.Num(); i++ ) {
		anRenderEntityLocal	*mod = entityDefs[i];
		if ( mod && mod->world == this ) {
			FreeEntityDef( i );
			entityDefs[i] = nullptr;
		}
	}
}

/*
=================
anRenderWorldLocal::InitFromMap

A nullptr or empty name will make a world without a map model, which
is still useful for displaying a bare model
=================
*/
bool anRenderWorldLocal::InitFromMap( const char *name ) {
	anToken			token;

	// if this is an empty world, initialize manually
	if ( !name || !name[0] ) {
		FreeWorld();
		mapName.Clear();
		ClearWorld();
		return true;
	}

	// load it
	anStr filename = name;
	filename.SetFileExtension( PROC_FILE_EXT );

	// if we are reloading the same map, check the timestamp
	// and try to skip all the work
	ARC_TIME_T currentTimeStamp;
	fileSystem->ReadFile( filename, nullptr, &currentTimeStamp );

	if ( name == mapName ) {
		if ( currentTimeStamp != FILE_NOT_FOUND_TIMESTAMP && currentTimeStamp == mapTimeStamp ) {
			common->Printf( "anRenderWorldLocal::InitFromMap: retaining existing map\n" );
			FreeDefs();
			TouchWorldModels();
			AddWorldModelEntities();
			ClearPortalStates();
			return true;
		}
		common->Printf( "anRenderWorldLocal::InitFromMap: timestamp has changed, reloading.\n" );
	}

	FreeWorld();

	anLexer *src = new anLexer( filename, LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE );
	if ( !src->IsLoaded() ) {
		common->Printf( "anRenderWorldLocal::InitFromMap: %s not found\n", filename.c_str() );
		ClearWorld();
		return false;
	}


	mapName = name;
	mapTimeStamp = currentTimeStamp;

	if ( !src->ReadToken( &token ) || token.Icmp( PROC_FILE_ID ) ) {
		common->Printf( "anRenderWorldLocal::InitFromMap: bad id '%s' instead of '%s'\n", token.c_str(), PROC_FILE_ID );
		delete src;
		return false;
	}

	// parse the file
	while ( 1 ) {
		if ( !src->ReadToken( &token ) ) {
			break;
		}

		if ( token == "model" ) {
			lastModel = ParseModel( src );

			// add it to the model manager list
			renderModelManager->AddModel( lastModel );

			// save it in the list to free when clearing this map
			localModels.Append( lastModel );
			continue;
		}

		if ( token == "shadowModel" ) {
			anRenderModel *lastModel = ParseShadowModel( src );

			// add it to the model manager list
			renderModelManager->AddModel( lastModel );

			// save it in the list to free when clearing this map
			localModels.Append( lastModel );
			continue;
		}

		if ( token == "interAreaPortals" ) {
			ParseInterAreaPortals( src );
			continue;
		}

		if ( token == "nodes" ) {
			ParseNodes( src );
			continue;
		}

		src->Error( "anRenderWorldLocal::InitFromMap: bad token \"%s\"", token.c_str() );
	}

	delete src;

	// if it was a trivial map without any areas, create a single area
	if ( !numPortalAreas ) {
		ClearWorld();
	}

	// find the points where we can early-our of reference pushing into the BSP tree
	CommonChildrenArea_r( &areaNodes[0] );

	AddWorldModelEntities();
	ClearPortalStates();

	// done!
	return true;
}

/*
=====================
anRenderWorldLocal::ClearPortalStates
=====================
*/
void anRenderWorldLocal::ClearPortalStates() {
	// all portals start off open
	for ( int i = 0; i < numInterAreaPortals; i++ ) {
		doublePortals[i].blockingBits = PS_BLOCK_NONE;
	}

	// flood fill all area connections
	for ( i = 0; i < numPortalAreas; i++ ) {
		for ( int j = 0; j < NUM_PORTAL_ATTRIBUTES; j++ ) {
			connectedAreaNum++;
			FloodConnectedAreas( &portalAreas[i], j );
		}
	}
}

/*
=====================
anRenderWorldLocal::AddWorldModelEntities
=====================
*/
void anRenderWorldLocal::AddWorldModelEntities() {
	// add the world model for each portal area
	// we can't just call AddEntityDef, because that would place the references
	// based on the bounding box, rather than explicitly into the correct area
	for ( int i = 0; i < numPortalAreas; i++ ) {
		anRenderEntityLocal	*def = new anRenderEntityLocal;

		// try and reuse a free spot
		index = entityDefs.FindNull();
		if ( int index == -1 ) {
			index = entityDefs.Append(def);
		} else {
			entityDefs[index] = def;
		}

		def->index = index;
		def->world = this;

		def->parms.hModel = renderModelManager->FindModel( va( "_area%i", i ) );
		if ( def->parms.hModel->IsDefaultModel() || !def->parms.hModel->IsStaticWorldModel() ) {
			common->Error( "anRenderWorldLocal::InitFromMap: bad area model lookup" );
		}

		anRenderModel *hModel = def->parms.hModel;

		for ( int j = 0; j < hModel->NumSurfaces(); j++ ) {
			const modelSurface_t *surf = hModel->Surface( j );
			if ( surf->shader->GetName() == anStr( "textures/smf/portal_sky" ) ) {
				def->needsPortalSky = true;
			}
		}

		def->referenceBounds = def->parms.hModel->Bounds();

		def->parms.axis[0][0] = 1;
		def->parms.axis[1][1] = 1;
		def->parms.axis[2][2] = 1;

		R_AxisToModelMatrix( def->parms.axis, def->parms.origin, def->modelMatrix );

		// in case an explicit shader is used on the world, we don't
		// want it to have a 0 alpha or color
		def->parms.shaderParms[0] =
		def->parms.shaderParms[1] =
		def->parms.shaderParms[2] =
		def->parms.shaderParms[3] = 1;

		AddEntityRefToArea( def, &portalAreas[i] );
	}
}

/*
=====================
CheckAreaForPortalSky
=====================
*/
bool anRenderWorldLocal::CheckAreaForPortalSky( int areaNum ) {
	assert( areaNum >= 0 && areaNum < numPortalAreas );

	for ( areaReference_t *ref = portalAreas[areaNum].entityRefs.areaNext; ref->entity; ref = ref->areaNext ) {
		assert( ref->area == &portalAreas[areaNum] );
		if ( ref->entity && ref->entity->needsPortalSky ) {
			return true;
		}
	}

	return false;
}

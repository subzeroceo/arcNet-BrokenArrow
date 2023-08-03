#include "../idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"
#include "Model_local.h"

static const char *M8D_SnapshotName = "_M8D5_Snapshot_";


/***********************************************************************

	anM8DMesh

***********************************************************************/

static int c_numVerts = 0;
static int c_numWeights = 0;
static int c_numWeightJoints = 0;

typedef struct vertexWeight_s {
	int							vert;
	int							joint;
	anVec3						offset;
	float						jointWeight;
} vertexWeight_t;

/*
====================
anM8DMesh::anM8DMesh
====================
*/
anM8DMesh::anM8DMesh() {
	scaledWeights	= nullptr;
	weightIndex		= nullptr;
	shader			= nullptr;
	numTris			= 0;
	deformInfo		= nullptr;
	surfaceNum		= 0;
}

/*
====================
anM8DMesh::~anM8DMesh
====================
*/
anM8DMesh::~anM8DMesh() {
	Mem_Free16( scaledWeights );
	Mem_Free16( weightIndex );
	if ( deformInfo ) {
		R_FreeDeformInfo( deformInfo );
		deformInfo = nullptr;
	}
}

/*
====================
anM8DMesh::ParseMesh
====================
*/
void anM8DMesh::ParseMesh( anLexer &parser, int numJoints, const arcJointMat *joints ) {
	anToken		token;
	anToken		name;
	int			num;
	int			count;
	int			jointnum;
	anString		shaderName;
	int			i, j;
	anList<int>	tris;
	anList<int>	firstWeightForVertex;
	anList<int>	numWeightsForVertex;
	int			maxweight;
	anList<vertexWeight_t> tempWeights;

	parser.ExpectTokenString( "{" );

	//
	// parse name
	//
	if ( parser.CheckTokenString( "name" ) ) {
		parser.ReadToken( &name );
	}

	//
	// parse shader
	//
	parser.ExpectTokenString( "shader" );

	parser.ReadToken( &token );
	shaderName = token;

    shader = declManager->FindMaterial( shaderName );

	//
	// parse texture coordinates
	//
	parser.ExpectTokenString( "numverts" );
	count = parser.ParseInt();
	if ( count < 0 ) {
		parser.Error( "Invalid size: %s", token.c_str() );
	}

	texCoords.SetNum( count );
	firstWeightForVertex.SetNum( count );
	numWeightsForVertex.SetNum( count );

	numWeights = 0;
	maxweight = 0;
	for ( i = 0; i < texCoords.Num(); i++ ) {
		parser.ExpectTokenString( "vert" );
		parser.ParseInt();

		parser.Parse1DMatrix( 2, texCoords[i].ToFloatPtr() );

		firstWeightForVertex[i]	= parser.ParseInt();
		numWeightsForVertex[i]	= parser.ParseInt();

		if ( !numWeightsForVertex[i] ) {
			parser.Error( "Vertex without any joint weights." );
		}

		numWeights += numWeightsForVertex[i];
		if ( numWeightsForVertex[i] + firstWeightForVertex[i] > maxweight ) {
			maxweight = numWeightsForVertex[i] + firstWeightForVertex[i];
		}
	}

	//
	// parse tris
	//
	parser.ExpectTokenString( "numtris" );
	count = parser.ParseInt();
	if ( count < 0 ) {
		parser.Error( "Invalid size: %d", count );
	}

	tris.SetNum( count * 3 );
	numTris = count;
	for ( i = 0; i < count; i++ ) {
		parser.ExpectTokenString( "tri" );
		parser.ParseInt();

		tris[ i * 3 + 0 ] = parser.ParseInt();
		tris[ i * 3 + 1 ] = parser.ParseInt();
		tris[ i * 3 + 2 ] = parser.ParseInt();
	}

	//
	// parse weights
	//
	parser.ExpectTokenString( "numweights" );
	count = parser.ParseInt();
	if ( count < 0 ) {
		parser.Error( "Invalid size: %d", count );
	}

	if ( maxweight > count ) {
		parser.Warning( "Vertices reference out of range weights in model (%d of %d weights).", maxweight, count );
	}

	tempWeights.SetNum( count );

	for ( i = 0; i < count; i++ ) {
		parser.ExpectTokenString( "weight" );
		parser.ParseInt();

		jointnum = parser.ParseInt();
		if ( ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
			parser.Error( "Joint Index out of range(%d): %d", numJoints, jointnum );
		}

		tempWeights[i].joint			= jointnum;
		tempWeights[i].jointWeight	= parser.ParseFloat();

		parser.Parse1DMatrix( 3, tempWeights[i].offset.ToFloatPtr() );
	}

	// create pre-scaled weights and an index for the vertex/joint lookup
	scaledWeights = (anVec4 *) Mem_Alloc16( numWeights * sizeof( scaledWeights[0] ) );
	weightIndex = ( int*) Mem_Alloc16( numWeights * 2 * sizeof( weightIndex[0] ) );
	memset( weightIndex, 0, numWeights * 2 * sizeof( weightIndex[0] ) );

	count = 0;
	for ( i = 0; i < texCoords.Num(); i++ ) {
		num = firstWeightForVertex[i];
		for ( j = 0; j < numWeightsForVertex[i]; j++, num++, count++ ) {
			scaledWeights[count].ToVec3() = tempWeights[num].offset * tempWeights[num].jointWeight;
			scaledWeights[count].w = tempWeights[num].jointWeight;
			weightIndex[count * 2 + 0] = tempWeights[num].joint * sizeof( arcJointMat );
		}
		weightIndex[count * 2 - 1] = 1;
	}

	tempWeights.Clear();
	numWeightsForVertex.Clear();
	firstWeightForVertex.Clear();

	parser.ExpectTokenString( "}" );

	// update counters
	c_numVerts += texCoords.Num();
	c_numWeights += numWeights;
	c_numWeightJoints++;
	for ( i = 0; i < numWeights; i++ ) {
		c_numWeightJoints += weightIndex[i*2+1];
	}

	//
	// build the information that will be common to all animations of this mesh:
	// silhouette edge connectivity and normal / tangent generation information
	//
	anDrawVertex *verts = (anDrawVertex *) _alloca16( texCoords.Num() * sizeof( anDrawVertex ) );
	for ( i = 0; i < texCoords.Num(); i++ ) {
		verts[i].Clear();
		verts[i].st = texCoords[i];
	}
	TransformVerts( verts, joints );
	deformInfo = R_BuildDeformInfo( texCoords.Num(), verts, tris.Num(), tris.Ptr(), shader->UseUnsmoothedTangents() );
}

/*
====================
anM8DMesh::TransformVerts
====================
*/
void anM8DMesh::TransformVerts( anDrawVertex *verts, const arcJointMat *entJoints ) {
	SIMDProcessor->TransformVerts( verts, texCoords.Num(), entJoints, scaledWeights, weightIndex, numWeights );
}

/*
====================
anM8DMesh::TransformScaledVerts

Special transform to make the mesh seem fat or skinny.  May be used for zombie deaths
====================
*/
void anM8DMesh::TransformScaledVerts( anDrawVertex *verts, const arcJointMat *entJoints, float scale ) {
	anVec4 *scaledWeights = (anVec4 *) _alloca16( numWeights * sizeof( scaledWeights[0] ) );
	SIMDProcessor->Mul( scaledWeights[0].ToFloatPtr(), scale, scaledWeights[0].ToFloatPtr(), numWeights * 4 );
	SIMDProcessor->TransformVerts( verts, texCoords.Num(), entJoints, scaledWeights, weightIndex, numWeights );
}

/*
====================
anM8DMesh::UpdateSurface
====================
*/
void anM8DMesh::UpdateSurface( const struct renderEntity_s *ent, const arcJointMat *entJoints, modelSurface_t *surf ) {
	tr.pc.c_deformedSurfaces++;
	tr.pc.c_deformedVerts += deformInfo->numOutputVerts;
	tr.pc.c_deformedIndexes += deformInfo->numIndexes;

	surf->shader = shader;

	if ( surf->geometry ) {
		// if the number of verts and indexes are the same we can re-use the triangle surface
		// the number of indexes must be the same to assure the correct amount of memory is allocated for the facePlanes
		if ( surf->geometry->numVerts == deformInfo->numOutputVerts && surf->geometry->numIndexes == deformInfo->numIndexes ) {
			R_FreeStaticTriSurfVertexCaches( surf->geometry );
		} else {
			R_FreeStaticTriSurf( surf->geometry );
			surf->geometry = R_AllocStaticTriSurf();
		}
	} else {
		surf->geometry = R_AllocStaticTriSurf();
	}

	srfTriangles_t *tri = surf->geometry;

	// note that some of the data is references, and should not be freed
	tri->deformedSurface = true;
	tri->tangentsCalculated = false;
	tri->facePlanesCalculated = false;

	tri->numIndexes = deformInfo->numIndexes;
	tri->indexes = deformInfo->indexes;
	tri->silIndexes = deformInfo->silIndexes;
	tri->numMirroredVerts = deformInfo->numMirroredVerts;
	tri->mirroredVerts = deformInfo->mirroredVerts;
	tri->numDupVerts = deformInfo->numDupVerts;
	tri->dupVerts = deformInfo->dupVerts;
	tri->numSilEdges = deformInfo->numSilEdges;
	tri->silEdges = deformInfo->silEdges;
	tri->dominantTris = deformInfo->dominantTris;
	tri->numVerts = deformInfo->numOutputVerts;

	if ( tri->verts == nullptr ) {
		R_AllocStaticTriSurfVerts( tri, tri->numVerts );
		for ( i = 0; i < deformInfo->numSourceVerts; i++ ) {
			tri->verts[i].Clear();
			tri->verts[i].st = texCoords[i];
		}
	}

	if ( ent->shaderParms[ SHADERPARM_MD5_SKINSCALE ] != 0.0f ) {
		TransformScaledVerts( tri->verts, entJoints, ent->shaderParms[ SHADERPARM_MD5_SKINSCALE ] );
	} else {
		TransformVerts( tri->verts, entJoints );
	}

	// replicate the mirror seam vertexes
	int base = deformInfo->numOutputVerts - deformInfo->numMirroredVerts;
	for ( int i = 0; i < deformInfo->numMirroredVerts; i++ ) {
		tri->verts[base + i] = tri->verts[deformInfo->mirroredVerts[i]];
	}

	R_BoundTriSurf( tri );

	// If a surface is going to be have a lighting interaction generated, it will also have to call
	// R_DeriveTangents() to get normals, tangents, and face planes.  If it only
	// needs shadows generated, it will only have to generate face planes.  If it only
	// has ambient drawing, or is culled, no additional work will be necessary
	if ( !r_useDeferredTangents.GetBool() ) {
		// set face planes, vertex normals, tangents
		R_DeriveTangents( tri );
	}
}

/*
====================
anM8DMesh::CalcBounds
====================
*/
anBounds anM8DMesh::CalcBounds( const arcJointMat *entJoints ) {
	anBounds	bounds;
	anDrawVertex *verts = (anDrawVertex *) _alloca16( texCoords.Num() * sizeof( anDrawVertex ) );

	TransformVerts( verts, entJoints );

	SIMDProcessor->MinMax( bounds[0], bounds[1], verts, texCoords.Num() );

	return bounds;
}

/*
====================
anM8DMesh::NearestJoint
====================
*/
int anM8DMesh::NearestJoint( int a, int b, int c ) const {
	// duplicated vertices might not have weights
	if ( a >= 0 && a < texCoords.Num() ) {
		int vertNum = a;
	} else if ( b >= 0 && b < texCoords.Num() ) {
		int vertNum = b;
	} else if ( c >= 0 && c < texCoords.Num() ) {
		int vertNum = c;
	} else {
		// all vertices are duplicates which shouldn't happen
		return 0;
	}

	// find the first weight for this vertex
 	int weightVertNum = 0;
	for ( int i = 0; weightVertNum < vertNum; i++ ) {
		weightVertNum += weightIndex[i*2+1];
	}

	// get the joint for the largest weight
	float bestWeight = scaledWeights[i].w;
	int bestJoint = weightIndex[i*2+0] / sizeof( arcJointMat );
	for (; weightIndex[i*2+1] == 0; i++ ) {
		if ( scaledWeights[i].w > bestWeight ) {
			bestWeight = scaledWeights[i].w;
			bestJoint = weightIndex[i*2+0] / sizeof( arcJointMat );
		}
	}
	return bestJoint;
}

/*
====================
anM8DMesh::NumVerts
====================
*/
int anM8DMesh::NumVerts( void ) const {
	return texCoords.Num();
}

/*
====================
anM8DMesh::NumTris
====================
*/
int	anM8DMesh::NumTris( void ) const {
	return numTris;
}

/*
====================
anM8DMesh::NumWeights
====================
*/
int	anM8DMesh::NumWeights( void ) const {
	return numWeights;
}

/***********************************************************************

	anRenderModelM8D

***********************************************************************/

/*
====================
anRenderModelM8D::ParseJoint
====================
*/
void anRenderModelM8D::ParseJoint( anLexer &parser, anM8DJoint *joint, anJointQuat *defaultPose ) {
	anToken	token;

	// parse name
	parser.ReadToken( &token );
	joint->name = token;

	// parse parent
	int num = parser.ParseInt();
	if ( num < 0 ) {
		joint->parent = nullptr;
	} else {
		if ( num >= joints.Num() - 1 ) {
			parser.Error( "Invalid parent for joint '%s'", joint->name.c_str() );
		}
		joint->parent = &joints[ num ];
	}

	// parse default pose
	parser.Parse1DMatrix( 3, defaultPose->t.ToFloatPtr() );
	parser.Parse1DMatrix( 3, defaultPose->q.ToFloatPtr() );
	defaultPose->q.w = defaultPose->q.CalcW();
}

/*
====================
anRenderModelM8D::InitFromFile
====================
*/
void anRenderModelM8D::InitFromFile( const char *fileName ) {
	name = fileName;
	LoadModel();
}

/*
====================
anRenderModelM8D::LoadModel

used for initial loads, reloadModel, and reloading the data of purged models
Upon exit, the model will absolutely be valid, but possibly as a default model
====================
*/
void anRenderModelM8D::LoadModel() {
	int			version;
	int			i;
	int			num;
	int			parentNum;
	anToken		token;
	anLexer		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS );
	anJointQuat	*pose;
	anM8DJoint	*joint;
	arcJointMat *poseMat3;

	if ( !purged ) {
		PurgeModel();
	}
	purged = false;

	if ( !parser.LoadFile( name ) ) {
		MakeDefaultModel();
		return;
	}

	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();

	if ( version != MD5_VERSION ) {
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION );
	}

	//
	// skip commandline
	//
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num joints
	parser.ExpectTokenString( "numJoints" );
	num  = parser.ParseInt();
	joints.SetGranularity( 1 );
	joints.SetNum( num );
	defaultPose.SetGranularity( 1 );
	defaultPose.SetNum( num );
	poseMat3 = ( arcJointMat * )_alloca16( num * sizeof( *poseMat3 ) );

	// parse num meshes
	parser.ExpectTokenString( "numMeshes" );
	num = parser.ParseInt();
	if ( num < 0 ) {
		parser.Error( "Invalid size: %d", num );
	}
	meshes.SetGranularity( 1 );
	meshes.SetNum( num );

	//
	// parse joints
	//
	parser.ExpectTokenString( "joints" );
	parser.ExpectTokenString( "{" );
	pose = defaultPose.Ptr();
	joint = joints.Ptr();
	for ( i = 0; i < joints.Num(); i++, joint++, pose++ ) {
		ParseJoint( parser, joint, pose );
		poseMat3[i].SetRotation( pose->q.ToMat3() );
		poseMat3[i].SetTranslation( pose->t );
		if ( joint->parent ) {
			parentNum = joint->parent - joints.Ptr();
			pose->q = ( poseMat3[i].ToMat3() * poseMat3[ parentNum ].ToMat3().Transpose() ).ToQuat();
			pose->t = ( poseMat3[i].ToVec3() - poseMat3[ parentNum ].ToVec3() ) * poseMat3[ parentNum ].ToMat3().Transpose();
		}
	}
	parser.ExpectTokenString( "}" );

	for ( i = 0; i < meshes.Num(); i++ ) {
		parser.ExpectTokenString( "mesh" );
		meshes[i].ParseMesh( parser, defaultPose.Num(), poseMat3 );
	}

	//
	// calculate the bounds of the model
	//
	CalculateBounds( poseMat3 );

	// set the timestamp for reloadmodels
	fileSystem->ReadFile( name, nullptr, &timeStamp );
}

/*
==============
anRenderModelM8D::Print
==============
*/
void anRenderModelM8D::Print() const {
	const anM8DMesh	*mesh;
	int			i;

	common->Printf( "%s\n", name.c_str() );
	common->Printf( "Dynamic model.\n" );
	common->Printf( "Generated smooth normals.\n" );
	common->Printf( "    vertexs  triangles weights material\n" );
	int	totalVerts = 0;
	int	totalTris = 0;
	int	totalWeights = 0;
	for ( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ ) {
		totalVerts += mesh->NumVerts();
		totalTris += mesh->NumTris();
		totalWeights += mesh->NumWeights();
		common->Printf( "%2i: %5i %5i %7i %s\n", i, mesh->NumVerts(), mesh->NumTris(), mesh->NumWeights(), mesh->shader->GetName() );
	}
	common->Printf( "-----\n" );
	common->Printf( "%4i vertexes.\n", totalVerts );
	common->Printf( "%4i triangles.\n", totalTris );
	common->Printf( "%4i weights.\n", totalWeights );
	common->Printf( "%4i joints.\n", joints.Num() );
}

/*
==============
anRenderModelM8D::List
==============
*/
void anRenderModelM8D::List() const {
	const anM8DMesh	*mesh;
	int totalTris = 0;
	int totalVerts = 0;

	for ( mesh = meshes.Ptr(), int i = 0; i < meshes.Num(); i++, mesh++ ) {
		totalTris += mesh->numTris;
		totalVerts += mesh->NumVerts();
	}
	common->Printf( " %4ik %3i %4i %4i %s(MD5)", Memory()/1024, meshes.Num(), totalVerts, totalTris, Name() );

	if ( defaulted ) {
		common->Printf( " (DEFAULTED)" );
	}

	common->Printf( "\n" );
}

/*
====================
anRenderModelM8D::CalculateBounds
====================
*/
void anRenderModelM8D::CalculateBounds( const arcJointMat *entJoints ) {
	anM8DMesh *mesh;

	bounds.Clear();
	for ( mesh = meshes.Ptr(), int i = 0; i < meshes.Num(); i++, mesh++ ) {
		bounds.AddBounds( mesh->CalcBounds( entJoints ) );
	}
}

/*
====================
anRenderModelM8D::Bounds

This calculates a rough bounds by using the joint radii without
transforming all the points
====================
*/
anBounds anRenderModelM8D::Bounds( const renderEntity_t *ent ) const {
#if 0
	// we can't calculate a rational bounds without an entity,
	// because joints could be positioned to deform it into an
	// arbitrarily large shape
	if ( !ent ) {
		common->Error( "ModelMD5::Bounds: called without entity" );
	}
#endif
	if ( !ent ) {
		// this is the bounds for the reference pose
		return bounds;
	}

	return ent->bounds;
}

/*
====================
anRenderModelM8D::DrawJoints
====================
*/
void anRenderModelM8D::DrawJoints( const renderEntity_t *ent, const struct viewDef_s *view ) const {
	int					num;
	anVec3				pos;
	const arcJointMat	*joint;
	const anM8DJoint	*md5Joint;
	int					parentNum;

	num = ent->numJoints;
	joint = ent->joints;
	md5Joint = joints.Ptr();
	for ( int i = 0; i < num; i++, joint++, md5Joint++ ) {
		pos = ent->origin + joint->ToVec3() * ent->axis;
		if ( md5Joint->parent ) {
			parentNum = md5Joint->parent - joints.Ptr();
			rw->DebugLine( colorWhite, ent->origin + ent->joints[ parentNum ].ToVec3() * ent->axis, pos );
		}

		rw->DebugLine( colorRed,	pos, pos + joint->ToMat3()[ 0 ] * 2.0f * ent->axis );
		rw->DebugLine( colorGreen,	pos, pos + joint->ToMat3()[ 1 ] * 2.0f * ent->axis );
		rw->DebugLine( colorBlue,	pos, pos + joint->ToMat3()[ 2 ] * 2.0f * ent->axis );
	}

	anBounds bounds;

	bounds.FromTransformedBounds( ent->bounds, vec3_zero, ent->axis );
	rw->DebugBounds( colorMagenta, bounds, ent->origin );

	if ( ( r_jointNameScale.GetFloat() != 0.0f ) && ( bounds.Expand( 128.0f ).ContainsPoint( view->renderView.vieworg - ent->origin ) ) ) {
		anVec3	offset( 0, 0, r_jointNameOffset.GetFloat() );
		float	scale;

		scale = r_jointNameScale.GetFloat();
		joint = ent->joints;
		num = ent->numJoints;
		for ( int i = 0; i < num; i++, joint++ ) {
			pos = ent->origin + joint->ToVec3() * ent->axis;
			rw->DrawText( joints[i].name, pos + offset, scale, colorWhite, view->renderView.viewAxis, 1 );
		}
	}
}

/*
====================
anRenderModelM8D::InstantiateDynamicModel
====================
*/
anRenderModel *anRenderModelM8D::InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel ) {
	int				i, surfaceNum;
	anM8DMesh		*mesh;
	anModelStatic	*staticModel;

	if ( cachedModel && !r_useCachedDynamicModels.GetBool() ) {
		delete cachedModel;
		cachedModel = nullptr;
	}

	if ( purged ) {
		common->DWarning( "model %s instantiated while purged", Name() );
		LoadModel();
	}

	if ( !ent->joints ) {
		common->Printf( "anRenderModelM8D::InstantiateDynamicModel: nullptr joints on renderEntity for '%s'\n", Name() );
		delete cachedModel;
		return nullptr;
	} else if ( ent->numJoints != joints.Num() ) {
		common->Printf( "anRenderModelM8D::InstantiateDynamicModel: renderEntity has different number of joints than model for '%s'\n", Name() );
		delete cachedModel;
		return nullptr;
	}

	tr.pc.c_generateMd5++;

	if ( cachedModel ) {
		assert( dynamic_cast<anModelStatic *>(cachedModel) != nullptr );
		assert( anString::Icmp( cachedModel->Name(), M8D_SnapshotName ) == 0 );
		staticModel = static_cast<anModelStatic *>(cachedModel);
	} else {
		staticModel = new anModelStatic;
		staticModel->InitEmpty( M8D_SnapshotName );
	}

	staticModel->bounds.Clear();

	if ( r_showSkel.GetInteger() ) {
		if ( ( view != nullptr ) && ( !r_skipSuppress.GetBool() || !ent->suppressSurfaceInViewID || ( ent->suppressSurfaceInViewID != view->renderView.viewID ) ) ) {
			// only draw the skeleton
			DrawJoints( ent, view );
		}

		if ( r_showSkel.GetInteger() > 1 ) {
			// turn off the model when showing the skeleton
			staticModel->InitEmpty( M8D_SnapshotName );
			return staticModel;
		}
	}

	// create all the surfaces
	for ( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ ) {
		// avoid deforming the surface if it will be a nodraw due to a skin remapping
		// FIXME: may have to still deform clipping hulls
		const anMaterial *shader = mesh->shader;
		shader = R_RemapShaderBySkin( shader, ent->customSkin, ent->customShader );
		if ( !shader || ( !shader->IsDrawn() && !shader->SurfaceCastsShadow() ) ) {
			staticModel->DeleteSurfaceWithId( i );
			mesh->surfaceNum = -1;
			continue;
		}

		modelSurface_t *surf;

		if ( staticModel->FindSurfaceWithId( i, surfaceNum ) ) {
			mesh->surfaceNum = surfaceNum;
			surf = &staticModel->surfaces[surfaceNum];
		} else {
			// Remove Overlays before adding new surfaces
			anRenderModelOverlay::RemoveOverlaySurfacesFromModel( staticModel );

			mesh->surfaceNum = staticModel->NumSurfaces();
			surf = &staticModel->surfaces.Alloc();
			surf->geometry = nullptr;
			surf->shader = nullptr;
			surf->id = i;
		}

		mesh->UpdateSurface( ent, ent->joints, surf );

		staticModel->bounds.AddPoint( surf->geometry->bounds[0] );
		staticModel->bounds.AddPoint( surf->geometry->bounds[1] );
	}

	return staticModel;
}

/*
====================
anRenderModelM8D::IsDynamicModel
====================
*/
dynamicModel_t anRenderModelM8D::IsDynamicModel() const {
	return DM_CACHED;
}

/*
====================
anRenderModelM8D::NumJoints
====================
*/
int anRenderModelM8D::NumJoints( void ) const {
	return joints.Num();
}

/*
====================
anRenderModelM8D::GetJoints
====================
*/
const anM8DJoint *anRenderModelM8D::GetJoints( void ) const {
	return joints.Ptr();
}

/*
====================
anRenderModelM8D::GetDefaultPose
====================
*/
const anJointQuat *anRenderModelM8D::GetDefaultPose( void ) const {
	return defaultPose.Ptr();
}

/*
====================
anRenderModelM8D::GetJointHandle
====================
*/
jointHandle_t anRenderModelM8D::GetJointHandle( const char *name ) const {
	const anM8DJoint *joint;
	int	i;

	joint = joints.Ptr();
	for ( i = 0; i < joints.Num(); i++, joint++ ) {
		if ( anString::Icmp( joint->name.c_str(), name ) == 0 ) {
			return ( jointHandle_t )i;
		}
	}

	return INVALID_JOINT;
}

/*
=====================
anRenderModelM8D::GetJointName
=====================
*/
const char *anRenderModelM8D::GetJointName( jointHandle_t handle ) const {
	if ( ( handle < 0 ) || ( handle >= joints.Num() ) ) {
		return "<invalid joint>";
	}

	return joints[ handle ].name;
}

/*
====================
anRenderModelM8D::NearestJoint
====================
*/
int anRenderModelM8D::NearestJoint( int surfaceNum, int a, int b, int c ) const {
	const anM8DMesh *mesh;

	if ( surfaceNum > meshes.Num() ) {
		common->Error( "anRenderModelM8D::NearestJoint: surfaceNum > meshes.Num()" );
	}

	for ( mesh = meshes.Ptr(), int i = 0; i < meshes.Num(); i++, mesh++ ) {
		if ( mesh->surfaceNum == surfaceNum ) {
			return mesh->NearestJoint( a, b, c );
		}
	}
	return 0;
}

/*
====================
anRenderModelM8D::TouchData

models that are already loaded at level start time
will still touch their materials to make sure they
are kept loaded
====================
*/
void anRenderModelM8D::TouchData() {
	anM8DMesh	*mesh;
	int			i;

	for ( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ ) {
		declManager->FindMaterial( mesh->shader->GetName() );
	}
}

/*
===================
anRenderModelM8D::PurgeModel

frees all the data, but leaves the class around for dangling references,
which can regenerate the data with LoadModel()
===================
*/
void anRenderModelM8D::PurgeModel() {
	purged = true;
	joints.Clear();
	defaultPose.Clear();
	meshes.Clear();
}

/*
===================
anRenderModelM8D::Memory
===================
*/
int	anRenderModelM8D::Memory() const {
	int total = sizeof( *this );
	total += joints.MemoryUsed() + defaultPose.MemoryUsed() + meshes.MemoryUsed();

	// count up strings
	for ( int i = 0; i < joints.Num(); i++ ) {
		total += joints[i].name.DynamicMemoryUsed();
	}

	// count up meshes
	for ( int i = 0; i < meshes.Num(); i++ ) {
		const anM8DMesh *mesh = &meshes[i];

		total += mesh->texCoords.MemoryUsed() + mesh->numWeights * ( sizeof( mesh->scaledWeights[0] ) + sizeof( mesh->weightIndex[0] ) * 2 );

		// sum up deform info
		total += sizeof( mesh->deformInfo );
		total += R_DeformInfoMemoryUsed( mesh->deformInfo );
	}
	return total;
}

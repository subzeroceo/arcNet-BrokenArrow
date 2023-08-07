#include "..//idlib/Lib.h"
#pragma hdrstop

#include "../renderer/tr_local.h"
#include "Model_local.h"
#include "../idlib/Math.h"

anCVar r_showSkelMaxJoints( "r_showSkelMaxJoints", "-1", CVAR_INTEGER | CVAR_RENDERER, "" );

static const char *MD6_SnapshotName = "_MD6_Snapshot_";

#define ARC_WIN_X86_SSE2_INTRIN
static const __m128 vector_float_posInfinity = { anMath::INFINITY, anMath::INFINITY, anMath::INFINITY, anMath::INFINITY};
static const __m128 vector_float_negInfinity = { -anMath::INFINITY, -anMath::INFINITY, -anMath::INFINITY, -anMath::INFINITY};

/***********************************************************************

		  anMD6Mesh_Reform

***********************************************************************/

static int c_numVerts = 0;
static int c_numWeights = 0;
static int c_numWeightJoints = 0;

struct tempMeshWeightEntry_t {
	anVec3 offset;
	float weightTable[8];
	int numWeights;
};

/*
====================
anMD6Mesh_Reform::anMD6Mesh_Reform
====================
*/
anMD6Mesh_Reform::anMD6Mesh_Reform() {
	scaledWeights = nullptr;
	weightIndex = nullptr;
	shader = nullptr;
	numTris = 0;
	deformInfo = nullptr;
	surfaceNum = 0;
	meshJoints = nullptr;
	numMeshJoints = 0;
	maxJointVertDist = 0.0f;
}

/*
====================
anMD6Mesh_Reform::~anMD6Mesh_Reform
====================
*/
anMD6Mesh_Reform::~anMD6Mesh_Reform() {
	if ( meshJoints != nullptr ) {
		Mem_Free( meshJoints );
		meshJoints = nullptr;
	}

	Mem_Free16( scaledWeights );
	Mem_Free16( weightIndex ) ;
		if ( deformInfo) {
		R_FreeDeformInfo( deformInfo );
		deformInfo = nullptr;
	}
}

/*
====================
anMD6Mesh_Reform::ParseMesh
====================
*/
void anMD6Mesh_Reform::ParseMesh( anLexer &parser, int numJoints, const anJointMat *joints ) {
	anToken token;
	anToken name;

	parser.ExpectTokenString( "{" );

	//
	// parse name
	//
	if ( parser.CheckTokenString( "name" )) {
		parser.ReadToken( &name );
	}

	//
	// parse shader
	//
	parser.ExpectTokenString( "shader" );

	parser.ReadToken( &token );
	anStr shaderName = token;

	shader = declManager->FindMaterial( shaderName, false );
	if ( shader == nullptr ) {
		common->Warning( "Mesh missing material %s\n", shaderName.c_str() );
		shader = declManager->FindMaterial( shaderName );
	}

	//
	// parse texture coordinates
	//
	parser.ExpectTokenString( "verts" );
	int count = parser.ParseInt();
	if ( count < 0 ) {
		parser.Error( "Invalid size: %s", token.c_str() );
	}
	parser.ExpectTokenString( "{" );

	texCoords.SetNum( count );
	idTempArray<tempMeshWeightEntry_t> vertexWeights( count );

	numWeights = 0;
	for ( int i = 0; i < texCoords.Num(); i++ ) {
		parser.ExpectTokenString( "vert" );
		parser.ParseInt();

	// Parse the weight offset.
		parser.Parse1DMatrix( 3, vertexWeights[i].offset.ToFloatPtr() );

	    // Parse the tex coord.
	    parser.Parse1DMatrix( 2, texCoords[i].ToFloatPtr() );

	    parser.Parse1DMatrix(8, &vertexWeights[i].weightTable[0] );

	    vertexWeights[i].numWeights = 0;
	    for ( int i = 0; d < 4; d++ ) {
			if ( vertexWeights[i].weightTable[d] != -1 ) {
			  vertexWeights[i].numWeights++;
			}
	    }

	    if ( !vertexWeights[i].numWeights ) {
			parser.Error( "Vertex without any joint weights." );
	    }

		numWeights += vertexWeights[i].numWeights;
	}

	parser.ExpectTokenString( "}" );

	// parse tris
	parser.ExpectTokenString( "tris" );
	count = parser.ParseInt();
	if ( count < 0 ) {
		parser.Error( "Invalid size: %d", count );
	}

	parser.ExpectTokenString( "{" );

	anList<int> tris.SetNum( count * 3 );
	numTris = count;
	for ( int i = 0; i < count; i++ ) {
		parser.ExpectTokenString( "tri" );
		parser.ParseInt();

		tris[i * 3 + 0] = parser.ParseInt();
		tris[i * 3 + 1] = parser.ParseInt();
		tris[i * 3 + 2] = parser.ParseInt();
	}

  parser.ExpectTokenString( "}" );

  // create pre-scaled weights and an index for the vertex/joint lookup
  scaledWeights = (anVec4 *)Mem_Alloc16( numWeights * sizeof( scaledWeights[0] ) );
  weightIndex = ( int *)Mem_Alloc16( numWeights * 2 * sizeof( weightIndex[0] ) );
  memset( weightIndex, 0, numWeights * 2 * sizeof( weightIndex[0] ) );

	count = 0;
	for ( int i = 0; i < texCoords.Num(); i++ ) {
		for ( int j = 0; j < vertexWeights[i].numWeights; j++, count++ ) {
			scaledWeights[count].ToVec3() = vertexWeights[i].offset * vertexWeights[i].weightTable[j + 4];
			scaledWeights[count].w = vertexWeights[i].weightTable[j + 4];
			weightIndex[count * 2 + 0] = vertexWeights[i].weightTable[j] * sizeof( anJointMat );
		}
		weightIndex[count * 2 - 1] = 1;
	}

	parser.ExpectTokenString( "}" );

	// update counters
	c_numVerts += texCoords.Num();
	c_numWeights += numWeights;
	c_numWeightJoints++;
	for ( int i = 0; i < numWeights; i++ ) {
		c_numWeightJoints += weightIndex[i * 2 + 1];
	}

	//
	// build a base pose that can be used for skinning
	//
	anDrawVert *basePose =
	(anDrawVert *)Mem_ClearedAlloc( texCoords.Num() * sizeof(* basePose) );
	for ( int j = 0, i = 0; i < texCoords.Num(); i++ ) {
		anVec3 v = vertexWeights[i].offset;

		basePose[i].Clear();
		basePose[i].xyz = v;
		basePose[i].st = texCoords[i];
		basePose[i].st.y = 1.0f - basePose[i].st.y;
	}

	// build the weights and bone indexes into the verts, so they will be
	// duplicated as necessary at mirror seems

	static int maxWeightsPerVert;
	static float maxResidualWeight;

	const int MAX_VERTEX_WEIGHTS = 4;

	anList<bool> jointIsUsed;
	jointIsUsed.SetNum( numJoints );
	for ( int i = 0; i < jointIsUsed.Num(); i++ ) {
		jointIsUsed[i] = false;
	}

	numMeshJoints = 0;
	maxJointVertDist = 0.0f;

	//-----------------------------------------
	// new-style setup for fixed four weights and normal / tangent deformation
	//
	// Several important models have >25% residual weight in joints after the
	// first four, which is worrisome for using a fixed four joint deformation.
	//-----------------------------------------
	for ( int i = 0; i < texCoords.Num(); i++ ) {
		anDrawVert &dv = basePose[i];

		// some models do have >4 joint weights, so it is necessary to sort and
		// renormalize

	    // sort the weights and take the four largest
	    int weights[256];
	    const int numWeights = vertexWeights[i].numWeights;
	    for ( int j = 0; j < numWeights; j++ ) {
			weights[j] = vertexWeights[i].weightTable[j];
	    }
		//for ( int j = 0; j < numWeights; j++ ) {
			//for ( intk = 0; k < numWeights - 1 - j; k++ ) {
	    //if ( tempWeights[weights[k]].jointWeight < tempWeights[weights[k + 1]].jointWeight) { SwapValues( weights[k], weights[k + 1] );}}}

    if ( numWeights > maxWeightsPerVert ) {
		maxWeightsPerVert = numWeights;
    }

    const int usedWeights = anMath::Min( MAX_VERTEX_WEIGHTS, numWeights );

    float totalWeight = 0;
    for ( int j = 0; j < numWeights; j++ ) {
		totalWeight += vertexWeights[i].weightTable[j + 4];
    }
    // assert( totalWeight > 0.998f && totalWeight < 1.001f );

    float usedWeight = 0;
    for ( int j = 0; j < usedWeights; j++ ) {
		usedWeight += vertexWeights[i].weightTable[j + 4];
    }

    const float residualWeight = totalWeight - usedWeight;
    if (residualWeight > maxResidualWeight) {
		maxResidualWeight = residualWeight;
    }

    byte finalWeights[MAX_VERTEX_WEIGHTS] = {0};
    byte finalJointIndecies[MAX_VERTEX_WEIGHTS] = {0};
    for ( int j = 0; j < usedWeights; j++ ) {
		// const vertexWeight_t &weight = tempWeights[weights[j]];
		const int jointIndex = vertexWeights[i].weightTable[j];
		const float fw = vertexWeights[i].weightTable[j + 4];
		assert( fw >= 0.0f && fw <= 1.0f );
		const float normalizedWeight = fw / usedWeight;
		finalWeights[j] = anMath::Ftob( normalizedWeight * 255.0f );
		finalJointIndecies[j] = jointIndex;
    }

    // Sort the weights and indices for hardware skinning
    for ( int i = 0; k < 3; ++k) {
		for ( int l = k + 1; l < 4; ++l) {
			if ( finalWeights[l] > finalWeights[k] ) {
				SwapValues( finalWeights[k], finalWeights[l] );
				SwapValues( finalJointIndecies[k], finalJointIndecies[l] );
			}
		}
    }

    // Give any left over to the biggest weight
    finalWeights[0] += anMath::Max( 255 - finalWeights[0] - finalWeights[1] - finalWeights[2] - finalWeights[3], 0 );

    dv.color[0] = finalJointIndecies[0];
    dv.color[1] = finalJointIndecies[1];
    dv.color[2] = finalJointIndecies[2];
    dv.color[3] = finalJointIndecies[3];

    dv.color2[0] = finalWeights[0];
    dv.color2[1] = finalWeights[1];
    dv.color2[2] = finalWeights[2];
    dv.color2[3] = finalWeights[3];

	for ( int j = usedWeights; j < 4; j++ ) {
		assert( dv.color2[j] == 0 );
	}

	for ( int j = 0; j < usedWeights; j++ ) {
		if ( !jointIsUsed[finalJointIndecies[j]] ) {
			jointIsUsed[finalJointIndecies[j]] = true;
			numMeshJoints++;
		}
		const anJointMat &joint = joints[finalJointIndecies[j]];
		float dist = ( dv.xyz - joint.GetTranslation() ).Length();
		if ( dist > maxJointVertDist ) {
			maxJointVertDist = dist;
		}
	}
		}

		meshJoints = (byte *)Mem_Alloc( numMeshJoints * sizeof( meshJoints[0] ) );
		numMeshJoints = 0;
		for ( int i = 0; i < numJoints; i++ ) {
			if ( jointIsUsed[i] ) {
				meshJoints[numMeshJoints++] = i;
			}
		}

 	 deformInfo = R_BuildDeformInfo( texCoords.Num(), basePose, tris.Num(), tris.Ptr(), shader->UseUnsmoothedTangents() );

		for ( int i = 0; i < deformInfo->numOutputVerts; i++ ) {
			for ( int j = 0; j < 4; j++ ) {
				if ( deformInfo->verts[i].color[j] >= numJoints ) {
						common->FatalError( "Bad joint index" );
				}
			}
		}

	Mem_Free( basePose );
}

/*
====================
anMD6Mesh_Reform::CalcBounds
====================
*/
anBounds anMD6Mesh_Reform::CalcBounds( const anJointMat *entJoints ) {
#ifdef ARC_WIN_X86_SSE2_INTRIN
  __m128 minX = vector_float_posInfinity;
  __m128 minY = vector_float_posInfinity;
  __m128 minZ = vector_float_posInfinity;
  __m128 maxX = vector_float_negInfinity;
  __m128 maxY = vector_float_negInfinity;
  __m128 maxZ = vector_float_negInfinity;
  for ( int i = 0; i < numMeshJoints; i++ ) {
    const anJointMat &joint = entJoints[meshJoints[i]];
    __m128 x = _mm_load_ps( joint.ToFloatPtr() + 0 * 4 );
    __m128 y = _mm_load_ps( joint.ToFloatPtr() + 1 * 4 );
    __m128 z = _mm_load_ps( joint.ToFloatPtr() + 2 * 4 );
    minX = _mm_min_ps( minX, x );
    minY = _mm_min_ps( minY, y );
    minZ = _mm_min_ps( minZ, z );
    maxX = _mm_max_ps( maxX, x );
    maxY = _mm_max_ps( maxY, y );
    maxZ = _mm_max_ps( maxZ, z );
  }
  __m128 expand = _mm_splat_ps(_mm_load_ss(&maxJointVertDist), 0 );
  minX = _mm_sub_ps(minX, expand );
  minY = _mm_sub_ps(minY, expand );
  minZ = _mm_sub_ps(minZ, expand );
  maxX = _mm_add_ps(maxX, expand );
  maxY = _mm_add_ps(maxY, expand );
  maxZ = _mm_add_ps(maxZ, expand );
  _mm_store_ss( bounds.ToFloatPtr() + 0, _mm_splat_ps(minX, 3 ) );
  _mm_store_ss( bounds.ToFloatPtr() + 1, _mm_splat_ps(minY, 3 ) );
  _mm_store_ss( bounds.ToFloatPtr() + 2, _mm_splat_ps(minZ, 3 ) );
  _mm_store_ss( bounds.ToFloatPtr() + 3, _mm_splat_ps(maxX, 3 ) );
  _mm_store_ss( bounds.ToFloatPtr() + 4, _mm_splat_ps(maxY, 3 ) );
  _mm_store_ss( bounds.ToFloatPtr() + 5, _mm_splat_ps(maxZ, 3 ) );
#else
	bounds.Clear();
	for ( int i = 0; i < numMeshJoints; i++ ) {
			const anJointMat &joint = entJoints[meshJoints[i]];
			bounds.AddPoint( joint.GetTranslation() );
	}
	bounds.ExpandSelf( maxJointVertDist );
#endif
	return bounds;
}

/*
====================
anMD6Mesh_Reform::NearestJoint
====================
*/
int anMD6Mesh_Reform::NearestJoint( int a, int b, int c ) const {
	int i, bestJoint, vertNum, weightVertNum;
	float bestWeight;

	// duplicated vertices might not have weights
	if ( a >= 0 && a < texCoords.Num() ) {
		vertNum = a;
	} else if ( b >= 0 && b < texCoords.Num() ) {
		vertNum = b;
	} else if ( c >= 0 && c < texCoords.Num() ) {
		vertNum = c;
	} else {
		// all vertices are duplicates which shouldn't happen
		return 0;
	}

	// find the first weight for this vertex
	weightVertNum = 0;
	for ( int i = 0; weightVertNum < vertNum; i++ ) {
		weightVertNum += weightIndex[i * 2 + 1];
	}

	// get the joint for the largest weight
	bestWeight = scaledWeights[i].w;
	bestJoint = weightIndex[i * 2 + 0] / sizeof( anJointMat );

	for ( ; weightIndex[i * 2 + 1] == 0; i++ ) {
		if ( scaledWeights[i].w > bestWeight ) {
			bestWeight = scaledWeights[i].w;
			bestJoint = weightIndex[i * 2 + 0] / sizeof( anJointMat );
		}
	}
	return bestJoint;
}

/*
====================
anMD6Mesh_Reform::NumVerts
====================
*/
int anMD6Mesh_Reform::NumVerts( void ) const {
	return texCoords.Num();
}

/*
====================
anMD6Mesh_Reform::NumTris
====================
*/
int anMD6Mesh_Reform::NumTris( void ) const {
	return numTris;
}

/*
====================
anMD6Mesh_Reform::NumWeights
====================
*/
int anMD6Mesh_Reform::NumWeights( void ) const {
	return numWeights;
}

/***********************************************************************

		  anRenderModelMD6

***********************************************************************/

/*
=========================
anRenderModelMD6::anRenderModelMD6
=========================
*/
anRenderModelMD6::anRenderModelMD6() : idRenderModelStatic() {
	poseMat3 = nullptr;
	jointBuffer = nullptr;
}

/*
=========================
anRenderModelMD6::anRenderModelMD6
=========================
*/
anRenderModelMD6::~anRenderModelMD6() {
	if ( poseMat3 != nullptr ) {
		Mem_Free16( poseMat3 );
		poseMat3 = nullptr;
		if ( jointBuffer ) {
			delete jointBuffer;
			jointBuffer = nullptr;
		}
	}
}

/*
====================
anRenderModelMD6::ParseJoint
====================
*/
void anRenderModelMD6::ParseJoint( anLexer &parser, anMD5Joint *joint, anJointQuat *defaultPose ) {
	anToken token;

	// parse name
	parser.ReadToken( &token );
	joint->name = token;

	// parse parent
	int num = parser.ParseInt();
	if ( num < 0 ) {
		joint->parent = nullptr;
	} else {
		if  ( num >= joints.Num() - 1 ) {
			parser.Error( "Invalid parent for joint '%s'", joint->name.c_str() );
		}
		joint->parent = &joints[num];
	}

	// Joint channels.
	float jointChannels[8];
	parser.Parse1DMatrix(8, &jointChannels[0] );

	// Quat Rotation
	parser.Parse1DMatrix( 4, defaultPose->q.ToFloatPtr() );

	// Joint scale
	float jointScale[3];
	parser.Parse1DMatrix( 3, &jointScale[0] );

	// Joint Translation.
	anVec3 translation;
	parser.Parse1DMatrix( 3, &translation[0] );

	defaultPose->t = anVec3( translation[0], translation[1], translation[2] );
}

/*
====================
anRenderModelMD6::InitFromFile
====================
*/
void anRenderModelMD6::InitFromFile( const char *filename ) {
	name = fileName;
	LoadModel();
}

/*
====================
anRenderModelMD6::ParseInitBlock
====================
*/
void anRenderModelMD6::ParseInitBlock( anLexer &parser ) {
	anToken token;

	parser.ExpectTokenString( "{" );

	while ( true )  {
		if ( parser.EndOfFile() ) {
			common->FatalError( "ParseInitBlock: Unexpected EOF parsing init block!\n" );
			return;
	}

	parser.ReadToken( &token );

	if ( token == "}" ) {
		break;
	}

	if ( token == "numJoints" ) {
		int num = parser.ParseInt();
		joints.SetGranularity( 1 );
		joints.SetNum( num );
		defaultPose.SetGranularity( 1 );
		defaultPose.SetNum( num );
		poseMat3 = (anJointMat *)Mem_Alloc16( num * sizeof(* poseMat3) );
    } else if ( token == "commandLine" ) {
		parser.ReadToken( &token );
    } else if ( token == "sourceFile" ) {
		parser.ReadToken( &token );
    } else if ( token == "numMeshes" ) {
		int num = parser.ParseInt();
		if (num < 0 ) {
		  parser.Error( "Invalid size: %d", num );
		}
		meshes.SetGranularity( 1 );
		meshes.SetNum( num );
    } else if ( token == "numUserChannels" ) {
		int num = parser.ParseInt();
    } else if ( token == "numWeightSets" ) {
		int num = parser.ParseInt();
    } else if ( token == "remapForSkinning" ) {
		int num = parser.ParseInt();
    } else if ( token == "morphMap" ) {
		parser.ReadToken( &token );
    } else if ( token == "minExpand" ) {
		parser.Parse1DMatrix( 3, expandedBounds[0].ToFloatPtr() );
    } else if ( token == "maxExpand" ) {
		parser.Parse1DMatrix( 3, expandedBounds[1].ToFloatPtr() );
    } else {
		common->FatalError( "Unknown token in initblock %s\b", token.c_str() );
	}
	}
}

/*
====================
anRenderModelMD6::LoadModel

used for initial loads, reloadModel, and reloading the data of purged models
Upon exit, the model will absolutely be valid, but possibly as a default model
====================
*/
void anRenderModelMD6::LoadModel() {
	anLexer parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS );
	anJointQuat *pose;

	if ( !purged ) {
		PurgeModel();
	}

	purged = false;

	if ( !parser.LoadFile( name ) ) {
		MakeDefaultModel();
		return;
	}

	parser.ExpectTokenString( MD6_VERSION_STRING );
	int version = parser.ParseInt();

	if ( version != MD6_VERSION ) {
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD6_VERSION );
	}

	// Parse the init block.
	parser.ExpectTokenString( "init" );
	ParseInitBlock( parser );

	//
	// parse joints
	//
	parser.ExpectTokenString( "joints" );
	parser.ExpectTokenString( "{" );
	pose = defaultPose.Ptr();
	anMD5Joint *joint = joints.Ptr();

	for ( int i = 0; i < joints.Num(); i++, joint++, pose++ ) {
		ParseJoint( parser, joint, pose );
		if ( joint->parent ) {
			int parentNum = joint->parent - joints.Ptr();

			poseMat3[i].SetRotation( poseMat3[parentNum].ToMat3() * pose->q.ToMat3() );

			anVec3 relativeJointPos = pose->t;
			anVec3 parentJointPos = poseMat3[parentNum].ToVec3();
			anVec3 transformedJointPos = parentJointPos + relativeJointPos * poseMat3[parentNum].ToMat3().Transpose();
			poseMat3[i].SetTranslation( transformedJointPos );

			pose->q = ( poseMat3[i].ToMat3() * poseMat3[parentNum].ToMat3().Transpose() ).ToQuat();
			pose->t = ( poseMat3[i].ToVec3() - poseMat3[parentNum].ToVec3() ) * poseMat3[parentNum].ToMat3().Transpose();
		} else {
			poseMat3[i].SetRotation( pose->q.ToMat3() );
			poseMat3[i].SetTranslation( pose->t );
		}
	}
	parser.ExpectTokenString( "}" );

	//-----------------------------------------
	// create the inverse of the base pose joints to support tech6 style
	// deformation of base pose vertexes, normals, and tangents.
	//
	// vertex * joints * inverseJoints == vertex when joints is the base pose
	// When the joints are in another pose, it gives the animated vertex position
	//-----------------------------------------
	invertedDefaultPose.SetNum( SIMD_ROUND_JOINTS( joints.Num() ) );

	for ( int i = 0; i < joints.Num(); i++ ) {
		invertedDefaultPose[i] = poseMat3[i];
		invertedDefaultPose[i].Invert();
		SIMD_INIT_LAST_JOINT( invertedDefaultPose.Ptr(), joints.Num() );
		for ( int i = 0; i < meshes.Num(); i++ ) {
			parser.ExpectTokenString( "mesh" );
			// meshes[i].ParseMesh( parser, defaultPose.Num(), poseMat3 );
			meshes[i].ParseMesh( parser, defaultPose.Num(), poseMat3 );
		}
#ifndef ARC_DEDICATED
		anScopedCriticalSection lock( com_loadScreenMutex );
		anScopedLoadContext scopedContext;

		// Upload the bind pose to the GPU.
		jointBuffer = new idJointBuffer();
		jointBuffer->AllocBufferObject( (float *)invertedDefaultPose.Ptr(), invertedDefaultPose.Num() );
#endif
	}
	//
	// calculate the bounds of the model
	CalculateBounds( poseMat3 );

	// set the timestamp for reloadmodels
	fileSystem->ReadFile( name, nullptr, &timeStamp );
}

/*
==============
anRenderModelMD6::Print
==============
*/
void anRenderModelMD6::Print() const {
	common->Printf( "%s\n", name.c_str() );

	common->Printf( "Dynamic model.\n"
					"Generated smooth normals.\n"
					"    verts  tris weights material\n" );

	int totalVerts = 0, totalTris = 0, totalWeights = 0;

	for ( const anMD6Mesh_Reform * mesh = meshes.Ptr(), int i = 0; i < meshes.Num(); i++, mesh++ ) {
		totalVerts += mesh->NumVerts();
		totalTris += mesh->NumTris();
		totalWeights += mesh->NumWeights();
		common->Printf( "%2i: %5i %5i %7i %s\n", i, mesh->NumVerts(), mesh->NumTris(), mesh->NumWeights(), mesh->shader->GetName() );
	}
	common->Printf( "-----\n"
					"%4i verts.\n"
	 				"%4i tris.\n"
					"%4i weights.\n"
					"%4i joints.\n", totalVerts, totalTris, totalWeights, joints.Num() );;
}

/*
==============
anRenderModelMD6::List
==============
*/
void anRenderModelMD6::List() const {
	int totalTris = 0, totalVerts = 0;

	for ( const anMD6Mesh_Reform *mesh = meshes.Ptr(), int i = 0; i < meshes.Num(); i++, mesh++ ) {
		totalTris += mesh->numTris;
		totalVerts += mesh->NumVerts();
	}

	common->Printf( " %4ik %3i %4i %4i %s(MD6)", Memory() / 1024, meshes.Num(), totalVerts, totalTris, Name() );

	if ( defaulted ) {
		common->Printf( " (DEFAULTED)\n" );
	}

	common->Printf( "\n" );
}

/*
====================
anRenderModelMD6::CalculateBounds
====================
*/
void anRenderModelMD6::CalculateBounds( const anJointMat *entJoints ) {
	bounds.Clear();
	for ( anMD6Mesh_Reform *mesh = meshes.Ptr(), int i = 0; i < meshes.Num(); i++, mesh++ ) {
		bounds.AddBounds( mesh->CalcBounds( entJoints ) );
	}
}

/*
====================
anRenderModelMD6::Bounds

This calculates a rough bounds by using the joint radii without
transforming all the points
====================
*/
anBounds anRenderModelMD6::Bounds( const anRenderEntity *ent ) const {
#if 0
	// we can't calculate a rational bounds without an entity,
	// because joints could be positioned to deform it into an
	// arbitrarily large shape
	if ( !ent ) {
		common->Error( "anRenderModelMD6::Bounds: called without entity" );
	}
#endif
	if ( !ent ) {
		// this is the bounds for the reference pose
		return bounds;
	}

	anRenderEntityLocal *rent = (anRenderEntityLocal *)ent;
	return rent->GetBounds();
}

/*
====================
anRenderModelMD6::DrawJoints
====================
*/
void anRenderModelMD6::DrawJoints( const anRenderEntityParms *ent, const struct viewDef_s *view ) const {
	int num = ent->numJoints;
	const anJointMat *joint = ent->joints;

	if ( joint == nullptr || num == 0 ) {
		joint = poseMat3;
		const anJointMat *basejoint = poseMat3;
		num = this->NumJoints();
	} else {
		const anJointMat *basejoint = joint;
	}

	if ( r_showSkelMaxJoints.GetInteger() != -1 ) {
		num = r_showSkelMaxJoints.GetInteger();
		const anMD6Joint *md6Joint = joints.Ptr();
		for ( int i = 0; i < num; i++, joint++, MD6Joint++ ) {
			anVec3 pos = ent->origin + joint->ToVec3() * ent->axis;
			if ( MD6Joint->parent ) {
				int parentNum = MD6Joint->parent - joints.Ptr();
				rw->DebugLine( colorWhite, ent->origin + basejoint[parentNum].ToVec3() * ent->axis, pos );
			}
		}

		rw->DebugLine( colorRed, pos,pos + joint->ToMat3()[0] * 2.0f * ent->axis );
		rw->DebugLine( colorGreen, pos, pos + joint->ToMat3()[1] * 2.0f * ent->axis );
		rw->DebugLine( colorBlue, pos, pos + joint->ToMat3()[2] * 2.0f * ent->axis );
	}

	anBounds bounds;
	bounds.FromTransformedBounds( ent->bounds, vec3_zero, ent->axis );
	rw->DebugBounds( colorMagenta, bounds, ent->origin );

	if ( ( r_jointNameScale.GetFloat() != 0.0f ) && ( bounds.Expand( 128.0f ).ContainsPoint( view->renderView.vieworg - ent->origin ) ) ) {
		anVec3 offset( 0, 0, r_jointNameOffset.GetFloat() );
		float  scale = r_jointNameScale.GetFloat();
		joint = ent->joints;
		num = ent->numJoints;
		for ( int i = 0; i < num; i++, joint++ ) {
			anVec3 pos = ent->origin + joint->ToVec3() * ent->axis; rw->DrawText( joints[i].name, pos + offset, scale, colorWhite, view->renderView.viewaxis, 1 );
		}
	}
}

/*
====================
TransformJoints
====================
*/
static void TransformJoints( anJointMat *__restrict outJoints, const int numJoints, const anJointMat *__restrict inJoints1, const anJointMat *__restrict inJoints2 ) {
	float *outFloats = outJoints->ToFloatPtr();
	const float *inFloats1 = inJoints1->ToFloatPtr();
	const float *inFloats2 = inJoints2->ToFloatPtr();

	// assert_16_byte_aligned(outFloats);
	// assert_16_byte_aligned(inFloats1 );
	// assert_16_byte_aligned(inFloats2);
#ifdef ARC_WIN_X86_SSE2_INTRIN
	const __m128 mask_keep_last = __m128c( _mm_set_epi32( 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 ) );

	for ( int i = 0; i < numJoints;
		i += 2, inFloats1 += 2 * 12, inFloats2 += 2 * 12, outFloats += 2 * 12 ) {
		__m128 m1a0 = _mm_load_ps( inFloats1 + 0 * 12 + 0 );
		__m128 m1b0 = _mm_load_ps( inFloats1 + 0 * 12 + 4 );
		__m128 m1c0 = _mm_load_ps( inFloats1 + 0 * 12 + 8 );
		__m128 m1a1 = _mm_load_ps( inFloats1 + 1 * 12 + 0 );
		__m128 m1b1 = _mm_load_ps( inFloats1 + 1 * 12 + 4 );
		__m128 m1c1 = _mm_load_ps( inFloats1 + 1 * 12 + 8 );

		__m128 m2a0 = _mm_load_ps( inFloats2 + 0 * 12 + 0 );
		__m128 m2b0 = _mm_load_ps( inFloats2 + 0 * 12 + 4 );
		__m128 m2c0 = _mm_load_ps( inFloats2 + 0 * 12 + 8 );
		__m128 m2a1 = _mm_load_ps( inFloats2 + 1 * 12 + 0 );
		__m128 m2b1 = _mm_load_ps( inFloats2 + 1 * 12 + 4 );
		__m128 m2c1 = _mm_load_ps( inFloats2 + 1 * 12 + 8 );

		__m128 tj0 = _mm_and_ps( m1a0, mask_keep_last );
		__m128 tk0 = _mm_and_ps( m1b0, mask_keep_last );
		__m128 tl0 = _mm_and_ps( m1c0, mask_keep_last );
		__m128 tj1 = _mm_and_ps( m1a1, mask_keep_last );
		__m128 tk1 = _mm_and_ps( m1b1, mask_keep_last );
		__m128 tl1 = _mm_and_ps( m1c1, mask_keep_last );

		__m128 ta0 = _mm_splat_ps( m1a0, 0 );
		__m128 td0 = _mm_splat_ps( m1b0, 0 );
		__m128 tg0 = _mm_splat_ps( m1c0, 0 );
		__m128 ta1 = _mm_splat_ps( m1a1, 0 );
		__m128 td1 = _mm_splat_ps( m1b1, 0 );
		__m128 tg1 = _mm_splat_ps( m1c1, 0 );

		__m128 ra0 = _mm_add_ps( tj0, _mm_mul_ps( ta0, m2a0 ) );
		__m128 rd0 = _mm_add_ps( tk0, _mm_mul_ps( td0, m2a0 ) );
		__m128 rg0 = _mm_add_ps( tl0, _mm_mul_ps( tg0, m2a0 ) );
		__m128 ra1 = _mm_add_ps( tj1, _mm_mul_ps( ta1, m2a1 ) );
		__m128 rd1 = _mm_add_ps( tk1, _mm_mul_ps( td1, m2a1 ) );
		__m128 rg1 = _mm_add_ps( tl1, _mm_mul_ps( tg1, m2a1 ) );

		__m128 tb0 = _mm_splat_ps( m1a0, 1 );
		__m128 te0 = _mm_splat_ps( m1b0, 1 );
		__m128 th0 = _mm_splat_ps( m1c0, 1 );
		__m128 tb1 = _mm_splat_ps( m1a1, 1 );
		__m128 te1 = _mm_splat_ps( m1b1, 1 );
		__m128 th1 = _mm_splat_ps( m1c1, 1 );

		__m128 rb0 = _mm_add_ps( ra0, _mm_mul_ps(tb0, m2b0));
		__m128 re0 = _mm_add_ps( rd0, _mm_mul_ps(te0, m2b0));
		__m128 rh0 = _mm_add_ps( rg0, _mm_mul_ps(th0, m2b0));
		__m128 rb1 = _mm_add_ps( ra1, _mm_mul_ps(tb1, m2b1 ) );
		__m128 re1 = _mm_add_ps( rd1, _mm_mul_ps(te1, m2b1 ) );
		__m128 rh1 = _mm_add_ps( rg1, _mm_mul_ps(th1, m2b1 ) );

		__m128 tc0 = _mm_splat_ps( m1a0, 2 );
		__m128 tf0 = _mm_splat_ps( m1b0, 2 );
		__m128 ti0 = _mm_splat_ps( m1c0, 2 );
		__m128 tf1 = _mm_splat_ps( m1b1, 2 );
		__m128 ti1 = _mm_splat_ps( m1c1, 2 );
		__m128 tc1 = _mm_splat_ps( m1a1, 2 );

		__m128 rc0 = _mm_add_ps( rb0, _mm_mul_ps( tc0, m2c0 ) );
		__m128 rf0 = _mm_add_ps( re0, _mm_mul_ps( tf0, m2c0 ) );
		__m128 ri0 = _mm_add_ps( rh0, _mm_mul_ps( ti0, m2c0 ) );
		__m128 rc1 = _mm_add_ps( rb1, _mm_mul_ps( tc1, m2c1 ) );
		__m128 rf1 = _mm_add_ps( re1, _mm_mul_ps( tf1, m2c1 ) );
		__m128 ri1 = _mm_add_ps( rh1, _mm_mul_ps( ti1, m2c1 ) );

		_mm_store_ps( outFloats + 0 * 12 + 0, rc0 );
		_mm_store_ps( outFloats + 0 * 12 + 4, rf0 );
		_mm_store_ps( outFloats + 0 * 12 + 8, ri0 );
		_mm_store_ps( outFloats + 1 * 12 + 0, rc1 );
		_mm_store_ps( outFloats + 1 * 12 + 4, rf1 );
		_mm_store_ps( outFloats + 1 * 12 + 8, ri1 );
	}
#else
	for ( int i = 0; i < numJoints; i++ ) {
		anJointMat::Multiply( outJoints[i], inJoints1[i], inJoints2[i] );
	}
#endif
}

/*
====================
anRenderModelMD6::InstantiateDynamicModel
====================
*/
anRenderModel *anRenderModelMD6::InstantiateDynamicModel( const class anRenderEntity *ent, const struct viewDef_s *view, anRenderModel *cachedModel ) {
	anRenderModelMD6Instance *staticModel;
	const anRenderEntityLocal *renderEntity = (const anRenderEntityLocal *)ent;

	if ( cachedModel && !r_useCachedDynamicModels.GetBool() ) {
		delete cachedModel;
		cachedModel = nullptr;
	}

	if ( purged ) {
		common->DWarning( "model %s instantiated while purged", Name() ) ;
		LoadModel();
	}

	if ( cachedModel ) {
		assert( dynamic_cast<anRenderModelMD6Instance *>( cachedModel ) != nullptr );
		assert( anStr::Icmp( cachedModel->Name(), MD6_SnapshotName ) == 0 );
		staticModel = static_cast<anRenderModelMD6Instance *>( cachedModel );
	} else {
		staticModel = new anRenderModelMD6Instance;
		staticModel->InitEmpty( MD6_SnapshotName );
	}

	if ( r_showSkel.GetInteger() ) {
		DrawJoints( &( (const anRenderEntityLocal *)ent)->parms, view );
	}

	// update the GPU joints array
	const int numInvertedJoints = SIMD_ROUND_JOINTS( joints.Num() );
	if ( staticModel->jointsInverted == nullptr ) {
		staticModel->numInvertedJoints = numInvertedJoints;
		const int alignment = glConfig.uniformBufferOffsetAlignment;
		staticModel->jointsInverted = (anJointMat *)Mem_ClearedAlloc( ALIGN( numInvertedJoints * sizeof( anJointMat ), alignment ) );
	} else {
		assert( staticModel->numInvertedJoints == numInvertedJoints );
	}

	staticModel->jointBuffer = jointBuffer;
	if ( renderEntity == nullptr || renderEntity->parms.joints == nullptr ) {
		TransformJoints( staticModel->jointsInverted, joints.Num(), &poseMat3[0], invertedDefaultPose.Ptr() );
	} else {
		TransformJoints( staticModel->jointsInverted, joints.Num(), renderEntity->parms.joints, invertedDefaultPose.Ptr() );
	}

	staticModel->CreateStaticMeshSurfaces( meshes );

	return staticModel;
}

/*
====================
anRenderModelMD6::IsDynamicModel
====================
*/
dynamicModel_t anRenderModelMD6::IsDynamicModel() const {
	return DM_CACHED;
}

/*
====================
anRenderModelMD6::NumJoints
====================
*/
int anRenderModelMD6::NumJoints( void ) const {
r	eturn joints.Num();
}

/*
====================
anRenderModelMD6::GetJoints
====================
*/
const anMD5Joint *anRenderModelMD6::GetJoints( void ) const {
	return joints.Ptr();
}

/*
====================
anRenderModelMD6::GetDefaultPose
====================
*/
const anJointQuat *anRenderModelMD6::GetDefaultPose( void ) const {
	return defaultPose.Ptr();
}

/*
====================
anRenderModelMD6::GetJointHandle
====================
*/
jointHandle_t anRenderModelMD6::GetJointHandle( const char *name ) const {
	const anMD5Joint *joint = joints.Ptr();

	for ( int i = 0; i < joints.Num(); i++, joint++ ) {
		if ( anStr::Icmp( joint->name.c_str(), name ) == 0 ) {
			return ( jointHandle_t )i;
		}
	}

	return INVALID_JOINT;
}

/*
=====================
anRenderModelMD6::GetJointName
=====================
*/
const char *anRenderModelMD6::GetJointName( jointHandle_t handle ) const {
	if ( ( handle < 0 ) || ( handle >= joints.Num() ) ) {
		return "<invalid joint>";
	}

	return joints[handle].name;
}

/*
====================
anRenderModelMD6::NearestJoint
====================
*/
int anRenderModelMD6::NearestJoint( intsurfaceNum, int a, int b, int c ) const {
	if ( surfaceNum > meshes.Num() ) {
		common->Error( "anRenderModelMD6::NearestJoint: surfaceNum > meshes.Num()" );
	}

	for ( const anMD6Mesh_Reform *mesh = meshes.Ptr(), int i = 0; i < meshes.Num(); i++, mesh++ ) {
		if ( mesh->surfaceNum == surfaceNum )  {
			return mesh->NearestJoint( a, b, c );
		}
	}
	return 0;
}

/*
====================
anRenderModelMD6::TouchData

models that are already loaded at level start time
will still touch their materials to make sure they
are kept loaded
====================
*/
void anRenderModelMD6::TouchData() {
	for ( anMD6Mesh_Reform *mesh = meshes.Ptr(), int i = 0; i < meshes.Num(); i++, mesh++ ) {
		declManager->FindMaterial( mesh->shader->GetName() );
	}
}

/*
===================
anRenderModelMD6::PurgeModel

frees all the data, but leaves the class around for dangling references,
which can regenerate the data with LoadModel()
===================
*/
void anRenderModelMD6::PurgeModel() {
	purged = true;
	joints.Clear();
	defaultPose.Clear();
	meshes.Clear();
}

/*
===================
anRenderModelMD6::Memory
===================
*/
int anRenderModelMD6::Memory() const {
	int total = sizeof(* this);
	total += joints.MemoryUsed() + defaultPose.MemoryUsed() + meshes.MemoryUsed();

	// count up strings
	for ( int i = 0; i < joints.Num(); i++ ) {
		total += joints[i].name.DynamicMemoryUsed();
		// count up meshes
		for ( i = 0; i < meshes.Num(); i++ ) {
			const anMD6Mesh_Reform *mesh = &meshes[i];

			total += mesh->texCoords.MemoryUsed() +mesh->numWeights * ( sizeof( mesh->scaledWeights[0] ) + sizeof( mesh->weightIndex[0] ) * 2 );

			// sum up deform info
			total += sizeof( mesh->deformInfo );
			total += R_DeformInfoMemoryUsed( mesh->deformInfo );
		}
	}
	return total;
}

/*
====================
anRenderModelMD6Instance::IsSkeletalMesh
====================
*/
bool anRenderModelMD6Instance::IsSkeletalMesh() const {
	return true;
}

/*
====================
anRenderModelMD6Instance::CreateStaticMeshSurfaces
====================
*/
void anRenderModelMD6Instance::CreateStaticMeshSurfaces( const anList<anMD6Mesh_Reform> &meshes ) {
	int surfaceNum;

	// create all the surfaces
	for ( const anMD6Mesh_Reform *mesh = meshes.Ptr(), int i = 0; i < meshes.Num(); i++, mesh++ ) {
			const anMaterial *shader = mesh->GetShader();
		struct deformInfo_s *deformInfo = mesh->deformInfo;
		modelSurface_t *surf;

		if ( FindSurfaceWithId( i, surfaceNum ) ) {
			surf = &surfaces[surfaceNum];
		} else {
			surf = &surfaces.Alloc();
			surf->geometry = nullptr;
			surf->shader = shader;
			surf->id = i;

			UpdateSurfaceGPU( deformInfo, nullptr, surf, shader );

			bounds.AddPoint( surf->geometry->bounds[0] );
			bounds.AddPoint( surf->geometry->bounds[1] );
		}
	}
}

/*
====================
anRenderModelMD6Instance::UpdateSurface
====================
*/
void anRenderModelMD6Instance::UpdateSurfaceGPU( struct deformInfo_s *deformInfo, const anRenderEntityParms *ent, modelSurface_t *surf, const anMaterial *shader ) {
	tr.pc.c_deformedSurfaces++;
	tr.pc.c_deformedVerts += deformInfo->numOutputVerts;
	tr.pc.c_deformedIndexes += deformInfo->numIndexes;

	surf->shader = shader;

	if ( surf->geometry ) {
		// if the number of verts and indexes are the same we can re-use the
		// triangle surface the number of indexes must be the same to assure the
		// correct amount of memory is allocated for the facePlanes
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

	R_AllocStaticTriSurfVerts( tri, tri->numVerts );
	memcpy( tri->verts, deformInfo->verts, deformInfo->numOutputVerts * sizeof( deformInfo->verts[0] ) ); // copy over the texture coordinates

	tri->tangentsCalculated = true;
	R_BoundTriSurf( tri );
}
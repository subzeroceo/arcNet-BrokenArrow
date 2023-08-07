#include "../idlib/Lib.h"
#pragma hdrstop

#include "Model_ase.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*
======================================================================

	Parses 3D Studio Max ASCII export files.
	The goal is to parse the information into memory exactly as it is
	represented in the file.  Users of the data will then move it
	into a form that is more convenient for them.

======================================================================
*/


#define VERBOSE( x ) { if ( ase.verbose ) { common->Printf x; } }

// working variables used during parsing
typedef struct {
	const char	*buffer;
	const char	*curpos;
	int			len;
	char		token[1024];

	bool	verbose;

	aseModel_t	*model;
	aseObject_t	*currentObject;
	aseMesh_t	*currentMesh;
	aseMaterial_t	*currentMaterial;
	int			currentFace;
	int			currentVertex;
} ase_t;

static ase_t ase;


static aseMesh_t *ASE_GetCurrentMesh( void ) {
	return ase.currentMesh;
}

static int CharIsTokenDelimiter( int ch ) {
	if ( ch <= 32 )
		return 1;
	return 0;
}

static int ASE_GetToken( bool restOfLine ) {
	int i = 0;

	if ( ase.buffer == 0 )
		return 0;

	if ( ( ase.curpos - ase.buffer ) == ase.len )
		return 0;

	// skip over crap
	while ( ( ( ase.curpos - ase.buffer ) < ase.len ) && ( *ase.curpos <= 32 ) ) {
		ase.curpos++;
	}

	while ( ( ase.curpos - ase.buffer ) < ase.len ) {
		ase.token[i] = *ase.curpos;
		ase.curpos++;
		i++;

		if ( ( CharIsTokenDelimiter( ase.token[i-1] ) && !restOfLine ) || ( ( ase.token[i-1] == '\n' ) || ( ase.token[i-1] == '\r' ) ) ) {
			ase.token[i-1] = 0;
			break;
		}
	}

	ase.token[i] = 0;

	return 1;
}

static void ASE_ParseBracedBlock( void (*parser)( const char *token ) ) {
	int indent = 0;

	while ( ASE_GetToken( false ) ) {
		if ( !strcmp( ase.token, "{" ) ) {
			indent++;
		} else if ( !strcmp( ase.token, "}" ) ) {
			--indent;
			if ( indent == 0 )
				break;
			else if ( indent < 0 )
				common->Error( "Unexpected '}'" );
		} else {
			if ( parser ) {
				parser( ase.token );
			}
		}
	}
}

static void ASE_SkipEnclosingBraces( void ) {
	int indent = 0;

	while ( ASE_GetToken( false ) ) {
		if ( !strcmp( ase.token, "{" ) ) {
			indent++;
		} else if ( !strcmp( ase.token, "}" ) ) {
			indent--;
			if ( indent == 0 ) {
				break;
			} else if ( indent < 0 ) {
				common->Error( "Unexpected '}'" );
			}
		}
	}
}

static void ASE_SkipRestOfLine( void ) {
	ASE_GetToken( true );
}

static void ASE_KeyMAP_DIFFUSE( const char *token ) {
	aseMaterial_t *material;

	if ( !strcmp( token, "*BITMAP" ) ) {
		anStr	qpath;
		anStr	matname;

		ASE_GetToken( false );

		// remove the quotes
		char *s = strstr( ase.token + 1, "\"" );
		if ( s ) {
			*s = 0;
		}
		matname = ase.token + 1;

		// convert the 3DSMax material pathname to a qpath
		matname.BackSlashesToSlashes();
		qpath = fileSystem->OSPathToRelativePath( matname );
		anStr::Copynz( ase.currentMaterial->name, qpath, sizeof( ase.currentMaterial->name ) );
	} else if ( !strcmp( token, "*UVW_U_OFFSET" ) ) {
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->uOffset = atof( ase.token );
	} else if ( !strcmp( token, "*UVW_V_OFFSET" ) ) {
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->vOffset = atof( ase.token );
	} else if ( !strcmp( token, "*UVW_U_TILING" ) ) {
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->uTiling = atof( ase.token );
	} else if ( !strcmp( token, "*UVW_V_TILING" ) ) {
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->vTiling = atof( ase.token );
	} else if ( !strcmp( token, "*UVW_ANGLE" ) ) {
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->angle = atof( ase.token );
	} else {
    if ( strstr( buff2, buff1 + 2 ) ) {
			strcpy( ase.materials[ase.numMaterials].name, strstr( buff2, buff1 + 2 ) + strlen( buff1 ) - 2 );
		} else {
			sprintf( ase.materials[ase.numMaterials].name, "(not converted: '%s')", buffer );
			printf( "WARNING: illegal material name '%s'\n", buffer );
		}
	}
}

static void ASE_KeyMATERIAL( const char *token ) {
	if ( !strcmp( token, "*MAP_DIFFUSE" ) ) {
		ASE_ParseBracedBlock( ASE_KeyMAP_DIFFUSE );
	} else {
	}
}

static void ASE_KeyMATERIAL_LIST( const char *token ) {
	if ( !strcmp( token, "*MATERIAL_COUNT" ) ) {
		ASE_GetToken( false );
		VERBOSE( ( "..num materials: %s\n", ase.token ) );
	} else if ( !strcmp( token, "*MATERIAL" ) ) {
		VERBOSE( ( "..material %d\n", ase.model->materials.Num() ) );

		ase.currentMaterial = (aseMaterial_t *)Mem_Alloc( sizeof( aseMaterial_t ) );
		memset( ase.currentMaterial, 0, sizeof( aseMaterial_t ) );
		ase.currentMaterial->uTiling = 1;
		ase.currentMaterial->vTiling = 1;
		ase.model->materials.Append(ase.currentMaterial);

		ASE_ParseBracedBlock( ASE_KeyMATERIAL );
	}
}

static void ASE_KeyNODE_TM( const char *token ) {
	if ( !strcmp( token, "*TM_ROW0" ) ) {
		for ( int i = 0; i < 3; i++ ) {
			ASE_GetToken( false );
			ase.currentObject->mesh.transform[0][i] = atof( ase.token );
		}
	} else if ( !strcmp( token, "*TM_ROW1" ) ) {
		for ( int i = 0; i < 3; i++ ) {
			ASE_GetToken( false );
			ase.currentObject->mesh.transform[1][i] = atof( ase.token );
		}
	} else if ( !strcmp( token, "*TM_ROW2" ) ) {
		for ( int i = 0; i < 3; i++ ) {
			ASE_GetToken( false );
			ase.currentObject->mesh.transform[2][i] = atof( ase.token );
		}
	} else if ( !strcmp( token, "*TM_ROW3" ) ) {
		for ( int i = 0; i < 3; i++ ) {
			ASE_GetToken( false );
			ase.currentObject->mesh.transform[3][i] = atof( ase.token );
		}
	}
}

static void ASE_KeyMESH_VERTEX_LIST( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_VERTEX" ) ) {
		ASE_GetToken( false );		// skip number
		ASE_GetToken( false );
		pMesh->vertexes[ase.currentVertex].x = atof( ase.token );
		ASE_GetToken( false );
		pMesh->vertexes[ase.currentVertex].y = atof( ase.token );
		ASE_GetToken( false );
		pMesh->vertexes[ase.currentVertex].z = atof( ase.token );
		ase.currentVertex++;

		if ( ase.currentVertex > pMesh->numVertexes ) {
			common->Error( "ase.currentVertex >= pMesh->numVertexes" );
		}
	} else {
		common->Error( "Unknown token '%s' while parsing MESH_VERTEX_LIST", token );
	}
}

static void ASE_KeyTFACE_LIST( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_TFACE" ) ) {
		ASE_GetToken( false );
		ASE_GetToken( false );
		int a = atoi( ase.token );
		ASE_GetToken( false );
		int c = atoi( ase.token );
		ASE_GetToken( false );
		int b = atoi( ase.token );

		pMesh->faces[ase.currentFace].tVertexNum[0] = a;
		pMesh->faces[ase.currentFace].tVertexNum[1] = b;
		pMesh->faces[ase.currentFace].tVertexNum[2] = c;
		ase.currentFace++;
	} else {
		common->Error( "Unknown token '%s' in MESH_TFACE", token );
	}
}

static void ASE_KeyCFACE_LIST( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_CFACE" ) ) {
		ASE_GetToken( false );
		for ( int i = 0; i < 3; i++ ) {
			ASE_GetToken( false );
			int a = atoi( ase.token );

			// we flip the vertex order to change the face direction to our style
			static int remap[3] = { 0, 2, 1 };
			pMesh->faces[ase.currentFace].vertexColors[remap[i]][0] = pMesh->cvertexes[a][0] * 255;
			pMesh->faces[ase.currentFace].vertexColors[remap[i]][1] = pMesh->cvertexes[a][1] * 255;
			pMesh->faces[ase.currentFace].vertexColors[remap[i]][2] = pMesh->cvertexes[a][2] * 255;
		}
		ase.currentFace++;
	} else {
		common->Error( "Unknown token '%s' in MESH_CFACE", token );
	}
}

static void ASE_KeyMESH_TVERTLIST( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_TVERT" ) ) {
		char u[80], v[80], w[80];

		ASE_GetToken( false );

		ASE_GetToken( false );
		strcpy( u, ase.token );

		ASE_GetToken( false );
		strcpy( v, ase.token );

		ASE_GetToken( false );
		strcpy( w, ase.token );

		pMesh->tvertexes[ase.currentVertex].x = atof( u );
		// our OpenGL second texture axis is inverted from MAX's sense
		pMesh->tvertexes[ase.currentVertex].y = 1.0f - atof( v );

		ase.currentVertex++;

		if ( ase.currentVertex > pMesh->numTVertexes ) {
			common->Error( "ase.currentVertex > pMesh->numTVertexes" );
		}
	} else {
		common->Error( "Unknown token '%s' while parsing MESH_TVERTLIST", token );
	}
}

static void ASE_KeyMESH_CVERTLIST( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	pMesh->colorsParsed = true;

	if ( !strcmp( token, "*MESH_VERTCOL" ) ) {
		ASE_GetToken( false );

		ASE_GetToken( false );
		pMesh->cvertexes[ase.currentVertex][0] = atof( token );

		ASE_GetToken( false );
		pMesh->cvertexes[ase.currentVertex][1] = atof( token );

		ASE_GetToken( false );
		pMesh->cvertexes[ase.currentVertex][2] = atof( token );

		ase.currentVertex++;

		if ( ase.currentVertex > pMesh->numCVertexes ) {
			common->Error( "ase.currentVertex > pMesh->numCVertexes" );
		}
	} else {
		common->Error( "Unknown token '%s' while parsing MESH_CVERTLIST", token );
	}
}

static void ASE_KeyMESH_NORMALS( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();
	aseFace_t	*f;
	anVec3		n;

	pMesh->normalsParsed = true;
	f = &pMesh->faces[ase.currentFace];

	if ( !strcmp( token, "*MESH_FACENORMAL" ) ) {
		ASE_GetToken( false );
		int num = atoi( ase.token );
		if ( num >= pMesh->numFaces || num < 0 ) {
			common->Error( "MESH_NORMALS face index out of range: %i", num );
		}

		if ( num != ase.currentFace ) {
			common->Error( "MESH_NORMALS face index != currentFace" );
		}

		ASE_GetToken( false );
		n[0] = atof( ase.token );
		ASE_GetToken( false );
		n[1] = atof( ase.token );
		ASE_GetToken( false );
		n[2]= atof( ase.token );

		f->faceNormal[0] = n[0] * pMesh->transform[0][0] + n[1] * pMesh->transform[1][0] + n[2] * pMesh->transform[2][0];
		f->faceNormal[1] = n[0] * pMesh->transform[0][1] + n[1] * pMesh->transform[1][1] + n[2] * pMesh->transform[2][1];
		f->faceNormal[2] = n[0] * pMesh->transform[0][2] + n[1] * pMesh->transform[1][2] + n[2] * pMesh->transform[2][2];

		f->faceNormal.Normalize();

		ase.currentFace++;
	} else if ( !strcmp( token, "*MESH_VERTEXNORMAL" ) ) {
		ASE_GetToken( false );
		int num = atoi( ase.token );

		if ( num >= pMesh->numVertexes || num < 0 ) {
			common->Error( "MESH_NORMALS vertex index out of range: %i", num );
		}

		f = &pMesh->faces[ ase.currentFace - 1 ];

		for ( int v = 0; v < 3; v++ ) {
			if ( num == f->vertexNum[ v ] ) {
				break;
			}
		}

		if ( v == 3 ) {
			common->Error( "MESH_NORMALS vertex index doesn't match face" );
		}

		ASE_GetToken( false );
		n[0] = atof( ase.token );
		ASE_GetToken( false );
		n[1] = atof( ase.token );
		ASE_GetToken( false );
		n[2]= atof( ase.token );

		f->vertexNormals[ v ][0] = n[0] * pMesh->transform[0][0] + n[1] * pMesh->transform[1][0] + n[2] * pMesh->transform[2][0];
		f->vertexNormals[ v ][1] = n[0] * pMesh->transform[0][1] + n[1] * pMesh->transform[1][1] + n[2] * pMesh->transform[2][1];
		f->vertexNormals[ v ][2] = n[0] * pMesh->transform[0][2] + n[1] * pMesh->transform[1][2] + n[2] * pMesh->transform[2][2];

		f->vertexNormals[v].Normalize();
	}
}

static void ASE_KeyMESH( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*TIMEVALUE" ) ) {
		ASE_GetToken( false );
		pMesh->timeValue = atoi( ase.token );
		VERBOSE( ( ".....timevalue: %d\n", pMesh->timeValue ) );
	} else if ( !strcmp( token, "*MESH_NUMVERTEX" ) ) {
		ASE_GetToken( false );
		pMesh->numVertexes = atoi( ase.token );
		VERBOSE( ( ".....num vertexes: %d\n", pMesh->numVertexes ) );
	} else if ( !strcmp( token, "*MESH_NUMTVERTEX" ) ) {
		ASE_GetToken( false );
		pMesh->numTVertexes = atoi( ase.token );
		VERBOSE( ( ".....num tvertexes: %d\n", pMesh->numTVertexes ) );
	} else if ( !strcmp( token, "*MESH_NUMCVERTEX" ) ) {
		ASE_GetToken( false );
		pMesh->numCVertexes = atoi( ase.token );
		VERBOSE( ( ".....num cvertexes: %d\n", pMesh->numCVertexes ) );
	} else if ( !strcmp( token, "*MESH_NUMFACES" ) ) {
		ASE_GetToken( false );
		pMesh->numFaces = atoi( ase.token );
		VERBOSE( ( ".....num faces: %d\n", pMesh->numFaces ) );
	} else if ( !strcmp( token, "*MESH_NUMTVFACES" ) ) {
		ASE_GetToken( false );
		pMesh->numTVFaces = atoi( ase.token );
		VERBOSE( ( ".....num tvfaces: %d\n", pMesh->numTVFaces ) );
		if ( pMesh->numTVFaces != pMesh->numFaces ) {
			common->Error( "MESH_NUMTVFACES != MESH_NUMFACES" );
		}
	} else if ( !strcmp( token, "*MESH_NUMCVFACES" ) ) {
		ASE_GetToken( false );
		pMesh->numCVFaces = atoi( ase.token );
		VERBOSE( ( ".....num cvfaces: %d\n", pMesh->numCVFaces ) );
		if ( pMesh->numTVFaces != pMesh->numFaces ) {
			common->Error( "MESH_NUMCVFACES != MESH_NUMFACES" );
		}
	} else if ( !strcmp( token, "*MESH_VERTEX_LIST" ) ) {
		pMesh->vertexes = (anVec3 *)Mem_Alloc( sizeof( anVec3 ) * pMesh->numVertexes );
		ase.currentVertex = 0;
		VERBOSE( ( ".....parsing MESH_VERTEX_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_VERTEX_LIST );
	} else if ( !strcmp( token, "*MESH_TVERTLIST" ) ) {
		ase.currentVertex = 0;
		pMesh->tvertexes = (anVec2 *)Mem_Alloc( sizeof( anVec2 ) * pMesh->numTVertexes );
		VERBOSE( ( ".....parsing MESH_TVERTLIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_TVERTLIST );
	} else if ( !strcmp( token, "*MESH_CVERTLIST" ) ) {
		ase.currentVertex = 0;
		pMesh->cvertexes = (anVec3 *)Mem_Alloc( sizeof( anVec3 ) * pMesh->numCVertexes );
		VERBOSE( ( ".....parsing MESH_CVERTLIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_CVERTLIST );
	} else if ( !strcmp( token, "*MESH_FACE_LIST" ) ) {
		pMesh->faces = (aseFace_t *)Mem_Alloc( sizeof( aseFace_t ) * pMesh->numFaces );
		ase.currentFace = 0;
		VERBOSE( ( ".....parsing MESH_FACE_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_FACE_LIST );
	} else if ( !strcmp( token, "*MESH_TFACELIST" ) ) {
		if ( !pMesh->faces ) {
			common->Error( "*MESH_TFACELIST before *MESH_FACE_LIST" );
		}
		ase.currentFace = 0;
		VERBOSE( ( ".....parsing MESH_TFACE_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyTFACE_LIST );
	} else if ( !strcmp( token, "*MESH_CFACELIST" ) ) {
		if ( !pMesh->faces ) {
			common->Error( "*MESH_CFACELIST before *MESH_FACE_LIST" );
		}
		ase.currentFace = 0;
		VERBOSE( ( ".....parsing MESH_CFACE_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyCFACE_LIST );
	} else if ( !strcmp( token, "*MESH_NORMALS" ) ) {
		if ( !pMesh->faces ) {
			common->Warning( "*MESH_NORMALS before *MESH_FACE_LIST" );
		}
		ase.currentFace = 0;
		VERBOSE( ( ".....parsing MESH_NORMALS\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_NORMALS );
	}
}

static void ASE_KeyMESH_ANIMATION( const char *token ) {
	aseMesh_t *mesh;

	// loads a single animation frame
	if ( !strcmp( token, "*MESH" ) ) {
		VERBOSE( ( "...found MESH\n" ) );

		mesh = (aseMesh_t *)Mem_Alloc( sizeof( aseMesh_t ) );
		memset( mesh, 0, sizeof( aseMesh_t ) );
		ase.currentMesh = mesh;

		ase.currentObject->frames.Append( mesh );

		ASE_ParseBracedBlock( ASE_KeyMESH );
	} else {
		common->Error( "Unknown token '%s' while parsing MESH_ANIMATION", token );
	}
}

static void ASE_KeyGEOMOBJECT( const char *token ) {
	aseObject_t	*object;

	object = ase.currentObject;

	if ( !strcmp( token, "*NODE_NAME" ) ) {
		ASE_GetToken( true );
		VERBOSE( ( " %s\n", ase.token ) );
		anStr::Copynz( object->name, ase.token, sizeof( object->name ) );
	} else if ( !strcmp( token, "*NODE_PARENT" ) ) {
		ASE_SkipRestOfLine();
	// ignore unused data blocks
	} else if ( !strcmp( token, "*NODE_TM" ) || !strcmp( token, "*TM_ANIMATION" ) ) {
		ASE_ParseBracedBlock( ASE_KeyNODE_TM );
	// ignore regular meshes that aren't part of animation
	} else if ( !strcmp( token, "*MESH" ) ) {
		ase.currentMesh = &ase.currentObject->mesh;
		memset( ase.currentMesh, 0, sizeof( ase.currentMesh ) );
		ASE_ParseBracedBlock( ASE_KeyMESH );
	// according to spec these are obsolete
	} else if ( !strcmp( token, "*MATERIAL_REF" ) ) {
		ASE_GetToken( false );
		object->materialRef = atoi( ase.token );
	// loads a sequence of animation frames
	} else if ( !strcmp( token, "*MESH_ANIMATION" ) ) {
		VERBOSE( ( "..found MESH_ANIMATION\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_ANIMATION );
	// skip unused info
	} else if ( !strcmp( token, "*PROP_MOTIONBLUR" ) || !strcmp( token, "*PROP_CASTSHADOW" ) || !strcmp( token, "*PROP_RECVSHADOW" ) ) {
		ASE_SkipRestOfLine();
	}

}

void ASE_ParseGeomObject( void ) {
	aseObject_t	*object;

	VERBOSE( ( "GEOMOBJECT" ) );

	object = (aseObject_t *)Mem_Alloc( sizeof( aseObject_t ) );
	memset( object, 0, sizeof( aseObject_t ) );
	ase.model->objects.Append( object );
	ase.currentObject = object;

	object->frames.Resize(32, 32);

	ASE_ParseBracedBlock( ASE_KeyGEOMOBJECT );
}

static void ASE_KeyGROUP( const char *token ) {
	if ( !strcmp( token, "*GEOMOBJECT" ) ) {
		ASE_ParseGeomObject();
	}
}

/*
=================
ASE_Parse
=================
*/
aseModel_t *ASE_Parse( const char *buffer, bool verbose ) {
	memset( &ase, 0, sizeof( ase ) );

	ase.verbose = verbose;

	ase.buffer = buffer;
	ase.len = strlen( buffer );
	ase.curpos = ase.buffer;
	ase.currentObject = nullptr;

	// NOTE: using new operator because aseModel_t contains anList class objects
	ase.model = new aseModel_t;
	memset( ase.model, 0, sizeof( aseModel_t ) );
	ase.model->objects.Resize( 32, 32 );
	ase.model->materials.Resize( 32, 32 );

	while ( ASE_GetToken( false ) ) {
		if ( !strcmp( ase.token, "*3DSMAX_ASCIIEXPORT" ) || !strcmp( ase.token, "*COMMENT" ) ) {
			ASE_SkipRestOfLine();
		} else if ( !strcmp( ase.token, "*SCENE" ) ) {
			ASE_SkipEnclosingBraces();
		} else if ( !strcmp( ase.token, "*GROUP" ) ) {
			ASE_GetToken( false );		// group name
			ASE_ParseBracedBlock( ASE_KeyGROUP );
		} else if ( !strcmp( ase.token, "*SHAPEOBJECT" ) ) {
			ASE_SkipEnclosingBraces();
		} else if ( !strcmp( ase.token, "*CAMERAOBJECT" ) ) {
			ASE_SkipEnclosingBraces();
		} else if ( !strcmp( ase.token, "*MATERIAL_LIST" ) ) {
			VERBOSE( ( "MATERIAL_LIST\n" ) );
			ASE_ParseBracedBlock( ASE_KeyMATERIAL_LIST );
		} else if ( !strcmp( ase.token, "*GEOMOBJECT" ) ) {
			ASE_ParseGeomObject();
		} else if ( ase.token[0] ) {
			common->Printf( "Unknown token '%s'\n", ase.token );
		}
	}

	return ase.model;
}

/*
=================
ASE_Load
=================
*/
aseModel_t *ASE_Load( const char *fileName ) {
	char *buf;
	ARC_TIME_T timeStamp;
	aseModel_t *ase;

	fileSystem->ReadFile( fileName, (void **)&buf, &timeStamp );
	if ( !buf ) {
		return nullptr;
	}

	ase = ASE_Parse( buf, false );
	ase->timeStamp = timeStamp;

	fileSystem->FreeFile( buf );

	return ase;
}

/*
=================
ASE_Free
=================
*/
void ASE_Free( aseModel_t *ase ) {
	int					i, j;
	aseObject_t			*obj;
	aseMesh_t			*mesh;
	aseMaterial_t		*material;

	if ( !ase ) {
		return;
	}
	for ( i = 0; i < ase->objects.Num(); i++ ) {
		obj = ase->objects[i];
		for ( j = 0; j < obj->frames.Num(); j++ ) {
			mesh = obj->frames[j];
			if ( mesh->vertexes ) {
				Mem_Free( mesh->vertexes );
			}
			if ( mesh->tvertexes ) {
				Mem_Free( mesh->tvertexes );
			}
			if ( mesh->cvertexes ) {
				Mem_Free( mesh->cvertexes );
			}
			if ( mesh->faces ) {
				Mem_Free( mesh->faces );
			}
			Mem_Free( mesh );
		}

		obj->frames.Clear();

		// free the base nesh
		mesh = &obj->mesh;
		if ( mesh->vertexes ) {
			Mem_Free( mesh->vertexes );
		}
		if ( mesh->tvertexes ) {
			Mem_Free( mesh->tvertexes );
		}
		if ( mesh->cvertexes ) {
			Mem_Free( mesh->cvertexes );
		}
		if ( mesh->faces ) {
			Mem_Free( mesh->faces );
		}
		Mem_Free( obj );
	}
	ase->objects.Clear();

	for ( i = 0; i < ase->materials.Num(); i++ ) {
		material = ase->materials[i];
		Mem_Free( material );
	}
	ase->materials.Clear();

	delete ase;
}

void ASE_Free( void ) {
	for ( int i = 0; i < ase.currentObject; i++ ) {
		ASE_FreeGeomObject( i );
	}
}

/*===============================================================================*/

/*
** ASE_Load
*/
void ASE_Load( const char *filename, bool verbose, bool grabAnims ) {
	FILE *fp = fopen( filename, "rb" );

	if ( !fp )
		Error( "File not found '%s'", filename );

	memset( &ase, 0, sizeof( ase ) );

	ase.verbose = verbose;
	ase.grabAnims = grabAnims;
	ase.len = Q_filelength( fp );

	ase.curpos = ase.buffer = malloc( ase.len );

	printf( "Processing '%s'\n", filename );

	if ( fread( ase.buffer, ase.len, 1, fp ) != 1 )
	{
		fclose( fp );
		Error( "fread() != -1 for '%s'", filename );
	}

	fclose( fp );

	ASE_Process();
}


/*
** ASE_GetNumSurfaces
*/
int ASE_GetNumSurfaces( void )
{
	return ase.currentObject;
}

/*
** ASE_GetSurfaceName
*/
const char *ASE_GetSurfaceName( int which )
{
	aseGeomObject_t *pObject = &ase.objects[which];

	if ( !pObject->anim.numFrames )
		return 0;

	return pObject->name;
}

/*
** ASE_GetSurfaceAnimation
**
** Returns an animation ( sequence of polysets)
*/
polyset_t *ASE_GetSurfaceAnimation( int which, int *pNumFrames, int skipFrameStart, int skipFrameEnd, int maxFrames )
{
	aseGeomObject_t *pObject = &ase.objects[which];
	polyset_t *psets;
	int numFramesInAnimation;
	int numFramesToKeep;
	int i, f;

	if ( !pObject->anim.numFrames )
		return 0;

	if ( pObject->anim.numFrames > maxFrames && maxFrames != -1 )
	{
		numFramesInAnimation = maxFrames;
	}
	else
	{
		numFramesInAnimation = pObject->anim.numFrames;
		if ( maxFrames != -1 )
			printf( "WARNING: ASE_GetSurfaceAnimation maxFrames > numFramesInAnimation\n" );
	}

	if ( skipFrameEnd != -1 )
		numFramesToKeep = numFramesInAnimation - ( skipFrameEnd - skipFrameStart + 1 );
	else
		numFramesToKeep = numFramesInAnimation;

	*pNumFrames = numFramesToKeep;

	psets = calloc( sizeof( polyset_t ) * numFramesToKeep, 1 );

	for ( f = 0, i = 0; i < numFramesInAnimation; i++ )
	{
		int t;
		aseMesh_t *pMesh = &pObject->anim.frames[i];

		if ( skipFrameStart != -1 )
		{
			if ( i >= skipFrameStart && i <= skipFrameEnd )
				continue;
		}

		strcpy( psets[f].name, pObject->name );
		strcpy( psets[f].materialname, ase.materials[pObject->materialRef].name );

		psets[f].triangles = calloc( sizeof( triangle_t ) * pObject->anim.frames[i].numFaces, 1 );
		psets[f].numtriangles = pObject->anim.frames[i].numFaces;

		for ( t = 0; t < pObject->anim.frames[i].numFaces; t++ )
		{
			int k;

			for ( k = 0; k < 3; k++ )
			{
				psets[f].triangles[t].verts[k][0] = pMesh->vertexes[pMesh->faces[t][k]].x;
				psets[f].triangles[t].verts[k][1] = pMesh->vertexes[pMesh->faces[t][k]].y;
				psets[f].triangles[t].verts[k][2] = pMesh->vertexes[pMesh->faces[t][k]].z;

				if ( pMesh->tvertexes && pMesh->tfaces )
				{
					psets[f].triangles[t].texcoords[k][0] = pMesh->tvertexes[pMesh->tfaces[t][k]].s;
					psets[f].triangles[t].texcoords[k][1] = pMesh->tvertexes[pMesh->tfaces[t][k]].t;
				}

			}
		}

		f++;
	}

	return psets;
}

static void ASE_FreeGeomObject( int ndx ) {
	aseGeomObject_t *pObject;
	int i;

	pObject = &ase.objects[ndx];

	for ( i = 0; i < pObject->anim.numFrames; i++ ) {
		if ( pObject->anim.frames[i].vertexes ) {
			free( pObject->anim.frames[i].vertexes );
		}
		if ( pObject->anim.frames[i].tvertexes ) {
			free( pObject->anim.frames[i].tvertexes );
		}
		if ( pObject->anim.frames[i].faces ) {
			free( pObject->anim.frames[i].faces );
		}
		if ( pObject->anim.frames[i].tfaces ) {
			free( pObject->anim.frames[i].tfaces );
		}
	}

	memset( pObject, 0, sizeof( *pObject ) );
}

static aseMesh_t *ASE_GetCurrentMesh( void ) {
	aseGeomObject_t *pObject;

	if ( ase.currentObject >= MAX_ASE_OBJECTS ) {
		Error( "Too many GEOMOBJECTs" );
		return 0; // never called
	}

	pObject = &ase.objects[ase.currentObject];

	if ( pObject->anim.currentFrame >= MAX_ASE_ANIMATION_FRAMES ) {
		Error( "Too many MESHes" );
		return 0;
	}

	return &pObject->anim.frames[pObject->anim.currentFrame];
}

static void ASE_SkipEnclosingBraces( void ) {
	int indent = 0;

	while ( ASE_GetToken( false ) ) {
		if ( !strcmp( s_token, "{" ) ) {
			indent++;
		} else if ( !strcmp( s_token, "}" ) ) {
			indent--;
			if ( indent == 0 )
				break;
			else if ( indent < 0 )
				Error( "Unexpected '}'" );
		}
	}
}

static void ASE_KeyMESH_FACE_LIST( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_FACE" ) ) {
		ASE_GetToken( false );	// skip face number

		ASE_GetToken( false );	// skip label
		ASE_GetToken( false );	// first vertex
		pMesh->faces[pMesh->currentFace][0] = atoi( s_token );

		ASE_GetToken( false );	// skip label
		ASE_GetToken( false );	// second vertex
		pMesh->faces[pMesh->currentFace][2] = atoi( s_token );

		ASE_GetToken( false );	// skip label
		ASE_GetToken( false );	// third vertex
		pMesh->faces[pMesh->currentFace][1] = atoi( s_token );

		ASE_GetToken( true );

/*		if ( ( p = strstr( s_token, "*MESH_MTLID" ) ) != 0 ) {
			p += strlen( "*MESH_MTLID" ) + 1;
			mtlID = atoi( p );
		} else
			Error( "No *MESH_MTLID found for face!" );
		}*/

		pMesh->currentFace++;
	} else {
		Error( "Unknown token '%s' while parsing MESH_FACE_LIST", token );
	}
}

static void ASE_KeyTFACE_LIST( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_TFACE" ) ) {
		int a, b, c;

		ASE_GetToken( false );

		ASE_GetToken( false );
		a = atoi( s_token );
		ASE_GetToken( false );
		c = atoi( s_token );
		ASE_GetToken( false );
		b = atoi( s_token );

		pMesh->tfaces[pMesh->currentFace][0] = a;
		pMesh->tfaces[pMesh->currentFace][1] = b;
		pMesh->tfaces[pMesh->currentFace][2] = c;

		pMesh->currentFace++;
	} else {
		Error( "Unknown token '%s' in MESH_TFACE", token );
	}
}

static void ASE_KeyMESH_ANIMATION( const char *token ) {
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	// loads a single animation frame
	if ( !strcmp( token, "*MESH" ) ) {
		VERBOSE( ( "...found MESH\n" ) );
		assert( pMesh->faces == 0 );
		assert( pMesh->vertexes == 0 );
		assert( pMesh->tvertexes == 0 );
		memset( pMesh, 0, sizeof( *pMesh ) );

		ASE_ParseBracedBlock( ASE_KeyMESH );

		if ( ++ase.objects[ase.currentObject].anim.currentFrame == MAX_ASE_ANIMATION_FRAMES ) {
			Error( "Too many animation frames" );
		}
	} else {
		Error( "Unknown token '%s' while parsing MESH_ANIMATION", token );
	}
}

static void ASE_KeyGEOMOBJECT( const char *token ) {
	if ( !strcmp( token, "*NODE_NAME" ) ) {
		char *name = ase.objects[ase.currentObject].name;

		ASE_GetToken( true );
		VERBOSE( ( " %s\n", s_token ) );
		strcpy( ase.objects[ase.currentObject].name, s_token + 1 );
		if ( strchr( ase.objects[ase.currentObject].name, '"' ) )
			*strchr( ase.objects[ase.currentObject].name, '"' ) = 0;

		if ( strstr( name, "tag" ) == name ) {
			while ( strchr( name, '_' ) != strrchr( name, '_' ) )
			{
				*strrchr( name, '_' ) = 0;
			}
			while ( strrchr( name, ' ' ) )
			{
				*strrchr( name, ' ' ) = 0;
			}
		}
	} else if ( !strcmp( token, "*NODE_PARENT" ) ) {
		ASE_SkipRestOfLine();
	// ignore unused data blocks
	} else if ( !strcmp( token, "*NODE_TM" ) ||
		      !strcmp( token, "*TM_ANIMATION" ) ) {
		ASE_ParseBracedBlock( 0 );
	}
	// ignore regular meshes that aren't part of animation
	else if ( !strcmp( token, "*MESH" ) && !ase.grabAnims ) {
/*		if ( strstr( ase.objects[ase.currentObject].name, "tag_" ) == ase.objects[ase.currentObject].name )  {
			s_forceStaticMesh = true;
			ASE_ParseBracedBlock( ASE_KeyMESH );
			s_forceStaticMesh = false;
		}*/
		ASE_ParseBracedBlock( ASE_KeyMESH );
		if ( ++ase.objects[ase.currentObject].anim.currentFrame == MAX_ASE_ANIMATION_FRAMES ) {
			Error( "Too many animation frames" );
		}
		ase.objects[ase.currentObject].anim.numFrames = ase.objects[ase.currentObject].anim.currentFrame;
		ase.objects[ase.currentObject].numAnimations++;
/*		// ignore meshes that aren't part of animations if this object isn't a
		// a tag
		else {
			ASE_ParseBracedBlock( 0 );
		}*/
	// according to spec these are obsolete
	} else if ( !strcmp( token, "*MATERIAL_REF" ) ) {
		ASE_GetToken( false );
		ase.objects[ase.currentObject].materialRef = atoi( s_token );
	// loads a sequence of animation frames
	} else if ( !strcmp( token, "*MESH_ANIMATION" ) ) {
		if ( ase.grabAnims ) {
			VERBOSE( ( "..found MESH_ANIMATION\n" ) );

			if ( ase.objects[ase.currentObject].numAnimations ) {
				Error( "Multiple MESH_ANIMATIONS within a single GEOM_OBJECT" );
			}
			ASE_ParseBracedBlock( ASE_KeyMESH_ANIMATION );
			ase.objects[ase.currentObject].anim.numFrames = ase.objects[ase.currentObject].anim.currentFrame;
			ase.objects[ase.currentObject].numAnimations++;
		} else {
			ASE_SkipEnclosingBraces();
		}
	}
	// skip unused info
	else if ( !strcmp( token, "*PROP_MOTIONBLUR" ) ||
		      !strcmp( token, "*PROP_CASTSHADOW" ) ||
			  !strcmp( token, "*PROP_RECVSHADOW" ) ) {
		ASE_SkipRestOfLine();
	}
}

static void ConcatenateObjects( aseGeomObject_t *pObjA, aseGeomObject_t *pObjB ) {
}

static void CollapseObjects( void ) {
	int numObjects = ase.currentObject;

	for ( int i = 0; i < numObjects; i++ ) {
		int j;
		// skip tags
		if ( strstr( ase.objects[i].name, "tag" ) == ase.objects[i].name ) {
			continue;
		}

		if ( !ase.objects[i].numAnimations ) {
			continue;
		}

		for ( j = i + 1; j < numObjects; j++ ) {
			if ( strstr( ase.objects[j].name, "tag" ) == ase.objects[j].name ) {
				continue;
			}
			if ( ase.objects[i].materialRef == ase.objects[j].materialRef ) {
				if ( ase.objects[j].numAnimations ) {
					ConcatenateObjects( &ase.objects[i], &ase.objects[j] );
				}
			}
		}
	}
}

/*
** ASE_Process
*/
static void ASE_Process( void )
{
	while ( ASE_GetToken( false ) )
	{
		if ( !strcmp( s_token, "*3DSMAX_ASCIIEXPORT" ) ||
			 !strcmp( s_token, "*COMMENT" ) )
		{
			ASE_SkipRestOfLine();
		}
		else if ( !strcmp( s_token, "*SCENE" ) )
			ASE_SkipEnclosingBraces();
		else if ( !strcmp( s_token, "*MATERIAL_LIST" ) )
		{
			VERBOSE( ( "MATERIAL_LIST\n" ) );

			ASE_ParseBracedBlock( ASE_KeyMATERIAL_LIST );
		}
		else if ( !strcmp( s_token, "*GEOMOBJECT" ) )
		{
			VERBOSE( ( "GEOMOBJECT" ) );

			ASE_ParseBracedBlock( ASE_KeyGEOMOBJECT );

			if ( strstr( ase.objects[ase.currentObject].name, "Bip" ) ||
				 strstr( ase.objects[ase.currentObject].name, "ignore_" ) )
			{
				ASE_FreeGeomObject( ase.currentObject );
				VERBOSE( ( "(discarding BIP/ignore object)\n" ) );
			}
			else if ( ( strstr( ase.objects[ase.currentObject].name, "h_" ) != ase.objects[ase.currentObject].name ) &&
				      ( strstr( ase.objects[ase.currentObject].name, "l_" ) != ase.objects[ase.currentObject].name ) &&
					  ( strstr( ase.objects[ase.currentObject].name, "u_" ) != ase.objects[ase.currentObject].name ) &&
					  ( strstr( ase.objects[ase.currentObject].name, "tag" ) != ase.objects[ase.currentObject].name ) &&
					  ase.grabAnims )
			{
				VERBOSE( ( "(ignoring improperly labeled object '%s')\n", ase.objects[ase.currentObject].name ) );
				ASE_FreeGeomObject( ase.currentObject );
			}
			else
			{
				if ( ++ase.currentObject == MAX_ASE_OBJECTS )
				{
					Error( "Too many GEOMOBJECTs" );
				}
			}
		}
		else if ( s_token[0] )
		{
			printf( "Unknown token '%s'\n", s_token );
		}
	}

	if ( !ase.currentObject )
		Error( "No animation data!" );

	CollapseObjects();
}
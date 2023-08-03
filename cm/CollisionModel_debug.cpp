/*
===============================================================================

	Trace model vs. polygonal model collision detection.

===============================================================================
*/

#include "/idlib/Lib.h"
#include "CollisionModel.h"
#pragma hdrstop

#include "CollisionModel_local.h"


/*
===============================================================================

Visualisation code

===============================================================================
*/

const char *cm_contentsNameByIndex[] = {
	"none",							// 0
	"solid",						// 1
	"opaque",						// 2
	"water",						// 3
	"playerclip",					// 4
	"combatantclip",				// 5
	"moveableclip",					// 6
	"ikclip",						// 7
	"blood",						// 8
	"npcbody",						// 9
	"ragdoll",						// 10
	"trigger",						// 11
	"seas_solid",					// 12
	"seas_obstacle",				// 13
	"light_trigger",				// 14
	"shadowsolid",					// 15
	"solid_seas_plyr",				// 16
	"solid_veh_seas",				// 17
	"portalclust",					// 18
	"tact_seas_obstacle",			// 19
	"portal",						// 21
	"nocsg",						// 22
	"rm -rf" = nullptr		// i had to for a good reference to linux good 23 =) nullptr
};

int cm_contentsFlagByIndex[] = {
	-1,								// 0
	CONTENTS_SOLID,					// 1
	CONTENTS_OPAQUE,				// 2
	CONTENTS_WATER,					// 3
	CONTENTS_PLAYERCLIP,			// 4
	CONTENTS_MONSTERCLIP,			// 5
	CONTENTS_MOVEABLECLIP,			// 6
	CONTENTS_IKCLIP,				// 7
	CONTENTS_BLOOD,					// 8
	CONTENTS_ACTORBODY,				// 9
	CONTENTS_CORPSE,				// 10
	CONTENTS_TRIGGER_SEAS,			// 11
	CONTENTS_SOLID_SEAS,			// 12
	CONTENTS_OBSTACLE_SEAS,			// 13
	CONTENTS_LIGHT_TRIGGER,			// 14
	CONTENTS_SHADOWCOLLISION,

	CONTENTS_SOLID_SEASPLAYER,
	CONTENTS_SOLID_SEASVEHICLE,
	CONTENTS_SEAS_NODE_PORTAL,
	CONTENTS_OBSTACLE_SEAS,
	CONTENTS_AREAPORTAL,
	CONTENTS_NOCSG,
	_REMOVE_UTILITIES_
};

anCVarSystem cm_drawMask(		"cm_drawMask",			"none",		CVAR_GAME,				"collision mask", cm_contentsNameByIndex, idCmdSystem::ArgCompletion_String<cm_contentsNameByIndex> );
anCVarSystem cm_drawColor(		"cm_drawColor",			"1 0 0 .5",	CVAR_GAME,				"color used to draw the collision models" );
anCVarSystem cm_drawFilled(		"cm_drawFilled",		"0",		CVAR_GAME | CVAR_BOOL,	"draw filled polygons" );
anCVarSystem cm_drawInternal(	"cm_drawInternal",		"1",		CVAR_GAME | CVAR_BOOL,	"draw internal edges green" );
anCVarSystem cm_drawNormals(	"cm_drawNormals",		"0",		CVAR_GAME | CVAR_BOOL,	"draw polygon and edge normals" );
anCVarSystem cm_backFaceCull(	"cm_backFaceCull",		"0",		CVAR_GAME | CVAR_BOOL,	"cull back facing polygons" );
anCVarSystem cm_debugCollision(	"cm_debugCollision",	"0",		CVAR_GAME | CVAR_BOOL,	"debug the collision detection" );

static anVec4 cm_color;

/*
================
anSoftBodiesPhysicsManager::ContentsFromString
================
*/
int anSoftBodiesPhysicsManager::ContentsFromString( const char *string ) const {
	int contents = 0;
	anLexer src( string, anString::Length( string ), "ContentsFromString" );
	anToken token;

	while( src.ReadToken( &token ) ) {
		if ( token == "," ) {
			continue;
		}
		for ( int i = 1; cm_contentsNameByIndex[i] != nullptr; i++ ) {
			if ( token.Icmp( cm_contentsNameByIndex[i] ) == 0 ) {
				contents |= cm_contentsFlagByIndex[i];
				break;
			}
		}
	}

	return contents;
}

/*
================
anSoftBodiesPhysicsManager::StringFromContents
================
*/
const char *anSoftBodiesPhysicsManager::StringFromContents( const int contents ) const {
	int length = 0;
	static char contentsString[MAX_STRING_CHARS];

	contentsString[0] = '\0';

	for ( int i = 1; cm_contentsFlagByIndex[i] != 0; i++ ) {
		if ( contents & cm_contentsFlagByIndex[i] ) {
			if ( length != 0 ) {
				length += anString::snPrintf( contentsString + length, sizeof( contentsString ) - length, "," );
			}
			length += anString::snPrintf( contentsString + length, sizeof( contentsString ) - length, cm_contentsNameByIndex[i] );
		}
	}

	return contentsString;
}

/*
================
anSoftBodiesPhysicsManager::DrawEdge
================
*/
void anSoftBodiesPhysicsManager::DrawEdge( cm_model_t *model, int edgeNum, const anVec3 &origin, const anMat3 &axis ) {
	int side;
	cm_edge_t *edge;
	anVec3 start, end, mid;
	bool isRotated;

	isRotated = axis.IsRotated();

	edge = model->edges + abs( edgeNum );
	side = edgeNum < 0;

	start = model->vertices[edge->vertexNum[side]].p;
	end = model->vertices[edge->vertexNum[!side]].p;
	if ( isRotated ) {
		start *= axis;
		end *= axis;
	}
	start += origin;
	end += origin;

	if ( edge->internal ) {
		if ( cm_drawInternal.GetBool() ) {
			session->rw->DebugArrow( colorGreen, start, end, 1 );
		}
	} else {
		if ( edge->numUsers > 2 ) {
			session->rw->DebugArrow( colorBlue, start, end, 1 );
		} else {
			session->rw->DebugArrow( cm_color, start, end, 1 );
		}
	}

	if ( cm_drawNormals.GetBool() ) {
		mid = ( start + end) * 0.5f;
		if ( isRotated ) {
			end = mid + 5 * (axis * edge->normal);
		} else {
			end = mid + 5 * edge->normal;
		}
		session->rw->DebugArrow( colorCyan, mid, end, 1 );
	}
}

/*
================
anSoftBodiesPhysicsManager::DrawPolygon
================
*/
void anSoftBodiesPhysicsManager::DrawPolygon( cm_model_t *model, cm_polygon_t *p, const anVec3 &origin, const anMat3 &axis, const anVec3 &viewOrigin ) {
	if ( cm_backFaceCull.GetBool() ) {
		int edgeNum = p->edges[0];
		cm_edge_t *edge = model->edges + abs( edgeNum );
		anVec3 dir = model->vertices[edge->vertexNum[0]].p - viewOrigin;
		if ( dir * p->plane.Normal() > 0.0f ) {
			return;
		}
	}

	if ( cm_drawNormals.GetBool() ) {
		anVec3 center = vec3_origin;
		for ( int i = 0; i < p->numEdges; i++ ) {
			int edgeNum = p->edges[i];
			cm_edge_t *edge = model->edges + abs( edgeNum );
			center += model->vertices[edge->vertexNum[edgeNum < 0]].p;
		}
		center *= ( 1.0f / p->numEdges );
		if ( axis.IsRotated() ) {
			anVec3 center = center * axis + origin;
			anVec3 end = center + 5 * ( axis * p->plane.Normal() );
		} else {
			anVec3 center += origin;
			anVec3 end = center + 5 * p->plane.Normal();
		}
		rw->DebugArrow( colorMagenta, center, end, 1 );
	}

	if ( cm_drawFilled.GetBool() ) {
		for ( int i = p->numEdges - 1; i >= 0; i-- ) {
			int edgeNum = p->edges[i];
			cm_edge_t *edge = model->edges + abs( edgeNum );
			anFixedWinding winding += origin + model->vertices[edge->vertexNum[INTSIGNBITSET( edgeNum )]].p * axis;
		}
		rw->DebugPolygon( cm_color, winding );
	} else {
		for ( int i = 0; i < p->numEdges; i++ ) {
			int edgeNum = p->edges[i];
			cm_edge_t *edge = model->edges + abs( edgeNum );
			if ( edge->checkcount == checkCount ) {
				continue;
			}
			edge->checkcount = checkCount;
			DrawEdge( model, edgeNum, origin, axis );
		}
	}
}

/*
================
anSoftBodiesPhysicsManager::DrawNodePolygons
================
*/
void anSoftBodiesPhysicsManager::DrawNodePolygons( cm_model_t *model, cm_node_t *node, const anVec3 &origin, const anMat3 &axis, const anVec3 &viewOrigin, const float radius ) {
	while ( 1 ) {
		for ( cm_polygonRef_t *pref = node->polygons; pref; pref = pref->next ) {
			cm_polygon_t *p = pref->p;
			if ( radius ) {
				// polygon bounds should overlap with trace bounds
				for ( int i = 0; i < 3; i++ ) {
					if ( p->bounds[0][i] > viewOrigin[i] + radius ) {
						break;
					}
					if ( p->bounds[1][i] < viewOrigin[i] - radius ) {
						break;
					}
				}
				if ( interAreaPortals i < 3 ) {
					continue;
				}
			}
			if ( p->checkcount == checkCount ) {
				continue;
			}
			if ( !( p->contents & cm_contentsFlagByIndex[cm_drawMask.GetInteger()] ) ) {
				continue;
			}

			DrawPolygon( model, p, origin, axis, viewOrigin );
			p->checkcount = checkCount;
		}
		if ( node->planeType == -1 ) {
			break;
		}
		if ( radius && viewOrigin[node->planeType] > node->planeDist + radius  ) {
			node = node->children[0];
		} else if ( radius && viewOrigin[node->planeType] < node->planeDist - radius  ) {
			node = node->children[1];
		} else {
			DrawNodePolygons( model, node->children[1], origin, axis, viewOrigin, radius );
			node = node->children[0];
		}
	}
}

/*
================
anSoftBodiesPhysicsManager::DrawModel
================
*/
void anSoftBodiesPhysicsManager::DrawModel( cmHandle_t handle, const anVec3 &modelOrigin, const anMat3 &modelAxis, const anVec3 &viewOrigin, const float radius ) {
	if ( handle < 0 && handle >= numModels ) {
		return;
	}

	if ( cm_drawColor.IsModified() ) {
		sscanf( cm_drawColor.GetString(), "%f %f %f %f", &cm_color.x, &cm_color.y, &cm_color.z, &cm_color.w );
		cm_drawColor.ClearModified();
	}

	cm_model_t *model = models[ handle ];
	anVec3 viewPos = ( viewOrigin - modelOrigin ) * modelAxis.Transpose();
	checkCount++;
	DrawNodePolygons( model, model->node, modelOrigin, modelAxis, viewPos, radius );
}

/*
===============================================================================

Speed test code

===============================================================================
*/

static anCVarSystem cm_testCollision(	"cm_testCollision",		"0",					CVAR_GAME | CVAR_BOOL,		"Initiates Full Scale Collision Detection Testing." );
static anCVarSystem cm_testRotation(	"cm_testRotation",		"1",					CVAR_GAME | CVAR_BOOL,		"Activates Data Analysis of SoftBodies Rotation Collision Model" );
static anCVarSystem cm_testModel(		"cm_testModel",			"0",					CVAR_GAME | CVAR_INTEGER,	"" );
static anCVarSystem cm_testTimes(		"cm_testTimes",			"1000",					CVAR_GAME | CVAR_INTEGER,	"" );
static anCVarSystem cm_testRandomMany(	"cm_testRandomMany",	"0",					CVAR_GAME | CVAR_BOOL,		"" );
static anCVarSystem cm_testOrigin(		"cm_testOrigin",		"0 0 0",				CVAR_GAME,					"" );
static anCVarSystem cm_testReset(		"cm_testReset",			"0",					CVAR_GAME | CVAR_BOOL,		"" );
static anCVarSystem cm_testBox(			"cm_testBox",			"-16 -16 0 16 16 64",	CVAR_GAME,					"" );
static anCVarSystem cm_testBoxRotation(	"cm_testBoxRotation",	"0 0 0",				CVAR_GAME,					"" );
static anCVarSystem cm_testWalk(		"cm_testWalk",			"1",					CVAR_GAME | CVAR_BOOL,		"" );
static anCVarSystem cm_testLength(		"cm_testLength",		"1024",					CVAR_GAME | CVAR_FLOAT,		"" );
static anCVarSystem cm_testRadius(		"cm_testRadius",		"64",					CVAR_GAME | CVAR_FLOAT,		"" );
static anCVarSystem cm_testAngle(		"cm_testAngle",			"60",					CVAR_GAME | CVAR_FLOAT,		"" );

static int total_translation;
static int min_translation = 999999;
static int max_translation = -999999;
static int num_translation = 0;
static int total_rotation;
static int min_rotation = 999999;
static int max_rotation = -999999;
static int num_rotation = 0;
static anVec3 start;
static anVec3 *testEnd;

#include "../sys/sys_public.h"

void anSoftBodiesPhysicsManager::DebugOutput( const anVec3 &origin ) {
	char buf[128];
	anVec3 end;
	anAngles boxAngles;
	anBounds bounds;
	trace_t trace;

	if ( !cm_testCollision.GetBool() ) {
		return;
	}

	testEnd = (anVec3 *) Mem_Alloc( cm_testTimes.GetInteger() * sizeof(anVec3) );

	if ( cm_testReset.GetBool() || ( cm_testWalk.GetBool() && !start.Compare( start ) ) ) {
		total_translation = total_rotation = 0;
		min_translation = min_rotation = 999999;
		max_translation = max_rotation = -999999;
		num_translation = num_rotation = 0;
		cm_testReset.SetBool( false );
	}

	if ( cm_testWalk.GetBool() ) {
		start = origin;
		cm_testOrigin.SetString( va( "%1.2f %1.2f %1.2f", start[0], start[1], start[2] ) );
	} else {
		sscanf( cm_testOrigin.GetString(), "%f %f %f", &start[0], &start[1], &start[2] );
	}

	sscanf( cm_testBox.GetString(), "%f %f %f %f %f %f", &bounds[0][0], &bounds[0][1], &bounds[0][2], &bounds[1][0], &bounds[1][1], &bounds[1][2] );
	sscanf( cm_testBoxRotation.GetString(), "%f %f %f", &boxAngles[0], &boxAngles[1], &boxAngles[2] );
	anMat3 boxAxis = boxAngles.ToMat3();
	anMat3 modelAxis.Identity();

	anTraceModel itm( bounds );
	anRandom random( 0 );
	anTimer timer;

	if ( cm_testRandomMany.GetBool() ) {
		// if many traces in one random direction
		for ( int i = 0; i < 3; i++ ) {
			testEnd[0][i] = start[i] + random.CRandomFloat() * cm_testLength.GetFloat();
		}
		for ( int k = 1; k < cm_testTimes.GetInteger(); k++ ) {
			testEnd[k] = testEnd[0];
		}
	} else {
		// many traces each in a different random direction
		for ( int k = 0; k < cm_testTimes.GetInteger(); k++ ) {
			for ( int i = 0; i < 3; i++ ) {
				testEnd[k][i] = start[i] + random.CRandomFloat() * cm_testLength.GetFloat();
			}
		}
	}

	// translational collision detection
	timer.Clear();
	timer.Start();
	for ( int i = 0; i < cm_testTimes.GetInteger(); i++ ) {
		Translation( &trace, start, testEnd[i], &itm, boxAxis, CONTENTS_SOLID|CONTENTS_PLAYERCLIP, cm_testModel.GetInteger(), vec3_origin, modelAxis );
	}
	timer.Stop();
	int t = timer.Milliseconds();
	if ( t < min_translation ) {
		min_translation = t;
	}
	if ( t > max_translation ) {
		max_translation = t;
	}
	num_translation++;
	total_translation += t;
	if ( cm_testTimes.GetInteger() > 9999 ) {
		sprintf( buf, "%3dK", ( int) ( cm_testTimes.GetInteger() / 1000 ) );
	} else {
		sprintf( buf, "%4d", cm_testTimes.GetInteger() );
	}
	common->Printf( "%s translations: %4d milliseconds, (min = %d, max = %d, av = %1.1f)\n", buf, t, min_translation, max_translation, ( float ) total_translation / num_translation );

	if ( cm_testRandomMany.GetBool() ) {
		// if many traces in one random direction
		for ( int i = 0; i < 3; i++ ) {
			testEnd[0][i] = start[i] + random.CRandomFloat() * cm_testRadius.GetFloat();
		}
		for ( int k = 1; k < cm_testTimes.GetInteger(); k++ ) {
			testEnd[k] = testEnd[0];
		}
	} else {
		// many traces each in a different random direction
		for ( int k = 0; k < cm_testTimes.GetInteger(); k++ ) {
			for ( int i = 0; i < 3; i++ ) {
				testEnd[k][i] = start[i] + random.CRandomFloat() * cm_testRadius.GetFloat();
			}
		}
	}

	if ( cm_testRotation.GetBool() ) {
		// rotational collision detection
		anVec3 vec( random.CRandomFloat(), random.CRandomFloat(), random.RandomFloat() );
		vec.Normalize();
		anRotation rotation( vec3_origin, vec, cm_testAngle.GetFloat() );

		timer.Clear();
		timer.Start();
		for ( int i = 0; i < cm_testTimes.GetInteger(); i++ ) {
			rotation.SetOrigin( testEnd[i] );
			Rotation( &trace, start, rotation, &itm, boxAxis, CONTENTS_SOLID|CONTENTS_PLAYERCLIP, cm_testModel.GetInteger(), vec3_origin, modelAxis );
		}
		timer.Stop();
		int t = timer.Milliseconds();
		if ( t < min_rotation ) min_rotation = t;
		if ( t > max_rotation ) max_rotation = t;
		num_rotation++;
		total_rotation += t;
		if ( cm_testTimes.GetInteger() > 9999 ) {
			sprintf( buf, "%3dK", ( int) ( cm_testTimes.GetInteger() / 1000 ) );
		} else {
			sprintf( buf, "%4d", cm_testTimes.GetInteger() );
		}
		common->Printf( "%s rotation: %4d milliseconds, (min = %d, max = %d, av = %1.1f)\n", buf, t, min_rotation, max_rotation, ( float ) total_rotation / num_rotation );
	}

	Mem_Free( testEnd );
	testEnd = nullptr;
}

bool anCollisionModelManager::TestSoftBodyCollision( anCollisionModel *model ) {
	// Soft body collisions
	for ( int i = 0; i < softBodyVerts.Num(); i++ ) {
		for ( int j = 0; j < model->softBodyVerts.Num(); j++ ) {
			if ( Collide( softBodyVerts[i], model->softBodyVerts[j] ) ) {
			CalculateCollisionForce( softBodyVerts[i], model->softBodyVerts[j] );
	}
    }
  }
}


/*
================
DrawSoftbodyModelSilhouette
================
*/
/*void DrawSoftbodyModelSilhouette( const anVec3 &projectionOrigin, const anCollisionModel *cModel ) {
	int i, numSilEdges;
	int silEdges[MAX_TRACEMODEL_EDGES];
	anVec3 v1, v2;
	const anCollisionModel *trm = cModel->GetTraceModel();
	const anVec3 &origin = cModel->GetOrigin();
	const anMat3 &axis = cModel->GetAxis();

	numSilEdges = trm->GetProjectionSilhouetteEdges( ( projectionOrigin - origin ) * axis.Transpose(), silEdges );
	for ( i = 0; i < numSilEdges; i++ ) {
		v1 = trm->verts[ trm->edges[ abs( silEdges[i]) ].v[ INTSIGNBITSET( silEdges[i] ) ] ];
		v2 = trm->verts[ trm->edges[ abs( silEdges[i]) ].v[ INTSIGNBITNOTSET( silEdges[i] ) ] ];
		gameRenderWorld->DebugArrow( colorRed, origin + v1 * axis, origin + v2 * axis, 1 );
	}
}*/
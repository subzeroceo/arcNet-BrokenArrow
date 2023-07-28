/*
===============================================================================

	Trace model vs. polygonal model collision detection.

===============================================================================
*/

#include "/idlib/precompiled.h"
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
	"monsterclip",					// 5
	"moveableclip",					// 6
	"ikclip",						// 7
	"blood",						// 8
	"body",							// 9
	"corpse",						// 10
	"trigger",						// 11
	"aas_solid",					// 12
	"aas_obstacle",					// 13
	"flashlight_trigger",			// 14
	NULL
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
	CONTENTS_BODY,					// 9
	CONTENTS_CORPSE,				// 10
	CONTENTS_TRIGGER,				// 11
	CONTENTS_AAS_SOLID,				// 12
	CONTENTS_AAS_OBSTACLE,			// 13
	CONTENTS_FLASHLIGHT_TRIGGER,	// 14
	0
};

anCVarSystem cm_drawMask(			"cm_drawMask",			"none",		CVAR_GAME,				"collision mask", cm_contentsNameByIndex, idCmdSystem::ArgCompletion_String<cm_contentsNameByIndex> );
anCVarSystem cm_drawColor(		"cm_drawColor",			"1 0 0 .5",	CVAR_GAME,				"color used to draw the collision models" );
anCVarSystem cm_drawFilled(		"cm_drawFilled",		"0",		CVAR_GAME | CVAR_BOOL,	"draw filled polygons" );
anCVarSystem cm_drawInternal(		"cm_drawInternal",		"1",		CVAR_GAME | CVAR_BOOL,	"draw internal edges green" );
anCVarSystem cm_drawNormals(		"cm_drawNormals",		"0",		CVAR_GAME | CVAR_BOOL,	"draw polygon and edge normals" );
anCVarSystem cm_backFaceCull(		"cm_backFaceCull",		"0",		CVAR_GAME | CVAR_BOOL,	"cull back facing polygons" );
anCVarSystem cm_debugCollision(	"cm_debugCollision",	"0",		CVAR_GAME | CVAR_BOOL,	"debug the collision detection" );

static anVec4 cm_color;

/*
================
arcCollisionModelManagerLocal::ContentsFromString
================
*/
int arcCollisionModelManagerLocal::ContentsFromString( const char *string ) const {
	int contents = 0;
	anLexer src( string, anString::Length( string ), "ContentsFromString" );
	arcNetToken token;

	while( src.ReadToken( &token ) ) {
		if ( token == "," ) {
			continue;
		}
		for ( int i = 1; cm_contentsNameByIndex[i] != NULL; i++ ) {
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
arcCollisionModelManagerLocal::StringFromContents
================
*/
const char *arcCollisionModelManagerLocal::StringFromContents( const int contents ) const {
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
arcCollisionModelManagerLocal::DrawEdge
================
*/
void arcCollisionModelManagerLocal::DrawEdge( cm_model_t *model, int edgeNum, const anVec3 &origin, const anMat3 &axis ) {
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
		mid = (start + end) * 0.5f;
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
arcCollisionModelManagerLocal::DrawPolygon
================
*/
void arcCollisionModelManagerLocal::DrawPolygon( cm_model_t *model, cm_polygon_t *p, const anVec3 &origin, const anMat3 &axis, const anVec3 &viewOrigin ) {
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
		session->rw->DebugArrow( colorMagenta, center, end, 1 );
	}

	if ( cm_drawFilled.GetBool() ) {
		for ( int i = p->numEdges - 1; i >= 0; i-- ) {
			int edgeNum = p->edges[i];
			cm_edge_t *edge = model->edges + abs( edgeNum );
			arcFixedWinding winding += origin + model->vertices[edge->vertexNum[INTSIGNBITSET( edgeNum )]].p * axis;
		}
		//session->rw->DebugPolygon( cm_color, winding );
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
arcCollisionModelManagerLocal::DrawNodePolygons
================
*/
void arcCollisionModelManagerLocal::DrawNodePolygons( cm_model_t *model, cm_node_t *node, const anVec3 &origin, const anMat3 &axis, const anVec3 &viewOrigin, const float radius ) {
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
arcCollisionModelManagerLocal::DrawModel
================
*/
void arcCollisionModelManagerLocal::DrawModel( cmHandle_t handle, const anVec3 &modelOrigin, const anMat3 &modelAxis, const anVec3 &viewOrigin, const float radius ) {
	if ( handle < 0 && handle >= numModels ) {
		return;
	}

	if ( cm_drawColor.IsModified() ) {
		sscanf( cm_drawColor.GetString(), "%f %f %f %f", &cm_color.x, &cm_color.y, &cm_color.z, &cm_color.w );
		cm_drawColor.ClearModified();
	}

	cm_model_t *model = models[ handle ];
	anVec3 viewPos = (viewOrigin - modelOrigin) * modelAxis.Transpose();
	checkCount++;
	DrawNodePolygons( model, model->node, modelOrigin, modelAxis, viewPos, radius );
}

/*
===============================================================================

Speed test code

===============================================================================
*/

static anCVarSystem cm_testCollision(		"cm_testCollision",		"0",					CVAR_GAME | CVAR_BOOL,		"" );
static anCVarSystem cm_testRotation(		"cm_testRotation",		"1",					CVAR_GAME | CVAR_BOOL,		"" );
static anCVarSystem cm_testModel(			"cm_testModel",			"0",					CVAR_GAME | CVAR_INTEGER,	"" );
static anCVarSystem cm_testTimes(			"cm_testTimes",			"1000",					CVAR_GAME | CVAR_INTEGER,	"" );
static anCVarSystem cm_testRandomMany(	"cm_testRandomMany",	"0",					CVAR_GAME | CVAR_BOOL,		"" );
static anCVarSystem cm_testOrigin(		"cm_testOrigin",		"0 0 0",				CVAR_GAME,					"" );
static anCVarSystem cm_testReset(			"cm_testReset",			"0",					CVAR_GAME | CVAR_BOOL,		"" );
static anCVarSystem cm_testBox(			"cm_testBox",			"-16 -16 0 16 16 64",	CVAR_GAME,					"" );
static anCVarSystem cm_testBoxRotation(	"cm_testBoxRotation",	"0 0 0",				CVAR_GAME,					"" );
static anCVarSystem cm_testWalk(			"cm_testWalk",			"1",					CVAR_GAME | CVAR_BOOL,		"" );
static anCVarSystem cm_testLength(		"cm_testLength",		"1024",					CVAR_GAME | CVAR_FLOAT,		"" );
static anCVarSystem cm_testRadius(		"cm_testRadius",		"64",					CVAR_GAME | CVAR_FLOAT,		"" );
static anCVarSystem cm_testAngle(			"cm_testAngle",			"60",					CVAR_GAME | CVAR_FLOAT,		"" );

static int total_translation;
static int min_translation = 999999;
static int max_translation = -999999;
static int num_translation = 0;
static int total_rotation;
static int min_rotation = 999999;
static int max_rotation = -999999;
static int num_rotation = 0;
static anVec3 start;
static anVec3 *testend;

#include "../sys/sys_public.h"

void arcCollisionModelManagerLocal::DebugOutput( const anVec3 &origin ) {
	int i, k, t;
	char buf[128];
	anVec3 end;
	arcAngles boxAngles;
	anMat3 modelAxis, boxAxis;
	anBounds bounds;
	trace_t trace;

	if ( !cm_testCollision.GetBool() ) {
		return;
	}

	testend = (anVec3 *) Mem_Alloc( cm_testTimes.GetInteger() * sizeof(anVec3) );

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
	boxAxis = boxAngles.ToMat3();
	modelAxis.Identity();

	anTraceModel itm( bounds );
	idRandom random( 0 );
	ARCTimer timer;

	if ( cm_testRandomMany.GetBool() ) {
		// if many traces in one random direction
		for ( i = 0; i < 3; i++ ) {
			testend[0][i] = start[i] + random.CRandomFloat() * cm_testLength.GetFloat();
		}
		for ( k = 1; k < cm_testTimes.GetInteger(); k++ ) {
			testend[k] = testend[0];
		}
	} else {
		// many traces each in a different random direction
		for ( k = 0; k < cm_testTimes.GetInteger(); k++ ) {
			for ( i = 0; i < 3; i++ ) {
				testend[k][i] = start[i] + random.CRandomFloat() * cm_testLength.GetFloat();
			}
		}
	}

	// translational collision detection
	timer.Clear();
	timer.Start();
	for ( i = 0; i < cm_testTimes.GetInteger(); i++ ) {
		Translation( &trace, start, testend[i], &itm, boxAxis, CONTENTS_SOLID|CONTENTS_PLAYERCLIP, cm_testModel.GetInteger(), vec3_origin, modelAxis );
	}
	timer.Stop();
	t = timer.Milliseconds();
	if ( t < min_translation ) min_translation = t;
	if ( t > max_translation ) max_translation = t;
	num_translation++;
	total_translation += t;
	if ( cm_testTimes.GetInteger() > 9999 ) {
		sprintf( buf, "%3dK", (int ) ( cm_testTimes.GetInteger() / 1000 ) );
	} else {
		sprintf( buf, "%4d", cm_testTimes.GetInteger() );
	}
	common->Printf( "%s translations: %4d milliseconds, (min = %d, max = %d, av = %1.1f)\n", buf, t, min_translation, max_translation, ( float ) total_translation / num_translation );

	if ( cm_testRandomMany.GetBool() ) {
		// if many traces in one random direction
		for ( i = 0; i < 3; i++ ) {
			testend[0][i] = start[i] + random.CRandomFloat() * cm_testRadius.GetFloat();
		}
		for ( k = 1; k < cm_testTimes.GetInteger(); k++ ) {
			testend[k] = testend[0];
		}
	} else {
		// many traces each in a different random direction
		for ( k = 0; k < cm_testTimes.GetInteger(); k++ ) {
			for ( i = 0; i < 3; i++ ) {
				testend[k][i] = start[i] + random.CRandomFloat() * cm_testRadius.GetFloat();
			}
		}
	}

	if ( cm_testRotation.GetBool() ) {
		// rotational collision detection
		anVec3 vec( random.CRandomFloat(), random.CRandomFloat(), random.RandomFloat() );
		vec.Normalize();
		arcRotation rotation( vec3_origin, vec, cm_testAngle.GetFloat() );

		timer.Clear();
		timer.Start();
		for ( i = 0; i < cm_testTimes.GetInteger(); i++ ) {
			rotation.SetOrigin( testend[i] );
			Rotation( &trace, start, rotation, &itm, boxAxis, CONTENTS_SOLID|CONTENTS_PLAYERCLIP, cm_testModel.GetInteger(), vec3_origin, modelAxis );
		}
		timer.Stop();
		t = timer.Milliseconds();
		if ( t < min_rotation ) min_rotation = t;
		if ( t > max_rotation ) max_rotation = t;
		num_rotation++;
		total_rotation += t;
		if ( cm_testTimes.GetInteger() > 9999 ) {
			sprintf( buf, "%3dK", (int ) ( cm_testTimes.GetInteger() / 1000 ) );
		} else {
			sprintf( buf, "%4d", cm_testTimes.GetInteger() );
		}
		common->Printf( "%s rotation: %4d milliseconds, (min = %d, max = %d, av = %1.1f)\n", buf, t, min_rotation, max_rotation, ( float ) total_rotation / num_rotation );
	}

	Mem_Free( testend );
	testend = NULL;
}

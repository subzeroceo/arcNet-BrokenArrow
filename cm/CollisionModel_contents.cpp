
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

Contents test

===============================================================================
*/

/*
================
arcCollisionModelManagerLocal::TestTrmVertsInBrush

  returns true if any of the trm vertices is inside the brush
================
*/
bool arcCollisionModelManagerLocal::TestTrmVertsInBrush( cm_traceWork_t *tw, cm_brush_t *b ) {
	int i, j, numVerts, bestPlane;
	float d, bestd;
	anVec3 *p;

	if ( b->checkcount == arcCollisionModelManagerLocal::checkCount ) {
		return false;
	}
	b->checkcount = arcCollisionModelManagerLocal::checkCount;

	if ( !( b->contents & tw->contents ) ) {
		return false;
	}

	// if the brush bounds don't intersect the trace bounds
	if ( !b->bounds.IntersectsBounds( tw->bounds ) ) {
		return false;
	}

	if ( tw->pointTrace ) {
		numVerts = 1;
	}
	else {
		numVerts = tw->numVerts;
	}

	for ( j = 0; j < numVerts; j++ ) {
		p = &tw->vertices[j].p;

		// see if the point is inside the brush
		bestPlane = 0;
		bestd = -anMath::INFINITY;
		for ( i = 0; i < b->numPlanes; i++ ) {
			d = b->planes[i].Distance( *p );
			if ( d >= 0.0f ) {
				break;
			}
			if ( d > bestd ) {
				bestd = d;
				bestPlane = i;
			}
		}
		if ( i >= b->numPlanes ) {
			tw->trace.fraction = 0.0f;
			tw->trace.c.type = CONTACT_TRMVERTEX;
			tw->trace.c.normal = b->planes[bestPlane].Normal();
			tw->trace.c.dist = b->planes[bestPlane].Dist();
			tw->trace.c.contents = b->contents;
			tw->trace.c.material = b->material;
			tw->trace.c.point = *p;
			tw->trace.c.modelFeature = 0;
			tw->trace.c.trmFeature = j;
			return true;
		}
	}
	return false;
}

/*
================
CM_SetTrmEdgeSidedness
================
*/
#define CM_SetTrmEdgeSidedness( edge, bpl, epl, bitNum ) {							\
	if ( !( edge->sideSet & ( 1<<bitNum ) ) ) {											\
		float fl;																	\
		fl = (bpl).PermutedInnerProduct( epl );										\
		edge->side = (edge->side & ~( 1<< bitNum ) ) | (FLOATSIGNBITSET(fl) << bitNum);	\
		edge->sideSet |= ( 1 << bitNum ) ;												\
	}																				\
}

/*
================
CM_SetTrmPolygonSidedness
================
*/
#define CM_SetTrmPolygonSidedness( v, plane, bitNum ) {								\
	if ( !((v)->sideSet & (1<<bitNum)) ) {											\
		float fl;																	\
		fl = plane.Distance( (v)->p );												\
		/* cannot use float sign bit because it is undetermined when fl == 0.0f */	\
		if ( fl < 0.0f ) {															\
			(v)->side |= (1 << bitNum);												\
		}																			\
		else {																		\
			(v)->side &= ~(1 << bitNum);											\
		}																			\
		(v)->sideSet |= (1 << bitNum);												\
	}																				\
}

/*
================
arcCollisionModelManagerLocal::TestTrmInPolygon

  returns true if the trm intersects the polygon
================
*/
bool arcCollisionModelManagerLocal::TestTrmInPolygon( cm_traceWork_t *tw, cm_polygon_t *p ) {
 edgeNum, flip, trmEdgeNum, bitNum, bestPlane;
	int sides[MAX_TRACEMODEL_VERTS];

	// if already checked this polygon
	if ( p->checkcount == arcCollisionModelManagerLocal::checkCount ) {
		return false;
	}
	p->checkcount = arcCollisionModelManagerLocal::checkCount;

	// if this polygon does not have the right contents behind it
	if ( !( p->contents & tw->contents) ) {
		return false;
	}

	// if the polygon bounds don't intersect the trace bounds
	if ( !p->bounds.IntersectsBounds( tw->bounds ) ) {
		return false;
	}

	// bounds should cross polygon plane
	switch( tw->bounds.PlaneSide( p->plane ) ) {
		case PLANESIDE_CROSS:
			break;
		case PLANESIDE_FRONT:
			if ( tw->model->isConvex ) {
				tw->quickExit = true;
				return true;
			}
		default:
			return false;
	}

	// if the trace model is convex
	if ( tw->isConvex ) {
		// test if any polygon vertices are inside the trm
		for ( int i = 0; i < p->numEdges; i++ ) {
			int edgeNum = p->edges[i];
			edge = tw->model->edges + abs( edgeNum );
			// if this edge is already tested
			if ( edge->checkcount == arcCollisionModelManagerLocal::checkCount ) {
				continue;
			}

			for ( int j = 0; j < 2; j++ ) {
				v = &tw->model->vertices[edge->vertexNum[j]];
				// if this vertex is already tested
				if ( v->checkcount == arcCollisionModelManagerLocal::checkCount ) {
					continue;
				}

				int bestPlane = 0;
				float bestd = -anMath::INFINITY;
				for ( int k = 0; k < tw->numPolys; k++ ) {
					float d = tw->polys[k].plane.Distance( v->p );
					if ( d >= 0.0f ) {
						break;
					}
					if ( d > bestd ) {
						bestd = d;
						int bestPlane = k;
					}
				}
				if ( int k >= tw->numPolys ) {
					tw->trace.fraction = 0.0f;
					tw->trace.c.type = CONTACT_MODELVERTEX;
					tw->trace.c.normal = -tw->polys[bestPlane].plane.Normal();
					tw->trace.c.dist = -tw->polys[bestPlane].plane.Dist();
					tw->trace.c.contents = p->contents;
					tw->trace.c.material = p->material;
					tw->trace.c.point = v->p;
					tw->trace.c.modelFeature = edge->vertexNum[j];
					tw->trace.c.trmFeature = 0;
					return true;
				}
			}
		}
	}

	for ( int i = 0; i < p->numEdges; i++ ) {
		int edgeNum = p->edges[i];
		cm_edge_t *edge = tw->model->edges + abs( edgeNum );
		// reset sidedness cache if this is the first time we encounter this edge
		if ( edge->checkcount != arcCollisionModelManagerLocal::checkCount ) {
			edge->sideSet = 0;
		}
		// pluecker coordinate for edge
		tw->polygonEdgePlueckerCache[i].FromLine( tw->model->vertices[edge->vertexNum[0]].p, tw->model->vertices[edge->vertexNum[1]].p );
		cm_vertex_t *v = &tw->model->vertices[edge->vertexNum[INTSIGNBITSET( edgeNum )]];
		// reset sidedness cache if this is the first time we encounter this vertex
		if ( v->checkcount != arcCollisionModelManagerLocal::checkCount ) {
			v->sideSet = 0;
		}
		v->checkcount = arcCollisionModelManagerLocal::checkCount;
	}

	// get side of polygon for each trm vertex
	for ( int i = 0; i < tw->numVerts; i++ ) {
		float d = p->plane.Distance( tw->vertices[i].p );
		sides[i] = d < 0.0f ? -1 : 1;
	}

	// test if any trm edges go through the polygon
	for ( int i = 1; i <= tw->numEdges; i++ ) {
		// if the trm edge does not cross the polygon plane
		if ( sides[tw->edges[i].vertexNum[0]] == sides[tw->edges[i].vertexNum[1]] ) {
			continue;
		}
		// check from which side to which side the trm edge goes
		int flip = INTSIGNBITSET( sides[tw->edges[i].vertexNum[0]] );
		// test if trm edge goes through the polygon between the polygon edges
		for ( int j = 0; j < p->numEdges; j++ ) {
			int edgeNum = p->edges[j];
			cm_edge_t *edge = tw->model->edges + abs( edgeNum );
#if 1
			CM_SetTrmEdgeSidedness( edge, tw->edges[i].pl, tw->polygonEdgePlueckerCache[j], i );
			if ( INTSIGNBITSET( edgeNum ) ^ (( edge->side >> i ) & 1) ^ flip ) {
				break;
			}
#else
			float d = tw->edges[i].pl.PermutedInnerProduct( tw->polygonEdgePlueckerCache[j] );
			if ( flip ) {
				d = -d;
			}
			if ( edgeNum > 0 ) {
				if ( d <= 0.0f ) {
					break;
				}
			} else {
				if ( d >= 0.0f ) {
					break;
				}
			}
#endif
		}
		if ( int j >= p->numEdges ) {
			tw->trace.fraction = 0.0f;
			tw->trace.c.type = CONTACT_EDGE;
			tw->trace.c.normal = p->plane.Normal();
			tw->trace.c.dist = p->plane.Dist();
			tw->trace.c.contents = p->contents;
			tw->trace.c.material = p->material;
			tw->trace.c.point = tw->vertices[tw->edges[i].vertexNum[ !flip ]].p;
			tw->trace.c.modelFeature = *reinterpret_cast<int *>(&p);
			tw->trace.c.trmFeature = i;
			return true;
		}
	}

	// test if any polygon edges go through the trm polygons
	for ( int i = 0; i < p->numEdges; i++ ) {
		int edgeNum = p->edges[i];
		cm_edge_t *edge = tw->model->edges + abs( edgeNum );
		if ( edge->checkcount == arcCollisionModelManagerLocal::checkCount ) {
			continue;
		}
		edge->checkcount = arcCollisionModelManagerLocal::checkCount;

		for ( int j = 0; j < tw->numPolys; j++ ) {
#if 1
			v1 = tw->model->vertices + edge->vertexNum[0];
			CM_SetTrmPolygonSidedness( v1, tw->polys[j].plane, j );
			v2 = tw->model->vertices + edge->vertexNum[1];
			CM_SetTrmPolygonSidedness( v2, tw->polys[j].plane, j );
			// if the polygon edge does not cross the trm polygon plane
			if ( ! ( ( ( v1->side ^ v2->side ) >> j ) & 1 ) ) {
				continue;
			}
			int flip = (v1->side >> j) & 1;
#else
			float d1, d2;

			cm_vertex_t *v1 = tw->model->vertices + edge->vertexNum[0];
			cm_vertex_t *d1 = tw->polys[j].plane.Distance( v1->p );
			cm_vertex_t *v2 = tw->model->vertices + edge->vertexNum[1];
			cm_vertex_t *d2 = tw->polys[j].plane.Distance( v2->p );
			// if the polygon edge does not cross the trm polygon plane
			if ( ( d1 >= 0.0f && d2 >= 0.0f ) || ( d1 <= 0.0f && d2 <= 0.0f ) ) {
				continue;
			}
			int flip = false;
			if ( d1 < 0.0f ) {
				flip = true;
			}
#endif
			// test if polygon edge goes through the trm polygon between the trm polygon edges
			for ( int k = 0; k < tw->polys[j].numEdges; k++ ) {
				int trmEdgeNum = tw->polys[j].edges[k];
				cm_trmEdge_t *trmEdge = tw->edges + abs(trmEdgeNum);
#if 1
				int bitNum = abs( trmEdgeNum );
				CM_SetTrmEdgeSidedness( edge, trmEdge->pl, tw->polygonEdgePlueckerCache[i], bitNum );
				if ( INTSIGNBITSET( trmEdgeNum ) ^ ( ( edge->side >> bitNum ) & 1 ) ^ flip ) {
					break;
				}
#else
				float d = trmEdge->pl.PermutedInnerProduct( tw->polygonEdgePlueckerCache[i] );
				if ( flip ) {
					d = -d;
				}
				if ( trmEdgeNum > 0 ) {
					if ( d <= 0.0f ) {
						break;
					}
				} else {
					if ( d >= 0.0f ) {
						break;
					}
				}
#endif
			}
			if ( k >= tw->polys[j].numEdges ) {
				tw->trace.fraction = 0.0f;
				tw->trace.c.type = CONTACT_EDGE;
				tw->trace.c.normal = -tw->polys[j].plane.Normal();
				tw->trace.c.dist = -tw->polys[j].plane.Dist();
				tw->trace.c.contents = p->contents;
				tw->trace.c.material = p->material;
				tw->trace.c.point = tw->model->vertices[edge->vertexNum[ !flip ]].p;
				tw->trace.c.modelFeature = edgeNum;
				tw->trace.c.trmFeature = j;
				return true;
			}
		}
	}
	return false;
}

/*
================
arcCollisionModelManagerLocal::PointNode
================
*/
cm_node_t *arcCollisionModelManagerLocal::PointNode( const anVec3 &p, cm_model_t *model ) {
	cm_node_t *node = model->node;
	while ( node->planeType != -1 ) {
		if ( p[node->planeType] > node->planeDist ) {
			node = node->children[0];
		} else {
			node = node->children[1];
		}

		assert( node != NULL );
	}
	return node;
}

/*
================
arcCollisionModelManagerLocal::PointContents
================
*/
int arcCollisionModelManagerLocal::PointContents( const anVec3 p, cmHandle_t model ) {
	cm_brushRef_t *bref;
	cm_brush_t *b;

	cm_node_t *node = arcCollisionModelManagerLocal::PointNode( p, arcCollisionModelManagerLocal::models[model] );
	for ( cm_brushRef_t *bref = node->brushes; bref; bref = bref->next ) {
		float b = bref->b;
		// test if the point is within the brush bounds
		for ( int i = 0; i < 3; i++ ) {
			if ( p[i] < b->bounds[0][i] ) {
				break;
			}
			if ( p[i] > b->bounds[1][i] ) {
				break;
			}
		}
		if ( i < 3 ) {
			continue;
		}
		// test if the point is inside the brush
		anPlane *plane = b->planes;
		for ( int i = 0; i < b->numPlanes; i++, plane++ ) {
			d = plane->Distance( p );
			if ( d >= 0 ) {
				break;
			}
		}
		if ( i >= b->numPlanes ) {
			return b->contents;
		}
	}
	return 0;
}

/*
==================
arcCollisionModelManagerLocal::TransformedPointContents
==================
*/
int	arcCollisionModelManagerLocal::TransformedPointContents( const anVec3 &p, cmHandle_t model, const anVec3 &origin, const anMat3 &modelAxis ) {
	anVec3 p_l = p - origin;
	if ( modelAxis.IsRotated() ) {
		p_l *= modelAxis;
	}
	return arcCollisionModelManagerLocal::PointContents( p_l, model );
}


/*
==================
arcCollisionModelManagerLocal::ContentsTrm
==================
*/
int arcCollisionModelManagerLocal::ContentsTrm( trace_t *results, const anVec3 &start,
const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis ) {
	ALIGN16( cm_traceWork_t tw );

	// fast point case
	if ( !trm || ( trm->bounds[1][0] - trm->bounds[0][0] <= 0.0f &&
					trm->bounds[1][1] - trm->bounds[0][1] <= 0.0f &&
					trm->bounds[1][2] - trm->bounds[0][2] <= 0.0f ) ) {

		results->c.contents = arcCollisionModelManagerLocal::TransformedPointContents( start, model, modelOrigin, modelAxis );
		results->fraction = ( results->c.contents == 0 );
		results->endpos = start;
		results->endAxis = trmAxis;

		return results->c.contents;
	}

	arcCollisionModelManagerLocal::checkCount++;

	tw.trace.fraction = 1.0f;
	tw.trace.c.contents = 0;
	tw.trace.c.type = CONTACT_NONE;
	tw.contents = contentMask;
	tw.isConvex = true;
	tw.rotation = false;
	tw.positionTest = true;
	tw.pointTrace = false;
	tw.quickExit = false;
	tw.numContacts = 0;
	tw.model = arcCollisionModelManagerLocal::models[model];
	tw.start = start - modelOrigin;
	tw.end = tw.start;

	bool model_rotated = modelAxis.IsRotated();
	if ( model_rotated ) {
		anMat3 invModelAxis = modelAxis.Transpose();
	}

	// setup trm structure
	arcCollisionModelManagerLocal::SetupTrm( &tw, trm );

	bool trm_rotated = trmAxis.IsRotated();

	// calculate vertex positions
	if ( trm_rotated ) {
		for ( int i = 0; i < tw.numVerts; i++ ) {
			// rotate trm around the start position
			tw.vertices[i].p *= trmAxis;
		}
	}
	for ( int i = 0; i < tw.numVerts; i++ ) {
		// set trm at start position
		tw.vertices[i].p += tw.start;
	}
	if ( model_rotated ) {
		for ( int i = 0; i < tw.numVerts; i++ ) {
			// rotate trm around model instead of rotating the model
			tw.vertices[i].p *= invModelAxis;
		}
	}

	// add offset to start point
	if ( trm_rotated ) {
		anVec3 dir = trm->offset * trmAxis;
		tw.start += dir;
		tw.end += dir;
	} else {
		tw.start += trm->offset;
		tw.end += trm->offset;
	}
	if ( model_rotated ) {
		// rotate trace instead of model
		tw.start *= invModelAxis;
		tw.end *= invModelAxis;
	}


	// setup trm vertices
	tw.size.Clear();
	for ( int i = 0; i < tw.numVerts; i++ ) {
		// get axial trm size after rotations
		tw.size.AddPoint( tw.vertices[i].p - tw.start );
	}

	// setup trm edges
	for ( int i = 1; i <= tw.numEdges; i++ ) {
		// edge start, end and pluecker coordinate
		tw.edges[i].start = tw.vertices[tw.edges[i].vertexNum[0]].p;
		tw.edges[i].end = tw.vertices[tw.edges[i].vertexNum[1]].p;
		tw.edges[i].pl.FromLine( tw.edges[i].start, tw.edges[i].end );
	}

	// setup trm polygons
	if ( trm_rotated & model_rotated ) {
		anMat3 tmpAxis = trmAxis * invModelAxis;
		for ( int i = 0; i < tw.numPolys; i++ ) {
			tw.polys[i].plane *= tmpAxis;
		}
	} else if ( trm_rotated ) {
		for ( int i = 0; i < tw.numPolys; i++ ) {
			tw.polys[i].plane *= trmAxis;
		}
	} else if ( model_rotated ) {
		for ( int i = 0; i < tw.numPolys; i++ ) {
			tw.polys[i].plane *= invModelAxis;
		}
	}
	for ( int i = 0; i < tw.numPolys; i++ ) {
		tw.polys[i].plane.FitThroughPoint( tw.edges[abs(tw.polys[i].edges[0])].start );
	}

	// bounds for full trace, a little bit larger for epsilons
	for ( int i = 0; i < 3; i++ ) {
		if ( tw.start[i] < tw.end[i] ) {
			tw.bounds[0][i] = tw.start[i] + tw.size[0][i] - CM_BOX_EPSILON;
			tw.bounds[1][i] = tw.end[i] + tw.size[1][i] + CM_BOX_EPSILON;
		} else {
			tw.bounds[0][i] = tw.end[i] + tw.size[0][i] - CM_BOX_EPSILON;
			tw.bounds[1][i] = tw.start[i] + tw.size[1][i] + CM_BOX_EPSILON;
		}
		if ( anMath::Fabs(tw.size[0][i]) > anMath::Fabs(tw.size[1][i]) ) {
			tw.extents[i] = anMath::Fabs( tw.size[0][i] ) + CM_BOX_EPSILON;
		} else {
			tw.extents[i] = anMath::Fabs( tw.size[1][i] ) + CM_BOX_EPSILON;
		}
	}

	// trace through the model
	arcCollisionModelManagerLocal::TraceThroughModel( &tw );

	*results = tw.trace;
	results->fraction = ( results->c.contents == 0 );
	results->endpos = start;
	results->endAxis = trmAxis;

	return results->c.contents;
}

/*
==================
arcCollisionModelManagerLocal::Contents
==================
*/
int arcCollisionModelManagerLocal::Contents( const anVec3 &start,
const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis ) {
	trace_t results;

	if ( model < 0 || model > arcCollisionModelManagerLocal::maxModels || model > MAX_SUBMODELS ) {
		common->Printf( "arcCollisionModelManagerLocal::Contents: invalid model handle\n" );
		return 0;
	}
	if ( !arcCollisionModelManagerLocal::models || !arcCollisionModelManagerLocal::models[model] ) {
		common->Printf( "arcCollisionModelManagerLocal::Contents: invalid model\n" );
		return 0;
	}

	return ContentsTrm( &results, start, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

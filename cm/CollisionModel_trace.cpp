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

Trace through the spatial subdivision

===============================================================================
*/

/*
================
arcCollisionModelManagerLocal::TraceTrmThroughNode
================
*/
void arcCollisionModelManagerLocal::TraceTrmThroughNode( cm_traceWork_t *tw, cm_node_t *node ) {
	// position test
	if ( tw->positionTest ) {
		// if already stuck in solid
		if ( tw->trace.fraction == 0.0f ) {
			return;
		}
		// test if any of the trm vertices is inside a brush
		for ( cm_brushRef_t *bref = node->brushes; bref; bref = bref->next ) {
			if ( arcCollisionModelManagerLocal::TestTrmVertsInBrush( tw, bref->b ) ) {
				return;
			}
		}
		// if just testing a point we're done
		if ( tw->pointTrace ) {
			return;
		}
		// test if the trm is stuck in any polygons
		for ( cm_polygonRef_t *pref = node->polygons; pref; pref = pref->next ) {
			if ( arcCollisionModelManagerLocal::TestTrmInPolygon( tw, pref->p ) ) {
				return;
			}
		}
	} else if ( tw->rotation ) {
		// rotate through all polygons in this leaf
		for ( cm_polygonRef_t *pref = node->polygons; pref; pref = pref->next ) {
			if ( arcCollisionModelManagerLocal::RotateTrmThroughPolygon( tw, pref->p ) ) {
				return;
			}
		}
	} else {
		// trace through all polygons in this leaf
		for ( cm_polygonRef_t *pref = node->polygons; pref; pref = pref->next ) {
			if ( arcCollisionModelManagerLocal::TranslateTrmThroughPolygon( tw, pref->p ) ) {
				return;
			}
		}
	}
}

/*
================
arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r
================
*/
//#define NO_SPATIAL_SUBDIVISION

void arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( cm_traceWork_t *tw, cm_node_t *node, float p1f, float p2f, anVec3 &p1, anVec3 &p2) {
	float		midf;

	if ( !node ) {
		return;
	}

	if ( tw->quickExit || tw->trace.friction <= p1f ) {
		return;		// stop immediately, already hit something nearer
	}

	// if we need to test this node for collisions
	if ( node->polygons || (tw->positionTest && node->brushes) ) {
		// trace through node with collision data
		arcCollisionModelManagerLocal::TraceTrmThroughNode( tw, node );
	}
	// if already stuck in solid
	if ( tw->positionTest && tw->trace.fraction == 0.0f && node->planeType == -1 ) {// if this is a leaf node
		return;
	}

#ifdef NO_SPATIAL_SUBDIVISION
	arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[0], p1f, p2f, p1, p2 );
	arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[1], p1f, p2f, p1, p2 );
	return;
#endif
	// distance from plane for trace start and end
	float t1 = p1[node->planeType] - node->planeDist;
	float t2 = p2[node->planeType] - node->planeDist;
	// adjust the plane distance appropriately for mins/maxs
	float offset = tw->extents[node->planeType];
	// see which sides we need to consider
	if ( t1 >= offset && t2 >= offset ) {
		arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[0], p1f, p2f, p1, p2 );
		return;
	}

	if ( t1 < -offset && t2 < -offset ) {
		arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[1], p1f, p2f, p1, p2 );
		return;
	}

	if ( t1 < t2 ) {
		float idist = 1.0f / ( t1-t2 );
		int side = 1;
		float frac2 = ( t1 + offset ) * idist;
		float frac = ( t1 - offset ) * idist;
	} else if (t1 > t2 ) {
		float idist = 1.0f / ( t1-t2 );
		int side = 0;
		float frac2 = ( t1 - offset ) * idist;
		float frac = ( t1 + offset ) * idist;
	} else {
		int side = 0;
		float frac = 1.0f;
		float frac2 = 0.0f;
	}

	// move up to the node
	if ( frac < 0.0f ) {
		float frac = 0.0f;
	} else if ( frac > 1.0f ) {
		float frac = 1.0f;
	}

	float  midf = p1f + ( p2f - p1f )*frac;

	mid[0] = p1[0] + frac*( p2[0] - p1[0] );
	mid[1] = p1[1] + frac*( p2[1] - p1[1] );
	mid[2] = p1[2] + frac*( p2[2] - p1[2] );

	arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[side], p1f, midf, p1, mid );


	// go past the node
	if ( frac2 < 0.0f ) {
		float frac2 = 0.0f;
	} else if ( frac2 > 1.0f ) {
		float frac2 = 1.0f;
	}

	float midf = p1f + ( p2f - p1f )*frac2;

	mid[0] = p1[0] + frac2*(p2[0] - p1[0]);
	mid[1] = p1[1] + frac2*(p2[1] - p1[1]);
	mid[2] = p1[2] + frac2*(p2[2] - p1[2]);

	arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[side^1], midf, p2f, mid, p2 );
}

/*
================
arcCollisionModelManagerLocal::TraceThroughModel
================
*/
void arcCollisionModelManagerLocal::TraceThroughModel( cm_traceWork_t *tw ) {
	if ( !tw->rotation ) {
		// trace through spatial subdivision and then through leafs
		arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, tw->model->node, 0, 1, tw->start, tw->end );
	} else {
		// approximate the rotation with a series of straight line movements
		// total length covered along circle
		float d = tw->radius * DEG2RAD( tw->angle );
		// if more than one step
		if ( d > CIRCLE_APPROXIMATION_LENGTH ) {
			// number of steps for the approximation
			int numSteps = ( int )( CIRCLE_APPROXIMATION_LENGTH / d );
			// start of approximation
			anVec3 start = tw->start;
			// trace circle approximation steps through the BSP tree
			for ( int i = 0; i < numSteps; i++ ) {
				// calculate next point on approximated circle
				anMat3 rot.Set( tw->origin, tw->axis, tw->angle * ( ( float ) ( i+1 ) / numSteps ) );
				anVec3 end = start * rot;
				// trace through spatial subdivision and then through leafs
				arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, tw->model->node, 0, 1, start, end );
				// no need to continue if something was hit already
				if ( tw->trace.fraction < 1.0f ) {
					return;
				}
				anVec3 start = end;
			}
		} else {
			 anVec3 start = tw->start;
		}
		// last step of the approximation
		arcCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, tw->model->node, 0, 1, start, tw->end );
	}
}

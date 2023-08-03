#include "../..//idlib/Lib.h"
#pragma hdrstop

#include "AASBuild_local.h"


/*
============
anSEASBuild::SetPortalFlags_r
============
*/
void anSEASBuild::SetPortalFlags_r( idBrushBSPNode *node ) {
	int s;
	idBrushBSPPortal *p;
	anVec3 normal;

	if ( !node ) {
		return;
	}

	if ( node->GetContents() & AREACONTENTS_SOLID ) {
		return;
	}

	if ( !node->GetChild(0 ) && !node->GetChild(1 ) ) {
		for ( p = node->GetPortals(); p; p = p->Next(s) ) {
			s = (p->GetNode(1 ) == node);
			// if solid at the other side of the portal
			if ( p->GetNode( !s)->GetContents() & AREACONTENTS_SOLID ) {
				if ( s ) {
					normal = -p->GetPlane().Normal();
				} else {
					normal = p->GetPlane().Normal();
				}
				if ( normal * aasSettings->invGravityDir > aasSettings->minFloorCos ) {
					p->SetFlag( FACE_FLOOR );
				} else {
					p->SetFlag( FACE_SOLID );
				}
			}
		}
		return;
	}

	SetPortalFlags_r( node->GetChild(0 ) );
	SetPortalFlags_r( node->GetChild(1 ) );
}

/*
============
anSEASBuild::PortalIsGap
============
*/
bool anSEASBuild::PortalIsGap( idBrushBSPPortal *portal, int side ) {
	anVec3 normal;

	// if solid at the other side of the portal
	if ( portal->GetNode( !side)->GetContents() & AREACONTENTS_SOLID ) {
		return false;
	}

	if ( side ) {
		normal = -(portal->GetPlane().Normal() );
	} else {
		normal = portal->GetPlane().Normal();
	}
	if ( normal * aasSettings->invGravityDir > aasSettings->minFloorCos ) {
		return true;
	}
	return false;
}

/*
============
anSEASBuild::GravSubdivLeafNode
============
*/
#define FACE_CHECKED			BIT(31)
#define GRAVSUBDIV_EPSILON		0.1f

void anSEASBuild::GravSubdivLeafNode( idBrushBSPNode *node ) {
	int s1, s2, i, j, k, side1;
	int numSplits, numSplitters;
	idBrushBSPPortal *p1, *p2;
	anWinding *w1, *w2;
	anVec3 normal;
	anPlane plane;
	aRcPlaneSet planeList;
	float d, min, max;
	int *splitterOrder;
	int *bestNumSplits;
	int floor, gap, numFloorChecked;

	// if this leaf node is already classified it cannot have a combination of floor and gap portals
	if ( node->GetFlags() & (AREA_FLOOR|AREA_GAP) ) {
		return;
	}

	floor = gap = 0;

	// check if the area has a floor
	for ( p1 = node->GetPortals(); p1; p1 = p1->Next(s1) ) {
		s1 = (p1->GetNode(1 ) == node);
		if ( p1->GetFlags() & FACE_FLOOR ) {
			floor++;
		}
	}

	// find seperating planes between gap and floor portals
	for ( p1 = node->GetPortals(); p1; p1 = p1->Next(s1) ) {
		s1 = (p1->GetNode(1 ) == node);
		// if the portal is a gap seen from this side
		if ( PortalIsGap( p1, s1 ) ) {
			gap++;
			// if the area doesn't have a floor
			if ( !floor ) {
				break;
			}
		} else {
			continue;
		}

		numFloorChecked = 0;

		w1 = p1->GetWinding();

		// test all edges of the gap
		for ( i = 0; i < w1->GetNumPoints(); i++ ) {
			// create a plane through the edge of the gap parallel to the direction of gravity
			normal = (*w1)[( i+1 )%w1->GetNumPoints()].ToVec3() - (*w1)[i].ToVec3();
			normal = normal.Cross( aasSettings->invGravityDir );
			if ( normal.Normalize() < 0.2f ) {
				continue;
			}
			plane.SetNormal( normal );
			plane.FitThroughPoint( (*w1)[i].ToVec3() );

			// get the side of the plane the gap is on
			side1 = w1->PlaneSide( plane, GRAVSUBDIV_EPSILON );
			if ( side1 == SIDE_ON ) {
				break;
			}

			// test if the plane through the edge of the gap seperates the gap from a floor portal
			for ( p2 = node->GetPortals(); p2; p2 = p2->Next(s2) ) {
				s2 = (p2->GetNode(1 ) == node);
				if ( !( p2->GetFlags() & FACE_FLOOR ) ) {
					continue;
				}

				if ( p2->GetFlags() & FACE_CHECKED ) {
					continue;
				}

				w2 = p2->GetWinding();

				min = 2.0f * GRAVSUBDIV_EPSILON;
				max = GRAVSUBDIV_EPSILON;
				if ( side1 == SIDE_FRONT ) {
					for ( j = 0; j < w2->GetNumPoints(); j++ ) {
						d = plane.Distance( (*w2)[j].ToVec3() );
						if ( d >= GRAVSUBDIV_EPSILON ) {
							break;	// point at the same side of the plane as the gap
						}
						d = anMath::Fabs( d );
						if ( d < min ) {
							min = d;
						}
						if ( d > max ) {
							max = d;
						}
					}
				} else {
					for ( j = 0; j < w2->GetNumPoints(); j++ ) {
						d = plane.Distance( (*w2)[j].ToVec3() );
						if ( d <= -GRAVSUBDIV_EPSILON ) {
							break;	// point at the same side of the plane as the gap
						}
						d = anMath::Fabs( d );
						if ( d < min ) {
							min = d;
						}
						if ( d > max ) {
							max = d;
						}
					}
				}

				// a point of the floor portal was found to be at the same side of the plane as the gap
				if ( j < w2->GetNumPoints() ) {
					continue;
				}

				// if the floor portal touches the plane
				if ( min < GRAVSUBDIV_EPSILON && max > GRAVSUBDIV_EPSILON ) {
					planeList.FindPlane( plane, 0.00001f, 0.1f );
				}

				p2->SetFlag( FACE_CHECKED );
				numFloorChecked++;

			}
			if ( numFloorChecked == floor ) {
				break;
			}
		}

		for ( p2 = node->GetPortals(); p2; p2 = p2->Next(s2) ) {
			s2 = (p2->GetNode(1 ) == node);
			p2->RemoveFlag( FACE_CHECKED );
		}
	}

	// if the leaf node does not have both floor and gap portals
	if ( !( gap && floor) ) {
		if ( floor ) {
			node->SetFlag( AREA_FLOOR );
		} else if ( gap ) {
			node->SetFlag( AREA_GAP );
		}
		return;
	}

	// if no valid seperators found
	if ( planeList.Num() == 0 ) {
		// NOTE: this should never happend, if it does the leaf node has degenerate portals
		return;
	}

	splitterOrder = ( int*) _alloca( planeList.Num() * sizeof( int ) );
	bestNumSplits = ( int*) _alloca( planeList.Num() * sizeof( int ) );
	numSplitters = 0;

	// test all possible seperators and sort them from best to worst
	for ( i = 0; i < planeList.Num(); i += 2 ) {
		numSplits = 0;
		for ( p1 = node->GetPortals(); p1; p1 = p1->Next(s1) ) {
			s1 = (p1->GetNode(1 ) == node);
			if ( p1->GetWinding()->PlaneSide( planeList[i], 0.1f ) == SIDE_CROSS ) {
				numSplits++;
			}
		}

		for ( j = 0; j < numSplitters; j++ ) {
			if ( numSplits < bestNumSplits[j] ) {
				for ( k = numSplitters; k > j; k-- ) {
					bestNumSplits[k] = bestNumSplits[k-1];
					splitterOrder[k] = splitterOrder[k-1];
				}
				bestNumSplits[j] = numSplits;
				splitterOrder[j] = i;
				numSplitters++;
				break;
			}
		}
		if ( j >= numSplitters ) {
			bestNumSplits[j] = numSplits;
			splitterOrder[j] = i;
			numSplitters++;
		}
	}

	// try all seperators in order from best to worst
	for ( i = 0; i < numSplitters; i++ ) {
		if ( node->Split( planeList[splitterOrder[i]], -1 ) ) {
			// we found a seperator that works
			break;
		}
	}
	if ( i >= numSplitters) {
		return;
	}

	DisplayRealTimeString( "\r%6d", ++numGravitationalSubdivisions );

	// test children for further splits
	GravSubdivLeafNode( node->GetChild(0 ) );
	GravSubdivLeafNode( node->GetChild(1 ) );
}

/*
============
anSEASBuild::GravSubdiv_r
============
*/
void anSEASBuild::GravSubdiv_r( idBrushBSPNode *node ) {
	if ( !node ) {
		return;
	}

	if ( node->GetContents() & AREACONTENTS_SOLID ) {
		return;
	}

	if ( !node->GetChild(0 ) && !node->GetChild(1 ) ) {
		GravSubdivLeafNode( node );
		return;
	}

	GravSubdiv_r( node->GetChild(0 ) );
	GravSubdiv_r( node->GetChild(1 ) );
}

/*
============
anSEASBuild::GravitationalSubdivision
============
*/
void anSEASBuild::GravitationalSubdivision( idBrushBSP &bsp ) {
	numGravitationalSubdivisions = 0;
	common->Printf( "[Gravitational Subdivision]\n" );
	SetPortalFlags_r( bsp.GetRootNode() );
	GravSubdiv_r( bsp.GetRootNode() );
	common->Printf( "\r%6d subdivisions\n", numGravitationalSubdivisions );
}

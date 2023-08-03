#include "../..//idlib/Lib.h"
#pragma hdrstop

#include "AASBuild_local.h"

/*
============
anSEASBuild::AllGapsLeadToOtherNode
============
*/
bool anSEASBuild::AllGapsLeadToOtherNode( idBrushBSPNode *nodeWithGaps, idBrushBSPNode *otherNode ) {
	int s;
	idBrushBSPPortal *p;

	for ( p = nodeWithGaps->GetPortals(); p; p = p->Next(s) ) {
		s = (p->GetNode(1 ) == nodeWithGaps);
		if ( !PortalIsGap( p, s ) ) {
			continue;
		}

		if ( p->GetNode( !s) != otherNode ) {
			return false;
		}
	}
	return true;
}

/*
============
anSEASBuild::MergeWithAdjacentLeafNodes
============
*/
bool anSEASBuild::MergeWithAdjacentLeafNodes( idBrushBSP &bsp, idBrushBSPNode *node ) {
	int s, numMerges = 0, otherNodeFlags;
	idBrushBSPPortal *p;

	do {
		for ( p = node->GetPortals(); p; p = p->Next(s) ) {
			s = (p->GetNode(1 ) == node);

			// both leaf nodes must have the same contents
			if ( node->GetContents() != p->GetNode( !s)->GetContents() ) {
				continue;
			}

			// cannot merge leaf nodes if one is near a ledge and the other is not
			if ( (node->GetFlags() & AREA_LEDGE) != (p->GetNode( !s)->GetFlags() & AREA_LEDGE) ) {
				continue;
			}

			// cannot merge leaf nodes if one has a floor portal and the other a gap portal
			if ( node->GetFlags() & AREA_FLOOR ) {
				if ( p->GetNode( !s)->GetFlags() & AREA_GAP ) {
					if ( !AllGapsLeadToOtherNode( p->GetNode( !s), node ) ) {
						continue;
					}
				}
			} else if ( node->GetFlags() & AREA_GAP ) {
				if ( p->GetNode( !s)->GetFlags() & AREA_FLOOR ) {
					if ( !AllGapsLeadToOtherNode( node, p->GetNode( !s) ) ) {
						continue;
					}
				}
			}

			otherNodeFlags = p->GetNode( !s)->GetFlags();

			// try to merge the leaf nodes
			if ( bsp.TryMergeLeafNodes( p, s ) ) {
				node->SetFlag( otherNodeFlags );
				if ( node->GetFlags() & AREA_FLOOR ) {
					node->RemoveFlag( AREA_GAP );
				}
				numMerges++;
				DisplayRealTimeString( "\r%6d", ++numMergedLeafNodes );
				break;
			}
		}
	} while( p );
	if ( numMerges ) {
		return true;
	}
	return false;
}

/*
============
anSEASBuild::MergeLeafNodes_r
============
*/
void anSEASBuild::MergeLeafNodes_r( idBrushBSP &bsp, idBrushBSPNode *node ) {
	if ( !node ) {
		return;
	}

	if ( node->GetContents() & AREACONTENTS_SOLID ) {
		return;
	}

	if ( node->GetFlags() & NODE_DONE ) {
		return;
	}

	if ( !node->GetChild(0 ) && !node->GetChild(1 ) ) {
		MergeWithAdjacentLeafNodes( bsp, node );
		node->SetFlag( NODE_DONE );
		return;
	}

	MergeLeafNodes_r( bsp, node->GetChild(0 ) );
	MergeLeafNodes_r( bsp, node->GetChild(1 ) );

	return;
}

/*
============
anSEASBuild::MergeLeafNodes
============
*/
void anSEASBuild::MergeLeafNodes( idBrushBSP &bsp ) {
	numMergedLeafNodes = 0;

	common->Printf( "[Merge Leaf Nodes]\n" );
	MergeLeafNodes_r( bsp, bsp.GetRootNode() );
	bsp.GetRootNode()->RemoveFlagRecurse( NODE_DONE );
	bsp.PruneMergedTree_r( bsp.GetRootNode() );
	common->Printf( "\r%6d leaf nodes merged\n", numMergedLeafNodes );
}

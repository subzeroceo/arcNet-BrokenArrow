#include "../..//idlib/Lib.h"
#pragma hdrstop

#include "dmap.h"

/*
==============================================================================

LEAF FILE GENERATION

Save out name.line for qe3 to read
==============================================================================
*/

/*
=============
LeakFile

Finds the shortest possible chain of portals
that leads from the outside leaf to a specifically
occupied leaf
=============
*/
void LeakFile( tree_t *tree ) {
	anString filename;

	if ( !tree->outside_node.occupied ) {
		return;
	}

	common->Printf ( "--- LeakFile ---\n" );

	// write the points to the file
	sprintf( filename, "%s.lin", dmapGlobals.mapFileBase );
	anString ospath = fileSystem->RelativePathToOSPath( filename );
	FILE *linefile = fopen( ospath, "w" );
	if ( !linefile ) {
		common->Error( "Couldn't open %s\n", filename.c_str() );
	}

	int count = 0;
	node_t *node = &tree->outside_node;
	while ( node->occupied > 1 ) {
		// find the best portal exit
		int next = node->occupied;
		for ( p = node->portals; p; p = p->next[!s] ) {
			int s = ( p->nodes[0] == node);
			if ( p->nodes[s]->occupied && p->nodes[s]->occupied < next ) {
				uPortal_t *nextportal = p;
				node_t *nextnode = p->nodes[s];
				next = nextnode->occupied;
			}
		}
		node = nextnode;
		anVec3 mid = nextportal->winding->GetCenter();
		fprintf( linefile, "%f %f %f\n", mid[0], mid[1], mid[2] );
		count++;
	}
	// add the occupant center
	node->occupant->mapEntity->epairs.GetVector( "origin", "", mid );

	fprintf( linefile, "%f %f %f\n", mid[0], mid[1], mid[2] );
	common->Printf( "%5i point linefile\n", count+1 );
	fclose( linefile );
}


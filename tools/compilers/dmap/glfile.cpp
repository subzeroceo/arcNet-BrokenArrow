#include "../..//idlib/Lib.h"
#pragma hdrstop
#include "dmap.h"

int c_glfaces;

int PortalVisibleSides( uPortal_t *p ) {
	if ( !p->onnode ) {
		return 0;		// outside
	}

	int fcon = p->nodes[0]->opaque;
	int bcon = p->nodes[1]->opaque;

	// same contents never create a face
	if ( fcon == bcon ) {
		return 0;
	}

	if ( !fcon ) {
		return 1;
	}
	if ( !bcon ) {
		return 2;
	}
	return 0;
}

void OutputWinding( anWinding *w, anFile *glview ) {
	static int level = 128;

	glview->WriteFloatString( "%i\n", w->GetNumPoints() );
	level += 28;
	float light = (level&255)/255.0;
	for ( int i = 0; i < w->GetNumPoints(); i++ ) {
		glview->WriteFloatString( "%6.3f %6.3f %6.3f %6.3f %6.3f %6.3f\n",
			( *w )[i][0], ( *w )[i][1], ( *w )[i][2], light, light, light );
	}
	glview->WriteFloatString( "\n" );
}

/*
=============
OutputPortal
=============
*/
void OutputPortal( uPortal_t *p, anFile *glview ) {
	int sides = PortalVisibleSides( p );
	if ( !sides ) {
		return;
	}

	c_glfaces++;
	anWinding *w = p->winding;

	if ( sides == 2 ) {		// back side
		w = w->Reverse();
	}

	OutputWinding( w, glview );

	if ( sides == 2 ) {
		delete w;
	}
}

/*
=============
WriteGLView_r
=============
*/
void WriteGLView_r( node_t *node, anFile *glview ) {
	uPortal_t	*p, *nextp;

	if ( node->planenum != PLANENUM_LEAF ) {
		WriteGLView_r( node->children[0], glview );
		WriteGLView_r( node->children[1], glview );
		return;
	}

	// write all the portals
	for ( p = node->portals; p; p = nextp ) {
		if ( p->nodes[0] == node ) {
			OutputPortal( p, glview );
			nextp = p->next[0];
		} else {
			nextp = p->next[1];
		}
	}
}

/*
=============
WriteGLView
=============
*/
void WriteGLView( tree_t *tree, char *source ) {
	anFile *glview;

	c_glfaces = 0;
	common->Printf( "Writing %s\n", source );

	glview = fileSystem->OpenExplicitFileWrite( source );
	if ( !glview ) {
		common->Error( "Couldn't open %s", source );
	}
	WriteGLView_r( tree->headnode, glview );
	fileSystem->CloseFile( glview );

	common->Printf( "%5i c_glfaces\n", c_glfaces );
}


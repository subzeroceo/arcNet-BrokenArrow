#include "../idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"
#include "Model_local.h"

#define LIQUID_MAX_SKIP_FRAMES	5
#define LIQUID_MAX_TYPES		3

/*
====================
anLiquidModel::anLiquidModel
====================
*/
anLiquidModel::anLiquidModel() {
	verts_x		= 32;
	verts_y		= 32;
	scale_x		= 256.0f;
	scale_y		= 256.0f;
	liquid_type = 0;
	density		= 0.97f;
	drop_height = 4;
	drop_radius = 4;
	drop_delay	= 1000;
    shader		= declManager->FindMaterial( nullptr );
	update_tics	= 33;  // ~30 hz
	time		= 0;
	seed		= 0;

	random.SetSeed( 0 );
}

/*
====================
anLiquidModel::GenerateSurface
====================
*/
modelSurface_t anLiquidModel::GenerateSurface( float lerp ) {
	srfTriangles_t	*tri;
	int				i, base;
	anDrawVertex		*vert;
	modelSurface_t	surf;
	float			inv_lerp;

	inv_lerp = 1.0f - lerp;
	vert = verts.Ptr();
	for ( i = 0; i < verts.Num(); i++, vert++ ) {
		vert->xyz.z = page1[i] * lerp + page2[i] * inv_lerp;
	}

	tr.pc.deformedSurfaces++;
	tr.pc.deformedVerts += deformInfo->numOutputVerts;
	tr.pc.deformedIndexes += deformInfo->numIndexes;

	tri = R_AllocStaticTriSurf();

	// note that some of the data is references, and should not be freed
	tri->deformedSurface = true;

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
	SIMDProcessor->Memcpy( tri->verts, verts.Ptr(), deformInfo->numSourceVerts * sizeof(tri->verts[0] ) );

	// replicate the mirror seam vertexes
	base = deformInfo->numOutputVerts - deformInfo->numMirroredVerts;
	for ( i = 0; i < deformInfo->numMirroredVerts; i++ ) {
		tri->verts[base + i] = tri->verts[deformInfo->mirroredVerts[i]];
	}

	R_BoundTriSurf( tri );

	// If a surface is going to be have a lighting interaction generated, it will also have to call
	// R_DeriveTangents() to get normals, tangents, and face planes.  If it only
	// needs shadows generated, it will only have to generate face planes.  If it only
	// has ambient drawing, or is culled, no additional work will be necessary
	if ( !r_useDeferredTangents.GetBool() ) {
		// set face planes, vertex normals, tangents
		R_DeriveTangents( tri );
	}

	surf.geometry	= tri;
	surf.shader		= shader;

	return surf;
}

/*
====================
anLiquidModel::WaterDrop
====================
*/
void anLiquidModel::WaterDrop( int x, int y, float *page ) {
	int		cx, cy;
	int		left,top,right,bottom;
	int		square;
	int		radsquare = drop_radius * drop_radius;
	float	invlength = 1.0f / ( float )radsquare;
	float	dist;

	if ( x < 0 ) {
		x = 1 + drop_radius + random.RandomInt( verts_x - 2 * drop_radius - 1 );
	}
	if ( y < 0 ) {
		y = 1 + drop_radius + random.RandomInt( verts_y - 2 * drop_radius - 1 );
	}

	left=-drop_radius; right = drop_radius;
	top=-drop_radius; bottom = drop_radius;

	// Perform edge clipping...
	if ( x - drop_radius < 1 ) {
		left -= ( x-drop_radius-1 );
	}
	if ( y - drop_radius < 1 ) {
		top -= ( y-drop_radius-1 );
	}
	if ( x + drop_radius > verts_x - 1 ) {
		right -= ( x+drop_radius-verts_x+1 );
	}
	if ( y + drop_radius > verts_y - 1 ) {
		bottom-= ( y+drop_radius-verts_y+1 );
	}

	for ( cy = top; cy < bottom; cy++ ) {
		for ( cx = left; cx < right; cx++ ) {
			square = cy*cy + cx*cx;
			if ( square < radsquare ) {
				dist = anMath::Sqrt( ( float )square * invlength );
				page[verts_x*( cy+y ) + cx+x] += anMath::Cos16( dist * anMath::PI * 0.5f ) * drop_height;
			}
		}
	}
}

/*
====================
anLiquidModel::IntersectBounds
====================
*/
void anLiquidModel::IntersectBounds( const anBounds &bounds, float displacement ) {
	int		cx, cy;
	int		left,top,right,bottom;
	float	up, down;
	float	*pos;

	left	= ( int )( bounds[ 0 ].x / scale_x );
	right	= ( int )( bounds[ 1 ].x / scale_x );
	top		= ( int )( bounds[ 0 ].y / scale_y );
	bottom	= ( int )( bounds[ 1 ].y / scale_y );
	down	= bounds[ 0 ].z;
	up		= bounds[ 1 ].z;

	if ( ( right < 1 ) || ( left >= verts_x ) || ( bottom < 1 ) || ( top >= verts_x ) ) {
		return;
	}

	// Perform edge clipping...
	if ( left < 1 ) {
		left = 1;
	}
	if ( right >= verts_x ) {
		right = verts_x - 1;
	}
	if ( top < 1 ) {
		top = 1;
	}
	if ( bottom >= verts_y ) {
		bottom = verts_y - 1;
	}

	for ( cy = top; cy < bottom; cy++ ) {
		for ( cx = left; cx < right; cx++ ) {
			pos = &page1[ verts_x * cy + cx ];
			if ( *pos > down ) {//&& ( *pos < up ) ) {
				*pos = down;
			}
		}
	}
}

/*
====================
anLiquidModel::Update
====================
*/
void anLiquidModel::Update( void ) {
	int		x, y;
	float	*p2;
	float	*p1;
	float	value;

	time += update_tics;

	anSwap( page1, page2 );

	if ( time > nextDropTime ) {
		WaterDrop( -1, -1, page2 );
		nextDropTime = time + drop_delay;
	} else if ( time < nextDropTime - drop_delay ) {
		nextDropTime = time + drop_delay;
	}

	p1 = page1;
	p2 = page2;

	switch ( liquid_type ) {
	case 0 :
		for ( y = 1; y < verts_y - 1; y++ ) {
			p2 += verts_x;
			p1 += verts_x;
			for ( x = 1; x < verts_x - 1; x++ ) {
				value =
					( p2[ x + verts_x ] +
					p2[ x - verts_x ] +
					p2[ x + 1 ] +
					p2[ x - 1 ] +
					p2[ x - verts_x - 1 ] +
					p2[ x - verts_x + 1 ] +
					p2[ x + verts_x - 1 ] +
					p2[ x + verts_x + 1 ] +
					p2[ x ] ) * ( 2.0f / 9.0f ) -
					p1[ x ];

				p1[ x ] = value * density;
			}
		}
		break;

	case 1 :
		for ( y = 1; y < verts_y - 1; y++ ) {
			p2 += verts_x;
			p1 += verts_x;
			for ( x = 1; x < verts_x - 1; x++ ) {
				value =
					( p2[ x + verts_x ] +
					p2[ x - verts_x ] +
					p2[ x + 1 ] +
					p2[ x - 1 ] +
					p2[ x - verts_x - 1 ] +
					p2[ x - verts_x + 1 ] +
					p2[ x + verts_x - 1 ] +
					p2[ x + verts_x + 1 ] ) * 0.25f -
					p1[ x ];

				p1[ x ] = value * density;
			}
		}
		break;

	case 2 :
		for ( y = 1; y < verts_y - 1; y++ ) {
			p2 += verts_x;
			p1 += verts_x;
			for ( x = 1; x < verts_x - 1; x++ ) {
				value =
					( p2[ x + verts_x ] +
					p2[ x - verts_x ] +
					p2[ x + 1 ] +
					p2[ x - 1 ] +
					p2[ x - verts_x - 1 ] +
					p2[ x - verts_x + 1 ] +
					p2[ x + verts_x - 1 ] +
					p2[ x + verts_x + 1 ] +
					p2[ x ] ) * ( 1.0f / 9.0f );

				p1[ x ] = value * density;
			}
		}
		break;
	}
}

/*
====================
anLiquidModel::Reset
====================
*/
void anLiquidModel::Reset() {
	int	i, x, y;

	if ( pages.Num() < 2 * verts_x * verts_y ) {
		return;
	}

	nextDropTime = 0;
	time = 0;
	random.SetSeed( seed );

	page1 = pages.Ptr();
	page2 = page1 + verts_x * verts_y;

	for ( i = 0, y = 0; y < verts_y; y++ ) {
		for ( x = 0; x < verts_x; x++, i++ ) {
			page1[i] = 0.0f;
			page2[i] = 0.0f;
			verts[i].xyz.z = 0.0f;
		}
	}
}

/*
====================
anLiquidModel::InitFromFile
====================
*/
void anLiquidModel::InitFromFile( const char *fileName ) {
	int				i, x, y;
	anToken			token;
	anParser		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS );
	anList<int>		tris;
	float			size_x, size_y;
	float			rate;

	name = fileName;

	if ( !parser.LoadFile( fileName ) ) {
		MakeDefaultModel();
		return;
	}

	size_x = scale_x * verts_x;
	size_y = scale_y * verts_y;

	while( parser.ReadToken( &token ) ) {
		if ( !token.Icmp( "seed" ) ) {
			seed = parser.ParseInt();
		} else if ( !token.Icmp( "size_x" ) ) {
			size_x = parser.ParseFloat();
		} else if ( !token.Icmp( "size_y" ) ) {
			size_y = parser.ParseFloat();
		} else if ( !token.Icmp( "verts_x" ) ) {
			verts_x = parser.ParseFloat();
			if ( verts_x < 2 ) {
				parser.Warning( "Invalid # of verts.  Using default model." );
				MakeDefaultModel();
				return;
			}
		} else if ( !token.Icmp( "verts_y" ) ) {
			verts_y = parser.ParseFloat();
			if ( verts_y < 2 ) {
				parser.Warning( "Invalid # of verts.  Using default model." );
				MakeDefaultModel();
				return;
			}
		} else if ( !token.Icmp( "liquid_type" ) ) {
			liquid_type = parser.ParseInt() - 1;
			if ( ( liquid_type < 0 ) || ( liquid_type >= LIQUID_MAX_TYPES ) ) {
				parser.Warning( "Invalid liquid_type.  Using default model." );
				MakeDefaultModel();
				return;
			}
		} else if ( !token.Icmp( "density" ) ) {
			density = parser.ParseFloat();
		} else if ( !token.Icmp( "drop_height" ) ) {
			drop_height = parser.ParseFloat();
		} else if ( !token.Icmp( "drop_radius" ) ) {
			drop_radius = parser.ParseInt();
		} else if ( !token.Icmp( "drop_delay" ) ) {
			drop_delay = SEC2MS( parser.ParseFloat() );
		} else if ( !token.Icmp( "shader" ) ) {
			parser.ReadToken( &token );
			shader = declManager->FindMaterial( token );
		} else if ( !token.Icmp( "seed" ) ) {
			seed = parser.ParseInt();
		} else if ( !token.Icmp( "update_rate" ) ) {
			rate = parser.ParseFloat();
			if ( ( rate <= 0.0f ) || ( rate > 60.0f ) ) {
				parser.Warning( "Invalid update_rate.  Must be between 0 and 60.  Using default model." );
				MakeDefaultModel();
				return;
			}
			update_tics = 1000 / rate;
		} else {
			parser.Warning( "Unknown parameter '%s'.  Using default model.", token.c_str() );
			MakeDefaultModel();
			return;
		}
	}

	scale_x = size_x / ( verts_x - 1 );
	scale_y = size_y / ( verts_y - 1 );

	pages.SetNum( 2 * verts_x * verts_y );
	page1 = pages.Ptr();
	page2 = page1 + verts_x * verts_y;

	verts.SetNum( verts_x * verts_y );
	for ( i = 0, y = 0; y < verts_y; y++ ) {
		for ( x = 0; x < verts_x; x++, i++ ) {
			page1[i] = 0.0f;
			page2[i] = 0.0f;
			verts[i].Clear();
			verts[i].xyz.Set( x * scale_x, y * scale_y, 0.0f );
			verts[i].st.Set( ( float ) x / ( float )( verts_x - 1 ), ( float ) -y / ( float )( verts_y - 1 ) );
		}
	}

	tris.SetNum( ( verts_x - 1 ) * ( verts_y - 1 ) * 6 );
	for ( i = 0, y = 0; y < verts_y - 1; y++ ) {
		for ( x = 1; x < verts_x; x++, i += 6 ) {
			tris[ i + 0 ] = y * verts_x + x;
			tris[ i + 1 ] = y * verts_x + x - 1;
			tris[ i + 2 ] = ( y + 1 ) * verts_x + x - 1;

			tris[ i + 3 ] = ( y + 1 ) * verts_x + x - 1;
			tris[ i + 4 ] = ( y + 1 ) * verts_x + x;
			tris[ i + 5 ] = y * verts_x + x;
		}
	}

	// build the information that will be common to all animations of this mesh:
	// sil edge connectivity and normal / tangent generation information
	deformInfo = R_BuildDeformInfo( verts.Num(), verts.Ptr(), tris.Num(), tris.Ptr(), true );

	bounds.Clear();
	bounds.AddPoint( anVec3( 0.0f, 0.0f, drop_height * -10.0f ) );
	bounds.AddPoint( anVec3( ( verts_x - 1 ) * scale_x, ( verts_y - 1 ) * scale_y, drop_height * 10.0f ) );

	// set the timestamp for reloadmodels
	fileSystem->ReadFile( name, nullptr, &timeStamp );

	Reset();
}

/*
====================
anLiquidModel::InstantiateDynamicModel
====================
*/
anRenderModel *anLiquidModel::InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel ) {
	anModelStatic	*staticModel;
	int		frames;
	int		t;
	float	lerp;

	if ( cachedModel ) {
		delete cachedModel;
		cachedModel = nullptr;
	}

	if ( !deformInfo ) {
		return nullptr;
	}

	if ( !view ) {
		t = 0;
	} else {
		t = view->renderView.time;
	}

	// update the liquid model
	frames = ( t - time ) / update_tics;
	if ( frames > LIQUID_MAX_SKIP_FRAMES ) {
		// don't let time accumalate when skipping frames
		time += update_tics * ( frames - LIQUID_MAX_SKIP_FRAMES );

		frames = LIQUID_MAX_SKIP_FRAMES;
	}

	while( frames > 0 ) {
		Update();
		frames--;
	}

	// create the surface
	lerp = ( float )( t - time ) / ( float )update_tics;
	modelSurface_t surf = GenerateSurface( lerp );

	staticModel = new anModelStatic;
	staticModel->AddSurface( surf );
	staticModel->bounds = surf.geometry->bounds;

	return staticModel;
}

/*
====================
anLiquidModel::IsDynamicModel
====================
*/
dynamicModel_t anLiquidModel::IsDynamicModel() const {
	return DM_CONTINUOUS;
}

/*
====================
anLiquidModel::Bounds
====================
*/
anBounds anLiquidModel::Bounds(const struct renderEntity_s *ent) const {
	// FIXME: need to do this better
	return bounds;
}
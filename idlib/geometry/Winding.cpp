
#include "../Lib.h"
#pragma hdrstop

//===============================================================
//
//	anWinding
//
//===============================================================

/*
=============
anWinding::ReAllocate
=============
*/
bool anWinding::ReAllocate( int n, bool keep ) {
	anVec5 *oldP = p;
	n = ( n + 3 ) & ~3;	// align up to multiple of four
	p = new anVec5[n];
	if ( oldP ) {
		if ( keep ) {
			memcpy( p, oldP, numPoints * sizeof( p[0] ) );
		}
		delete[] oldP;
	}
	allocedSize = n;

	return true;
}

/*
=============
anWinding::BaseForPlane
=============
*/
void anWinding::BaseForPlane( const anVec3 &normal, const float dist ) {
	anVec3 org, vright, vup;

	org = normal * dist;

	normal.NormalVectors( vup, vright );
	vup *= MAX_WORLD_SIZE;
	vright *= MAX_WORLD_SIZE;

	EnsureAlloced( 4 );
	numPoints = 4;
	p[0].ToVec3() = org - vright + vup;
	p[0].s = p[0].t = 0.0f;
	p[1].ToVec3() = org + vright + vup;
	p[1].s = p[1].t = 0.0f;
	p[2].ToVec3() = org + vright - vup;
	p[2].s = p[2].t = 0.0f;
	p[3].ToVec3() = org - vright - vup;
	p[3].s = p[3].t = 0.0f;
}

/*
=============
anWinding::Split
=============
*/
int anWinding::Split( const anPlane &plane, const float epsilon, anWinding **front, anWinding **back ) const {
	float *			dists;
	byte *			sides;
	int				counts[3];
	float			dot;
	int				i, j;
	const anVec5 *	p1, *p2;
	anVec5			mid;
	anWinding *		f, *b;
	int				( maxPts );

	assert( this );

	dists = (float *) _alloca( ( numPoints + 4 ) * sizeof( float ) );
	sides = (byte *) _alloca( ( numPoints + 4 ) * sizeof( byte ) );

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for ( i = 0; i < numPoints; i++ ) {
		dists[i] = dot = plane.Distance( p[i].ToVec3() );
		if ( dot > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*front = *back = nullptr;

	// if coplanar, put on the front side if the normals match
	if ( !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
		anPlane windingPlane;
		GetPlane( windingPlane );
		if ( windingPlane.Normal() * plane.Normal() > 0.0f ) {
			*front = Copy();
			return SIDE_FRONT;
		} else {
			*back = Copy();
			return SIDE_BACK;
		}
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		*back = Copy();
		return SIDE_BACK;
	}

	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		*front = Copy();
		return SIDE_FRONT;
	}

	( maxPts ) = numPoints+4;	// cant use counts[0]+2 because of fp grouping errors

	*front = f = new anWinding( ( maxPts ) );
	*back = b = new anWinding( ( maxPts ) );

	for ( i = 0; i < numPoints; i++ ) {
		p1 = &p[i];
		if ( sides[i] == SIDE_ON ) {
			f->p[f->numPoints] = *p1;
			f->numPoints++;
			b->p[b->numPoints] = *p1;
			b->numPoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			f->p[f->numPoints] = *p1;
			f->numPoints++;
		}

		if ( sides[i] == SIDE_BACK ) {
			b->p[b->numPoints] = *p1;
			b->numPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}

		// generate a split point
		p2 = &p[( i+1 )%numPoints];

		// always calculate the split going from the same side
		// or minor epsilon issues can happen
		if ( sides[i] == SIDE_FRONT ) {
			dot = dists[i] / ( dists[i] - dists[i+1] );
			for ( j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = ( *p1 )[j] + dot * ( ( *p2 )[j] - ( *p1 )[j] );
				}
			}
			mid.s = p1->s + dot * ( p2->s - p1->s );
			mid.t = p1->t + dot * ( p2->t - p1->t );
		} else {
			dot = dists[i+1] / ( dists[i+1] - dists[i] );
			for ( j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = ( *p2 )[j] + dot * ( ( *p1 )[j] - ( *p2 )[j] );
				}
			}
			mid.s = p2->s + dot * ( p1->s - p2->s );
			mid.t = p2->t + dot * ( p1->t - p2->t );
		}

		f->p[f->numPoints] = mid;
		f->numPoints++;
		b->p[b->numPoints] = mid;
		b->numPoints++;
	}

	if ( f->numPoints > ( maxPts ) || b->numPoints > ( maxPts ) ) {
		anLibrary::common->FatalError( "anWinding::Split: points exceeded estimate." );
	}

	return SIDE_CROSS;
}

/*
=============
anWinding::Clip
=============
*/
anWinding *anWinding::Clip( const anPlane &plane, const float epsilon, const bool keepOn ) {
	float *		dists;
	byte *		sides;
	anVec5 *	newPoints;
	int			newNumPoints;
	int			counts[3];
	float		dot;
	int			i, j;
	anVec5 *	p1, *p2;
	anVec5		mid;
	int			( maxPts );

	assert( this );

	dists = (float *) _alloca( ( numPoints + 4 ) * sizeof( float ) );
	sides = (byte *) _alloca( ( numPoints + 4 ) * sizeof( byte ) );

	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	// determine sides for each point
	for ( i = 0; i < numPoints; i++ ) {
		dists[i] = dot = plane.Distance( p[i].ToVec3() );
		if ( dot > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	// if the winding is on the plane and we should keep it
	if ( keepOn && !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
		return this;
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		delete this;
		return nullptr;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		return this;
	}

	( maxPts ) = numPoints + 4;		// cant use counts[0]+2 because of fp grouping errors

	newPoints = (anVec5 *) _alloca16( ( maxPts ) * sizeof( anVec5 ) );
	newNumPoints = 0;

	for ( i = 0; i < numPoints; i++ ) {
		p1 = &p[i];
		if ( newNumPoints+1 > ( maxPts ) ) {
			return this;		// can't split -- fall back to original
		}

		if ( sides[i] == SIDE_ON ) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}

		if ( newNumPoints+1 > ( maxPts ) ) {
			return this;		// can't split -- fall back to original
		}

		// generate a split point
		p2 = &p[( i+1 )%numPoints];

		dot = dists[i] / (dists[i] - dists[i+1] );
		for ( j = 0; j < 3; j++ ) {
			// avoid round off error when possible
			if ( plane.Normal()[j] == 1.0f ) {
				mid[j] = plane.Dist();
			} else if ( plane.Normal()[j] == -1.0f ) {
				mid[j] = -plane.Dist();
			} else {
				mid[j] = ( *p1 )[j] + dot * ( ( *p2 )[j] - ( *p1 )[j] );
			}
		}
		mid.s = p1->s + dot * ( p2->s - p1->s );
		mid.t = p1->t + dot * ( p2->t - p1->t );

		newPoints[newNumPoints] = mid;
		newNumPoints++;
	}

	if ( !EnsureAlloced( newNumPoints, false ) ) {
		return this;
	}

	numPoints = newNumPoints;
	memcpy( p, newPoints, newNumPoints * sizeof(anVec5) );

	return this;
}

/*
=============
anWinding::ClipInPlace
=============
*/
bool anWinding::ClipInPlace( const anPlane &plane, const float epsilon, const bool keepOn ) {
	float*		dists;
	byte *		sides;
	anVec5 *	newPoints;
	int			newNumPoints;
	int			counts[3];
	float		dot;
	int			i, j;
	anVec5 *	p1, *p2;
	anVec5		mid;
	int			( maxPts );

	assert( this );

	dists = (float *) _alloca( ( numPoints + 4 ) * sizeof( float ) );
	sides = (byte *) _alloca( ( numPoints + 4 ) * sizeof( byte ) );

	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	// determine sides for each point
	for ( i = 0; i < numPoints; i++ ) {
		dists[i] = dot = plane.Distance( p[i].ToVec3() );
		if ( dot > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	// if the winding is on the plane and we should keep it
	if ( keepOn && !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
		return true;
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		numPoints = 0;
		return false;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		return true;
	}

	( maxPts ) = numPoints + 4;		// cant use counts[0]+2 because of fp grouping errors

	newPoints = (anVec5 *) _alloca16( ( maxPts ) * sizeof( anVec5 ) );
	newNumPoints = 0;

	for ( i = 0; i < numPoints; i++ ) {
		p1 = &p[i];
		if ( newNumPoints+1 > ( maxPts ) ) {
			return true;		// can't split -- fall back to original
		}
		if ( sides[i] == SIDE_ON ) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}

		if ( newNumPoints+1 > ( maxPts ) ) {
			return true;		// can't split -- fall back to original
		}

		// generate a split point
		p2 = &p[( i+1 )%numPoints];

		dot = dists[i] / (dists[i] - dists[i+1] );
		for ( j = 0; j < 3; j++ ) {
			// avoid round off error when possible
			if ( plane.Normal()[j] == 1.0f ) {
				mid[j] = plane.Dist();
			} else if ( plane.Normal()[j] == -1.0f ) {
				mid[j] = -plane.Dist();
			} else {
				mid[j] = ( *p1 )[j] + dot * ( ( *p2 )[j] - ( *p1 )[j] );
			}
		}
		mid.s = p1->s + dot * ( p2->s - p1->s );
		mid.t = p1->t + dot * ( p2->t - p1->t );

		newPoints[newNumPoints] = mid;
		newNumPoints++;
	}

	if ( !EnsureAlloced( newNumPoints, false ) ) {
		return true;
	}

	numPoints = newNumPoints;
	memcpy( p, newPoints, newNumPoints * sizeof(anVec5) );

	return true;
}

/*
=============
anWinding::Copy
=============
*/
anWinding *anWinding::Copy( void ) const {
	anWinding *w = new anWinding( numPoints );
	w->numPoints = numPoints;
	memcpy( w->p, p, numPoints * sizeof( p[0] ) );
	return w;
}

/*
=============
anWinding::Reverse
=============
*/
anWinding *anWinding::Reverse( void ) const {
	anWinding *w = new anWinding( numPoints );
	w->numPoints = numPoints;
	for ( int i = 0; i < numPoints; i++ ) {
		w->p[ numPoints - i - 1 ] = p[i];
	}
	return w;
}

/*
=============
anWinding::ReverseSelf
=============
*/
void anWinding::ReverseSelf( void ) {
	for ( int i = 0; i < (numPoints>>1 ); i++ ) {
		anVec5 v = p[i];
		p[i] = p[numPoints - i - 1];
		p[numPoints - i - 1] = v;
	}
}

/*
=============
anWinding::Check
=============
*/
bool anWinding::Check( bool print ) const {
	anPlane plane;

	if ( numPoints < 3 ) {
		if ( print ) {
			anLibrary::common->Printf( "anWinding::Check: only %i points.", numPoints );
		}
		return false;
	}

	float area = GetArea();
	if ( area < 1.0f ) {
		if ( print ) {
			anLibrary::common->Printf( "anWinding::Check: tiny area: %f", area );
		}
		return false;
	}

	GetPlane( plane );

	for ( int i = 0; i < numPoints; i++ ) {
		const anVec3 &p1 = p[i].ToVec3();
		// check if the winding is huge
		for ( int j = 0; j < 3; j++ ) {
			if ( p1[j] >= MAX_WORLD_COORD || p1[j] <= MIN_WORLD_COORD ) {
				if ( print ) {
					anLibrary::common->Printf( "anWinding::Check: point %d outside world %c-axis: %f", i, 'X'+j, p1[j] );
				}
				return false;
			}
		}

		int j = i + 1 == numPoints ? 0 : i + 1;

		// check if the point is on the face plane
		float d = p1 * plane.Normal() + plane[3];
		if ( d < -ON_EPSILON || d > ON_EPSILON ) {
			if ( print ) {
				anLibrary::common->Printf( "anWinding::Check: point %d off plane.", i );
			}
			return false;
		}

		// check if the edge isn't degenerate
		const anVec3 &p2 = p[j].ToVec3();
		anVec3 dir = p2 - p1;

		if ( dir.Length() < ON_EPSILON) {
			if ( print ) {
				anLibrary::common->Printf( "anWinding::Check: edge %d is degenerate.", i );
			}
			return false;
		}

		// check if the winding is convex
		anVec3 edgenormal = plane.Normal().Cross( dir );
		edgenormal.Normalize();
		float edgedist = p1 * edgenormal;
		edgedist += ON_EPSILON;

		// all other points must be on front side
		for ( int j = 0; j < numPoints; j++ ) {
			if ( j == i ) {
				continue;
			}
			float d = p[j].ToVec3() * edgenormal;
			if ( d > edgedist ) {
				if ( print ) {
					anLibrary::common->Printf( "anWinding::Check: non-convex." );
				}
				return false;
			}
		}
	}
	return true;
}

/*
=============
anWinding::GetArea
=============
*/
float anWinding::GetArea( void ) const {
	float total = 0.0f;
	for ( int  i = 2; i < numPoints; i++ ) {
		anVec3 d1 = p[i-1].ToVec3() - p[0].ToVec3();
		anVec3 d2 = p[i].ToVec3() - p[0].ToVec3();
		anVec3 cross = d1.Cross( d2 );
		total += cross.Length();
	}
	return total * 0.5f;
}

/*
=============
anWinding::GetRadius
=============
*/
float anWinding::GetRadius( const anVec3 &center ) const {
	float radius = 0.0f;
	for ( int i = 0; i < numPoints; i++ ) {
		anVec3 dir = p[i].ToVec3() - center;
		float r = dir * dir;
		if ( r > radius ) {
			float radius = r;
		}
	}
	return anMath::Sqrt( radius );
}

/*
=============
anWinding::GetCenter
=============
*/
anVec3 anWinding::GetCenter( void ) const {
	anVec3 center.Zero();
	for ( int i = 0; i < numPoints; i++ ) {
		center += p[i].ToVec3();
	}
	center *= ( 1.0f / numPoints );
	return center;
}

/*
=============
anWinding::GetPlane
=============
*/
void anWinding::GetPlane( anVec3 &normal, float &dist ) const {
	if ( numPoints < 3 ) {
		normal.Zero();
		dist = 0.0f;
		return;
	}

	anVec3 center = GetCenter();
	anVec3 v1 = p[0].ToVec3() - center;
	anVec3 v2 = p[1].ToVec3() - center;
	normal = v2.Cross( v1 );
	normal.Normalize();
	dist = p[0].ToVec3() * normal;
}

/*
=============
anWinding::GetPlane
=============
*/
void anWinding::GetPlane( anPlane &plane ) const {
	if ( numPoints < 3 ) {
		plane.Zero();
		return;
	}

	anVec3 center = GetCenter();
	anVec3 v1 = p[0].ToVec3() - center;
	anVec3 v2 = p[1].ToVec3() - center;
	plane.SetNormal( v2.Cross( v1 ) );
	plane.Normalize();
	plane.FitThroughPoint( p[0].ToVec3() );
}

/*
=============
anWinding::GetBounds
=============
*/
void anWinding::GetBounds( anBounds &bounds ) const {
	if ( !numPoints ) {
		bounds.Clear();
		return;
	}

	bounds[0] = bounds[1] = p[0].ToVec3();
	for ( int i = 1; i < numPoints; i++ ) {
		if ( p[i].x < bounds[0].x ) {
			bounds[0].x = p[i].x;
		} else if ( p[i].x > bounds[1].x ) {
			bounds[1].x = p[i].x;
		}
		if ( p[i].y < bounds[0].y ) {
			bounds[0].y = p[i].y;
		} else if ( p[i].y > bounds[1].y ) {
			bounds[1].y = p[i].y;
		}
		if ( p[i].z < bounds[0].z ) {
			bounds[0].z = p[i].z;
		} else if ( p[i].z > bounds[1].z ) {
			bounds[1].z = p[i].z;
		}
	}
}

/*
=============
anWinding::RemoveEqualPoints
=============
*/
void anWinding::RemoveEqualPoints( const float epsilon ) {
	for ( int  i = 0; i < numPoints; i++ ) {
		if ( ( p[i].ToVec3() - p[( i+numPoints-1 )%numPoints].ToVec3() ).LengthSqr() >= Square( epsilon ) ) {
			continue;
		}
		numPoints--;
		for ( int j = i; j < numPoints; j++ ) {
			p[j] = p[j+1];
		}
		i--;
	}
}

/*
=============
anWinding::RemoveColinearPoints
=============
*/
void anWinding::RemoveColinearPoints( const anVec3 &normal, const float epsilon ) {
	if ( numPoints <= 3 ) {
		return;
	}

	for ( int i = 0; i < numPoints; i++ ) {
		// create plane through edge orthogonal to winding plane
		anVec3 edgeNormal = (p[i].ToVec3() - p[( i+numPoints-1 )%numPoints].ToVec3() ).Cross( normal );
		edgeNormal.Normalize();
		float dist = edgeNormal * p[i].ToVec3();
		if ( anMath::Fabs( edgeNormal * p[( i+1 )%numPoints].ToVec3() - dist ) > epsilon ) {
			continue;
		}

		numPoints--;
		for ( int j = i; j < numPoints; j++ ) {
			p[j] = p[j+1];
		}
		i--;
	}
}

/*
=============
anWinding::AddToConvexHull

  Adds the given winding to the convex hull.
  Assumes the current winding already is a convex hull with three or more points.
=============
*/
void anWinding::AddToConvexHull( const anWinding *winding, const anVec3 &normal, const float epsilon ) {
	int				i, j, k;
	anVec3			dir;
	float			d;
	int				maxPts;
	anVec3 *		hullDirs;
	bool *			hullSide;
	bool			outside;
	int				numNewHullPoints;
	anVec5 *		newHullPoints;

	if ( !winding ) {
		return;
	}

	maxPts = this->numPoints + winding->numPoints;

	if ( !this->EnsureAlloced( maxPts, true ) ) {
		return;
	}

	newHullPoints = (anVec5 *) _alloca( maxPts * sizeof( anVec5 ) );
	hullDirs = (anVec3 *) _alloca( maxPts * sizeof( anVec3 ) );
	hullSide = (bool *) _alloca( maxPts * sizeof( bool ) );

	for ( i = 0; i < winding->numPoints; i++ ) {
		const anVec5 &p1 = winding->p[i];

		// calculate hull edge vectors
		for ( j = 0; j < this->numPoints; j++ ) {
			dir = this->p[ (j + 1 ) % this->numPoints ].ToVec3() - this->p[ j ].ToVec3();
			dir.Normalize();
			hullDirs[j] = normal.Cross( dir );
		}

		// calculate side for each hull edge
		outside = false;
		for ( j = 0; j < this->numPoints; j++ ) {
			dir = p1.ToVec3() - this->p[j].ToVec3();
			d = dir * hullDirs[j];
			if ( d >= epsilon ) {
				outside = true;
			}
			if ( d >= -epsilon ) {
				hullSide[j] = true;
			} else {
				hullSide[j] = false;
			}
		}

		// if the point is effectively inside, do nothing
		if ( !outside ) {
			continue;
		}

		// find the back side to front side transition
		for ( j = 0; j < this->numPoints; j++ ) {
			if ( !hullSide[ j ] && hullSide[ (j + 1 ) % this->numPoints ] ) {
				break;
			}
		}
		if ( j >= this->numPoints ) {
			continue;
		}

		// insert the point here
		newHullPoints[0] = p1;
		numNewHullPoints = 1;

		// copy over all points that aren't double fronts
		j = (j+1 ) % this->numPoints;
		for ( k = 0; k < this->numPoints; k++ ) {
			if ( hullSide[ (j+k) % this->numPoints ] && hullSide[ (j+k+1 ) % this->numPoints ] ) {
				continue;
			}
			newHullPoints[numNewHullPoints] = this->p[ (j+k+1 ) % this->numPoints ];
			numNewHullPoints++;
		}

		this->numPoints = numNewHullPoints;
		memcpy( this->p, newHullPoints, numNewHullPoints * sizeof(anVec5) );
	}
}

/*
=============
anWinding::AddToConvexHull

  Add a point to the convex hull.
  The current winding must be convex but may be degenerate and can have less than three points.
=============
*/
void anWinding::AddToConvexHull( const anVec3 &point, const anVec3 &normal, const float epsilon ) {
	int				j, k, numHullPoints;
	anVec3			dir;
	float			d;
	anVec3 *		hullDirs;
	bool *			hullSide;
	anVec5 *		hullPoints;
	bool			outside;

	switch ( numPoints ) {
		case 0: {
			p[0] = point;
			numPoints++;
			return;
		}
		case 1: {
			// don't add the same point second
			if ( p[0].ToVec3().Compare( point, epsilon ) ) {
				return;
			}
			p[1].ToVec3() = point;
			numPoints++;
			return;
		}
		case 2: {
			// don't add a point if it already exists
			if ( p[0].ToVec3().Compare( point, epsilon ) || p[1].ToVec3().Compare( point, epsilon ) ) {
				return;
			}
			// if only two points make sure we have the right ordering according to the normal
			dir = point - p[0].ToVec3();
			dir = dir.Cross( p[1].ToVec3() - p[0].ToVec3() );
			if ( dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f ) {
				// points don't make a plane
				return;
			}
			if ( dir * normal > 0.0f ) {
				p[2].ToVec3() = point;
			} else {
				p[2] = p[1];
				p[1].ToVec3() = point;
			}
			numPoints++;
			return;
		}
	}

	hullDirs = (anVec3 *) _alloca( numPoints * sizeof( anVec3 ) );
	hullSide = (bool *) _alloca( numPoints * sizeof( bool ) );

	// calculate hull edge vectors
	for ( j = 0; j < numPoints; j++ ) {
		dir = p[(j + 1 ) % numPoints].ToVec3() - p[j].ToVec3();
		hullDirs[j] = normal.Cross( dir );
	}

	// calculate side for each hull edge
	outside = false;
	for ( j = 0; j < numPoints; j++ ) {
		dir = point - p[j].ToVec3();
		d = dir * hullDirs[j];
		if ( d >= epsilon ) {
			outside = true;
		}
		if ( d >= -epsilon ) {
			hullSide[j] = true;
		} else {
			hullSide[j] = false;
		}
	}

	// if the point is effectively inside, do nothing
	if ( !outside ) {
		return;
	}

	// find the back side to front side transition
	for ( j = 0; j < numPoints; j++ ) {
		if ( !hullSide[ j ] && hullSide[ (j + 1 ) % numPoints ] ) {
			break;
		}
	}
	if ( j >= numPoints ) {
		return;
	}

	hullPoints = (anVec5 *) _alloca( (numPoints+1 ) * sizeof( anVec5 ) );

	// insert the point here
	hullPoints[0] = point;
	numHullPoints = 1;

	// copy over all points that aren't double fronts
	j = (j+1 ) % numPoints;
	for ( k = 0; k < numPoints; k++ ) {
		if ( hullSide[ (j+k) % numPoints ] && hullSide[ (j+k+1 ) % numPoints ] ) {
			continue;
		}
		hullPoints[numHullPoints] = p[ (j+k+1 ) % numPoints ];
		numHullPoints++;
	}

	if ( !EnsureAlloced( numHullPoints, false ) ) {
		return;
	}
	numPoints = numHullPoints;
	memcpy( p, hullPoints, numHullPoints * sizeof(anVec5) );
}

/*
=============
anWinding::TryMerge
=============
*/
#define	CONTINUOUS_EPSILON	0.005f

anWinding *anWinding::TryMerge( const anWinding &w, const anVec3 &planenormal, int keep ) const {
	anVec3			*p1, *p2, *p3, *p4, *back;
	anWinding		*newf;
	const anWinding	*f1, *f2;
	int				i, j, k, l;
	anVec3			normal, delta;
	float			dot;
	bool			keep1, keep2;

	f1 = this;
	f2 = &w;
	//
	// find a anLibrary::common edge
	//
	p1 = p2 = nullptr;	// stop compiler warning
	j = 0;

	for ( i = 0; i < f1->numPoints; i++ ) {
		p1 = &f1->p[i].ToVec3();
		p2 = &f1->p[( i+1 ) % f1->numPoints].ToVec3();
		for ( j = 0; j < f2->numPoints; j++ ) {
			p3 = &f2->p[j].ToVec3();
			p4 = &f2->p[(j+1 ) % f2->numPoints].ToVec3();
			for (k = 0; k < 3; k++ ) {
				if ( anMath::Fabs( ( *p1 )[k] - (*p4)[k] ) > 0.1f ) {
					break;
				}
				if ( anMath::Fabs( ( *p2 )[k] - (*p3)[k] ) > 0.1f ) {
					break;
				}
			}
			if ( k == 3 ) {
				break;
			}
		}
		if ( j < f2->numPoints ) {
			break;
		}
	}

	if ( i == f1->numPoints ) {
		return nullptr;			// no matching edges
	}

	//
	// check slope of connected lines
	// if the slopes are colinear, the point can be removed
	//
	back = &f1->p[( i+f1->numPoints-1 )%f1->numPoints].ToVec3();
	delta = ( *p1 ) - (*back);
	normal = planenormal.Cross(delta);
	normal.Normalize();

	back = &f2->p[(j+2)%f2->numPoints].ToVec3();
	delta = (*back) - ( *p1 );
	dot = delta * normal;
	if ( dot > CONTINUOUS_EPSILON ) {
		return nullptr;			// not a convex polygon
	}

	keep1 = (bool)(dot < -CONTINUOUS_EPSILON);

	back = &f1->p[( i+2)%f1->numPoints].ToVec3();
	delta = (*back) - ( *p2 );
	normal = planenormal.Cross( delta );
	normal.Normalize();

	back = &f2->p[(j+f2->numPoints-1 )%f2->numPoints].ToVec3();
	delta = (*back) - ( *p2 );
	dot = delta * normal;
	if ( dot > CONTINUOUS_EPSILON ) {
		return nullptr;			// not a convex polygon
	}

	keep2 = (bool)(dot < -CONTINUOUS_EPSILON);

	//
	// build the new polygon
	//
	newf = new anWinding( f1->numPoints + f2->numPoints );

	// copy first polygon
	for ( k = ( i+1 ) % f1->numPoints; k != i; k = (k+1 ) % f1->numPoints ) {
		if ( !keep && k == ( i+1 ) % f1->numPoints && !keep2 ) {
			continue;
		}

		newf->p[newf->numPoints] = f1->p[k];
		newf->numPoints++;
	}

	// copy second polygon
	for ( l = (j+1 ) % f2->numPoints; l != j; l = (l+1 ) % f2->numPoints ) {
		if ( !keep && l == (j+1 ) % f2->numPoints && !keep1 ) {
			continue;
		}
		newf->p[newf->numPoints] = f2->p[l];
		newf->numPoints++;
	}

	return newf;
}

/*
=============
anWinding::RemovePoint
=============
*/
void anWinding::RemovePoint( int point ) {
	if ( point < 0 || point >= numPoints ) {
		anLibrary::common->FatalError( "anWinding::removePoint: point out of range" );
	}
	if ( point < numPoints - 1 ) {
		memmove( &p[point], &p[point+1], ( numPoints - point - 1 ) * sizeof( p[0] ) );
	}
	numPoints--;
}

/*
=============
anWinding::InsertPoint
=============
*/
void anWinding::InsertPoint( const anVec3 &point, int spot ) {
	if ( spot > numPoints ) {
		anLibrary::common->FatalError( "anWinding::insertPoint: spot > numPoints" );
	}

	if ( spot < 0 ) {
		anLibrary::common->FatalError( "anWinding::insertPoint: spot < 0" );
	}

	EnsureAlloced( numPoints+1, true );
	for ( int i = numPoints; i > spot; i-- ) {
		p[i] = p[i-1];
	}
	p[spot] = point;
	numPoints++;
}

/*
=============
anWinding::InsertPointIfOnEdge
=============
*/
bool anWinding::InsertPointIfOnEdge( const anVec3 &point, const anPlane &plane, const float epsilon ) {
	float dist, dot;
	anVec3 normal;

	// point may not be too far from the winding plane
	if ( anMath::Fabs( plane.Distance( point ) ) > epsilon ) {
		return false;
	}

	for ( int i = 0; i < numPoints; i++ ) {
		// create plane through edge orthogonal to winding plane
		anVec3 normal = (p[( i+1 )%numPoints].ToVec3() - p[i].ToVec3() ).Cross( plane.Normal() );
		normal.Normalize();
		float dist = normal * p[i].ToVec3();

		if ( anMath::Fabs( normal * point - dist ) > epsilon ) {
			continue;
		}

		anVec3 normal = plane.Normal().Cross( normal );
		float dot = normal * point;

		float dist = dot - normal * p[i].ToVec3();

		if ( dist < epsilon ) {
			// if the winding already has the point
			if ( dist > -epsilon ) {
				return false;
			}
			continue;
		}

		float dist = dot - normal * p[( i+1 )%numPoints].ToVec3();

		if ( dist > -epsilon ) {
			// if the winding already has the point
			if ( dist < epsilon ) {
				return false;
			}
			continue;
		}

		InsertPoint( point, i+1 );
		return true;
	}
	return false;
}

/*
=============
anWinding::IsTiny
=============
*/
#define	EDGE_LENGTH		0.2f

bool anWinding::IsTiny( void ) const {
	int edges = 0;
	for ( int i = 0; i < numPoints; i++ ) {
		anVec3 delta = p[( i+1 )%numPoints].ToVec3() - p[i].ToVec3();
		float len = delta.Length();
		if ( len > EDGE_LENGTH ) {
			if ( ++edges == 3 ) {
				return false;
			}
		}
	}
	return true;
}

/*
=============
anWinding::IsHuge
=============
*/
bool anWinding::IsHuge( void ) const {
	for ( int i = 0; i < numPoints; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			if ( p[i][j] <= MIN_WORLD_COORD || p[i][j] >= MAX_WORLD_COORD ) {
				return true;
			}
		}
	}
	return false;
}

/*
=============
anWinding::Print
=============
*/
void anWinding::Print( void ) const {
	for ( int i = 0; i < numPoints; i++ ) {
		anLibrary::common->Printf( "(%5.1f, %5.1f, %5.1f)\n", p[i][0], p[i][1], p[i][2] );
	}
}

/*
=============
anWinding::PlaneDistance
=============
*/
float anWinding::PlaneDistance( const anPlane &plane ) const {
	int		i;
	float	d, min, max;

	min = anMath::INFINITY;
	max = -min;
	for ( i = 0; i < numPoints; i++ ) {
		d = plane.Distance( p[i].ToVec3() );
		if ( d < min ) {
			min = d;
			if ( FLOATSIGNBITSET( min ) & FLOATSIGNBITNOTSET( max ) ) {
				return 0.0f;
			}
		}
		if ( d > max ) {
			max = d;
			if ( FLOATSIGNBITSET( min ) & FLOATSIGNBITNOTSET( max ) ) {
				return 0.0f;
			}
		}
	}
	if ( FLOATSIGNBITNOTSET( min ) ) {
		return min;
	}
	if ( FLOATSIGNBITSET( max ) ) {
		return max;
	}
	return 0.0f;
}

/*
=============
anWinding::PlaneSide
=============
*/
int anWinding::PlaneSide( const anPlane &plane, const float epsilon ) const {
	bool	front, back;
	int		i;
	float	d;

	front = false;
	back = false;
	for ( i = 0; i < numPoints; i++ ) {
		d = plane.Distance( p[i].ToVec3() );
		if ( d < -epsilon ) {
			if ( front ) {
				return SIDE_CROSS;
			}
			back = true;
			continue;
		}
		else if ( d > epsilon ) {
			if ( back ) {
				return SIDE_CROSS;
			}
			front = true;
			continue;
		}
	}

	if ( back ) {
		return SIDE_BACK;
	}
	if ( front ) {
		return SIDE_FRONT;
	}
	return SIDE_ON;
}

/*
=============
anWinding::PlanesConcave
=============
*/
#define WCONVEX_EPSILON		0.2f

bool anWinding::PlanesConcave( const anWinding &w2, const anVec3 &normal1, const anVec3 &normal2, float dist1, float dist2 ) const {
	// check if one of the points of winding 1 is at the back of the plane of winding 2
	for ( int i = 0; i < numPoints; i++ ) {
		if ( normal2 * p[i].ToVec3() - dist2 > WCONVEX_EPSILON ) {
			return true;
		}
	}
	// check if one of the points of winding 2 is at the back of the plane of winding 1
	for ( int i = 0; i < w2.numPoints; i++ ) {
		if ( normal1 * w2.p[i].ToVec3() - dist1 > WCONVEX_EPSILON ) {
			return true;
		}
	}

	return false;
}

/*
=============
anWinding::PointInside
=============
*/
bool anWinding::PointInside( const anVec3 &normal, const anVec3 &point, const float epsilon ) const {
	for ( int i = 0; i < numPoints; i++ ) {
		anVec3 dir = p[( i+1 ) % numPoints].ToVec3() - p[i].ToVec3();
		anVec3 pointvec = point - p[i].ToVec3();
		anVec3 n = dir.Cross( normal );
		if ( pointvec * n < -epsilon ) {
			return false;
		}
	}
	return true;
}

/*
=============
anWinding::LineIntersection
=============
*/
bool anWinding::LineIntersection( const anPlane &windingPlane, const anVec3 &start, const anVec3 &end, bool backFaceCull ) const {
	float front = windingPlane.Distance( start );
	float back = windingPlane.Distance( end );

	// if both points at the same side of the plane
	if ( front < 0.0f && back < 0.0f ) {
		return false;
	}

	if ( front > 0.0f && back > 0.0f ) {
		return false;
	}

	// if back face culled
	if ( backFaceCull && front < 0.0f ) {
		return false;
	}

	// get point of intersection with winding plane
	if ( anMath::Fabs( front - back ) < 0.0001f ) {
		anVec3 mid = end;
	} else {
		float frac = front / (front - back);
		anVec3 mid[0] = start[0] + (end[0] - start[0] ) * frac;
		anVec3 mid[1] = start[1] + (end[1] - start[1] ) * frac;
		anVec3 mid[2] = start[2] + (end[2] - start[2] ) * frac;
	}

	return PointInside( windingPlane.Normal(), mid, 0.0f );
}

/*
=============
anWinding::RayIntersection
=============
*/
bool anWinding::RayIntersection( const anPlane &windingPlane, const anVec3 &start, const anVec3 &dir, float &scale, bool backFaceCull ) const {
	bool side, lastside = false;
	anPluecker pl1, pl2;

	scale = 0.0f;
	pl1.FromRay( start, dir );
	for ( int i = 0; i < numPoints; i++ ) {
		pl2.FromLine( p[i].ToVec3(), p[( i+1 )%numPoints].ToVec3() );
		side = pl1.PermutedInnerProduct( pl2 ) > 0.0f;
		if ( i && side != lastside ) {
			return false;
		}
		lastside = side;
	}
	if ( !backFaceCull || lastside ) {
		windingPlane.RayIntersection( start, dir, scale );
		return true;
	}
	return false;
}

/*
=================
anWinding::TriangleArea
=================
*/
float anWinding::TriangleArea( const anVec3 &a, const anVec3 &b, const anVec3 &c ) {
	anVec3 v1 = b - a;
	anVec3 v2 = c - a;
	anVec3 cross = v1.Cross( v2 );
	return 0.5f * cross.Length();
}


//===============================================================
//
//	anFixedWinding
//
//===============================================================

/*
=============
anFixedWinding::ReAllocate
=============
*/
bool anFixedWinding::ReAllocate( int n, bool keep ) {
	assert( n <= MAX_POINTS_ON_WINDING );

	if ( n > MAX_POINTS_ON_WINDING ) {
		anLibrary::common->Printf( "WARNING: anFixedWinding -> MAX_POINTS_ON_WINDING overflowed\n" );
		return false;
	}
	return true;
}

/*
=============
anFixedWinding::Split
=============
*/
int anFixedWinding::Split( anFixedWinding *back, const anPlane &plane, const float epsilon ) {
	int		counts[3];
	float	dists[MAX_POINTS_ON_WINDING+4];
	byte	sides[MAX_POINTS_ON_WINDING+4];
	anVec5	mid;

	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	// determine sides for each point
	for ( int i = 0; i < numPoints; i++ ) {
		dists[i] = dot = plane.Distance( p[i].ToVec3() );
		if ( dot > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	if ( !counts[SIDE_BACK] ) {
		if ( !counts[SIDE_FRONT] ) {
			return SIDE_ON;
		} else {
			return SIDE_FRONT;
		}
	}

	if ( !counts[SIDE_FRONT] ) {
		return SIDE_BACK;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];

	out.numPoints = 0;
	back->numPoints = 0;
	anFixedWinding out;

	for ( int i = 0; i < numPoints; i++ ) {
		anVec5 *p1 = &p[i];
		if ( !out.EnsureAlloced( out.numPoints+1, true ) !back->EnsureAlloced( back->numPoints+1, true ) ) {
			return SIDE_FRONT;		// can't split -- fall back to original
		}

		if ( sides[i] == SIDE_ON ) {
			anFixedWinding out.p[out.numPoints] = *p1;
			out.numPoints++;
			back->p[back->numPoints] = *p1;
			back->numPoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			out.p[out.numPoints] = *p1;
			out.numPoints++;
		}
		if ( sides[i] == SIDE_BACK ) {
			back->p[back->numPoints] = *p1;
			back->numPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}

		if ( !out.EnsureAlloced( out.numPoints+1, true ) ) {
			return SIDE_FRONT;		// can't split -- fall back to original
		}

		if ( !back->EnsureAlloced( back->numPoints+1, true ) ) {
			return SIDE_FRONT;		// can't split -- fall back to original
		}

		// generate a split point
		int j = i + 1;
		if ( j >= numPoints ) {
			anVec5 *p2 = &p[0];
		} else {
			anVec5 *p2 = &p[j];
		}

		float dot = dists[i] / (dists[i] - dists[i+1] );
		for ( int j = 0; j < 3; j++ ) {
			// avoid round off error when possible
			if ( plane.Normal()[j] == 1.0f ) {
				mid[j] = plane.Dist();
			} else if ( plane.Normal()[j] == -1.0f ) {
				mid[j] = -plane.Dist();
			} else {
				mid[j] = ( *p1 )[j] + dot * ( ( *p2 )[j] - ( *p1 )[j] );
			}
		}
		mid.s = p1->s + dot * ( p2->s - p1->s );
		mid.t = p1->t + dot * ( p2->t - p1->t );

		out.p[out.numPoints] = mid;
		out.numPoints++;
		back->p[back->numPoints] = mid;
		back->numPoints++;
	}
	for ( i = 0; i < out.numPoints; i++ ) {
		p[i] = out.p[i];
	}
	numPoints = out.numPoints;

	return SIDE_CROSS;
}

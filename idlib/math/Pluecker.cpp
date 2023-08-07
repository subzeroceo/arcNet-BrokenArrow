#include "../Lib.h"
#pragma hdrstop

anPluecker pluecker_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );

/*
================
anPluecker::FromPlanes

  pluecker coordinate for the intersection of two planes
================
*/
bool anPluecker::FromPlanes( const anPlane &p1, const anPlane &p2 ) {
	p[0] = -( p1[2] * -p2[3] - p2[2] * -p1[3] );
	p[1] = -( p2[1] * -p1[3] - p1[1] * -p2[3] );
	p[2] = p1[1] * p2[2] - p2[1] * p1[2];

	p[3] = -( p1[0] * -p2[3] - p2[0] * -p1[3] );
	p[4] = p1[0] * p2[1] - p2[0] * p1[1];
	p[5] = p1[0] * p2[2] - p2[0] * p1[2];

	return ( p[2] != 0.0f || p[5] != 0.0f || p[4] != 0.0f );
}

/*
================
anPluecker::Distance3DSqr

  calculates square of shortest distance between the two
  3D lines represented by their pluecker coordinates
================
*/
float anPluecker::Distance3DSqr( const anPluecker &a ) const {
	float d, s;
	anVec3 dir;

	dir[0] = -a.p[5] *  p[4] -  a.p[4] * -p[5];
	dir[1] =  a.p[4] *  p[2] -  a.p[2] *  p[4];
	dir[2] =  a.p[2] * -p[5] - -a.p[5] *  p[2];
	if ( dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f ) {
		return -1.0f;	// FIXME: implement for parallel lines
	}
	d = a.p[4] * ( p[2]*dir[1] - -p[5]*dir[0] ) +
		a.p[5] * ( p[2]*dir[2] -  p[4]*dir[0] ) +
		a.p[2] * (-p[5]*dir[2] -  p[4]*dir[1] );
	s = PermutedInnerProduct( a ) / d;
	return ( dir * dir ) * ( s * s );
}

/*
=============
anPluecker::ToString
=============
*/
const char *anPluecker::ToString( int precision ) const {
	return anStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

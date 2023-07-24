#include "../precompiled.h"
#pragma hdrstop

//#define FRUSTUM_DEBUG

/*
  bit 0 = min x
  bit 1 = max x
  bit 2 = min y
  bit 3 = max y
  bit 4 = min z
  bit 5 = max z
*/
static int boxVertPlanes[8] = {
	( (1<<0 ) | (1<<2) | (1<<4) ),
	( (1<<1 ) | (1<<2) | (1<<4) ),
	( (1<<1 ) | (1<<3) | (1<<4) ),
	( (1<<0 ) | (1<<3) | (1<<4) ),
	( (1<<0 ) | (1<<2) | (1<<5) ),
	( (1<<1 ) | (1<<2) | (1<<5) ),
	( (1<<1 ) | (1<<3) | (1<<5) ),
	( (1<<0 ) | (1<<3) | (1<<5) ),
};

/*
============
BoxToPoints
============
*/
void BoxToPoints( const arcVec3 &center, const arcVec3 &extents, const arcMat3 &axis, arcVec3 points[8] ) {
	arcMat3 ax;
	arcVec3 temp[4];

	ax[0] = extents[0] * axis[0];
	ax[1] = extents[1] * axis[1];
	ax[2] = extents[2] * axis[2];
	temp[0] = center - ax[0];
	temp[1] = center + ax[0];
	temp[2] = ax[1] - ax[2];
	temp[3] = ax[1] + ax[2];
	points[0] = temp[0] - temp[3];
	points[1] = temp[1] - temp[3];
	points[2] = temp[1] + temp[2];
	points[3] = temp[0] + temp[2];
	points[4] = temp[0] - temp[2];
	points[5] = temp[1] - temp[2];
	points[6] = temp[1] + temp[3];
	points[7] = temp[0] + temp[3];
}

/*
================
ARCFrustum::PlaneDistance
================
*/
float ARCFrustum::PlaneDistance( const arcPlane &plane ) const {
	float min, max;

	AxisProjection( plane.Normal(), min, max );
	if ( min + plane[3] > 0.0f ) {
		return min + plane[3];
	}
	if ( max + plane[3] < 0.0f ) {
		return max + plane[3];
	}
	return 0.0f;
}

/*
================
ARCFrustum::PlaneSide
================
*/
int ARCFrustum::PlaneSide( const arcPlane &plane, const float epsilon ) const {
	float min, max;

	AxisProjection( plane.Normal(), min, max );
	if ( min + plane[3] > epsilon ) {
		return PLANESIDE_FRONT;
	}
	if ( max + plane[3] < epsilon ) {
		return PLANESIDE_BACK;
	}
	return PLANESIDE_CROSS;
}

/*
============
ARCFrustum::CullPoint
============
*/
bool ARCFrustum::CullPoint( const arcVec3 &point ) const {
	arcVec3 p;
	float scale;

	// transform point to frustum space
	p = ( point - origin ) * axis.Transpose();
	// test whether or not the point is within the frustum
	if ( p.x < dNear || p.x > dFar ) {
		return true;
	}
	scale = p.x * invFar;
	if ( arcMath::Fabs( p.y ) > dLeft * scale ) {
		return true;
	}
	if ( arcMath::Fabs( p.z ) > dUp * scale ) {
		return true;
	}
	return false;
}

/*
============
ARCFrustum::CullLocalBox

  Tests if any of the planes of the frustum can be used as a separating plane.

   3 muls best case
  25 muls worst case
============
*/
bool ARCFrustum::CullLocalBox( const arcVec3 &localOrigin, const arcVec3 &extents, const arcMat3 &localAxis ) const {
	float d1, d2;
	arcVec3 testOrigin;
	arcMat3 testAxis;

	// near plane
	d1 = dNear - localOrigin.x;
	d2 = arcMath::Fabs( extents[0] * localAxis[0][0] ) +
				arcMath::Fabs( extents[1] * localAxis[1][0] ) +
						arcMath::Fabs( extents[2] * localAxis[2][0] );
	if ( d1 - d2 > 0.0f ) {
		return true;
	}

	// far plane
	d1 = localOrigin.x - dFar;
	if ( d1 - d2 > 0.0f ) {
		return true;
	}

	testOrigin = localOrigin;
	testAxis = localAxis;

	if ( testOrigin.y < 0.0f ) {
		testOrigin.y = -testOrigin.y;
		testAxis[0][1] = -testAxis[0][1];
		testAxis[1][1] = -testAxis[1][1];
		testAxis[2][1] = -testAxis[2][1];
	}

	// test left/right planes
	d1 = dFar * testOrigin.y - dLeft * testOrigin.x;
	d2 = arcMath::Fabs( extents[0] * ( dFar * testAxis[0][1] - dLeft * testAxis[0][0] ) ) +
				arcMath::Fabs( extents[1] * ( dFar * testAxis[1][1] - dLeft * testAxis[1][0] ) ) +
					arcMath::Fabs( extents[2] * ( dFar * testAxis[2][1] - dLeft * testAxis[2][0] ) );
	if ( d1 - d2 > 0.0f ) {
		return true;
	}

	if ( testOrigin.z < 0.0f ) {
		testOrigin.z = -testOrigin.z;
		testAxis[0][2] = -testAxis[0][2];
		testAxis[1][2] = -testAxis[1][2];
		testAxis[2][2] = -testAxis[2][2];
	}

	// test up/down planes
	d1 = dFar * testOrigin.z - dUp * testOrigin.x;
	d2 = arcMath::Fabs( extents[0] * ( dFar * testAxis[0][2] - dUp * testAxis[0][0] ) ) +
				arcMath::Fabs( extents[1] * ( dFar * testAxis[1][2] - dUp * testAxis[1][0] ) ) +
					arcMath::Fabs( extents[2] * ( dFar * testAxis[2][2] - dUp * testAxis[2][0] ) );
	if ( d1 - d2 > 0.0f ) {
		return true;
	}

	return false;
}

/*
============
ARCFrustum::CullBounds

  Tests if any of the planes of the frustum can be used as a separating plane.

  24 muls best case
  37 muls worst case
============
*/
bool ARCFrustum::CullBounds( const arcBounds &bounds ) const {
	arcVec3 localOrigin, center, extents;
	arcMat3 localAxis;

	center = ( bounds[0] + bounds[1] ) * 0.5f;
	extents = bounds[1] - center;

	// transform the bounds into the space of this frustum
	localOrigin = ( center - origin ) * axis.Transpose();
	localAxis = axis.Transpose();

	return CullLocalBox( localOrigin, extents, localAxis );
}

/*
============
ARCFrustum::CullBounds

  Tests if any of the planes of the frustum can be used as a separating plane.

  39 muls best case
  61 muls worst case
============
*/
bool ARCFrustum::CullBox( const ARCBox &box ) const {
	arcVec3 localOrigin;
	arcMat3 localAxis;

	// transform the box into the space of this frustum
	localOrigin = ( box.GetCenter() - origin ) * axis.Transpose();
	localAxis = box.GetAxis() * axis.Transpose();

	return CullLocalBox( localOrigin, box.GetExtents(), localAxis );
}

/*
============
ARCFrustum::CullSphere

  Tests if any of the planes of the frustum can be used as a separating plane.

   9 muls best case
  21 muls worst case
============
*/
bool ARCFrustum::CullSphere( const ARCSphere &sphere ) const {
	float d, r, rs, sFar;
	arcVec3 center;

	center = ( sphere.GetOrigin() - origin ) * axis.Transpose();
	r = sphere.GetRadius();

	// test near plane
	if ( dNear - center.x > r ) {
		return true;
	}

	// test far plane
	if ( center.x - dFar > r ) {
		return true;
	}

	rs = r * r;
	sFar = dFar * dFar;

	// test left/right planes
	d = dFar * arcMath::Fabs( center.y ) - dLeft * center.x;
	if ( ( d * d ) > rs * ( sFar + dLeft * dLeft ) ) {
		return true;
	}

	// test up/down planes
	d = dFar * arcMath::Fabs( center.z ) - dUp * center.x;
	if ( ( d * d ) > rs * ( sFar + dUp * dUp ) ) {
		return true;
	}

	return false;
}

/*
============
ARCFrustum::CullLocalFrustum

  Tests if any of the planes of this frustum can be used as a separating plane.

   0 muls best case
  30 muls worst case
============
*/
bool ARCFrustum::CullLocalFrustum( const ARCFrustum &localFrustum, const arcVec3 indexPoints[8], const arcVec3 cornerVecs[4] ) const {
	int index;
	float dx, dy, dz, leftScale, upScale;

	// test near plane
	dy = -localFrustum.axis[1].x;
	dz = -localFrustum.axis[2].x;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = -cornerVecs[index].x;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].x < dNear ) {
		return true;
	}

	// test far plane
	dy = localFrustum.axis[1].x;
	dz = localFrustum.axis[2].x;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = cornerVecs[index].x;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].x > dFar ) {
		return true;
	}

	leftScale = dLeft * invFar;

	// test left plane
	dy = dFar * localFrustum.axis[1].y - dLeft * localFrustum.axis[1].x;
	dz = dFar * localFrustum.axis[2].y - dLeft * localFrustum.axis[2].x;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = dFar * cornerVecs[index].y - dLeft * cornerVecs[index].x;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].y > indexPoints[index].x * leftScale ) {
		return true;
	}

	// test right plane
	dy = -dFar * localFrustum.axis[1].y - dLeft * localFrustum.axis[1].x;
	dz = -dFar * localFrustum.axis[2].y - dLeft * localFrustum.axis[2].x;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = -dFar * cornerVecs[index].y - dLeft * cornerVecs[index].x;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].y < -indexPoints[index].x * leftScale ) {
		return true;
	}

	upScale = dUp * invFar;

	// test up plane
	dy = dFar * localFrustum.axis[1].z - dUp * localFrustum.axis[1].x;
	dz = dFar * localFrustum.axis[2].z - dUp * localFrustum.axis[2].x;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = dFar * cornerVecs[index].z - dUp * cornerVecs[index].x;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].z > indexPoints[index].x * upScale ) {
		return true;
	}

	// test down plane
	dy = -dFar * localFrustum.axis[1].z - dUp * localFrustum.axis[1].x;
	dz = -dFar * localFrustum.axis[2].z - dUp * localFrustum.axis[2].x;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = -dFar * cornerVecs[index].z - dUp * cornerVecs[index].x;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].z < -indexPoints[index].x * upScale ) {
		return true;
	}

	return false;
}

/*
============
ARCFrustum::CullFrustum

  Tests if any of the planes of this frustum can be used as a separating plane.

  58 muls best case
  88 muls worst case
============
*/
bool ARCFrustum::CullFrustum( const ARCFrustum &frustum ) const {
	ARCFrustum localFrustum;
	arcVec3 indexPoints[8], cornerVecs[4];

	// transform the given frustum into the space of this frustum
	localFrustum = frustum;
	localFrustum.origin = ( frustum.origin - origin ) * axis.Transpose();
	localFrustum.axis = frustum.axis * axis.Transpose();

	localFrustum.ToIndexPointsAndCornerVecs( indexPoints, cornerVecs );

	return CullLocalFrustum( localFrustum, indexPoints, cornerVecs );
}

/*
============
ARCFrustum::CullLocalWinding
============
*/
bool ARCFrustum::CullLocalWinding( const arcVec3 *points, const int numPoints, int *pointCull ) const {
	int i, pCull, culled;
	float leftScale, upScale;

	leftScale = dLeft * invFar;
	upScale = dUp * invFar;

	culled = -1;
	for ( i = 0; i < numPoints; i++ ) {
		const arcVec3 &p = points[i];
		pCull = 0;
		if ( p.x < dNear ) {
			pCull = 1;
		}
		else if ( p.x > dFar ) {
			pCull = 2;
		}
		if ( arcMath::Fabs( p.y ) > p.x * leftScale ) {
			pCull |= 4 << FLOATSIGNBITSET( p.y );
		}
		if ( arcMath::Fabs( p.z ) > p.x * upScale ) {
			pCull |= 16 << FLOATSIGNBITSET( p.z );
		}
		culled &= pCull;
		pointCull[i] = pCull;
	}

	return ( culled != 0 );
}

/*
============
ARCFrustum::CullWinding
============
*/
bool ARCFrustum::CullWinding( const arcWinding &winding ) const {
	int i, *pointCull;
	arcVec3 *localPoints;
	arcMat3 transpose;

	localPoints = (arcVec3 *) _alloca16( winding.GetNumPoints() * sizeof( arcVec3 ) );
	pointCull = ( int * ) _alloca16( winding.GetNumPoints() * sizeof( int ) );

	transpose = axis.Transpose();
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		localPoints[i] = ( winding[i].ToVec3() - origin ) * transpose;
	}

	return CullLocalWinding( localPoints, winding.GetNumPoints(), pointCull );
}

/*
============
ARCFrustum::BoundsCullLocalFrustum

  Tests if any of the bounding box planes can be used as a separating plane.
============
*/
bool ARCFrustum::BoundsCullLocalFrustum( const arcBounds &bounds, const ARCFrustum &localFrustum, const arcVec3 indexPoints[8], const arcVec3 cornerVecs[4] ) const {
	int index;
	float dx, dy, dz;

	dy = -localFrustum.axis[1].x;
	dz = -localFrustum.axis[2].x;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = -cornerVecs[index].x;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].x < bounds[0].x ) {
		return true;
	}

	dy = localFrustum.axis[1].x;
	dz = localFrustum.axis[2].x;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = cornerVecs[index].x;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].x > bounds[1].x ) {
		return true;
	}

	dy = -localFrustum.axis[1].y;
	dz = -localFrustum.axis[2].y;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = -cornerVecs[index].y;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].y < bounds[0].y ) {
		return true;
	}

	dy = localFrustum.axis[1].y;
	dz = localFrustum.axis[2].y;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = cornerVecs[index].y;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].y > bounds[1].y ) {
		return true;
	}

	dy = -localFrustum.axis[1].z;
	dz = -localFrustum.axis[2].z;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = -cornerVecs[index].z;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].z < bounds[0].z ) {
		return true;
	}

	dy = localFrustum.axis[1].z;
	dz = localFrustum.axis[2].z;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = cornerVecs[index].z;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );

	if ( indexPoints[index].z > bounds[1].z ) {
		return true;
	}

	return false;
}

/*
============
ARCFrustum::LocalLineIntersection

   7 divs
  30 muls
============
*/
bool ARCFrustum::LocalLineIntersection( const arcVec3 &start, const arcVec3 &end ) const {
	arcVec3 dir;
	float d1, d2, fstart, fend, lstart, lend, f, x;
	float leftScale, upScale;
	int startInside = 1;

	leftScale = dLeft * invFar;
	upScale = dUp * invFar;
	dir = end - start;

	// test near plane
	if ( dNear > 0.0f ) {
		d1 = dNear - start.x;
		startInside &= FLOATSIGNBITSET( d1 );
		if ( FLOATNOTZERO( d1 ) ) {
			d2 = dNear - end.x;
			if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
				f = d1 / ( d1 - d2 );
				if ( arcMath::Fabs( start.y + f * dir.y ) <= dNear * leftScale ) {
					if ( arcMath::Fabs( start.z + f * dir.z ) <= dNear * upScale ) {
						return true;
					}
				}
			}
		}
	}

	// test far plane
	d1 = start.x - dFar;
	startInside &= FLOATSIGNBITSET( d1 );
	if ( FLOATNOTZERO( d1 ) ) {
		d2 = end.x - dFar;
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			if ( arcMath::Fabs( start.y + f * dir.y ) <= dFar * leftScale ) {
				if ( arcMath::Fabs( start.z + f * dir.z ) <= dFar * upScale ) {
					return true;
				}
			}
		}
	}

	fstart = dFar * start.y;
	fend = dFar * end.y;
	lstart = dLeft * start.x;
	lend = dLeft * end.x;

	// test left plane
	d1 = fstart - lstart;
	startInside &= FLOATSIGNBITSET( d1 );
	if ( FLOATNOTZERO( d1 ) ) {
		d2 = fend - lend;
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			x = start.x + f * dir.x;
			if ( x >= dNear && x <= dFar ) {
				if ( arcMath::Fabs( start.z + f * dir.z ) <= x * upScale ) {
					return true;
				}
			}
		}
	}

	// test right plane
	d1 = -fstart - lstart;
	startInside &= FLOATSIGNBITSET( d1 );
	if ( FLOATNOTZERO( d1 ) ) {
		d2 = -fend - lend;
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			x = start.x + f * dir.x;
			if ( x >= dNear && x <= dFar ) {
				if ( arcMath::Fabs( start.z + f * dir.z ) <= x * upScale ) {
					return true;
				}
			}
		}
	}

	fstart = dFar * start.z;
	fend = dFar * end.z;
	lstart = dUp * start.x;
	lend = dUp * end.x;

	// test up plane
	d1 = fstart - lstart;
	startInside &= FLOATSIGNBITSET( d1 );
	if ( FLOATNOTZERO( d1 ) ) {
		d2 = fend - lend;
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			x = start.x + f * dir.x;
			if ( x >= dNear && x <= dFar ) {
				if ( arcMath::Fabs( start.y + f * dir.y ) <= x * leftScale ) {
					return true;
				}
			}
		}
	}

	// test down plane
	d1 = -fstart - lstart;
	startInside &= FLOATSIGNBITSET( d1 );
	if ( FLOATNOTZERO( d1 ) ) {
		d2 = -fend - lend;
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			x = start.x + f * dir.x;
			if ( x >= dNear && x <= dFar ) {
				if ( arcMath::Fabs( start.y + f * dir.y ) <= x * leftScale ) {
					return true;
				}
			}
		}
	}

	return ( startInside != 0 );
}

/*
============
ARCFrustum::LocalRayIntersection

  Returns true if the ray starts inside the frustum.
  If there was an intersection scale1 <= scale2
============
*/
bool ARCFrustum::LocalRayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale1, float &scale2 ) const {
	arcVec3 end;
	float d1, d2, fstart, fend, lstart, lend, f, x;
	float leftScale, upScale;
	int startInside = 1;

	leftScale = dLeft * invFar;
	upScale = dUp * invFar;
	end = start + dir;

	scale1 = arcMath::INFINITY;
	scale2 = -arcMath::INFINITY;

	// test near plane
	if ( dNear > 0.0f ) {
		d1 = dNear - start.x;
		startInside &= FLOATSIGNBITSET( d1 );
		d2 = dNear - end.x;
		if ( d1 != d2 ) {
			f = d1 / ( d1 - d2 );
			if ( arcMath::Fabs( start.y + f * dir.y ) <= dNear * leftScale ) {
				if ( arcMath::Fabs( start.z + f * dir.z ) <= dNear * upScale ) {
					if ( f < scale1 ) scale1 = f;
					if ( f > scale2 ) scale2 = f;
				}
			}
		}
	}

	// test far plane
	d1 = start.x - dFar;
	startInside &= FLOATSIGNBITSET( d1 );
	d2 = end.x - dFar;
	if ( d1 != d2 ) {
		f = d1 / ( d1 - d2 );
		if ( arcMath::Fabs( start.y + f * dir.y ) <= dFar * leftScale ) {
			if ( arcMath::Fabs( start.z + f * dir.z ) <= dFar * upScale ) {
				if ( f < scale1 ) scale1 = f;
				if ( f > scale2 ) scale2 = f;
			}
		}
	}

	fstart = dFar * start.y;
	fend = dFar * end.y;
	lstart = dLeft * start.x;
	lend = dLeft * end.x;

	// test left plane
	d1 = fstart - lstart;
	startInside &= FLOATSIGNBITSET( d1 );
	d2 = fend - lend;
	if ( d1 != d2 ) {
		f = d1 / ( d1 - d2 );
		x = start.x + f * dir.x;
		if ( x >= dNear && x <= dFar ) {
			if ( arcMath::Fabs( start.z + f * dir.z ) <= x * upScale ) {
				if ( f < scale1 ) scale1 = f;
				if ( f > scale2 ) scale2 = f;
			}
		}
	}

	// test right plane
	d1 = -fstart - lstart;
	startInside &= FLOATSIGNBITSET( d1 );
	d2 = -fend - lend;
	if ( d1 != d2 ) {
		f = d1 / ( d1 - d2 );
		x = start.x + f * dir.x;
		if ( x >= dNear && x <= dFar ) {
			if ( arcMath::Fabs( start.z + f * dir.z ) <= x * upScale ) {
				if ( f < scale1 ) scale1 = f;
				if ( f > scale2 ) scale2 = f;
			}
		}
	}

	fstart = dFar * start.z;
	fend = dFar * end.z;
	lstart = dUp * start.x;
	lend = dUp * end.x;

	// test up plane
	d1 = fstart - lstart;
	startInside &= FLOATSIGNBITSET( d1 );
	d2 = fend - lend;
	if ( d1 != d2 ) {
		f = d1 / ( d1 - d2 );
		x = start.x + f * dir.x;
		if ( x >= dNear && x <= dFar ) {
			if ( arcMath::Fabs( start.y + f * dir.y ) <= x * leftScale ) {
				if ( f < scale1 ) scale1 = f;
				if ( f > scale2 ) scale2 = f;
			}
		}
	}

	// test down plane
	d1 = -fstart - lstart;
	startInside &= FLOATSIGNBITSET( d1 );
	d2 = -fend - lend;
	if ( d1 != d2 ) {
		f = d1 / ( d1 - d2 );
		x = start.x + f * dir.x;
		if ( x >= dNear && x <= dFar ) {
			if ( arcMath::Fabs( start.y + f * dir.y ) <= x * leftScale ) {
				if ( f < scale1 ) scale1 = f;
				if ( f > scale2 ) scale2 = f;
			}
		}
	}

	return ( startInside != 0 );
}

/*
============
ARCFrustum::ContainsPoint
============
*/
bool ARCFrustum::ContainsPoint( const arcVec3 &point ) const {
	return !CullPoint( point );
}

/*
============
ARCFrustum::LocalFrustumIntersectsFrustum
============
*/
bool ARCFrustum::LocalFrustumIntersectsFrustum( const arcVec3 points[8], const bool testFirstSide ) const {
	int i;

	// test if any edges of the other frustum intersect this frustum
	for ( i = 0; i < 4; i++ ) {
		if ( LocalLineIntersection( points[i], points[4+i] ) ) {
			return true;
		}
	}
	if ( testFirstSide ) {
		for ( i = 0; i < 4; i++ ) {
			if ( LocalLineIntersection( points[i], points[( i+1 )&3] ) ) {
				return true;
			}
		}
	}
	for ( i = 0; i < 4; i++ ) {
		if ( LocalLineIntersection( points[4+i], points[4+(( i+1 )&3)] ) ) {
			return true;
		}
	}

	return false;
}

/*
============
ARCFrustum::LocalFrustumIntersectsBounds
============
*/
bool ARCFrustum::LocalFrustumIntersectsBounds( const arcVec3 points[8], const arcBounds &bounds ) const {
	int i;

	// test if any edges of the other frustum intersect this frustum
	for ( i = 0; i < 4; i++ ) {
		if ( bounds.LineIntersection( points[i], points[4+i] ) ) {
			return true;
		}
	}
	if ( dNear > 0.0f ) {
		for ( i = 0; i < 4; i++ ) {
			if ( bounds.LineIntersection( points[i], points[( i+1 )&3] ) ) {
				return true;
			}
		}
	}
	for ( i = 0; i < 4; i++ ) {
		if ( bounds.LineIntersection( points[4+i], points[4+(( i+1 )&3)] ) ) {
			return true;
		}
	}

	return false;
}

/*
============
ARCFrustum::IntersectsBounds
============
*/
bool ARCFrustum::IntersectsBounds( const arcBounds &bounds ) const {
	arcVec3 localOrigin, center, extents;
	arcMat3 localAxis;

	center = ( bounds[0] + bounds[1] ) * 0.5f;
	extents = bounds[1] - center;

	localOrigin = ( center - origin ) * axis.Transpose();
	localAxis = axis.Transpose();

	if ( CullLocalBox( localOrigin, extents, localAxis ) ) {
		return false;
	}

	arcVec3 indexPoints[8], cornerVecs[4];

	ToIndexPointsAndCornerVecs( indexPoints, cornerVecs );

	if ( BoundsCullLocalFrustum( bounds, *this, indexPoints, cornerVecs ) ) {
		return false;
	}

	idSwap( indexPoints[2], indexPoints[3] );
	idSwap( indexPoints[6], indexPoints[7] );

	if ( LocalFrustumIntersectsBounds( indexPoints, bounds ) ) {
		return true;
	}

	BoxToPoints( localOrigin, extents, localAxis, indexPoints );

	if ( LocalFrustumIntersectsFrustum( indexPoints, true ) ) {
		return true;
	}

	return false;
}

/*
============
ARCFrustum::IntersectsBox
============
*/
bool ARCFrustum::IntersectsBox( const ARCBox &box ) const {
	arcVec3 localOrigin;
	arcMat3 localAxis;

	localOrigin = ( box.GetCenter() - origin ) * axis.Transpose();
	localAxis = box.GetAxis() * axis.Transpose();

	if ( CullLocalBox( localOrigin, box.GetExtents(), localAxis ) ) {
		return false;
	}

	arcVec3 indexPoints[8], cornerVecs[4];
	ARCFrustum localFrustum;

	localFrustum = *this;
	localFrustum.origin = ( origin - box.GetCenter() ) * box.GetAxis().Transpose();
	localFrustum.axis = axis * box.GetAxis().Transpose();
	localFrustum.ToIndexPointsAndCornerVecs( indexPoints, cornerVecs );

	if ( BoundsCullLocalFrustum( arcBounds( -box.GetExtents(), box.GetExtents() ), localFrustum, indexPoints, cornerVecs ) ) {
		return false;
	}

	idSwap( indexPoints[2], indexPoints[3] );
	idSwap( indexPoints[6], indexPoints[7] );

	if ( LocalFrustumIntersectsBounds( indexPoints, arcBounds( -box.GetExtents(), box.GetExtents() ) ) ) {
		return true;
	}

	BoxToPoints( localOrigin, box.GetExtents(), localAxis, indexPoints );

	if ( LocalFrustumIntersectsFrustum( indexPoints, true ) ) {
		return true;
	}

	return false;
}

/*
============
ARCFrustum::IntersectsSphere

  FIXME: test this
============
*/
#define VORONOI_INDEX( x, y, z )	( x + y * 3 + z * 9 )

bool ARCFrustum::IntersectsSphere( const ARCSphere &sphere ) const {
	int index, x, y, z;
	float scale, r, d;
	arcVec3 p, dir, points[8];

	if ( CullSphere( sphere ) ) {
		return false;
	}

	x = y = z = 0;
	dir.Zero();

	p = ( sphere.GetOrigin() - origin ) * axis.Transpose();

	if ( p.x <= dNear ) {
		scale = dNear * invFar;
		dir.y = arcMath::Fabs( p.y ) - dLeft * scale;
		dir.z = arcMath::Fabs( p.z ) - dUp * scale;
	}
	else if ( p.x >= dFar ) {
		dir.y = arcMath::Fabs( p.y ) - dLeft;
		dir.z = arcMath::Fabs( p.z ) - dUp;
	}
	else {
		scale = p.x * invFar;
		dir.y = arcMath::Fabs( p.y ) - dLeft * scale;
		dir.z = arcMath::Fabs( p.z ) - dUp * scale;
	}
	if ( dir.y > 0.0f ) {
		y = ( 1 + FLOATSIGNBITNOTSET( p.y ) );
	}
	if ( dir.z > 0.0f ) {
		z = ( 1 + FLOATSIGNBITNOTSET( p.z ) );
	}
	if ( p.x < dNear ) {
		scale = dLeft * dNear * invFar;
		if ( p.x < dNear + ( scale - p.y ) * scale * invFar ) {
			scale = dUp * dNear * invFar;
			if ( p.x < dNear + ( scale - p.z ) * scale * invFar ) {
				x = 1;
			}
		}
	}
	else {
		if ( p.x > dFar ) {
			x = 2;
		}
		else if ( p.x > dFar + ( dLeft - p.y ) * dLeft * invFar ) {
			x = 2;
		}
		else if ( p.x > dFar + ( dUp - p.z ) * dUp * invFar ) {
			x = 2;
		}
	}

	r = sphere.GetRadius();
	index = VORONOI_INDEX( x, y, z );
	switch( index ) {
		case VORONOI_INDEX( 0, 0, 0 ): return true;
		case VORONOI_INDEX( 1, 0, 0 ): return ( dNear - p.x < r );
		case VORONOI_INDEX( 2, 0, 0 ): return ( p.x - dFar < r );
		case VORONOI_INDEX( 0, 1, 0 ): d = dFar * p.y - dLeft * p.x; return ( d * d < r * r * ( dFar * dFar + dLeft * dLeft ) );
		case VORONOI_INDEX( 0, 2, 0 ): d = -dFar * p.z - dLeft * p.x; return ( d * d < r * r * ( dFar * dFar + dLeft * dLeft ) );
		case VORONOI_INDEX( 0, 0, 1 ): d = dFar * p.z - dUp * p.x; return ( d * d < r * r * ( dFar * dFar + dUp * dUp ) );
		case VORONOI_INDEX( 0, 0, 2 ): d = -dFar * p.z - dUp * p.x; return ( d * d < r * r * ( dFar * dFar + dUp * dUp ) );
		default: {
			ToIndexPoints( points );
			switch( index ) {
				case VORONOI_INDEX( 1, 1, 1 ): return sphere.ContainsPoint( points[0] );
				case VORONOI_INDEX( 2, 1, 1 ): return sphere.ContainsPoint( points[4] );
				case VORONOI_INDEX( 1, 2, 1 ): return sphere.ContainsPoint( points[1] );
				case VORONOI_INDEX( 2, 2, 1 ): return sphere.ContainsPoint( points[5] );
				case VORONOI_INDEX( 1, 1, 2 ): return sphere.ContainsPoint( points[2] );
				case VORONOI_INDEX( 2, 1, 2 ): return sphere.ContainsPoint( points[6] );
				case VORONOI_INDEX( 1, 2, 2 ): return sphere.ContainsPoint( points[3] );
				case VORONOI_INDEX( 2, 2, 2 ): return sphere.ContainsPoint( points[7] );
				case VORONOI_INDEX( 1, 1, 0 ): return sphere.LineIntersection( points[0], points[2] );
				case VORONOI_INDEX( 2, 1, 0 ): return sphere.LineIntersection( points[4], points[6] );
				case VORONOI_INDEX( 1, 2, 0 ): return sphere.LineIntersection( points[1], points[3] );
				case VORONOI_INDEX( 2, 2, 0 ): return sphere.LineIntersection( points[5], points[7] );
				case VORONOI_INDEX( 1, 0, 1 ): return sphere.LineIntersection( points[0], points[1] );
				case VORONOI_INDEX( 2, 0, 1 ): return sphere.LineIntersection( points[4], points[5] );
				case VORONOI_INDEX( 0, 1, 1 ): return sphere.LineIntersection( points[0], points[4] );
				case VORONOI_INDEX( 0, 2, 1 ): return sphere.LineIntersection( points[1], points[5] );
				case VORONOI_INDEX( 1, 0, 2 ): return sphere.LineIntersection( points[2], points[3] );
				case VORONOI_INDEX( 2, 0, 2 ): return sphere.LineIntersection( points[6], points[7] );
				case VORONOI_INDEX( 0, 1, 2 ): return sphere.LineIntersection( points[2], points[6] );
				case VORONOI_INDEX( 0, 2, 2 ): return sphere.LineIntersection( points[3], points[7] );
			}
			break;
		}
	}
	return false;
}

/*
============
ARCFrustum::IntersectsFrustum
============
*/
bool ARCFrustum::IntersectsFrustum( const ARCFrustum &frustum ) const {
	arcVec3 indexPoints2[8], cornerVecs2[4];
	ARCFrustum localFrustum2;

	localFrustum2 = frustum;
	localFrustum2.origin = ( frustum.origin - origin ) * axis.Transpose();
	localFrustum2.axis = frustum.axis * axis.Transpose();
	localFrustum2.ToIndexPointsAndCornerVecs( indexPoints2, cornerVecs2 );

	if ( CullLocalFrustum( localFrustum2, indexPoints2, cornerVecs2 ) ) {
		return false;
	}

	arcVec3 indexPoints1[8], cornerVecs1[4];
	ARCFrustum localFrustum1;

	localFrustum1 = *this;
	localFrustum1.origin = ( origin - frustum.origin ) * frustum.axis.Transpose();
	localFrustum1.axis = axis * frustum.axis.Transpose();
	localFrustum1.ToIndexPointsAndCornerVecs( indexPoints1, cornerVecs1 );

	if ( frustum.CullLocalFrustum( localFrustum1, indexPoints1, cornerVecs1 ) ) {
		return false;
	}

	idSwap( indexPoints2[2], indexPoints2[3] );
	idSwap( indexPoints2[6], indexPoints2[7] );

	if ( LocalFrustumIntersectsFrustum( indexPoints2, ( localFrustum2.dNear > 0.0f ) ) ) {
		return true;
	}

	idSwap( indexPoints1[2], indexPoints1[3] );
	idSwap( indexPoints1[6], indexPoints1[7] );

	if ( frustum.LocalFrustumIntersectsFrustum( indexPoints1, ( localFrustum1.dNear > 0.0f ) ) ) {
		return true;
	}

	return false;
}

/*
============
ARCFrustum::IntersectsWinding
============
*/
bool ARCFrustum::IntersectsWinding( const arcWinding &winding ) const {
	int i, j, *pointCull;
	float min, max;
	arcVec3 *localPoints, indexPoints[8], cornerVecs[4];
	arcMat3 transpose;
	arcPlane plane;

	localPoints = (arcVec3 *) _alloca16( winding.GetNumPoints() * sizeof( arcVec3 ) );
	pointCull = ( int * ) _alloca16( winding.GetNumPoints() * sizeof( int ) );

	transpose = axis.Transpose();
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		localPoints[i] = ( winding[i].ToVec3() - origin ) * transpose;
	}

	// if the winding is culled
	if ( CullLocalWinding( localPoints, winding.GetNumPoints(), pointCull ) ) {
		return false;
	}

	winding.GetPlane( plane );

	ToIndexPointsAndCornerVecs( indexPoints, cornerVecs );
	AxisProjection( indexPoints, cornerVecs, plane.Normal(), min, max );

	// if the frustum does not cross the winding plane
	if ( min + plane[3] > 0.0f || max + plane[3] < 0.0f ) {
		return false;
	}

	// test if any of the winding edges goes through the frustum
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		j = ( i+1 )%winding.GetNumPoints();
		if ( !( pointCull[i] & pointCull[j] ) ) {
			if ( LocalLineIntersection( localPoints[i], localPoints[j] ) ) {
				return true;
			}
		}
	}

	idSwap( indexPoints[2], indexPoints[3] );
	idSwap( indexPoints[6], indexPoints[7] );

	// test if any edges of the frustum intersect the winding
	for ( i = 0; i < 4; i++ ) {
		if ( winding.LineIntersection( plane, indexPoints[i], indexPoints[4+i] ) ) {
			return true;
		}
	}
	if ( dNear > 0.0f ) {
		for ( i = 0; i < 4; i++ ) {
			if ( winding.LineIntersection( plane, indexPoints[i], indexPoints[( i+1 )&3] ) ) {
				return true;
			}
		}
	}
	for ( i = 0; i < 4; i++ ) {
		if ( winding.LineIntersection( plane, indexPoints[4+i], indexPoints[4+(( i+1 )&3)] ) ) {
			return true;
		}
	}

	return false;
}

/*
============
ARCFrustum::LineIntersection

  Returns true if the line intersects the box between the start and end point.
============
*/
bool ARCFrustum::LineIntersection( const arcVec3 &start, const arcVec3 &end ) const {
	return LocalLineIntersection( ( start - origin ) * axis.Transpose(), ( end - origin ) * axis.Transpose() );
}

/*
============
ARCFrustum::RayIntersection

  Returns true if the ray intersects the bounds.
  The ray can intersect the bounds in both directions from the start point.
  If start is inside the frustum then scale1 < 0 and scale2 > 0.
============
*/
bool ARCFrustum::RayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale1, float &scale2 ) const {
	if ( LocalRayIntersection( ( start - origin ) * axis.Transpose(), dir * axis.Transpose(), scale1, scale2 ) ) {
		return true;
	}
	if ( scale1 <= scale2 ) {
		return true;
	}
	return false;
}

/*
============
ARCFrustum::FromProjection

  Creates a frustum which contains the projection of the bounds.
============
*/
bool ARCFrustum::FromProjection( const arcBounds &bounds, const arcVec3 &projectionOrigin, const float dFar ) {
	return FromProjection( ARCBox( bounds, vec3_origin, mat3_identity ), projectionOrigin, dFar );
}

/*
============
ARCFrustum::FromProjection

  Creates a frustum which contains the projection of the box.
============
*/
bool ARCFrustum::FromProjection( const ARCBox &box, const arcVec3 &projectionOrigin, const float dFar ) {
	int i, bestAxis;
	float value, bestValue;
	arcVec3 dir;

	assert( dFar > 0.0f );

	this->dNear = this->dFar = this->invFar = 0.0f;

	dir = box.GetCenter() - projectionOrigin;
	if ( dir.Normalize() == 0.0f ) {
		return false;
	}

	bestAxis = 0;
	bestValue = arcMath::Fabs( box.GetAxis()[0] * dir );
	for ( i = 1; i < 3; i++ ) {
		value = arcMath::Fabs( box.GetAxis()[i] * dir );
		if ( value * box.GetExtents()[bestAxis] * box.GetExtents()[bestAxis] < bestValue * box.GetExtents()[i] * box.GetExtents()[i] ) {
			bestValue = value;
			bestAxis = i;
		}
	}

#if 1

	int j, minX, minY, maxY, minZ, maxZ;
	arcVec3 points[8];

	minX = minY = maxY = minZ = maxZ = 0;

	for ( j = 0; j < 2; j++ ) {

		axis[0] = dir;
		axis[1] = box.GetAxis()[bestAxis] - ( box.GetAxis()[bestAxis] * axis[0] ) * axis[0];
		axis[1].Normalize();
		axis[2].Cross( axis[0], axis[1] );

		BoxToPoints( ( box.GetCenter() - projectionOrigin ) * axis.Transpose(), box.GetExtents(), box.GetAxis() * axis.Transpose(), points );

		if ( points[0].x <= 1.0f ) {
			return false;
		}

		minX = minY = maxY = minZ = maxZ = 0;
		for ( i = 1; i < 8; i++ ) {
			if ( points[i].x <= 1.0f ) {
				return false;
			}
			if ( points[i].x < points[minX].x ) {
				minX = i;
			}
			if ( points[minY].x * points[i].y < points[i].x * points[minY].y ) {
				minY = i;
			} else if ( points[maxY].x * points[i].y > points[i].x * points[maxY].y ) {
				maxY = i;
			}
			if ( points[minZ].x * points[i].z < points[i].x * points[minZ].z ) {
				minZ = i;
			} else if ( points[maxZ].x * points[i].z > points[i].x * points[maxZ].z ) {
				maxZ = i;
			}
		}

		if ( j == 0 ) {
			dir += arcMath::Tan16( 0.5f * ( arcMath::ATan16( points[minY].y, points[minY].x ) + arcMath::ATan16( points[maxY].y, points[maxY].x ) ) ) * axis[1];
			dir += arcMath::Tan16( 0.5f * ( arcMath::ATan16( points[minZ].z, points[minZ].x ) + arcMath::ATan16( points[maxZ].z, points[maxZ].x ) ) ) * axis[2];
			dir.Normalize();
		}
	}

	this->origin = projectionOrigin;
	this->dNear = points[minX].x;
	this->dFar = dFar;
	this->dLeft = Max( arcMath::Fabs( points[minY].y / points[minY].x ), arcMath::Fabs( points[maxY].y / points[maxY].x ) ) * dFar;
	this->dUp = Max( arcMath::Fabs( points[minZ].z / points[minZ].x ), arcMath::Fabs( points[maxZ].z / points[maxZ].x ) ) * dFar;
	this->invFar = 1.0f / dFar;

#elif 1

	int j;
	float f, x;
	arcBounds b;
	arcVec3 points[8];

	for ( j = 0; j < 2; j++ ) {

		axis[0] = dir;
		axis[1] = box.GetAxis()[bestAxis] - ( box.GetAxis()[bestAxis] * axis[0] ) * axis[0];
		axis[1].Normalize();
		axis[2].Cross( axis[0], axis[1] );

		BoxToPoints( ( box.GetCenter() - projectionOrigin ) * axis.Transpose(), box.GetExtents(), box.GetAxis() * axis.Transpose(), points );

		b.Clear();
		for ( i = 0; i < 8; i++ ) {
			x = points[i].x;
			if ( x <= 1.0f ) {
				return false;
			}
			f = 1.0f / x;
			points[i].y *= f;
			points[i].z *= f;
			b.AddPoint( points[i] );
		}

		if ( j == 0 ) {
			dir += arcMath::Tan16( 0.5f * ( arcMath::ATan16( b[1][1] ) + arcMath::ATan16( b[0][1] ) ) ) * axis[1];
			dir += arcMath::Tan16( 0.5f * ( arcMath::ATan16( b[1][2] ) + arcMath::ATan16( b[0][2] ) ) ) * axis[2];
			dir.Normalize();
		}
	}

	this->origin = projectionOrigin;
	this->dNear = b[0][0];
	this->dFar = dFar;
	this->dLeft = Max( arcMath::Fabs( b[0][1] ), arcMath::Fabs( b[1][1] ) ) * dFar;
	this->dUp = Max( arcMath::Fabs( b[0][2] ), arcMath::Fabs( b[1][2] ) ) * dFar;
	this->invFar = 1.0f / dFar;

#else

	float dist;
	arcVec3 org;

	axis[0] = dir;
	axis[1] = box.GetAxis()[bestAxis] - ( box.GetAxis()[bestAxis] * axis[0] ) * axis[0];
	axis[1].Normalize();
	axis[2].Cross( axis[0], axis[1] );

	for ( i = 0; i < 3; i++ ) {
		dist[i] = arcMath::Fabs( box.GetExtents()[0] * ( axis[i] * box.GetAxis()[0] ) ) +
					arcMath::Fabs( box.GetExtents()[1] * ( axis[i] * box.GetAxis()[1] ) ) +
						arcMath::Fabs( box.GetExtents()[2] * ( axis[i] * box.GetAxis()[2] ) );
	}

	dist[0] = axis[0] * ( box.GetCenter() - projectionOrigin ) - dist[0];
	if ( dist[0] <= 1.0f ) {
		return false;
	}
	float invDist = 1.0f / dist[0];

	this->origin = projectionOrigin;
	this->dNear = dist[0];
	this->dFar = dFar;
	this->dLeft = dist[1] * invDist * dFar;
	this->dUp = dist[2] * invDist * dFar;
	this->invFar = 1.0f / dFar;

#endif

	return true;
}

/*
============
ARCFrustum::FromProjection

  Creates a frustum which contains the projection of the sphere.
============
*/
bool ARCFrustum::FromProjection( const ARCSphere &sphere, const arcVec3 &projectionOrigin, const float dFar ) {
	arcVec3 dir;
	float d, r, s, x, y;

	assert( dFar > 0.0f );

	dir = sphere.GetOrigin() - projectionOrigin;
	d = dir.Normalize();
	r = sphere.GetRadius();

	if ( d <= r + 1.0f ) {
		this->dNear = this->dFar = this->invFar = 0.0f;
		return false;
	}

	origin = projectionOrigin;
	axis = dir.ToMat3();

	s = arcMath::Sqrt( d * d - r * r );
	x = r / d * s;
	y = arcMath::Sqrt( s * s - x * x );

	this->dNear = d - r;
	this->dFar = dFar;
	this->dLeft = x / y * dFar;
	this->dUp = dLeft;
	this->invFar = 1.0f / dFar;

	return true;
}

/*
============
ARCFrustum::ConstrainToBounds

  Returns false if no part of the bounds extends beyond the near plane.
============
*/
bool ARCFrustum::ConstrainToBounds( const arcBounds &bounds ) {
	float min, max, newdFar;

	bounds.AxisProjection( axis[0], min, max );
	newdFar = max - axis[0] * origin;
	if ( newdFar <= dNear ) {
		MoveFarDistance( dNear + 1.0f );
		return false;
	}
	MoveFarDistance( newdFar );
	return true;
}

/*
============
ARCFrustum::ConstrainToBox

  Returns false if no part of the box extends beyond the near plane.
============
*/
bool ARCFrustum::ConstrainToBox( const ARCBox &box ) {
	float min, max, newdFar;

	box.AxisProjection( axis[0], min, max );
	newdFar = max - axis[0] * origin;
	if ( newdFar <= dNear ) {
		MoveFarDistance( dNear + 1.0f );
		return false;
	}
	MoveFarDistance( newdFar );
	return true;
}

/*
============
ARCFrustum::ConstrainToSphere

  Returns false if no part of the sphere extends beyond the near plane.
============
*/
bool ARCFrustum::ConstrainToSphere( const ARCSphere &sphere ) {
	float min, max, newdFar;

	sphere.AxisProjection( axis[0], min, max );
	newdFar = max - axis[0] * origin;
	if ( newdFar <= dNear ) {
		MoveFarDistance( dNear + 1.0f );
		return false;
	}
	MoveFarDistance( newdFar );
	return true;
}

/*
============
ARCFrustum::ConstrainToFrustum

  Returns false if no part of the frustum extends beyond the near plane.
============
*/
bool ARCFrustum::ConstrainToFrustum( const ARCFrustum &frustum ) {
	float min, max, newdFar;

	frustum.AxisProjection( axis[0], min, max );
	newdFar = max - axis[0] * origin;
	if ( newdFar <= dNear ) {
		MoveFarDistance( dNear + 1.0f );
		return false;
	}
	MoveFarDistance( newdFar );
	return true;
}

/*
============
ARCFrustum::ToPlanes

  planes point outwards
============
*/
void ARCFrustum::ToPlanes( arcPlane planes[6] ) const {
	int i;
	arcVec3 scaled[2];
	arcVec3 points[4];

	planes[0].Normal() = -axis[0];
	planes[0].SetDist( -dNear );
	planes[1].Normal() = axis[0];
	planes[1].SetDist( dFar );

	scaled[0] = axis[1] * dLeft;
	scaled[1] = axis[2] * dUp;
	points[0] = scaled[0] + scaled[1];
	points[1] = -scaled[0] + scaled[1];
	points[2] = -scaled[0] - scaled[1];
	points[3] = scaled[0] - scaled[1];

	for ( i = 0; i < 4; i++ ) {
		planes[i+2].Normal() = points[i].Cross( points[( i+1 )&3] - points[i] );
		planes[i+2].Normalize();
		planes[i+2].FitThroughPoint( points[i] );
	}
}

/*
============
ARCFrustum::ToPoints
============
*/
void ARCFrustum::ToPoints( arcVec3 points[8] ) const {
	arcMat3 scaled;

	scaled[0] = origin + axis[0] * dNear;
	scaled[1] = axis[1] * ( dLeft * dNear * invFar );
	scaled[2] = axis[2] * ( dUp * dNear * invFar );

	points[0] = scaled[0] + scaled[1];
	points[1] = scaled[0] - scaled[1];
	points[2] = points[1] - scaled[2];
	points[3] = points[0] - scaled[2];
	points[0] += scaled[2];
	points[1] += scaled[2];

	scaled[0] = origin + axis[0] * dFar;
	scaled[1] = axis[1] * dLeft;
	scaled[2] = axis[2] * dUp;

	points[4] = scaled[0] + scaled[1];
	points[5] = scaled[0] - scaled[1];
	points[6] = points[5] - scaled[2];
	points[7] = points[4] - scaled[2];
	points[4] += scaled[2];
	points[5] += scaled[2];
}

/*
============
ARCFrustum::ToClippedPoints
============
*/
void ARCFrustum::ToClippedPoints( const float fractions[4], arcVec3 points[8] ) const {
	arcMat3 scaled;

	scaled[0] = origin + axis[0] * dNear;
	scaled[1] = axis[1] * ( dLeft * dNear * invFar );
	scaled[2] = axis[2] * ( dUp * dNear * invFar );

	points[0] = scaled[0] + scaled[1];
	points[1] = scaled[0] - scaled[1];
	points[2] = points[1] - scaled[2];
	points[3] = points[0] - scaled[2];
	points[0] += scaled[2];
	points[1] += scaled[2];

	scaled[0] = axis[0] * dFar;
	scaled[1] = axis[1] * dLeft;
	scaled[2] = axis[2] * dUp;

	points[4] = scaled[0] + scaled[1];
	points[5] = scaled[0] - scaled[1];
	points[6] = points[5] - scaled[2];
	points[7] = points[4] - scaled[2];
	points[4] += scaled[2];
	points[5] += scaled[2];

	points[4] = origin + fractions[0] * points[4];
	points[5] = origin + fractions[1] * points[5];
	points[6] = origin + fractions[2] * points[6];
	points[7] = origin + fractions[3] * points[7];
}

/*
============
ARCFrustum::ToIndexPoints
============
*/
void ARCFrustum::ToIndexPoints( arcVec3 indexPoints[8] ) const {
	arcMat3 scaled;

	scaled[0] = origin + axis[0] * dNear;
	scaled[1] = axis[1] * ( dLeft * dNear * invFar );
	scaled[2] = axis[2] * ( dUp * dNear * invFar );

	indexPoints[0] = scaled[0] - scaled[1];
	indexPoints[2] = scaled[0] + scaled[1];
	indexPoints[1] = indexPoints[0] + scaled[2];
	indexPoints[3] = indexPoints[2] + scaled[2];
	indexPoints[0] -= scaled[2];
	indexPoints[2] -= scaled[2];

	scaled[0] = origin + axis[0] * dFar;
	scaled[1] = axis[1] * dLeft;
	scaled[2] = axis[2] * dUp;

	indexPoints[4] = scaled[0] - scaled[1];
	indexPoints[6] = scaled[0] + scaled[1];
	indexPoints[5] = indexPoints[4] + scaled[2];
	indexPoints[7] = indexPoints[6] + scaled[2];
	indexPoints[4] -= scaled[2];
	indexPoints[6] -= scaled[2];
}

/*
============
ARCFrustum::ToIndexPointsAndCornerVecs

  22 muls
============
*/
void ARCFrustum::ToIndexPointsAndCornerVecs( arcVec3 indexPoints[8], arcVec3 cornerVecs[4] ) const {
	arcMat3 scaled;

	scaled[0] = origin + axis[0] * dNear;
	scaled[1] = axis[1] * ( dLeft * dNear * invFar );
	scaled[2] = axis[2] * ( dUp * dNear * invFar );

	indexPoints[0] = scaled[0] - scaled[1];
	indexPoints[2] = scaled[0] + scaled[1];
	indexPoints[1] = indexPoints[0] + scaled[2];
	indexPoints[3] = indexPoints[2] + scaled[2];
	indexPoints[0] -= scaled[2];
	indexPoints[2] -= scaled[2];

	scaled[0] = axis[0] * dFar;
	scaled[1] = axis[1] * dLeft;
	scaled[2] = axis[2] * dUp;

	cornerVecs[0] = scaled[0] - scaled[1];
	cornerVecs[2] = scaled[0] + scaled[1];
	cornerVecs[1] = cornerVecs[0] + scaled[2];
	cornerVecs[3] = cornerVecs[2] + scaled[2];
	cornerVecs[0] -= scaled[2];
	cornerVecs[2] -= scaled[2];

	indexPoints[4] = cornerVecs[0] + origin;
	indexPoints[5] = cornerVecs[1] + origin;
	indexPoints[6] = cornerVecs[2] + origin;
	indexPoints[7] = cornerVecs[3] + origin;
}

/*
============
ARCFrustum::AxisProjection

  18 muls
============
*/
void ARCFrustum::AxisProjection( const arcVec3 indexPoints[8], const arcVec3 cornerVecs[4], const arcVec3 &dir, float &min, float &max ) const {
	float dx, dy, dz;
	int index;

	dy = dir.x * axis[1].x + dir.y * axis[1].y + dir.z * axis[1].z;
	dz = dir.x * axis[2].x + dir.y * axis[2].y + dir.z * axis[2].z;
	index = ( FLOATSIGNBITSET( dy ) << 1 ) | FLOATSIGNBITSET( dz );
	dx = dir.x * cornerVecs[index].x + dir.y * cornerVecs[index].y + dir.z * cornerVecs[index].z;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );
	min = indexPoints[index] * dir;
	index = ~index & 3;
	dx = -dir.x * cornerVecs[index].x - dir.y * cornerVecs[index].y - dir.z * cornerVecs[index].z;
	index |= ( FLOATSIGNBITSET( dx ) << 2 );
	max = indexPoints[index] * dir;
}

/*
============
ARCFrustum::AxisProjection

  40 muls
============
*/
void ARCFrustum::AxisProjection( const arcVec3 &dir, float &min, float &max ) const {
	arcVec3 indexPoints[8], cornerVecs[4];

	ToIndexPointsAndCornerVecs( indexPoints, cornerVecs );
	AxisProjection( indexPoints, cornerVecs, dir, min, max );
}

/*
============
ARCFrustum::AxisProjection

  76 muls
============
*/
void ARCFrustum::AxisProjection( const arcMat3 &ax, arcBounds &bounds ) const {
	arcVec3 indexPoints[8], cornerVecs[4];

	ToIndexPointsAndCornerVecs( indexPoints, cornerVecs );
	AxisProjection( indexPoints, cornerVecs, ax[0], bounds[0][0], bounds[1][0] );
	AxisProjection( indexPoints, cornerVecs, ax[1], bounds[0][1], bounds[1][1] );
	AxisProjection( indexPoints, cornerVecs, ax[2], bounds[0][2], bounds[1][2] );
}

/*
============
ARCFrustum::AddLocalLineToProjectionBoundsSetCull
============
*/
void ARCFrustum::AddLocalLineToProjectionBoundsSetCull( const arcVec3 &start, const arcVec3 &end, int &startCull, int &endCull, arcBounds &bounds ) const {
	arcVec3 dir, p;
	float d1, d2, fstart, fend, lstart, lend, f;
	float leftScale, upScale;
	int cull1, cull2;

#ifdef FRUSTUM_DEBUG
	static arcCVarSystem r_showInteractionScissors( "r_showInteractionScissors", "0", CVAR_RENDERER | CVAR_INTEGER, "", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
	if ( r_showInteractionScissors.GetInteger() > 1 ) {
		session->rw->DebugLine( colorGreen, origin + start * axis, origin + end * axis );
	}
#endif

	leftScale = dLeft * invFar;
	upScale = dUp * invFar;
	dir = end - start;

	fstart = dFar * start.y;
	fend = dFar * end.y;
	lstart = dLeft * start.x;
	lend = dLeft * end.x;

	// test left plane
	d1 = -fstart + lstart;
	d2 = -fend + lend;
	cull1 = FLOATSIGNBITSET( d1 );
	cull2 = FLOATSIGNBITSET( d2 );
	if ( FLOATNOTZERO( d1 ) ) {
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			p.x = start.x + f * dir.x;
			if ( p.x > 0.0f ) {
				p.z = start.z + f * dir.z;
				if ( arcMath::Fabs( p.z ) <= p.x * upScale ) {
					p.y = 1.0f;
					p.z = p.z * dFar / ( p.x * dUp );
					bounds.AddPoint( p );
				}
			}
		}
	}

	// test right plane
	d1 = fstart + lstart;
	d2 = fend + lend;
	cull1 |= FLOATSIGNBITSET( d1 ) << 1;
	cull2 |= FLOATSIGNBITSET( d2 ) << 1;
	if ( FLOATNOTZERO( d1 ) ) {
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			p.x = start.x + f * dir.x;
			if ( p.x > 0.0f ) {
				p.z = start.z + f * dir.z;
				if ( arcMath::Fabs( p.z  ) <= p.x * upScale ) {
					p.y = -1.0f;
					p.z = p.z * dFar / ( p.x * dUp );
					bounds.AddPoint( p );
				}
			}
		}
	}

	fstart = dFar * start.z;
	fend = dFar * end.z;
	lstart = dUp * start.x;
	lend = dUp * end.x;

	// test up plane
	d1 = -fstart + lstart;
	d2 = -fend + lend;
	cull1 |= FLOATSIGNBITSET( d1 ) << 2;
	cull2 |= FLOATSIGNBITSET( d2 ) << 2;
	if ( FLOATNOTZERO( d1 ) ) {
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			p.x = start.x + f * dir.x;
			if ( p.x > 0.0f ) {
				p.y = start.y + f * dir.y;
				if ( arcMath::Fabs( p.y ) <= p.x * leftScale ) {
					p.y = p.y * dFar / ( p.x * dLeft );
					p.z = 1.0f;
					bounds.AddPoint( p );
				}
			}
		}
	}

	// test down plane
	d1 = fstart + lstart;
	d2 = fend + lend;
	cull1 |= FLOATSIGNBITSET( d1 ) << 3;
	cull2 |= FLOATSIGNBITSET( d2 ) << 3;
	if ( FLOATNOTZERO( d1 ) ) {
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			p.x = start.x + f * dir.x;
			if ( p.x > 0.0f ) {
				p.y = start.y + f * dir.y;
				if ( arcMath::Fabs( p.y ) <= p.x * leftScale ) {
					p.y = p.y * dFar / ( p.x * dLeft );
					p.z = -1.0f;
					bounds.AddPoint( p );
				}
			}
		}
	}

	if ( cull1 == 0 && start.x > 0.0f ) {
		// add start point to projection bounds
		p.x = start.x;
		p.y = start.y * dFar / ( start.x * dLeft );
		p.z = start.z * dFar / ( start.x * dUp );
		bounds.AddPoint( p );
	}

	if ( cull2 == 0 && end.x > 0.0f ) {
		// add end point to projection bounds
		p.x = end.x;
		p.y = end.y * dFar / ( end.x * dLeft );
		p.z = end.z * dFar / ( end.x * dUp );
		bounds.AddPoint( p );
	}

	if ( start.x < bounds[0].x ) {
		bounds[0].x = start.x < 0.0f ? 0.0f : start.x;
	}
	if ( end.x < bounds[0].x ) {
		bounds[0].x = end.x < 0.0f ? 0.0f : end.x;
	}

	startCull = cull1;
	endCull = cull2;
}

/*
============
ARCFrustum::AddLocalLineToProjectionBoundsUseCull
============
*/
void ARCFrustum::AddLocalLineToProjectionBoundsUseCull( const arcVec3 &start, const arcVec3 &end, int startCull, int endCull, arcBounds &bounds ) const {
	arcVec3 dir, p;
	float d1, d2, fstart, fend, lstart, lend, f;
	float leftScale, upScale;
	int clip;

	clip = startCull ^ endCull;
	if ( !clip ) {
		return;
	}

#ifdef FRUSTUM_DEBUG
	static arcCVarSystem r_showInteractionScissors( "r_showInteractionScissors", "0", CVAR_RENDERER | CVAR_INTEGER, "", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
	if ( r_showInteractionScissors.GetInteger() > 1 ) {
		session->rw->DebugLine( colorGreen, origin + start * axis, origin + end * axis );
	}
#endif

	leftScale = dLeft * invFar;
	upScale = dUp * invFar;
	dir = end - start;

	if ( clip & (1|2) ) {

		fstart = dFar * start.y;
		fend = dFar * end.y;
		lstart = dLeft * start.x;
		lend = dLeft * end.x;

		if ( clip & 1 ) {
			// test left plane
			d1 = -fstart + lstart;
			d2 = -fend + lend;
			if ( FLOATNOTZERO( d1 ) ) {
				if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
					f = d1 / ( d1 - d2 );
					p.x = start.x + f * dir.x;
					if ( p.x > 0.0f ) {
						p.z = start.z + f * dir.z;
						if ( arcMath::Fabs( p.z ) <= p.x * upScale ) {
							p.y = 1.0f;
							p.z = p.z * dFar / ( p.x * dUp );
							bounds.AddPoint( p );
						}
					}
				}
			}
		}

		if ( clip & 2 ) {
			// test right plane
			d1 = fstart + lstart;
			d2 = fend + lend;
			if ( FLOATNOTZERO( d1 ) ) {
				if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
					f = d1 / ( d1 - d2 );
					p.x = start.x + f * dir.x;
					if ( p.x > 0.0f ) {
						p.z = start.z + f * dir.z;
						if ( arcMath::Fabs( p.z  ) <= p.x * upScale ) {
							p.y = -1.0f;
							p.z = p.z * dFar / ( p.x * dUp );
							bounds.AddPoint( p );
						}
					}
				}
			}
		}
	}

	if ( clip & (4|8) ) {

		fstart = dFar * start.z;
		fend = dFar * end.z;
		lstart = dUp * start.x;
		lend = dUp * end.x;

		if ( clip & 4 ) {
			// test up plane
			d1 = -fstart + lstart;
			d2 = -fend + lend;
			if ( FLOATNOTZERO( d1 ) ) {
				if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
					f = d1 / ( d1 - d2 );
					p.x = start.x + f * dir.x;
					if ( p.x > 0.0f ) {
						p.y = start.y + f * dir.y;
						if ( arcMath::Fabs( p.y ) <= p.x * leftScale ) {
							p.y = p.y * dFar / ( p.x * dLeft );
							p.z = 1.0f;
							bounds.AddPoint( p );
						}
					}
				}
			}
		}

		if ( clip & 8 ) {
			// test down plane
			d1 = fstart + lstart;
			d2 = fend + lend;
			if ( FLOATNOTZERO( d1 ) ) {
				if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
					f = d1 / ( d1 - d2 );
					p.x = start.x + f * dir.x;
					if ( p.x > 0.0f ) {
						p.y = start.y + f * dir.y;
						if ( arcMath::Fabs( p.y ) <= p.x * leftScale ) {
							p.y = p.y * dFar / ( p.x * dLeft );
							p.z = -1.0f;
							bounds.AddPoint( p );
						}
					}
				}
			}
		}
	}
}

/*
============
ARCFrustum::BoundsRayIntersection

  Returns true if the ray starts inside the bounds.
  If there was an intersection scale1 <= scale2
============
*/
bool ARCFrustum::BoundsRayIntersection( const arcBounds &bounds, const arcVec3 &start, const arcVec3 &dir, float &scale1, float &scale2 ) const {
	arcVec3 end, p;
	float d1, d2, f;
	int i, startInside = 1;

	scale1 = arcMath::INFINITY;
	scale2 = -arcMath::INFINITY;

	end = start + dir;

	for ( i = 0; i < 2; i++ ) {
		d1 = start.x - bounds[i].x;
		startInside &= FLOATSIGNBITSET( d1 ) ^ i;
		d2 = end.x - bounds[i].x;
		if ( d1 != d2 ) {
			f = d1 / ( d1 - d2 );
			p.y = start.y + f * dir.y;
			if ( bounds[0].y <= p.y && p.y <= bounds[1].y ) {
				p.z = start.z + f * dir.z;
				if ( bounds[0].z <= p.z && p.z <= bounds[1].z ) {
					if ( f < scale1 ) scale1 = f;
					if ( f > scale2 ) scale2 = f;
				}
			}
		}

		d1 = start.y - bounds[i].y;
		startInside &= FLOATSIGNBITSET( d1 ) ^ i;
		d2 = end.y - bounds[i].y;
		if ( d1 != d2 ) {
			f = d1 / ( d1 - d2 );
			p.x = start.x + f * dir.x;
			if ( bounds[0].x <= p.x && p.x <= bounds[1].x ) {
				p.z = start.z + f * dir.z;
				if ( bounds[0].z <= p.z && p.z <= bounds[1].z ) {
					if ( f < scale1 ) scale1 = f;
					if ( f > scale2 ) scale2 = f;
				}
			}
		}

		d1 = start.z - bounds[i].z;
		startInside &= FLOATSIGNBITSET( d1 ) ^ i;
		d2 = end.z - bounds[i].z;
		if ( d1 != d2 ) {
			f = d1 / ( d1 - d2 );
			p.x = start.x + f * dir.x;
			if ( bounds[0].x <= p.x && p.x <= bounds[1].x ) {
				p.y = start.y + f * dir.y;
				if ( bounds[0].y <= p.y && p.y <= bounds[1].y ) {
					if ( f < scale1 ) scale1 = f;
					if ( f > scale2 ) scale2 = f;
				}
			}
		}
	}

	return ( startInside != 0 );
}

/*
============
ARCFrustum::ProjectionBounds
============
*/
bool ARCFrustum::ProjectionBounds( const arcBounds &bounds, arcBounds &projectionBounds ) const {
	return ProjectionBounds( ARCBox( bounds, vec3_origin, mat3_identity ), projectionBounds );
}

#ifndef __linux__

/*
============
ARCFrustum::ProjectionBounds
============
*/
bool ARCFrustum::ProjectionBounds( const ARCBox &box, arcBounds &projectionBounds ) const {
	int i, p1, p2, pointCull[8], culled, outside;
	float scale1, scale2;
	ARCFrustum localFrustum;
	arcVec3 points[8], localOrigin;
	arcMat3 localAxis, localScaled;
	arcBounds bounds( -box.GetExtents(), box.GetExtents() );

	// if the frustum origin is inside the bounds
	if ( bounds.ContainsPoint( ( origin - box.GetCenter() ) * box.GetAxis().Transpose() ) ) {
		// bounds that cover the whole frustum
		float boxMin, boxMax, base;

		base = origin * axis[0];
		box.AxisProjection( axis[0], boxMin, boxMax );

		projectionBounds[0].x = boxMin - base;
		projectionBounds[1].x = boxMax - base;
		projectionBounds[0].y = projectionBounds[0].z = -1.0f;
		projectionBounds[1].y = projectionBounds[1].z = 1.0f;

		return true;
	}

	projectionBounds.Clear();

	// transform the bounds into the space of this frustum
	localOrigin = ( box.GetCenter() - origin ) * axis.Transpose();
	localAxis = box.GetAxis() * axis.Transpose();
	BoxToPoints( localOrigin, box.GetExtents(), localAxis, points );

	// test outer four edges of the bounds
	culled = -1;
	outside = 0;
	for ( i = 0; i < 4; i++ ) {
		p1 = i;
		p2 = 4 + i;
		AddLocalLineToProjectionBoundsSetCull( points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds );
		culled &= pointCull[p1] & pointCull[p2];
		outside |= pointCull[p1] | pointCull[p2];
	}

	// if the bounds are completely outside this frustum
	if ( culled ) {
		return false;
	}

	// if the bounds are completely inside this frustum
	if ( !outside ) {
		return true;
	}

	// test the remaining edges of the bounds
	for ( i = 0; i < 4; i++ ) {
		p1 = i;
		p2 = ( i+1 )&3;
		AddLocalLineToProjectionBoundsUseCull( points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds );
	}

	for ( i = 0; i < 4; i++ ) {
		p1 = 4 + i;
		p2 = 4 + (( i+1 )&3);
		AddLocalLineToProjectionBoundsUseCull( points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds );
	}

	// if the bounds extend beyond two or more boundaries of this frustum
	if ( outside != 1 && outside != 2 && outside != 4 && outside != 8 ) {

		localOrigin = ( origin - box.GetCenter() ) * box.GetAxis().Transpose();
		localScaled = axis * box.GetAxis().Transpose();
		localScaled[0] *= dFar;
		localScaled[1] *= dLeft;
		localScaled[2] *= dUp;

		// test the outer edges of this frustum for intersection with the bounds
		if ( (outside & 2) && (outside & 8) ) {
			BoundsRayIntersection( bounds, localOrigin, localScaled[0] - localScaled[1] - localScaled[2], scale1, scale2 );
			if ( scale1 <= scale2 && scale1 >= 0.0f ) {
				projectionBounds.AddPoint( arcVec3( scale1 * dFar, -1.0f, -1.0f ) );
				projectionBounds.AddPoint( arcVec3( scale2 * dFar, -1.0f, -1.0f ) );
			}
		}
		if ( (outside & 2) && (outside & 4) ) {
			BoundsRayIntersection( bounds, localOrigin, localScaled[0] - localScaled[1] + localScaled[2], scale1, scale2 );
			if ( scale1 <= scale2 && scale1 >= 0.0f  ) {
				projectionBounds.AddPoint( arcVec3( scale1 * dFar, -1.0f, 1.0f ) );
				projectionBounds.AddPoint( arcVec3( scale2 * dFar, -1.0f, 1.0f ) );
			}
		}
		if ( (outside & 1 ) && (outside & 8) ) {
			BoundsRayIntersection( bounds, localOrigin, localScaled[0] + localScaled[1] - localScaled[2], scale1, scale2 );
			if ( scale1 <= scale2 && scale1 >= 0.0f  ) {
				projectionBounds.AddPoint( arcVec3( scale1 * dFar, 1.0f, -1.0f ) );
				projectionBounds.AddPoint( arcVec3( scale2 * dFar, 1.0f, -1.0f ) );
			}
		}
		if ( (outside & 1 ) && (outside & 2) ) {
			BoundsRayIntersection( bounds, localOrigin, localScaled[0] + localScaled[1] + localScaled[2], scale1, scale2 );
			if ( scale1 <= scale2 && scale1 >= 0.0f  ) {
				projectionBounds.AddPoint( arcVec3( scale1 * dFar, 1.0f, 1.0f ) );
				projectionBounds.AddPoint( arcVec3( scale2 * dFar, 1.0f, 1.0f ) );
			}
		}
	}

	return true;
}

#endif

/*
============
ARCFrustum::ProjectionBounds
============
*/
bool ARCFrustum::ProjectionBounds( const ARCSphere &sphere, arcBounds &projectionBounds ) const {
	float d, r, rs, sFar;
	arcVec3 center;

	projectionBounds.Clear();

	center = ( sphere.GetOrigin() - origin ) * axis.Transpose();
	r = sphere.GetRadius();
	rs = r * r;
	sFar = dFar * dFar;

	// test left/right planes
	d = dFar * arcMath::Fabs( center.y ) - dLeft * center.x;
	if ( ( d * d ) > rs * ( sFar + dLeft * dLeft ) ) {
		return false;
	}

	// test up/down planes
	d = dFar * arcMath::Fabs( center.z ) - dUp * center.x;
	if ( ( d * d ) > rs * ( sFar + dUp * dUp ) ) {
		return false;
	}

	// bounds that cover the whole frustum
	projectionBounds[0].x = 0.0f;
	projectionBounds[1].x = dFar;
	projectionBounds[0].y = projectionBounds[0].z = -1.0f;
	projectionBounds[1].y = projectionBounds[1].z = 1.0f;
	return true;
}

/*
============
ARCFrustum::ProjectionBounds
============
*/
bool ARCFrustum::ProjectionBounds( const ARCFrustum &frustum, arcBounds &projectionBounds ) const {
	int i, p1, p2, pointCull[8], culled, outside;
	float scale1, scale2;
	ARCFrustum localFrustum;
	arcVec3 points[8], localOrigin;
	arcMat3 localScaled;

	// if the frustum origin is inside the other frustum
	if ( frustum.ContainsPoint( origin ) ) {
		// bounds that cover the whole frustum
		float frustumMin, frustumMax, base;

		base = origin * axis[0];
		frustum.AxisProjection( axis[0], frustumMin, frustumMax );

		projectionBounds[0].x = frustumMin - base;
		projectionBounds[1].x = frustumMax - base;
		projectionBounds[0].y = projectionBounds[0].z = -1.0f;
		projectionBounds[1].y = projectionBounds[1].z = 1.0f;
		return true;
	}

	projectionBounds.Clear();

	// transform the given frustum into the space of this frustum
	localFrustum = frustum;
	localFrustum.origin = ( frustum.origin - origin ) * axis.Transpose();
	localFrustum.axis = frustum.axis * axis.Transpose();
	localFrustum.ToPoints( points );

	// test outer four edges of the other frustum
	culled = -1;
	outside = 0;
	for ( i = 0; i < 4; i++ ) {
		p1 = i;
		p2 = 4 + i;
		AddLocalLineToProjectionBoundsSetCull( points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds );
		culled &= pointCull[p1] & pointCull[p2];
		outside |= pointCull[p1] | pointCull[p2];
	}

	// if the other frustum is completely outside this frustum
	if ( culled ) {
		return false;
	}

	// if the other frustum is completely inside this frustum
	if ( !outside ) {
		return true;
	}

	// test the remaining edges of the other frustum
	if ( localFrustum.dNear > 0.0f ) {
		for ( i = 0; i < 4; i++ ) {
			p1 = i;
			p2 = ( i+1 )&3;
			AddLocalLineToProjectionBoundsUseCull( points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds );
		}
	}

	for ( i = 0; i < 4; i++ ) {
		p1 = 4 + i;
		p2 = 4 + (( i+1 )&3);
		AddLocalLineToProjectionBoundsUseCull( points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds );
	}

	// if the other frustum extends beyond two or more boundaries of this frustum
	if ( outside != 1 && outside != 2 && outside != 4 && outside != 8 ) {

		localOrigin = ( origin - frustum.origin ) * frustum.axis.Transpose();
		localScaled = axis * frustum.axis.Transpose();
		localScaled[0] *= dFar;
		localScaled[1] *= dLeft;
		localScaled[2] *= dUp;

		// test the outer edges of this frustum for intersection with the other frustum
		if ( (outside & 2) && (outside & 8) ) {
			frustum.LocalRayIntersection( localOrigin, localScaled[0] - localScaled[1] - localScaled[2], scale1, scale2 );
			if ( scale1 <= scale2 && scale1 >= 0.0f ) {
				projectionBounds.AddPoint( arcVec3( scale1 * dFar, -1.0f, -1.0f ) );
				projectionBounds.AddPoint( arcVec3( scale2 * dFar, -1.0f, -1.0f ) );
			}
		}
		if ( (outside & 2) && (outside & 4) ) {
			frustum.LocalRayIntersection( localOrigin, localScaled[0] - localScaled[1] + localScaled[2], scale1, scale2 );
			if ( scale1 <= scale2 && scale1 >= 0.0f  ) {
				projectionBounds.AddPoint( arcVec3( scale1 * dFar, -1.0f, 1.0f ) );
				projectionBounds.AddPoint( arcVec3( scale2 * dFar, -1.0f, 1.0f ) );
			}
		}
		if ( (outside & 1 ) && (outside & 8) ) {
			frustum.LocalRayIntersection( localOrigin, localScaled[0] + localScaled[1] - localScaled[2], scale1, scale2 );
			if ( scale1 <= scale2 && scale1 >= 0.0f  ) {
				projectionBounds.AddPoint( arcVec3( scale1 * dFar, 1.0f, -1.0f ) );
				projectionBounds.AddPoint( arcVec3( scale2 * dFar, 1.0f, -1.0f ) );
			}
		}
		if ( (outside & 1 ) && (outside & 2) ) {
			frustum.LocalRayIntersection( localOrigin, localScaled[0] + localScaled[1] + localScaled[2], scale1, scale2 );
			if ( scale1 <= scale2 && scale1 >= 0.0f  ) {
				projectionBounds.AddPoint( arcVec3( scale1 * dFar, 1.0f, 1.0f ) );
				projectionBounds.AddPoint( arcVec3( scale2 * dFar, 1.0f, 1.0f ) );
			}
		}
	}

	return true;
}

/*
============
ARCFrustum::ProjectionBounds
============
*/
bool ARCFrustum::ProjectionBounds( const arcWinding &winding, arcBounds &projectionBounds ) const {
	int i, p1, p2, *pointCull, culled, outside;
	float scale;
	arcVec3 *localPoints;
	arcMat3 transpose, scaled;
	arcPlane plane;

	projectionBounds.Clear();

	// transform the winding points into the space of this frustum
	localPoints = (arcVec3 *) _alloca16( winding.GetNumPoints() * sizeof( arcVec3 ) );
	transpose = axis.Transpose();
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		localPoints[i] = ( winding[i].ToVec3() - origin ) * transpose;
	}

	// test the winding edges
	culled = -1;
	outside = 0;
	pointCull = ( int * ) _alloca16( winding.GetNumPoints() * sizeof( int ) );
	for ( i = 0; i < winding.GetNumPoints(); i += 2 ) {
		p1 = i;
		p2 = ( i+1 )%winding.GetNumPoints();
		AddLocalLineToProjectionBoundsSetCull( localPoints[p1], localPoints[p2], pointCull[p1], pointCull[p2], projectionBounds );
		culled &= pointCull[p1] & pointCull[p2];
		outside |= pointCull[p1] | pointCull[p2];
	}

	// if completely culled
	if ( culled ) {
		return false;
	}

	// if completely inside
	if ( !outside ) {
		return true;
	}

	// test remaining winding edges
	for ( i = 1; i < winding.GetNumPoints(); i += 2 ) {
		p1 = i;
		p2 = ( i+1 )%winding.GetNumPoints();
		AddLocalLineToProjectionBoundsUseCull( localPoints[p1], localPoints[p2], pointCull[p1], pointCull[p2], projectionBounds );
	}

	// if the winding extends beyond two or more boundaries of this frustum
	if ( outside != 1 && outside != 2 && outside != 4 && outside != 8 ) {

		winding.GetPlane( plane );
		scaled[0] = axis[0] * dFar;
		scaled[1] = axis[1] * dLeft;
		scaled[2] = axis[2] * dUp;

		// test the outer edges of this frustum for intersection with the winding
		if ( (outside & 2) && (outside & 8) ) {
			if ( winding.RayIntersection( plane, origin, scaled[0] - scaled[1] - scaled[2], scale ) ) {
				projectionBounds.AddPoint( arcVec3( scale * dFar, -1.0f, -1.0f ) );
			}
		}
		if ( (outside & 2) && (outside & 4) ) {
			if ( winding.RayIntersection( plane, origin, scaled[0] - scaled[1] + scaled[2], scale ) ) {
				projectionBounds.AddPoint( arcVec3( scale * dFar, -1.0f, 1.0f ) );
			}
		}
		if ( (outside & 1 ) && (outside & 8) ) {
			if ( winding.RayIntersection( plane, origin, scaled[0] + scaled[1] - scaled[2], scale ) ) {
				projectionBounds.AddPoint( arcVec3( scale * dFar, 1.0f, -1.0f ) );
			}
		}
		if ( (outside & 1 ) && (outside & 2) ) {
			if ( winding.RayIntersection( plane, origin, scaled[0] + scaled[1] + scaled[2], scale ) ) {
				projectionBounds.AddPoint( arcVec3( scale * dFar, 1.0f, 1.0f ) );
			}
		}
	}

	return true;
}

/*
============
ARCFrustum::ClipFrustumToBox

  Clips the frustum far extents to the box.
============
*/
void ARCFrustum::ClipFrustumToBox( const ARCBox &box, float clipFractions[4], int clipPlanes[4] ) const {
	int i, index;
	float f, minf;
	arcMat3 scaled, localAxis, transpose;
	arcVec3 localOrigin, cornerVecs[4];
	arcBounds bounds;

	transpose = box.GetAxis();
	transpose.TransposeSelf();
	localOrigin = ( origin - box.GetCenter() ) * transpose;
	localAxis = axis * transpose;

	scaled[0] = localAxis[0] * dFar;
	scaled[1] = localAxis[1] * dLeft;
	scaled[2] = localAxis[2] * dUp;
	cornerVecs[0] = scaled[0] + scaled[1];
	cornerVecs[1] = scaled[0] - scaled[1];
	cornerVecs[2] = cornerVecs[1] - scaled[2];
	cornerVecs[3] = cornerVecs[0] - scaled[2];
	cornerVecs[0] += scaled[2];
	cornerVecs[1] += scaled[2];

	bounds[0] = -box.GetExtents();
	bounds[1] = box.GetExtents();

	minf = ( dNear + 1.0f ) * invFar;

	for ( i = 0; i < 4; i++ ) {

		index = FLOATSIGNBITNOTSET( cornerVecs[i].x );
		f = ( bounds[index].x - localOrigin.x ) / cornerVecs[i].x;
		clipFractions[i] = f;
		clipPlanes[i] = 1 << index;

		index = FLOATSIGNBITNOTSET( cornerVecs[i].y );
		f = ( bounds[index].y - localOrigin.y ) / cornerVecs[i].y;
		if ( f < clipFractions[i] ) {
			clipFractions[i] = f;
			clipPlanes[i] = 4 << index;
		}

		index = FLOATSIGNBITNOTSET( cornerVecs[i].z );
		f = ( bounds[index].z - localOrigin.z ) / cornerVecs[i].z;
		if ( f < clipFractions[i] ) {
			clipFractions[i] = f;
			clipPlanes[i] = 16 << index;
		}

		// make sure the frustum is not clipped between the frustum origin and the near plane
		if ( clipFractions[i] < minf ) {
			clipFractions[i] = minf;
		}
	}
}

/*
============
ARCFrustum::ClipLine

  Returns true if part of the line is inside the frustum.
  Does not clip to the near and far plane.
============
*/
bool ARCFrustum::ClipLine( const arcVec3 localPoints[8], const arcVec3 points[8], int startIndex, int endIndex, arcVec3 &start, arcVec3 &end, int &startClip, int &endClip ) const {
	float d1, d2, fstart, fend, lstart, lend, f, x;
	float leftScale, upScale;
	float scale1, scale2;
	int startCull, endCull;
	arcVec3 localStart, localEnd, localDir;

	leftScale = dLeft * invFar;
	upScale = dUp * invFar;

	localStart = localPoints[startIndex];
	localEnd = localPoints[endIndex];
	localDir = localEnd - localStart;

	startClip = endClip = -1;
	scale1 = arcMath::INFINITY;
	scale2 = -arcMath::INFINITY;

	fstart = dFar * localStart.y;
	fend = dFar * localEnd.y;
	lstart = dLeft * localStart.x;
	lend = dLeft * localEnd.x;

	// test left plane
	d1 = -fstart + lstart;
	d2 = -fend + lend;
	startCull = FLOATSIGNBITSET( d1 );
	endCull = FLOATSIGNBITSET( d2 );
	if ( FLOATNOTZERO( d1 ) ) {
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			x = localStart.x + f * localDir.x;
			if ( x >= 0.0f ) {
				if ( arcMath::Fabs( localStart.z + f * localDir.z ) <= x * upScale ) {
					if ( f < scale1 ) { scale1 = f; startClip = 0; }
					if ( f > scale2 ) { scale2 = f; endClip = 0; }
				}
			}
		}
	}

	// test right plane
	d1 = fstart + lstart;
	d2 = fend + lend;
	startCull |= FLOATSIGNBITSET( d1 ) << 1;
	endCull |= FLOATSIGNBITSET( d2 ) << 1;
	if ( FLOATNOTZERO( d1 ) ) {
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			x = localStart.x + f * localDir.x;
			if ( x >= 0.0f ) {
				if ( arcMath::Fabs( localStart.z + f * localDir.z ) <= x * upScale ) {
					if ( f < scale1 ) { scale1 = f; startClip = 1; }
					if ( f > scale2 ) { scale2 = f; endClip = 1; }
				}
			}
		}
	}

	fstart = dFar * localStart.z;
	fend = dFar * localEnd.z;
	lstart = dUp * localStart.x;
	lend = dUp * localEnd.x;

	// test up plane
	d1 = -fstart + lstart;
	d2 = -fend + lend;
	startCull |= FLOATSIGNBITSET( d1 ) << 2;
	endCull |= FLOATSIGNBITSET( d2 ) << 2;
	if ( FLOATNOTZERO( d1 ) ) {
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			x = localStart.x + f * localDir.x;
			if ( x >= 0.0f ) {
				if ( arcMath::Fabs( localStart.y + f * localDir.y ) <= x * leftScale ) {
					if ( f < scale1 ) { scale1 = f; startClip = 2; }
					if ( f > scale2 ) { scale2 = f; endClip = 2; }
				}
			}
		}
	}

	// test down plane
	d1 = fstart + lstart;
	d2 = fend + lend;
	startCull |= FLOATSIGNBITSET( d1 ) << 3;
	endCull |= FLOATSIGNBITSET( d2 ) << 3;
	if ( FLOATNOTZERO( d1 ) ) {
		if ( FLOATSIGNBITSET( d1 ) ^ FLOATSIGNBITSET( d2 ) ) {
			f = d1 / ( d1 - d2 );
			x = localStart.x + f * localDir.x;
			if ( x >= 0.0f ) {
				if ( arcMath::Fabs( localStart.y + f * localDir.y ) <= x * leftScale ) {
					if ( f < scale1 ) { scale1 = f; startClip = 3; }
					if ( f > scale2 ) { scale2 = f; endClip = 3; }
				}
			}
		}
	}

	// if completely inside
	if ( !( startCull | endCull ) ) {
		start = points[startIndex];
		end = points[endIndex];
		return true;
	}
	else if ( scale1 <= scale2 ) {
		if ( !startCull ) {
			start = points[startIndex];
			startClip = -1;
		}
		else {
			start = points[startIndex] + scale1 * ( points[endIndex] - points[startIndex] );
		}
		if ( !endCull ) {
			end = points[endIndex];
			endClip = -1;
		}
		else {
			end = points[startIndex] + scale2 * ( points[endIndex] - points[startIndex] );
		}
		return true;
	}
	return false;
}

/*
============
ARCFrustum::AddLocalCapsToProjectionBounds
============
*/
static int capPointIndex[4][2] = {
	{ 0, 3 },
	{ 1, 2 },
	{ 0, 1 },
	{ 2, 3 }
};

ARC_INLINE bool ARCFrustum::AddLocalCapsToProjectionBounds( const arcVec3 endPoints[4], const int endPointCull[4], const arcVec3 &point, int pointCull, int pointClip, arcBounds &projectionBounds ) const {
	int *p;

	if ( pointClip < 0 ) {
		return false;
	}
	p = capPointIndex[pointClip];
	AddLocalLineToProjectionBoundsUseCull( endPoints[p[0]], point, endPointCull[p[0]], pointCull, projectionBounds );
	AddLocalLineToProjectionBoundsUseCull( endPoints[p[1]], point, endPointCull[p[1]], pointCull, projectionBounds );
	return true;
}

/*
============
ARCFrustum::ClippedProjectionBounds
============
*/
bool ARCFrustum::ClippedProjectionBounds( const ARCFrustum &frustum, const ARCBox &clipBox, arcBounds &projectionBounds ) const {
	int i, p1, p2, clipPointCull[8], clipPlanes[4], usedClipPlanes, nearCull, farCull, outside;
	int pointCull[2], startClip, endClip, boxPointCull[8];
	float clipFractions[4], s1, s2, t1, t2, leftScale, upScale;
	ARCFrustum localFrustum;
	arcVec3 clipPoints[8], localPoints1[8], localPoints2[8], localOrigin1, localOrigin2, start, end;
	arcMat3 localAxis1, localAxis2, transpose;
	arcBounds clipBounds;

	// if the frustum origin is inside the other frustum
	if ( frustum.ContainsPoint( origin ) ) {
		// bounds that cover the whole frustum
		float clipBoxMin, clipBoxMax, frustumMin, frustumMax, base;

		base = origin * axis[0];
		clipBox.AxisProjection( axis[0], clipBoxMin, clipBoxMax );
		frustum.AxisProjection( axis[0], frustumMin, frustumMax );

		projectionBounds[0].x = Max( clipBoxMin, frustumMin ) - base;
		projectionBounds[1].x = Min( clipBoxMax, frustumMax ) - base;
		projectionBounds[0].y = projectionBounds[0].z = -1.0f;
		projectionBounds[1].y = projectionBounds[1].z = 1.0f;
		return true;
	}

	projectionBounds.Clear();

	// clip the outer edges of the given frustum to the clip bounds
	frustum.ClipFrustumToBox( clipBox, clipFractions, clipPlanes );
	usedClipPlanes = clipPlanes[0] | clipPlanes[1] | clipPlanes[2] | clipPlanes[3];

	// transform the clipped frustum to the space of this frustum
	transpose = axis;
	transpose.TransposeSelf();
	localFrustum = frustum;
	localFrustum.origin = ( frustum.origin - origin ) * transpose;
	localFrustum.axis = frustum.axis * transpose;
	localFrustum.ToClippedPoints( clipFractions, clipPoints );

	// test outer four edges of the clipped frustum
	for ( i = 0; i < 4; i++ ) {
		p1 = i;
		p2 = 4 + i;
		AddLocalLineToProjectionBoundsSetCull( clipPoints[p1], clipPoints[p2], clipPointCull[p1], clipPointCull[p2], projectionBounds );
	}

	// get cull bits for the clipped frustum
	outside = clipPointCull[0] | clipPointCull[1] | clipPointCull[2] | clipPointCull[3] |
					clipPointCull[4] | clipPointCull[5] | clipPointCull[6] | clipPointCull[7];
	nearCull = clipPointCull[0] & clipPointCull[1] & clipPointCull[2] & clipPointCull[3];
	farCull = clipPointCull[4] & clipPointCull[5] & clipPointCull[6] & clipPointCull[7];

	// if the clipped frustum is not completely inside this frustum
	if ( outside ) {

		// test the remaining edges of the clipped frustum
		if ( !nearCull && localFrustum.dNear > 0.0f ) {
			for ( i = 0; i < 4; i++ ) {
				p1 = i;
				p2 = ( i+1 )&3;
				AddLocalLineToProjectionBoundsUseCull( clipPoints[p1], clipPoints[p2], clipPointCull[p1], clipPointCull[p2], projectionBounds );
			}
		}

		if ( !farCull ) {
			for ( i = 0; i < 4; i++ ) {
				p1 = 4 + i;
				p2 = 4 + (( i+1 )&3);
				AddLocalLineToProjectionBoundsUseCull( clipPoints[p1], clipPoints[p2], clipPointCull[p1], clipPointCull[p2], projectionBounds );
			}
		}
	}

	// if the clipped frustum far end points are inside this frustum
	if ( !( farCull && !( nearCull & farCull ) ) &&
			// if the clipped frustum is not clipped to a single plane of the clip bounds
			( clipPlanes[0] != clipPlanes[1] || clipPlanes[1] != clipPlanes[2] || clipPlanes[2] != clipPlanes[3] ) ) {

		// transform the clip box into the space of the other frustum
		transpose = frustum.axis;
		transpose.TransposeSelf();
		localOrigin1 = ( clipBox.GetCenter() - frustum.origin ) * transpose;
		localAxis1 = clipBox.GetAxis() * transpose;
		BoxToPoints( localOrigin1, clipBox.GetExtents(), localAxis1, localPoints1 );

		// cull the box corners with the other frustum
		leftScale = frustum.dLeft * frustum.invFar;
		upScale = frustum.dUp * frustum.invFar;
		for ( i = 0; i < 8; i++ ) {
			arcVec3 &p = localPoints1[i];
			if ( !( boxVertPlanes[i] & usedClipPlanes ) || p.x <= 0.0f ) {
				boxPointCull[i] = 1|2|4|8;
			}
			else {
				boxPointCull[i] = 0;
				if ( arcMath::Fabs( p.y ) > p.x * leftScale ) {
					boxPointCull[i] |= 1 << FLOATSIGNBITSET( p.y );
				}
				if ( arcMath::Fabs( p.z ) > p.x * upScale ) {
					boxPointCull[i] |= 4 << FLOATSIGNBITSET( p.z );
				}
			}
		}

		// transform the clip box into the space of this frustum
		transpose = axis;
		transpose.TransposeSelf();
		localOrigin2 = ( clipBox.GetCenter() - origin ) * transpose;
		localAxis2 = clipBox.GetAxis() * transpose;
		BoxToPoints( localOrigin2, clipBox.GetExtents(), localAxis2, localPoints2 );

		// clip the edges of the clip bounds to the other frustum and add the clipped edges to the projection bounds
		for ( i = 0; i < 4; i++ ) {
			p1 = i;
			p2 = 4 + i;
			if ( !( boxPointCull[p1] & boxPointCull[p2] ) ) {
				if ( frustum.ClipLine( localPoints1, localPoints2, p1, p2, start, end, startClip, endClip ) ) {
					AddLocalLineToProjectionBoundsSetCull( start, end, pointCull[0], pointCull[1], projectionBounds );
					AddLocalCapsToProjectionBounds( clipPoints+4, clipPointCull+4, start, pointCull[0], startClip, projectionBounds );
					AddLocalCapsToProjectionBounds( clipPoints+4, clipPointCull+4, end, pointCull[1], endClip, projectionBounds );
					outside |= pointCull[0] | pointCull[1];
				}
			}
		}

		for ( i = 0; i < 4; i++ ) {
			p1 = i;
			p2 = ( i+1 )&3;
			if ( !( boxPointCull[p1] & boxPointCull[p2] ) ) {
				if ( frustum.ClipLine( localPoints1, localPoints2, p1, p2, start, end, startClip, endClip ) ) {
					AddLocalLineToProjectionBoundsSetCull( start, end, pointCull[0], pointCull[1], projectionBounds );
					AddLocalCapsToProjectionBounds( clipPoints+4, clipPointCull+4, start, pointCull[0], startClip, projectionBounds );
					AddLocalCapsToProjectionBounds( clipPoints+4, clipPointCull+4, end, pointCull[1], endClip, projectionBounds );
					outside |= pointCull[0] | pointCull[1];
				}
			}
		}

		for ( i = 0; i < 4; i++ ) {
			p1 = 4 + i;
			p2 = 4 + (( i+1 )&3);
			if ( !( boxPointCull[p1] & boxPointCull[p2] ) ) {
				if ( frustum.ClipLine( localPoints1, localPoints2, p1, p2, start, end, startClip, endClip ) ) {
					AddLocalLineToProjectionBoundsSetCull( start, end, pointCull[0], pointCull[1], projectionBounds );
					AddLocalCapsToProjectionBounds( clipPoints+4, clipPointCull+4, start, pointCull[0], startClip, projectionBounds );
					AddLocalCapsToProjectionBounds( clipPoints+4, clipPointCull+4, end, pointCull[1], endClip, projectionBounds );
					outside |= pointCull[0] | pointCull[1];
				}
			}
		}
	}

	// if the clipped frustum extends beyond two or more boundaries of this frustum
	if ( outside != 1 && outside != 2 && outside != 4 && outside != 8 ) {

		// transform this frustum into the space of the other frustum
		transpose = frustum.axis;
		transpose.TransposeSelf();
		localOrigin1 = ( origin - frustum.origin ) * transpose;
		localAxis1 = axis * transpose;
		localAxis1[0] *= dFar;
		localAxis1[1] *= dLeft;
		localAxis1[2] *= dUp;

		// transform this frustum into the space of the clip bounds
		transpose = clipBox.GetAxis();
		transpose.TransposeSelf();
		localOrigin2 = ( origin - clipBox.GetCenter() ) * transpose;
		localAxis2 = axis * transpose;
		localAxis2[0] *= dFar;
		localAxis2[1] *= dLeft;
		localAxis2[2] *= dUp;

		clipBounds[0] = -clipBox.GetExtents();
		clipBounds[1] = clipBox.GetExtents();

		// test the outer edges of this frustum for intersection with both the other frustum and the clip bounds
		if ( (outside & 2) && (outside & 8) ) {
			frustum.LocalRayIntersection( localOrigin1, localAxis1[0] - localAxis1[1] - localAxis1[2], s1, s2 );
			if ( s1 <= s2 && s1 >= 0.0f ) {
				BoundsRayIntersection( clipBounds, localOrigin2, localAxis2[0] - localAxis2[1] - localAxis2[2], t1, t2 );
				if ( t1 <= t2 && t2 > s1 && t1 < s2 ) {
					projectionBounds.AddPoint( arcVec3( s1 * dFar, -1.0f, -1.0f ) );
					projectionBounds.AddPoint( arcVec3( s2 * dFar, -1.0f, -1.0f ) );
				}
			}
		}
		if ( (outside & 2) && (outside & 4) ) {
			frustum.LocalRayIntersection( localOrigin1, localAxis1[0] - localAxis1[1] + localAxis1[2], s1, s2 );
			if ( s1 <= s2 && s1 >= 0.0f ) {
				BoundsRayIntersection( clipBounds, localOrigin2, localAxis2[0] - localAxis2[1] + localAxis2[2], t1, t2 );
				if ( t1 <= t2 && t2 > s1 && t1 < s2 ) {
					projectionBounds.AddPoint( arcVec3( s1 * dFar, -1.0f, 1.0f ) );
					projectionBounds.AddPoint( arcVec3( s2 * dFar, -1.0f, 1.0f ) );
				}
			}
		}
		if ( (outside & 1 ) && (outside & 8) ) {
			frustum.LocalRayIntersection( localOrigin1, localAxis1[0] + localAxis1[1] - localAxis1[2], s1, s2 );
			if ( s1 <= s2 && s1 >= 0.0f ) {
				BoundsRayIntersection( clipBounds, localOrigin2, localAxis2[0] + localAxis2[1] - localAxis2[2], t1, t2 );
				if ( t1 <= t2 && t2 > s1 && t1 < s2 ) {
					projectionBounds.AddPoint( arcVec3( s1 * dFar, 1.0f, -1.0f ) );
					projectionBounds.AddPoint( arcVec3( s2 * dFar, 1.0f, -1.0f ) );
				}
			}
		}
		if ( (outside & 1 ) && (outside & 2) ) {
			frustum.LocalRayIntersection( localOrigin1, localAxis1[0] + localAxis1[1] + localAxis1[2], s1, s2 );
			if ( s1 <= s2 && s1 >= 0.0f ) {
				BoundsRayIntersection( clipBounds, localOrigin2, localAxis2[0] + localAxis2[1] + localAxis2[2], t1, t2 );
				if ( t1 <= t2 && t2 > s1 && t1 < s2 ) {
					projectionBounds.AddPoint( arcVec3( s1 * dFar, 1.0f, 1.0f ) );
					projectionBounds.AddPoint( arcVec3( s2 * dFar, 1.0f, 1.0f ) );
				}
			}
		}
	}

	return true;
}

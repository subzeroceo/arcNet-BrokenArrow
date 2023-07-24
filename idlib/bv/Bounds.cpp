#include "../precompiled.h"
#pragma hdrstop

arcBounds bounds_zero( vec3_zero, vec3_zero );
arcBounds bounds_zeroOneCube( arcVec3( 0.0f ), arcVec3( 1.0f ) );
arcBounds bounds_unitCube( arcVec3( -1.0f ), arcVec3( 1.0f ) );

/*
============
arcBounds::GetRadius
============
*/
float arcBounds::GetRadius( void ) const {
	float total = 0.0f;
	for ( int i = 0; i < 3; i++ ) {
		float b0 = ( float )arcMath::Fabs( b[0][i] );
		float b1 = ( float )arcMath::Fabs( b[1][i] );
		if ( b0 > b1 ) {
			total += b0 * b0;
		} else {
			total += b1 * b1;
		}
	}
	return arcMath::Sqrt( total );
}

/*
============
arcBounds::GetRadius
============
*/
float arcBounds::GetRadius( const arcVec3 &center ) const {
	float total = 0.0f;
	for ( int i = 0; i < 3; i++ ) {
		float b0 = ( float )arcMath::Fabs( center[i] - b[0][i] );
		float b1 = ( float )arcMath::Fabs( b[1][i] - center[i] );
		if ( b0 > b1 ) {
			total += b0 * b0;
		} else {
			total += b1 * b1;
		}
	}
	return arcMath::Sqrt( total );
}

/*
================
arcBounds::PlaneDistance
================
*/
float arcBounds::PlaneDistance( const arcPlane &plane ) const {
	arcVec3 center = ( b[0] + b[1] ) * 0.5f;

	float d1 = plane.Distance( center );
	float d2 = arcMath::Fabs( ( b[1][0] - center[0] ) * plane.Normal()[0] ) +
		arcMath::Fabs( ( b[1][1] - center[1] ) * plane.Normal()[1] ) +
		arcMath::Fabs( ( b[1][2] - center[2] ) * plane.Normal()[2] );

	if ( d1 - d2 > 0.0f ) {
		return d1 - d2;
	}
	if ( d1 + d2 < 0.0f ) {
		return d1 + d2;
	}
	return 0.0f;
}

/*
================
arcBounds::PlaneSide
================
*/
int arcBounds::PlaneSide( const arcPlane &plane, const float epsilon ) const {
	arcVec3 center = ( b[0] + b[1] ) * 0.5f;

	float d1 = plane.Distance( center );
	float d2 = arcMath::Fabs( ( b[1][0] - center[0] ) * plane.Normal()[0] ) +
			arcMath::Fabs( ( b[1][1] - center[1] ) * plane.Normal()[1] ) +
				arcMath::Fabs( ( b[1][2] - center[2] ) * plane.Normal()[2] );

	if ( d1 - d2 > epsilon ) {
		return PLANESIDE_FRONT;
	}
	if ( d1 + d2 < -epsilon ) {
		return PLANESIDE_BACK;
	}
	return PLANESIDE_CROSS;
}

/*
============
arcBounds::LineIntersection

  Returns true if the line intersects the bounds between the start and end point.
============
*/
bool arcBounds::LineIntersection( const arcVec3 &start, const arcVec3 &end ) const {
    float ld[3];
	arcVec3 center = ( b[0] + b[1] ) * 0.5f;
	arcVec3 extents = b[1] - center;
    arcVec3 lineDir = 0.5f * ( end - start );
    arcVec3 lineCenter = start + lineDir;
    arcVec3 dir = lineCenter - center;

    ld[0] = arcMath::Fabs( lineDir[0] );
	if ( arcMath::Fabs( dir[0] ) > extents[0] + ld[0] ) {
        return false;
	}

    ld[1] = arcMath::Fabs( lineDir[1] );
	if ( arcMath::Fabs( dir[1] ) > extents[1] + ld[1] ) {
        return false;
	}

    ld[2] = arcMath::Fabs( lineDir[2] );
	if ( arcMath::Fabs( dir[2] ) > extents[2] + ld[2] ) {
        return false;
	}

    arcVec3 cross = lineDir.Cross( dir );

	if ( arcMath::Fabs( cross[0] ) > extents[1] * ld[2] + extents[2] * ld[1] ) {
        return false;
	}

	if ( arcMath::Fabs( cross[1] ) > extents[0] * ld[2] + extents[2] * ld[0] ) {
        return false;
	}

	if ( arcMath::Fabs( cross[2] ) > extents[0] * ld[1] + extents[1] * ld[0] ) {
        return false;
	}

    return true;
}

/*
============
arcBounds::RayIntersection

  Returns true if the ray intersects the bounds.
  The ray can intersect the bounds in both directions from the start point.
  If start is inside the bounds it is considered an intersection with scale = 0
============
*/
bool arcBounds::RayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale ) const {
	arcVec3 hit;

	int ax0 = -1;
	int inside = 0;
	for ( int i = 0; i < 3; i++ ) {
		if ( start[i] < b[0][i] ) {
			int side = 0;
		} else if ( start[i] > b[1][i] ) {
			int side = 1;
		} else {
			int inside++;
			continue;
		}
		if ( dir[i] == 0.0f ) {
			continue;
		}
		float f = ( start[i] - b[side][i] );
		if ( ax0 < 0 || arcMath::Fabs( f ) > arcMath::Fabs( scale * dir[i] ) ) {
			scale = - ( f / dir[i] );
			ax0 = i;
		}
	}

	if ( ax0 < 0 ) {
		float scale = 0.0f;
		// return true if the start point is inside the bounds
		return ( inside == 3 );
	}

	int ax1 = ( ax0+1 )%3;
	int ax2 = ( ax0+2 )%3;
	hit[ax1] = start[ax1] + scale * dir[ax1];
	hit[ax2] = start[ax2] + scale * dir[ax2];

	return ( hit[ax1] >= b[0][ax1] && hit[ax1] <= b[1][ax1] && hit[ax2] >= b[0][ax2] && hit[ax2] <= b[1][ax2] );
}

/*
============
arcBounds::FromTransformedBounds
============
*/
void arcBounds::FromTransformedBounds( const arcBounds &bounds, const arcVec3 &origin, const arcMat3 &axis ) {
	arcVec3 center = (bounds[0] + bounds[1] ) * 0.5f;
	arcVec3 extents = bounds[1] - center;

	for ( int i = 0; i < 3; i++ ) {
		arcVec3 rotatedExtents[i] = arcMath::Fabs( extents[0] * axis[0][i] ) +
		arcMath::Fabs( extents[1] * axis[1][i] ) +
		arcMath::Fabs( extents[2] * axis[2][i] );
	}

	center = origin + center * axis;
	b[0] = center - rotatedExtents;
	b[1] = center + rotatedExtents;
}

/*
============
arcBounds::FromPoints

  Most tight bounds for a point set.
============
*/
void arcBounds::FromPoints( const arcVec3 *points, const int numPoints ) {
	SIMDProcessor->MinMax( b[0], b[1], points, numPoints );
}

/*
============
arcBounds::FromPointTranslation

  Most tight bounds for the translational movement of the given point.
============
*/
void arcBounds::FromPointTranslation( const arcVec3 &point, const arcVec3 &translation ) {
	for ( int i = 0; i < 3; i++ ) {
		if ( translation[i] < 0.0f ) {
			b[0][i] = point[i] + translation[i];
			b[1][i] = point[i];
		} else {
			b[0][i] = point[i];
			b[1][i] = point[i] + translation[i];
		}
	}
}

/*
============
arcBounds::FromBoundsTranslation

  Most tight bounds for the translational movement of the given bounds.
============
*/
void arcBounds::FromBoundsTranslation( const arcBounds &bounds, const arcVec3 &origin, const arcMat3 &axis, const arcVec3 &translation ) {
	if ( axis.IsRotated() ) {
		FromTransformedBounds( bounds, origin, axis );
	} else {
		b[0] = bounds[0] + origin;
		b[1] = bounds[1] + origin;
	}
	for ( int i = 0; i < 3; i++ ) {
		if ( translation[i] < 0.0f ) {
			b[0][i] += translation[i];
		} else {
			b[1][i] += translation[i];
		}
	}
}

/*
================
BoundsForPointRotation

  only for rotations < 180 degrees
================
*/
arcBounds BoundsForPointRotation( const arcVec3 &start, const arcRotate &rotation ) {
	arcBounds bounds;

	arcVec3 end = start * rotation;
	arcVec3 axis = rotation.GetVec();
	arcVec3 origin = rotation.GetOrigin() + axis * ( axis * ( start - rotation.GetOrigin() ) );
	float radiusSqr = ( start - origin ).LengthSqr();
	arcVec3 v1 = ( start - origin ).Cross( axis );
	arcVec3 v2 = ( end - origin ).Cross( axis );

	for ( int i = 0; i < 3; i++ ) {
		// if the derivative changes sign along this axis during the rotation from start to end
		if ( ( v1[i] > 0.0f && v2[i] < 0.0f ) || ( v1[i] < 0.0f && v2[i] > 0.0f ) ) {
			if ( ( 0.5f * (start[i] + end[i] ) - origin[i] ) > 0.0f ) {
				bounds[0][i] = Min( start[i], end[i] );
				bounds[1][i] = origin[i] + arcMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
			} else {
				bounds[0][i] = origin[i] - arcMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
				bounds[1][i] = Max( start[i], end[i] );
			}
		} else if ( start[i] > end[i] ) {
			bounds[0][i] = end[i];
			bounds[1][i] = start[i];
		} else {
			bounds[0][i] = start[i];
			bounds[1][i] = end[i];
		}
	}

	return bounds;
}

/*
============
arcBounds::FromPointRotation

  Most tight bounds for the rotational movement of the given point.
============
*/
void arcBounds::FromPointRotation( const arcVec3 &point, const arcRotate &rotation ) {
	if ( arcMath::Fabs( rotation.GetAngle() ) < 180.0f ) {
		(*this) = BoundsForPointRotation( point, rotation );
	} else {
		float radius = ( point - rotation.GetOrigin() ).Length();

		// FIXME: these bounds are usually way larger
		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );
	}
}

/*
============
arcBounds::FromBoundsRotation

  Most tight bounds for the rotational movement of the given bounds.
============
*/
void arcBounds::FromBoundsRotation( const arcBounds &bounds, const arcVec3 &origin, const arcMat3 &axis, const arcRotate &rotation ) {
	arcVec3 point;
	arcBounds rBounds;

	if ( arcMath::Fabs( rotation.GetAngle() ) < 180.0f ) {
		(*this) = BoundsForPointRotation( bounds[0] * axis + origin, rotation );
		for ( int i = 1; i < 8; i++ ) {
			point[0] = bounds[( i^( i>>1 ) )&1][0];
			point[1] = bounds[( i>>1 )&1][1];
			point[2] = bounds[( i >> 2 )&1][2];
			(*this) += BoundsForPointRotation( point * axis + origin, rotation );
		}
	} else {
		point = (bounds[1] - bounds[0] ) * 0.5f;
		float radius = (bounds[1] - point).Length() + (point - rotation.GetOrigin() ).Length();

		// FIXME: these bounds are usually way larger
		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );
	}
}

/*
============
arcBounds::ToPoints
============
*/
void arcBounds::ToPoints( arcVec3 points[8] ) const {
	for ( int i = 0; i < 8; i++ ) {
		points[i][0] = b[( i^( i>>1 ) )&1][0];
		points[i][1] = b[( i>>1 )&1][1];
		points[i][2] = b[( i >> 2 )&1][2];
	}
}

/*
================
arcBounds::ShortestDistance
================
*/
float arcBounds::ShortestDistance( const arcVec3 &point ) const {
	if ( ContainsPoint( point ) ) {
		return( 0.0f );
	}

	float distance = 0.0f;
	for ( int i = 0; i < 3; i++) {
		if ( point[i] < b[0][i] ) {
			float delta = b[0][i] - point[i];
			distance += delta * delta;
		} else if ( point[i] > b[1][i] ) {
			float delta = point[i] - b[1][i];
			distance += delta * delta;
		}
	}

	return ( arcMath::Sqrt( distance ) );
}
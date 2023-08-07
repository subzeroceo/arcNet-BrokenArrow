#include "../Lib.h"
#pragma hdrstop

anBounds bounds_zero( vec3_zero, vec3_zero );
anBounds bounds_zeroOneCube( anVec3( 0.0f ), anVec3( 1.0f ) );
anBounds bounds_unitCube( anVec3( -1.0f ), anVec3( 1.0f ) );

/*
============
anBounds::GetRadius
============
*/
float anBounds::GetRadius( void ) const {
	float total = 0.0f;
	for ( int i = 0; i < 3; i++ ) {
		float b0 = ( float )anMath::Fabs( b[0][i] );
		float b1 = ( float )anMath::Fabs( b[1][i] );
		if ( b0 > b1 ) {
			total += b0 * b0;
		} else {
			total += b1 * b1;
		}
	}
	return anMath::Sqrt( total );
}

/*
============
anBounds::GetRadius
============
*/
float anBounds::GetRadius( const anVec3 &center ) const {
	float total = 0.0f;
	for ( int i = 0; i < 3; i++ ) {
		float b0 = ( float )anMath::Fabs( center[i] - b[0][i] );
		float b1 = ( float )anMath::Fabs( b[1][i] - center[i] );
		if ( b0 > b1 ) {
			total += b0 * b0;
		} else {
			total += b1 * b1;
		}
	}
	return anMath::Sqrt( total );
}

/*
================
anBounds::PlaneDistance
================
*/
float anBounds::PlaneDistance( const anPlane &plane ) const {
	anVec3 center = ( b[0] + b[1] ) * 0.5f;

	float d1 = plane.Distance( center );
	float d2 = anMath::Fabs( ( b[1][0] - center[0] ) * plane.Normal()[0] ) +
		anMath::Fabs( ( b[1][1] - center[1] ) * plane.Normal()[1] ) +
		anMath::Fabs( ( b[1][2] - center[2] ) * plane.Normal()[2] );

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
anBounds::PlaneSide
================
*/
int anBounds::PlaneSide( const anPlane &plane, const float epsilon ) const {
	anVec3 center = ( b[0] + b[1] ) * 0.5f;

	float d1 = plane.Distance( center );
	float d2 = anMath::Fabs( ( b[1][0] - center[0] ) * plane.Normal()[0] ) +
			anMath::Fabs( ( b[1][1] - center[1] ) * plane.Normal()[1] ) +
				anMath::Fabs( ( b[1][2] - center[2] ) * plane.Normal()[2] );

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
anBounds::LineIntersection

  Returns true if the line intersects the bounds between the start and end point.
============
*/
bool anBounds::LineIntersection( const anVec3 &start, const anVec3 &end ) const {
    float ld[3];
	anVec3 center = ( b[0] + b[1] ) * 0.5f;
	anVec3 extents = b[1] - center;
    anVec3 lineDir = 0.5f * ( end - start );
    anVec3 lineCenter = start + lineDir;
    anVec3 dir = lineCenter - center;

    ld[0] = anMath::Fabs( lineDir[0] );
	if ( anMath::Fabs( dir[0] ) > extents[0] + ld[0] ) {
        return false;
	}

    ld[1] = anMath::Fabs( lineDir[1] );
	if ( anMath::Fabs( dir[1] ) > extents[1] + ld[1] ) {
        return false;
	}

    ld[2] = anMath::Fabs( lineDir[2] );
	if ( anMath::Fabs( dir[2] ) > extents[2] + ld[2] ) {
        return false;
	}

    anVec3 cross = lineDir.Cross( dir );

	if ( anMath::Fabs( cross[0] ) > extents[1] * ld[2] + extents[2] * ld[1] ) {
        return false;
	}

	if ( anMath::Fabs( cross[1] ) > extents[0] * ld[2] + extents[2] * ld[0] ) {
        return false;
	}

	if ( anMath::Fabs( cross[2] ) > extents[0] * ld[1] + extents[1] * ld[0] ) {
        return false;
	}

    return true;
}

/*
============
anBounds::RayIntersection

Returns true if the ray intersects the bounds.
The ray can intersect the bounds in both directions from the start point.
If start is inside the bounds it is considered an intersection with scale = 0
============
*/
bool anBounds::RayIntersection( const anVec3 &start, const anVec3 &dir, float &scale ) const {
	anVec3 hit;

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
		if ( ax0 < 0 || anMath::Fabs( f ) > anMath::Fabs( scale * dir[i] ) ) {
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
anBounds::FromTransformedBounds
============
*/
void anBounds::FromTransformedBounds( const anBounds &bounds, const anVec3 &origin, const anMat3 &axis ) {
	anVec3 center = (bounds[0] + bounds[1] ) * 0.5f;
	anVec3 extents = bounds[1] - center;

	for ( int i = 0; i < 3; i++ ) {
		anVec3 rotatedExtents[i] = anMath::Fabs( extents[0] * axis[0][i] ) +
		anMath::Fabs( extents[1] * axis[1][i] ) +
		anMath::Fabs( extents[2] * axis[2][i] );
	}

	center = origin + center * axis;
	b[0] = center - rotatedExtents;
	b[1] = center + rotatedExtents;
}

/*
============
anBounds::FromPoints

Most tight bounds for a point set.
============
*/
void anBounds::FromPoints( const anVec3 *points, const int numPoints ) {
	SIMDProcessor->MinMax( b[0], b[1], points, numPoints );
}

/*
============
anBounds::FromPointTranslation

Most tight bounds for the translational movement of the given point.
============
*/
void anBounds::FromPointTranslation( const anVec3 &point, const anVec3 &translation ) {
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
anBounds::FromBoundsTranslation

  Most tight bounds for the translational movement of the given bounds.
============
*/
void anBounds::FromBoundsTranslation( const anBounds &bounds, const anVec3 &origin, const anMat3 &axis, const anVec3 &translation ) {
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
anBounds BoundsForPointRotation( const anVec3 &start, const anRotation &rotation ) {
	anBounds bounds;

	anVec3 end = start * rotation;
	anVec3 axis = rotation.GetVec();
	anVec3 origin = rotation.GetOrigin() + axis * ( axis * ( start - rotation.GetOrigin() ) );
	float radiusSqr = ( start - origin ).LengthSqr();
	anVec3 v1 = ( start - origin ).Cross( axis );
	anVec3 v2 = ( end - origin ).Cross( axis );

	for ( int i = 0; i < 3; i++ ) {
		// if the derivative changes sign along this axis during the rotation from start to end
		if ( ( v1[i] > 0.0f && v2[i] < 0.0f ) || ( v1[i] < 0.0f && v2[i] > 0.0f ) ) {
			if ( ( 0.5f * ( start[i] + end[i] ) - origin[i] ) > 0.0f ) {
				bounds[0][i] = Min( start[i], end[i] );
				bounds[1][i] = origin[i] + anMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
			} else {
				bounds[0][i] = origin[i] - anMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
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
anBounds::FromPointRotation

Most tight bounds for the rotational movement of the given point.
============
*/
void anBounds::FromPointRotation( const anVec3 &point, const anRotation &rotation ) {
	if ( anMath::Fabs( rotation.GetAngle() ) < 180.0f ) {
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
anBounds::FromBoundsRotation

Most tight bounds for the rotational movement of the given bounds.
============
*/
void anBounds::FromBoundsRotation( const anBounds &bounds, const anVec3 &origin, const anMat3 &axis, const anRotation &rotation ) {
	anBounds rBounds;

	if ( anMath::Fabs( rotation.GetAngle() ) < 180.0f ) {
		(*this) = BoundsForPointRotation( bounds[0] * axis + origin, rotation );
		for ( int i = 1; i < 8; i++ ) {
			anVec3 point[0] = bounds[( i^( i>>1 ) )&1][0];
			anVec3 point[1] = bounds[( i>>1 )&1][1];
			anVec3 point[2] = bounds[( i >> 2 )&1][2];
			(*this) += BoundsForPointRotation( point * axis + origin, rotation );
		}
	} else {
		anVec3 point = ( bounds[1] - bounds[0] ) * 0.5f;
		float radius = ( bounds[1] - point ).Length() + ( point - rotation.GetOrigin() ).Length();

		// FIXME: these bounds are usually way larger
		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );
	}
}

/*
============
anBounds::ToPoints
============
*/
void anBounds::ToPoints( anVec3 points[8] ) const {
	for ( int i = 0; i < 8; i++ ) {
		points[i][0] = b[( i^( i>>1 ) )&1][0];
		points[i][1] = b[( i>>1 )&1][1];
		points[i][2] = b[( i >> 2 )&1][2];
	}
}

/*
================
anBounds::ShortestDistance
================
*/
float anBounds::ShortestDistance( const anVec3 &point ) const {
	if ( ContainsPoint( point ) ) {
		return( 0.0f );
	}

	float distance = 0.0f;
	for ( int i = 0; i < 3; i++ ) {
		if ( point[i] < b[0][i] ) {
			float delta = b[0][i] - point[i];
			distance += delta * delta;
		} else if ( point[i] > b[1][i] ) {
			float delta = point[i] - b[1][i];
			distance += delta * delta;
		}
	}

	return ( anMath::Sqrt( distance ) );
}

/*
========================
anBounds::ProjectedBounds

Calculates the bounds of the given bounding box projected with the given Model View Projection (MVP) matrix.
If 'windowSpace' is true then the calculated bounds along each axis are moved and clamped to the [0, 1] range.

The given bounding box is not clipped to the MVP so the projected bounds may not be as tight as possible.
If the given bounding box is W=0 clipped then the projected bounds will cover the full X-Y range.
Note that while projected[0][1] will be set to the minimum when the given bounding box is W=0 clipped,
projected[1][1] will still be valid and will NOT be set to the maximum when the given bounding box
is W=0 clipped.
========================
*/
void anBounds::ProjectedBounds( anBounds & projected, const anGLMatrix & mvp, const anBounds & bounds, bool windowSpace ) {
#ifdef ARC_WIN_X86_SSE2_INTRIN
	__m128 mvp0 = _mm_loadu_ps( mvp[0] );
	__m128 mvp1 = _mm_loadu_ps( mvp[1] );
	__m128 mvp2 = _mm_loadu_ps( mvp[2] );
	__m128 mvp3 = _mm_loadu_ps( mvp[3] );

	__m128 b0 = _mm_loadu_bounds_0( bounds );
	__m128 b1 = _mm_loadu_bounds_1( bounds );

	// take the four points on the X-Y plane
	__m128 vxy = _mm_unpacklo_ps( b0, b1 );						// min X, max X, min Y, max Y
	__m128 vx = _mm_perm_ps( vxy, _MM_SHUFFLE( 1, 0, 1, 0 ) );	// min X, max X, min X, max X
	__m128 vy = _mm_perm_ps( vxy, _MM_SHUFFLE( 3, 3, 2, 2 ) );	// min Y, min Y, max Y, max Y

	__m128 vz0 = _mm_splat_ps( b0, 2 );							// min Z, min Z, min Z, min Z
	__m128 vz1 = _mm_splat_ps( b1, 2 );							// max Z, max Z, max Z, max Z

	// compute four partial X,Y,Z,W values
	__m128 parx = _mm_splat_ps( mvp0, 3 );
	__m128 pary = _mm_splat_ps( mvp1, 3 );
	__m128 parz = _mm_splat_ps( mvp2, 3 );
	__m128 parw = _mm_splat_ps( mvp3, 3 );

	parx = _mm_madd_ps( vx, _mm_splat_ps( mvp0, 0 ), parx );
	pary = _mm_madd_ps( vx, _mm_splat_ps( mvp1, 0 ), pary );
	parz = _mm_madd_ps( vx, _mm_splat_ps( mvp2, 0 ), parz );
	parw = _mm_madd_ps( vx, _mm_splat_ps( mvp3, 0 ), parw );

	parx = _mm_madd_ps( vy, _mm_splat_ps( mvp0, 1 ), parx );
	pary = _mm_madd_ps( vy, _mm_splat_ps( mvp1, 1 ), pary );
	parz = _mm_madd_ps( vy, _mm_splat_ps( mvp2, 1 ), parz );
	parw = _mm_madd_ps( vy, _mm_splat_ps( mvp3, 1 ), parw );

	// compute full X,Y,Z,W values
	__m128 mvp0Z = _mm_splat_ps( mvp0, 2 );
	__m128 mvp1Z = _mm_splat_ps( mvp1, 2 );
	__m128 mvp2Z = _mm_splat_ps( mvp2, 2 );
	__m128 mvp3Z = _mm_splat_ps( mvp3, 2 );

	__m128 x0 = _mm_madd_ps( vz0, mvp0Z, parx );
	__m128 y0 = _mm_madd_ps( vz0, mvp1Z, pary );
	__m128 z0 = _mm_madd_ps( vz0, mvp2Z, parz );
	__m128 w0 = _mm_madd_ps( vz0, mvp3Z, parw );

	__m128 x1 = _mm_madd_ps( vz1, mvp0Z, parx );
	__m128 y1 = _mm_madd_ps( vz1, mvp1Z, pary );
	__m128 z1 = _mm_madd_ps( vz1, mvp2Z, parz );
	__m128 w1 = _mm_madd_ps( vz1, mvp3Z, parw );

	__m128 s0 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w0 );
	__m128 s1 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w1 );

	w0 = _mm_sel_ps( w0, vector_float_one, s0 );
	w1 = _mm_sel_ps( w1, vector_float_one, s1 );

	__m128 rw0 = _mm_rcp32_ps( w0 );
	__m128 rw1 = _mm_rcp32_ps( w1 );

	x0 = _mm_mul_ps( x0, rw0 );
	y0 = _mm_mul_ps( y0, rw0 );
	z0 = _mm_mul_ps( z0, rw0 );

	x1 = _mm_mul_ps( x1, rw1 );
	y1 = _mm_mul_ps( y1, rw1 );
	z1 = _mm_mul_ps( z1, rw1 );

	__m128 minX = _mm_min_ps( x0, x1 );
	__m128 minY = _mm_min_ps( y0, y1 );
	__m128 minZ = _mm_min_ps( z0, z1 );

	__m128 maxX = _mm_max_ps( x0, x1 );
	__m128 maxY = _mm_max_ps( y0, y1 );
	__m128 maxZ = _mm_max_ps( z0, z1 );

	minX = _mm_min_ps( minX, _mm_perm_ps( minX, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	minY = _mm_min_ps( minY, _mm_perm_ps( minY, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	minZ = _mm_min_ps( minZ, _mm_perm_ps( minZ, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

	minX = _mm_min_ps( minX, _mm_perm_ps( minX, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	minY = _mm_min_ps( minY, _mm_perm_ps( minY, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	minZ = _mm_min_ps( minZ, _mm_perm_ps( minZ, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	maxX = _mm_max_ps( maxX, _mm_perm_ps( maxX, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	maxY = _mm_max_ps( maxY, _mm_perm_ps( maxY, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	maxZ = _mm_max_ps( maxZ, _mm_perm_ps( maxZ, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

	maxX = _mm_max_ps( maxX, _mm_perm_ps( maxX, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	maxY = _mm_max_ps( maxY, _mm_perm_ps( maxY, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	maxZ = _mm_max_ps( maxZ, _mm_perm_ps( maxZ, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	s0 = _mm_or_ps( s0, s1 );
	s0 = _mm_or_ps( s0, _mm_perm_ps( s0, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	s0 = _mm_or_ps( s0, _mm_perm_ps( s0, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	minX = _mm_sel_ps( minX, vector_float_neg_infinity, s0 );
	minY = _mm_sel_ps( minY, vector_float_neg_infinity, s0 );
	minZ = _mm_sel_ps( minZ, vector_float_neg_infinity, s0 );

	maxX = _mm_sel_ps( maxX, vector_float_pos_infinity, s0 );
	maxY = _mm_sel_ps( maxY, vector_float_pos_infinity, s0 );
	// NOTE: maxZ is valid either way

	if ( windowSpace ) {
		minX = _mm_madd_ps( minX, vector_float_half, vector_float_half );
		maxX = _mm_madd_ps( maxX, vector_float_half, vector_float_half );

		minY = _mm_madd_ps( minY, vector_float_half, vector_float_half );
		maxY = _mm_madd_ps( maxY, vector_float_half, vector_float_half );

#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		minZ = _mm_madd_ps( minZ, vector_float_half, vector_float_half );
		maxZ = _mm_madd_ps( maxZ, vector_float_half, vector_float_half );
#endif

		minX = _mm_max_ps( _mm_min_ps( minX, vector_float_one ), vector_float_zero );
		maxX = _mm_max_ps( _mm_min_ps( maxX, vector_float_one ), vector_float_zero );

		minY = _mm_max_ps( _mm_min_ps( minY, vector_float_one ), vector_float_zero );
		maxY = _mm_max_ps( _mm_min_ps( maxY, vector_float_one ), vector_float_zero );

		minZ = _mm_max_ps( _mm_min_ps( minZ, vector_float_one ), vector_float_zero );
		maxZ = _mm_max_ps( _mm_min_ps( maxZ, vector_float_one ), vector_float_zero );
	}

	_mm_store_ss( & projected[0].x, minX );
	_mm_store_ss( & projected[0].y, minY );
	_mm_store_ss( & projected[0].z, minZ );

	_mm_store_ss( & projected[1].x, maxX );
	_mm_store_ss( & projected[1].y, maxY );
	_mm_store_ss( & projected[1].z, maxZ );
#else
	for ( int i = 0; i < 3; i++ ) {
		projected[0][i] = RENDER_MATRIX_INFINITY;
		projected[1][i] = - RENDER_MATRIX_INFINITY;
	}

	anVec3 v;
	for ( int x = 0; x < 2; x++ ) {
		v[0] = bounds[x][0];
		for ( int y = 0; y < 2; y++ ) {
			v[1] = bounds[y][1];
			for ( int z = 0; z < 2; z++ ) {
				v[2] = bounds[z][2];

				float tx = v[0] * mvp[0][0] + v[1] * mvp[0][1] + v[2] * mvp[0][2] + mvp[0][3];
				float ty = v[0] * mvp[1][0] + v[1] * mvp[1][1] + v[2] * mvp[1][2] + mvp[1][3];
				float tz = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
				float tw = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];

				if ( tw <= anMath::FLT_SMALLEST_NON_DENORMAL ) {
					projected[0][0] = -RENDER_MATRIX_INFINITY;
					projected[0][1] = -RENDER_MATRIX_INFINITY;
					projected[0][2] = -RENDER_MATRIX_INFINITY;
					projected[1][0] = RENDER_MATRIX_INFINITY;
					projected[1][1] = RENDER_MATRIX_INFINITY;
					// NOTE: projected[1][1] is still valid
					continue;
				}

				float rw = 1.0f / tw;

				tx = tx * rw;
				ty = ty * rw;
				tz = tz * rw;

				projected[0][0] = Min( projected[0][0], tx );
				projected[0][1] = Min( projected[0][1], ty );
				projected[0][2] = Min( projected[0][2], tz );

				projected[1][0] = Max( projected[1][0], tx );
				projected[1][1] = Max( projected[1][1], ty );
				projected[1][2] = Max( projected[1][2], tz );
			}
		}
	}

	if ( windowSpace ) {
		// convert to window coords
		projected[0][0] = projected[0][0] * 0.5f + 0.5f;
		projected[1][0] = projected[1][0] * 0.5f + 0.5f;

		projected[0][1] = projected[0][1] * 0.5f + 0.5f;
		projected[1][1] = projected[1][1] * 0.5f + 0.5f;

#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		projected[0][2] = projected[0][2] * 0.5f + 0.5f;
		projected[1][2] = projected[1][2] * 0.5f + 0.5f;
#endif

		// clamp to [0, 1] range
		projected[0][0] = anMath::ClampFloat( 0.0f, 1.0f, projected[0][0] );
		projected[1][0] = anMath::ClampFloat( 0.0f, 1.0f, projected[1][0] );

		projected[0][1] = anMath::ClampFloat( 0.0f, 1.0f, projected[0][1] );
		projected[1][1] = anMath::ClampFloat( 0.0f, 1.0f, projected[1][1] );

		projected[0][2] = anMath::ClampFloat( 0.0f, 1.0f, projected[0][2] );
		projected[1][2] = anMath::ClampFloat( 0.0f, 1.0f, projected[1][2] );
	}

#endif
}

/*
========================
anBounds::ProjectedNearClippedBounds

Calculates the bounds of the given bounding box projected with the given Model View Projection (MVP) matrix.
If 'windowSpace' is true then the calculated bounds along each axis are moved and clamped to the [0, 1] range.

The given bounding box is first near clipped so the projected bounds do not cover the full X-Y range when
the given bounding box crosses the W=0 plane. However, the given bounding box is not clipped against the
other planes so the projected bounds are still not as tight as they could be if the given bounding box
crosses a corner. Fortunately, clipping to the near clipping planes typically provides more than 50% of
the gain between not clipping at all and fully clipping the bounding box to all planes. Only clipping to
the near clipping plane is much cheaper than clipping to all planes and can be easily implemented with
completely branchless SIMD.
========================
*/
void anBounds::ProjectedNearClippedBounds( anBounds & projected, const anGLMatrix & mvp, const anBounds & bounds, bool windowSpace ) {
#ifdef ARC_WIN_X86_SSE2_INTRIN
	const __m128 mvp0 = _mm_loadu_ps( mvp[0] );
	const __m128 mvp1 = _mm_loadu_ps( mvp[1] );
	const __m128 mvp2 = _mm_loadu_ps( mvp[2] );
	const __m128 mvp3 = _mm_loadu_ps( mvp[3] );

	const __m128 b0 = _mm_loadu_bounds_0( bounds );
	const __m128 b1 = _mm_loadu_bounds_1( bounds );

	// take the four points on the X-Y plane
	const __m128 vxy = _mm_unpacklo_ps( b0, b1 );						// min X, max X, min Y, max Y
	const __m128 vx = _mm_perm_ps( vxy, _MM_SHUFFLE( 1, 0, 1, 0 ) );	// min X, max X, min X, max X
	const __m128 vy = _mm_perm_ps( vxy, _MM_SHUFFLE( 3, 3, 2, 2 ) );	// min Y, min Y, max Y, max Y

	const __m128 vz0 = _mm_splat_ps( b0, 2 );							// min Z, min Z, min Z, min Z
	const __m128 vz1 = _mm_splat_ps( b1, 2 );							// max Z, max Z, max Z, max Z

	// compute four partial X,Y,Z,W values
	__m128 parx = _mm_splat_ps( mvp0, 3 );
	__m128 pary = _mm_splat_ps( mvp1, 3 );
	__m128 parz = _mm_splat_ps( mvp2, 3 );
	__m128 parw = _mm_splat_ps( mvp3, 3 );

	parx = _mm_madd_ps( vx, _mm_splat_ps( mvp0, 0 ), parx );
	pary = _mm_madd_ps( vx, _mm_splat_ps( mvp1, 0 ), pary );
	parz = _mm_madd_ps( vx, _mm_splat_ps( mvp2, 0 ), parz );
	parw = _mm_madd_ps( vx, _mm_splat_ps( mvp3, 0 ), parw );

	parx = _mm_madd_ps( vy, _mm_splat_ps( mvp0, 1 ), parx );
	pary = _mm_madd_ps( vy, _mm_splat_ps( mvp1, 1 ), pary );
	parz = _mm_madd_ps( vy, _mm_splat_ps( mvp2, 1 ), parz );
	parw = _mm_madd_ps( vy, _mm_splat_ps( mvp3, 1 ), parw );

	// compute full X,Y,Z,W values
	const __m128 mvp0Z = _mm_splat_ps( mvp0, 2 );
	const __m128 mvp1Z = _mm_splat_ps( mvp1, 2 );
	const __m128 mvp2Z = _mm_splat_ps( mvp2, 2 );
	const __m128 mvp3Z = _mm_splat_ps( mvp3, 2 );

	const __m128 x_0123 = _mm_madd_ps( vz0, mvp0Z, parx );
	const __m128 y_0123 = _mm_madd_ps( vz0, mvp1Z, pary );
	const __m128 z_0123 = _mm_madd_ps( vz0, mvp2Z, parz );
	const __m128 w_0123 = _mm_madd_ps( vz0, mvp3Z, parw );

	const __m128 x_4567 = _mm_madd_ps( vz1, mvp0Z, parx );
	const __m128 y_4567 = _mm_madd_ps( vz1, mvp1Z, pary );
	const __m128 z_4567 = _mm_madd_ps( vz1, mvp2Z, parz );
	const __m128 w_4567 = _mm_madd_ps( vz1, mvp3Z, parw );

	// rotate the X,Y,Z,W values up by one
	const __m128 x_1230 = _mm_perm_ps( x_0123, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 y_1230 = _mm_perm_ps( y_0123, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 z_1230 = _mm_perm_ps( z_0123, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 w_1230 = _mm_perm_ps( w_0123, _MM_SHUFFLE( 0, 3, 2, 1 ) );

	const __m128 x_5674 = _mm_perm_ps( x_4567, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 y_5674 = _mm_perm_ps( y_4567, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 z_5674 = _mm_perm_ps( z_4567, _MM_SHUFFLE( 0, 3, 2, 1 ) );
	const __m128 w_5674 = _mm_perm_ps( w_4567, _MM_SHUFFLE( 0, 3, 2, 1 ) );

#if defined( CLIP_SPACE_D3D )	// the D3D near plane is at Z=0 instead of Z=-1
	const __m128 d_0123 = z_0123;
	const __m128 d_4567 = z_4567;
	const __m128 d_1230 = z_1230;
	const __m128 d_5674 = z_5674;
#else
	const __m128 d_0123 = _mm_add_ps( z_0123, w_0123 );
	const __m128 d_4567 = _mm_add_ps( z_4567, w_4567 );
	const __m128 d_1230 = _mm_add_ps( z_1230, w_1230 );
	const __m128 d_5674 = _mm_add_ps( z_5674, w_5674 );
#endif

	const __m128 deltaABCD = _mm_sub_ps( d_0123, d_1230 );
	const __m128 deltaEFGH = _mm_sub_ps( d_4567, d_5674 );
	const __m128 deltaIJKL = _mm_sub_ps( d_0123, d_4567 );

	const __m128 maskABCD = _mm_cmpgt_ps( _mm_and_ps( deltaABCD, vector_float_abs_mask ), vector_float_smallest_non_denorm );
	const __m128 maskEFGH = _mm_cmpgt_ps( _mm_and_ps( deltaEFGH, vector_float_abs_mask ), vector_float_smallest_non_denorm );
	const __m128 maskIJKL = _mm_cmpgt_ps( _mm_and_ps( deltaIJKL, vector_float_abs_mask ), vector_float_smallest_non_denorm );

	const __m128 fractionABCD = _mm_and_ps( _mm_div32_ps( d_0123, _mm_sel_ps( vector_float_one, deltaABCD, maskABCD ) ), maskABCD );
	const __m128 fractionEFGH = _mm_and_ps( _mm_div32_ps( d_4567, _mm_sel_ps( vector_float_one, deltaEFGH, maskEFGH ) ), maskEFGH );
	const __m128 fractionIJKL = _mm_and_ps( _mm_div32_ps( d_0123, _mm_sel_ps( vector_float_one, deltaIJKL, maskIJKL ) ), maskIJKL );

	const __m128 clipABCD = _mm_and_ps( _mm_cmpgt_ps( fractionABCD, vector_float_zero ), _mm_cmpgt_ps( vector_float_one, fractionABCD ) );
	const __m128 clipEFGH = _mm_and_ps( _mm_cmpgt_ps( fractionEFGH, vector_float_zero ), _mm_cmpgt_ps( vector_float_one, fractionEFGH ) );
	const __m128 clipIJKL = _mm_and_ps( _mm_cmpgt_ps( fractionIJKL, vector_float_zero ), _mm_cmpgt_ps( vector_float_one, fractionIJKL ) );

	const __m128 intersectionABCD_x = _mm_madd_ps( fractionABCD, _mm_sub_ps( x_1230, x_0123 ), x_0123 );
	const __m128 intersectionABCD_y = _mm_madd_ps( fractionABCD, _mm_sub_ps( y_1230, y_0123 ), y_0123 );
	const __m128 intersectionABCD_z = _mm_madd_ps( fractionABCD, _mm_sub_ps( z_1230, z_0123 ), z_0123 );
	const __m128 intersectionABCD_w = _mm_madd_ps( fractionABCD, _mm_sub_ps( w_1230, w_0123 ), w_0123 );

	const __m128 intersectionEFGH_x = _mm_madd_ps( fractionEFGH, _mm_sub_ps( x_5674, x_4567 ), x_4567 );
	const __m128 intersectionEFGH_y = _mm_madd_ps( fractionEFGH, _mm_sub_ps( y_5674, y_4567 ), y_4567 );
	const __m128 intersectionEFGH_z = _mm_madd_ps( fractionEFGH, _mm_sub_ps( z_5674, z_4567 ), z_4567 );
	const __m128 intersectionEFGH_w = _mm_madd_ps( fractionEFGH, _mm_sub_ps( w_5674, w_4567 ), w_4567 );

	const __m128 intersectionIJKL_x = _mm_madd_ps( fractionIJKL, _mm_sub_ps( x_4567, x_0123 ), x_0123 );
	const __m128 intersectionIJKL_y = _mm_madd_ps( fractionIJKL, _mm_sub_ps( y_4567, y_0123 ), y_0123 );
	const __m128 intersectionIJKL_z = _mm_madd_ps( fractionIJKL, _mm_sub_ps( z_4567, z_0123 ), z_0123 );
	const __m128 intersectionIJKL_w = _mm_madd_ps( fractionIJKL, _mm_sub_ps( w_4567, w_0123 ), w_0123 );

	const __m128 mask_0123 = _mm_cmpgt_ps( vector_float_zero, d_0123 );
	const __m128 mask_1230 = _mm_cmpgt_ps( vector_float_zero, d_1230 );
	const __m128 mask_4567 = _mm_cmpgt_ps( vector_float_zero, d_4567 );
	const __m128 mask_5674 = _mm_cmpgt_ps( vector_float_zero, d_5674 );

	const __m128 maskABCD_0123 = _mm_and_ps( clipABCD, mask_0123 );
	const __m128 maskABCD_1230 = _mm_and_ps( clipABCD, mask_1230 );
	const __m128 maskEFGH_4567 = _mm_and_ps( clipEFGH, mask_4567 );
	const __m128 maskEFGH_5674 = _mm_and_ps( clipEFGH, mask_5674 );
	const __m128 maskIJKL_0123 = _mm_and_ps( clipIJKL, mask_0123 );
	const __m128 maskIJKL_4567 = _mm_and_ps( clipIJKL, mask_4567 );

	__m128 edgeVertsABCD_x0 = _mm_sel_ps( x_0123, intersectionABCD_x, maskABCD_0123 );
	__m128 edgeVertsABCD_y0 = _mm_sel_ps( y_0123, intersectionABCD_y, maskABCD_0123 );
	__m128 edgeVertsABCD_z0 = _mm_sel_ps( z_0123, intersectionABCD_z, maskABCD_0123 );
	__m128 edgeVertsABCD_w0 = _mm_sel_ps( w_0123, intersectionABCD_w, maskABCD_0123 );

	__m128 edgeVertsABCD_x1 = _mm_sel_ps( x_1230, intersectionABCD_x, maskABCD_1230 );
	__m128 edgeVertsABCD_y1 = _mm_sel_ps( y_1230, intersectionABCD_y, maskABCD_1230 );
	__m128 edgeVertsABCD_z1 = _mm_sel_ps( z_1230, intersectionABCD_z, maskABCD_1230 );
	__m128 edgeVertsABCD_w1 = _mm_sel_ps( w_1230, intersectionABCD_w, maskABCD_1230 );

	__m128 edgeVertsEFGH_x0 = _mm_sel_ps( x_4567, intersectionEFGH_x, maskEFGH_4567 );
	__m128 edgeVertsEFGH_y0 = _mm_sel_ps( y_4567, intersectionEFGH_y, maskEFGH_4567 );
	__m128 edgeVertsEFGH_z0 = _mm_sel_ps( z_4567, intersectionEFGH_z, maskEFGH_4567 );
	__m128 edgeVertsEFGH_w0 = _mm_sel_ps( w_4567, intersectionEFGH_w, maskEFGH_4567 );

	__m128 edgeVertsEFGH_x1 = _mm_sel_ps( x_5674, intersectionEFGH_x, maskEFGH_5674 );
	__m128 edgeVertsEFGH_y1 = _mm_sel_ps( y_5674, intersectionEFGH_y, maskEFGH_5674 );
	__m128 edgeVertsEFGH_z1 = _mm_sel_ps( z_5674, intersectionEFGH_z, maskEFGH_5674 );
	__m128 edgeVertsEFGH_w1 = _mm_sel_ps( w_5674, intersectionEFGH_w, maskEFGH_5674 );

	__m128 edgeVertsIJKL_x0 = _mm_sel_ps( x_0123, intersectionIJKL_x, maskIJKL_0123 );
	__m128 edgeVertsIJKL_y0 = _mm_sel_ps( y_0123, intersectionIJKL_y, maskIJKL_0123 );
	__m128 edgeVertsIJKL_z0 = _mm_sel_ps( z_0123, intersectionIJKL_z, maskIJKL_0123 );
	__m128 edgeVertsIJKL_w0 = _mm_sel_ps( w_0123, intersectionIJKL_w, maskIJKL_0123 );

	__m128 edgeVertsIJKL_x1 = _mm_sel_ps( x_4567, intersectionIJKL_x, maskIJKL_4567 );
	__m128 edgeVertsIJKL_y1 = _mm_sel_ps( y_4567, intersectionIJKL_y, maskIJKL_4567 );
	__m128 edgeVertsIJKL_z1 = _mm_sel_ps( z_4567, intersectionIJKL_z, maskIJKL_4567 );
	__m128 edgeVertsIJKL_w1 = _mm_sel_ps( w_4567, intersectionIJKL_w, maskIJKL_4567 );

	const __m128 maskABCD_w0 = _mm_cmpgt_ps( edgeVertsABCD_w0, vector_float_smallest_non_denorm );
	const __m128 maskABCD_w1 = _mm_cmpgt_ps( edgeVertsABCD_w1, vector_float_smallest_non_denorm );
	const __m128 maskEFGH_w0 = _mm_cmpgt_ps( edgeVertsEFGH_w0, vector_float_smallest_non_denorm );
	const __m128 maskEFGH_w1 = _mm_cmpgt_ps( edgeVertsEFGH_w1, vector_float_smallest_non_denorm );
	const __m128 maskIJKL_w0 = _mm_cmpgt_ps( edgeVertsIJKL_w0, vector_float_smallest_non_denorm );
	const __m128 maskIJKL_w1 = _mm_cmpgt_ps( edgeVertsIJKL_w1, vector_float_smallest_non_denorm );

	edgeVertsABCD_w0 = _mm_rcp32_ps( _mm_sel_ps( vector_float_one, edgeVertsABCD_w0, maskABCD_w0 ) );
	edgeVertsABCD_w1 = _mm_rcp32_ps( _mm_sel_ps( vector_float_one, edgeVertsABCD_w1, maskABCD_w1 ) );
	edgeVertsEFGH_w0 = _mm_rcp32_ps( _mm_sel_ps( vector_float_one, edgeVertsEFGH_w0, maskEFGH_w0 ) );
	edgeVertsEFGH_w1 = _mm_rcp32_ps( _mm_sel_ps( vector_float_one, edgeVertsEFGH_w1, maskEFGH_w1 ) );
	edgeVertsIJKL_w0 = _mm_rcp32_ps( _mm_sel_ps( vector_float_one, edgeVertsIJKL_w0, maskIJKL_w0 ) );
	edgeVertsIJKL_w1 = _mm_rcp32_ps( _mm_sel_ps( vector_float_one, edgeVertsIJKL_w1, maskIJKL_w1 ) );

	edgeVertsABCD_x0 = _mm_mul_ps( edgeVertsABCD_x0, edgeVertsABCD_w0 );
	edgeVertsABCD_x1 = _mm_mul_ps( edgeVertsABCD_x1, edgeVertsABCD_w1 );
	edgeVertsEFGH_x0 = _mm_mul_ps( edgeVertsEFGH_x0, edgeVertsEFGH_w0 );
	edgeVertsEFGH_x1 = _mm_mul_ps( edgeVertsEFGH_x1, edgeVertsEFGH_w1 );
	edgeVertsIJKL_x0 = _mm_mul_ps( edgeVertsIJKL_x0, edgeVertsIJKL_w0 );
	edgeVertsIJKL_x1 = _mm_mul_ps( edgeVertsIJKL_x1, edgeVertsIJKL_w1 );

	edgeVertsABCD_y0 = _mm_mul_ps( edgeVertsABCD_y0, edgeVertsABCD_w0 );
	edgeVertsABCD_y1 = _mm_mul_ps( edgeVertsABCD_y1, edgeVertsABCD_w1 );
	edgeVertsEFGH_y0 = _mm_mul_ps( edgeVertsEFGH_y0, edgeVertsEFGH_w0 );
	edgeVertsEFGH_y1 = _mm_mul_ps( edgeVertsEFGH_y1, edgeVertsEFGH_w1 );
	edgeVertsIJKL_y0 = _mm_mul_ps( edgeVertsIJKL_y0, edgeVertsIJKL_w0 );
	edgeVertsIJKL_y1 = _mm_mul_ps( edgeVertsIJKL_y1, edgeVertsIJKL_w1 );

	edgeVertsABCD_z0 = _mm_mul_ps( edgeVertsABCD_z0, edgeVertsABCD_w0 );
	edgeVertsABCD_z1 = _mm_mul_ps( edgeVertsABCD_z1, edgeVertsABCD_w1 );
	edgeVertsEFGH_z0 = _mm_mul_ps( edgeVertsEFGH_z0, edgeVertsEFGH_w0 );
	edgeVertsEFGH_z1 = _mm_mul_ps( edgeVertsEFGH_z1, edgeVertsEFGH_w1 );
	edgeVertsIJKL_z0 = _mm_mul_ps( edgeVertsIJKL_z0, edgeVertsIJKL_w0 );
	edgeVertsIJKL_z1 = _mm_mul_ps( edgeVertsIJKL_z1, edgeVertsIJKL_w1 );

	const __m128 posInf = vector_float_pos_infinity;
	const __m128 negInf = vector_float_neg_infinity;

	const __m128 minX0 = _mm_min_ps( _mm_sel_ps( posInf, edgeVertsABCD_x0, maskABCD_w0 ), _mm_sel_ps( posInf, edgeVertsABCD_x1, maskABCD_w1 ) );
	const __m128 minX1 = _mm_min_ps( _mm_sel_ps( posInf, edgeVertsEFGH_x0, maskEFGH_w0 ), _mm_sel_ps( posInf, edgeVertsEFGH_x1, maskEFGH_w1 ) );
	const __m128 minX2 = _mm_min_ps( _mm_sel_ps( posInf, edgeVertsIJKL_x0, maskIJKL_w0 ), _mm_sel_ps( posInf, edgeVertsIJKL_x1, maskIJKL_w1 ) );

	const __m128 minY0 = _mm_min_ps( _mm_sel_ps( posInf, edgeVertsABCD_y0, maskABCD_w0 ), _mm_sel_ps( posInf, edgeVertsABCD_y1, maskABCD_w1 ) );
	const __m128 minY1 = _mm_min_ps( _mm_sel_ps( posInf, edgeVertsEFGH_y0, maskEFGH_w0 ), _mm_sel_ps( posInf, edgeVertsEFGH_y1, maskEFGH_w1 ) );
	const __m128 minY2 = _mm_min_ps( _mm_sel_ps( posInf, edgeVertsIJKL_y0, maskIJKL_w0 ), _mm_sel_ps( posInf, edgeVertsIJKL_y1, maskIJKL_w1 ) );

	const __m128 minZ0 = _mm_min_ps( _mm_sel_ps( posInf, edgeVertsABCD_z0, maskABCD_w0 ), _mm_sel_ps( posInf, edgeVertsABCD_z1, maskABCD_w1 ) );
	const __m128 minZ1 = _mm_min_ps( _mm_sel_ps( posInf, edgeVertsEFGH_z0, maskEFGH_w0 ), _mm_sel_ps( posInf, edgeVertsEFGH_z1, maskEFGH_w1 ) );
	const __m128 minZ2 = _mm_min_ps( _mm_sel_ps( posInf, edgeVertsIJKL_z0, maskIJKL_w0 ), _mm_sel_ps( posInf, edgeVertsIJKL_z1, maskIJKL_w1 ) );

	const __m128 maxX0 = _mm_max_ps( _mm_sel_ps( negInf, edgeVertsABCD_x0, maskABCD_w0 ), _mm_sel_ps( negInf, edgeVertsABCD_x1, maskABCD_w1 ) );
	const __m128 maxX1 = _mm_max_ps( _mm_sel_ps( negInf, edgeVertsEFGH_x0, maskEFGH_w0 ), _mm_sel_ps( negInf, edgeVertsEFGH_x1, maskEFGH_w1 ) );
	const __m128 maxX2 = _mm_max_ps( _mm_sel_ps( negInf, edgeVertsIJKL_x0, maskIJKL_w0 ), _mm_sel_ps( negInf, edgeVertsIJKL_x1, maskIJKL_w1 ) );

	const __m128 maxY0 = _mm_max_ps( _mm_sel_ps( negInf, edgeVertsABCD_y0, maskABCD_w0 ), _mm_sel_ps( negInf, edgeVertsABCD_y1, maskABCD_w1 ) );
	const __m128 maxY1 = _mm_max_ps( _mm_sel_ps( negInf, edgeVertsEFGH_y0, maskEFGH_w0 ), _mm_sel_ps( negInf, edgeVertsEFGH_y1, maskEFGH_w1 ) );
	const __m128 maxY2 = _mm_max_ps( _mm_sel_ps( negInf, edgeVertsIJKL_y0, maskIJKL_w0 ), _mm_sel_ps( negInf, edgeVertsIJKL_y1, maskIJKL_w1 ) );

	const __m128 maxZ0 = _mm_max_ps( _mm_sel_ps( negInf, edgeVertsABCD_z0, maskABCD_w0 ), _mm_sel_ps( negInf, edgeVertsABCD_z1, maskABCD_w1 ) );
	const __m128 maxZ1 = _mm_max_ps( _mm_sel_ps( negInf, edgeVertsEFGH_z0, maskEFGH_w0 ), _mm_sel_ps( negInf, edgeVertsEFGH_z1, maskEFGH_w1 ) );
	const __m128 maxZ2 = _mm_max_ps( _mm_sel_ps( negInf, edgeVertsIJKL_z0, maskIJKL_w0 ), _mm_sel_ps( negInf, edgeVertsIJKL_z1, maskIJKL_w1 ) );

	__m128 minX = _mm_min_ps( minX0, _mm_min_ps( minX1, minX2 ) );
	__m128 minY = _mm_min_ps( minY0, _mm_min_ps( minY1, minY2 ) );
	__m128 minZ = _mm_min_ps( minZ0, _mm_min_ps( minZ1, minZ2 ) );

	__m128 maxX = _mm_max_ps( maxX0, _mm_max_ps( maxX1, maxX2 ) );
	__m128 maxY = _mm_max_ps( maxY0, _mm_max_ps( maxY1, maxY2 ) );
	__m128 maxZ = _mm_max_ps( maxZ0, _mm_max_ps( maxZ1, maxZ2 ) );

	minX = _mm_min_ps( minX, _mm_perm_ps( minX, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	minY = _mm_min_ps( minY, _mm_perm_ps( minY, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	minZ = _mm_min_ps( minZ, _mm_perm_ps( minZ, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

	minX = _mm_min_ps( minX, _mm_perm_ps( minX, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	minY = _mm_min_ps( minY, _mm_perm_ps( minY, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	minZ = _mm_min_ps( minZ, _mm_perm_ps( minZ, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	maxX = _mm_max_ps( maxX, _mm_perm_ps( maxX, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	maxY = _mm_max_ps( maxY, _mm_perm_ps( maxY, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	maxZ = _mm_max_ps( maxZ, _mm_perm_ps( maxZ, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );

	maxX = _mm_max_ps( maxX, _mm_perm_ps( maxX, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	maxY = _mm_max_ps( maxY, _mm_perm_ps( maxY, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
	maxZ = _mm_max_ps( maxZ, _mm_perm_ps( maxZ, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	if ( windowSpace ) {
		minX = _mm_madd_ps( minX, vector_float_half, vector_float_half );
		maxX = _mm_madd_ps( maxX, vector_float_half, vector_float_half );

		minY = _mm_madd_ps( minY, vector_float_half, vector_float_half );
		maxY = _mm_madd_ps( maxY, vector_float_half, vector_float_half );

#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		minZ = _mm_madd_ps( minZ, vector_float_half, vector_float_half );
		maxZ = _mm_madd_ps( maxZ, vector_float_half, vector_float_half );
#endif

		minX = _mm_max_ps( _mm_min_ps( minX, vector_float_one ), vector_float_zero );
		maxX = _mm_max_ps( _mm_min_ps( maxX, vector_float_one ), vector_float_zero );

		minY = _mm_max_ps( _mm_min_ps( minY, vector_float_one ), vector_float_zero );
		maxY = _mm_max_ps( _mm_min_ps( maxY, vector_float_one ), vector_float_zero );

		minZ = _mm_max_ps( _mm_min_ps( minZ, vector_float_one ), vector_float_zero );
		maxZ = _mm_max_ps( _mm_min_ps( maxZ, vector_float_one ), vector_float_zero );
	}

	_mm_store_ss( & projected[0].x, minX );
	_mm_store_ss( & projected[0].y, minY );
	_mm_store_ss( & projected[0].z, minZ );

	_mm_store_ss( & projected[1].x, maxX );
	_mm_store_ss( & projected[1].y, maxY );
	_mm_store_ss( & projected[1].z, maxZ );
#elif 1
{
	const anVec3 points[8] = {
		anVec3( bounds[0][0], bounds[0][1], bounds[0][2] ),
		anVec3( bounds[1][0], bounds[0][1], bounds[0][2] ),
		anVec3( bounds[1][0], bounds[1][1], bounds[0][2] ),
		anVec3( bounds[0][0], bounds[1][1], bounds[0][2] ),
		anVec3( bounds[0][0], bounds[0][1], bounds[1][2] ),
		anVec3( bounds[1][0], bounds[0][1], bounds[1][2] ),
		anVec3( bounds[1][0], bounds[1][1], bounds[1][2] ),
		anVec3( bounds[0][0], bounds[1][1], bounds[1][2] )
	};

	anVec4 projectedPoints[8];
	for ( int i = 0; i < 8; i++ ) {
		const anVec3 & v = points[i];
		projectedPoints[i].x = v[0] * mvp[0][0] + v[1] * mvp[0][1] + v[2] * mvp[0][2] + mvp[0][3];
		projectedPoints[i].y = v[0] * mvp[1][0] + v[1] * mvp[1][1] + v[2] * mvp[1][2] + mvp[1][3];
		projectedPoints[i].z = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
		projectedPoints[i].w = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];
	}

	const anVec4 & p0 = projectedPoints[0];
	const anVec4 & p1 = projectedPoints[1];
	const anVec4 & p2 = projectedPoints[2];
	const anVec4 & p3 = projectedPoints[3];
	const anVec4 & p4 = projectedPoints[4];
	const anVec4 & p5 = projectedPoints[5];
	const anVec4 & p6 = projectedPoints[6];
	const anVec4 & p7 = projectedPoints[7];

#if defined( CLIP_SPACE_D3D )	// the D3D near plane is at Z=0 instead of Z=-1
	const float d0 = p0.z;
	const float d1 = p1.z;
	const float d2 = p2.z;
	const float d3 = p3.z;
	const float d4 = p4.z;
	const float d5 = p5.z;
	const float d6 = p6.z;
	const float d7 = p7.z;
#else
	const float d0 = p0.z + p0.w;
	const float d1 = p1.z + p1.w;
	const float d2 = p2.z + p2.w;
	const float d3 = p3.z + p3.w;
	const float d4 = p4.z + p4.w;
	const float d5 = p5.z + p5.w;
	const float d6 = p6.z + p6.w;
	const float d7 = p7.z + p7.w;
#endif

	const float deltaA = d0 - d1;
	const float deltaB = d1 - d2;
	const float deltaC = d2 - d3;
	const float deltaD = d3 - d0;

	const float deltaE = d4 - d5;
	const float deltaF = d5 - d6;
	const float deltaG = d6 - d7;
	const float deltaH = d7 - d4;

	const float deltaI = d0 - d4;
	const float deltaJ = d1 - d5;
	const float deltaK = d2 - d6;
	const float deltaL = d3 - d7;

	const float fractionA = ( fabs( deltaA ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d0 / deltaA ) : 0.0f;
	const float fractionB = ( fabs( deltaB ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d1 / deltaB ) : 0.0f;
	const float fractionC = ( fabs( deltaC ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d2 / deltaC ) : 0.0f;
	const float fractionD = ( fabs( deltaD ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d3 / deltaD ) : 0.0f;

	const float fractionE = ( fabs( deltaE ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d4 / deltaE ) : 0.0f;
	const float fractionF = ( fabs( deltaF ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d5 / deltaF ) : 0.0f;
	const float fractionG = ( fabs( deltaG ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d6 / deltaG ) : 0.0f;
	const float fractionH = ( fabs( deltaH ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d7 / deltaH ) : 0.0f;

	const float fractionI = ( fabs( deltaI ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d0 / deltaI ) : 0.0f;
	const float fractionJ = ( fabs( deltaJ ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d1 / deltaJ ) : 0.0f;
	const float fractionK = ( fabs( deltaK ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d2 / deltaK ) : 0.0f;
	const float fractionL = ( fabs( deltaL ) > anMath::FLT_SMALLEST_NON_DENORMAL ) ? ( d3 / deltaL ) : 0.0f;

	const bool clipA = ( fractionA > 0.0f && fractionA < 1.0f );
	const bool clipB = ( fractionB > 0.0f && fractionB < 1.0f );
	const bool clipC = ( fractionC > 0.0f && fractionC < 1.0f );
	const bool clipD = ( fractionD > 0.0f && fractionD < 1.0f );

	const bool clipE = ( fractionE > 0.0f && fractionE < 1.0f );
	const bool clipF = ( fractionF > 0.0f && fractionF < 1.0f );
	const bool clipG = ( fractionG > 0.0f && fractionG < 1.0f );
	const bool clipH = ( fractionH > 0.0f && fractionH < 1.0f );

	const bool clipI = ( fractionI > 0.0f && fractionI < 1.0f );
	const bool clipJ = ( fractionJ > 0.0f && fractionJ < 1.0f );
	const bool clipK = ( fractionK > 0.0f && fractionK < 1.0f );
	const bool clipL = ( fractionL > 0.0f && fractionL < 1.0f );

	const anVec4 intersectionA = p0 + fractionA * ( p1 - p0 );
	const anVec4 intersectionB = p1 + fractionB * ( p2 - p1 );
	const anVec4 intersectionC = p2 + fractionC * ( p3 - p2 );
	const anVec4 intersectionD = p3 + fractionD * ( p0 - p3 );

	const anVec4 intersectionE = p4 + fractionE * ( p5 - p4 );
	const anVec4 intersectionF = p5 + fractionF * ( p6 - p5 );
	const anVec4 intersectionG = p6 + fractionG * ( p7 - p6 );
	const anVec4 intersectionH = p7 + fractionH * ( p4 - p7 );

	const anVec4 intersectionI = p0 + fractionI * ( p4 - p0 );
	const anVec4 intersectionJ = p1 + fractionJ * ( p5 - p1 );
	const anVec4 intersectionK = p2 + fractionK * ( p6 - p2 );
	const anVec4 intersectionL = p3 + fractionL * ( p7 - p3 );

	anVec4 edgeVerts[24];

	edgeVerts[ 0] = ( clipA && d0 < 0.0f ) ? intersectionA : p0;
	edgeVerts[ 2] = ( clipB && d1 < 0.0f ) ? intersectionB : p1;
	edgeVerts[ 4] = ( clipC && d2 < 0.0f ) ? intersectionC : p2;
	edgeVerts[ 6] = ( clipD && d3 < 0.0f ) ? intersectionD : p3;

	edgeVerts[ 1] = ( clipA && d1 < 0.0f ) ? intersectionA : p1;
	edgeVerts[ 3] = ( clipB && d2 < 0.0f ) ? intersectionB : p2;
	edgeVerts[ 5] = ( clipC && d3 < 0.0f ) ? intersectionC : p3;
	edgeVerts[ 7] = ( clipD && d0 < 0.0f ) ? intersectionD : p0;

	edgeVerts[ 8] = ( clipE && d4 < 0.0f ) ? intersectionE : p4;
	edgeVerts[10] = ( clipF && d5 < 0.0f ) ? intersectionF : p5;
	edgeVerts[12] = ( clipG && d6 < 0.0f ) ? intersectionG : p6;
	edgeVerts[14] = ( clipH && d7 < 0.0f ) ? intersectionH : p7;

	edgeVerts[ 9] = ( clipE && d5 < 0.0f ) ? intersectionE : p5;
	edgeVerts[11] = ( clipF && d6 < 0.0f ) ? intersectionF : p6;
	edgeVerts[13] = ( clipG && d7 < 0.0f ) ? intersectionG : p7;
	edgeVerts[15] = ( clipH && d4 < 0.0f ) ? intersectionH : p4;

	edgeVerts[16] = ( clipI && d0 < 0.0f ) ? intersectionI : p0;
	edgeVerts[18] = ( clipJ && d1 < 0.0f ) ? intersectionJ : p1;
	edgeVerts[20] = ( clipK && d2 < 0.0f ) ? intersectionK : p2;
	edgeVerts[22] = ( clipL && d3 < 0.0f ) ? intersectionL : p3;

	edgeVerts[17] = ( clipI && d4 < 0.0f ) ? intersectionI : p4;
	edgeVerts[19] = ( clipJ && d5 < 0.0f ) ? intersectionJ : p5;
	edgeVerts[21] = ( clipK && d6 < 0.0f ) ? intersectionK : p6;
	edgeVerts[23] = ( clipL && d7 < 0.0f ) ? intersectionL : p7;

	anBounds projBnds;
	for ( int i = 0; i < 3; i++ ) {
		projBnds[0][i] = RENDER_MATRIX_INFINITY;
		projBnds[1][i] = - RENDER_MATRIX_INFINITY;
	}

	for ( int i = 0; i < 24; i++ ) {
		const anVec4 & v = edgeVerts[i];
		if ( v.w <= anMath::FLT_SMALLEST_NON_DENORMAL ) {
			continue;
		}

		const float rw = 1.0f / v.w;

		const float px = v.x * rw;
		const float py = v.y * rw;
		const float pz = v.z * rw;

		projBnds[0][0] = Min( projBnds[0][0], px );
		projBnds[0][1] = Min( projBnds[0][1], py );
		projBnds[0][2] = Min( projBnds[0][2], pz );

		projBnds[1][0] = Max( projBnds[1][0], px );
		projBnds[1][1] = Max( projBnds[1][1], py );
		projBnds[1][2] = Max( projBnds[1][2], pz );
	}

	if ( windowSpace ) {
		// convert to window coords
		projBnds[0][0] = projBnds[0][0] * 0.5f + 0.5f;
		projBnds[1][0] = projBnds[1][0] * 0.5f + 0.5f;

		projBnds[0][1] = projBnds[0][1] * 0.5f + 0.5f;
		projBnds[1][1] = projBnds[1][1] * 0.5f + 0.5f;

#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		projBnds[0][2] = projBnds[0][2] * 0.5f + 0.5f;
		projBnds[1][2] = projBnds[1][2] * 0.5f + 0.5f;
#endif

		// clamp to [0, 1] range
		projBnds[0][0] = anMath::ClampFloat( 0.0f, 1.0f, projBnds[0][0] );
		projBnds[1][0] = anMath::ClampFloat( 0.0f, 1.0f, projBnds[1][0] );

		projBnds[0][1] = anMath::ClampFloat( 0.0f, 1.0f, projBnds[0][1] );
		projBnds[1][1] = anMath::ClampFloat( 0.0f, 1.0f, projBnds[1][1] );

		projBnds[0][2] = anMath::ClampFloat( 0.0f, 1.0f, projBnds[0][2] );
		projBnds[1][2] = anMath::ClampFloat( 0.0f, 1.0f, projBnds[1][2] );
	}

	assert( projected[0].Compare( projBnds[0], 0.01f ) );
	assert( projected[1].Compare( projBnds[1], 0.01f ) );
}
#else
	const anVec3 points[8] = {
		anVec3( bounds[0][0], bounds[0][1], bounds[0][2] ),
		anVec3( bounds[1][0], bounds[0][1], bounds[0][2] ),
		anVec3( bounds[1][0], bounds[1][1], bounds[0][2] ),
		anVec3( bounds[0][0], bounds[1][1], bounds[0][2] ),
		anVec3( bounds[0][0], bounds[0][1], bounds[1][2] ),
		anVec3( bounds[1][0], bounds[0][1], bounds[1][2] ),
		anVec3( bounds[1][0], bounds[1][1], bounds[1][2] ),
		anVec3( bounds[0][0], bounds[1][1], bounds[1][2] )
	};

	anVec4 projectedPoints[8];
	for ( int i = 0; i < 8; i++ ) {
		const anVec3 & v = points[i];
		projectedPoints[i].x = v[0] * mvp[0][0] + v[1] * mvp[0][1] + v[2] * mvp[0][2] + mvp[0][3];
		projectedPoints[i].y = v[0] * mvp[1][0] + v[1] * mvp[1][1] + v[2] * mvp[1][2] + mvp[1][3];
		projectedPoints[i].z = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
		projectedPoints[i].w = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];
	}

	anVec4 edgeVerts[24];
	for ( int i = 0; i < 3; i++ ) {
		int offset0 = ( i & 1 ) * 4;
		int offset1 = ( i & 1 ) * 4 + ( i & 2 ) * 2;
		int offset3 = ~( i >> 1 ) & 1;
		for ( int j = 0; j < 4; j++ ) {
			const anVec4 p0 = projectedPoints[offset0 + ( ( j + 0 ) & 3 )];
			const anVec4 p1 = projectedPoints[offset1 + ( ( j + offset3 ) & 3 )];

#if defined( CLIP_SPACE_D3D )	// the D3D near plane is at Z=0 instead of Z=-1
			const float d0 = p0.z;
			const float d1 = p1.z;
#else
			const float d0 = p0.z + p0.w;
			const float d1 = p1.z + p1.w;
#endif
			const float delta = d0 - d1;
			const float fraction = anMath::Fabs( delta ) > anMath::FLT_SMALLEST_NON_DENORMAL ? ( d0 / delta ) : 1.0f;
			const bool clip = ( fraction > 0.0f && fraction < 1.0f );
			const anVec4 intersection = p0 + fraction * ( p1 - p0 );

			edgeVerts[i * 8 + j * 2 + 0] = ( clip && d0 < 0.0f ) ? intersection : p0;
			edgeVerts[i * 8 + j * 2 + 1] = ( clip && d1 < 0.0f ) ? intersection : p1;
		}
	}

	for ( int i = 0; i < 3; i++ ) {
		projected[0][i] = RENDER_MATRIX_INFINITY;
		projected[1][i] = - RENDER_MATRIX_INFINITY;
	}

	for ( int i = 0; i < 24; i++ ) {
		const anVec4 & v = edgeVerts[i];

		if ( v.w <= anMath::FLT_SMALLEST_NON_DENORMAL ) {
			continue;
		}

		const float rw = 1.0f / v.w;

		const float px = v.x * rw;
		const float py = v.y * rw;
		const float pz = v.z * rw;

		projected[0][0] = Min( projected[0][0], px );
		projected[0][1] = Min( projected[0][1], py );
		projected[0][2] = Min( projected[0][2], pz );

		projected[1][0] = Max( projected[1][0], px );
		projected[1][1] = Max( projected[1][1], py );
		projected[1][2] = Max( projected[1][2], pz );
	}

	if ( windowSpace ) {
		// convert to window coords
		projected[0][0] = projected[0][0] * 0.5f + 0.5f;
		projected[1][0] = projected[1][0] * 0.5f + 0.5f;

		projected[0][1] = projected[0][1] * 0.5f + 0.5f;
		projected[1][1] = projected[1][1] * 0.5f + 0.5f;

#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		projected[0][2] = projected[0][2] * 0.5f + 0.5f;
		projected[1][2] = projected[1][2] * 0.5f + 0.5f;
#endif

		// clamp to [0, 1] range
		projected[0][0] = anMath::ClampFloat( 0.0f, 1.0f, projected[0][0] );
		projected[1][0] = anMath::ClampFloat( 0.0f, 1.0f, projected[1][0] );

		projected[0][1] = anMath::ClampFloat( 0.0f, 1.0f, projected[0][1] );
		projected[1][1] = anMath::ClampFloat( 0.0f, 1.0f, projected[1][1] );

		projected[0][2] = anMath::ClampFloat( 0.0f, 1.0f, projected[0][2] );
		projected[1][2] = anMath::ClampFloat( 0.0f, 1.0f, projected[1][2] );
	}
#endif
}

/*
========================
anGLMatrix::DepthBoundsForBounds

Calculates the depth bounds of the given bounding box projected with the given Model View Projection (MVP) matrix.
If 'windowSpace' is true then the calculated depth bounds are moved and clamped to the [0, 1] range.

The given bounding box is not clipped to the MVP so the depth bounds may not be as tight as possible.
========================
*/
void anGLMatrix::DepthBoundsForBounds( float & min, float & max, const anGLMatrix & mvp, const anBounds & bounds, bool windowSpace ) {
#ifdef ARC_WIN_X86_SSE2_INTRIN
	__m128 mvp2 = _mm_loadu_ps( mvp[2] );
	__m128 mvp3 = _mm_loadu_ps( mvp[3] );

	__m128 b0 = _mm_loadu_bounds_0( bounds );
	__m128 b1 = _mm_loadu_bounds_1( bounds );

	// take the four points on the X-Y plane
	__m128 vxy = _mm_unpacklo_ps( b0, b1 );						// min X, max X, min Y, max Y
	__m128 vx = _mm_perm_ps( vxy, _MM_SHUFFLE( 1, 0, 1, 0 ) );	// min X, max X, min X, max X
	__m128 vy = _mm_perm_ps( vxy, _MM_SHUFFLE( 3, 3, 2, 2 ) );	// min Y, min Y, max Y, max Y

	__m128 vz0 = _mm_splat_ps( b0, 2 );							// min Z, min Z, min Z, min Z
	__m128 vz1 = _mm_splat_ps( b1, 2 );							// max Z, max Z, max Z, max Z

	// compute four partial Z,W values
	__m128 parz = _mm_splat_ps( mvp2, 3 );
	__m128 parw = _mm_splat_ps( mvp3, 3 );

	parz = _mm_madd_ps( vx, _mm_splat_ps( mvp2, 0 ), parz );
	parw = _mm_madd_ps( vx, _mm_splat_ps( mvp3, 0 ), parw );

	parw = _mm_madd_ps( vy, _mm_splat_ps( mvp3, 1 ), parw );
	parz = _mm_madd_ps( vy, _mm_splat_ps( mvp2, 1 ), parz );

	__m128 z0 = _mm_madd_ps( vz0, _mm_splat_ps( mvp2, 2 ), parz );
	__m128 w0 = _mm_madd_ps( vz0, _mm_splat_ps( mvp3, 2 ), parw );

	__m128 z1 = _mm_madd_ps( vz1, _mm_splat_ps( mvp2, 2 ), parz );
	__m128 w1 = _mm_madd_ps( vz1, _mm_splat_ps( mvp3, 2 ), parw );

	__m128 s0 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w0 );
	w0 = _mm_or_ps( w0, _mm_and_ps( vector_float_smallest_non_denorm, s0 ) );

	__m128 rw0 = _mm_rcp32_ps( w0 );
	z0 = _mm_mul_ps( z0, rw0 );
	z0 = _mm_sel_ps( z0, vector_float_neg_infinity, s0 );

	__m128 s1 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w1 );
	w1 = _mm_or_ps( w1, _mm_and_ps( vector_float_smallest_non_denorm, s1 ) );

	__m128 rw1 = _mm_rcp32_ps( w1 );
	z1 = _mm_mul_ps( z1, rw1 );
	z1 = _mm_sel_ps( z1, vector_float_neg_infinity, s1 );

	__m128 minv = _mm_min_ps( z0, z1 );
	__m128 maxv = _mm_max_ps( z0, z1 );

	minv = _mm_min_ps( minv, _mm_perm_ps( minv, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	minv = _mm_min_ps( minv, _mm_perm_ps( minv, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	maxv = _mm_max_ps( maxv, _mm_perm_ps( maxv, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	maxv = _mm_max_ps( maxv, _mm_perm_ps( maxv, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	if ( windowSpace ) {
#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		minv = _mm_madd_ps( minv, vector_float_half, vector_float_half );
		maxv = _mm_madd_ps( maxv, vector_float_half, vector_float_half );
#endif
		minv = _mm_max_ps( minv, vector_float_zero );
		maxv = _mm_min_ps( maxv, vector_float_one );
	}

	_mm_store_ss( & min, minv );
	_mm_store_ss( & max, maxv );
#else
	float localMin = RENDER_MATRIX_INFINITY;
	float localMax = - RENDER_MATRIX_INFINITY;

	anVec3 v;
	for ( int x = 0; x < 2; x++ ) {
		v[0] = bounds[x][0];
		for ( int y = 0; y < 2; y++ ) {
			v[1] = bounds[y][1];
			for ( int z = 0; z < 2; z++ ) {
				v[2] = bounds[z][2];

				float tz = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
				float tw = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];

				if ( tw > anMath::FLT_SMALLEST_NON_DENORMAL ) {
					tz = tz / tw;
				} else {
					tz = -RENDER_MATRIX_INFINITY;
				}

				localMin = Min( localMin, tz );
				localMax = Max( localMax, tz );
			}
		}
	}

	if ( windowSpace ) {
		// convert to window coords
#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
 		min = localMin * 0.5f + 0.5f;
		max = localMax * 0.5f + 0.5f;
#endif
		// clamp to the [0, 1] range
		min = Max( min, 0.0f );
		max = Min( max, 1.0f );
	}

#endif
}

/*
========================
anGLMatrix::DepthBoundsForExtrudedBounds

Calculates the depth bounds of the given extruded bounding box projected with the given Model View Projection (MVP) matrix.
The given bounding box is extruded in the 'extrudeDirection' up to the 'clipPlane'.
If 'windowSpace' is true then the calculated depth bounds are moved and clamped to the [0, 1] range.

The extruded bounding box is not clipped to the MVP so the depth bounds may not be as tight as possible.
========================
*/
void anGLMatrix::DepthBoundsForExtrudedBounds( float & min, float & max, const anGLMatrix & mvp, const anBounds & bounds, const anVec3 & extrudeDirection, const anPlane & clipPlane, bool windowSpace ) {
	assert( anMath::Fabs( extrudeDirection * clipPlane.Normal() ) >= anMath::FLT_SMALLEST_NON_DENORMAL );
#ifdef ARC_WIN_X86_SSE2_INTRIN
 	__m128 mvp2 = _mm_loadu_ps( mvp[2] );
	__m128 mvp3 = _mm_loadu_ps( mvp[3] );

	__m128 b0 = _mm_loadu_bounds_0( bounds );
	__m128 b1 = _mm_loadu_bounds_1( bounds );

	// take the four points on the X-Y plane
	__m128 vxy = _mm_unpacklo_ps( b0, b1 );						// min X, max X, min Y, max Y
	__m128 vx = _mm_perm_ps( vxy, _MM_SHUFFLE( 1, 0, 1, 0 ) );	// min X, max X, min X, max X
	__m128 vy = _mm_perm_ps( vxy, _MM_SHUFFLE( 3, 3, 2, 2 ) );	// min Y, min Y, max Y, max Y

	__m128 vz0 = _mm_splat_ps( b0, 2 );							// min Z, min Z, min Z, min Z
	__m128 vz1 = _mm_splat_ps( b1, 2 );							// max Z, max Z, max Z, max Z

	__m128 minv;
	__m128 maxv;
	// calculate the min/max depth values for the bounding box corners
	{
		// compute four partial Z,W values
		__m128 parz = _mm_splat_ps( mvp2, 3 );
		__m128 parw = _mm_splat_ps( mvp3, 3 );

		parz = _mm_madd_ps( vx, _mm_splat_ps( mvp2, 0 ), parz );
		parw = _mm_madd_ps( vx, _mm_splat_ps( mvp3, 0 ), parw );

		parw = _mm_madd_ps( vy, _mm_splat_ps( mvp3, 1 ), parw );
		parz = _mm_madd_ps( vy, _mm_splat_ps( mvp2, 1 ), parz );

		__m128 z0 = _mm_madd_ps( vz0, _mm_splat_ps( mvp2, 2 ), parz );
		__m128 w0 = _mm_madd_ps( vz0, _mm_splat_ps( mvp3, 2 ), parw );

		__m128 z1 = _mm_madd_ps( vz1, _mm_splat_ps( mvp2, 2 ), parz );
		__m128 w1 = _mm_madd_ps( vz1, _mm_splat_ps( mvp3, 2 ), parw );

		__m128 s0 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w0 );
		w0 = _mm_or_ps( w0, _mm_and_ps( vector_float_smallest_non_denorm, s0 ) );

		__m128 rw0 = _mm_rcp32_ps( w0 );
		z0 = _mm_mul_ps( z0, rw0 );
		z0 = _mm_sel_ps( z0, vector_float_neg_infinity, s0 );

		__m128 s1 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w1 );
		w1 = _mm_or_ps( w1, _mm_and_ps( vector_float_smallest_non_denorm, s1 ) );

		__m128 rw1 = _mm_rcp32_ps( w1 );
		z1 = _mm_mul_ps( z1, rw1 );
		z1 = _mm_sel_ps( z1, vector_float_neg_infinity, s1 );

		minv = _mm_min_ps( z0, z1 );
		maxv = _mm_max_ps( z0, z1 );
	}
	// calculate and include the min/max depth value for the extruded bounding box corners
	{
		__m128 clipX = _mm_splat_ps( _mm_load_ss( clipPlane.ToFloatPtr() + 0 ), 0 );
		__m128 clipY = _mm_splat_ps( _mm_load_ss( clipPlane.ToFloatPtr() + 1 ), 0 );
		__m128 clipZ = _mm_splat_ps( _mm_load_ss( clipPlane.ToFloatPtr() + 2 ), 0 );
		__m128 clipW = _mm_splat_ps( _mm_load_ss( clipPlane.ToFloatPtr() + 3 ), 0 );

		__m128 extrudeX = _mm_splat_ps( _mm_load_ss( extrudeDirection.ToFloatPtr() + 0 ), 0 );
		__m128 extrudeY = _mm_splat_ps( _mm_load_ss( extrudeDirection.ToFloatPtr() + 1 ), 0 );
		__m128 extrudeZ = _mm_splat_ps( _mm_load_ss( extrudeDirection.ToFloatPtr() + 2 ), 0 );

		__m128 closing = _mm_madd_ps( clipX, extrudeX, _mm_madd_ps( clipY, extrudeY, _mm_mul_ps( clipZ, extrudeZ ) ) );
		__m128 invClosing = _mm_rcp32_ps( closing );
		invClosing = _mm_xor_ps( invClosing, vector_float_sign_bit );

		__m128 dt = _mm_madd_ps( clipX, vx, _mm_madd_ps( clipY, vy, clipW ) );
		__m128 d0 = _mm_madd_ps( clipZ, vz0, dt );
		__m128 d1 = _mm_madd_ps( clipZ, vz1, dt );

		d0 = _mm_mul_ps( d0, invClosing );
		d1 = _mm_mul_ps( d1, invClosing );

		__m128 vx0 = _mm_madd_ps( extrudeX, d0, vx );
		__m128 vx1 = _mm_madd_ps( extrudeX, d1, vx );

		__m128 vy0 = _mm_madd_ps( extrudeY, d0, vy );
		__m128 vy1 = _mm_madd_ps( extrudeY, d1, vy );

		vz0 = _mm_madd_ps( extrudeZ, d0, vz0 );
		vz1 = _mm_madd_ps( extrudeZ, d1, vz1 );

		__m128 mvp2X = _mm_splat_ps( mvp2, 0 );
		__m128 mvp3X = _mm_splat_ps( mvp3, 0 );

		__m128 mvp2W = _mm_splat_ps( mvp2, 3 );
		__m128 mvp3W = _mm_splat_ps( mvp3, 3 );

		__m128 z0 = _mm_madd_ps( vx0, mvp2X, mvp2W );
		__m128 w0 = _mm_madd_ps( vx0, mvp3X, mvp3W );

		__m128 z1 = _mm_madd_ps( vx1, mvp2X, mvp2W );
		__m128 w1 = _mm_madd_ps( vx1, mvp3X, mvp3W );

		__m128 mvp2Y = _mm_splat_ps( mvp2, 1 );
		__m128 mvp3Y = _mm_splat_ps( mvp3, 1 );

		z0 = _mm_madd_ps( vy0, mvp2Y, z0 );
		w0 = _mm_madd_ps( vy0, mvp3Y, w0 );

		z1 = _mm_madd_ps( vy1, mvp2Y, z1 );
		w1 = _mm_madd_ps( vy1, mvp3Y, w1 );

		__m128 mvp2Z = _mm_splat_ps( mvp2, 2 );
		__m128 mvp3Z = _mm_splat_ps( mvp3, 2 );

		z0 = _mm_madd_ps( vz0, mvp2Z, z0 );
		w0 = _mm_madd_ps( vz0, mvp3Z, w0 );

		z1 = _mm_madd_ps( vz1, mvp2Z, z1 );
		w1 = _mm_madd_ps( vz1, mvp3Z, w1 );

		__m128 s0 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w0 );
		w0 = _mm_or_ps( w0, _mm_and_ps( vector_float_smallest_non_denorm, s0 ) );

		__m128 rw0 = _mm_rcp32_ps( w0 );
		z0 = _mm_mul_ps( z0, rw0 );
		z0 = _mm_sel_ps( z0, vector_float_neg_infinity, s0 );

		__m128 s1 = _mm_cmpgt_ps( vector_float_smallest_non_denorm, w1 );
		w1 = _mm_or_ps( w1, _mm_and_ps( vector_float_smallest_non_denorm, s1 ) );

		__m128 rw1 = _mm_rcp32_ps( w1 );
		z1 = _mm_mul_ps( z1, rw1 );
		z1 = _mm_sel_ps( z1, vector_float_neg_infinity, s1 );

		minv = _mm_min_ps( minv, z0 );
		maxv = _mm_max_ps( maxv, z0 );

		minv = _mm_min_ps( minv, z1 );
		maxv = _mm_max_ps( maxv, z1 );
	}

	minv = _mm_min_ps( minv, _mm_perm_ps( minv, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	minv = _mm_min_ps( minv, _mm_perm_ps( minv, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	maxv = _mm_max_ps( maxv, _mm_perm_ps( maxv, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	maxv = _mm_max_ps( maxv, _mm_perm_ps( maxv, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	if ( windowSpace ) {
#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		minv = _mm_madd_ps( minv, vector_float_half, vector_float_half );
		maxv = _mm_madd_ps( maxv, vector_float_half, vector_float_half );
#endif
		minv = _mm_max_ps( minv, vector_float_zero );
		maxv = _mm_min_ps( maxv, vector_float_one );
	}

	_mm_store_ss( & min, minv );
	_mm_store_ss( & max, maxv );
#else

	const float closing = extrudeDirection * clipPlane.Normal();
	const float invClosing = -1.0f / closing;
	float localMin = RENDER_MATRIX_INFINITY;
	float localMax = - RENDER_MATRIX_INFINITY;

	anVec3 v;
	for ( int x = 0; x < 2; x++ ) {
		v[0] = bounds[x][0];
		for ( int y = 0; y < 2; y++ ) {
			v[1] = bounds[y][1];
			for ( int z = 0; z < 2; z++ ) {
				v[2] = bounds[z][2];
				for ( int extrude = 0; extrude <= 1; extrude++ ) {
					anVec3 test;
					if ( extrude ) {
						float extrudeDist = clipPlane.Distance( v ) * invClosing;
						test = v + extrudeDirection * extrudeDist;
					} else {
						test = v;
					}

					float tz = test[0] * mvp[2][0] + test[1] * mvp[2][1] + test[2] * mvp[2][2] + mvp[2][3];
					float tw = test[0] * mvp[3][0] + test[1] * mvp[3][1] + test[2] * mvp[3][2] + mvp[3][3];

					if ( tw > anMath::FLT_SMALLEST_NON_DENORMAL ) {
						tz = tz / tw;
					} else {
						tz = -RENDER_MATRIX_INFINITY;
					}

					localMin = Min( localMin, tz );
					localMax = Max( localMax, tz );
				}
			}
		}
	}

	if ( windowSpace ) {
		// convert to window coords
#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		min = localMin * 0.5f + 0.5f;
		max = localMax * 0.5f + 0.5f;
#endif
		// clamp to the [0, 1] range
		min = Max( min, 0.0f );
		max = Min( max, 1.0f );
	}
#endif
}

/*
========================
anGLMatrix::DepthBoundsForShadowBounds

Calculates the depth bounds of the infinite shadow volume projected with the given Model View Projection (MVP) matrix.
The infinite shadow volume is cast from the given occluder bounding box and the given light position.
If 'windowSpace' is true then the calculated depth bounds are moved and clamped to the [0, 1] range.

The infinite shadow volume is fully clipped to the MVP to get the tightest possible bounds.

Note that this code assumes the MVP matrix has an infinite far clipping plane. When the far plane is at
infinity the shadow volume is never far clipped and it is sufficient to test whether or not the center
of the near clip plane is inside the shadow volume to calculate the correct minimum Z. If the far plane
is not at infinity then this code would also have to test for the view frustum being completely contained
inside the shadow volume to also calculate the correct maximum Z. This could be done, for instance, by
testing if the center of the far clipping plane is contained inside the shadow volume.
========================
*/
void anGLMatrix::DepthBoundsForShadowBounds( float & min, float & max, const anGLMatrix & mvp, const anBounds & bounds, const anVec3 & localLightOrigin, bool windowSpace ) {
#ifdef ARC_WIN_X86_SSE2_INTRIN
	const __m128 mvp0 = _mm_loadu_ps( mvp[0] );
	const __m128 mvp1 = _mm_loadu_ps( mvp[1] );
	const __m128 mvp2 = _mm_loadu_ps( mvp[2] );
	const __m128 mvp3 = _mm_loadu_ps( mvp[3] );

	const __m128 t0 = _mm_unpacklo_ps( mvp0, mvp2 );	// mvp[0][0], mvp[2][0], mvp[0][1], mvp[2][1]
	const __m128 t1 = _mm_unpackhi_ps( mvp0, mvp2 );	// mvp[0][2], mvp[2][2], mvp[0][3], mvp[2][3]
	const __m128 t2 = _mm_unpacklo_ps( mvp1, mvp3 );	// mvp[1][0], mvp[3][0], mvp[1][1], mvp[3][1]
	const __m128 t3 = _mm_unpackhi_ps( mvp1, mvp3 );	// mvp[1][2], mvp[3][2], mvp[1][3], mvp[3][3]

	const __m128 mvpX = _mm_unpacklo_ps( t0, t2 );		// mvp[0][0], mvp[1][0], mvp[2][0], mvp[3][0]
	const __m128 mvpY = _mm_unpackhi_ps( t0, t2 );		// mvp[0][1], mvp[1][1], mvp[2][1], mvp[3][1]
	const __m128 mvpZ = _mm_unpacklo_ps( t1, t3 );		// mvp[0][2], mvp[1][2], mvp[2][2], mvp[3][2]
	const __m128 mvpW = _mm_unpackhi_ps( t1, t3 );		// mvp[0][3], mvp[1][3], mvp[2][3], mvp[3][3]

	const __m128 b0 = _mm_loadu_bounds_0( bounds );
	const __m128 b1 = _mm_loadu_bounds_1( bounds );

	const __m128 lightOriginX = _mm_load_ss( localLightOrigin.ToFloatPtr() + 0 );
	const __m128 lightOriginY = _mm_load_ss( localLightOrigin.ToFloatPtr() + 1 );
	const __m128 lightOriginZ = _mm_load_ss( localLightOrigin.ToFloatPtr() + 2 );
	const __m128 lightOrigin = _mm_unpacklo_ps( _mm_unpacklo_ps( lightOriginX, lightOriginZ ), lightOriginY );

	// calculate the front facing polygon bits
	int frontBits = GetBoxFrontBits_SSE2( b0, b1, lightOrigin );

	const __m128 b0X = _mm_splat_ps( b0, 0 );
	const __m128 b0Y = _mm_splat_ps( b0, 1 );
	const __m128 b0Z = _mm_splat_ps( b0, 2 );

	const __m128 b1X = _mm_splat_ps( b1, 0 );
	const __m128 b1Y = _mm_splat_ps( b1, 1 );
	const __m128 b1Z = _mm_splat_ps( b1, 2 );

	// bounding box corners
	const __m128 np0 = _mm_madd_ps( b0X, mvpX, _mm_madd_ps( b0Y, mvpY, _mm_madd_ps( b0Z, mvpZ, mvpW ) ) );
	const __m128 np1 = _mm_madd_ps( b1X, mvpX, _mm_madd_ps( b0Y, mvpY, _mm_madd_ps( b0Z, mvpZ, mvpW ) ) );
	const __m128 np2 = _mm_madd_ps( b1X, mvpX, _mm_madd_ps( b1Y, mvpY, _mm_madd_ps( b0Z, mvpZ, mvpW ) ) );
	const __m128 np3 = _mm_madd_ps( b0X, mvpX, _mm_madd_ps( b1Y, mvpY, _mm_madd_ps( b0Z, mvpZ, mvpW ) ) );
	const __m128 np4 = _mm_madd_ps( b0X, mvpX, _mm_madd_ps( b0Y, mvpY, _mm_madd_ps( b1Z, mvpZ, mvpW ) ) );
	const __m128 np5 = _mm_madd_ps( b1X, mvpX, _mm_madd_ps( b0Y, mvpY, _mm_madd_ps( b1Z, mvpZ, mvpW ) ) );
	const __m128 np6 = _mm_madd_ps( b1X, mvpX, _mm_madd_ps( b1Y, mvpY, _mm_madd_ps( b1Z, mvpZ, mvpW ) ) );
	const __m128 np7 = _mm_madd_ps( b0X, mvpX, _mm_madd_ps( b1Y, mvpY, _mm_madd_ps( b1Z, mvpZ, mvpW ) ) );

	ALIGNTYPE16 anVec4 projectedNearPoints[8];
	_mm_store_ps( projectedNearPoints[0].ToFloatPtr(), np0 );
	_mm_store_ps( projectedNearPoints[1].ToFloatPtr(), np1 );
	_mm_store_ps( projectedNearPoints[2].ToFloatPtr(), np2 );
	_mm_store_ps( projectedNearPoints[3].ToFloatPtr(), np3 );
	_mm_store_ps( projectedNearPoints[4].ToFloatPtr(), np4 );
	_mm_store_ps( projectedNearPoints[5].ToFloatPtr(), np5 );
	_mm_store_ps( projectedNearPoints[6].ToFloatPtr(), np6 );
	_mm_store_ps( projectedNearPoints[7].ToFloatPtr(), np7 );

	// subtract the light position from the bounding box
	const __m128 lightX = _mm_splat_ps( lightOriginX, 0 );
	const __m128 lightY = _mm_splat_ps( lightOriginY, 0 );
	const __m128 lightZ = _mm_splat_ps( lightOriginZ, 0 );

	const __m128 d0X = _mm_sub_ps( b0X, lightX );
	const __m128 d0Y = _mm_sub_ps( b0Y, lightY );
	const __m128 d0Z = _mm_sub_ps( b0Z, lightZ );

	const __m128 d1X = _mm_sub_ps( b1X, lightX );
	const __m128 d1Y = _mm_sub_ps( b1Y, lightY );
	const __m128 d1Z = _mm_sub_ps( b1Z, lightZ );

	// bounding box corners projected to infinity from the light position
	const __m128 fp0 = _mm_madd_ps( d0X, mvpX, _mm_madd_ps( d0Y, mvpY, _mm_mul_ps( d0Z, mvpZ ) ) );
	const __m128 fp1 = _mm_madd_ps( d1X, mvpX, _mm_madd_ps( d0Y, mvpY, _mm_mul_ps( d0Z, mvpZ ) ) );
	const __m128 fp2 = _mm_madd_ps( d1X, mvpX, _mm_madd_ps( d1Y, mvpY, _mm_mul_ps( d0Z, mvpZ ) ) );
	const __m128 fp3 = _mm_madd_ps( d0X, mvpX, _mm_madd_ps( d1Y, mvpY, _mm_mul_ps( d0Z, mvpZ ) ) );
	const __m128 fp4 = _mm_madd_ps( d0X, mvpX, _mm_madd_ps( d0Y, mvpY, _mm_mul_ps( d1Z, mvpZ ) ) );
	const __m128 fp5 = _mm_madd_ps( d1X, mvpX, _mm_madd_ps( d0Y, mvpY, _mm_mul_ps( d1Z, mvpZ ) ) );
	const __m128 fp6 = _mm_madd_ps( d1X, mvpX, _mm_madd_ps( d1Y, mvpY, _mm_mul_ps( d1Z, mvpZ ) ) );
	const __m128 fp7 = _mm_madd_ps( d0X, mvpX, _mm_madd_ps( d1Y, mvpY, _mm_mul_ps( d1Z, mvpZ ) ) );

	ALIGNTYPE16 anVec4 projectedFarPoints[8];
	_mm_store_ps( projectedFarPoints[0].ToFloatPtr(), fp0 );
	_mm_store_ps( projectedFarPoints[1].ToFloatPtr(), fp1 );
	_mm_store_ps( projectedFarPoints[2].ToFloatPtr(), fp2 );
	_mm_store_ps( projectedFarPoints[3].ToFloatPtr(), fp3 );
	_mm_store_ps( projectedFarPoints[4].ToFloatPtr(), fp4 );
	_mm_store_ps( projectedFarPoints[5].ToFloatPtr(), fp5 );
	_mm_store_ps( projectedFarPoints[6].ToFloatPtr(), fp6 );
	_mm_store_ps( projectedFarPoints[7].ToFloatPtr(), fp7 );

	ALIGNTYPE16 anVec4 clippedPoints[( 6 + 12 ) * 16];
	int numClippedPoints = 0;

	// clip the front facing bounding box polygons at the near cap
	const frontPolygons_t & frontPolygons = boxFrontPolygonsForFrontBits[frontBits];
	for ( int i = 0; i < frontPolygons.count; i++ ) {
		const int polygon = frontPolygons.indices[i];
		_mm_store_ps( clippedPoints[numClippedPoints + 0].ToFloatPtr(), _mm_load_ps( projectedNearPoints[boxPolygonVertices[polygon][0]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 1].ToFloatPtr(), _mm_load_ps( projectedNearPoints[boxPolygonVertices[polygon][1]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 2].ToFloatPtr(), _mm_load_ps( projectedNearPoints[boxPolygonVertices[polygon][2]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 3].ToFloatPtr(), _mm_load_ps( projectedNearPoints[boxPolygonVertices[polygon][3]].ToFloatPtr() ) );
		numClippedPoints += ClipHomogeneousPolygonToUnitCube_SSE2( &clippedPoints[numClippedPoints], 4 );
	}

	// clip the front facing bounding box polygons projected to the far cap
	for ( int i = 0; i < frontPolygons.count; i++ ) {
		const int polygon = frontPolygons.indices[i];
		_mm_store_ps( clippedPoints[numClippedPoints + 0].ToFloatPtr(), _mm_load_ps( projectedFarPoints[boxPolygonVertices[polygon][0]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 1].ToFloatPtr(), _mm_load_ps( projectedFarPoints[boxPolygonVertices[polygon][1]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 2].ToFloatPtr(), _mm_load_ps( projectedFarPoints[boxPolygonVertices[polygon][2]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 3].ToFloatPtr(), _mm_load_ps( projectedFarPoints[boxPolygonVertices[polygon][3]].ToFloatPtr() ) );
		numClippedPoints += ClipHomogeneousPolygonToUnitCube_SSE2( &clippedPoints[numClippedPoints], 4 );
	}

	// clip the silhouette edge polygons that stretch to infinity
	const silhouetteEdges_t & silhouetteEdges = boxSilhouetteEdgesForFrontBits[frontBits];
	for ( int i = 0; i < silhouetteEdges.count; i++ ) {
		const int edge = silhouetteEdges.indices[i];
		_mm_store_ps( clippedPoints[numClippedPoints + 0].ToFloatPtr(), _mm_load_ps( projectedNearPoints[boxEdgeVertices[edge][0]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 1].ToFloatPtr(), _mm_load_ps( projectedNearPoints[boxEdgeVertices[edge][1]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 2].ToFloatPtr(), _mm_load_ps( projectedFarPoints[boxEdgeVertices[edge][1]].ToFloatPtr() ) );
		_mm_store_ps( clippedPoints[numClippedPoints + 3].ToFloatPtr(), _mm_load_ps( projectedFarPoints[boxEdgeVertices[edge][0]].ToFloatPtr() ) );
		numClippedPoints += ClipHomogeneousPolygonToUnitCube_SSE2( &clippedPoints[numClippedPoints], 4 );
	}

	// repeat the first clipped point at the end to get a multiple of 4 clipped points
	const __m128 point0 = _mm_load_ps( clippedPoints[0].ToFloatPtr() );
	for ( int i = numClippedPoints; ( i & 3 ) != 0; i++ ) {
		_mm_store_ps( clippedPoints[i].ToFloatPtr(), point0 );
	}

	// test if the center of the near clip plane is inside the infinite shadow volume
	const anVec3 localNearClipCenter = LocalNearClipCenterFromMVP( mvp );
	const bool inside = PointInsideInfiniteShadow( bounds, localLightOrigin, localNearClipCenter, RENDER_MATRIX_PROJECTION_EPSILON );

	__m128 minZ = inside ? vector_float_neg_one : vector_float_pos_infinity;
	__m128 maxZ = vector_float_neg_infinity;

	for ( int i = 0; i < numClippedPoints; i += 4 ) {
		const __m128 cp0 = _mm_load_ps( clippedPoints[i + 0].ToFloatPtr() );
		const __m128 cp1 = _mm_load_ps( clippedPoints[i + 1].ToFloatPtr() );
		const __m128 cp2 = _mm_load_ps( clippedPoints[i + 2].ToFloatPtr() );
		const __m128 cp3 = _mm_load_ps( clippedPoints[i + 3].ToFloatPtr() );

		const __m128 s1 = _mm_unpackhi_ps( cp0, cp2 );		// cp0[2], cp2[2], cp0[3], cp2[3]
		const __m128 s3 = _mm_unpackhi_ps( cp1, cp3 );		// cp1[2], cp3[2], cp1[3], cp3[3]

		const __m128 cpZ = _mm_unpacklo_ps( s1, s3 );		// cp0[2], cp1[2], cp2[2], cp3[2]
		const __m128 cpW = _mm_unpackhi_ps( s1, s3 );		// cp0[3], cp1[3], cp2[3], cp3[3]

		const __m128 rW = _mm_rcp32_ps( cpW );
		const __m128 rZ = _mm_mul_ps( cpZ, rW );

		minZ = _mm_min_ps( minZ, rZ );
		maxZ = _mm_max_ps( maxZ, rZ );
	}

	minZ = _mm_min_ps( minZ, _mm_perm_ps( minZ, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	minZ = _mm_min_ps( minZ, _mm_perm_ps( minZ, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	maxZ = _mm_max_ps( maxZ, _mm_perm_ps( maxZ, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
	maxZ = _mm_max_ps( maxZ, _mm_perm_ps( maxZ, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );

	if ( windowSpace ) {
#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		minZ = _mm_madd_ps( minZ, vector_float_half, vector_float_half );
		maxZ = _mm_madd_ps( maxZ, vector_float_half, vector_float_half );
#endif

		minZ = _mm_max_ps( _mm_min_ps( minZ, vector_float_one ), vector_float_zero );
		maxZ = _mm_max_ps( _mm_min_ps( maxZ, vector_float_one ), vector_float_zero );
	}

	_mm_store_ss( & min, minZ );
	_mm_store_ss( & max, maxZ );

#else

	const anVec3 points[8] = {
		anVec3( bounds[0][0], bounds[0][1], bounds[0][2] ),
		anVec3( bounds[1][0], bounds[0][1], bounds[0][2] ),
		anVec3( bounds[1][0], bounds[1][1], bounds[0][2] ),
		anVec3( bounds[0][0], bounds[1][1], bounds[0][2] ),
		anVec3( bounds[0][0], bounds[0][1], bounds[1][2] ),
		anVec3( bounds[1][0], bounds[0][1], bounds[1][2] ),
		anVec3( bounds[1][0], bounds[1][1], bounds[1][2] ),
		anVec3( bounds[0][0], bounds[1][1], bounds[1][2] )
	};

	// calculate the front facing polygon bits
	int frontBits = GetBoxFrontBits_Generic( bounds, localLightOrigin );

	// bounding box corners
	ALIGNTYPE16 anVec4 projectedNearPoints[8];
	for ( int i = 0; i < 8; i++ ) {
		const anVec3 & v = points[i];
		projectedNearPoints[i].x = v[0] * mvp[0][0] + v[1] * mvp[0][1] + v[2] * mvp[0][2] + mvp[0][3];
		projectedNearPoints[i].y = v[0] * mvp[1][0] + v[1] * mvp[1][1] + v[2] * mvp[1][2] + mvp[1][3];
		projectedNearPoints[i].z = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
		projectedNearPoints[i].w = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];
	}

	// bounding box corners projected to infinity from the light position
	ALIGNTYPE16 anVec4 projectedFarPoints[8];
	for ( int i = 0; i < 8; i++ ) {
		const anVec3 v = points[i] - localLightOrigin;
		projectedFarPoints[i].x = v[0] * mvp[0][0] + v[1] * mvp[0][1] + v[2] * mvp[0][2];
		projectedFarPoints[i].y = v[0] * mvp[1][0] + v[1] * mvp[1][1] + v[2] * mvp[1][2];
		projectedFarPoints[i].z = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2];
		projectedFarPoints[i].w = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2];
	}

	ALIGNTYPE16 anVec4 clippedPoints[( 6 + 12 ) * 16];
	int numClippedPoints = 0;

	// clip the front facing bounding box polygons at the near cap
	const frontPolygons_t & frontPolygons = boxFrontPolygonsForFrontBits[frontBits];
	for ( int i = 0; i < frontPolygons.count; i++ ) {
		const int polygon = frontPolygons.indices[i];
		clippedPoints[numClippedPoints + 0] = projectedNearPoints[boxPolygonVertices[polygon][0]];
		clippedPoints[numClippedPoints + 1] = projectedNearPoints[boxPolygonVertices[polygon][1]];
		clippedPoints[numClippedPoints + 2] = projectedNearPoints[boxPolygonVertices[polygon][2]];
		clippedPoints[numClippedPoints + 3] = projectedNearPoints[boxPolygonVertices[polygon][3]];
		numClippedPoints += ClipHomogeneousPolygonToUnitCube_Generic( &clippedPoints[numClippedPoints], 4 );
	}

	// clip the front facing bounding box polygons projected to the far cap
	for ( int i = 0; i < frontPolygons.count; i++ ) {
		const int polygon = frontPolygons.indices[i];
		clippedPoints[numClippedPoints + 0] = projectedFarPoints[boxPolygonVertices[polygon][0]];
		clippedPoints[numClippedPoints + 1] = projectedFarPoints[boxPolygonVertices[polygon][1]];
		clippedPoints[numClippedPoints + 2] = projectedFarPoints[boxPolygonVertices[polygon][2]];
		clippedPoints[numClippedPoints + 3] = projectedFarPoints[boxPolygonVertices[polygon][3]];
		numClippedPoints += ClipHomogeneousPolygonToUnitCube_Generic( &clippedPoints[numClippedPoints], 4 );
	}

	// clip the silhouette edge polygons that stretch to infinity
	const silhouetteEdges_t & silhouetteEdges = boxSilhouetteEdgesForFrontBits[frontBits];
	for ( int i = 0; i < silhouetteEdges.count; i++ ) {
		const int edge = silhouetteEdges.indices[i];
		clippedPoints[numClippedPoints + 0] = projectedNearPoints[boxEdgeVertices[edge][0]];
		clippedPoints[numClippedPoints + 1] = projectedNearPoints[boxEdgeVertices[edge][1]];
		clippedPoints[numClippedPoints + 2] = projectedFarPoints[boxEdgeVertices[edge][1]];
		clippedPoints[numClippedPoints + 3] = projectedFarPoints[boxEdgeVertices[edge][0]];
		numClippedPoints += ClipHomogeneousPolygonToUnitCube_Generic( &clippedPoints[numClippedPoints], 4 );
	}

	// test if the center of the near clip plane is inside the infinite shadow volume
	const anVec3 localNearClipCenter = LocalNearClipCenterFromMVP( mvp );
	const bool inside = PointInsideInfiniteShadow( bounds, localLightOrigin, localNearClipCenter, RENDER_MATRIX_PROJECTION_EPSILON );

	min = inside ? -1.0f : RENDER_MATRIX_INFINITY;
	max = - RENDER_MATRIX_INFINITY;

	for ( int i = 0; i < numClippedPoints; i++ ) {
		const anVec4 & c = clippedPoints[i];

		assert( c.w > anMath::FLT_SMALLEST_NON_DENORMAL );

		const float rw = 1.0f / c.w;
		const float pz = c.z * rw;

		min = Min( min, pz );
		max = Max( max, pz );
	}

	if ( windowSpace ) {
		// convert to window coords
#if !defined( CLIP_SPACE_D3D )	// the D3D clip space Z is already in the range [0,1]
		min = min * 0.5f + 0.5f;
		max = max * 0.5f + 0.5f;
#endif
		// clamp to [0, 1] range
		min = anMath::ClampFloat( 0.0f, 1.0f, min );
		max = anMath::ClampFloat( 0.0f, 1.0f, max );
	}
#endif
}

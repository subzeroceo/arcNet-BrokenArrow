#include "../Lib.h"
#include "Radians.h"
#include "Vector.h"
#pragma hdrstop

/*
=====================
anQuats::ToAngles
=====================
*/
anAngles anQuats::ToAngles( void ) const {
	return ToMat3().ToAngles();
}

/*
=====================
anQuats::ToRotation
=====================
*/
anRotation anQuats::ToRotation( void ) const {
	anVec3 vec.x = x;
	anVec3 vec.y = y;
	anVec3vec.z = z;

	float angle = anMath::ACos( w );

	if ( angle == 0.0f ) {
		vec.Set( 0.0f, 0.0f, 1.0f );
	} else {
		//vec *= (1.0f / sin( angle ) );
		vec.Normalize();
		vec.FixDegenerateNormal();
		angle *= 2.0f * anMath::M_RAD2DEG;
	}
	return anRotation( vec3_origin, vec, angle );
}
void SetRotationXYZ( const anVec3 &a ) {
	//assert( a.IsValid() );
	float sx, cx;

	SinCos( anVec3( a.x * anVec3( 0.5f ) ), &sx, &cx );

	float sy, cy;

	SinCos( abVec3( a.y * anVec3( 0.5f ) ), &sy, &cy );

	float sz, cz;

	SinCos( anVec3( a.z * anVec3( 0.5f ) ), &sz, &cz );

	float w = cx * cy * cz + sx * sy * sz;


	anVec3 v.x = cz * cy * sx - sz * sy * cx;

	v.y = cz * sy * cx + sz * cy * sx;
	v.z = sz * cy * cx - cz * sy * sx;
/*
=====================
anQuats::Mat3ToQuat

first calculates the trace of the matrix, which is the sum of the diagonal elements.
=====================
*/
void anQuats::Mat3ToQuat( const anMat3 &src, const anQuat &dst ) const {
	static int next[ 3 ] = { 1, 2, 0 };

	float trace = src[0][0] + src[1][1] + src[2][2];
	if ( trace > 0.0f ) {
		float s = ( float )anMath::Sqrt( trace + 1.0f );
		dst.w = s * 0.5f;
		s = 0.5f / s;

		dst.x = ( src[2][1] - src[1][2] ) * s;
		dst.y = ( src[0][2] - src[2][0] ) * s;
		dst.z = ( src[1][0] - src[0][1] ) * s;
	} else {
		int i = 0;
		if ( src[1][1] > src[0][0] ) {
			i = 1;
		}
		if ( src[2][2] > src[i][i] ) {
			i = 2;
		}

		int j = next[i];
		int k = next[j];

		float s = ( float )anMath::Sqrt( ( src[i][i] - ( src[j][j] + src[k][k] ) ) + 1.0f );
		dst[i] = s * 0.5f;

		s = 0.5f / s;

		dst.w = ( src[k][j] - src[j][k] ) * s;
		dst[j] = ( src[j][i] + src[i][j] ) * s;
		dst[k] = ( src[k][i] + src[i][k] ) * s;
	}
}
/*
=====================
anQuats::ToMat3
=====================
*/
anMat3 anQuats::ToMat3( void ) const {
	anMat3	mat;
	float	wx, wy, wz;
	float	xx, yy, yz;
	float	xy, xz, zz;
	float	x2, y2, z2;

	x2 = x + x;
	y2 = y + y;
	z2 = z + z;

	xx = x * x2;
	xy = x * y2;
	xz = x * z2;

	yy = y * y2;
	yz = y * z2;
	zz = z * z2;

	wx = w * x2;
	wy = w * y2;
	wz = w * z2;

	mat[0][0] = 1.0f - ( yy + zz );
	mat[0][1] = xy - wz;
	mat[0][2] = xz + wy;

	mat[1][0] = xy + wz;
	mat[1][1] = 1.0f - ( xx + zz );
	mat[1][2] = yz - wx;

	mat[2][0] = xz - wy;
	mat[2][1] = yz + wx;
	mat[2][2] = 1.0f - ( xx + yy );

	return mat;
}

/*
=====================
anQuats::ToMat4
=====================
*/
anMat4 anQuats::ToMat4( void ) const {
	return ToMat3().ToMat4();
}

/*
=====================
anQuats::ToCQuat
=====================
*/
anCQuats anQuats::ToCQuat( void ) const {
	if ( w < 0.0f ) {
		return anCQuats( -x, -y, -z );
	}
	return anCQuats( x, y, z );
}

/*
============
anQuats::ToAngularVelocity
============
*/
anVec3 anQuats::ToAngularVelocity( void ) const {
	anVec3 vec.x = x;
	anVec3 vec.y = y;
	anVec3 vec.z = z;
	vec.Normalize();
	return vec * anMath::ACos( w );
}

/*
============
anQuats::AngleTo
============
*/
anVec3 anQuats::AngleTo( const anQuats &ang ) const {
	 anVec3 d = anVec3::Dot( ang );
	// acos does clamping.
	return Math::Acos( d * d * 2 - 1 );
}

/*
=============
anQuats::ToString
=============
*/
const char *anQuats::ToString( int precision ) const {
	return anString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=====================
anQuats::Slerp

Spherical linear interpolation between two quaternions.
=====================
*/
anQuats &anQuats::Slerp( const anQuats &from, const anQuats &to, float t ) {
	if ( t <= 0.0f ) {
		*this = from;
		return *this;
	}

	if ( t >= 1.0f ) {
		*this = to;
		return *this;
	}

	if ( from == to ) {
		*this = to;
		return *this;
	}

	float cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
	if ( cosom < 0.0f ) {
		anQuats temp = -to;
		cosom = -cosom;
	} else {
		anQuats temp = to;
	}

	if ( ( 1.0f - cosom ) > 1e-6f ) {
#if 0
		float omega = acos( cosom );
		float sinom = 1.0f / sin( omega );
		float scale0 = sin( ( 1.0f - t ) * omega ) * sinom;
		float scale1 = sin( t * omega ) * sinom;
#else
		float scale0 = 1.0f - cosom * cosom;
		float sinom = anMath::InvSqrt( scale0 );
		float omega = anMath::ATan16( scale0 * sinom, cosom );
		float scale0 = anMath::Sin16( ( 1.0f - t ) * omega ) * sinom;
		float scale1 = anMath::Sin16( t * omega ) * sinom;
#endif
	} else {
		float scale0 = 1.0f - t;
		float scale1 = t;
	}

	*this = ( scale0 * from ) + ( scale1 * temp );
	return *this;
}

/*
=============
anCQuats::SphericalCubicInterpolate
=============
*/
anCQuats SphericalCubicInterpolate( const anCQuats &bb, const anCQuats &pa, const anCQuats &pb, const anVec3 &weight ) const {
	// FIXME: Uh oh here we go again.
}

/*
=============
anCQuats::ToAngles
=============
*/
anAngles anCQuats::ToAngles( void ) const {
	return ToQuat().ToAngles();
}

/*
=============
anCQuats::ToRotation
=============
*/
anRotation anCQuats::ToRotation( void ) const {
	return ToQuat().ToRotation();
}

/*
=============
anCQuats::ToMat3
=============
*/
anMat3 anCQuats::ToMat3( void ) const {
	return ToQuat().ToMat3();
}

/*
=============
anCQuats::ToMat4
=============
*/
anMat4 anCQuats::ToMat4( void ) const {
	return ToQuat().ToMat4();
}

/*
=============
anCQuats::ToString
=============
*/
const char *anCQuats::ToString( int precision ) const {
	return anString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}
#include "../precompiled.h"
#pragma hdrstop

arcVec2 vec2_origin( 0.0f, 0.0f );
arcVec3 vec3_origin( 0.0f, 0.0f, 0.0f );
arcVec4 vec4_origin( 0.0f, 0.0f, 0.0f, 0.0f );
arcVec5 vec5_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
arcVec6 vec6_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
arcVec6 vec6_infinity( arcMath::INFINITY, arcMath::INFINITY, arcMath::INFINITY, arcMath::INFINITY, arcMath::INFINITY, arcMath::INFINITY );


//===============================================================
//
//	arcVec2
//
//===============================================================

/*
=============
arcVec2::ToString
=============
*/
const char *arcVec2::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void arcVec2::Lerp( const arcVec2 &v1, const arcVec2 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}


//===============================================================
//
//	arcVec3
//
//===============================================================

/*
=============
arcVec3::VectorNormalize
=============
*/
arcVec3 arcVec3::VectorNormalize( arcVec3 v ) {
	float sqrLength, invLength;

	sqrLength = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

	if ( sqrLength ) {
		// writing it this way allows gcc to recognize that rsqrt can be used
		invLength = 1/( float )arcMath::Sqrt( sqrLength );
		// sqrt(sqrLength) = length * (1 / sqrt(sqrLength) )
		sqrLength *= invLength;
		v[0] *= invLength;
		v[1] *= invLength;
		v[2] *= invLength;
	}

	return sqrLength;
}

/*
=============
arcVec3::Rotate
=============
*/
void arcVec3::Rotate( arcVec3 in, arcMat3 matrix[3], arcVec3 out ) {
	out[0] = Dot( in, matrix[0] );
	out[1] = Dot( in, matrix[1] );
	out[2] = Dot( in, matrix[2] );
}

/*
=============
arcVec3::ToYaw
=============
*/
float arcVec3::ToYaw( void ) const {
	float yaw;

	if ( ( y == 0.0f ) && ( x == 0.0f ) ) {
		yaw = 0.0f;
	} else {
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}
	}

	return yaw;
}

/*
=============
arcVec3::ToPitch
=============
*/
float arcVec3::ToPitch( void ) const {
	float	forward;
	float	pitch;

	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		forward = ( float )arcMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( arcMath::ATan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return pitch;
}

/*
=============
arcVec3::ToRadians
=============
*/
TeKRadians arcVec3::ToRadians( void ) const {
	float forward;
	float yaw;
	float pitch;

	if ( !x && !y ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = arcMath::HALF_PI;
		} else {
			pitch = arcMath::THREEFOURTHS_PI;
		}
	} else {
		yaw = arcMath::ATan( y, x );
		if ( yaw < 0.0f )
		{
			yaw += arcMath::TWO_PI;
		}

		forward = ( float )arcMath::Sqrt( x * x + y * y );
		pitch = arcMath::ATan( z, forward );
		if ( pitch < 0.0f ) {
			pitch += arcMath::TWO_PI;
		}
	}

	return( rvAngles( -pitch, yaw, 0.0f ) );
}

/*
=============
arcVec3::ToAngles
=============
*/
arcAngles arcVec3::ToAngles( void ) const {
	float forward;
	float yaw;
	float pitch;

	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )arcMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return arcAngles( -pitch, yaw, 0.0f );
}

/*
=============
arcVec3::ToPolar
=============
*/
idPolar3 arcVec3::ToPolar( void ) const {
	float forward;
	float yaw;
	float pitch;

	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )arcMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}
	return idPolar3( arcMath::Sqrt( x * x + y * y + z * z ), yaw, -pitch );
}

/*
=============
arcVec3::ToMat3
=============
*/
arcMat3 arcVec3::ToMat3( void ) const {
	arcMat3	mat;
	float	d;

	mat[0] = *this;
	d = x * x + y * y;
	if ( !d ) {
		mat[1][0] = 1.0f;
		mat[1][1] = 0.0f;
		mat[1][2] = 0.0f;
	} else {
		d = arcMath::InvSqrt( d );
		mat[1][0] = -y * d;
		mat[1][1] = x * d;
		mat[1][2] = 0.0f;
	}
	mat[2] = Cross( mat[1] );

	return mat;
}

/*
=============
arcVec3::ToString
=============
*/
const char *arcVec3::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void arcVec3::Lerp( const arcVec3 &v1, const arcVec3 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}

/*
=============
SLerp

Spherical linear interpolation from v1 to v2.
Vectors are expected to be normalized.
=============
*/
#define LERP_DELTA 1e-6
void arcVec3::SLerp( const arcVec3 &v1, const arcVec3 &v2, const float t ) {
	float omega, cosom, sinom, scale0, scale1;

	if ( t <= 0.0f ) {
		(*this) = v1;
		return;
	} else if ( t >= 1.0f ) {
		(*this) = v2;
		return;
	}

	cosom = v1 * v2;
	if ( ( 1.0f - cosom ) > LERP_DELTA ) {
		omega = acos( cosom );
		sinom = sin( omega );
		scale0 = sin( ( 1.0f - t ) * omega ) / sinom;
		scale1 = sin( t * omega ) / sinom;
	} else {
		scale0 = 1.0f - t;
		scale1 = t;
	}

	(*this) = ( v1 * scale0 + v2 * scale1 );
}

/*
=============
LerpAngle

from older libraries.
=============
*/
float arcVec3::LerpAngle( const float from, const float to, const float frac ) {
	float a;

	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * ( to - from );

	return a;
}

/*
=============
ProjectSelfOntoSphere

Projects the z component onto a sphere.
=============
*/
void arcVec3::ProjectSelfOntoSphere( const float radius ) {
	float rsqr = radius * radius;
	float len = Length();
	if ( len  < rsqr * 0.5f ) {
		z = sqrt( rsqr - len );
	} else {
		z = rsqr / ( 2.0f * sqrt( len ) );
	}
}

/*
=============
Inverse
inverts the given vector
=============
*/
void arcVec3::Inverse( const arcVec3 v ) {
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void arcVec3::MA( const arcVec3 a, float scale, const arcVec3 b, arcVec3 c ) {
	c[0] = a[0] + scale * b[0];
	c[1] = a[1] + scale * b[1];
	c[2] = a[2] + scale * b[2];
}

void arcVec3::Subtract( const arcVec3 a, const arcVec3 b, arcVec3 out ) {
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
}

void arcVec3::Add( const arcVec3 a, const arcVec3 b, arcVec3 out ) {
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}

void arcVec3::Copy( const arcVec3 in, arcVec3 out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void arcVec3::Scale( const arcVec3 in, const arcVec3 scale, arcVec3 out ) {
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

//===============================================================
//
//	arcVec4
//
//===============================================================

/*
=============
arcVec4::ToString
=============
*/
const char *arcVec4::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void arcVec4::Lerp( const arcVec4 &v1, const arcVec4 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}

void arcVec4::Scale( const arcVec4 in, const arcVec4 scale, arcVec4 out ) {
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
	out[3] = in[3] * scale;
}
//===============================================================
//
//	arcVec5
//
//===============================================================

/*
=============
arcVec5::ToString
=============
*/
const char *arcVec5::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
arcVec5::Lerp
=============
*/
void arcVec5::Lerp( const arcVec5 &v1, const arcVec5 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		x = v1.x + l * ( v2.x - v1.x );
		y = v1.y + l * ( v2.y - v1.y );
		z = v1.z + l * ( v2.z - v1.z );
		s = v1.s + l * ( v2.s - v1.s );
		t = v1.t + l * ( v2.t - v1.t );
	}
}


//===============================================================
//
//	arcVec6
//
//===============================================================

/*
=============
arcVec6::ToString
=============
*/
const char *arcVec6::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

void Vector10Copy( vec3_origin *in, vec3_origin *out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
	out[4] = in[4];
	out[5] = in[5];
	out[6] = in[6];
	out[7] = in[7];
	out[8] = in[8];
	out[9] = in[9];
}
//===============================================================
//
//	arcVecX
//
//===============================================================

float	arcVecX::temp[VECX_MAX_TEMP+4];
float *	arcVecX::tempPtr = (float *) ( ( ( int ) arcVecX::temp + 15 ) & ~15 );
int		arcVecX::tempIndex = 0;

/*
=============
arcVecX::ToString
=============
*/
const char *arcVecX::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

#include "../Lib.h"
#pragma hdrstop

anVec2 vec2_origin( 0.0f, 0.0f );
anVec3 vec3_origin( 0.0f, 0.0f, 0.0f );
anVec4 vec4_origin( 0.0f, 0.0f, 0.0f, 0.0f );
anVec5 vec5_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
anVec6 vec6_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
anVec6 vec6_infinity( anMath::INFINITY, anMath::INFINITY, anMath::INFINITY, anMath::INFINITY, anMath::INFINITY, anMath::INFINITY );


//===============================================================
//
//	anVec2
//
//===============================================================

/*
=============
anVec2::ToString
=============
*/
const char *anVec2::ToString( int precision ) const {
	return anString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void anVec2::Lerp( const anVec2 &v1, const anVec2 &v2, const float l ) {
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
//	anVec3
//
//===============================================================

/*
=============
anVec3::VectorNormalize
=============
*/
anVec3 anVec3::VectorNormalize( anVec3 v ) {
	float sqrLength, invLength;

	sqrLength = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

	if ( sqrLength ) {
		// writing it this way allows gcc to recognize that rsqrt can be used
		invLength = 1/( float )anMath::Sqrt( sqrLength );
		// sqrt( sqrLength) = length * (1 / sqrt( sqrLength) )
		sqrLength *= invLength;
		v[0] *= invLength;
		v[1] *= invLength;
		v[2] *= invLength;
	}

	return sqrLength;
}

/*
=============
anVec3::Rotate
=============
*/
void anVec3::Rotate( anVec3 in, anMat3 matrix[3], anVec3 out ) {
	out[0] = Dot( in, matrix[0] );
	out[1] = Dot( in, matrix[1] );
	out[2] = Dot( in, matrix[2] );
}

/*
=============
anVec3::ToYaw
=============
*/
float anVec3::ToYaw( void ) const {
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
anVec3::ToPitch
=============
*/
float anVec3::ToPitch( void ) const {
	float	forward;
	float	pitch;

	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		forward = ( float )anMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( anMath::ATan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return pitch;
}

/*
=============
anVec3::ToRadians
=============
*/
TeKRadians anVec3::ToRadians( void ) const {
	float forward;
	float yaw;
	float pitch;

	if ( !x && !y ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = anMath::HALF_PI;
		} else {
			pitch = anMath::THREEFOURTHS_PI;
		}
	} else {
		yaw = anMath::ATan( y, x );
		if ( yaw < 0.0f )
		{
			yaw += anMath::TWO_PI;
		}

		forward = ( float )anMath::Sqrt( x * x + y * y );
		pitch = anMath::ATan( z, forward );
		if ( pitch < 0.0f ) {
			pitch += anMath::TWO_PI;
		}
	}

	return( rvAngles( -pitch, yaw, 0.0f ) );
}

/*
=============
anVec3::ToAngles
=============
*/
anAngles anVec3::ToAngles( void ) const {
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

		forward = ( float )anMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return anAngles( -pitch, yaw, 0.0f );
}

/*
=============
anVec3::ToPolar
=============
*/
anPolar3 anVec3::ToPolar( void ) const {
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

		forward = ( float )anMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}
	return anPolar3( anMath::Sqrt( x * x + y * y + z * z ), yaw, -pitch );
}

/*
=============
anVec3::ToMat3
=============
*/
anMat3 anVec3::ToMat3( void ) const {
	anMat3	mat;
	float	d;

	mat[0] = *this;
	d = x * x + y * y;
	if ( !d ) {
		mat[1][0] = 1.0f;
		mat[1][1] = 0.0f;
		mat[1][2] = 0.0f;
	} else {
		d = anMath::InvSqrt( d );
		mat[1][0] = -y * d;
		mat[1][1] = x * d;
		mat[1][2] = 0.0f;
	}
	mat[2] = Cross( mat[1] );

	return mat;
}

/*
=============
anVec3::ToString
=============
*/
const char *anVec3::ToString( int precision ) const {
	return anString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void anVec3::Lerp( const anVec3 &v1, const anVec3 &v2, const float l ) {
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
void anVec3::SLerp( const anVec3 &v1, const anVec3 &v2, const float t ) {
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
float anVec3::LerpAngle( const float from, const float to, const float frac ) {
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
void anVec3::ProjectSelfOntoSphere( const float radius ) {
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
void anVec3::Inverse( const anVec3 v ) {
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void anVec3::MA( const anVec3 a, float scale, const anVec3 b, anVec3 c ) {
	c[0] = a[0] + scale * b[0];
	c[1] = a[1] + scale * b[1];
	c[2] = a[2] + scale * b[2];
}

void anVec3::Subtract( const anVec3 a, const anVec3 b, anVec3 out ) {
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
}

void anVec3::Add( const anVec3 a, const anVec3 b, anVec3 out ) {
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}

void anVec3::Copy( const anVec3 in, anVec3 out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void anVec3::Scale( const anVec3 in, const anVec3 scale, anVec3 out ) {
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

//===============================================================
//
//	anVec4
//
//===============================================================

/*
=============
anVec4::ToString
=============
*/
const char *anVec4::ToString( int precision ) const {
	return anString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void anVec4::Lerp( const anVec4 &v1, const anVec4 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}

void anVec4::Scale( const anVec4 in, const anVec4 scale, anVec4 out ) {
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
	out[3] = in[3] * scale;
}
//===============================================================
//
//	anVec5
//
//===============================================================

/*
=============
anVec5::ToString
=============
*/
const char *anVec5::ToString( int precision ) const {
	return anString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
anVec5::Lerp
=============
*/
void anVec5::Lerp( const anVec5 &v1, const anVec5 &v2, const float l ) {
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
//	anVec6
//
//===============================================================

/*
=============
anVec6::ToString
=============
*/
const char *anVec6::ToString( int precision ) const {
	return anString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

//===============================================================
//
//	anVecX
//
//===============================================================

float	anVecX::temp[VECX_MAX_TEMP+4];
float *	anVecX::tempPtr = (float *) ( ( ( int ) anVecX::temp + 15 ) & ~15 );
int		anVecX::tempIndex = 0;

/*
=============
anVecX::ToString
=============
*/
const char *anVecX::ToString( int precision ) const {
	return anString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

void anVecX::Copy( vec3_origin *in, vec3_origin *out ) {
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
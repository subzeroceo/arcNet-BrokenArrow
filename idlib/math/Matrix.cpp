#include "../precompiled.h"
#pragma hdrstop


//===============================================================
//
//	arcMat2
//
//===============================================================

arcMat2 mat2_zero( arcVec2( 0, 0 ), arcVec2( 0, 0 ) );
arcMat2 mat2_identity( arcVec2( 1, 0 ), arcVec2( 0, 1 ) );

/*
============
arcMat2::InverseSelf
============
*/
bool arcMat2::InverseSelf( void ) {
	// 2+4 = 6 multiplications
	//		 1 division
	double det, invDet, a;

	det = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	a = mat[0][0];
	mat[0][0] =   mat[1][1] * invDet;
	mat[0][1] = - mat[0][1] * invDet;
	mat[1][0] = - mat[1][0] * invDet;
	mat[1][1] =   a * invDet;

	return true;
}

/*
============
arcMat2::InverseFastSelf
============
*/
bool arcMat2::InverseFastSelf( void ) {
#if 1
	// 2+4 = 6 multiplications
	//		 1 division
	double det, invDet, a;

	det = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	a = mat[0][0];
	mat[0][0] =   mat[1][1] * invDet;
	mat[0][1] = - mat[0][1] * invDet;
	mat[1][0] = - mat[1][0] * invDet;
	mat[1][1] =   a * invDet;

	return true;
#else
	// 2*4 = 8 multiplications
	//		 2 division
	float *mat = reinterpret_cast<float *>( this );
	double d, di;
	float s;

	di = mat[0];
	s = di;
	mat[0*2+0] = d = 1.0f / di;
	mat[0*2+1] *= d;
	d = -d;
	mat[1*2+0] *= d;
	d = mat[1*2+0] * di;
	mat[1*2+1] += mat[0*2+1] * d;
	di = mat[1*2+1];
	s *= di;
	mat[1*2+1] = d = 1.0f / di;
	mat[1*2+0] *= d;
	d = -d;
	mat[0*2+1] *= d;
	d = mat[0*2+1] * di;
	mat[0*2+0] += mat[1*2+0] * d;

	return ( s != 0.0f && !FLOAT_IS_NAN( s ) );
#endif
}

/*
=============
arcMat2::ToString
=============
*/
const char *arcMat2::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}


//===============================================================
//
//	arcMat3
//
//===============================================================

arcMat3 mat3_zero( arcVec3( 0, 0, 0 ), arcVec3( 0, 0, 0 ), arcVec3( 0, 0, 0 ) );
arcMat3 mat3_identity( arcVec3( 1, 0, 0 ), arcVec3( 0, 1, 0 ), arcVec3( 0, 0, 1 ) );

/*
============
arcMat3::ToAngles
============
*/
arcAngles arcMat3::ToAngles( void ) const {
	arcAngles	angles;

	float sp = mat[ 0 ][ 2 ];

	// cap off our sin value so that we don't get any NANs
	if ( sp > 1.0f ) {
		sp = 1.0f;
	} else if ( sp < -1.0f ) {
		sp = -1.0f;
	}

	double theta = -asin( sp );
	double cp = cos( theta );

	if ( cp > 8192.0f * arcMath::FLT_EPSILON ) {
		angles.pitch	= RAD2DEG( theta );
		angles.yaw		= RAD2DEG( atan2( mat[ 0 ][ 1 ], mat[ 0 ][ 0 ] ) );
		angles.roll		= RAD2DEG( atan2( mat[ 1 ][ 2 ], mat[ 2 ][ 2 ] ) );
	} else {
		angles.pitch	= RAD2DEG( theta );
		angles.yaw		= RAD2DEG( -atan2( mat[ 1 ][ 0 ], mat[ 1 ][ 1 ] ) );
		angles.roll		= 0;
	}
	return angles;
}

/*
============
arcMat3::ToQuat
============
*/
arcQuats arcMat3::ToQuat( void ) const {
	arcQuats q;

	static int 	next[ 3 ] = { 1, 2, 0 };
	float trace = mat[ 0 ][ 0 ] + mat[ 1 ][ 1 ] + mat[ 2 ][ 2 ];

	if ( trace > 0.0f ) {
		float t = trace + 1.0f;
		float s = arcMath::InvSqrt( t ) * 0.5f;

		q[3] = s * t;
		q[0] = ( mat[ 2 ][ 1 ] - mat[ 1 ][ 2 ] ) * s;
		q[1] = ( mat[ 0 ][ 2 ] - mat[ 2 ][ 0 ] ) * s;
		q[2] = ( mat[ 1 ][ 0 ] - mat[ 0 ][ 1 ] ) * s;
	} else {
		int i = 0;
		if ( mat[ 1 ][ 1 ] > mat[ 0 ][ 0 ] ) {
			i = 1;
		}
		if ( mat[ 2 ][ 2 ] > mat[ i ][ i ] ) {
			i = 2;
		}

		int j = next[ i ];
		int k = next[ j ];

		float t = ( mat[ i ][ i ] - ( mat[ j ][ j ] + mat[ k ][ k ] ) ) + 1.0f;
		float s = arcMath::InvSqrt( t ) * 0.5f;

		q[i] = s * t;
		q[3] = ( mat[ k ][ j ] - mat[ j ][ k ] ) * s;
		q[j] = ( mat[ j ][ i ] + mat[ i ][ j ] ) * s;
		q[k] = ( mat[ k ][ i ] + mat[ i ][ k ] ) * s;
	}

	return q;
}

/*
============
arcMat3::ToCQuat
============
*/
arcCQuats arcMat3::ToCQuat( void ) const {
	arcQuats q = ToQuat();
	if ( q.w < 0.0f ) {
		return arcCQuats( -q.x, -q.y, -q.z );
	}
	return arcCQuats( q.x, q.y, q.z );
}

/*
============
arcMat3::ToRotation
============
*/
arcRotate arcMat3::ToRotation( void ) const {
	arcRotate	r;
	float		trace;
	float		s;
	float		t;
	int     	i;
	int			j;
	int			k;
	static int 	next[ 3 ] = { 1, 2, 0 };

	trace = mat[ 0 ][ 0 ] + mat[ 1 ][ 1 ] + mat[ 2 ][ 2 ];
	if ( trace > 0.0f ) {

		t = trace + 1.0f;
		s = arcMath::InvSqrt( t ) * 0.5f;

		r.angle = s * t;
		r.vec[0] = ( mat[ 2 ][ 1 ] - mat[ 1 ][ 2 ] ) * s;
		r.vec[1] = ( mat[ 0 ][ 2 ] - mat[ 2 ][ 0 ] ) * s;
		r.vec[2] = ( mat[ 1 ][ 0 ] - mat[ 0 ][ 1 ] ) * s;
	} else {
		i = 0;
		if ( mat[ 1 ][ 1 ] > mat[ 0 ][ 0 ] ) {
			i = 1;
		}
		if ( mat[ 2 ][ 2 ] > mat[ i ][ i ] ) {
			i = 2;
		}
		j = next[ i ];
		k = next[ j ];

		t = ( mat[ i ][ i ] - ( mat[ j ][ j ] + mat[ k ][ k ] ) ) + 1.0f;
		s = arcMath::InvSqrt( t ) * 0.5f;

		r.vec[i]	= s * t;
		r.angle		= ( mat[ k ][ j ] - mat[ j ][ k ] ) * s;
		r.vec[j]	= ( mat[ j ][ i ] + mat[ i ][ j ] ) * s;
		r.vec[k]	= ( mat[ k ][ i ] + mat[ i ][ k ] ) * s;
	}
	r.angle = arcMath::ACos( r.angle );
	if ( arcMath::Fabs( r.angle ) < 1e-10f ) {
		r.vec.Set( 0.0f, 0.0f, 1.0f );
		r.angle = 0.0f;
	} else {
		//vec *= (1.0f / sin( angle ) );
		r.vec.Normalize();
		r.vec.FixDegenerateNormal();
		r.angle *= 2.0f * arcMath::M_RAD2DEG;
	}

	r.origin.Zero();
	r.axis = *this;
	r.axisValid = true;
	return r;
}

/*
=================
arcMat3::ToAngularVelocity
=================
*/
arcVec3 arcMat3::ToAngularVelocity( void ) const {
	arcRotate rotation = ToRotation();
	return rotation.GetVec() * DEG2RAD( rotation.GetAngle() );
}

/*
============
arcMat3::Determinant
============
*/
float arcMat3::Determinant( void ) const {
	float det2_12_01 = mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0];
	float det2_12_02 = mat[1][0] * mat[2][2] - mat[1][2] * mat[2][0];
	float det2_12_12 = mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1];

	return mat[0][0] * det2_12_12 - mat[0][1] * det2_12_02 + mat[0][2] * det2_12_01;
}

/*
============
arcMat3::InverseSelf
============
*/
bool arcMat3::InverseSelf( void ) {
	// 18+3+9 = 30 multiplications
	//			 1 division
	arcMat3 inverse;
	double det, invDet;

	inverse[0][0] = mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1];
	inverse[1][0] = mat[1][2] * mat[2][0] - mat[1][0] * mat[2][2];
	inverse[2][0] = mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0];

	det = mat[0][0] * inverse[0][0] + mat[0][1] * inverse[1][0] + mat[0][2] * inverse[2][0];

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	inverse[0][1] = mat[0][2] * mat[2][1] - mat[0][1] * mat[2][2];
	inverse[0][2] = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
	inverse[1][1] = mat[0][0] * mat[2][2] - mat[0][2] * mat[2][0];
	inverse[1][2] = mat[0][2] * mat[1][0] - mat[0][0] * mat[1][2];
	inverse[2][1] = mat[0][1] * mat[2][0] - mat[0][0] * mat[2][1];
	inverse[2][2] = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

	mat[0][0] = inverse[0][0] * invDet;
	mat[0][1] = inverse[0][1] * invDet;
	mat[0][2] = inverse[0][2] * invDet;

	mat[1][0] = inverse[1][0] * invDet;
	mat[1][1] = inverse[1][1] * invDet;
	mat[1][2] = inverse[1][2] * invDet;

	mat[2][0] = inverse[2][0] * invDet;
	mat[2][1] = inverse[2][1] * invDet;
	mat[2][2] = inverse[2][2] * invDet;

	return true;
}

/*
============
arcMat3::InverseFastSelf
============
*/
bool arcMat3::InverseFastSelf( void ) {
#if 1
	// 18+3+9 = 30 multiplications
	//			 1 division
	arcMat3 inverse;
	double det, invDet;

	inverse[0][0] = mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1];
	inverse[1][0] = mat[1][2] * mat[2][0] - mat[1][0] * mat[2][2];
	inverse[2][0] = mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0];

	det = mat[0][0] * inverse[0][0] + mat[0][1] * inverse[1][0] + mat[0][2] * inverse[2][0];

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	inverse[0][1] = mat[0][2] * mat[2][1] - mat[0][1] * mat[2][2];
	inverse[0][2] = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
	inverse[1][1] = mat[0][0] * mat[2][2] - mat[0][2] * mat[2][0];
	inverse[1][2] = mat[0][2] * mat[1][0] - mat[0][0] * mat[1][2];
	inverse[2][1] = mat[0][1] * mat[2][0] - mat[0][0] * mat[2][1];
	inverse[2][2] = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

	mat[0][0] = inverse[0][0] * invDet;
	mat[0][1] = inverse[0][1] * invDet;
	mat[0][2] = inverse[0][2] * invDet;

	mat[1][0] = inverse[1][0] * invDet;
	mat[1][1] = inverse[1][1] * invDet;
	mat[1][2] = inverse[1][2] * invDet;

	mat[2][0] = inverse[2][0] * invDet;
	mat[2][1] = inverse[2][1] * invDet;
	mat[2][2] = inverse[2][2] * invDet;

	return true;
#elif 0
	// 3*10 = 30 multiplications
	//		   3 divisions
	float *mat = reinterpret_cast<float *>( this );
	float s;
	double d, di;

	di = mat[0];
	s = di;
	mat[0] = d = 1.0f / di;
	mat[1] *= d;
	mat[2] *= d;
	d = -d;
	mat[3] *= d;
	mat[6] *= d;
	d = mat[3] * di;
	mat[4] += mat[1] * d;
	mat[5] += mat[2] * d;
	d = mat[6] * di;
	mat[7] += mat[1] * d;
	mat[8] += mat[2] * d;
	di = mat[4];
	s *= di;
	mat[4] = d = 1.0f / di;
	mat[3] *= d;
	mat[5] *= d;
	d = -d;
	mat[1] *= d;
	mat[7] *= d;
	d = mat[1] * di;
	mat[0] += mat[3] * d;
	mat[2] += mat[5] * d;
	d = mat[7] * di;
	mat[6] += mat[3] * d;
	mat[8] += mat[5] * d;
	di = mat[8];
	s *= di;
	mat[8] = d = 1.0f / di;
	mat[6] *= d;
	mat[7] *= d;
	d = -d;
	mat[2] *= d;
	mat[5] *= d;
	d = mat[2] * di;
	mat[0] += mat[6] * d;
	mat[1] += mat[7] * d;
	d = mat[5] * di;
	mat[3] += mat[6] * d;
	mat[4] += mat[7] * d;

	return ( s != 0.0f && !FLOAT_IS_NAN( s ) );
#else
	//	4*2+4*4 = 24 multiplications
	//		2*1 =  2 divisions
	arcMat2 r0;
	float r1[2], r2[2], r3;
	float det, invDet;
	float *mat = reinterpret_cast<float *>( this );

	// r0 = m0.Inverse();	// 2x2
	det = mat[0*3+0] * mat[1*3+1] - mat[0*3+1] * mat[1*3+0];

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	r0[0][0] =   mat[1*3+1] * invDet;
	r0[0][1] = - mat[0*3+1] * invDet;
	r0[1][0] = - mat[1*3+0] * invDet;
	r0[1][1] =   mat[0*3+0] * invDet;

	// r1 = r0 * m1;		// 2x1 = 2x2 * 2x1
	r1[0] = r0[0][0] * mat[0*3+2] + r0[0][1] * mat[1*3+2];
	r1[1] = r0[1][0] * mat[0*3+2] + r0[1][1] * mat[1*3+2];

	// r2 = m2 * r1;		// 1x1 = 1x2 * 2x1
	r2[0] = mat[2*3+0] * r1[0] + mat[2*3+1] * r1[1];

	// r3 = r2 - m3;		// 1x1 = 1x1 - 1x1
	r3 = r2[0] - mat[2*3+2];

	// r3.InverseSelf();
	if ( arcMath::Fabs( r3 ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	r3 = 1.0f / r3;

	// r2 = m2 * r0;		// 1x2 = 1x2 * 2x2
	r2[0] = mat[2*3+0] * r0[0][0] + mat[2*3+1] * r0[1][0];
	r2[1] = mat[2*3+0] * r0[0][1] + mat[2*3+1] * r0[1][1];

	// m2 = r3 * r2;		// 1x2 = 1x1 * 1x2
	mat[2*3+0] = r3 * r2[0];
	mat[2*3+1] = r3 * r2[1];

	// m0 = r0 - r1 * m2;	// 2x2 - 2x1 * 1x2
	mat[0*3+0] = r0[0][0] - r1[0] * mat[2*3+0];
	mat[0*3+1] = r0[0][1] - r1[0] * mat[2*3+1];
	mat[1*3+0] = r0[1][0] - r1[1] * mat[2*3+0];
	mat[1*3+1] = r0[1][1] - r1[1] * mat[2*3+1];

	// m1 = r1 * r3;		// 2x1 = 2x1 * 1x1
	mat[0*3+2] = r1[0] * r3;
	mat[1*3+2] = r1[1] * r3;

	// m3 = -r3;
	mat[2*3+2] = -r3;

	return true;
#endif
}

/*
============
arcMat3::InertiaTranslate
============
*/
arcMat3 arcMat3::InertiaTranslate( const float mass, const arcVec3 &centerOfMass, const arcVec3 &translation ) const {
	arcMat3 m;
	arcVec3 newCenter;

	newCenter = centerOfMass + translation;

	m[0][0] = mass * ( ( centerOfMass[1] * centerOfMass[1] + centerOfMass[2] * centerOfMass[2] )
				- ( newCenter[1] * newCenter[1] + newCenter[2] * newCenter[2] ) );
	m[1][1] = mass * ( ( centerOfMass[0] * centerOfMass[0] + centerOfMass[2] * centerOfMass[2] )
				- ( newCenter[0] * newCenter[0] + newCenter[2] * newCenter[2] ) );
	m[2][2] = mass * ( ( centerOfMass[0] * centerOfMass[0] + centerOfMass[1] * centerOfMass[1] )
				- ( newCenter[0] * newCenter[0] + newCenter[1] * newCenter[1] ) );

	m[0][1] = m[1][0] = mass * ( newCenter[0] * newCenter[1] - centerOfMass[0] * centerOfMass[1] );
	m[1][2] = m[2][1] = mass * ( newCenter[1] * newCenter[2] - centerOfMass[1] * centerOfMass[2] );
	m[0][2] = m[2][0] = mass * ( newCenter[0] * newCenter[2] - centerOfMass[0] * centerOfMass[2] );

	return (*this) + m;
}

/*
============
arcMat3::InertiaTranslateSelf
============
*/
arcMat3 &arcMat3::InertiaTranslateSelf( const float mass, const arcVec3 &centerOfMass, const arcVec3 &translation ) {
	arcMat3 m;
	arcVec3 newCenter;

	newCenter = centerOfMass + translation;

	m[0][0] = mass * ( ( centerOfMass[1] * centerOfMass[1] + centerOfMass[2] * centerOfMass[2] )
				- ( newCenter[1] * newCenter[1] + newCenter[2] * newCenter[2] ) );
	m[1][1] = mass * ( ( centerOfMass[0] * centerOfMass[0] + centerOfMass[2] * centerOfMass[2] )
				- ( newCenter[0] * newCenter[0] + newCenter[2] * newCenter[2] ) );
	m[2][2] = mass * ( ( centerOfMass[0] * centerOfMass[0] + centerOfMass[1] * centerOfMass[1] )
				- ( newCenter[0] * newCenter[0] + newCenter[1] * newCenter[1] ) );

	m[0][1] = m[1][0] = mass * ( newCenter[0] * newCenter[1] - centerOfMass[0] * centerOfMass[1] );
	m[1][2] = m[2][1] = mass * ( newCenter[1] * newCenter[2] - centerOfMass[1] * centerOfMass[2] );
	m[0][2] = m[2][0] = mass * ( newCenter[0] * newCenter[2] - centerOfMass[0] * centerOfMass[2] );

	(*this) += m;

	return (*this);
}

/*
============
arcMat3::InertiaRotate
============
*/
arcMat3 arcMat3::InertiaRotate( const arcMat3 &rotation ) const {
	// NOTE: the rotation matrix is stored column-major
	return rotation.Transpose() * (*this) * rotation;
}

/*
============
arcMat3::InertiaRotateSelf
============
*/
arcMat3 &arcMat3::InertiaRotateSelf( const arcMat3 &rotation ) {
	// NOTE: the rotation matrix is stored column-major
	*this = rotation.Transpose() * (*this) * rotation;
	return *this;
}

/*
=============
arcMat3::ToString
=============
*/
const char *arcMat3::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

//===============================================================
//
//	arcMat4
//
//===============================================================

arcMat4 mat4_zero( arcVec4( 0, 0, 0, 0 ), arcVec4( 0, 0, 0, 0 ), arcVec4( 0, 0, 0, 0 ), arcVec4( 0, 0, 0, 0 ) );
arcMat4 mat4_identity( arcVec4( 1, 0, 0, 0 ), arcVec4( 0, 1, 0, 0 ), arcVec4( 0, 0, 1, 0 ), arcVec4( 0, 0, 0, 1 ) );

/*
============
arcMat4::Transpose
============
*/
arcMat4 arcMat4::Transpose( void ) const {
	arcMat4	transpose;
	int		i, j;

	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			transpose[ i ][ j ] = mat[ j ][ i ];
        }
	}
	return transpose;
}

/*
============
arcMat4::TransposeSelf
============
*/
arcMat4 &arcMat4::TransposeSelf( void ) {
	float	temp;
	int		i, j;

	for ( i = 0; i < 4; i++ ) {
		for ( j = i + 1; j < 4; j++ ) {
			temp = mat[ i ][ j ];
			mat[ i ][ j ] = mat[ j ][ i ];
			mat[ j ][ i ] = temp;
        }
	}
	return *this;
}

/*
============
arcMat4::Determinant
============
*/
float arcMat4::Determinant( void ) const {

	// 2x2 sub-determinants
	float det2_01_01 = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
	float det2_01_02 = mat[0][0] * mat[1][2] - mat[0][2] * mat[1][0];
	float det2_01_03 = mat[0][0] * mat[1][3] - mat[0][3] * mat[1][0];
	float det2_01_12 = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
	float det2_01_13 = mat[0][1] * mat[1][3] - mat[0][3] * mat[1][1];
	float det2_01_23 = mat[0][2] * mat[1][3] - mat[0][3] * mat[1][2];

	// 3x3 sub-determinants
	float det3_201_012 = mat[2][0] * det2_01_12 - mat[2][1] * det2_01_02 + mat[2][2] * det2_01_01;
	float det3_201_013 = mat[2][0] * det2_01_13 - mat[2][1] * det2_01_03 + mat[2][3] * det2_01_01;
	float det3_201_023 = mat[2][0] * det2_01_23 - mat[2][2] * det2_01_03 + mat[2][3] * det2_01_02;
	float det3_201_123 = mat[2][1] * det2_01_23 - mat[2][2] * det2_01_13 + mat[2][3] * det2_01_12;

	return ( - det3_201_123 * mat[3][0] + det3_201_023 * mat[3][1] - det3_201_013 * mat[3][2] + det3_201_012 * mat[3][3] );
}

/*
============
arcMat4::InverseSelf
============
*/
bool arcMat4::InverseSelf( void ) {
	// 84+4+16 = 104 multiplications
	//			   1 division
	double det, invDet;

	// 2x2 sub-determinants required to calculate 4x4 determinant
	float det2_01_01 = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
	float det2_01_02 = mat[0][0] * mat[1][2] - mat[0][2] * mat[1][0];
	float det2_01_03 = mat[0][0] * mat[1][3] - mat[0][3] * mat[1][0];
	float det2_01_12 = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
	float det2_01_13 = mat[0][1] * mat[1][3] - mat[0][3] * mat[1][1];
	float det2_01_23 = mat[0][2] * mat[1][3] - mat[0][3] * mat[1][2];

	// 3x3 sub-determinants required to calculate 4x4 determinant
	float det3_201_012 = mat[2][0] * det2_01_12 - mat[2][1] * det2_01_02 + mat[2][2] * det2_01_01;
	float det3_201_013 = mat[2][0] * det2_01_13 - mat[2][1] * det2_01_03 + mat[2][3] * det2_01_01;
	float det3_201_023 = mat[2][0] * det2_01_23 - mat[2][2] * det2_01_03 + mat[2][3] * det2_01_02;
	float det3_201_123 = mat[2][1] * det2_01_23 - mat[2][2] * det2_01_13 + mat[2][3] * det2_01_12;

	det = ( - det3_201_123 * mat[3][0] + det3_201_023 * mat[3][1] - det3_201_013 * mat[3][2] + det3_201_012 * mat[3][3] );

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	// remaining 2x2 sub-determinants
	float det2_03_01 = mat[0][0] * mat[3][1] - mat[0][1] * mat[3][0];
	float det2_03_02 = mat[0][0] * mat[3][2] - mat[0][2] * mat[3][0];
	float det2_03_03 = mat[0][0] * mat[3][3] - mat[0][3] * mat[3][0];
	float det2_03_12 = mat[0][1] * mat[3][2] - mat[0][2] * mat[3][1];
	float det2_03_13 = mat[0][1] * mat[3][3] - mat[0][3] * mat[3][1];
	float det2_03_23 = mat[0][2] * mat[3][3] - mat[0][3] * mat[3][2];

	float det2_13_01 = mat[1][0] * mat[3][1] - mat[1][1] * mat[3][0];
	float det2_13_02 = mat[1][0] * mat[3][2] - mat[1][2] * mat[3][0];
	float det2_13_03 = mat[1][0] * mat[3][3] - mat[1][3] * mat[3][0];
	float det2_13_12 = mat[1][1] * mat[3][2] - mat[1][2] * mat[3][1];
	float det2_13_13 = mat[1][1] * mat[3][3] - mat[1][3] * mat[3][1];
	float det2_13_23 = mat[1][2] * mat[3][3] - mat[1][3] * mat[3][2];

	// remaining 3x3 sub-determinants
	float det3_203_012 = mat[2][0] * det2_03_12 - mat[2][1] * det2_03_02 + mat[2][2] * det2_03_01;
	float det3_203_013 = mat[2][0] * det2_03_13 - mat[2][1] * det2_03_03 + mat[2][3] * det2_03_01;
	float det3_203_023 = mat[2][0] * det2_03_23 - mat[2][2] * det2_03_03 + mat[2][3] * det2_03_02;
	float det3_203_123 = mat[2][1] * det2_03_23 - mat[2][2] * det2_03_13 + mat[2][3] * det2_03_12;

	float det3_213_012 = mat[2][0] * det2_13_12 - mat[2][1] * det2_13_02 + mat[2][2] * det2_13_01;
	float det3_213_013 = mat[2][0] * det2_13_13 - mat[2][1] * det2_13_03 + mat[2][3] * det2_13_01;
	float det3_213_023 = mat[2][0] * det2_13_23 - mat[2][2] * det2_13_03 + mat[2][3] * det2_13_02;
	float det3_213_123 = mat[2][1] * det2_13_23 - mat[2][2] * det2_13_13 + mat[2][3] * det2_13_12;

	float det3_301_012 = mat[3][0] * det2_01_12 - mat[3][1] * det2_01_02 + mat[3][2] * det2_01_01;
	float det3_301_013 = mat[3][0] * det2_01_13 - mat[3][1] * det2_01_03 + mat[3][3] * det2_01_01;
	float det3_301_023 = mat[3][0] * det2_01_23 - mat[3][2] * det2_01_03 + mat[3][3] * det2_01_02;
	float det3_301_123 = mat[3][1] * det2_01_23 - mat[3][2] * det2_01_13 + mat[3][3] * det2_01_12;

	mat[0][0] =	- det3_213_123 * invDet;
	mat[1][0] = + det3_213_023 * invDet;
	mat[2][0] = - det3_213_013 * invDet;
	mat[3][0] = + det3_213_012 * invDet;

	mat[0][1] = + det3_203_123 * invDet;
	mat[1][1] = - det3_203_023 * invDet;
	mat[2][1] = + det3_203_013 * invDet;
	mat[3][1] = - det3_203_012 * invDet;

	mat[0][2] = + det3_301_123 * invDet;
	mat[1][2] = - det3_301_023 * invDet;
	mat[2][2] = + det3_301_013 * invDet;
	mat[3][2] = - det3_301_012 * invDet;

	mat[0][3] = - det3_201_123 * invDet;
	mat[1][3] = + det3_201_023 * invDet;
	mat[2][3] = - det3_201_013 * invDet;
	mat[3][3] = + det3_201_012 * invDet;

	return true;
}

/*
============
arcMat4::InverseFastSelf
============
*/
bool arcMat4::InverseFastSelf( void ) {
#if 0
	// 84+4+16 = 104 multiplications
	//			   1 division
	double det, invDet;

	// 2x2 sub-determinants required to calculate 4x4 determinant
	float det2_01_01 = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
	float det2_01_02 = mat[0][0] * mat[1][2] - mat[0][2] * mat[1][0];
	float det2_01_03 = mat[0][0] * mat[1][3] - mat[0][3] * mat[1][0];
	float det2_01_12 = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
	float det2_01_13 = mat[0][1] * mat[1][3] - mat[0][3] * mat[1][1];
	float det2_01_23 = mat[0][2] * mat[1][3] - mat[0][3] * mat[1][2];

	// 3x3 sub-determinants required to calculate 4x4 determinant
	float det3_201_012 = mat[2][0] * det2_01_12 - mat[2][1] * det2_01_02 + mat[2][2] * det2_01_01;
	float det3_201_013 = mat[2][0] * det2_01_13 - mat[2][1] * det2_01_03 + mat[2][3] * det2_01_01;
	float det3_201_023 = mat[2][0] * det2_01_23 - mat[2][2] * det2_01_03 + mat[2][3] * det2_01_02;
	float det3_201_123 = mat[2][1] * det2_01_23 - mat[2][2] * det2_01_13 + mat[2][3] * det2_01_12;

	det = ( - det3_201_123 * mat[3][0] + det3_201_023 * mat[3][1] - det3_201_013 * mat[3][2] + det3_201_012 * mat[3][3] );

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	// remaining 2x2 sub-determinants
	float det2_03_01 = mat[0][0] * mat[3][1] - mat[0][1] * mat[3][0];
	float det2_03_02 = mat[0][0] * mat[3][2] - mat[0][2] * mat[3][0];
	float det2_03_03 = mat[0][0] * mat[3][3] - mat[0][3] * mat[3][0];
	float det2_03_12 = mat[0][1] * mat[3][2] - mat[0][2] * mat[3][1];
	float det2_03_13 = mat[0][1] * mat[3][3] - mat[0][3] * mat[3][1];
	float det2_03_23 = mat[0][2] * mat[3][3] - mat[0][3] * mat[3][2];

	float det2_13_01 = mat[1][0] * mat[3][1] - mat[1][1] * mat[3][0];
	float det2_13_02 = mat[1][0] * mat[3][2] - mat[1][2] * mat[3][0];
	float det2_13_03 = mat[1][0] * mat[3][3] - mat[1][3] * mat[3][0];
	float det2_13_12 = mat[1][1] * mat[3][2] - mat[1][2] * mat[3][1];
	float det2_13_13 = mat[1][1] * mat[3][3] - mat[1][3] * mat[3][1];
	float det2_13_23 = mat[1][2] * mat[3][3] - mat[1][3] * mat[3][2];

	// remaining 3x3 sub-determinants
	float det3_203_012 = mat[2][0] * det2_03_12 - mat[2][1] * det2_03_02 + mat[2][2] * det2_03_01;
	float det3_203_013 = mat[2][0] * det2_03_13 - mat[2][1] * det2_03_03 + mat[2][3] * det2_03_01;
	float det3_203_023 = mat[2][0] * det2_03_23 - mat[2][2] * det2_03_03 + mat[2][3] * det2_03_02;
	float det3_203_123 = mat[2][1] * det2_03_23 - mat[2][2] * det2_03_13 + mat[2][3] * det2_03_12;

	float det3_213_012 = mat[2][0] * det2_13_12 - mat[2][1] * det2_13_02 + mat[2][2] * det2_13_01;
	float det3_213_013 = mat[2][0] * det2_13_13 - mat[2][1] * det2_13_03 + mat[2][3] * det2_13_01;
	float det3_213_023 = mat[2][0] * det2_13_23 - mat[2][2] * det2_13_03 + mat[2][3] * det2_13_02;
	float det3_213_123 = mat[2][1] * det2_13_23 - mat[2][2] * det2_13_13 + mat[2][3] * det2_13_12;

	float det3_301_012 = mat[3][0] * det2_01_12 - mat[3][1] * det2_01_02 + mat[3][2] * det2_01_01;
	float det3_301_013 = mat[3][0] * det2_01_13 - mat[3][1] * det2_01_03 + mat[3][3] * det2_01_01;
	float det3_301_023 = mat[3][0] * det2_01_23 - mat[3][2] * det2_01_03 + mat[3][3] * det2_01_02;
	float det3_301_123 = mat[3][1] * det2_01_23 - mat[3][2] * det2_01_13 + mat[3][3] * det2_01_12;

	mat[0][0] =	- det3_213_123 * invDet;
	mat[1][0] = + det3_213_023 * invDet;
	mat[2][0] = - det3_213_013 * invDet;
	mat[3][0] = + det3_213_012 * invDet;

	mat[0][1] = + det3_203_123 * invDet;
	mat[1][1] = - det3_203_023 * invDet;
	mat[2][1] = + det3_203_013 * invDet;
	mat[3][1] = - det3_203_012 * invDet;

	mat[0][2] = + det3_301_123 * invDet;
	mat[1][2] = - det3_301_023 * invDet;
	mat[2][2] = + det3_301_013 * invDet;
	mat[3][2] = - det3_301_012 * invDet;

	mat[0][3] = - det3_201_123 * invDet;
	mat[1][3] = + det3_201_023 * invDet;
	mat[2][3] = - det3_201_013 * invDet;
	mat[3][3] = + det3_201_012 * invDet;

	return true;
#elif 0
	// 4*18 = 72 multiplications
	//		   4 divisions
	float *mat = reinterpret_cast<float *>( this );
	float s;
	double d, di;

	di = mat[0];
	s = di;
	mat[0] = d = 1.0f / di;
	mat[1] *= d;
	mat[2] *= d;
	mat[3] *= d;
	d = -d;
	mat[4] *= d;
	mat[8] *= d;
	mat[12] *= d;
	d = mat[4] * di;
	mat[5] += mat[1] * d;
	mat[6] += mat[2] * d;
	mat[7] += mat[3] * d;
	d = mat[8] * di;
	mat[9] += mat[1] * d;
	mat[10] += mat[2] * d;
	mat[11] += mat[3] * d;
	d = mat[12] * di;
	mat[13] += mat[1] * d;
	mat[14] += mat[2] * d;
	mat[15] += mat[3] * d;
	di = mat[5];
	s *= di;
	mat[5] = d = 1.0f / di;
	mat[4] *= d;
	mat[6] *= d;
	mat[7] *= d;
	d = -d;
	mat[1] *= d;
	mat[9] *= d;
	mat[13] *= d;
	d = mat[1] * di;
	mat[0] += mat[4] * d;
	mat[2] += mat[6] * d;
	mat[3] += mat[7] * d;
	d = mat[9] * di;
	mat[8] += mat[4] * d;
	mat[10] += mat[6] * d;
	mat[11] += mat[7] * d;
	d = mat[13] * di;
	mat[12] += mat[4] * d;
	mat[14] += mat[6] * d;
	mat[15] += mat[7] * d;
	di = mat[10];
	s *= di;
	mat[10] = d = 1.0f / di;
	mat[8] *= d;
	mat[9] *= d;
	mat[11] *= d;
	d = -d;
	mat[2] *= d;
	mat[6] *= d;
	mat[14] *= d;
	d = mat[2] * di;
	mat[0] += mat[8] * d;
	mat[1] += mat[9] * d;
	mat[3] += mat[11] * d;
	d = mat[6] * di;
	mat[4] += mat[8] * d;
	mat[5] += mat[9] * d;
	mat[7] += mat[11] * d;
	d = mat[14] * di;
	mat[12] += mat[8] * d;
	mat[13] += mat[9] * d;
	mat[15] += mat[11] * d;
	di = mat[15];
	s *= di;
	mat[15] = d = 1.0f / di;
	mat[12] *= d;
	mat[13] *= d;
	mat[14] *= d;
	d = -d;
	mat[3] *= d;
	mat[7] *= d;
	mat[11] *= d;
	d = mat[3] * di;
	mat[0] += mat[12] * d;
	mat[1] += mat[13] * d;
	mat[2] += mat[14] * d;
	d = mat[7] * di;
	mat[4] += mat[12] * d;
	mat[5] += mat[13] * d;
	mat[6] += mat[14] * d;
	d = mat[11] * di;
	mat[8] += mat[12] * d;
	mat[9] += mat[13] * d;
	mat[10] += mat[14] * d;

	return ( s != 0.0f && !FLOAT_IS_NAN( s ) );
#else
	//	6*8+2*6 = 60 multiplications
	//		2*1 =  2 divisions
	arcMat2 r0, r1, r2, r3;
	float a, det, invDet;
	float *mat = reinterpret_cast<float *>( this );

	// r0 = m0.Inverse();
	det = mat[0*4+0] * mat[1*4+1] - mat[0*4+1] * mat[1*4+0];

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	r0[0][0] =   mat[1*4+1] * invDet;
	r0[0][1] = - mat[0*4+1] * invDet;
	r0[1][0] = - mat[1*4+0] * invDet;
	r0[1][1] =   mat[0*4+0] * invDet;

	// r1 = r0 * m1;
	r1[0][0] = r0[0][0] * mat[0*4+2] + r0[0][1] * mat[1*4+2];
	r1[0][1] = r0[0][0] * mat[0*4+3] + r0[0][1] * mat[1*4+3];
	r1[1][0] = r0[1][0] * mat[0*4+2] + r0[1][1] * mat[1*4+2];
	r1[1][1] = r0[1][0] * mat[0*4+3] + r0[1][1] * mat[1*4+3];

	// r2 = m2 * r1;
	r2[0][0] = mat[2*4+0] * r1[0][0] + mat[2*4+1] * r1[1][0];
	r2[0][1] = mat[2*4+0] * r1[0][1] + mat[2*4+1] * r1[1][1];
	r2[1][0] = mat[3*4+0] * r1[0][0] + mat[3*4+1] * r1[1][0];
	r2[1][1] = mat[3*4+0] * r1[0][1] + mat[3*4+1] * r1[1][1];

	// r3 = r2 - m3;
	r3[0][0] = r2[0][0] - mat[2*4+2];
	r3[0][1] = r2[0][1] - mat[2*4+3];
	r3[1][0] = r2[1][0] - mat[3*4+2];
	r3[1][1] = r2[1][1] - mat[3*4+3];

	// r3.InverseSelf();
	det = r3[0][0] * r3[1][1] - r3[0][1] * r3[1][0];

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	a = r3[0][0];
	r3[0][0] =   r3[1][1] * invDet;
	r3[0][1] = - r3[0][1] * invDet;
	r3[1][0] = - r3[1][0] * invDet;
	r3[1][1] =   a * invDet;

	// r2 = m2 * r0;
	r2[0][0] = mat[2*4+0] * r0[0][0] + mat[2*4+1] * r0[1][0];
	r2[0][1] = mat[2*4+0] * r0[0][1] + mat[2*4+1] * r0[1][1];
	r2[1][0] = mat[3*4+0] * r0[0][0] + mat[3*4+1] * r0[1][0];
	r2[1][1] = mat[3*4+0] * r0[0][1] + mat[3*4+1] * r0[1][1];

	// m2 = r3 * r2;
	mat[2*4+0] = r3[0][0] * r2[0][0] + r3[0][1] * r2[1][0];
	mat[2*4+1] = r3[0][0] * r2[0][1] + r3[0][1] * r2[1][1];
	mat[3*4+0] = r3[1][0] * r2[0][0] + r3[1][1] * r2[1][0];
	mat[3*4+1] = r3[1][0] * r2[0][1] + r3[1][1] * r2[1][1];

	// m0 = r0 - r1 * m2;
	mat[0*4+0] = r0[0][0] - r1[0][0] * mat[2*4+0] - r1[0][1] * mat[3*4+0];
	mat[0*4+1] = r0[0][1] - r1[0][0] * mat[2*4+1] - r1[0][1] * mat[3*4+1];
	mat[1*4+0] = r0[1][0] - r1[1][0] * mat[2*4+0] - r1[1][1] * mat[3*4+0];
	mat[1*4+1] = r0[1][1] - r1[1][0] * mat[2*4+1] - r1[1][1] * mat[3*4+1];

	// m1 = r1 * r3;
	mat[0*4+2] = r1[0][0] * r3[0][0] + r1[0][1] * r3[1][0];
	mat[0*4+3] = r1[0][0] * r3[0][1] + r1[0][1] * r3[1][1];
	mat[1*4+2] = r1[1][0] * r3[0][0] + r1[1][1] * r3[1][0];
	mat[1*4+3] = r1[1][0] * r3[0][1] + r1[1][1] * r3[1][1];

	// m3 = -r3;
	mat[2*4+2] = -r3[0][0];
	mat[2*4+3] = -r3[0][1];
	mat[3*4+2] = -r3[1][0];
	mat[3*4+3] = -r3[1][1];

	return true;
#endif
}

/*
=============
arcMat4::ToString
=============
*/
const char *arcMat4::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}


//===============================================================
//
//	arcMat5
//
//===============================================================

arcMat5 mat5_zero( arcVec5( 0, 0, 0, 0, 0 ), arcVec5( 0, 0, 0, 0, 0 ), arcVec5( 0, 0, 0, 0, 0 ), arcVec5( 0, 0, 0, 0, 0 ), arcVec5( 0, 0, 0, 0, 0 ) );
arcMat5 mat5_identity( arcVec5( 1, 0, 0, 0, 0 ), arcVec5( 0, 1, 0, 0, 0 ), arcVec5( 0, 0, 1, 0, 0 ), arcVec5( 0, 0, 0, 1, 0 ), arcVec5( 0, 0, 0, 0, 1 ) );

/*
============
arcMat5::Transpose
============
*/
arcMat5 arcMat5::Transpose( void ) const {
	arcMat5	transpose;
	int		i, j;

	for ( i = 0; i < 5; i++ ) {
		for ( j = 0; j < 5; j++ ) {
			transpose[ i ][ j ] = mat[ j ][ i ];
        }
	}
	return transpose;
}

/*
============
arcMat5::TransposeSelf
============
*/
arcMat5 &arcMat5::TransposeSelf( void ) {
	float	temp;
	int		i, j;

	for ( i = 0; i < 5; i++ ) {
		for ( j = i + 1; j < 5; j++ ) {
			temp = mat[ i ][ j ];
			mat[ i ][ j ] = mat[ j ][ i ];
			mat[ j ][ i ] = temp;
        }
	}
	return *this;
}

/*
============
arcMat5::Determinant
============
*/
float arcMat5::Determinant( void ) const {

	// 2x2 sub-determinants required to calculate 5x5 determinant
	float det2_34_01 = mat[3][0] * mat[4][1] - mat[3][1] * mat[4][0];
	float det2_34_02 = mat[3][0] * mat[4][2] - mat[3][2] * mat[4][0];
	float det2_34_03 = mat[3][0] * mat[4][3] - mat[3][3] * mat[4][0];
	float det2_34_04 = mat[3][0] * mat[4][4] - mat[3][4] * mat[4][0];
	float det2_34_12 = mat[3][1] * mat[4][2] - mat[3][2] * mat[4][1];
	float det2_34_13 = mat[3][1] * mat[4][3] - mat[3][3] * mat[4][1];
	float det2_34_14 = mat[3][1] * mat[4][4] - mat[3][4] * mat[4][1];
	float det2_34_23 = mat[3][2] * mat[4][3] - mat[3][3] * mat[4][2];
	float det2_34_24 = mat[3][2] * mat[4][4] - mat[3][4] * mat[4][2];
	float det2_34_34 = mat[3][3] * mat[4][4] - mat[3][4] * mat[4][3];

	// 3x3 sub-determinants required to calculate 5x5 determinant
	float det3_234_012 = mat[2][0] * det2_34_12 - mat[2][1] * det2_34_02 + mat[2][2] * det2_34_01;
	float det3_234_013 = mat[2][0] * det2_34_13 - mat[2][1] * det2_34_03 + mat[2][3] * det2_34_01;
	float det3_234_014 = mat[2][0] * det2_34_14 - mat[2][1] * det2_34_04 + mat[2][4] * det2_34_01;
	float det3_234_023 = mat[2][0] * det2_34_23 - mat[2][2] * det2_34_03 + mat[2][3] * det2_34_02;
	float det3_234_024 = mat[2][0] * det2_34_24 - mat[2][2] * det2_34_04 + mat[2][4] * det2_34_02;
	float det3_234_034 = mat[2][0] * det2_34_34 - mat[2][3] * det2_34_04 + mat[2][4] * det2_34_03;
	float det3_234_123 = mat[2][1] * det2_34_23 - mat[2][2] * det2_34_13 + mat[2][3] * det2_34_12;
	float det3_234_124 = mat[2][1] * det2_34_24 - mat[2][2] * det2_34_14 + mat[2][4] * det2_34_12;
	float det3_234_134 = mat[2][1] * det2_34_34 - mat[2][3] * det2_34_14 + mat[2][4] * det2_34_13;
	float det3_234_234 = mat[2][2] * det2_34_34 - mat[2][3] * det2_34_24 + mat[2][4] * det2_34_23;

	// 4x4 sub-determinants required to calculate 5x5 determinant
	float det4_1234_0123 = mat[1][0] * det3_234_123 - mat[1][1] * det3_234_023 + mat[1][2] * det3_234_013 - mat[1][3] * det3_234_012;
	float det4_1234_0124 = mat[1][0] * det3_234_124 - mat[1][1] * det3_234_024 + mat[1][2] * det3_234_014 - mat[1][4] * det3_234_012;
	float det4_1234_0134 = mat[1][0] * det3_234_134 - mat[1][1] * det3_234_034 + mat[1][3] * det3_234_014 - mat[1][4] * det3_234_013;
	float det4_1234_0234 = mat[1][0] * det3_234_234 - mat[1][2] * det3_234_034 + mat[1][3] * det3_234_024 - mat[1][4] * det3_234_023;
	float det4_1234_1234 = mat[1][1] * det3_234_234 - mat[1][2] * det3_234_134 + mat[1][3] * det3_234_124 - mat[1][4] * det3_234_123;

	// determinant of 5x5 matrix
	return mat[0][0] * det4_1234_1234 - mat[0][1] * det4_1234_0234 + mat[0][2] * det4_1234_0134 - mat[0][3] * det4_1234_0124 + mat[0][4] * det4_1234_0123;
}

/*
============
arcMat5::InverseSelf
============
*/
bool arcMat5::InverseSelf( void ) {
	// 280+5+25 = 310 multiplications
	//				1 division
	double det, invDet;

	// 2x2 sub-determinants required to calculate 5x5 determinant
	float det2_34_01 = mat[3][0] * mat[4][1] - mat[3][1] * mat[4][0];
	float det2_34_02 = mat[3][0] * mat[4][2] - mat[3][2] * mat[4][0];
	float det2_34_03 = mat[3][0] * mat[4][3] - mat[3][3] * mat[4][0];
	float det2_34_04 = mat[3][0] * mat[4][4] - mat[3][4] * mat[4][0];
	float det2_34_12 = mat[3][1] * mat[4][2] - mat[3][2] * mat[4][1];
	float det2_34_13 = mat[3][1] * mat[4][3] - mat[3][3] * mat[4][1];
	float det2_34_14 = mat[3][1] * mat[4][4] - mat[3][4] * mat[4][1];
	float det2_34_23 = mat[3][2] * mat[4][3] - mat[3][3] * mat[4][2];
	float det2_34_24 = mat[3][2] * mat[4][4] - mat[3][4] * mat[4][2];
	float det2_34_34 = mat[3][3] * mat[4][4] - mat[3][4] * mat[4][3];

	// 3x3 sub-determinants required to calculate 5x5 determinant
	float det3_234_012 = mat[2][0] * det2_34_12 - mat[2][1] * det2_34_02 + mat[2][2] * det2_34_01;
	float det3_234_013 = mat[2][0] * det2_34_13 - mat[2][1] * det2_34_03 + mat[2][3] * det2_34_01;
	float det3_234_014 = mat[2][0] * det2_34_14 - mat[2][1] * det2_34_04 + mat[2][4] * det2_34_01;
	float det3_234_023 = mat[2][0] * det2_34_23 - mat[2][2] * det2_34_03 + mat[2][3] * det2_34_02;
	float det3_234_024 = mat[2][0] * det2_34_24 - mat[2][2] * det2_34_04 + mat[2][4] * det2_34_02;
	float det3_234_034 = mat[2][0] * det2_34_34 - mat[2][3] * det2_34_04 + mat[2][4] * det2_34_03;
	float det3_234_123 = mat[2][1] * det2_34_23 - mat[2][2] * det2_34_13 + mat[2][3] * det2_34_12;
	float det3_234_124 = mat[2][1] * det2_34_24 - mat[2][2] * det2_34_14 + mat[2][4] * det2_34_12;
	float det3_234_134 = mat[2][1] * det2_34_34 - mat[2][3] * det2_34_14 + mat[2][4] * det2_34_13;
	float det3_234_234 = mat[2][2] * det2_34_34 - mat[2][3] * det2_34_24 + mat[2][4] * det2_34_23;

	// 4x4 sub-determinants required to calculate 5x5 determinant
	float det4_1234_0123 = mat[1][0] * det3_234_123 - mat[1][1] * det3_234_023 + mat[1][2] * det3_234_013 - mat[1][3] * det3_234_012;
	float det4_1234_0124 = mat[1][0] * det3_234_124 - mat[1][1] * det3_234_024 + mat[1][2] * det3_234_014 - mat[1][4] * det3_234_012;
	float det4_1234_0134 = mat[1][0] * det3_234_134 - mat[1][1] * det3_234_034 + mat[1][3] * det3_234_014 - mat[1][4] * det3_234_013;
	float det4_1234_0234 = mat[1][0] * det3_234_234 - mat[1][2] * det3_234_034 + mat[1][3] * det3_234_024 - mat[1][4] * det3_234_023;
	float det4_1234_1234 = mat[1][1] * det3_234_234 - mat[1][2] * det3_234_134 + mat[1][3] * det3_234_124 - mat[1][4] * det3_234_123;

	// determinant of 5x5 matrix
	det = mat[0][0] * det4_1234_1234 - mat[0][1] * det4_1234_0234 + mat[0][2] * det4_1234_0134 - mat[0][3] * det4_1234_0124 + mat[0][4] * det4_1234_0123;

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	// remaining 2x2 sub-determinants
	float det2_23_01 = mat[2][0] * mat[3][1] - mat[2][1] * mat[3][0];
	float det2_23_02 = mat[2][0] * mat[3][2] - mat[2][2] * mat[3][0];
	float det2_23_03 = mat[2][0] * mat[3][3] - mat[2][3] * mat[3][0];
	float det2_23_04 = mat[2][0] * mat[3][4] - mat[2][4] * mat[3][0];
	float det2_23_12 = mat[2][1] * mat[3][2] - mat[2][2] * mat[3][1];
	float det2_23_13 = mat[2][1] * mat[3][3] - mat[2][3] * mat[3][1];
	float det2_23_14 = mat[2][1] * mat[3][4] - mat[2][4] * mat[3][1];
	float det2_23_23 = mat[2][2] * mat[3][3] - mat[2][3] * mat[3][2];
	float det2_23_24 = mat[2][2] * mat[3][4] - mat[2][4] * mat[3][2];
	float det2_23_34 = mat[2][3] * mat[3][4] - mat[2][4] * mat[3][3];
	float det2_24_01 = mat[2][0] * mat[4][1] - mat[2][1] * mat[4][0];
	float det2_24_02 = mat[2][0] * mat[4][2] - mat[2][2] * mat[4][0];
	float det2_24_03 = mat[2][0] * mat[4][3] - mat[2][3] * mat[4][0];
	float det2_24_04 = mat[2][0] * mat[4][4] - mat[2][4] * mat[4][0];
	float det2_24_12 = mat[2][1] * mat[4][2] - mat[2][2] * mat[4][1];
	float det2_24_13 = mat[2][1] * mat[4][3] - mat[2][3] * mat[4][1];
	float det2_24_14 = mat[2][1] * mat[4][4] - mat[2][4] * mat[4][1];
	float det2_24_23 = mat[2][2] * mat[4][3] - mat[2][3] * mat[4][2];
	float det2_24_24 = mat[2][2] * mat[4][4] - mat[2][4] * mat[4][2];
	float det2_24_34 = mat[2][3] * mat[4][4] - mat[2][4] * mat[4][3];

	// remaining 3x3 sub-determinants
	float det3_123_012 = mat[1][0] * det2_23_12 - mat[1][1] * det2_23_02 + mat[1][2] * det2_23_01;
	float det3_123_013 = mat[1][0] * det2_23_13 - mat[1][1] * det2_23_03 + mat[1][3] * det2_23_01;
	float det3_123_014 = mat[1][0] * det2_23_14 - mat[1][1] * det2_23_04 + mat[1][4] * det2_23_01;
	float det3_123_023 = mat[1][0] * det2_23_23 - mat[1][2] * det2_23_03 + mat[1][3] * det2_23_02;
	float det3_123_024 = mat[1][0] * det2_23_24 - mat[1][2] * det2_23_04 + mat[1][4] * det2_23_02;
	float det3_123_034 = mat[1][0] * det2_23_34 - mat[1][3] * det2_23_04 + mat[1][4] * det2_23_03;
	float det3_123_123 = mat[1][1] * det2_23_23 - mat[1][2] * det2_23_13 + mat[1][3] * det2_23_12;
	float det3_123_124 = mat[1][1] * det2_23_24 - mat[1][2] * det2_23_14 + mat[1][4] * det2_23_12;
	float det3_123_134 = mat[1][1] * det2_23_34 - mat[1][3] * det2_23_14 + mat[1][4] * det2_23_13;
	float det3_123_234 = mat[1][2] * det2_23_34 - mat[1][3] * det2_23_24 + mat[1][4] * det2_23_23;
	float det3_124_012 = mat[1][0] * det2_24_12 - mat[1][1] * det2_24_02 + mat[1][2] * det2_24_01;
	float det3_124_013 = mat[1][0] * det2_24_13 - mat[1][1] * det2_24_03 + mat[1][3] * det2_24_01;
	float det3_124_014 = mat[1][0] * det2_24_14 - mat[1][1] * det2_24_04 + mat[1][4] * det2_24_01;
	float det3_124_023 = mat[1][0] * det2_24_23 - mat[1][2] * det2_24_03 + mat[1][3] * det2_24_02;
	float det3_124_024 = mat[1][0] * det2_24_24 - mat[1][2] * det2_24_04 + mat[1][4] * det2_24_02;
	float det3_124_034 = mat[1][0] * det2_24_34 - mat[1][3] * det2_24_04 + mat[1][4] * det2_24_03;
	float det3_124_123 = mat[1][1] * det2_24_23 - mat[1][2] * det2_24_13 + mat[1][3] * det2_24_12;
	float det3_124_124 = mat[1][1] * det2_24_24 - mat[1][2] * det2_24_14 + mat[1][4] * det2_24_12;
	float det3_124_134 = mat[1][1] * det2_24_34 - mat[1][3] * det2_24_14 + mat[1][4] * det2_24_13;
	float det3_124_234 = mat[1][2] * det2_24_34 - mat[1][3] * det2_24_24 + mat[1][4] * det2_24_23;
	float det3_134_012 = mat[1][0] * det2_34_12 - mat[1][1] * det2_34_02 + mat[1][2] * det2_34_01;
	float det3_134_013 = mat[1][0] * det2_34_13 - mat[1][1] * det2_34_03 + mat[1][3] * det2_34_01;
	float det3_134_014 = mat[1][0] * det2_34_14 - mat[1][1] * det2_34_04 + mat[1][4] * det2_34_01;
	float det3_134_023 = mat[1][0] * det2_34_23 - mat[1][2] * det2_34_03 + mat[1][3] * det2_34_02;
	float det3_134_024 = mat[1][0] * det2_34_24 - mat[1][2] * det2_34_04 + mat[1][4] * det2_34_02;
	float det3_134_034 = mat[1][0] * det2_34_34 - mat[1][3] * det2_34_04 + mat[1][4] * det2_34_03;
	float det3_134_123 = mat[1][1] * det2_34_23 - mat[1][2] * det2_34_13 + mat[1][3] * det2_34_12;
	float det3_134_124 = mat[1][1] * det2_34_24 - mat[1][2] * det2_34_14 + mat[1][4] * det2_34_12;
	float det3_134_134 = mat[1][1] * det2_34_34 - mat[1][3] * det2_34_14 + mat[1][4] * det2_34_13;
	float det3_134_234 = mat[1][2] * det2_34_34 - mat[1][3] * det2_34_24 + mat[1][4] * det2_34_23;

	// remaining 4x4 sub-determinants
	float det4_0123_0123 = mat[0][0] * det3_123_123 - mat[0][1] * det3_123_023 + mat[0][2] * det3_123_013 - mat[0][3] * det3_123_012;
	float det4_0123_0124 = mat[0][0] * det3_123_124 - mat[0][1] * det3_123_024 + mat[0][2] * det3_123_014 - mat[0][4] * det3_123_012;
	float det4_0123_0134 = mat[0][0] * det3_123_134 - mat[0][1] * det3_123_034 + mat[0][3] * det3_123_014 - mat[0][4] * det3_123_013;
	float det4_0123_0234 = mat[0][0] * det3_123_234 - mat[0][2] * det3_123_034 + mat[0][3] * det3_123_024 - mat[0][4] * det3_123_023;
	float det4_0123_1234 = mat[0][1] * det3_123_234 - mat[0][2] * det3_123_134 + mat[0][3] * det3_123_124 - mat[0][4] * det3_123_123;
	float det4_0124_0123 = mat[0][0] * det3_124_123 - mat[0][1] * det3_124_023 + mat[0][2] * det3_124_013 - mat[0][3] * det3_124_012;
	float det4_0124_0124 = mat[0][0] * det3_124_124 - mat[0][1] * det3_124_024 + mat[0][2] * det3_124_014 - mat[0][4] * det3_124_012;
	float det4_0124_0134 = mat[0][0] * det3_124_134 - mat[0][1] * det3_124_034 + mat[0][3] * det3_124_014 - mat[0][4] * det3_124_013;
	float det4_0124_0234 = mat[0][0] * det3_124_234 - mat[0][2] * det3_124_034 + mat[0][3] * det3_124_024 - mat[0][4] * det3_124_023;
	float det4_0124_1234 = mat[0][1] * det3_124_234 - mat[0][2] * det3_124_134 + mat[0][3] * det3_124_124 - mat[0][4] * det3_124_123;
	float det4_0134_0123 = mat[0][0] * det3_134_123 - mat[0][1] * det3_134_023 + mat[0][2] * det3_134_013 - mat[0][3] * det3_134_012;
	float det4_0134_0124 = mat[0][0] * det3_134_124 - mat[0][1] * det3_134_024 + mat[0][2] * det3_134_014 - mat[0][4] * det3_134_012;
	float det4_0134_0134 = mat[0][0] * det3_134_134 - mat[0][1] * det3_134_034 + mat[0][3] * det3_134_014 - mat[0][4] * det3_134_013;
	float det4_0134_0234 = mat[0][0] * det3_134_234 - mat[0][2] * det3_134_034 + mat[0][3] * det3_134_024 - mat[0][4] * det3_134_023;
	float det4_0134_1234 = mat[0][1] * det3_134_234 - mat[0][2] * det3_134_134 + mat[0][3] * det3_134_124 - mat[0][4] * det3_134_123;
	float det4_0234_0123 = mat[0][0] * det3_234_123 - mat[0][1] * det3_234_023 + mat[0][2] * det3_234_013 - mat[0][3] * det3_234_012;
	float det4_0234_0124 = mat[0][0] * det3_234_124 - mat[0][1] * det3_234_024 + mat[0][2] * det3_234_014 - mat[0][4] * det3_234_012;
	float det4_0234_0134 = mat[0][0] * det3_234_134 - mat[0][1] * det3_234_034 + mat[0][3] * det3_234_014 - mat[0][4] * det3_234_013;
	float det4_0234_0234 = mat[0][0] * det3_234_234 - mat[0][2] * det3_234_034 + mat[0][3] * det3_234_024 - mat[0][4] * det3_234_023;
	float det4_0234_1234 = mat[0][1] * det3_234_234 - mat[0][2] * det3_234_134 + mat[0][3] * det3_234_124 - mat[0][4] * det3_234_123;

	mat[0][0] =  det4_1234_1234 * invDet;
	mat[0][1] = -det4_0234_1234 * invDet;
	mat[0][2] =  det4_0134_1234 * invDet;
	mat[0][3] = -det4_0124_1234 * invDet;
	mat[0][4] =  det4_0123_1234 * invDet;

	mat[1][0] = -det4_1234_0234 * invDet;
	mat[1][1] =  det4_0234_0234 * invDet;
	mat[1][2] = -det4_0134_0234 * invDet;
	mat[1][3] =  det4_0124_0234 * invDet;
	mat[1][4] = -det4_0123_0234 * invDet;

	mat[2][0] =  det4_1234_0134 * invDet;
	mat[2][1] = -det4_0234_0134 * invDet;
	mat[2][2] =  det4_0134_0134 * invDet;
	mat[2][3] = -det4_0124_0134 * invDet;
	mat[2][4] =  det4_0123_0134 * invDet;

	mat[3][0] = -det4_1234_0124 * invDet;
	mat[3][1] =  det4_0234_0124 * invDet;
	mat[3][2] = -det4_0134_0124 * invDet;
	mat[3][3] =  det4_0124_0124 * invDet;
	mat[3][4] = -det4_0123_0124 * invDet;

	mat[4][0] =  det4_1234_0123 * invDet;
	mat[4][1] = -det4_0234_0123 * invDet;
	mat[4][2] =  det4_0134_0123 * invDet;
	mat[4][3] = -det4_0124_0123 * invDet;
	mat[4][4] =  det4_0123_0123 * invDet;

	return true;
}

/*
============
arcMat5::InverseFastSelf
============
*/
bool arcMat5::InverseFastSelf( void ) {
#if 0
	// 280+5+25 = 310 multiplications
	//				1 division
	double det, invDet;

	// 2x2 sub-determinants required to calculate 5x5 determinant
	float det2_34_01 = mat[3][0] * mat[4][1] - mat[3][1] * mat[4][0];
	float det2_34_02 = mat[3][0] * mat[4][2] - mat[3][2] * mat[4][0];
	float det2_34_03 = mat[3][0] * mat[4][3] - mat[3][3] * mat[4][0];
	float det2_34_04 = mat[3][0] * mat[4][4] - mat[3][4] * mat[4][0];
	float det2_34_12 = mat[3][1] * mat[4][2] - mat[3][2] * mat[4][1];
	float det2_34_13 = mat[3][1] * mat[4][3] - mat[3][3] * mat[4][1];
	float det2_34_14 = mat[3][1] * mat[4][4] - mat[3][4] * mat[4][1];
	float det2_34_23 = mat[3][2] * mat[4][3] - mat[3][3] * mat[4][2];
	float det2_34_24 = mat[3][2] * mat[4][4] - mat[3][4] * mat[4][2];
	float det2_34_34 = mat[3][3] * mat[4][4] - mat[3][4] * mat[4][3];

	// 3x3 sub-determinants required to calculate 5x5 determinant
	float det3_234_012 = mat[2][0] * det2_34_12 - mat[2][1] * det2_34_02 + mat[2][2] * det2_34_01;
	float det3_234_013 = mat[2][0] * det2_34_13 - mat[2][1] * det2_34_03 + mat[2][3] * det2_34_01;
	float det3_234_014 = mat[2][0] * det2_34_14 - mat[2][1] * det2_34_04 + mat[2][4] * det2_34_01;
	float det3_234_023 = mat[2][0] * det2_34_23 - mat[2][2] * det2_34_03 + mat[2][3] * det2_34_02;
	float det3_234_024 = mat[2][0] * det2_34_24 - mat[2][2] * det2_34_04 + mat[2][4] * det2_34_02;
	float det3_234_034 = mat[2][0] * det2_34_34 - mat[2][3] * det2_34_04 + mat[2][4] * det2_34_03;
	float det3_234_123 = mat[2][1] * det2_34_23 - mat[2][2] * det2_34_13 + mat[2][3] * det2_34_12;
	float det3_234_124 = mat[2][1] * det2_34_24 - mat[2][2] * det2_34_14 + mat[2][4] * det2_34_12;
	float det3_234_134 = mat[2][1] * det2_34_34 - mat[2][3] * det2_34_14 + mat[2][4] * det2_34_13;
	float det3_234_234 = mat[2][2] * det2_34_34 - mat[2][3] * det2_34_24 + mat[2][4] * det2_34_23;

	// 4x4 sub-determinants required to calculate 5x5 determinant
	float det4_1234_0123 = mat[1][0] * det3_234_123 - mat[1][1] * det3_234_023 + mat[1][2] * det3_234_013 - mat[1][3] * det3_234_012;
	float det4_1234_0124 = mat[1][0] * det3_234_124 - mat[1][1] * det3_234_024 + mat[1][2] * det3_234_014 - mat[1][4] * det3_234_012;
	float det4_1234_0134 = mat[1][0] * det3_234_134 - mat[1][1] * det3_234_034 + mat[1][3] * det3_234_014 - mat[1][4] * det3_234_013;
	float det4_1234_0234 = mat[1][0] * det3_234_234 - mat[1][2] * det3_234_034 + mat[1][3] * det3_234_024 - mat[1][4] * det3_234_023;
	float det4_1234_1234 = mat[1][1] * det3_234_234 - mat[1][2] * det3_234_134 + mat[1][3] * det3_234_124 - mat[1][4] * det3_234_123;

	// determinant of 5x5 matrix
	det = mat[0][0] * det4_1234_1234 - mat[0][1] * det4_1234_0234 + mat[0][2] * det4_1234_0134 - mat[0][3] * det4_1234_0124 + mat[0][4] * det4_1234_0123;

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	// remaining 2x2 sub-determinants
	float det2_23_01 = mat[2][0] * mat[3][1] - mat[2][1] * mat[3][0];
	float det2_23_02 = mat[2][0] * mat[3][2] - mat[2][2] * mat[3][0];
	float det2_23_03 = mat[2][0] * mat[3][3] - mat[2][3] * mat[3][0];
	float det2_23_04 = mat[2][0] * mat[3][4] - mat[2][4] * mat[3][0];
	float det2_23_12 = mat[2][1] * mat[3][2] - mat[2][2] * mat[3][1];
	float det2_23_13 = mat[2][1] * mat[3][3] - mat[2][3] * mat[3][1];
	float det2_23_14 = mat[2][1] * mat[3][4] - mat[2][4] * mat[3][1];
	float det2_23_23 = mat[2][2] * mat[3][3] - mat[2][3] * mat[3][2];
	float det2_23_24 = mat[2][2] * mat[3][4] - mat[2][4] * mat[3][2];
	float det2_23_34 = mat[2][3] * mat[3][4] - mat[2][4] * mat[3][3];
	float det2_24_01 = mat[2][0] * mat[4][1] - mat[2][1] * mat[4][0];
	float det2_24_02 = mat[2][0] * mat[4][2] - mat[2][2] * mat[4][0];
	float det2_24_03 = mat[2][0] * mat[4][3] - mat[2][3] * mat[4][0];
	float det2_24_04 = mat[2][0] * mat[4][4] - mat[2][4] * mat[4][0];
	float det2_24_12 = mat[2][1] * mat[4][2] - mat[2][2] * mat[4][1];
	float det2_24_13 = mat[2][1] * mat[4][3] - mat[2][3] * mat[4][1];
	float det2_24_14 = mat[2][1] * mat[4][4] - mat[2][4] * mat[4][1];
	float det2_24_23 = mat[2][2] * mat[4][3] - mat[2][3] * mat[4][2];
	float det2_24_24 = mat[2][2] * mat[4][4] - mat[2][4] * mat[4][2];
	float det2_24_34 = mat[2][3] * mat[4][4] - mat[2][4] * mat[4][3];

	// remaining 3x3 sub-determinants
	float det3_123_012 = mat[1][0] * det2_23_12 - mat[1][1] * det2_23_02 + mat[1][2] * det2_23_01;
	float det3_123_013 = mat[1][0] * det2_23_13 - mat[1][1] * det2_23_03 + mat[1][3] * det2_23_01;
	float det3_123_014 = mat[1][0] * det2_23_14 - mat[1][1] * det2_23_04 + mat[1][4] * det2_23_01;
	float det3_123_023 = mat[1][0] * det2_23_23 - mat[1][2] * det2_23_03 + mat[1][3] * det2_23_02;
	float det3_123_024 = mat[1][0] * det2_23_24 - mat[1][2] * det2_23_04 + mat[1][4] * det2_23_02;
	float det3_123_034 = mat[1][0] * det2_23_34 - mat[1][3] * det2_23_04 + mat[1][4] * det2_23_03;
	float det3_123_123 = mat[1][1] * det2_23_23 - mat[1][2] * det2_23_13 + mat[1][3] * det2_23_12;
	float det3_123_124 = mat[1][1] * det2_23_24 - mat[1][2] * det2_23_14 + mat[1][4] * det2_23_12;
	float det3_123_134 = mat[1][1] * det2_23_34 - mat[1][3] * det2_23_14 + mat[1][4] * det2_23_13;
	float det3_123_234 = mat[1][2] * det2_23_34 - mat[1][3] * det2_23_24 + mat[1][4] * det2_23_23;
	float det3_124_012 = mat[1][0] * det2_24_12 - mat[1][1] * det2_24_02 + mat[1][2] * det2_24_01;
	float det3_124_013 = mat[1][0] * det2_24_13 - mat[1][1] * det2_24_03 + mat[1][3] * det2_24_01;
	float det3_124_014 = mat[1][0] * det2_24_14 - mat[1][1] * det2_24_04 + mat[1][4] * det2_24_01;
	float det3_124_023 = mat[1][0] * det2_24_23 - mat[1][2] * det2_24_03 + mat[1][3] * det2_24_02;
	float det3_124_024 = mat[1][0] * det2_24_24 - mat[1][2] * det2_24_04 + mat[1][4] * det2_24_02;
	float det3_124_034 = mat[1][0] * det2_24_34 - mat[1][3] * det2_24_04 + mat[1][4] * det2_24_03;
	float det3_124_123 = mat[1][1] * det2_24_23 - mat[1][2] * det2_24_13 + mat[1][3] * det2_24_12;
	float det3_124_124 = mat[1][1] * det2_24_24 - mat[1][2] * det2_24_14 + mat[1][4] * det2_24_12;
	float det3_124_134 = mat[1][1] * det2_24_34 - mat[1][3] * det2_24_14 + mat[1][4] * det2_24_13;
	float det3_124_234 = mat[1][2] * det2_24_34 - mat[1][3] * det2_24_24 + mat[1][4] * det2_24_23;
	float det3_134_012 = mat[1][0] * det2_34_12 - mat[1][1] * det2_34_02 + mat[1][2] * det2_34_01;
	float det3_134_013 = mat[1][0] * det2_34_13 - mat[1][1] * det2_34_03 + mat[1][3] * det2_34_01;
	float det3_134_014 = mat[1][0] * det2_34_14 - mat[1][1] * det2_34_04 + mat[1][4] * det2_34_01;
	float det3_134_023 = mat[1][0] * det2_34_23 - mat[1][2] * det2_34_03 + mat[1][3] * det2_34_02;
	float det3_134_024 = mat[1][0] * det2_34_24 - mat[1][2] * det2_34_04 + mat[1][4] * det2_34_02;
	float det3_134_034 = mat[1][0] * det2_34_34 - mat[1][3] * det2_34_04 + mat[1][4] * det2_34_03;
	float det3_134_123 = mat[1][1] * det2_34_23 - mat[1][2] * det2_34_13 + mat[1][3] * det2_34_12;
	float det3_134_124 = mat[1][1] * det2_34_24 - mat[1][2] * det2_34_14 + mat[1][4] * det2_34_12;
	float det3_134_134 = mat[1][1] * det2_34_34 - mat[1][3] * det2_34_14 + mat[1][4] * det2_34_13;
	float det3_134_234 = mat[1][2] * det2_34_34 - mat[1][3] * det2_34_24 + mat[1][4] * det2_34_23;

	// remaining 4x4 sub-determinants
	float det4_0123_0123 = mat[0][0] * det3_123_123 - mat[0][1] * det3_123_023 + mat[0][2] * det3_123_013 - mat[0][3] * det3_123_012;
	float det4_0123_0124 = mat[0][0] * det3_123_124 - mat[0][1] * det3_123_024 + mat[0][2] * det3_123_014 - mat[0][4] * det3_123_012;
	float det4_0123_0134 = mat[0][0] * det3_123_134 - mat[0][1] * det3_123_034 + mat[0][3] * det3_123_014 - mat[0][4] * det3_123_013;
	float det4_0123_0234 = mat[0][0] * det3_123_234 - mat[0][2] * det3_123_034 + mat[0][3] * det3_123_024 - mat[0][4] * det3_123_023;
	float det4_0123_1234 = mat[0][1] * det3_123_234 - mat[0][2] * det3_123_134 + mat[0][3] * det3_123_124 - mat[0][4] * det3_123_123;
	float det4_0124_0123 = mat[0][0] * det3_124_123 - mat[0][1] * det3_124_023 + mat[0][2] * det3_124_013 - mat[0][3] * det3_124_012;
	float det4_0124_0124 = mat[0][0] * det3_124_124 - mat[0][1] * det3_124_024 + mat[0][2] * det3_124_014 - mat[0][4] * det3_124_012;
	float det4_0124_0134 = mat[0][0] * det3_124_134 - mat[0][1] * det3_124_034 + mat[0][3] * det3_124_014 - mat[0][4] * det3_124_013;
	float det4_0124_0234 = mat[0][0] * det3_124_234 - mat[0][2] * det3_124_034 + mat[0][3] * det3_124_024 - mat[0][4] * det3_124_023;
	float det4_0124_1234 = mat[0][1] * det3_124_234 - mat[0][2] * det3_124_134 + mat[0][3] * det3_124_124 - mat[0][4] * det3_124_123;
	float det4_0134_0123 = mat[0][0] * det3_134_123 - mat[0][1] * det3_134_023 + mat[0][2] * det3_134_013 - mat[0][3] * det3_134_012;
	float det4_0134_0124 = mat[0][0] * det3_134_124 - mat[0][1] * det3_134_024 + mat[0][2] * det3_134_014 - mat[0][4] * det3_134_012;
	float det4_0134_0134 = mat[0][0] * det3_134_134 - mat[0][1] * det3_134_034 + mat[0][3] * det3_134_014 - mat[0][4] * det3_134_013;
	float det4_0134_0234 = mat[0][0] * det3_134_234 - mat[0][2] * det3_134_034 + mat[0][3] * det3_134_024 - mat[0][4] * det3_134_023;
	float det4_0134_1234 = mat[0][1] * det3_134_234 - mat[0][2] * det3_134_134 + mat[0][3] * det3_134_124 - mat[0][4] * det3_134_123;
	float det4_0234_0123 = mat[0][0] * det3_234_123 - mat[0][1] * det3_234_023 + mat[0][2] * det3_234_013 - mat[0][3] * det3_234_012;
	float det4_0234_0124 = mat[0][0] * det3_234_124 - mat[0][1] * det3_234_024 + mat[0][2] * det3_234_014 - mat[0][4] * det3_234_012;
	float det4_0234_0134 = mat[0][0] * det3_234_134 - mat[0][1] * det3_234_034 + mat[0][3] * det3_234_014 - mat[0][4] * det3_234_013;
	float det4_0234_0234 = mat[0][0] * det3_234_234 - mat[0][2] * det3_234_034 + mat[0][3] * det3_234_024 - mat[0][4] * det3_234_023;
	float det4_0234_1234 = mat[0][1] * det3_234_234 - mat[0][2] * det3_234_134 + mat[0][3] * det3_234_124 - mat[0][4] * det3_234_123;

	mat[0][0] =  det4_1234_1234 * invDet;
	mat[0][1] = -det4_0234_1234 * invDet;
	mat[0][2] =  det4_0134_1234 * invDet;
	mat[0][3] = -det4_0124_1234 * invDet;
	mat[0][4] =  det4_0123_1234 * invDet;

	mat[1][0] = -det4_1234_0234 * invDet;
	mat[1][1] =  det4_0234_0234 * invDet;
	mat[1][2] = -det4_0134_0234 * invDet;
	mat[1][3] =  det4_0124_0234 * invDet;
	mat[1][4] = -det4_0123_0234 * invDet;

	mat[2][0] =  det4_1234_0134 * invDet;
	mat[2][1] = -det4_0234_0134 * invDet;
	mat[2][2] =  det4_0134_0134 * invDet;
	mat[2][3] = -det4_0124_0134 * invDet;
	mat[2][4] =  det4_0123_0134 * invDet;

	mat[3][0] = -det4_1234_0124 * invDet;
	mat[3][1] =  det4_0234_0124 * invDet;
	mat[3][2] = -det4_0134_0124 * invDet;
	mat[3][3] =  det4_0124_0124 * invDet;
	mat[3][4] = -det4_0123_0124 * invDet;

	mat[4][0] =  det4_1234_0123 * invDet;
	mat[4][1] = -det4_0234_0123 * invDet;
	mat[4][2] =  det4_0134_0123 * invDet;
	mat[4][3] = -det4_0124_0123 * invDet;
	mat[4][4] =  det4_0123_0123 * invDet;

	return true;
#elif 0
	// 5*28 = 140 multiplications
	//			5 divisions
	float *mat = reinterpret_cast<float *>( this );
	float s;
	double d, di;

	di = mat[0];
	s = di;
	mat[0] = d = 1.0f / di;
	mat[1] *= d;
	mat[2] *= d;
	mat[3] *= d;
	mat[4] *= d;
	d = -d;
	mat[5] *= d;
	mat[10] *= d;
	mat[15] *= d;
	mat[20] *= d;
	d = mat[5] * di;
	mat[6] += mat[1] * d;
	mat[7] += mat[2] * d;
	mat[8] += mat[3] * d;
	mat[9] += mat[4] * d;
	d = mat[10] * di;
	mat[11] += mat[1] * d;
	mat[12] += mat[2] * d;
	mat[13] += mat[3] * d;
	mat[14] += mat[4] * d;
	d = mat[15] * di;
	mat[16] += mat[1] * d;
	mat[17] += mat[2] * d;
	mat[18] += mat[3] * d;
	mat[19] += mat[4] * d;
	d = mat[20] * di;
	mat[21] += mat[1] * d;
	mat[22] += mat[2] * d;
	mat[23] += mat[3] * d;
	mat[24] += mat[4] * d;
	di = mat[6];
	s *= di;
	mat[6] = d = 1.0f / di;
	mat[5] *= d;
	mat[7] *= d;
	mat[8] *= d;
	mat[9] *= d;
	d = -d;
	mat[1] *= d;
	mat[11] *= d;
	mat[16] *= d;
	mat[21] *= d;
	d = mat[1] * di;
	mat[0] += mat[5] * d;
	mat[2] += mat[7] * d;
	mat[3] += mat[8] * d;
	mat[4] += mat[9] * d;
	d = mat[11] * di;
	mat[10] += mat[5] * d;
	mat[12] += mat[7] * d;
	mat[13] += mat[8] * d;
	mat[14] += mat[9] * d;
	d = mat[16] * di;
	mat[15] += mat[5] * d;
	mat[17] += mat[7] * d;
	mat[18] += mat[8] * d;
	mat[19] += mat[9] * d;
	d = mat[21] * di;
	mat[20] += mat[5] * d;
	mat[22] += mat[7] * d;
	mat[23] += mat[8] * d;
	mat[24] += mat[9] * d;
	di = mat[12];
	s *= di;
	mat[12] = d = 1.0f / di;
	mat[10] *= d;
	mat[11] *= d;
	mat[13] *= d;
	mat[14] *= d;
	d = -d;
	mat[2] *= d;
	mat[7] *= d;
	mat[17] *= d;
	mat[22] *= d;
	d = mat[2] * di;
	mat[0] += mat[10] * d;
	mat[1] += mat[11] * d;
	mat[3] += mat[13] * d;
	mat[4] += mat[14] * d;
	d = mat[7] * di;
	mat[5] += mat[10] * d;
	mat[6] += mat[11] * d;
	mat[8] += mat[13] * d;
	mat[9] += mat[14] * d;
	d = mat[17] * di;
	mat[15] += mat[10] * d;
	mat[16] += mat[11] * d;
	mat[18] += mat[13] * d;
	mat[19] += mat[14] * d;
	d = mat[22] * di;
	mat[20] += mat[10] * d;
	mat[21] += mat[11] * d;
	mat[23] += mat[13] * d;
	mat[24] += mat[14] * d;
	di = mat[18];
	s *= di;
	mat[18] = d = 1.0f / di;
	mat[15] *= d;
	mat[16] *= d;
	mat[17] *= d;
	mat[19] *= d;
	d = -d;
	mat[3] *= d;
	mat[8] *= d;
	mat[13] *= d;
	mat[23] *= d;
	d = mat[3] * di;
	mat[0] += mat[15] * d;
	mat[1] += mat[16] * d;
	mat[2] += mat[17] * d;
	mat[4] += mat[19] * d;
	d = mat[8] * di;
	mat[5] += mat[15] * d;
	mat[6] += mat[16] * d;
	mat[7] += mat[17] * d;
	mat[9] += mat[19] * d;
	d = mat[13] * di;
	mat[10] += mat[15] * d;
	mat[11] += mat[16] * d;
	mat[12] += mat[17] * d;
	mat[14] += mat[19] * d;
	d = mat[23] * di;
	mat[20] += mat[15] * d;
	mat[21] += mat[16] * d;
	mat[22] += mat[17] * d;
	mat[24] += mat[19] * d;
	di = mat[24];
	s *= di;
	mat[24] = d = 1.0f / di;
	mat[20] *= d;
	mat[21] *= d;
	mat[22] *= d;
	mat[23] *= d;
	d = -d;
	mat[4] *= d;
	mat[9] *= d;
	mat[14] *= d;
	mat[19] *= d;
	d = mat[4] * di;
	mat[0] += mat[20] * d;
	mat[1] += mat[21] * d;
	mat[2] += mat[22] * d;
	mat[3] += mat[23] * d;
	d = mat[9] * di;
	mat[5] += mat[20] * d;
	mat[6] += mat[21] * d;
	mat[7] += mat[22] * d;
	mat[8] += mat[23] * d;
	d = mat[14] * di;
	mat[10] += mat[20] * d;
	mat[11] += mat[21] * d;
	mat[12] += mat[22] * d;
	mat[13] += mat[23] * d;
	d = mat[19] * di;
	mat[15] += mat[20] * d;
	mat[16] += mat[21] * d;
	mat[17] += mat[22] * d;
	mat[18] += mat[23] * d;

	return ( s != 0.0f && !FLOAT_IS_NAN( s ) );
#else
	// 86+30+6 = 122 multiplications
	//	  2*1  =   2 divisions
	arcMat3 r0, r1, r2, r3;
	float c0, c1, c2, det, invDet;
	float *mat = reinterpret_cast<float *>( this );

	// r0 = m0.Inverse();	// 3x3
	c0 = mat[1*5+1] * mat[2*5+2] - mat[1*5+2] * mat[2*5+1];
	c1 = mat[1*5+2] * mat[2*5+0] - mat[1*5+0] * mat[2*5+2];
	c2 = mat[1*5+0] * mat[2*5+1] - mat[1*5+1] * mat[2*5+0];

	det = mat[0*5+0] * c0 + mat[0*5+1] * c1 + mat[0*5+2] * c2;

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	r0[0][0] = c0 * invDet;
	r0[0][1] = ( mat[0*5+2] * mat[2*5+1] - mat[0*5+1] * mat[2*5+2] ) * invDet;
	r0[0][2] = ( mat[0*5+1] * mat[1*5+2] - mat[0*5+2] * mat[1*5+1] ) * invDet;
	r0[1][0] = c1 * invDet;
	r0[1][1] = ( mat[0*5+0] * mat[2*5+2] - mat[0*5+2] * mat[2*5+0] ) * invDet;
	r0[1][2] = ( mat[0*5+2] * mat[1*5+0] - mat[0*5+0] * mat[1*5+2] ) * invDet;
	r0[2][0] = c2 * invDet;
	r0[2][1] = ( mat[0*5+1] * mat[2*5+0] - mat[0*5+0] * mat[2*5+1] ) * invDet;
	r0[2][2] = ( mat[0*5+0] * mat[1*5+1] - mat[0*5+1] * mat[1*5+0] ) * invDet;

	// r1 = r0 * m1;		// 3x2 = 3x3 * 3x2
	r1[0][0] = r0[0][0] * mat[0*5+3] + r0[0][1] * mat[1*5+3] + r0[0][2] * mat[2*5+3];
	r1[0][1] = r0[0][0] * mat[0*5+4] + r0[0][1] * mat[1*5+4] + r0[0][2] * mat[2*5+4];
	r1[1][0] = r0[1][0] * mat[0*5+3] + r0[1][1] * mat[1*5+3] + r0[1][2] * mat[2*5+3];
	r1[1][1] = r0[1][0] * mat[0*5+4] + r0[1][1] * mat[1*5+4] + r0[1][2] * mat[2*5+4];
	r1[2][0] = r0[2][0] * mat[0*5+3] + r0[2][1] * mat[1*5+3] + r0[2][2] * mat[2*5+3];
	r1[2][1] = r0[2][0] * mat[0*5+4] + r0[2][1] * mat[1*5+4] + r0[2][2] * mat[2*5+4];

	// r2 = m2 * r1;		// 2x2 = 2x3 * 3x2
	r2[0][0] = mat[3*5+0] * r1[0][0] + mat[3*5+1] * r1[1][0] + mat[3*5+2] * r1[2][0];
	r2[0][1] = mat[3*5+0] * r1[0][1] + mat[3*5+1] * r1[1][1] + mat[3*5+2] * r1[2][1];
	r2[1][0] = mat[4*5+0] * r1[0][0] + mat[4*5+1] * r1[1][0] + mat[4*5+2] * r1[2][0];
	r2[1][1] = mat[4*5+0] * r1[0][1] + mat[4*5+1] * r1[1][1] + mat[4*5+2] * r1[2][1];

	// r3 = r2 - m3;		// 2x2 = 2x2 - 2x2
	r3[0][0] = r2[0][0] - mat[3*5+3];
	r3[0][1] = r2[0][1] - mat[3*5+4];
	r3[1][0] = r2[1][0] - mat[4*5+3];
	r3[1][1] = r2[1][1] - mat[4*5+4];

	// r3.InverseSelf();	// 2x2
	det = r3[0][0] * r3[1][1] - r3[0][1] * r3[1][0];

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	c0 = r3[0][0];
	r3[0][0] =   r3[1][1] * invDet;
	r3[0][1] = - r3[0][1] * invDet;
	r3[1][0] = - r3[1][0] * invDet;
	r3[1][1] =   c0 * invDet;

	// r2 = m2 * r0;		// 2x3 = 2x3 * 3x3
	r2[0][0] = mat[3*5+0] * r0[0][0] + mat[3*5+1] * r0[1][0] + mat[3*5+2] * r0[2][0];
	r2[0][1] = mat[3*5+0] * r0[0][1] + mat[3*5+1] * r0[1][1] + mat[3*5+2] * r0[2][1];
	r2[0][2] = mat[3*5+0] * r0[0][2] + mat[3*5+1] * r0[1][2] + mat[3*5+2] * r0[2][2];
	r2[1][0] = mat[4*5+0] * r0[0][0] + mat[4*5+1] * r0[1][0] + mat[4*5+2] * r0[2][0];
	r2[1][1] = mat[4*5+0] * r0[0][1] + mat[4*5+1] * r0[1][1] + mat[4*5+2] * r0[2][1];
	r2[1][2] = mat[4*5+0] * r0[0][2] + mat[4*5+1] * r0[1][2] + mat[4*5+2] * r0[2][2];

	// m2 = r3 * r2;		// 2x3 = 2x2 * 2x3
	mat[3*5+0] = r3[0][0] * r2[0][0] + r3[0][1] * r2[1][0];
	mat[3*5+1] = r3[0][0] * r2[0][1] + r3[0][1] * r2[1][1];
	mat[3*5+2] = r3[0][0] * r2[0][2] + r3[0][1] * r2[1][2];
	mat[4*5+0] = r3[1][0] * r2[0][0] + r3[1][1] * r2[1][0];
	mat[4*5+1] = r3[1][0] * r2[0][1] + r3[1][1] * r2[1][1];
	mat[4*5+2] = r3[1][0] * r2[0][2] + r3[1][1] * r2[1][2];

	// m0 = r0 - r1 * m2;	// 3x3 = 3x3 - 3x2 * 2x3
	mat[0*5+0] = r0[0][0] - r1[0][0] * mat[3*5+0] - r1[0][1] * mat[4*5+0];
	mat[0*5+1] = r0[0][1] - r1[0][0] * mat[3*5+1] - r1[0][1] * mat[4*5+1];
	mat[0*5+2] = r0[0][2] - r1[0][0] * mat[3*5+2] - r1[0][1] * mat[4*5+2];
	mat[1*5+0] = r0[1][0] - r1[1][0] * mat[3*5+0] - r1[1][1] * mat[4*5+0];
	mat[1*5+1] = r0[1][1] - r1[1][0] * mat[3*5+1] - r1[1][1] * mat[4*5+1];
	mat[1*5+2] = r0[1][2] - r1[1][0] * mat[3*5+2] - r1[1][1] * mat[4*5+2];
	mat[2*5+0] = r0[2][0] - r1[2][0] * mat[3*5+0] - r1[2][1] * mat[4*5+0];
	mat[2*5+1] = r0[2][1] - r1[2][0] * mat[3*5+1] - r1[2][1] * mat[4*5+1];
	mat[2*5+2] = r0[2][2] - r1[2][0] * mat[3*5+2] - r1[2][1] * mat[4*5+2];

	// m1 = r1 * r3;		// 3x2 = 3x2 * 2x2
	mat[0*5+3] = r1[0][0] * r3[0][0] + r1[0][1] * r3[1][0];
	mat[0*5+4] = r1[0][0] * r3[0][1] + r1[0][1] * r3[1][1];
	mat[1*5+3] = r1[1][0] * r3[0][0] + r1[1][1] * r3[1][0];
	mat[1*5+4] = r1[1][0] * r3[0][1] + r1[1][1] * r3[1][1];
	mat[2*5+3] = r1[2][0] * r3[0][0] + r1[2][1] * r3[1][0];
	mat[2*5+4] = r1[2][0] * r3[0][1] + r1[2][1] * r3[1][1];

	// m3 = -r3;			// 2x2 = - 2x2
	mat[3*5+3] = -r3[0][0];
	mat[3*5+4] = -r3[0][1];
	mat[4*5+3] = -r3[1][0];
	mat[4*5+4] = -r3[1][1];

	return true;
#endif
}

/*
=============
arcMat5::ToString
=============
*/
const char *arcMat5::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}


//===============================================================
//
//	arcMat6
//
//===============================================================

arcMat6 mat6_zero( arcVec6( 0, 0, 0, 0, 0, 0 ), arcVec6( 0, 0, 0, 0, 0, 0 ), arcVec6( 0, 0, 0, 0, 0, 0 ), arcVec6( 0, 0, 0, 0, 0, 0 ), arcVec6( 0, 0, 0, 0, 0, 0 ), arcVec6( 0, 0, 0, 0, 0, 0 ) );
arcMat6 mat6_identity( arcVec6( 1, 0, 0, 0, 0, 0 ), arcVec6( 0, 1, 0, 0, 0, 0 ), arcVec6( 0, 0, 1, 0, 0, 0 ), arcVec6( 0, 0, 0, 1, 0, 0 ), arcVec6( 0, 0, 0, 0, 1, 0 ), arcVec6( 0, 0, 0, 0, 0, 1 ) );

/*
============
arcMat6::Transpose
============
*/
arcMat6 arcMat6::Transpose( void ) const {
	arcMat6	transpose;
	int		i, j;

	for ( i = 0; i < 6; i++ ) {
		for ( j = 0; j < 6; j++ ) {
			transpose[ i ][ j ] = mat[ j ][ i ];
        }
	}
	return transpose;
}

/*
============
arcMat6::TransposeSelf
============
*/
arcMat6 &arcMat6::TransposeSelf( void ) {
	float	temp;
	int		i, j;

	for ( i = 0; i < 6; i++ ) {
		for ( j = i + 1; j < 6; j++ ) {
			temp = mat[ i ][ j ];
			mat[ i ][ j ] = mat[ j ][ i ];
			mat[ j ][ i ] = temp;
        }
	}
	return *this;
}

/*
============
arcMat6::Determinant
============
*/
float arcMat6::Determinant( void ) const {

	// 2x2 sub-determinants required to calculate 6x6 determinant
	float det2_45_01 = mat[4][0] * mat[5][1] - mat[4][1] * mat[5][0];
	float det2_45_02 = mat[4][0] * mat[5][2] - mat[4][2] * mat[5][0];
	float det2_45_03 = mat[4][0] * mat[5][3] - mat[4][3] * mat[5][0];
	float det2_45_04 = mat[4][0] * mat[5][4] - mat[4][4] * mat[5][0];
	float det2_45_05 = mat[4][0] * mat[5][5] - mat[4][5] * mat[5][0];
	float det2_45_12 = mat[4][1] * mat[5][2] - mat[4][2] * mat[5][1];
	float det2_45_13 = mat[4][1] * mat[5][3] - mat[4][3] * mat[5][1];
	float det2_45_14 = mat[4][1] * mat[5][4] - mat[4][4] * mat[5][1];
	float det2_45_15 = mat[4][1] * mat[5][5] - mat[4][5] * mat[5][1];
	float det2_45_23 = mat[4][2] * mat[5][3] - mat[4][3] * mat[5][2];
	float det2_45_24 = mat[4][2] * mat[5][4] - mat[4][4] * mat[5][2];
	float det2_45_25 = mat[4][2] * mat[5][5] - mat[4][5] * mat[5][2];
	float det2_45_34 = mat[4][3] * mat[5][4] - mat[4][4] * mat[5][3];
	float det2_45_35 = mat[4][3] * mat[5][5] - mat[4][5] * mat[5][3];
	float det2_45_45 = mat[4][4] * mat[5][5] - mat[4][5] * mat[5][4];

	// 3x3 sub-determinants required to calculate 6x6 determinant
	float det3_345_012 = mat[3][0] * det2_45_12 - mat[3][1] * det2_45_02 + mat[3][2] * det2_45_01;
	float det3_345_013 = mat[3][0] * det2_45_13 - mat[3][1] * det2_45_03 + mat[3][3] * det2_45_01;
	float det3_345_014 = mat[3][0] * det2_45_14 - mat[3][1] * det2_45_04 + mat[3][4] * det2_45_01;
	float det3_345_015 = mat[3][0] * det2_45_15 - mat[3][1] * det2_45_05 + mat[3][5] * det2_45_01;
	float det3_345_023 = mat[3][0] * det2_45_23 - mat[3][2] * det2_45_03 + mat[3][3] * det2_45_02;
	float det3_345_024 = mat[3][0] * det2_45_24 - mat[3][2] * det2_45_04 + mat[3][4] * det2_45_02;
	float det3_345_025 = mat[3][0] * det2_45_25 - mat[3][2] * det2_45_05 + mat[3][5] * det2_45_02;
	float det3_345_034 = mat[3][0] * det2_45_34 - mat[3][3] * det2_45_04 + mat[3][4] * det2_45_03;
	float det3_345_035 = mat[3][0] * det2_45_35 - mat[3][3] * det2_45_05 + mat[3][5] * det2_45_03;
	float det3_345_045 = mat[3][0] * det2_45_45 - mat[3][4] * det2_45_05 + mat[3][5] * det2_45_04;
	float det3_345_123 = mat[3][1] * det2_45_23 - mat[3][2] * det2_45_13 + mat[3][3] * det2_45_12;
	float det3_345_124 = mat[3][1] * det2_45_24 - mat[3][2] * det2_45_14 + mat[3][4] * det2_45_12;
	float det3_345_125 = mat[3][1] * det2_45_25 - mat[3][2] * det2_45_15 + mat[3][5] * det2_45_12;
	float det3_345_134 = mat[3][1] * det2_45_34 - mat[3][3] * det2_45_14 + mat[3][4] * det2_45_13;
	float det3_345_135 = mat[3][1] * det2_45_35 - mat[3][3] * det2_45_15 + mat[3][5] * det2_45_13;
	float det3_345_145 = mat[3][1] * det2_45_45 - mat[3][4] * det2_45_15 + mat[3][5] * det2_45_14;
	float det3_345_234 = mat[3][2] * det2_45_34 - mat[3][3] * det2_45_24 + mat[3][4] * det2_45_23;
	float det3_345_235 = mat[3][2] * det2_45_35 - mat[3][3] * det2_45_25 + mat[3][5] * det2_45_23;
	float det3_345_245 = mat[3][2] * det2_45_45 - mat[3][4] * det2_45_25 + mat[3][5] * det2_45_24;
	float det3_345_345 = mat[3][3] * det2_45_45 - mat[3][4] * det2_45_35 + mat[3][5] * det2_45_34;

	// 4x4 sub-determinants required to calculate 6x6 determinant
	float det4_2345_0123 = mat[2][0] * det3_345_123 - mat[2][1] * det3_345_023 + mat[2][2] * det3_345_013 - mat[2][3] * det3_345_012;
	float det4_2345_0124 = mat[2][0] * det3_345_124 - mat[2][1] * det3_345_024 + mat[2][2] * det3_345_014 - mat[2][4] * det3_345_012;
	float det4_2345_0125 = mat[2][0] * det3_345_125 - mat[2][1] * det3_345_025 + mat[2][2] * det3_345_015 - mat[2][5] * det3_345_012;
	float det4_2345_0134 = mat[2][0] * det3_345_134 - mat[2][1] * det3_345_034 + mat[2][3] * det3_345_014 - mat[2][4] * det3_345_013;
	float det4_2345_0135 = mat[2][0] * det3_345_135 - mat[2][1] * det3_345_035 + mat[2][3] * det3_345_015 - mat[2][5] * det3_345_013;
	float det4_2345_0145 = mat[2][0] * det3_345_145 - mat[2][1] * det3_345_045 + mat[2][4] * det3_345_015 - mat[2][5] * det3_345_014;
	float det4_2345_0234 = mat[2][0] * det3_345_234 - mat[2][2] * det3_345_034 + mat[2][3] * det3_345_024 - mat[2][4] * det3_345_023;
	float det4_2345_0235 = mat[2][0] * det3_345_235 - mat[2][2] * det3_345_035 + mat[2][3] * det3_345_025 - mat[2][5] * det3_345_023;
	float det4_2345_0245 = mat[2][0] * det3_345_245 - mat[2][2] * det3_345_045 + mat[2][4] * det3_345_025 - mat[2][5] * det3_345_024;
	float det4_2345_0345 = mat[2][0] * det3_345_345 - mat[2][3] * det3_345_045 + mat[2][4] * det3_345_035 - mat[2][5] * det3_345_034;
	float det4_2345_1234 = mat[2][1] * det3_345_234 - mat[2][2] * det3_345_134 + mat[2][3] * det3_345_124 - mat[2][4] * det3_345_123;
	float det4_2345_1235 = mat[2][1] * det3_345_235 - mat[2][2] * det3_345_135 + mat[2][3] * det3_345_125 - mat[2][5] * det3_345_123;
	float det4_2345_1245 = mat[2][1] * det3_345_245 - mat[2][2] * det3_345_145 + mat[2][4] * det3_345_125 - mat[2][5] * det3_345_124;
	float det4_2345_1345 = mat[2][1] * det3_345_345 - mat[2][3] * det3_345_145 + mat[2][4] * det3_345_135 - mat[2][5] * det3_345_134;
	float det4_2345_2345 = mat[2][2] * det3_345_345 - mat[2][3] * det3_345_245 + mat[2][4] * det3_345_235 - mat[2][5] * det3_345_234;

	// 5x5 sub-determinants required to calculate 6x6 determinant
	float det5_12345_01234 = mat[1][0] * det4_2345_1234 - mat[1][1] * det4_2345_0234 + mat[1][2] * det4_2345_0134 - mat[1][3] * det4_2345_0124 + mat[1][4] * det4_2345_0123;
	float det5_12345_01235 = mat[1][0] * det4_2345_1235 - mat[1][1] * det4_2345_0235 + mat[1][2] * det4_2345_0135 - mat[1][3] * det4_2345_0125 + mat[1][5] * det4_2345_0123;
	float det5_12345_01245 = mat[1][0] * det4_2345_1245 - mat[1][1] * det4_2345_0245 + mat[1][2] * det4_2345_0145 - mat[1][4] * det4_2345_0125 + mat[1][5] * det4_2345_0124;
	float det5_12345_01345 = mat[1][0] * det4_2345_1345 - mat[1][1] * det4_2345_0345 + mat[1][3] * det4_2345_0145 - mat[1][4] * det4_2345_0135 + mat[1][5] * det4_2345_0134;
	float det5_12345_02345 = mat[1][0] * det4_2345_2345 - mat[1][2] * det4_2345_0345 + mat[1][3] * det4_2345_0245 - mat[1][4] * det4_2345_0235 + mat[1][5] * det4_2345_0234;
	float det5_12345_12345 = mat[1][1] * det4_2345_2345 - mat[1][2] * det4_2345_1345 + mat[1][3] * det4_2345_1245 - mat[1][4] * det4_2345_1235 + mat[1][5] * det4_2345_1234;

	// determinant of 6x6 matrix
	return	mat[0][0] * det5_12345_12345 - mat[0][1] * det5_12345_02345 + mat[0][2] * det5_12345_01345 -
			mat[0][3] * det5_12345_01245 + mat[0][4] * det5_12345_01235 - mat[0][5] * det5_12345_01234;
}

/*
============
arcMat6::InverseSelf
============
*/
bool arcMat6::InverseSelf( void ) {
	// 810+6+36 = 852 multiplications
	//				1 division
	double det, invDet;

	// 2x2 sub-determinants required to calculate 6x6 determinant
	float det2_45_01 = mat[4][0] * mat[5][1] - mat[4][1] * mat[5][0];
	float det2_45_02 = mat[4][0] * mat[5][2] - mat[4][2] * mat[5][0];
	float det2_45_03 = mat[4][0] * mat[5][3] - mat[4][3] * mat[5][0];
	float det2_45_04 = mat[4][0] * mat[5][4] - mat[4][4] * mat[5][0];
	float det2_45_05 = mat[4][0] * mat[5][5] - mat[4][5] * mat[5][0];
	float det2_45_12 = mat[4][1] * mat[5][2] - mat[4][2] * mat[5][1];
	float det2_45_13 = mat[4][1] * mat[5][3] - mat[4][3] * mat[5][1];
	float det2_45_14 = mat[4][1] * mat[5][4] - mat[4][4] * mat[5][1];
	float det2_45_15 = mat[4][1] * mat[5][5] - mat[4][5] * mat[5][1];
	float det2_45_23 = mat[4][2] * mat[5][3] - mat[4][3] * mat[5][2];
	float det2_45_24 = mat[4][2] * mat[5][4] - mat[4][4] * mat[5][2];
	float det2_45_25 = mat[4][2] * mat[5][5] - mat[4][5] * mat[5][2];
	float det2_45_34 = mat[4][3] * mat[5][4] - mat[4][4] * mat[5][3];
	float det2_45_35 = mat[4][3] * mat[5][5] - mat[4][5] * mat[5][3];
	float det2_45_45 = mat[4][4] * mat[5][5] - mat[4][5] * mat[5][4];

	// 3x3 sub-determinants required to calculate 6x6 determinant
	float det3_345_012 = mat[3][0] * det2_45_12 - mat[3][1] * det2_45_02 + mat[3][2] * det2_45_01;
	float det3_345_013 = mat[3][0] * det2_45_13 - mat[3][1] * det2_45_03 + mat[3][3] * det2_45_01;
	float det3_345_014 = mat[3][0] * det2_45_14 - mat[3][1] * det2_45_04 + mat[3][4] * det2_45_01;
	float det3_345_015 = mat[3][0] * det2_45_15 - mat[3][1] * det2_45_05 + mat[3][5] * det2_45_01;
	float det3_345_023 = mat[3][0] * det2_45_23 - mat[3][2] * det2_45_03 + mat[3][3] * det2_45_02;
	float det3_345_024 = mat[3][0] * det2_45_24 - mat[3][2] * det2_45_04 + mat[3][4] * det2_45_02;
	float det3_345_025 = mat[3][0] * det2_45_25 - mat[3][2] * det2_45_05 + mat[3][5] * det2_45_02;
	float det3_345_034 = mat[3][0] * det2_45_34 - mat[3][3] * det2_45_04 + mat[3][4] * det2_45_03;
	float det3_345_035 = mat[3][0] * det2_45_35 - mat[3][3] * det2_45_05 + mat[3][5] * det2_45_03;
	float det3_345_045 = mat[3][0] * det2_45_45 - mat[3][4] * det2_45_05 + mat[3][5] * det2_45_04;
	float det3_345_123 = mat[3][1] * det2_45_23 - mat[3][2] * det2_45_13 + mat[3][3] * det2_45_12;
	float det3_345_124 = mat[3][1] * det2_45_24 - mat[3][2] * det2_45_14 + mat[3][4] * det2_45_12;
	float det3_345_125 = mat[3][1] * det2_45_25 - mat[3][2] * det2_45_15 + mat[3][5] * det2_45_12;
	float det3_345_134 = mat[3][1] * det2_45_34 - mat[3][3] * det2_45_14 + mat[3][4] * det2_45_13;
	float det3_345_135 = mat[3][1] * det2_45_35 - mat[3][3] * det2_45_15 + mat[3][5] * det2_45_13;
	float det3_345_145 = mat[3][1] * det2_45_45 - mat[3][4] * det2_45_15 + mat[3][5] * det2_45_14;
	float det3_345_234 = mat[3][2] * det2_45_34 - mat[3][3] * det2_45_24 + mat[3][4] * det2_45_23;
	float det3_345_235 = mat[3][2] * det2_45_35 - mat[3][3] * det2_45_25 + mat[3][5] * det2_45_23;
	float det3_345_245 = mat[3][2] * det2_45_45 - mat[3][4] * det2_45_25 + mat[3][5] * det2_45_24;
	float det3_345_345 = mat[3][3] * det2_45_45 - mat[3][4] * det2_45_35 + mat[3][5] * det2_45_34;

	// 4x4 sub-determinants required to calculate 6x6 determinant
	float det4_2345_0123 = mat[2][0] * det3_345_123 - mat[2][1] * det3_345_023 + mat[2][2] * det3_345_013 - mat[2][3] * det3_345_012;
	float det4_2345_0124 = mat[2][0] * det3_345_124 - mat[2][1] * det3_345_024 + mat[2][2] * det3_345_014 - mat[2][4] * det3_345_012;
	float det4_2345_0125 = mat[2][0] * det3_345_125 - mat[2][1] * det3_345_025 + mat[2][2] * det3_345_015 - mat[2][5] * det3_345_012;
	float det4_2345_0134 = mat[2][0] * det3_345_134 - mat[2][1] * det3_345_034 + mat[2][3] * det3_345_014 - mat[2][4] * det3_345_013;
	float det4_2345_0135 = mat[2][0] * det3_345_135 - mat[2][1] * det3_345_035 + mat[2][3] * det3_345_015 - mat[2][5] * det3_345_013;
	float det4_2345_0145 = mat[2][0] * det3_345_145 - mat[2][1] * det3_345_045 + mat[2][4] * det3_345_015 - mat[2][5] * det3_345_014;
	float det4_2345_0234 = mat[2][0] * det3_345_234 - mat[2][2] * det3_345_034 + mat[2][3] * det3_345_024 - mat[2][4] * det3_345_023;
	float det4_2345_0235 = mat[2][0] * det3_345_235 - mat[2][2] * det3_345_035 + mat[2][3] * det3_345_025 - mat[2][5] * det3_345_023;
	float det4_2345_0245 = mat[2][0] * det3_345_245 - mat[2][2] * det3_345_045 + mat[2][4] * det3_345_025 - mat[2][5] * det3_345_024;
	float det4_2345_0345 = mat[2][0] * det3_345_345 - mat[2][3] * det3_345_045 + mat[2][4] * det3_345_035 - mat[2][5] * det3_345_034;
	float det4_2345_1234 = mat[2][1] * det3_345_234 - mat[2][2] * det3_345_134 + mat[2][3] * det3_345_124 - mat[2][4] * det3_345_123;
	float det4_2345_1235 = mat[2][1] * det3_345_235 - mat[2][2] * det3_345_135 + mat[2][3] * det3_345_125 - mat[2][5] * det3_345_123;
	float det4_2345_1245 = mat[2][1] * det3_345_245 - mat[2][2] * det3_345_145 + mat[2][4] * det3_345_125 - mat[2][5] * det3_345_124;
	float det4_2345_1345 = mat[2][1] * det3_345_345 - mat[2][3] * det3_345_145 + mat[2][4] * det3_345_135 - mat[2][5] * det3_345_134;
	float det4_2345_2345 = mat[2][2] * det3_345_345 - mat[2][3] * det3_345_245 + mat[2][4] * det3_345_235 - mat[2][5] * det3_345_234;

	// 5x5 sub-determinants required to calculate 6x6 determinant
	float det5_12345_01234 = mat[1][0] * det4_2345_1234 - mat[1][1] * det4_2345_0234 + mat[1][2] * det4_2345_0134 - mat[1][3] * det4_2345_0124 + mat[1][4] * det4_2345_0123;
	float det5_12345_01235 = mat[1][0] * det4_2345_1235 - mat[1][1] * det4_2345_0235 + mat[1][2] * det4_2345_0135 - mat[1][3] * det4_2345_0125 + mat[1][5] * det4_2345_0123;
	float det5_12345_01245 = mat[1][0] * det4_2345_1245 - mat[1][1] * det4_2345_0245 + mat[1][2] * det4_2345_0145 - mat[1][4] * det4_2345_0125 + mat[1][5] * det4_2345_0124;
	float det5_12345_01345 = mat[1][0] * det4_2345_1345 - mat[1][1] * det4_2345_0345 + mat[1][3] * det4_2345_0145 - mat[1][4] * det4_2345_0135 + mat[1][5] * det4_2345_0134;
	float det5_12345_02345 = mat[1][0] * det4_2345_2345 - mat[1][2] * det4_2345_0345 + mat[1][3] * det4_2345_0245 - mat[1][4] * det4_2345_0235 + mat[1][5] * det4_2345_0234;
	float det5_12345_12345 = mat[1][1] * det4_2345_2345 - mat[1][2] * det4_2345_1345 + mat[1][3] * det4_2345_1245 - mat[1][4] * det4_2345_1235 + mat[1][5] * det4_2345_1234;

	// determinant of 6x6 matrix
	det = mat[0][0] * det5_12345_12345 - mat[0][1] * det5_12345_02345 + mat[0][2] * det5_12345_01345 -
				mat[0][3] * det5_12345_01245 + mat[0][4] * det5_12345_01235 - mat[0][5] * det5_12345_01234;

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	// remaining 2x2 sub-determinants
	float det2_34_01 = mat[3][0] * mat[4][1] - mat[3][1] * mat[4][0];
	float det2_34_02 = mat[3][0] * mat[4][2] - mat[3][2] * mat[4][0];
	float det2_34_03 = mat[3][0] * mat[4][3] - mat[3][3] * mat[4][0];
	float det2_34_04 = mat[3][0] * mat[4][4] - mat[3][4] * mat[4][0];
	float det2_34_05 = mat[3][0] * mat[4][5] - mat[3][5] * mat[4][0];
	float det2_34_12 = mat[3][1] * mat[4][2] - mat[3][2] * mat[4][1];
	float det2_34_13 = mat[3][1] * mat[4][3] - mat[3][3] * mat[4][1];
	float det2_34_14 = mat[3][1] * mat[4][4] - mat[3][4] * mat[4][1];
	float det2_34_15 = mat[3][1] * mat[4][5] - mat[3][5] * mat[4][1];
	float det2_34_23 = mat[3][2] * mat[4][3] - mat[3][3] * mat[4][2];
	float det2_34_24 = mat[3][2] * mat[4][4] - mat[3][4] * mat[4][2];
	float det2_34_25 = mat[3][2] * mat[4][5] - mat[3][5] * mat[4][2];
	float det2_34_34 = mat[3][3] * mat[4][4] - mat[3][4] * mat[4][3];
	float det2_34_35 = mat[3][3] * mat[4][5] - mat[3][5] * mat[4][3];
	float det2_34_45 = mat[3][4] * mat[4][5] - mat[3][5] * mat[4][4];
	float det2_35_01 = mat[3][0] * mat[5][1] - mat[3][1] * mat[5][0];
	float det2_35_02 = mat[3][0] * mat[5][2] - mat[3][2] * mat[5][0];
	float det2_35_03 = mat[3][0] * mat[5][3] - mat[3][3] * mat[5][0];
	float det2_35_04 = mat[3][0] * mat[5][4] - mat[3][4] * mat[5][0];
	float det2_35_05 = mat[3][0] * mat[5][5] - mat[3][5] * mat[5][0];
	float det2_35_12 = mat[3][1] * mat[5][2] - mat[3][2] * mat[5][1];
	float det2_35_13 = mat[3][1] * mat[5][3] - mat[3][3] * mat[5][1];
	float det2_35_14 = mat[3][1] * mat[5][4] - mat[3][4] * mat[5][1];
	float det2_35_15 = mat[3][1] * mat[5][5] - mat[3][5] * mat[5][1];
	float det2_35_23 = mat[3][2] * mat[5][3] - mat[3][3] * mat[5][2];
	float det2_35_24 = mat[3][2] * mat[5][4] - mat[3][4] * mat[5][2];
	float det2_35_25 = mat[3][2] * mat[5][5] - mat[3][5] * mat[5][2];
	float det2_35_34 = mat[3][3] * mat[5][4] - mat[3][4] * mat[5][3];
	float det2_35_35 = mat[3][3] * mat[5][5] - mat[3][5] * mat[5][3];
	float det2_35_45 = mat[3][4] * mat[5][5] - mat[3][5] * mat[5][4];

	// remaining 3x3 sub-determinants
	float det3_234_012 = mat[2][0] * det2_34_12 - mat[2][1] * det2_34_02 + mat[2][2] * det2_34_01;
	float det3_234_013 = mat[2][0] * det2_34_13 - mat[2][1] * det2_34_03 + mat[2][3] * det2_34_01;
	float det3_234_014 = mat[2][0] * det2_34_14 - mat[2][1] * det2_34_04 + mat[2][4] * det2_34_01;
	float det3_234_015 = mat[2][0] * det2_34_15 - mat[2][1] * det2_34_05 + mat[2][5] * det2_34_01;
	float det3_234_023 = mat[2][0] * det2_34_23 - mat[2][2] * det2_34_03 + mat[2][3] * det2_34_02;
	float det3_234_024 = mat[2][0] * det2_34_24 - mat[2][2] * det2_34_04 + mat[2][4] * det2_34_02;
	float det3_234_025 = mat[2][0] * det2_34_25 - mat[2][2] * det2_34_05 + mat[2][5] * det2_34_02;
	float det3_234_034 = mat[2][0] * det2_34_34 - mat[2][3] * det2_34_04 + mat[2][4] * det2_34_03;
	float det3_234_035 = mat[2][0] * det2_34_35 - mat[2][3] * det2_34_05 + mat[2][5] * det2_34_03;
	float det3_234_045 = mat[2][0] * det2_34_45 - mat[2][4] * det2_34_05 + mat[2][5] * det2_34_04;
	float det3_234_123 = mat[2][1] * det2_34_23 - mat[2][2] * det2_34_13 + mat[2][3] * det2_34_12;
	float det3_234_124 = mat[2][1] * det2_34_24 - mat[2][2] * det2_34_14 + mat[2][4] * det2_34_12;
	float det3_234_125 = mat[2][1] * det2_34_25 - mat[2][2] * det2_34_15 + mat[2][5] * det2_34_12;
	float det3_234_134 = mat[2][1] * det2_34_34 - mat[2][3] * det2_34_14 + mat[2][4] * det2_34_13;
	float det3_234_135 = mat[2][1] * det2_34_35 - mat[2][3] * det2_34_15 + mat[2][5] * det2_34_13;
	float det3_234_145 = mat[2][1] * det2_34_45 - mat[2][4] * det2_34_15 + mat[2][5] * det2_34_14;
	float det3_234_234 = mat[2][2] * det2_34_34 - mat[2][3] * det2_34_24 + mat[2][4] * det2_34_23;
	float det3_234_235 = mat[2][2] * det2_34_35 - mat[2][3] * det2_34_25 + mat[2][5] * det2_34_23;
	float det3_234_245 = mat[2][2] * det2_34_45 - mat[2][4] * det2_34_25 + mat[2][5] * det2_34_24;
	float det3_234_345 = mat[2][3] * det2_34_45 - mat[2][4] * det2_34_35 + mat[2][5] * det2_34_34;
	float det3_235_012 = mat[2][0] * det2_35_12 - mat[2][1] * det2_35_02 + mat[2][2] * det2_35_01;
	float det3_235_013 = mat[2][0] * det2_35_13 - mat[2][1] * det2_35_03 + mat[2][3] * det2_35_01;
	float det3_235_014 = mat[2][0] * det2_35_14 - mat[2][1] * det2_35_04 + mat[2][4] * det2_35_01;
	float det3_235_015 = mat[2][0] * det2_35_15 - mat[2][1] * det2_35_05 + mat[2][5] * det2_35_01;
	float det3_235_023 = mat[2][0] * det2_35_23 - mat[2][2] * det2_35_03 + mat[2][3] * det2_35_02;
	float det3_235_024 = mat[2][0] * det2_35_24 - mat[2][2] * det2_35_04 + mat[2][4] * det2_35_02;
	float det3_235_025 = mat[2][0] * det2_35_25 - mat[2][2] * det2_35_05 + mat[2][5] * det2_35_02;
	float det3_235_034 = mat[2][0] * det2_35_34 - mat[2][3] * det2_35_04 + mat[2][4] * det2_35_03;
	float det3_235_035 = mat[2][0] * det2_35_35 - mat[2][3] * det2_35_05 + mat[2][5] * det2_35_03;
	float det3_235_045 = mat[2][0] * det2_35_45 - mat[2][4] * det2_35_05 + mat[2][5] * det2_35_04;
	float det3_235_123 = mat[2][1] * det2_35_23 - mat[2][2] * det2_35_13 + mat[2][3] * det2_35_12;
	float det3_235_124 = mat[2][1] * det2_35_24 - mat[2][2] * det2_35_14 + mat[2][4] * det2_35_12;
	float det3_235_125 = mat[2][1] * det2_35_25 - mat[2][2] * det2_35_15 + mat[2][5] * det2_35_12;
	float det3_235_134 = mat[2][1] * det2_35_34 - mat[2][3] * det2_35_14 + mat[2][4] * det2_35_13;
	float det3_235_135 = mat[2][1] * det2_35_35 - mat[2][3] * det2_35_15 + mat[2][5] * det2_35_13;
	float det3_235_145 = mat[2][1] * det2_35_45 - mat[2][4] * det2_35_15 + mat[2][5] * det2_35_14;
	float det3_235_234 = mat[2][2] * det2_35_34 - mat[2][3] * det2_35_24 + mat[2][4] * det2_35_23;
	float det3_235_235 = mat[2][2] * det2_35_35 - mat[2][3] * det2_35_25 + mat[2][5] * det2_35_23;
	float det3_235_245 = mat[2][2] * det2_35_45 - mat[2][4] * det2_35_25 + mat[2][5] * det2_35_24;
	float det3_235_345 = mat[2][3] * det2_35_45 - mat[2][4] * det2_35_35 + mat[2][5] * det2_35_34;
	float det3_245_012 = mat[2][0] * det2_45_12 - mat[2][1] * det2_45_02 + mat[2][2] * det2_45_01;
	float det3_245_013 = mat[2][0] * det2_45_13 - mat[2][1] * det2_45_03 + mat[2][3] * det2_45_01;
	float det3_245_014 = mat[2][0] * det2_45_14 - mat[2][1] * det2_45_04 + mat[2][4] * det2_45_01;
	float det3_245_015 = mat[2][0] * det2_45_15 - mat[2][1] * det2_45_05 + mat[2][5] * det2_45_01;
	float det3_245_023 = mat[2][0] * det2_45_23 - mat[2][2] * det2_45_03 + mat[2][3] * det2_45_02;
	float det3_245_024 = mat[2][0] * det2_45_24 - mat[2][2] * det2_45_04 + mat[2][4] * det2_45_02;
	float det3_245_025 = mat[2][0] * det2_45_25 - mat[2][2] * det2_45_05 + mat[2][5] * det2_45_02;
	float det3_245_034 = mat[2][0] * det2_45_34 - mat[2][3] * det2_45_04 + mat[2][4] * det2_45_03;
	float det3_245_035 = mat[2][0] * det2_45_35 - mat[2][3] * det2_45_05 + mat[2][5] * det2_45_03;
	float det3_245_045 = mat[2][0] * det2_45_45 - mat[2][4] * det2_45_05 + mat[2][5] * det2_45_04;
	float det3_245_123 = mat[2][1] * det2_45_23 - mat[2][2] * det2_45_13 + mat[2][3] * det2_45_12;
	float det3_245_124 = mat[2][1] * det2_45_24 - mat[2][2] * det2_45_14 + mat[2][4] * det2_45_12;
	float det3_245_125 = mat[2][1] * det2_45_25 - mat[2][2] * det2_45_15 + mat[2][5] * det2_45_12;
	float det3_245_134 = mat[2][1] * det2_45_34 - mat[2][3] * det2_45_14 + mat[2][4] * det2_45_13;
	float det3_245_135 = mat[2][1] * det2_45_35 - mat[2][3] * det2_45_15 + mat[2][5] * det2_45_13;
	float det3_245_145 = mat[2][1] * det2_45_45 - mat[2][4] * det2_45_15 + mat[2][5] * det2_45_14;
	float det3_245_234 = mat[2][2] * det2_45_34 - mat[2][3] * det2_45_24 + mat[2][4] * det2_45_23;
	float det3_245_235 = mat[2][2] * det2_45_35 - mat[2][3] * det2_45_25 + mat[2][5] * det2_45_23;
	float det3_245_245 = mat[2][2] * det2_45_45 - mat[2][4] * det2_45_25 + mat[2][5] * det2_45_24;
	float det3_245_345 = mat[2][3] * det2_45_45 - mat[2][4] * det2_45_35 + mat[2][5] * det2_45_34;

	// remaining 4x4 sub-determinants
	float det4_1234_0123 = mat[1][0] * det3_234_123 - mat[1][1] * det3_234_023 + mat[1][2] * det3_234_013 - mat[1][3] * det3_234_012;
	float det4_1234_0124 = mat[1][0] * det3_234_124 - mat[1][1] * det3_234_024 + mat[1][2] * det3_234_014 - mat[1][4] * det3_234_012;
	float det4_1234_0125 = mat[1][0] * det3_234_125 - mat[1][1] * det3_234_025 + mat[1][2] * det3_234_015 - mat[1][5] * det3_234_012;
	float det4_1234_0134 = mat[1][0] * det3_234_134 - mat[1][1] * det3_234_034 + mat[1][3] * det3_234_014 - mat[1][4] * det3_234_013;
	float det4_1234_0135 = mat[1][0] * det3_234_135 - mat[1][1] * det3_234_035 + mat[1][3] * det3_234_015 - mat[1][5] * det3_234_013;
	float det4_1234_0145 = mat[1][0] * det3_234_145 - mat[1][1] * det3_234_045 + mat[1][4] * det3_234_015 - mat[1][5] * det3_234_014;
	float det4_1234_0234 = mat[1][0] * det3_234_234 - mat[1][2] * det3_234_034 + mat[1][3] * det3_234_024 - mat[1][4] * det3_234_023;
	float det4_1234_0235 = mat[1][0] * det3_234_235 - mat[1][2] * det3_234_035 + mat[1][3] * det3_234_025 - mat[1][5] * det3_234_023;
	float det4_1234_0245 = mat[1][0] * det3_234_245 - mat[1][2] * det3_234_045 + mat[1][4] * det3_234_025 - mat[1][5] * det3_234_024;
	float det4_1234_0345 = mat[1][0] * det3_234_345 - mat[1][3] * det3_234_045 + mat[1][4] * det3_234_035 - mat[1][5] * det3_234_034;
	float det4_1234_1234 = mat[1][1] * det3_234_234 - mat[1][2] * det3_234_134 + mat[1][3] * det3_234_124 - mat[1][4] * det3_234_123;
	float det4_1234_1235 = mat[1][1] * det3_234_235 - mat[1][2] * det3_234_135 + mat[1][3] * det3_234_125 - mat[1][5] * det3_234_123;
	float det4_1234_1245 = mat[1][1] * det3_234_245 - mat[1][2] * det3_234_145 + mat[1][4] * det3_234_125 - mat[1][5] * det3_234_124;
	float det4_1234_1345 = mat[1][1] * det3_234_345 - mat[1][3] * det3_234_145 + mat[1][4] * det3_234_135 - mat[1][5] * det3_234_134;
	float det4_1234_2345 = mat[1][2] * det3_234_345 - mat[1][3] * det3_234_245 + mat[1][4] * det3_234_235 - mat[1][5] * det3_234_234;
	float det4_1235_0123 = mat[1][0] * det3_235_123 - mat[1][1] * det3_235_023 + mat[1][2] * det3_235_013 - mat[1][3] * det3_235_012;
	float det4_1235_0124 = mat[1][0] * det3_235_124 - mat[1][1] * det3_235_024 + mat[1][2] * det3_235_014 - mat[1][4] * det3_235_012;
	float det4_1235_0125 = mat[1][0] * det3_235_125 - mat[1][1] * det3_235_025 + mat[1][2] * det3_235_015 - mat[1][5] * det3_235_012;
	float det4_1235_0134 = mat[1][0] * det3_235_134 - mat[1][1] * det3_235_034 + mat[1][3] * det3_235_014 - mat[1][4] * det3_235_013;
	float det4_1235_0135 = mat[1][0] * det3_235_135 - mat[1][1] * det3_235_035 + mat[1][3] * det3_235_015 - mat[1][5] * det3_235_013;
	float det4_1235_0145 = mat[1][0] * det3_235_145 - mat[1][1] * det3_235_045 + mat[1][4] * det3_235_015 - mat[1][5] * det3_235_014;
	float det4_1235_0234 = mat[1][0] * det3_235_234 - mat[1][2] * det3_235_034 + mat[1][3] * det3_235_024 - mat[1][4] * det3_235_023;
	float det4_1235_0235 = mat[1][0] * det3_235_235 - mat[1][2] * det3_235_035 + mat[1][3] * det3_235_025 - mat[1][5] * det3_235_023;
	float det4_1235_0245 = mat[1][0] * det3_235_245 - mat[1][2] * det3_235_045 + mat[1][4] * det3_235_025 - mat[1][5] * det3_235_024;
	float det4_1235_0345 = mat[1][0] * det3_235_345 - mat[1][3] * det3_235_045 + mat[1][4] * det3_235_035 - mat[1][5] * det3_235_034;
	float det4_1235_1234 = mat[1][1] * det3_235_234 - mat[1][2] * det3_235_134 + mat[1][3] * det3_235_124 - mat[1][4] * det3_235_123;
	float det4_1235_1235 = mat[1][1] * det3_235_235 - mat[1][2] * det3_235_135 + mat[1][3] * det3_235_125 - mat[1][5] * det3_235_123;
	float det4_1235_1245 = mat[1][1] * det3_235_245 - mat[1][2] * det3_235_145 + mat[1][4] * det3_235_125 - mat[1][5] * det3_235_124;
	float det4_1235_1345 = mat[1][1] * det3_235_345 - mat[1][3] * det3_235_145 + mat[1][4] * det3_235_135 - mat[1][5] * det3_235_134;
	float det4_1235_2345 = mat[1][2] * det3_235_345 - mat[1][3] * det3_235_245 + mat[1][4] * det3_235_235 - mat[1][5] * det3_235_234;
	float det4_1245_0123 = mat[1][0] * det3_245_123 - mat[1][1] * det3_245_023 + mat[1][2] * det3_245_013 - mat[1][3] * det3_245_012;
	float det4_1245_0124 = mat[1][0] * det3_245_124 - mat[1][1] * det3_245_024 + mat[1][2] * det3_245_014 - mat[1][4] * det3_245_012;
	float det4_1245_0125 = mat[1][0] * det3_245_125 - mat[1][1] * det3_245_025 + mat[1][2] * det3_245_015 - mat[1][5] * det3_245_012;
	float det4_1245_0134 = mat[1][0] * det3_245_134 - mat[1][1] * det3_245_034 + mat[1][3] * det3_245_014 - mat[1][4] * det3_245_013;
	float det4_1245_0135 = mat[1][0] * det3_245_135 - mat[1][1] * det3_245_035 + mat[1][3] * det3_245_015 - mat[1][5] * det3_245_013;
	float det4_1245_0145 = mat[1][0] * det3_245_145 - mat[1][1] * det3_245_045 + mat[1][4] * det3_245_015 - mat[1][5] * det3_245_014;
	float det4_1245_0234 = mat[1][0] * det3_245_234 - mat[1][2] * det3_245_034 + mat[1][3] * det3_245_024 - mat[1][4] * det3_245_023;
	float det4_1245_0235 = mat[1][0] * det3_245_235 - mat[1][2] * det3_245_035 + mat[1][3] * det3_245_025 - mat[1][5] * det3_245_023;
	float det4_1245_0245 = mat[1][0] * det3_245_245 - mat[1][2] * det3_245_045 + mat[1][4] * det3_245_025 - mat[1][5] * det3_245_024;
	float det4_1245_0345 = mat[1][0] * det3_245_345 - mat[1][3] * det3_245_045 + mat[1][4] * det3_245_035 - mat[1][5] * det3_245_034;
	float det4_1245_1234 = mat[1][1] * det3_245_234 - mat[1][2] * det3_245_134 + mat[1][3] * det3_245_124 - mat[1][4] * det3_245_123;
	float det4_1245_1235 = mat[1][1] * det3_245_235 - mat[1][2] * det3_245_135 + mat[1][3] * det3_245_125 - mat[1][5] * det3_245_123;
	float det4_1245_1245 = mat[1][1] * det3_245_245 - mat[1][2] * det3_245_145 + mat[1][4] * det3_245_125 - mat[1][5] * det3_245_124;
	float det4_1245_1345 = mat[1][1] * det3_245_345 - mat[1][3] * det3_245_145 + mat[1][4] * det3_245_135 - mat[1][5] * det3_245_134;
	float det4_1245_2345 = mat[1][2] * det3_245_345 - mat[1][3] * det3_245_245 + mat[1][4] * det3_245_235 - mat[1][5] * det3_245_234;
	float det4_1345_0123 = mat[1][0] * det3_345_123 - mat[1][1] * det3_345_023 + mat[1][2] * det3_345_013 - mat[1][3] * det3_345_012;
	float det4_1345_0124 = mat[1][0] * det3_345_124 - mat[1][1] * det3_345_024 + mat[1][2] * det3_345_014 - mat[1][4] * det3_345_012;
	float det4_1345_0125 = mat[1][0] * det3_345_125 - mat[1][1] * det3_345_025 + mat[1][2] * det3_345_015 - mat[1][5] * det3_345_012;
	float det4_1345_0134 = mat[1][0] * det3_345_134 - mat[1][1] * det3_345_034 + mat[1][3] * det3_345_014 - mat[1][4] * det3_345_013;
	float det4_1345_0135 = mat[1][0] * det3_345_135 - mat[1][1] * det3_345_035 + mat[1][3] * det3_345_015 - mat[1][5] * det3_345_013;
	float det4_1345_0145 = mat[1][0] * det3_345_145 - mat[1][1] * det3_345_045 + mat[1][4] * det3_345_015 - mat[1][5] * det3_345_014;
	float det4_1345_0234 = mat[1][0] * det3_345_234 - mat[1][2] * det3_345_034 + mat[1][3] * det3_345_024 - mat[1][4] * det3_345_023;
	float det4_1345_0235 = mat[1][0] * det3_345_235 - mat[1][2] * det3_345_035 + mat[1][3] * det3_345_025 - mat[1][5] * det3_345_023;
	float det4_1345_0245 = mat[1][0] * det3_345_245 - mat[1][2] * det3_345_045 + mat[1][4] * det3_345_025 - mat[1][5] * det3_345_024;
	float det4_1345_0345 = mat[1][0] * det3_345_345 - mat[1][3] * det3_345_045 + mat[1][4] * det3_345_035 - mat[1][5] * det3_345_034;
	float det4_1345_1234 = mat[1][1] * det3_345_234 - mat[1][2] * det3_345_134 + mat[1][3] * det3_345_124 - mat[1][4] * det3_345_123;
	float det4_1345_1235 = mat[1][1] * det3_345_235 - mat[1][2] * det3_345_135 + mat[1][3] * det3_345_125 - mat[1][5] * det3_345_123;
	float det4_1345_1245 = mat[1][1] * det3_345_245 - mat[1][2] * det3_345_145 + mat[1][4] * det3_345_125 - mat[1][5] * det3_345_124;
	float det4_1345_1345 = mat[1][1] * det3_345_345 - mat[1][3] * det3_345_145 + mat[1][4] * det3_345_135 - mat[1][5] * det3_345_134;
	float det4_1345_2345 = mat[1][2] * det3_345_345 - mat[1][3] * det3_345_245 + mat[1][4] * det3_345_235 - mat[1][5] * det3_345_234;

	// remaining 5x5 sub-determinants
	float det5_01234_01234 = mat[0][0] * det4_1234_1234 - mat[0][1] * det4_1234_0234 + mat[0][2] * det4_1234_0134 - mat[0][3] * det4_1234_0124 + mat[0][4] * det4_1234_0123;
	float det5_01234_01235 = mat[0][0] * det4_1234_1235 - mat[0][1] * det4_1234_0235 + mat[0][2] * det4_1234_0135 - mat[0][3] * det4_1234_0125 + mat[0][5] * det4_1234_0123;
	float det5_01234_01245 = mat[0][0] * det4_1234_1245 - mat[0][1] * det4_1234_0245 + mat[0][2] * det4_1234_0145 - mat[0][4] * det4_1234_0125 + mat[0][5] * det4_1234_0124;
	float det5_01234_01345 = mat[0][0] * det4_1234_1345 - mat[0][1] * det4_1234_0345 + mat[0][3] * det4_1234_0145 - mat[0][4] * det4_1234_0135 + mat[0][5] * det4_1234_0134;
	float det5_01234_02345 = mat[0][0] * det4_1234_2345 - mat[0][2] * det4_1234_0345 + mat[0][3] * det4_1234_0245 - mat[0][4] * det4_1234_0235 + mat[0][5] * det4_1234_0234;
	float det5_01234_12345 = mat[0][1] * det4_1234_2345 - mat[0][2] * det4_1234_1345 + mat[0][3] * det4_1234_1245 - mat[0][4] * det4_1234_1235 + mat[0][5] * det4_1234_1234;
	float det5_01235_01234 = mat[0][0] * det4_1235_1234 - mat[0][1] * det4_1235_0234 + mat[0][2] * det4_1235_0134 - mat[0][3] * det4_1235_0124 + mat[0][4] * det4_1235_0123;
	float det5_01235_01235 = mat[0][0] * det4_1235_1235 - mat[0][1] * det4_1235_0235 + mat[0][2] * det4_1235_0135 - mat[0][3] * det4_1235_0125 + mat[0][5] * det4_1235_0123;
	float det5_01235_01245 = mat[0][0] * det4_1235_1245 - mat[0][1] * det4_1235_0245 + mat[0][2] * det4_1235_0145 - mat[0][4] * det4_1235_0125 + mat[0][5] * det4_1235_0124;
	float det5_01235_01345 = mat[0][0] * det4_1235_1345 - mat[0][1] * det4_1235_0345 + mat[0][3] * det4_1235_0145 - mat[0][4] * det4_1235_0135 + mat[0][5] * det4_1235_0134;
	float det5_01235_02345 = mat[0][0] * det4_1235_2345 - mat[0][2] * det4_1235_0345 + mat[0][3] * det4_1235_0245 - mat[0][4] * det4_1235_0235 + mat[0][5] * det4_1235_0234;
	float det5_01235_12345 = mat[0][1] * det4_1235_2345 - mat[0][2] * det4_1235_1345 + mat[0][3] * det4_1235_1245 - mat[0][4] * det4_1235_1235 + mat[0][5] * det4_1235_1234;
	float det5_01245_01234 = mat[0][0] * det4_1245_1234 - mat[0][1] * det4_1245_0234 + mat[0][2] * det4_1245_0134 - mat[0][3] * det4_1245_0124 + mat[0][4] * det4_1245_0123;
	float det5_01245_01235 = mat[0][0] * det4_1245_1235 - mat[0][1] * det4_1245_0235 + mat[0][2] * det4_1245_0135 - mat[0][3] * det4_1245_0125 + mat[0][5] * det4_1245_0123;
	float det5_01245_01245 = mat[0][0] * det4_1245_1245 - mat[0][1] * det4_1245_0245 + mat[0][2] * det4_1245_0145 - mat[0][4] * det4_1245_0125 + mat[0][5] * det4_1245_0124;
	float det5_01245_01345 = mat[0][0] * det4_1245_1345 - mat[0][1] * det4_1245_0345 + mat[0][3] * det4_1245_0145 - mat[0][4] * det4_1245_0135 + mat[0][5] * det4_1245_0134;
	float det5_01245_02345 = mat[0][0] * det4_1245_2345 - mat[0][2] * det4_1245_0345 + mat[0][3] * det4_1245_0245 - mat[0][4] * det4_1245_0235 + mat[0][5] * det4_1245_0234;
	float det5_01245_12345 = mat[0][1] * det4_1245_2345 - mat[0][2] * det4_1245_1345 + mat[0][3] * det4_1245_1245 - mat[0][4] * det4_1245_1235 + mat[0][5] * det4_1245_1234;
	float det5_01345_01234 = mat[0][0] * det4_1345_1234 - mat[0][1] * det4_1345_0234 + mat[0][2] * det4_1345_0134 - mat[0][3] * det4_1345_0124 + mat[0][4] * det4_1345_0123;
	float det5_01345_01235 = mat[0][0] * det4_1345_1235 - mat[0][1] * det4_1345_0235 + mat[0][2] * det4_1345_0135 - mat[0][3] * det4_1345_0125 + mat[0][5] * det4_1345_0123;
	float det5_01345_01245 = mat[0][0] * det4_1345_1245 - mat[0][1] * det4_1345_0245 + mat[0][2] * det4_1345_0145 - mat[0][4] * det4_1345_0125 + mat[0][5] * det4_1345_0124;
	float det5_01345_01345 = mat[0][0] * det4_1345_1345 - mat[0][1] * det4_1345_0345 + mat[0][3] * det4_1345_0145 - mat[0][4] * det4_1345_0135 + mat[0][5] * det4_1345_0134;
	float det5_01345_02345 = mat[0][0] * det4_1345_2345 - mat[0][2] * det4_1345_0345 + mat[0][3] * det4_1345_0245 - mat[0][4] * det4_1345_0235 + mat[0][5] * det4_1345_0234;
	float det5_01345_12345 = mat[0][1] * det4_1345_2345 - mat[0][2] * det4_1345_1345 + mat[0][3] * det4_1345_1245 - mat[0][4] * det4_1345_1235 + mat[0][5] * det4_1345_1234;
	float det5_02345_01234 = mat[0][0] * det4_2345_1234 - mat[0][1] * det4_2345_0234 + mat[0][2] * det4_2345_0134 - mat[0][3] * det4_2345_0124 + mat[0][4] * det4_2345_0123;
	float det5_02345_01235 = mat[0][0] * det4_2345_1235 - mat[0][1] * det4_2345_0235 + mat[0][2] * det4_2345_0135 - mat[0][3] * det4_2345_0125 + mat[0][5] * det4_2345_0123;
	float det5_02345_01245 = mat[0][0] * det4_2345_1245 - mat[0][1] * det4_2345_0245 + mat[0][2] * det4_2345_0145 - mat[0][4] * det4_2345_0125 + mat[0][5] * det4_2345_0124;
	float det5_02345_01345 = mat[0][0] * det4_2345_1345 - mat[0][1] * det4_2345_0345 + mat[0][3] * det4_2345_0145 - mat[0][4] * det4_2345_0135 + mat[0][5] * det4_2345_0134;
	float det5_02345_02345 = mat[0][0] * det4_2345_2345 - mat[0][2] * det4_2345_0345 + mat[0][3] * det4_2345_0245 - mat[0][4] * det4_2345_0235 + mat[0][5] * det4_2345_0234;
	float det5_02345_12345 = mat[0][1] * det4_2345_2345 - mat[0][2] * det4_2345_1345 + mat[0][3] * det4_2345_1245 - mat[0][4] * det4_2345_1235 + mat[0][5] * det4_2345_1234;

	mat[0][0] =  det5_12345_12345 * invDet;
	mat[0][1] = -det5_02345_12345 * invDet;
	mat[0][2] =  det5_01345_12345 * invDet;
	mat[0][3] = -det5_01245_12345 * invDet;
	mat[0][4] =  det5_01235_12345 * invDet;
	mat[0][5] = -det5_01234_12345 * invDet;

	mat[1][0] = -det5_12345_02345 * invDet;
	mat[1][1] =  det5_02345_02345 * invDet;
	mat[1][2] = -det5_01345_02345 * invDet;
	mat[1][3] =  det5_01245_02345 * invDet;
	mat[1][4] = -det5_01235_02345 * invDet;
	mat[1][5] =  det5_01234_02345 * invDet;

	mat[2][0] =  det5_12345_01345 * invDet;
	mat[2][1] = -det5_02345_01345 * invDet;
	mat[2][2] =  det5_01345_01345 * invDet;
	mat[2][3] = -det5_01245_01345 * invDet;
	mat[2][4] =  det5_01235_01345 * invDet;
	mat[2][5] = -det5_01234_01345 * invDet;

	mat[3][0] = -det5_12345_01245 * invDet;
	mat[3][1] =  det5_02345_01245 * invDet;
	mat[3][2] = -det5_01345_01245 * invDet;
	mat[3][3] =  det5_01245_01245 * invDet;
	mat[3][4] = -det5_01235_01245 * invDet;
	mat[3][5] =  det5_01234_01245 * invDet;

	mat[4][0] =  det5_12345_01235 * invDet;
	mat[4][1] = -det5_02345_01235 * invDet;
	mat[4][2] =  det5_01345_01235 * invDet;
	mat[4][3] = -det5_01245_01235 * invDet;
	mat[4][4] =  det5_01235_01235 * invDet;
	mat[4][5] = -det5_01234_01235 * invDet;

	mat[5][0] = -det5_12345_01234 * invDet;
	mat[5][1] =  det5_02345_01234 * invDet;
	mat[5][2] = -det5_01345_01234 * invDet;
	mat[5][3] =  det5_01245_01234 * invDet;
	mat[5][4] = -det5_01235_01234 * invDet;
	mat[5][5] =  det5_01234_01234 * invDet;

	return true;
}

/*
============
arcMat6::InverseFastSelf
============
*/
bool arcMat6::InverseFastSelf( void ) {
#if 0
	// 810+6+36 = 852 multiplications
	//				1 division
	double det, invDet;

	// 2x2 sub-determinants required to calculate 6x6 determinant
	float det2_45_01 = mat[4][0] * mat[5][1] - mat[4][1] * mat[5][0];
	float det2_45_02 = mat[4][0] * mat[5][2] - mat[4][2] * mat[5][0];
	float det2_45_03 = mat[4][0] * mat[5][3] - mat[4][3] * mat[5][0];
	float det2_45_04 = mat[4][0] * mat[5][4] - mat[4][4] * mat[5][0];
	float det2_45_05 = mat[4][0] * mat[5][5] - mat[4][5] * mat[5][0];
	float det2_45_12 = mat[4][1] * mat[5][2] - mat[4][2] * mat[5][1];
	float det2_45_13 = mat[4][1] * mat[5][3] - mat[4][3] * mat[5][1];
	float det2_45_14 = mat[4][1] * mat[5][4] - mat[4][4] * mat[5][1];
	float det2_45_15 = mat[4][1] * mat[5][5] - mat[4][5] * mat[5][1];
	float det2_45_23 = mat[4][2] * mat[5][3] - mat[4][3] * mat[5][2];
	float det2_45_24 = mat[4][2] * mat[5][4] - mat[4][4] * mat[5][2];
	float det2_45_25 = mat[4][2] * mat[5][5] - mat[4][5] * mat[5][2];
	float det2_45_34 = mat[4][3] * mat[5][4] - mat[4][4] * mat[5][3];
	float det2_45_35 = mat[4][3] * mat[5][5] - mat[4][5] * mat[5][3];
	float det2_45_45 = mat[4][4] * mat[5][5] - mat[4][5] * mat[5][4];

	// 3x3 sub-determinants required to calculate 6x6 determinant
	float det3_345_012 = mat[3][0] * det2_45_12 - mat[3][1] * det2_45_02 + mat[3][2] * det2_45_01;
	float det3_345_013 = mat[3][0] * det2_45_13 - mat[3][1] * det2_45_03 + mat[3][3] * det2_45_01;
	float det3_345_014 = mat[3][0] * det2_45_14 - mat[3][1] * det2_45_04 + mat[3][4] * det2_45_01;
	float det3_345_015 = mat[3][0] * det2_45_15 - mat[3][1] * det2_45_05 + mat[3][5] * det2_45_01;
	float det3_345_023 = mat[3][0] * det2_45_23 - mat[3][2] * det2_45_03 + mat[3][3] * det2_45_02;
	float det3_345_024 = mat[3][0] * det2_45_24 - mat[3][2] * det2_45_04 + mat[3][4] * det2_45_02;
	float det3_345_025 = mat[3][0] * det2_45_25 - mat[3][2] * det2_45_05 + mat[3][5] * det2_45_02;
	float det3_345_034 = mat[3][0] * det2_45_34 - mat[3][3] * det2_45_04 + mat[3][4] * det2_45_03;
	float det3_345_035 = mat[3][0] * det2_45_35 - mat[3][3] * det2_45_05 + mat[3][5] * det2_45_03;
	float det3_345_045 = mat[3][0] * det2_45_45 - mat[3][4] * det2_45_05 + mat[3][5] * det2_45_04;
	float det3_345_123 = mat[3][1] * det2_45_23 - mat[3][2] * det2_45_13 + mat[3][3] * det2_45_12;
	float det3_345_124 = mat[3][1] * det2_45_24 - mat[3][2] * det2_45_14 + mat[3][4] * det2_45_12;
	float det3_345_125 = mat[3][1] * det2_45_25 - mat[3][2] * det2_45_15 + mat[3][5] * det2_45_12;
	float det3_345_134 = mat[3][1] * det2_45_34 - mat[3][3] * det2_45_14 + mat[3][4] * det2_45_13;
	float det3_345_135 = mat[3][1] * det2_45_35 - mat[3][3] * det2_45_15 + mat[3][5] * det2_45_13;
	float det3_345_145 = mat[3][1] * det2_45_45 - mat[3][4] * det2_45_15 + mat[3][5] * det2_45_14;
	float det3_345_234 = mat[3][2] * det2_45_34 - mat[3][3] * det2_45_24 + mat[3][4] * det2_45_23;
	float det3_345_235 = mat[3][2] * det2_45_35 - mat[3][3] * det2_45_25 + mat[3][5] * det2_45_23;
	float det3_345_245 = mat[3][2] * det2_45_45 - mat[3][4] * det2_45_25 + mat[3][5] * det2_45_24;
	float det3_345_345 = mat[3][3] * det2_45_45 - mat[3][4] * det2_45_35 + mat[3][5] * det2_45_34;

	// 4x4 sub-determinants required to calculate 6x6 determinant
	float det4_2345_0123 = mat[2][0] * det3_345_123 - mat[2][1] * det3_345_023 + mat[2][2] * det3_345_013 - mat[2][3] * det3_345_012;
	float det4_2345_0124 = mat[2][0] * det3_345_124 - mat[2][1] * det3_345_024 + mat[2][2] * det3_345_014 - mat[2][4] * det3_345_012;
	float det4_2345_0125 = mat[2][0] * det3_345_125 - mat[2][1] * det3_345_025 + mat[2][2] * det3_345_015 - mat[2][5] * det3_345_012;
	float det4_2345_0134 = mat[2][0] * det3_345_134 - mat[2][1] * det3_345_034 + mat[2][3] * det3_345_014 - mat[2][4] * det3_345_013;
	float det4_2345_0135 = mat[2][0] * det3_345_135 - mat[2][1] * det3_345_035 + mat[2][3] * det3_345_015 - mat[2][5] * det3_345_013;
	float det4_2345_0145 = mat[2][0] * det3_345_145 - mat[2][1] * det3_345_045 + mat[2][4] * det3_345_015 - mat[2][5] * det3_345_014;
	float det4_2345_0234 = mat[2][0] * det3_345_234 - mat[2][2] * det3_345_034 + mat[2][3] * det3_345_024 - mat[2][4] * det3_345_023;
	float det4_2345_0235 = mat[2][0] * det3_345_235 - mat[2][2] * det3_345_035 + mat[2][3] * det3_345_025 - mat[2][5] * det3_345_023;
	float det4_2345_0245 = mat[2][0] * det3_345_245 - mat[2][2] * det3_345_045 + mat[2][4] * det3_345_025 - mat[2][5] * det3_345_024;
	float det4_2345_0345 = mat[2][0] * det3_345_345 - mat[2][3] * det3_345_045 + mat[2][4] * det3_345_035 - mat[2][5] * det3_345_034;
	float det4_2345_1234 = mat[2][1] * det3_345_234 - mat[2][2] * det3_345_134 + mat[2][3] * det3_345_124 - mat[2][4] * det3_345_123;
	float det4_2345_1235 = mat[2][1] * det3_345_235 - mat[2][2] * det3_345_135 + mat[2][3] * det3_345_125 - mat[2][5] * det3_345_123;
	float det4_2345_1245 = mat[2][1] * det3_345_245 - mat[2][2] * det3_345_145 + mat[2][4] * det3_345_125 - mat[2][5] * det3_345_124;
	float det4_2345_1345 = mat[2][1] * det3_345_345 - mat[2][3] * det3_345_145 + mat[2][4] * det3_345_135 - mat[2][5] * det3_345_134;
	float det4_2345_2345 = mat[2][2] * det3_345_345 - mat[2][3] * det3_345_245 + mat[2][4] * det3_345_235 - mat[2][5] * det3_345_234;

	// 5x5 sub-determinants required to calculate 6x6 determinant
	float det5_12345_01234 = mat[1][0] * det4_2345_1234 - mat[1][1] * det4_2345_0234 + mat[1][2] * det4_2345_0134 - mat[1][3] * det4_2345_0124 + mat[1][4] * det4_2345_0123;
	float det5_12345_01235 = mat[1][0] * det4_2345_1235 - mat[1][1] * det4_2345_0235 + mat[1][2] * det4_2345_0135 - mat[1][3] * det4_2345_0125 + mat[1][5] * det4_2345_0123;
	float det5_12345_01245 = mat[1][0] * det4_2345_1245 - mat[1][1] * det4_2345_0245 + mat[1][2] * det4_2345_0145 - mat[1][4] * det4_2345_0125 + mat[1][5] * det4_2345_0124;
	float det5_12345_01345 = mat[1][0] * det4_2345_1345 - mat[1][1] * det4_2345_0345 + mat[1][3] * det4_2345_0145 - mat[1][4] * det4_2345_0135 + mat[1][5] * det4_2345_0134;
	float det5_12345_02345 = mat[1][0] * det4_2345_2345 - mat[1][2] * det4_2345_0345 + mat[1][3] * det4_2345_0245 - mat[1][4] * det4_2345_0235 + mat[1][5] * det4_2345_0234;
	float det5_12345_12345 = mat[1][1] * det4_2345_2345 - mat[1][2] * det4_2345_1345 + mat[1][3] * det4_2345_1245 - mat[1][4] * det4_2345_1235 + mat[1][5] * det4_2345_1234;

	// determinant of 6x6 matrix
	det = mat[0][0] * det5_12345_12345 - mat[0][1] * det5_12345_02345 + mat[0][2] * det5_12345_01345 -
				mat[0][3] * det5_12345_01245 + mat[0][4] * det5_12345_01235 - mat[0][5] * det5_12345_01234;

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	// remaining 2x2 sub-determinants
	float det2_34_01 = mat[3][0] * mat[4][1] - mat[3][1] * mat[4][0];
	float det2_34_02 = mat[3][0] * mat[4][2] - mat[3][2] * mat[4][0];
	float det2_34_03 = mat[3][0] * mat[4][3] - mat[3][3] * mat[4][0];
	float det2_34_04 = mat[3][0] * mat[4][4] - mat[3][4] * mat[4][0];
	float det2_34_05 = mat[3][0] * mat[4][5] - mat[3][5] * mat[4][0];
	float det2_34_12 = mat[3][1] * mat[4][2] - mat[3][2] * mat[4][1];
	float det2_34_13 = mat[3][1] * mat[4][3] - mat[3][3] * mat[4][1];
	float det2_34_14 = mat[3][1] * mat[4][4] - mat[3][4] * mat[4][1];
	float det2_34_15 = mat[3][1] * mat[4][5] - mat[3][5] * mat[4][1];
	float det2_34_23 = mat[3][2] * mat[4][3] - mat[3][3] * mat[4][2];
	float det2_34_24 = mat[3][2] * mat[4][4] - mat[3][4] * mat[4][2];
	float det2_34_25 = mat[3][2] * mat[4][5] - mat[3][5] * mat[4][2];
	float det2_34_34 = mat[3][3] * mat[4][4] - mat[3][4] * mat[4][3];
	float det2_34_35 = mat[3][3] * mat[4][5] - mat[3][5] * mat[4][3];
	float det2_34_45 = mat[3][4] * mat[4][5] - mat[3][5] * mat[4][4];
	float det2_35_01 = mat[3][0] * mat[5][1] - mat[3][1] * mat[5][0];
	float det2_35_02 = mat[3][0] * mat[5][2] - mat[3][2] * mat[5][0];
	float det2_35_03 = mat[3][0] * mat[5][3] - mat[3][3] * mat[5][0];
	float det2_35_04 = mat[3][0] * mat[5][4] - mat[3][4] * mat[5][0];
	float det2_35_05 = mat[3][0] * mat[5][5] - mat[3][5] * mat[5][0];
	float det2_35_12 = mat[3][1] * mat[5][2] - mat[3][2] * mat[5][1];
	float det2_35_13 = mat[3][1] * mat[5][3] - mat[3][3] * mat[5][1];
	float det2_35_14 = mat[3][1] * mat[5][4] - mat[3][4] * mat[5][1];
	float det2_35_15 = mat[3][1] * mat[5][5] - mat[3][5] * mat[5][1];
	float det2_35_23 = mat[3][2] * mat[5][3] - mat[3][3] * mat[5][2];
	float det2_35_24 = mat[3][2] * mat[5][4] - mat[3][4] * mat[5][2];
	float det2_35_25 = mat[3][2] * mat[5][5] - mat[3][5] * mat[5][2];
	float det2_35_34 = mat[3][3] * mat[5][4] - mat[3][4] * mat[5][3];
	float det2_35_35 = mat[3][3] * mat[5][5] - mat[3][5] * mat[5][3];
	float det2_35_45 = mat[3][4] * mat[5][5] - mat[3][5] * mat[5][4];

	// remaining 3x3 sub-determinants
	float det3_234_012 = mat[2][0] * det2_34_12 - mat[2][1] * det2_34_02 + mat[2][2] * det2_34_01;
	float det3_234_013 = mat[2][0] * det2_34_13 - mat[2][1] * det2_34_03 + mat[2][3] * det2_34_01;
	float det3_234_014 = mat[2][0] * det2_34_14 - mat[2][1] * det2_34_04 + mat[2][4] * det2_34_01;
	float det3_234_015 = mat[2][0] * det2_34_15 - mat[2][1] * det2_34_05 + mat[2][5] * det2_34_01;
	float det3_234_023 = mat[2][0] * det2_34_23 - mat[2][2] * det2_34_03 + mat[2][3] * det2_34_02;
	float det3_234_024 = mat[2][0] * det2_34_24 - mat[2][2] * det2_34_04 + mat[2][4] * det2_34_02;
	float det3_234_025 = mat[2][0] * det2_34_25 - mat[2][2] * det2_34_05 + mat[2][5] * det2_34_02;
	float det3_234_034 = mat[2][0] * det2_34_34 - mat[2][3] * det2_34_04 + mat[2][4] * det2_34_03;
	float det3_234_035 = mat[2][0] * det2_34_35 - mat[2][3] * det2_34_05 + mat[2][5] * det2_34_03;
	float det3_234_045 = mat[2][0] * det2_34_45 - mat[2][4] * det2_34_05 + mat[2][5] * det2_34_04;
	float det3_234_123 = mat[2][1] * det2_34_23 - mat[2][2] * det2_34_13 + mat[2][3] * det2_34_12;
	float det3_234_124 = mat[2][1] * det2_34_24 - mat[2][2] * det2_34_14 + mat[2][4] * det2_34_12;
	float det3_234_125 = mat[2][1] * det2_34_25 - mat[2][2] * det2_34_15 + mat[2][5] * det2_34_12;
	float det3_234_134 = mat[2][1] * det2_34_34 - mat[2][3] * det2_34_14 + mat[2][4] * det2_34_13;
	float det3_234_135 = mat[2][1] * det2_34_35 - mat[2][3] * det2_34_15 + mat[2][5] * det2_34_13;
	float det3_234_145 = mat[2][1] * det2_34_45 - mat[2][4] * det2_34_15 + mat[2][5] * det2_34_14;
	float det3_234_234 = mat[2][2] * det2_34_34 - mat[2][3] * det2_34_24 + mat[2][4] * det2_34_23;
	float det3_234_235 = mat[2][2] * det2_34_35 - mat[2][3] * det2_34_25 + mat[2][5] * det2_34_23;
	float det3_234_245 = mat[2][2] * det2_34_45 - mat[2][4] * det2_34_25 + mat[2][5] * det2_34_24;
	float det3_234_345 = mat[2][3] * det2_34_45 - mat[2][4] * det2_34_35 + mat[2][5] * det2_34_34;
	float det3_235_012 = mat[2][0] * det2_35_12 - mat[2][1] * det2_35_02 + mat[2][2] * det2_35_01;
	float det3_235_013 = mat[2][0] * det2_35_13 - mat[2][1] * det2_35_03 + mat[2][3] * det2_35_01;
	float det3_235_014 = mat[2][0] * det2_35_14 - mat[2][1] * det2_35_04 + mat[2][4] * det2_35_01;
	float det3_235_015 = mat[2][0] * det2_35_15 - mat[2][1] * det2_35_05 + mat[2][5] * det2_35_01;
	float det3_235_023 = mat[2][0] * det2_35_23 - mat[2][2] * det2_35_03 + mat[2][3] * det2_35_02;
	float det3_235_024 = mat[2][0] * det2_35_24 - mat[2][2] * det2_35_04 + mat[2][4] * det2_35_02;
	float det3_235_025 = mat[2][0] * det2_35_25 - mat[2][2] * det2_35_05 + mat[2][5] * det2_35_02;
	float det3_235_034 = mat[2][0] * det2_35_34 - mat[2][3] * det2_35_04 + mat[2][4] * det2_35_03;
	float det3_235_035 = mat[2][0] * det2_35_35 - mat[2][3] * det2_35_05 + mat[2][5] * det2_35_03;
	float det3_235_045 = mat[2][0] * det2_35_45 - mat[2][4] * det2_35_05 + mat[2][5] * det2_35_04;
	float det3_235_123 = mat[2][1] * det2_35_23 - mat[2][2] * det2_35_13 + mat[2][3] * det2_35_12;
	float det3_235_124 = mat[2][1] * det2_35_24 - mat[2][2] * det2_35_14 + mat[2][4] * det2_35_12;
	float det3_235_125 = mat[2][1] * det2_35_25 - mat[2][2] * det2_35_15 + mat[2][5] * det2_35_12;
	float det3_235_134 = mat[2][1] * det2_35_34 - mat[2][3] * det2_35_14 + mat[2][4] * det2_35_13;
	float det3_235_135 = mat[2][1] * det2_35_35 - mat[2][3] * det2_35_15 + mat[2][5] * det2_35_13;
	float det3_235_145 = mat[2][1] * det2_35_45 - mat[2][4] * det2_35_15 + mat[2][5] * det2_35_14;
	float det3_235_234 = mat[2][2] * det2_35_34 - mat[2][3] * det2_35_24 + mat[2][4] * det2_35_23;
	float det3_235_235 = mat[2][2] * det2_35_35 - mat[2][3] * det2_35_25 + mat[2][5] * det2_35_23;
	float det3_235_245 = mat[2][2] * det2_35_45 - mat[2][4] * det2_35_25 + mat[2][5] * det2_35_24;
	float det3_235_345 = mat[2][3] * det2_35_45 - mat[2][4] * det2_35_35 + mat[2][5] * det2_35_34;
	float det3_245_012 = mat[2][0] * det2_45_12 - mat[2][1] * det2_45_02 + mat[2][2] * det2_45_01;
	float det3_245_013 = mat[2][0] * det2_45_13 - mat[2][1] * det2_45_03 + mat[2][3] * det2_45_01;
	float det3_245_014 = mat[2][0] * det2_45_14 - mat[2][1] * det2_45_04 + mat[2][4] * det2_45_01;
	float det3_245_015 = mat[2][0] * det2_45_15 - mat[2][1] * det2_45_05 + mat[2][5] * det2_45_01;
	float det3_245_023 = mat[2][0] * det2_45_23 - mat[2][2] * det2_45_03 + mat[2][3] * det2_45_02;
	float det3_245_024 = mat[2][0] * det2_45_24 - mat[2][2] * det2_45_04 + mat[2][4] * det2_45_02;
	float det3_245_025 = mat[2][0] * det2_45_25 - mat[2][2] * det2_45_05 + mat[2][5] * det2_45_02;
	float det3_245_034 = mat[2][0] * det2_45_34 - mat[2][3] * det2_45_04 + mat[2][4] * det2_45_03;
	float det3_245_035 = mat[2][0] * det2_45_35 - mat[2][3] * det2_45_05 + mat[2][5] * det2_45_03;
	float det3_245_045 = mat[2][0] * det2_45_45 - mat[2][4] * det2_45_05 + mat[2][5] * det2_45_04;
	float det3_245_123 = mat[2][1] * det2_45_23 - mat[2][2] * det2_45_13 + mat[2][3] * det2_45_12;
	float det3_245_124 = mat[2][1] * det2_45_24 - mat[2][2] * det2_45_14 + mat[2][4] * det2_45_12;
	float det3_245_125 = mat[2][1] * det2_45_25 - mat[2][2] * det2_45_15 + mat[2][5] * det2_45_12;
	float det3_245_134 = mat[2][1] * det2_45_34 - mat[2][3] * det2_45_14 + mat[2][4] * det2_45_13;
	float det3_245_135 = mat[2][1] * det2_45_35 - mat[2][3] * det2_45_15 + mat[2][5] * det2_45_13;
	float det3_245_145 = mat[2][1] * det2_45_45 - mat[2][4] * det2_45_15 + mat[2][5] * det2_45_14;
	float det3_245_234 = mat[2][2] * det2_45_34 - mat[2][3] * det2_45_24 + mat[2][4] * det2_45_23;
	float det3_245_235 = mat[2][2] * det2_45_35 - mat[2][3] * det2_45_25 + mat[2][5] * det2_45_23;
	float det3_245_245 = mat[2][2] * det2_45_45 - mat[2][4] * det2_45_25 + mat[2][5] * det2_45_24;
	float det3_245_345 = mat[2][3] * det2_45_45 - mat[2][4] * det2_45_35 + mat[2][5] * det2_45_34;

	// remaining 4x4 sub-determinants
	float det4_1234_0123 = mat[1][0] * det3_234_123 - mat[1][1] * det3_234_023 + mat[1][2] * det3_234_013 - mat[1][3] * det3_234_012;
	float det4_1234_0124 = mat[1][0] * det3_234_124 - mat[1][1] * det3_234_024 + mat[1][2] * det3_234_014 - mat[1][4] * det3_234_012;
	float det4_1234_0125 = mat[1][0] * det3_234_125 - mat[1][1] * det3_234_025 + mat[1][2] * det3_234_015 - mat[1][5] * det3_234_012;
	float det4_1234_0134 = mat[1][0] * det3_234_134 - mat[1][1] * det3_234_034 + mat[1][3] * det3_234_014 - mat[1][4] * det3_234_013;
	float det4_1234_0135 = mat[1][0] * det3_234_135 - mat[1][1] * det3_234_035 + mat[1][3] * det3_234_015 - mat[1][5] * det3_234_013;
	float det4_1234_0145 = mat[1][0] * det3_234_145 - mat[1][1] * det3_234_045 + mat[1][4] * det3_234_015 - mat[1][5] * det3_234_014;
	float det4_1234_0234 = mat[1][0] * det3_234_234 - mat[1][2] * det3_234_034 + mat[1][3] * det3_234_024 - mat[1][4] * det3_234_023;
	float det4_1234_0235 = mat[1][0] * det3_234_235 - mat[1][2] * det3_234_035 + mat[1][3] * det3_234_025 - mat[1][5] * det3_234_023;
	float det4_1234_0245 = mat[1][0] * det3_234_245 - mat[1][2] * det3_234_045 + mat[1][4] * det3_234_025 - mat[1][5] * det3_234_024;
	float det4_1234_0345 = mat[1][0] * det3_234_345 - mat[1][3] * det3_234_045 + mat[1][4] * det3_234_035 - mat[1][5] * det3_234_034;
	float det4_1234_1234 = mat[1][1] * det3_234_234 - mat[1][2] * det3_234_134 + mat[1][3] * det3_234_124 - mat[1][4] * det3_234_123;
	float det4_1234_1235 = mat[1][1] * det3_234_235 - mat[1][2] * det3_234_135 + mat[1][3] * det3_234_125 - mat[1][5] * det3_234_123;
	float det4_1234_1245 = mat[1][1] * det3_234_245 - mat[1][2] * det3_234_145 + mat[1][4] * det3_234_125 - mat[1][5] * det3_234_124;
	float det4_1234_1345 = mat[1][1] * det3_234_345 - mat[1][3] * det3_234_145 + mat[1][4] * det3_234_135 - mat[1][5] * det3_234_134;
	float det4_1234_2345 = mat[1][2] * det3_234_345 - mat[1][3] * det3_234_245 + mat[1][4] * det3_234_235 - mat[1][5] * det3_234_234;
	float det4_1235_0123 = mat[1][0] * det3_235_123 - mat[1][1] * det3_235_023 + mat[1][2] * det3_235_013 - mat[1][3] * det3_235_012;
	float det4_1235_0124 = mat[1][0] * det3_235_124 - mat[1][1] * det3_235_024 + mat[1][2] * det3_235_014 - mat[1][4] * det3_235_012;
	float det4_1235_0125 = mat[1][0] * det3_235_125 - mat[1][1] * det3_235_025 + mat[1][2] * det3_235_015 - mat[1][5] * det3_235_012;
	float det4_1235_0134 = mat[1][0] * det3_235_134 - mat[1][1] * det3_235_034 + mat[1][3] * det3_235_014 - mat[1][4] * det3_235_013;
	float det4_1235_0135 = mat[1][0] * det3_235_135 - mat[1][1] * det3_235_035 + mat[1][3] * det3_235_015 - mat[1][5] * det3_235_013;
	float det4_1235_0145 = mat[1][0] * det3_235_145 - mat[1][1] * det3_235_045 + mat[1][4] * det3_235_015 - mat[1][5] * det3_235_014;
	float det4_1235_0234 = mat[1][0] * det3_235_234 - mat[1][2] * det3_235_034 + mat[1][3] * det3_235_024 - mat[1][4] * det3_235_023;
	float det4_1235_0235 = mat[1][0] * det3_235_235 - mat[1][2] * det3_235_035 + mat[1][3] * det3_235_025 - mat[1][5] * det3_235_023;
	float det4_1235_0245 = mat[1][0] * det3_235_245 - mat[1][2] * det3_235_045 + mat[1][4] * det3_235_025 - mat[1][5] * det3_235_024;
	float det4_1235_0345 = mat[1][0] * det3_235_345 - mat[1][3] * det3_235_045 + mat[1][4] * det3_235_035 - mat[1][5] * det3_235_034;
	float det4_1235_1234 = mat[1][1] * det3_235_234 - mat[1][2] * det3_235_134 + mat[1][3] * det3_235_124 - mat[1][4] * det3_235_123;
	float det4_1235_1235 = mat[1][1] * det3_235_235 - mat[1][2] * det3_235_135 + mat[1][3] * det3_235_125 - mat[1][5] * det3_235_123;
	float det4_1235_1245 = mat[1][1] * det3_235_245 - mat[1][2] * det3_235_145 + mat[1][4] * det3_235_125 - mat[1][5] * det3_235_124;
	float det4_1235_1345 = mat[1][1] * det3_235_345 - mat[1][3] * det3_235_145 + mat[1][4] * det3_235_135 - mat[1][5] * det3_235_134;
	float det4_1235_2345 = mat[1][2] * det3_235_345 - mat[1][3] * det3_235_245 + mat[1][4] * det3_235_235 - mat[1][5] * det3_235_234;
	float det4_1245_0123 = mat[1][0] * det3_245_123 - mat[1][1] * det3_245_023 + mat[1][2] * det3_245_013 - mat[1][3] * det3_245_012;
	float det4_1245_0124 = mat[1][0] * det3_245_124 - mat[1][1] * det3_245_024 + mat[1][2] * det3_245_014 - mat[1][4] * det3_245_012;
	float det4_1245_0125 = mat[1][0] * det3_245_125 - mat[1][1] * det3_245_025 + mat[1][2] * det3_245_015 - mat[1][5] * det3_245_012;
	float det4_1245_0134 = mat[1][0] * det3_245_134 - mat[1][1] * det3_245_034 + mat[1][3] * det3_245_014 - mat[1][4] * det3_245_013;
	float det4_1245_0135 = mat[1][0] * det3_245_135 - mat[1][1] * det3_245_035 + mat[1][3] * det3_245_015 - mat[1][5] * det3_245_013;
	float det4_1245_0145 = mat[1][0] * det3_245_145 - mat[1][1] * det3_245_045 + mat[1][4] * det3_245_015 - mat[1][5] * det3_245_014;
	float det4_1245_0234 = mat[1][0] * det3_245_234 - mat[1][2] * det3_245_034 + mat[1][3] * det3_245_024 - mat[1][4] * det3_245_023;
	float det4_1245_0235 = mat[1][0] * det3_245_235 - mat[1][2] * det3_245_035 + mat[1][3] * det3_245_025 - mat[1][5] * det3_245_023;
	float det4_1245_0245 = mat[1][0] * det3_245_245 - mat[1][2] * det3_245_045 + mat[1][4] * det3_245_025 - mat[1][5] * det3_245_024;
	float det4_1245_0345 = mat[1][0] * det3_245_345 - mat[1][3] * det3_245_045 + mat[1][4] * det3_245_035 - mat[1][5] * det3_245_034;
	float det4_1245_1234 = mat[1][1] * det3_245_234 - mat[1][2] * det3_245_134 + mat[1][3] * det3_245_124 - mat[1][4] * det3_245_123;
	float det4_1245_1235 = mat[1][1] * det3_245_235 - mat[1][2] * det3_245_135 + mat[1][3] * det3_245_125 - mat[1][5] * det3_245_123;
	float det4_1245_1245 = mat[1][1] * det3_245_245 - mat[1][2] * det3_245_145 + mat[1][4] * det3_245_125 - mat[1][5] * det3_245_124;
	float det4_1245_1345 = mat[1][1] * det3_245_345 - mat[1][3] * det3_245_145 + mat[1][4] * det3_245_135 - mat[1][5] * det3_245_134;
	float det4_1245_2345 = mat[1][2] * det3_245_345 - mat[1][3] * det3_245_245 + mat[1][4] * det3_245_235 - mat[1][5] * det3_245_234;
	float det4_1345_0123 = mat[1][0] * det3_345_123 - mat[1][1] * det3_345_023 + mat[1][2] * det3_345_013 - mat[1][3] * det3_345_012;
	float det4_1345_0124 = mat[1][0] * det3_345_124 - mat[1][1] * det3_345_024 + mat[1][2] * det3_345_014 - mat[1][4] * det3_345_012;
	float det4_1345_0125 = mat[1][0] * det3_345_125 - mat[1][1] * det3_345_025 + mat[1][2] * det3_345_015 - mat[1][5] * det3_345_012;
	float det4_1345_0134 = mat[1][0] * det3_345_134 - mat[1][1] * det3_345_034 + mat[1][3] * det3_345_014 - mat[1][4] * det3_345_013;
	float det4_1345_0135 = mat[1][0] * det3_345_135 - mat[1][1] * det3_345_035 + mat[1][3] * det3_345_015 - mat[1][5] * det3_345_013;
	float det4_1345_0145 = mat[1][0] * det3_345_145 - mat[1][1] * det3_345_045 + mat[1][4] * det3_345_015 - mat[1][5] * det3_345_014;
	float det4_1345_0234 = mat[1][0] * det3_345_234 - mat[1][2] * det3_345_034 + mat[1][3] * det3_345_024 - mat[1][4] * det3_345_023;
	float det4_1345_0235 = mat[1][0] * det3_345_235 - mat[1][2] * det3_345_035 + mat[1][3] * det3_345_025 - mat[1][5] * det3_345_023;
	float det4_1345_0245 = mat[1][0] * det3_345_245 - mat[1][2] * det3_345_045 + mat[1][4] * det3_345_025 - mat[1][5] * det3_345_024;
	float det4_1345_0345 = mat[1][0] * det3_345_345 - mat[1][3] * det3_345_045 + mat[1][4] * det3_345_035 - mat[1][5] * det3_345_034;
	float det4_1345_1234 = mat[1][1] * det3_345_234 - mat[1][2] * det3_345_134 + mat[1][3] * det3_345_124 - mat[1][4] * det3_345_123;
	float det4_1345_1235 = mat[1][1] * det3_345_235 - mat[1][2] * det3_345_135 + mat[1][3] * det3_345_125 - mat[1][5] * det3_345_123;
	float det4_1345_1245 = mat[1][1] * det3_345_245 - mat[1][2] * det3_345_145 + mat[1][4] * det3_345_125 - mat[1][5] * det3_345_124;
	float det4_1345_1345 = mat[1][1] * det3_345_345 - mat[1][3] * det3_345_145 + mat[1][4] * det3_345_135 - mat[1][5] * det3_345_134;
	float det4_1345_2345 = mat[1][2] * det3_345_345 - mat[1][3] * det3_345_245 + mat[1][4] * det3_345_235 - mat[1][5] * det3_345_234;

	// remaining 5x5 sub-determinants
	float det5_01234_01234 = mat[0][0] * det4_1234_1234 - mat[0][1] * det4_1234_0234 + mat[0][2] * det4_1234_0134 - mat[0][3] * det4_1234_0124 + mat[0][4] * det4_1234_0123;
	float det5_01234_01235 = mat[0][0] * det4_1234_1235 - mat[0][1] * det4_1234_0235 + mat[0][2] * det4_1234_0135 - mat[0][3] * det4_1234_0125 + mat[0][5] * det4_1234_0123;
	float det5_01234_01245 = mat[0][0] * det4_1234_1245 - mat[0][1] * det4_1234_0245 + mat[0][2] * det4_1234_0145 - mat[0][4] * det4_1234_0125 + mat[0][5] * det4_1234_0124;
	float det5_01234_01345 = mat[0][0] * det4_1234_1345 - mat[0][1] * det4_1234_0345 + mat[0][3] * det4_1234_0145 - mat[0][4] * det4_1234_0135 + mat[0][5] * det4_1234_0134;
	float det5_01234_02345 = mat[0][0] * det4_1234_2345 - mat[0][2] * det4_1234_0345 + mat[0][3] * det4_1234_0245 - mat[0][4] * det4_1234_0235 + mat[0][5] * det4_1234_0234;
	float det5_01234_12345 = mat[0][1] * det4_1234_2345 - mat[0][2] * det4_1234_1345 + mat[0][3] * det4_1234_1245 - mat[0][4] * det4_1234_1235 + mat[0][5] * det4_1234_1234;
	float det5_01235_01234 = mat[0][0] * det4_1235_1234 - mat[0][1] * det4_1235_0234 + mat[0][2] * det4_1235_0134 - mat[0][3] * det4_1235_0124 + mat[0][4] * det4_1235_0123;
	float det5_01235_01235 = mat[0][0] * det4_1235_1235 - mat[0][1] * det4_1235_0235 + mat[0][2] * det4_1235_0135 - mat[0][3] * det4_1235_0125 + mat[0][5] * det4_1235_0123;
	float det5_01235_01245 = mat[0][0] * det4_1235_1245 - mat[0][1] * det4_1235_0245 + mat[0][2] * det4_1235_0145 - mat[0][4] * det4_1235_0125 + mat[0][5] * det4_1235_0124;
	float det5_01235_01345 = mat[0][0] * det4_1235_1345 - mat[0][1] * det4_1235_0345 + mat[0][3] * det4_1235_0145 - mat[0][4] * det4_1235_0135 + mat[0][5] * det4_1235_0134;
	float det5_01235_02345 = mat[0][0] * det4_1235_2345 - mat[0][2] * det4_1235_0345 + mat[0][3] * det4_1235_0245 - mat[0][4] * det4_1235_0235 + mat[0][5] * det4_1235_0234;
	float det5_01235_12345 = mat[0][1] * det4_1235_2345 - mat[0][2] * det4_1235_1345 + mat[0][3] * det4_1235_1245 - mat[0][4] * det4_1235_1235 + mat[0][5] * det4_1235_1234;
	float det5_01245_01234 = mat[0][0] * det4_1245_1234 - mat[0][1] * det4_1245_0234 + mat[0][2] * det4_1245_0134 - mat[0][3] * det4_1245_0124 + mat[0][4] * det4_1245_0123;
	float det5_01245_01235 = mat[0][0] * det4_1245_1235 - mat[0][1] * det4_1245_0235 + mat[0][2] * det4_1245_0135 - mat[0][3] * det4_1245_0125 + mat[0][5] * det4_1245_0123;
	float det5_01245_01245 = mat[0][0] * det4_1245_1245 - mat[0][1] * det4_1245_0245 + mat[0][2] * det4_1245_0145 - mat[0][4] * det4_1245_0125 + mat[0][5] * det4_1245_0124;
	float det5_01245_01345 = mat[0][0] * det4_1245_1345 - mat[0][1] * det4_1245_0345 + mat[0][3] * det4_1245_0145 - mat[0][4] * det4_1245_0135 + mat[0][5] * det4_1245_0134;
	float det5_01245_02345 = mat[0][0] * det4_1245_2345 - mat[0][2] * det4_1245_0345 + mat[0][3] * det4_1245_0245 - mat[0][4] * det4_1245_0235 + mat[0][5] * det4_1245_0234;
	float det5_01245_12345 = mat[0][1] * det4_1245_2345 - mat[0][2] * det4_1245_1345 + mat[0][3] * det4_1245_1245 - mat[0][4] * det4_1245_1235 + mat[0][5] * det4_1245_1234;
	float det5_01345_01234 = mat[0][0] * det4_1345_1234 - mat[0][1] * det4_1345_0234 + mat[0][2] * det4_1345_0134 - mat[0][3] * det4_1345_0124 + mat[0][4] * det4_1345_0123;
	float det5_01345_01235 = mat[0][0] * det4_1345_1235 - mat[0][1] * det4_1345_0235 + mat[0][2] * det4_1345_0135 - mat[0][3] * det4_1345_0125 + mat[0][5] * det4_1345_0123;
	float det5_01345_01245 = mat[0][0] * det4_1345_1245 - mat[0][1] * det4_1345_0245 + mat[0][2] * det4_1345_0145 - mat[0][4] * det4_1345_0125 + mat[0][5] * det4_1345_0124;
	float det5_01345_01345 = mat[0][0] * det4_1345_1345 - mat[0][1] * det4_1345_0345 + mat[0][3] * det4_1345_0145 - mat[0][4] * det4_1345_0135 + mat[0][5] * det4_1345_0134;
	float det5_01345_02345 = mat[0][0] * det4_1345_2345 - mat[0][2] * det4_1345_0345 + mat[0][3] * det4_1345_0245 - mat[0][4] * det4_1345_0235 + mat[0][5] * det4_1345_0234;
	float det5_01345_12345 = mat[0][1] * det4_1345_2345 - mat[0][2] * det4_1345_1345 + mat[0][3] * det4_1345_1245 - mat[0][4] * det4_1345_1235 + mat[0][5] * det4_1345_1234;
	float det5_02345_01234 = mat[0][0] * det4_2345_1234 - mat[0][1] * det4_2345_0234 + mat[0][2] * det4_2345_0134 - mat[0][3] * det4_2345_0124 + mat[0][4] * det4_2345_0123;
	float det5_02345_01235 = mat[0][0] * det4_2345_1235 - mat[0][1] * det4_2345_0235 + mat[0][2] * det4_2345_0135 - mat[0][3] * det4_2345_0125 + mat[0][5] * det4_2345_0123;
	float det5_02345_01245 = mat[0][0] * det4_2345_1245 - mat[0][1] * det4_2345_0245 + mat[0][2] * det4_2345_0145 - mat[0][4] * det4_2345_0125 + mat[0][5] * det4_2345_0124;
	float det5_02345_01345 = mat[0][0] * det4_2345_1345 - mat[0][1] * det4_2345_0345 + mat[0][3] * det4_2345_0145 - mat[0][4] * det4_2345_0135 + mat[0][5] * det4_2345_0134;
	float det5_02345_02345 = mat[0][0] * det4_2345_2345 - mat[0][2] * det4_2345_0345 + mat[0][3] * det4_2345_0245 - mat[0][4] * det4_2345_0235 + mat[0][5] * det4_2345_0234;
	float det5_02345_12345 = mat[0][1] * det4_2345_2345 - mat[0][2] * det4_2345_1345 + mat[0][3] * det4_2345_1245 - mat[0][4] * det4_2345_1235 + mat[0][5] * det4_2345_1234;

	mat[0][0] =  det5_12345_12345 * invDet;
	mat[0][1] = -det5_02345_12345 * invDet;
	mat[0][2] =  det5_01345_12345 * invDet;
	mat[0][3] = -det5_01245_12345 * invDet;
	mat[0][4] =  det5_01235_12345 * invDet;
	mat[0][5] = -det5_01234_12345 * invDet;

	mat[1][0] = -det5_12345_02345 * invDet;
	mat[1][1] =  det5_02345_02345 * invDet;
	mat[1][2] = -det5_01345_02345 * invDet;
	mat[1][3] =  det5_01245_02345 * invDet;
	mat[1][4] = -det5_01235_02345 * invDet;
	mat[1][5] =  det5_01234_02345 * invDet;

	mat[2][0] =  det5_12345_01345 * invDet;
	mat[2][1] = -det5_02345_01345 * invDet;
	mat[2][2] =  det5_01345_01345 * invDet;
	mat[2][3] = -det5_01245_01345 * invDet;
	mat[2][4] =  det5_01235_01345 * invDet;
	mat[2][5] = -det5_01234_01345 * invDet;

	mat[3][0] = -det5_12345_01245 * invDet;
	mat[3][1] =  det5_02345_01245 * invDet;
	mat[3][2] = -det5_01345_01245 * invDet;
	mat[3][3] =  det5_01245_01245 * invDet;
	mat[3][4] = -det5_01235_01245 * invDet;
	mat[3][5] =  det5_01234_01245 * invDet;

	mat[4][0] =  det5_12345_01235 * invDet;
	mat[4][1] = -det5_02345_01235 * invDet;
	mat[4][2] =  det5_01345_01235 * invDet;
	mat[4][3] = -det5_01245_01235 * invDet;
	mat[4][4] =  det5_01235_01235 * invDet;
	mat[4][5] = -det5_01234_01235 * invDet;

	mat[5][0] = -det5_12345_01234 * invDet;
	mat[5][1] =  det5_02345_01234 * invDet;
	mat[5][2] = -det5_01345_01234 * invDet;
	mat[5][3] =  det5_01245_01234 * invDet;
	mat[5][4] = -det5_01235_01234 * invDet;
	mat[5][5] =  det5_01234_01234 * invDet;

	return true;
#elif 0
	// 6*40 = 240 multiplications
	//			6 divisions
	float *mat = reinterpret_cast<float *>( this );
	float s;
	double d, di;

	di = mat[0];
	s = di;
	mat[0] = d = 1.0f / di;
	mat[1] *= d;
	mat[2] *= d;
	mat[3] *= d;
	mat[4] *= d;
	mat[5] *= d;
	d = -d;
	mat[6] *= d;
	mat[12] *= d;
	mat[18] *= d;
	mat[24] *= d;
	mat[30] *= d;
	d = mat[6] * di;
	mat[7] += mat[1] * d;
	mat[8] += mat[2] * d;
	mat[9] += mat[3] * d;
	mat[10] += mat[4] * d;
	mat[11] += mat[5] * d;
	d = mat[12] * di;
	mat[13] += mat[1] * d;
	mat[14] += mat[2] * d;
	mat[15] += mat[3] * d;
	mat[16] += mat[4] * d;
	mat[17] += mat[5] * d;
	d = mat[18] * di;
	mat[19] += mat[1] * d;
	mat[20] += mat[2] * d;
	mat[21] += mat[3] * d;
	mat[22] += mat[4] * d;
	mat[23] += mat[5] * d;
	d = mat[24] * di;
	mat[25] += mat[1] * d;
	mat[26] += mat[2] * d;
	mat[27] += mat[3] * d;
	mat[28] += mat[4] * d;
	mat[29] += mat[5] * d;
	d = mat[30] * di;
	mat[31] += mat[1] * d;
	mat[32] += mat[2] * d;
	mat[33] += mat[3] * d;
	mat[34] += mat[4] * d;
	mat[35] += mat[5] * d;
	di = mat[7];
	s *= di;
	mat[7] = d = 1.0f / di;
	mat[6] *= d;
	mat[8] *= d;
	mat[9] *= d;
	mat[10] *= d;
	mat[11] *= d;
	d = -d;
	mat[1] *= d;
	mat[13] *= d;
	mat[19] *= d;
	mat[25] *= d;
	mat[31] *= d;
	d = mat[1] * di;
	mat[0] += mat[6] * d;
	mat[2] += mat[8] * d;
	mat[3] += mat[9] * d;
	mat[4] += mat[10] * d;
	mat[5] += mat[11] * d;
	d = mat[13] * di;
	mat[12] += mat[6] * d;
	mat[14] += mat[8] * d;
	mat[15] += mat[9] * d;
	mat[16] += mat[10] * d;
	mat[17] += mat[11] * d;
	d = mat[19] * di;
	mat[18] += mat[6] * d;
	mat[20] += mat[8] * d;
	mat[21] += mat[9] * d;
	mat[22] += mat[10] * d;
	mat[23] += mat[11] * d;
	d = mat[25] * di;
	mat[24] += mat[6] * d;
	mat[26] += mat[8] * d;
	mat[27] += mat[9] * d;
	mat[28] += mat[10] * d;
	mat[29] += mat[11] * d;
	d = mat[31] * di;
	mat[30] += mat[6] * d;
	mat[32] += mat[8] * d;
	mat[33] += mat[9] * d;
	mat[34] += mat[10] * d;
	mat[35] += mat[11] * d;
	di = mat[14];
	s *= di;
	mat[14] = d = 1.0f / di;
	mat[12] *= d;
	mat[13] *= d;
	mat[15] *= d;
	mat[16] *= d;
	mat[17] *= d;
	d = -d;
	mat[2] *= d;
	mat[8] *= d;
	mat[20] *= d;
	mat[26] *= d;
	mat[32] *= d;
	d = mat[2] * di;
	mat[0] += mat[12] * d;
	mat[1] += mat[13] * d;
	mat[3] += mat[15] * d;
	mat[4] += mat[16] * d;
	mat[5] += mat[17] * d;
	d = mat[8] * di;
	mat[6] += mat[12] * d;
	mat[7] += mat[13] * d;
	mat[9] += mat[15] * d;
	mat[10] += mat[16] * d;
	mat[11] += mat[17] * d;
	d = mat[20] * di;
	mat[18] += mat[12] * d;
	mat[19] += mat[13] * d;
	mat[21] += mat[15] * d;
	mat[22] += mat[16] * d;
	mat[23] += mat[17] * d;
	d = mat[26] * di;
	mat[24] += mat[12] * d;
	mat[25] += mat[13] * d;
	mat[27] += mat[15] * d;
	mat[28] += mat[16] * d;
	mat[29] += mat[17] * d;
	d = mat[32] * di;
	mat[30] += mat[12] * d;
	mat[31] += mat[13] * d;
	mat[33] += mat[15] * d;
	mat[34] += mat[16] * d;
	mat[35] += mat[17] * d;
	di = mat[21];
	s *= di;
	mat[21] = d = 1.0f / di;
	mat[18] *= d;
	mat[19] *= d;
	mat[20] *= d;
	mat[22] *= d;
	mat[23] *= d;
	d = -d;
	mat[3] *= d;
	mat[9] *= d;
	mat[15] *= d;
	mat[27] *= d;
	mat[33] *= d;
	d = mat[3] * di;
	mat[0] += mat[18] * d;
	mat[1] += mat[19] * d;
	mat[2] += mat[20] * d;
	mat[4] += mat[22] * d;
	mat[5] += mat[23] * d;
	d = mat[9] * di;
	mat[6] += mat[18] * d;
	mat[7] += mat[19] * d;
	mat[8] += mat[20] * d;
	mat[10] += mat[22] * d;
	mat[11] += mat[23] * d;
	d = mat[15] * di;
	mat[12] += mat[18] * d;
	mat[13] += mat[19] * d;
	mat[14] += mat[20] * d;
	mat[16] += mat[22] * d;
	mat[17] += mat[23] * d;
	d = mat[27] * di;
	mat[24] += mat[18] * d;
	mat[25] += mat[19] * d;
	mat[26] += mat[20] * d;
	mat[28] += mat[22] * d;
	mat[29] += mat[23] * d;
	d = mat[33] * di;
	mat[30] += mat[18] * d;
	mat[31] += mat[19] * d;
	mat[32] += mat[20] * d;
	mat[34] += mat[22] * d;
	mat[35] += mat[23] * d;
	di = mat[28];
	s *= di;
	mat[28] = d = 1.0f / di;
	mat[24] *= d;
	mat[25] *= d;
	mat[26] *= d;
	mat[27] *= d;
	mat[29] *= d;
	d = -d;
	mat[4] *= d;
	mat[10] *= d;
	mat[16] *= d;
	mat[22] *= d;
	mat[34] *= d;
	d = mat[4] * di;
	mat[0] += mat[24] * d;
	mat[1] += mat[25] * d;
	mat[2] += mat[26] * d;
	mat[3] += mat[27] * d;
	mat[5] += mat[29] * d;
	d = mat[10] * di;
	mat[6] += mat[24] * d;
	mat[7] += mat[25] * d;
	mat[8] += mat[26] * d;
	mat[9] += mat[27] * d;
	mat[11] += mat[29] * d;
	d = mat[16] * di;
	mat[12] += mat[24] * d;
	mat[13] += mat[25] * d;
	mat[14] += mat[26] * d;
	mat[15] += mat[27] * d;
	mat[17] += mat[29] * d;
	d = mat[22] * di;
	mat[18] += mat[24] * d;
	mat[19] += mat[25] * d;
	mat[20] += mat[26] * d;
	mat[21] += mat[27] * d;
	mat[23] += mat[29] * d;
	d = mat[34] * di;
	mat[30] += mat[24] * d;
	mat[31] += mat[25] * d;
	mat[32] += mat[26] * d;
	mat[33] += mat[27] * d;
	mat[35] += mat[29] * d;
	di = mat[35];
	s *= di;
	mat[35] = d = 1.0f / di;
	mat[30] *= d;
	mat[31] *= d;
	mat[32] *= d;
	mat[33] *= d;
	mat[34] *= d;
	d = -d;
	mat[5] *= d;
	mat[11] *= d;
	mat[17] *= d;
	mat[23] *= d;
	mat[29] *= d;
	d = mat[5] * di;
	mat[0] += mat[30] * d;
	mat[1] += mat[31] * d;
	mat[2] += mat[32] * d;
	mat[3] += mat[33] * d;
	mat[4] += mat[34] * d;
	d = mat[11] * di;
	mat[6] += mat[30] * d;
	mat[7] += mat[31] * d;
	mat[8] += mat[32] * d;
	mat[9] += mat[33] * d;
	mat[10] += mat[34] * d;
	d = mat[17] * di;
	mat[12] += mat[30] * d;
	mat[13] += mat[31] * d;
	mat[14] += mat[32] * d;
	mat[15] += mat[33] * d;
	mat[16] += mat[34] * d;
	d = mat[23] * di;
	mat[18] += mat[30] * d;
	mat[19] += mat[31] * d;
	mat[20] += mat[32] * d;
	mat[21] += mat[33] * d;
	mat[22] += mat[34] * d;
	d = mat[29] * di;
	mat[24] += mat[30] * d;
	mat[25] += mat[31] * d;
	mat[26] += mat[32] * d;
	mat[27] += mat[33] * d;
	mat[28] += mat[34] * d;

	return ( s != 0.0f && !FLOAT_IS_NAN( s ) );
#else
	// 6*27+2*30 = 222 multiplications
	//		2*1  =	 2 divisions
	arcMat3 r0, r1, r2, r3;
	float c0, c1, c2, det, invDet;
	float *mat = reinterpret_cast<float *>( this );

	// r0 = m0.Inverse();
	c0 = mat[1*6+1] * mat[2*6+2] - mat[1*6+2] * mat[2*6+1];
	c1 = mat[1*6+2] * mat[2*6+0] - mat[1*6+0] * mat[2*6+2];
	c2 = mat[1*6+0] * mat[2*6+1] - mat[1*6+1] * mat[2*6+0];

	det = mat[0*6+0] * c0 + mat[0*6+1] * c1 + mat[0*6+2] * c2;

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	r0[0][0] = c0 * invDet;
	r0[0][1] = ( mat[0*6+2] * mat[2*6+1] - mat[0*6+1] * mat[2*6+2] ) * invDet;
	r0[0][2] = ( mat[0*6+1] * mat[1*6+2] - mat[0*6+2] * mat[1*6+1] ) * invDet;
	r0[1][0] = c1 * invDet;
	r0[1][1] = ( mat[0*6+0] * mat[2*6+2] - mat[0*6+2] * mat[2*6+0] ) * invDet;
	r0[1][2] = ( mat[0*6+2] * mat[1*6+0] - mat[0*6+0] * mat[1*6+2] ) * invDet;
	r0[2][0] = c2 * invDet;
	r0[2][1] = ( mat[0*6+1] * mat[2*6+0] - mat[0*6+0] * mat[2*6+1] ) * invDet;
	r0[2][2] = ( mat[0*6+0] * mat[1*6+1] - mat[0*6+1] * mat[1*6+0] ) * invDet;

	// r1 = r0 * m1;
	r1[0][0] = r0[0][0] * mat[0*6+3] + r0[0][1] * mat[1*6+3] + r0[0][2] * mat[2*6+3];
	r1[0][1] = r0[0][0] * mat[0*6+4] + r0[0][1] * mat[1*6+4] + r0[0][2] * mat[2*6+4];
	r1[0][2] = r0[0][0] * mat[0*6+5] + r0[0][1] * mat[1*6+5] + r0[0][2] * mat[2*6+5];
	r1[1][0] = r0[1][0] * mat[0*6+3] + r0[1][1] * mat[1*6+3] + r0[1][2] * mat[2*6+3];
	r1[1][1] = r0[1][0] * mat[0*6+4] + r0[1][1] * mat[1*6+4] + r0[1][2] * mat[2*6+4];
	r1[1][2] = r0[1][0] * mat[0*6+5] + r0[1][1] * mat[1*6+5] + r0[1][2] * mat[2*6+5];
	r1[2][0] = r0[2][0] * mat[0*6+3] + r0[2][1] * mat[1*6+3] + r0[2][2] * mat[2*6+3];
	r1[2][1] = r0[2][0] * mat[0*6+4] + r0[2][1] * mat[1*6+4] + r0[2][2] * mat[2*6+4];
	r1[2][2] = r0[2][0] * mat[0*6+5] + r0[2][1] * mat[1*6+5] + r0[2][2] * mat[2*6+5];

	// r2 = m2 * r1;
	r2[0][0] = mat[3*6+0] * r1[0][0] + mat[3*6+1] * r1[1][0] + mat[3*6+2] * r1[2][0];
	r2[0][1] = mat[3*6+0] * r1[0][1] + mat[3*6+1] * r1[1][1] + mat[3*6+2] * r1[2][1];
	r2[0][2] = mat[3*6+0] * r1[0][2] + mat[3*6+1] * r1[1][2] + mat[3*6+2] * r1[2][2];
	r2[1][0] = mat[4*6+0] * r1[0][0] + mat[4*6+1] * r1[1][0] + mat[4*6+2] * r1[2][0];
	r2[1][1] = mat[4*6+0] * r1[0][1] + mat[4*6+1] * r1[1][1] + mat[4*6+2] * r1[2][1];
	r2[1][2] = mat[4*6+0] * r1[0][2] + mat[4*6+1] * r1[1][2] + mat[4*6+2] * r1[2][2];
	r2[2][0] = mat[5*6+0] * r1[0][0] + mat[5*6+1] * r1[1][0] + mat[5*6+2] * r1[2][0];
	r2[2][1] = mat[5*6+0] * r1[0][1] + mat[5*6+1] * r1[1][1] + mat[5*6+2] * r1[2][1];
	r2[2][2] = mat[5*6+0] * r1[0][2] + mat[5*6+1] * r1[1][2] + mat[5*6+2] * r1[2][2];

	// r3 = r2 - m3;
	r3[0][0] = r2[0][0] - mat[3*6+3];
	r3[0][1] = r2[0][1] - mat[3*6+4];
	r3[0][2] = r2[0][2] - mat[3*6+5];
	r3[1][0] = r2[1][0] - mat[4*6+3];
	r3[1][1] = r2[1][1] - mat[4*6+4];
	r3[1][2] = r2[1][2] - mat[4*6+5];
	r3[2][0] = r2[2][0] - mat[5*6+3];
	r3[2][1] = r2[2][1] - mat[5*6+4];
	r3[2][2] = r2[2][2] - mat[5*6+5];

	// r3.InverseSelf();
	r2[0][0] = r3[1][1] * r3[2][2] - r3[1][2] * r3[2][1];
	r2[1][0] = r3[1][2] * r3[2][0] - r3[1][0] * r3[2][2];
	r2[2][0] = r3[1][0] * r3[2][1] - r3[1][1] * r3[2][0];

	det = r3[0][0] * r2[0][0] + r3[0][1] * r2[1][0] + r3[0][2] * r2[2][0];

	if ( arcMath::Fabs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	r2[0][1] = r3[0][2] * r3[2][1] - r3[0][1] * r3[2][2];
	r2[0][2] = r3[0][1] * r3[1][2] - r3[0][2] * r3[1][1];
	r2[1][1] = r3[0][0] * r3[2][2] - r3[0][2] * r3[2][0];
	r2[1][2] = r3[0][2] * r3[1][0] - r3[0][0] * r3[1][2];
	r2[2][1] = r3[0][1] * r3[2][0] - r3[0][0] * r3[2][1];
	r2[2][2] = r3[0][0] * r3[1][1] - r3[0][1] * r3[1][0];

	r3[0][0] = r2[0][0] * invDet;
	r3[0][1] = r2[0][1] * invDet;
	r3[0][2] = r2[0][2] * invDet;
	r3[1][0] = r2[1][0] * invDet;
	r3[1][1] = r2[1][1] * invDet;
	r3[1][2] = r2[1][2] * invDet;
	r3[2][0] = r2[2][0] * invDet;
	r3[2][1] = r2[2][1] * invDet;
	r3[2][2] = r2[2][2] * invDet;

	// r2 = m2 * r0;
	r2[0][0] = mat[3*6+0] * r0[0][0] + mat[3*6+1] * r0[1][0] + mat[3*6+2] * r0[2][0];
	r2[0][1] = mat[3*6+0] * r0[0][1] + mat[3*6+1] * r0[1][1] + mat[3*6+2] * r0[2][1];
	r2[0][2] = mat[3*6+0] * r0[0][2] + mat[3*6+1] * r0[1][2] + mat[3*6+2] * r0[2][2];
	r2[1][0] = mat[4*6+0] * r0[0][0] + mat[4*6+1] * r0[1][0] + mat[4*6+2] * r0[2][0];
	r2[1][1] = mat[4*6+0] * r0[0][1] + mat[4*6+1] * r0[1][1] + mat[4*6+2] * r0[2][1];
	r2[1][2] = mat[4*6+0] * r0[0][2] + mat[4*6+1] * r0[1][2] + mat[4*6+2] * r0[2][2];
	r2[2][0] = mat[5*6+0] * r0[0][0] + mat[5*6+1] * r0[1][0] + mat[5*6+2] * r0[2][0];
	r2[2][1] = mat[5*6+0] * r0[0][1] + mat[5*6+1] * r0[1][1] + mat[5*6+2] * r0[2][1];
	r2[2][2] = mat[5*6+0] * r0[0][2] + mat[5*6+1] * r0[1][2] + mat[5*6+2] * r0[2][2];

	// m2 = r3 * r2;
	mat[3*6+0] = r3[0][0] * r2[0][0] + r3[0][1] * r2[1][0] + r3[0][2] * r2[2][0];
	mat[3*6+1] = r3[0][0] * r2[0][1] + r3[0][1] * r2[1][1] + r3[0][2] * r2[2][1];
	mat[3*6+2] = r3[0][0] * r2[0][2] + r3[0][1] * r2[1][2] + r3[0][2] * r2[2][2];
	mat[4*6+0] = r3[1][0] * r2[0][0] + r3[1][1] * r2[1][0] + r3[1][2] * r2[2][0];
	mat[4*6+1] = r3[1][0] * r2[0][1] + r3[1][1] * r2[1][1] + r3[1][2] * r2[2][1];
	mat[4*6+2] = r3[1][0] * r2[0][2] + r3[1][1] * r2[1][2] + r3[1][2] * r2[2][2];
	mat[5*6+0] = r3[2][0] * r2[0][0] + r3[2][1] * r2[1][0] + r3[2][2] * r2[2][0];
	mat[5*6+1] = r3[2][0] * r2[0][1] + r3[2][1] * r2[1][1] + r3[2][2] * r2[2][1];
	mat[5*6+2] = r3[2][0] * r2[0][2] + r3[2][1] * r2[1][2] + r3[2][2] * r2[2][2];

	// m0 = r0 - r1 * m2;
	mat[0*6+0] = r0[0][0] - r1[0][0] * mat[3*6+0] - r1[0][1] * mat[4*6+0] - r1[0][2] * mat[5*6+0];
	mat[0*6+1] = r0[0][1] - r1[0][0] * mat[3*6+1] - r1[0][1] * mat[4*6+1] - r1[0][2] * mat[5*6+1];
	mat[0*6+2] = r0[0][2] - r1[0][0] * mat[3*6+2] - r1[0][1] * mat[4*6+2] - r1[0][2] * mat[5*6+2];
	mat[1*6+0] = r0[1][0] - r1[1][0] * mat[3*6+0] - r1[1][1] * mat[4*6+0] - r1[1][2] * mat[5*6+0];
	mat[1*6+1] = r0[1][1] - r1[1][0] * mat[3*6+1] - r1[1][1] * mat[4*6+1] - r1[1][2] * mat[5*6+1];
	mat[1*6+2] = r0[1][2] - r1[1][0] * mat[3*6+2] - r1[1][1] * mat[4*6+2] - r1[1][2] * mat[5*6+2];
	mat[2*6+0] = r0[2][0] - r1[2][0] * mat[3*6+0] - r1[2][1] * mat[4*6+0] - r1[2][2] * mat[5*6+0];
	mat[2*6+1] = r0[2][1] - r1[2][0] * mat[3*6+1] - r1[2][1] * mat[4*6+1] - r1[2][2] * mat[5*6+1];
	mat[2*6+2] = r0[2][2] - r1[2][0] * mat[3*6+2] - r1[2][1] * mat[4*6+2] - r1[2][2] * mat[5*6+2];

	// m1 = r1 * r3;
	mat[0*6+3] = r1[0][0] * r3[0][0] + r1[0][1] * r3[1][0] + r1[0][2] * r3[2][0];
	mat[0*6+4] = r1[0][0] * r3[0][1] + r1[0][1] * r3[1][1] + r1[0][2] * r3[2][1];
	mat[0*6+5] = r1[0][0] * r3[0][2] + r1[0][1] * r3[1][2] + r1[0][2] * r3[2][2];
	mat[1*6+3] = r1[1][0] * r3[0][0] + r1[1][1] * r3[1][0] + r1[1][2] * r3[2][0];
	mat[1*6+4] = r1[1][0] * r3[0][1] + r1[1][1] * r3[1][1] + r1[1][2] * r3[2][1];
	mat[1*6+5] = r1[1][0] * r3[0][2] + r1[1][1] * r3[1][2] + r1[1][2] * r3[2][2];
	mat[2*6+3] = r1[2][0] * r3[0][0] + r1[2][1] * r3[1][0] + r1[2][2] * r3[2][0];
	mat[2*6+4] = r1[2][0] * r3[0][1] + r1[2][1] * r3[1][1] + r1[2][2] * r3[2][1];
	mat[2*6+5] = r1[2][0] * r3[0][2] + r1[2][1] * r3[1][2] + r1[2][2] * r3[2][2];

	// m3 = -r3;
	mat[3*6+3] = -r3[0][0];
	mat[3*6+4] = -r3[0][1];
	mat[3*6+5] = -r3[0][2];
	mat[4*6+3] = -r3[1][0];
	mat[4*6+4] = -r3[1][1];
	mat[4*6+5] = -r3[1][2];
	mat[5*6+3] = -r3[2][0];
	mat[5*6+4] = -r3[2][1];
	mat[5*6+5] = -r3[2][2];

	return true;
#endif
}

/*
=============
arcMat6::ToString
=============
*/
const char *arcMat6::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}


//===============================================================
//
//  arcMatX
//
//===============================================================

float	arcMatX::temp[MATX_MAX_TEMP+4];
float *	arcMatX::tempPtr = (float *) ( ( ( int ) arcMatX::temp + 15 ) & ~15 );
int		arcMatX::tempIndex = 0;


/*
============
arcMatX::ChangeSize
============
*/
void arcMatX::ChangeSize( int rows, int columns, bool makeZero ) {
	int alloc = ( rows * columns + 3 ) & ~3;
	if ( alloc > alloced && alloced != -1 ) {
		float *oldMat = mat;
		mat = (float *) Mem_Alloc16( alloc * sizeof( float ) );
		if ( makeZero ) {
			memset( mat, 0, alloc * sizeof( float ) );
		}
		alloced = alloc;
		if ( oldMat ) {
			int minRow = Min( numRows, rows );
			int minColumn = Min( numColumns, columns );
			for ( int i = 0; i < minRow; i++ ) {
				for ( int j = 0; j < minColumn; j++ ) {
					mat[ i * columns + j ] = oldMat[ i * numColumns + j ];
				}
			}
			Mem_Free16( oldMat );
		}
	} else {
		if ( columns < numColumns ) {
			int minRow = Min( numRows, rows );
			for ( int i = 0; i < minRow; i++ ) {
				for ( int j = 0; j < columns; j++ ) {
					mat[ i * columns + j ] = mat[ i * numColumns + j ];
				}
			}
		} else if ( columns > numColumns ) {
			for ( int i = Min( numRows, rows ) - 1; i >= 0; i-- ) {
				if ( makeZero ) {
					for ( int j = columns - 1; j >= numColumns; j-- ) {
						mat[ i * columns + j ] = 0.0f;
					}
				}
				for ( int j = numColumns - 1; j >= 0; j-- ) {
					mat[ i * columns + j ] = mat[ i * numColumns + j ];
				}
			}
		}
		if ( makeZero && rows > numRows ) {
			memset( mat + numRows * columns, 0, ( rows - numRows ) * columns * sizeof( float ) );
		}
	}
	numRows = rows;
	numColumns = columns;
	MATX_CLEAREND();
}

/*
============
arcMatX::RemoveRow
============
*/
arcMatX &arcMatX::RemoveRow( int r ) {
	int i;

	assert( r < numRows );

	numRows--;

	for ( i = r; i < numRows; i++ ) {
		memcpy( &mat[i * numColumns], &mat[( i + 1 ) * numColumns], numColumns * sizeof( float ) );
	}

	return *this;
}

/*
============
arcMatX::RemoveColumn
============
*/
arcMatX &arcMatX::RemoveColumn( int r ) {
	int i;

	assert( r < numColumns );

	numColumns--;

	for ( i = 0; i < numRows - 1; i++ ) {
		memmove( &mat[i * numColumns + r], &mat[i * ( numColumns + 1 ) + r + 1], numColumns * sizeof( float ) );
	}
	memmove( &mat[i * numColumns + r], &mat[i * ( numColumns + 1 ) + r + 1], ( numColumns - r ) * sizeof( float ) );

	return *this;
}

/*
============
arcMatX::RemoveRowColumn
============
*/
arcMatX &arcMatX::RemoveRowColumn( int r ) {
	int i;

	assert( r < numRows && r < numColumns );

	numRows--;
	numColumns--;

	if ( r > 0 ) {
		for ( i = 0; i < r - 1; i++ ) {
			memmove( &mat[i * numColumns + r], &mat[i * ( numColumns + 1 ) + r + 1], numColumns * sizeof( float ) );
		}
		memmove( &mat[i * numColumns + r], &mat[i * ( numColumns + 1 ) + r + 1], ( numColumns - r ) * sizeof( float ) );
	}

	memcpy( &mat[r * numColumns], &mat[( r + 1 ) * ( numColumns + 1 )], r * sizeof( float ) );

	for ( i = r; i < numRows - 1; i++ ) {
		memcpy( &mat[i * numColumns + r], &mat[( i + 1 ) * ( numColumns + 1 ) + r + 1], numColumns * sizeof( float ) );
	}
	memcpy( &mat[i * numColumns + r], &mat[( i + 1 ) * ( numColumns + 1 ) + r + 1], ( numColumns - r ) * sizeof( float ) );

	return *this;
}

/*
============
arcMatX::IsOrthogonal

  returns true if (*this) * this->Transpose() == Identity
============
*/
bool arcMatX::IsOrthogonal( const float epsilon ) const {
	float *ptr1, *ptr2, sum;

	if ( !IsSquare() ) {
		return false;
	}

	ptr1 = mat;
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			ptr2 = mat + j;
			sum = ptr1[0] * ptr2[0] - ( float ) ( i == j );
			for ( int n = 1; n < numColumns; n++ ) {
				ptr2 += numColumns;
				sum += ptr1[n] * ptr2[0];
			}
			if ( arcMath::Fabs( sum ) > epsilon ) {
				return false;
			}
		}
		ptr1 += numColumns;
	}
	return true;
}

/*
============
arcMatX::IsOrthonormal

  returns true if (*this) * this->Transpose() == Identity and the length of each column vector is 1
============
*/
bool arcMatX::IsOrthonormal( const float epsilon ) const {
	float *ptr1, *ptr2, sum;

	if ( !IsSquare() ) {
		return false;
	}

	ptr1 = mat;
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			ptr2 = mat + j;
			sum = ptr1[0] * ptr2[0] - ( float ) ( i == j );
			for ( int n = 1; n < numColumns; n++ ) {
				ptr2 += numColumns;
				sum += ptr1[n] * ptr2[0];
			}
			if ( arcMath::Fabs( sum ) > epsilon ) {
				return false;
			}
		}
		ptr1 += numColumns;

		ptr2 = mat + i;
		sum = ptr2[0] * ptr2[0] - 1.0f;
		for ( i = 1; i < numRows; i++ ) {
			ptr2 += numColumns;
			sum += ptr2[i] * ptr2[i];
		}
		if ( arcMath::Fabs( sum ) > epsilon ) {
			return false;
		}
	}
	return true;
}

/*
============
arcMatX::IsPMatrix

  returns true if the matrix is a P-matrix
  A square matrix is a P-matrix if all its principal minors are positive.
============
*/
bool arcMatX::IsPMatrix( const float epsilon ) const {
	int i, j;
	float d;
	arcMatX m;

	if ( !IsSquare() ) {
		return false;
	}

	if ( numRows <= 0 ) {
		return true;
	}

	if ( (*this)[0][0] <= epsilon ) {
		return false;
	}

	if ( numRows <= 1 ) {
		return true;
	}

	m.SetData( numRows - 1, numColumns - 1, MATX_ALLOCA( ( numRows - 1 ) * ( numColumns - 1 ) ) );

	for ( i = 1; i < numRows; i++ ) {
		for ( j = 1; j < numColumns; j++ ) {
			m[i-1][j-1] = (*this)[i][j];
		}
	}

	if ( !m.IsPMatrix( epsilon ) ) {
		return false;
	}

	for ( i = 1; i < numRows; i++ ) {
		d = (*this)[i][0] / (*this)[0][0];
		for ( j = 1; j < numColumns; j++ ) {
			m[i-1][j-1] = (*this)[i][j] - d * (*this)[0][j];
		}
	}

	if ( !m.IsPMatrix( epsilon ) ) {
		return false;
	}

	return true;
}

/*
============
arcMatX::IsZMatrix

  returns true if the matrix is a Z-matrix
  A square matrix M is a Z-matrix if M[i][j] <= 0 for all i != j.
============
*/
bool arcMatX::IsZMatrix( const float epsilon ) const {
	int i, j;

	if ( !IsSquare() ) {
		return false;
	}

	for ( i = 0; i < numRows; i++ ) {
		for ( j = 0; j < numColumns; j++ ) {
			if ( (*this)[i][j] > epsilon && i != j ) {
				return false;
			}
		}
	}
	return true;
}

/*
============
arcMatX::IsPositiveDefinite

  returns true if the matrix is Positive Definite (PD)
  A square matrix M of order n is said to be PD if y'My > 0 for all vectors y of dimension n, y != 0.
============
*/
bool arcMatX::IsPositiveDefinite( const float epsilon ) const {
	int i, j, k;
	float d, s;
	arcMatX m;

	// the matrix must be square
	if ( !IsSquare() ) {
		return false;
	}

	// copy matrix
	m.SetData( numRows, numColumns, MATX_ALLOCA( numRows * numColumns ) );
	m = *this;

	// add transpose
	for ( i = 0; i < numRows; i++ ) {
		for ( j = 0; j < numColumns; j++ ) {
			m[i][j] += (*this)[j][i];
		}
	}

	// test Positive Definiteness with Gaussian pivot steps
	for ( i = 0; i < numRows; i++ ) {

		for ( j = i; j < numColumns; j++ ) {
			if ( m[j][j] <= epsilon ) {
				return false;
			}
		}

		d = 1.0f / m[i][i];
		for ( j = i + 1; j < numColumns; j++ ) {
			s = d * m[j][i];
			m[j][i] = 0.0f;
			for ( k = i + 1; k < numRows; k++ ) {
				m[j][k] -= s * m[i][k];
			}
		}
	}

	return true;
}

/*
============
arcMatX::IsSymmetricPositiveDefinite

  returns true if the matrix is Symmetric Positive Definite (PD)
============
*/
bool arcMatX::IsSymmetricPositiveDefinite( const float epsilon ) const {
	arcMatX m;

	// the matrix must be symmetric
	if ( !IsSymmetric( epsilon ) ) {
		return false;
	}

	// copy matrix
	m.SetData( numRows, numColumns, MATX_ALLOCA( numRows * numColumns ) );
	m = *this;

	// being able to obtain Cholesky factors is both a necessary and sufficient condition for positive definiteness
	return m.Cholesky_Factor();
}

/*
============
arcMatX::IsPositiveSemiDefinite

  returns true if the matrix is Positive Semi Definite (PSD)
  A square matrix M of order n is said to be PSD if y'My >= 0 for all vectors y of dimension n, y != 0.
============
*/
bool arcMatX::IsPositiveSemiDefinite( const float epsilon ) const {
	int i, j, k;
	float d, s;
	arcMatX m;

	// the matrix must be square
	if ( !IsSquare() ) {
		return false;
	}

	// copy original matrix
	m.SetData( numRows, numColumns, MATX_ALLOCA( numRows * numColumns ) );
	m = *this;

	// add transpose
	for ( i = 0; i < numRows; i++ ) {
		for ( j = 0; j < numColumns; j++ ) {
			m[i][j] += (*this)[j][i];
		}
	}

	// test Positive Semi Definiteness with Gaussian pivot steps
	for ( i = 0; i < numRows; i++ ) {

		for ( j = i; j < numColumns; j++ ) {
			if ( m[j][j] < -epsilon ) {
				return false;
			}
			if ( m[j][j] > epsilon ) {
				continue;
			}
			for ( k = 0; k < numRows; k++ ) {
				if ( arcMath::Fabs( m[k][j] ) > epsilon ) {
					return false;
				}
				if ( arcMath::Fabs( m[j][k] ) > epsilon ) {
					return false;
				}
			}
		}

		if ( m[i][i] <= epsilon ) {
			continue;
		}

		d = 1.0f / m[i][i];
		for ( j = i + 1; j < numColumns; j++ ) {
			s = d * m[j][i];
			m[j][i] = 0.0f;
			for ( k = i + 1; k < numRows; k++ ) {
				m[j][k] -= s * m[i][k];
			}
		}
	}

	return true;
}

/*
============
arcMatX::IsSymmetricPositiveSemiDefinite

  returns true if the matrix is Symmetric Positive Semi Definite (PSD)
============
*/
bool arcMatX::IsSymmetricPositiveSemiDefinite( const float epsilon ) const {

	// the matrix must be symmetric
	if ( !IsSymmetric( epsilon ) ) {
		return false;
	}

	return IsPositiveSemiDefinite( epsilon );
}

/*
============
arcMatX::LowerTriangularInverse

  in-place inversion of the lower triangular matrix
============
*/
bool arcMatX::LowerTriangularInverse( void ) {
	int i, j, k;
	double d, sum;

	for ( i = 0; i < numRows; i++ ) {
		d = (*this)[i][i];
		if ( d == 0.0f ) {
			return false;
		}
		(*this)[i][i] = d = 1.0f / d;

		for ( j = 0; j < i; j++ ) {
			sum = 0.0f;
			for ( k = j; k < i; k++ ) {
				sum -= (*this)[i][k] * (*this)[k][j];
			}
			(*this)[i][j] = sum * d;
		}
	}
	return true;
}

/*
============
arcMatX::UpperTriangularInverse

  in-place inversion of the upper triangular matrix
============
*/
bool arcMatX::UpperTriangularInverse( void ) {
	int i, j, k;
	double d, sum;

	for ( i = numRows-1; i >= 0; i-- ) {
		d = (*this)[i][i];
		if ( d == 0.0f ) {
			return false;
		}
		(*this)[i][i] = d = 1.0f / d;

		for ( j = numRows-1; j > i; j-- ) {
			sum = 0.0f;
			for ( k = j; k > i; k-- ) {
				sum -= (*this)[i][k] * (*this)[k][j];
			}
			(*this)[i][j] = sum * d;
		}
	}
	return true;
}

/*
=============
arcMatX::ToString
=============
*/
const char *arcMatX::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
============
arcMatX::Update_RankOne

  Updates the matrix to obtain the matrix: A + alpha * v * w'
============
*/
void arcMatX::Update_RankOne( const arcVecX &v, const arcVecX &w, float alpha ) {
	int i, j;
	float s;

	assert( v.GetSize() >= numRows );
	assert( w.GetSize() >= numColumns );

	for ( i = 0; i < numRows; i++ ) {
		s = alpha * v[i];
		for ( j = 0; j < numColumns; j++ ) {
			(*this)[i][j] += s * w[j];
		}
	}
}

/*
============
arcMatX::Update_RankOneSymmetric

  Updates the matrix to obtain the matrix: A + alpha * v * v'
============
*/
void arcMatX::Update_RankOneSymmetric( const arcVecX &v, float alpha ) {
	int i, j;
	float s;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );

	for ( i = 0; i < numRows; i++ ) {
		s = alpha * v[i];
		for ( j = 0; j < numColumns; j++ ) {
			(*this)[i][j] += s * v[j];
		}
	}
}

/*
============
arcMatX::Update_RowColumn

  Updates the matrix to obtain the matrix:

      [ 0  a  0 ]
  A + [ d  b  e ]
      [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1], d = w[0,r-1], w[r] = 0.0f, e = w[r+1,numColumns-1]
============
*/
void arcMatX::Update_RowColumn( const arcVecX &v, const arcVecX &w, int r ) {
	int i;

	assert( w[r] == 0.0f );
	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );

	for ( i = 0; i < numRows; i++ ) {
		(*this)[i][r] += v[i];
	}
	for ( i = 0; i < numColumns; i++ ) {
		(*this)[r][i] += w[i];
	}
}

/*
============
arcMatX::Update_RowColumnSymmetric

  Updates the matrix to obtain the matrix:

      [ 0  a  0 ]
  A + [ a  b  c ]
      [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1]
============
*/
void arcMatX::Update_RowColumnSymmetric( const arcVecX &v, int r ) {
	int i;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );

	for ( i = 0; i < r; i++ ) {
		(*this)[i][r] += v[i];
		(*this)[r][i] += v[i];
	}
	(*this)[r][r] += v[r];
	for ( i = r+1; i < numRows; i++ ) {
		(*this)[i][r] += v[i];
		(*this)[r][i] += v[i];
	}
}

/*
============
arcMatX::Update_Increment

  Updates the matrix to obtain the matrix:

  [ A  a ]
  [ c  b ]

  where: a = v[0,numRows-1], b = v[numRows], c = w[0,numColumns-1]], w[numColumns] = 0
============
*/
void arcMatX::Update_Increment( const arcVecX &v, const arcVecX &w ) {
	int i;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );
	assert( w.GetSize() >= numColumns+1 );

	ChangeSize( numRows+1, numColumns+1, false );

	for ( i = 0; i < numRows; i++ ) {
		(*this)[i][numColumns-1] = v[i];
	}
	for ( i = 0; i < numColumns-1; i++ ) {
		(*this)[numRows-1][i] = w[i];
	}
}

/*
============
arcMatX::Update_IncrementSymmetric

  Updates the matrix to obtain the matrix:

  [ A  a ]
  [ a  b ]

  where: a = v[0,numRows-1], b = v[numRows]
============
*/
void arcMatX::Update_IncrementSymmetric( const arcVecX &v ) {
	int i;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );

	ChangeSize( numRows+1, numColumns+1, false );

	for ( i = 0; i < numRows-1; i++ ) {
		(*this)[i][numColumns-1] = v[i];
	}
	for ( i = 0; i < numColumns; i++ ) {
		(*this)[numRows-1][i] = v[i];
	}
}

/*
============
arcMatX::Update_Decrement

  Updates the matrix to obtain a matrix with row r and column r removed.
============
*/
void arcMatX::Update_Decrement( int r ) {
	RemoveRowColumn( r );
}

/*
============
arcMatX::Inverse_GaussJordan

  in-place inversion using Gauss-Jordan elimination
============
*/
bool arcMatX::Inverse_GaussJordan( void ) {
	int i, j, k, r, c;
	float d, max;

	assert( numRows == numColumns );

	int *columnIndex = ( int * ) _alloca16( numRows * sizeof( int ) );
	int *rowIndex = ( int * ) _alloca16( numRows * sizeof( int ) );
	bool *pivot = (bool *) _alloca16( numRows * sizeof( bool ) );

	memset( pivot, 0, numRows * sizeof( bool ) );

	// elimination with full pivoting
	for ( i = 0; i < numRows; i++ ) {

		// search the whole matrix except for pivoted rows for the maximum absolute value
		max = 0.0f;
		r = c = 0;
		for ( j = 0; j < numRows; j++ ) {
			if ( !pivot[j] ) {
				for ( k = 0; k < numRows; k++ ) {
					if ( !pivot[k] ) {
						d = arcMath::Fabs( (*this)[j][k] );
						if ( d > max ) {
							max = d;
							r = j;
							c = k;
						}
					}
				}
			}
		}

		if ( max == 0.0f ) {
			// matrix is not invertible
			return false;
		}

		pivot[c] = true;

		// swap rows such that entry (c,c) has the pivot entry
		if ( r != c ) {
			SwapRows( r, c );
		}

		// keep track of the row permutation
		rowIndex[i] = r;
		columnIndex[i] = c;

		// scale the row to make the pivot entry equal to 1
		d = 1.0f / (*this)[c][c];
		(*this)[c][c] = 1.0f;
		for ( k = 0; k < numRows; k++ ) {
			(*this)[c][k] *= d;
		}

		// zero out the pivot column entries in the other rows
		for ( j = 0; j < numRows; j++ ) {
			if ( j != c ) {
				d = (*this)[j][c];
				(*this)[j][c] = 0.0f;
				for ( k = 0; k < numRows; k++ ) {
					(*this)[j][k] -= (*this)[c][k] * d;
				}
			}
		}
	}

	// reorder rows to store the inverse of the original matrix
	for ( j = numRows - 1; j >= 0; j-- ) {
		if ( rowIndex[j] != columnIndex[j] ) {
			for ( k = 0; k < numRows; k++ ) {
				d = (*this)[k][rowIndex[j]];
				(*this)[k][rowIndex[j]] = (*this)[k][columnIndex[j]];
				(*this)[k][columnIndex[j]] = d;
			}
		}
	}

	return true;
}

/*
============
arcMatX::Inverse_UpdateRankOne

  Updates the in-place inverse using the Sherman-Morrison formula to obtain the inverse for the matrix: A + alpha * v * w'
============
*/
bool arcMatX::Inverse_UpdateRankOne( const arcVecX &v, const arcVecX &w, float alpha ) {
	int i, j;
	float beta, s;
	arcVecX y, z;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );

	y.SetData( numRows, VECX_ALLOCA( numRows ) );
	z.SetData( numRows, VECX_ALLOCA( numRows ) );

	Multiply( y, v );
	TransposeMultiply( z, w );
	beta = 1.0f + ( w * y );

	if ( beta == 0.0f ) {
		return false;
	}

	alpha /= beta;

	for ( i = 0; i < numRows; i++ ) {
		s = y[i] * alpha;
		for ( j = 0; j < numColumns; j++ ) {
			(*this)[i][j] -= s * z[j];
		}
	}
	return true;
}

/*
============
arcMatX::Inverse_UpdateRowColumn

  Updates the in-place inverse to obtain the inverse for the matrix:

      [ 0  a  0 ]
  A + [ d  b  e ]
      [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1], d = w[0,r-1], w[r] = 0.0f, e = w[r+1,numColumns-1]
============
*/
bool arcMatX::Inverse_UpdateRowColumn( const arcVecX &v, const arcVecX &w, int r ) {
	arcVecX s;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && r < numRows && r < numColumns );
	assert( w[r] == 0.0f );

	s.SetData( Max( numRows, numColumns ), VECX_ALLOCA( Max( numRows, numColumns ) ) );
	s.Zero();
	s[r] = 1.0f;

	if ( !Inverse_UpdateRankOne( v, s, 1.0f ) ) {
		return false;
	}
	if ( !Inverse_UpdateRankOne( s, w, 1.0f ) ) {
		return false;
	}
	return true;
}

/*
============
arcMatX::Inverse_UpdateIncrement

  Updates the in-place inverse to obtain the inverse for the matrix:

  [ A  a ]
  [ c  b ]

  where: a = v[0,numRows-1], b = v[numRows], c = w[0,numColumns-1], w[numColumns] = 0
============
*/
bool arcMatX::Inverse_UpdateIncrement( const arcVecX &v, const arcVecX &w ) {
	arcVecX v2;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );
	assert( w.GetSize() >= numColumns+1 );

	ChangeSize( numRows+1, numColumns+1, true );
	(*this)[numRows-1][numRows-1] = 1.0f;

	v2.SetData( numRows, VECX_ALLOCA( numRows ) );
	v2 = v;
	v2[numRows-1] -= 1.0f;

	return Inverse_UpdateRowColumn( v2, w, numRows-1 );
}

/*
============
arcMatX::Inverse_UpdateDecrement

  Updates the in-place inverse to obtain the inverse of the matrix with row r and column r removed.
  v and w should store the column and row of the original matrix respectively.
============
*/
bool arcMatX::Inverse_UpdateDecrement( const arcVecX &v, const arcVecX &w, int r ) {
	arcVecX v1, w1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( w.GetSize() >= numColumns );
	assert( r >= 0 && r < numRows && r < numColumns );

	v1.SetData( numRows, VECX_ALLOCA( numRows ) );
	w1.SetData( numRows, VECX_ALLOCA( numRows ) );

	// update the row and column to identity
	v1 = -v;
	w1 = -w;
	v1[r] += 1.0f;
	w1[r] = 0.0f;

	if ( !Inverse_UpdateRowColumn( v1, w1, r ) ) {
		return false;
	}

	// physically remove the row and column
	Update_Decrement( r );

	return true;
}

/*
============
arcMatX::Inverse_Solve

  Solve Ax = b with A inverted
============
*/
void arcMatX::Inverse_Solve( arcVecX &x, const arcVecX &b ) const {
	Multiply( x, b );
}

/*
============
arcMatX::LU_Factor

  in-place factorization: LU
  L is a triangular matrix stored in the lower triangle.
  L has ones on the diagonal that are not stored.
  U is a triangular matrix stored in the upper triangle.
  If index != NULL partial pivoting is used for numerical stability.
  If index != NULL it must point to an array of numRow integers and is used to keep track of the row permutation.
  If det != NULL the determinant of the matrix is calculated and stored.
============
*/
bool arcMatX::LU_Factor( int *index, float *det ) {
	int i, j, k, newi, min;
	double s, t, d, w;

	// if partial pivoting should be used
	if ( index ) {
		for ( i = 0; i < numRows; i++ ) {
			index[i] = i;
		}
	}

	w = 1.0f;
	min = Min( numRows, numColumns );
	for ( i = 0; i < min; i++ ) {

		newi = i;
		s = arcMath::Fabs( (*this)[i][i] );

		if ( index ) {
			// find the largest absolute pivot
			for ( j = i + 1; j < numRows; j++ ) {
				t = arcMath::Fabs( (*this)[j][i] );
				if ( t > s ) {
					newi = j;
					s = t;
				}
			}
		}

		if ( s == 0.0f ) {
			return false;
		}

		if ( newi != i ) {

			w = -w;

			// swap index elements
			k = index[i];
			index[i] = index[newi];
			index[newi] = k;

			// swap rows
			for ( j = 0; j < numColumns; j++ ) {
				t = (*this)[newi][j];
				(*this)[newi][j] = (*this)[i][j];
				(*this)[i][j] = t;
			}
		}

		if ( i < numRows ) {
			d = 1.0f / (*this)[i][i];
			for ( j = i + 1; j < numRows; j++ ) {
				(*this)[j][i] *= d;
			}
		}

		if ( i < min-1 ) {
			for ( j = i + 1; j < numRows; j++ ) {
				d = (*this)[j][i];
				for ( k = i + 1; k < numColumns; k++ ) {
					(*this)[j][k] -= d * (*this)[i][k];
				}
			}
		}
	}

	if ( det ) {
		for ( i = 0; i < numRows; i++ ) {
			w *= (*this)[i][i];
		}
		*det = w;
	}

	return true;
}

/*
============
arcMatX::LU_UpdateRankOne

  Updates the in-place LU factorization to obtain the factors for the matrix: LU + alpha * v * w'
============
*/
bool arcMatX::LU_UpdateRankOne( const arcVecX &v, const arcVecX &w, float alpha, int *index ) {
	int i, j, max;
	float *y, *z;
	double diag, beta, p0, p1, d;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );

	y = (float *) _alloca16( v.GetSize() * sizeof( float ) );
	z = (float *) _alloca16( w.GetSize() * sizeof( float ) );

	if ( index != NULL ) {
		for ( i = 0; i < numRows; i++ ) {
			y[i] = alpha * v[index[i]];
		}
	} else {
		for ( i = 0; i < numRows; i++ ) {
			y[i] = alpha * v[i];
		}
	}

	memcpy( z, w.ToFloatPtr(), w.GetSize() * sizeof( float ) );

	max = Min( numRows, numColumns );
	for ( i = 0; i < max; i++ ) {
		diag = (*this)[i][i];

		p0 = y[i];
		p1 = z[i];
		diag += p0 * p1;

		if ( diag == 0.0f ) {
			return false;
		}

		beta = p1 / diag;

		(*this)[i][i] = diag;

		for ( j = i+1; j < numColumns; j++ ) {

			d = (*this)[i][j];

			d += p0 * z[j];
			z[j] -= beta * d;

			(*this)[i][j] = d;
		}

		for ( j = i+1; j < numRows; j++ ) {

			d = (*this)[j][i];

			y[j] -= p0 * d;
			d += beta * y[j];

			(*this)[j][i] = d;
		}
	}
	return true;
}

/*
============
arcMatX::LU_UpdateRowColumn

  Updates the in-place LU factorization to obtain the factors for the matrix:

       [ 0  a  0 ]
  LU + [ d  b  e ]
       [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1], d = w[0,r-1], w[r] = 0.0f, e = w[r+1,numColumns-1]
============
*/
bool arcMatX::LU_UpdateRowColumn( const arcVecX &v, const arcVecX &w, int r, int *index ) {
#if 0

	arcVecX s;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && r < numRows && r < numColumns );
	assert( w[r] == 0.0f );

	s.SetData( Max( numRows, numColumns ), VECX_ALLOCA( Max( numRows, numColumns ) ) );
	s.Zero();
	s[r] = 1.0f;

	if ( !LU_UpdateRankOne( v, s, 1.0f, index ) ) {
		return false;
	}
	if ( !LU_UpdateRankOne( s, w, 1.0f, index ) ) {
		return false;
	}
	return true;

#else

	int i, j, min, max, rp;
	float *y0, *y1, *z0, *z1;
	double diag, beta0, beta1, p0, p1, q0, q1, d;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && r < numColumns && r < numRows );
	assert( w[r] == 0.0f );

	y0 = (float *) _alloca16( v.GetSize() * sizeof( float ) );
	z0 = (float *) _alloca16( w.GetSize() * sizeof( float ) );
	y1 = (float *) _alloca16( v.GetSize() * sizeof( float ) );
	z1 = (float *) _alloca16( w.GetSize() * sizeof( float ) );

	if ( index != NULL ) {
		for ( i = 0; i < numRows; i++ ) {
			y0[i] = v[index[i]];
		}
		rp = r;
		for ( i = 0; i < numRows; i++ ) {
			if ( index[i] == r ) {
				rp = i;
				break;
			}
		}
	} else {
		memcpy( y0, v.ToFloatPtr(), v.GetSize() * sizeof( float ) );
		rp = r;
	}

	memset( y1, 0, v.GetSize() * sizeof( float ) );
	y1[rp] = 1.0f;

	memset( z0, 0, w.GetSize() * sizeof( float ) );
	z0[r] = 1.0f;

	memcpy( z1, w.ToFloatPtr(), w.GetSize() * sizeof( float ) );

	// update the beginning of the to be updated row and column
	min = Min( r, rp );
	for ( i = 0; i < min; i++ ) {
		p0 = y0[i];
		beta1 = z1[i] / (*this)[i][i];

		(*this)[i][r] += p0;
		for ( j = i+1; j < numColumns; j++ ) {
			z1[j] -= beta1 * (*this)[i][j];
		}
		for ( j = i+1; j < numRows; j++ ) {
			y0[j] -= p0 * (*this)[j][i];
		}
		(*this)[rp][i] += beta1;
	}

	// update the lower right corner starting at r,r
	max = Min( numRows, numColumns );
	for ( i = min; i < max; i++ ) {
		diag = (*this)[i][i];

		p0 = y0[i];
		p1 = z0[i];
		diag += p0 * p1;

		if ( diag == 0.0f ) {
			return false;
		}

		beta0 = p1 / diag;

		q0 = y1[i];
		q1 = z1[i];
		diag += q0 * q1;

		if ( diag == 0.0f ) {
			return false;
		}

		beta1 = q1 / diag;

		(*this)[i][i] = diag;

		for ( j = i+1; j < numColumns; j++ ) {

			d = (*this)[i][j];

			d += p0 * z0[j];
			z0[j] -= beta0 * d;

			d += q0 * z1[j];
			z1[j] -= beta1 * d;

			(*this)[i][j] = d;
		}

		for ( j = i+1; j < numRows; j++ ) {

			d = (*this)[j][i];

			y0[j] -= p0 * d;
			d += beta0 * y0[j];

			y1[j] -= q0 * d;
			d += beta1 * y1[j];

			(*this)[j][i] = d;
		}
	}
	return true;

#endif
}

/*
============
arcMatX::LU_UpdateIncrement

  Updates the in-place LU factorization to obtain the factors for the matrix:

  [ A  a ]
  [ c  b ]

  where: a = v[0,numRows-1], b = v[numRows], c = w[0,numColumns-1], w[numColumns] = 0
============
*/
bool arcMatX::LU_UpdateIncrement( const arcVecX &v, const arcVecX &w, int *index ) {
	int i, j;
	float sum;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );
	assert( w.GetSize() >= numColumns+1 );

	ChangeSize( numRows+1, numColumns+1, true );

	// add row to L
	for ( i = 0; i < numRows - 1; i++ ) {
		sum = w[i];
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[numRows - 1][j] * (*this)[j][i];
		}
		(*this)[numRows - 1 ][i] = sum / (*this)[i][i];
	}

	// add row to the permutation index
	if ( index != NULL ) {
		index[numRows - 1] = numRows - 1;
	}

	// add column to U
	for ( i = 0; i < numRows; i++ ) {
		if ( index != NULL ) {
			sum = v[index[i]];
		} else {
			sum = v[i];
		}
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * (*this)[j][numRows - 1];
		}
		(*this)[i][numRows - 1] = sum;
	}

	return true;
}

/*
============
arcMatX::LU_UpdateDecrement

  Updates the in-place LU factorization to obtain the factors for the matrix with row r and column r removed.
  v and w should store the column and row of the original matrix respectively.
  If index != NULL then u should store row index[r] of the original matrix. If index == NULL then u = w.
============
*/
bool arcMatX::LU_UpdateDecrement( const arcVecX &v, const arcVecX &w, const arcVecX &u, int r, int *index ) {
	int i, p;
	arcVecX v1, w1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && r < numRows && r < numColumns );

	v1.SetData( numRows, VECX_ALLOCA( numRows ) );
	w1.SetData( numRows, VECX_ALLOCA( numRows ) );

	if ( index != NULL ) {

		// find the pivot row
		for ( p = i = 0; i < numRows; i++ ) {
			if ( index[i] == r ) {
				p = i;
				break;
			}
		}

		// update the row and column to identity
		v1 = -v;
		w1 = -u;

		if ( p != r ) {
			idSwap( v1[index[r]], v1[index[p]] );
			idSwap( index[r], index[p] );
		}

		v1[r] += 1.0f;
		w1[r] = 0.0f;

		if ( !LU_UpdateRowColumn( v1, w1, r, index ) ) {
			return false;
		}

		if ( p != r ) {

			if ( arcMath::Fabs( u[p] ) < 1e-4f ) {
				// NOTE: an additional row interchange is required for numerical stability
			}

			// move row index[r] of the original matrix to row index[p] of the original matrix
			v1.Zero();
			v1[index[p]] = 1.0f;
			w1 = u - w;

			if ( !LU_UpdateRankOne( v1, w1, 1.0f, index ) ) {
				return false;
			}
		}

		// remove the row from the permutation index
		for ( i = r; i < numRows - 1; i++ ) {
			index[i] = index[i+1];
		}
		for ( i = 0; i < numRows - 1; i++ ) {
			if ( index[i] > r ) {
				index[i]--;
			}
		}

	} else {

		v1 = -v;
		w1 = -w;
		v1[r] += 1.0f;
		w1[r] = 0.0f;

		if ( !LU_UpdateRowColumn( v1, w1, r, index ) ) {
			return false;
		}
	}

	// physically remove the row and column
	Update_Decrement( r );

	return true;
}

/*
============
arcMatX::LU_Solve

  Solve Ax = b with A factored in-place as: LU
============
*/
void arcMatX::LU_Solve( arcVecX &x, const arcVecX &b, const int *index ) const {
	int i, j;
	double sum;

	assert( x.GetSize() == numColumns && b.GetSize() == numRows );

	// solve L
	for ( i = 0; i < numRows; i++ ) {
		if ( index != NULL ) {
			sum = b[index[i]];
		} else {
			sum = b[i];
		}
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = sum;
	}

	// solve U
	for ( i = numRows - 1; i >= 0; i-- ) {
		sum = x[i];
		for ( j = i + 1; j < numRows; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = sum / (*this)[i][i];
	}
}

/*
============
arcMatX::LU_Inverse

  Calculates the inverse of the matrix which is factored in-place as LU
============
*/
void arcMatX::LU_Inverse( arcMatX &inv, const int *index ) const {
	int i, j;
	arcVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for ( i = 0; i < numRows; i++ ) {

		b[i] = 1.0f;
		LU_Solve( x, b, index );
		for ( j = 0; j < numRows; j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
arcMatX::LU_UnpackFactors

  Unpacks the in-place LU factorization.
============
*/
void arcMatX::LU_UnpackFactors( arcMatX &L, arcMatX &U ) const {
	int i, j;

	L.Zero( numRows, numColumns );
	U.Zero( numRows, numColumns );
	for ( i = 0; i < numRows; i++ ) {
		for ( j = 0; j < i; j++ ) {
			L[i][j] = (*this)[i][j];
		}
		L[i][i] = 1.0f;
		for ( j = i; j < numColumns; j++ ) {
			U[i][j] = (*this)[i][j];
		}
	}
}

/*
============
arcMatX::LU_MultiplyFactors

  Multiplies the factors of the in-place LU factorization to form the original matrix.
============
*/
void arcMatX::LU_MultiplyFactors( arcMatX &m, const int *index ) const {
	int r, rp, i, j;
	double sum;

	m.SetSize( numRows, numColumns );

	for ( r = 0; r < numRows; r++ ) {

		if ( index != NULL ) {
			rp = index[r];
		} else {
			rp = r;
		}

		// calculate row of matrix
		for ( i = 0; i < numColumns; i++ ) {
			if ( i >= r ) {
				sum = (*this)[r][i];
			} else {
				sum = 0.0f;
			}
			for ( j = 0; j <= i && j < r; j++ ) {
				sum += (*this)[r][j] * (*this)[j][i];
			}
			m[rp][i] = sum;
		}
	}
}

/*
============
arcMatX::QR_Factor

  in-place factorization: QR
  Q is an orthogonal matrix represented as a product of Householder matrices stored in the lower triangle and c.
  R is a triangular matrix stored in the upper triangle except for the diagonal elements which are stored in d.
  The initial matrix has to be square.
============
*/
bool arcMatX::QR_Factor( arcVecX &c, arcVecX &d ) {
	int i, j, k;
	double scale, s, t, sum;
	bool singular = false;

	assert( numRows == numColumns );
	assert( c.GetSize() >= numRows && d.GetSize() >= numRows );

	for ( k = 0; k < numRows-1; k++ ) {

		scale = 0.0f;
		for ( i = k; i < numRows; i++ ) {
			s = arcMath::Fabs( (*this)[i][k] );
			if ( s > scale ) {
				scale = s;
			}
		}
		if ( scale == 0.0f ) {
			singular = true;
			c[k] = d[k] = 0.0f;
		} else {

			s = 1.0f / scale;
			for ( i = k; i < numRows; i++ ) {
				(*this)[i][k] *= s;
			}

			sum = 0.0f;
			for ( i = k; i < numRows; i++ ) {
				s = (*this)[i][k];
				sum += s * s;
			}

			s = arcMath::Sqrt( sum );
			if ( (*this)[k][k] < 0.0f ) {
				s = -s;
			}
			(*this)[k][k] += s;
			c[k] = s * (*this)[k][k];
			d[k] = -scale * s;

			for ( j = k+1; j < numRows; j++ ) {

				sum = 0.0f;
				for ( i = k; i < numRows; i++ ) {
					sum += (*this)[i][k] * (*this)[i][j];
				}
				t = sum / c[k];
				for ( i = k; i < numRows; i++ ) {
					(*this)[i][j] -= t * (*this)[i][k];
				}
			}
		}
	}
	d[numRows-1] = (*this)[ (numRows-1 ) ][ (numRows-1 ) ];
	if ( d[numRows-1] == 0.0f ) {
		singular = true;
	}

	return !singular;
}

/*
============
arcMatX::QR_Rotate

  Performs a Jacobi rotation on the rows i and i+1 of the unpacked QR factors.
============
*/
void arcMatX::QR_Rotate( arcMatX &R, int i, float a, float b ) {
	int j;
	float f, c, s, w, y;

	if ( a == 0.0f ) {
		c = 0.0f;
		s = ( b >= 0.0f ) ? 1.0f : -1.0f;
	} else if ( arcMath::Fabs( a ) > arcMath::Fabs( b ) ) {
		f = b / a;
		c = arcMath::Fabs( 1.0f / arcMath::Sqrt( 1.0f + f * f ) );
		if ( a < 0.0f ) {
			c = -c;
		}
		s = f * c;
	} else {
		f = a / b;
		s = arcMath::Fabs( 1.0f / arcMath::Sqrt( 1.0f + f * f ) );
		if ( b < 0.0f ) {
			s = -s;
		}
		c = f * s;
	}
	for ( j = i; j < numRows; j++ ) {
		y = R[i][j];
		w = R[i+1][j];
		R[i][j] = c * y - s * w;
		R[i+1][j] = s * y + c * w;
	}
	for ( j = 0; j < numRows; j++ ) {
		y = (*this)[j][i];
		w = (*this)[j][i+1];
		(*this)[j][i] = c * y - s * w;
		(*this)[j][i+1] = s * y + c * w;
	}
}

/*
============
arcMatX::QR_UpdateRankOne

  Updates the unpacked QR factorization to obtain the factors for the matrix: QR + alpha * v * w'
============
*/
bool arcMatX::QR_UpdateRankOne( arcMatX &R, const arcVecX &v, const arcVecX &w, float alpha ) {
	int i, k;
	float f;
	arcVecX u;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );

	u.SetData( v.GetSize(), VECX_ALLOCA( v.GetSize() ) );
	TransposeMultiply( u, v );
	u *= alpha;

	for ( k = v.GetSize()-1; k > 0; k-- ) {
		if ( u[k] != 0.0f ) {
			break;
		}
	}
	for ( i = k-1; i >= 0; i-- ) {
		QR_Rotate( R, i, u[i], -u[i+1] );
		if ( u[i] == 0.0f ) {
			u[i] = arcMath::Fabs( u[i+1] );
		} else if ( arcMath::Fabs( u[i] ) > arcMath::Fabs( u[i+1] ) ) {
			f = u[i+1] / u[i];
			u[i] = arcMath::Fabs( u[i] ) * arcMath::Sqrt( 1.0f + f * f );
		} else {
			f = u[i] / u[i+1];
			u[i] = arcMath::Fabs( u[i+1] ) * arcMath::Sqrt( 1.0f + f * f );
		}
	}
	for ( i = 0; i < v.GetSize(); i++ ) {
		R[0][i] += u[0] * w[i];
	}
	for ( i = 0; i < k; i++ ) {
		QR_Rotate( R, i, -R[i][i], R[i+1][i] );
	}
	return true;
}

/*
============
arcMatX::QR_UpdateRowColumn

  Updates the unpacked QR factorization to obtain the factors for the matrix:

       [ 0  a  0 ]
  QR + [ d  b  e ]
       [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1], d = w[0,r-1], w[r] = 0.0f, e = w[r+1,numColumns-1]
============
*/
bool arcMatX::QR_UpdateRowColumn( arcMatX &R, const arcVecX &v, const arcVecX &w, int r ) {
	arcVecX s;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && r < numRows && r < numColumns );
	assert( w[r] == 0.0f );

	s.SetData( Max( numRows, numColumns ), VECX_ALLOCA( Max( numRows, numColumns ) ) );
	s.Zero();
	s[r] = 1.0f;

	if ( !QR_UpdateRankOne( R, v, s, 1.0f ) ) {
		return false;
	}
	if ( !QR_UpdateRankOne( R, s, w, 1.0f ) ) {
		return false;
	}
	return true;
}

/*
============
arcMatX::QR_UpdateIncrement

  Updates the unpacked QR factorization to obtain the factors for the matrix:

  [ A  a ]
  [ c  b ]

  where: a = v[0,numRows-1], b = v[numRows], c = w[0,numColumns-1], w[numColumns] = 0
============
*/
bool arcMatX::QR_UpdateIncrement( arcMatX &R, const arcVecX &v, const arcVecX &w ) {
	arcVecX v2;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );
	assert( w.GetSize() >= numColumns+1 );

	ChangeSize( numRows+1, numColumns+1, true );
	(*this)[numRows-1][numRows-1] = 1.0f;

	R.ChangeSize( R.numRows+1, R.numColumns+1, true );
	R[R.numRows-1][R.numRows-1] = 1.0f;

	v2.SetData( numRows, VECX_ALLOCA( numRows ) );
	v2 = v;
	v2[numRows-1] -= 1.0f;

	return QR_UpdateRowColumn( R, v2, w, numRows-1 );
}

/*
============
arcMatX::QR_UpdateDecrement

  Updates the unpacked QR factorization to obtain the factors for the matrix with row r and column r removed.
  v and w should store the column and row of the original matrix respectively.
============
*/
bool arcMatX::QR_UpdateDecrement( arcMatX &R, const arcVecX &v, const arcVecX &w, int r ) {
	arcVecX v1, w1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( w.GetSize() >= numColumns );
	assert( r >= 0 && r < numRows && r < numColumns );

	v1.SetData( numRows, VECX_ALLOCA( numRows ) );
	w1.SetData( numRows, VECX_ALLOCA( numRows ) );

	// update the row and column to identity
	v1 = -v;
	w1 = -w;
	v1[r] += 1.0f;
	w1[r] = 0.0f;

	if ( !QR_UpdateRowColumn( R, v1, w1, r ) ) {
		return false;
	}

	// physically remove the row and column
	Update_Decrement( r );
	R.Update_Decrement( r );

	return true;
}

/*
============
arcMatX::QR_Solve

  Solve Ax = b with A factored in-place as: QR
============
*/
void arcMatX::QR_Solve( arcVecX &x, const arcVecX &b, const arcVecX &c, const arcVecX &d ) const {
	int i, j;
	double sum, t;

	assert( numRows == numColumns );
	assert( x.GetSize() >= numRows && b.GetSize() >= numRows );
	assert( c.GetSize() >= numRows && d.GetSize() >= numRows );

	for ( i = 0; i < numRows; i++ ) {
		x[i] = b[i];
	}

	// multiply b with transpose of Q
	for ( i = 0; i < numRows-1; i++ ) {

		sum = 0.0f;
		for ( j = i; j < numRows; j++ ) {
			sum += (*this)[j][i] * x[j];
		}
		t = sum / c[i];
		for ( j = i; j < numRows; j++ ) {
			x[j] -= t * (*this)[j][i];
		}
	}

	// backsubstitution with R
	for ( i = numRows-1; i >= 0; i-- ) {

		sum = x[i];
		for ( j = i + 1; j < numRows; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = sum / d[i];
	}
}

/*
============
arcMatX::QR_Solve

  Solve Ax = b with A factored as: QR
============
*/
void arcMatX::QR_Solve( arcVecX &x, const arcVecX &b, const arcMatX &R ) const {
	int i, j;
	double sum;

	assert( numRows == numColumns );

	// multiply b with transpose of Q
	TransposeMultiply( x, b );

	// backsubstitution with R
	for ( i = numRows-1; i >= 0; i-- ) {

		sum = x[i];
		for ( j = i + 1; j < numRows; j++ ) {
			sum -= R[i][j] * x[j];
		}
		x[i] = sum / R[i][i];
	}
}

/*
============
arcMatX::QR_Inverse

  Calculates the inverse of the matrix which is factored in-place as: QR
============
*/
void arcMatX::QR_Inverse( arcMatX &inv, const arcVecX &c, const arcVecX &d ) const {
	int i, j;
	arcVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for ( i = 0; i < numRows; i++ ) {

		b[i] = 1.0f;
		QR_Solve( x, b, c, d );
		for ( j = 0; j < numRows; j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
arcMatX::QR_UnpackFactors

  Unpacks the in-place QR factorization.
============
*/
void arcMatX::QR_UnpackFactors( arcMatX &Q, arcMatX &R, const arcVecX &c, const arcVecX &d ) const {
	int i, j, k;
	double sum;

	Q.Identity( numRows, numColumns );
	for ( i = 0; i < numColumns-1; i++ ) {
		if ( c[i] == 0.0f ) {
			continue;
		}
		for ( j = 0; j < numRows; j++ ) {
			sum = 0.0f;
			for ( k = i; k < numColumns; k++ ) {
				sum += (*this)[k][i] * Q[j][k];
			}
			sum /= c[i];
			for ( k = i; k < numColumns; k++ ) {
				Q[j][k] -= sum * (*this)[k][i];
			}
		}
	}

	R.Zero( numRows, numColumns );
	for ( i = 0; i < numRows; i++ ) {
		R[i][i] = d[i];
		for ( j = i+1; j < numColumns; j++ ) {
			R[i][j] = (*this)[i][j];
		}
	}
}

/*
============
arcMatX::QR_MultiplyFactors

  Multiplies the factors of the in-place QR factorization to form the original matrix.
============
*/
void arcMatX::QR_MultiplyFactors( arcMatX &m, const arcVecX &c, const arcVecX &d ) const {
	int i, j, k;
	double sum;
	arcMatX Q;

	Q.Identity( numRows, numColumns );
	for ( i = 0; i < numColumns-1; i++ ) {
		if ( c[i] == 0.0f ) {
			continue;
		}
		for ( j = 0; j < numRows; j++ ) {
			sum = 0.0f;
			for ( k = i; k < numColumns; k++ ) {
				sum += (*this)[k][i] * Q[j][k];
			}
			sum /= c[i];
			for ( k = i; k < numColumns; k++ ) {
				Q[j][k] -= sum * (*this)[k][i];
			}
		}
	}

	for ( i = 0; i < numRows; i++ ) {
		for ( j = 0; j < numColumns; j++ ) {
			sum = Q[i][j] * d[i];
			for ( k = 0; k < i; k++ ) {
				sum += Q[i][k] * (*this)[j][k];
			}
			m[i][j] = sum;
		}
	}
}

/*
============
arcMatX::Pythag

  Computes (a^2 + b^2)^1/2 without underflow or overflow.
============
*/
float arcMatX::Pythag( float a, float b ) const {
	double at, bt, ct;

	at = arcMath::Fabs( a );
	bt = arcMath::Fabs( b );
	if ( at > bt ) {
		ct = bt / at;
		return at * arcMath::Sqrt( 1.0f + ct * ct );
	} else {
		if ( bt ) {
			ct = at / bt;
			return bt * arcMath::Sqrt( 1.0f + ct * ct );
		} else {
			return 0.0f;
		}
	}
}

/*
============
arcMatX::SVD_BiDiag
============
*/
void arcMatX::SVD_BiDiag( arcVecX &w, arcVecX &rv1, float &anorm ) {
	int i, j, k, l;
	double f, h, r, g, s, scale;

	anorm = 0.0f;
	g = s = scale = 0.0f;
	for ( i = 0; i < numColumns; i++ ) {
		l = i + 1;
		rv1[i] = scale * g;
		g = s = scale = 0.0f;
		if ( i < numRows ) {
			for ( k = i; k < numRows; k++ ) {
				scale += arcMath::Fabs( (*this)[k][i] );
			}
			if ( scale ) {
				for ( k = i; k < numRows; k++ ) {
					(*this)[k][i] /= scale;
					s += (*this)[k][i] * (*this)[k][i];
				}
				f = (*this)[i][i];
				g = arcMath::Sqrt( s );
				if ( f >= 0.0f ) {
					g = -g;
				}
				h = f * g - s;
				(*this)[i][i] = f - g;
				if ( i != (numColumns-1 ) ) {
					for ( j = l; j < numColumns; j++ ) {
						for ( s = 0.0f, k = i; k < numRows; k++ ) {
							s += (*this)[k][i] * (*this)[k][j];
						}
						f = s / h;
						for ( k = i; k < numRows; k++ ) {
							(*this)[k][j] += f * (*this)[k][i];
						}
					}
				}
				for ( k = i; k < numRows; k++ ) {
					(*this)[k][i] *= scale;
				}
			}
		}
		w[i] = scale * g;
		g = s = scale = 0.0f;
		if ( i < numRows && i != (numColumns-1 ) ) {
			for ( k = l; k < numColumns; k++ ) {
				scale += arcMath::Fabs( (*this)[i][k] );
			}
			if ( scale ) {
				for ( k = l; k < numColumns; k++ ) {
					(*this)[i][k] /= scale;
					s += (*this)[i][k] * (*this)[i][k];
				}
				f = (*this)[i][l];
				g = arcMath::Sqrt( s );
				if ( f >= 0.0f ) {
					g = -g;
				}
				h = 1.0f / ( f * g - s );
				(*this)[i][l] = f - g;
				for ( k = l; k < numColumns; k++ ) {
					rv1[k] = (*this)[i][k] * h;
				}
				if ( i != (numRows-1 ) ) {
					for ( j = l; j < numRows; j++ ) {
						for ( s = 0.0f, k = l; k < numColumns; k++ ) {
							s += (*this)[j][k] * (*this)[i][k];
						}
						for ( k = l; k < numColumns; k++ ) {
							(*this)[j][k] += s * rv1[k];
						}
					}
				}
				for ( k = l; k < numColumns; k++ ) {
					(*this)[i][k] *= scale;
				}
			}
		}
		r = arcMath::Fabs( w[i] ) + arcMath::Fabs( rv1[i] );
		if ( r > anorm ) {
			anorm = r;
		}
	}
}

/*
============
arcMatX::SVD_InitialWV
============
*/
void arcMatX::SVD_InitialWV( arcVecX &w, arcMatX &V, arcVecX &rv1 ) {
	int i, j, k, l;
	double f, g, s;

	g = 0.0f;
	for ( i = (numColumns-1 ); i >= 0; i-- ) {
		l = i + 1;
		if ( i < ( numColumns - 1 ) ) {
			if ( g ) {
				for ( j = l; j < numColumns; j++ ) {
					V[j][i] = ((*this)[i][j] / (*this)[i][l] ) / g;
				}
				// double division to reduce underflow
				for ( j = l; j < numColumns; j++ ) {
					for ( s = 0.0f, k = l; k < numColumns; k++ ) {
						s += (*this)[i][k] * V[k][j];
					}
					for ( k = l; k < numColumns; k++ ) {
						V[k][j] += s * V[k][i];
					}
				}
			}
			for ( j = l; j < numColumns; j++ ) {
				V[i][j] = V[j][i] = 0.0f;
			}
		}
		V[i][i] = 1.0f;
		g = rv1[i];
	}
	for ( i = numColumns - 1; i >= 0; i-- ) {
		l = i + 1;
		g = w[i];
		if ( i < (numColumns-1 ) ) {
			for ( j = l; j < numColumns; j++ ) {
				(*this)[i][j] = 0.0f;
			}
		}
		if ( g ) {
			g = 1.0f / g;
			if ( i != (numColumns-1 ) ) {
				for ( j = l; j < numColumns; j++ ) {
					for ( s = 0.0f, k = l; k < numRows; k++ ) {
						s += (*this)[k][i] * (*this)[k][j];
					}
					f = (s / (*this)[i][i] ) * g;
					for ( k = i; k < numRows; k++ ) {
						(*this)[k][j] += f * (*this)[k][i];
					}
				}
			}
			for ( j = i; j < numRows; j++ ) {
				(*this)[j][i] *= g;
			}
		}
		else {
			for ( j = i; j < numRows; j++ ) {
				(*this)[j][i] = 0.0f;
			}
		}
		(*this)[i][i] += 1.0f;
	}
}

/*
============
arcMatX::SVD_Factor

  in-place factorization: U * Diag(w) * V.Transpose()
  known as the Singular Value Decomposition.
  U is a column-orthogonal matrix which overwrites the original matrix.
  w is a diagonal matrix with all elements >= 0 which are the singular values.
  V is the transpose of an orthogonal matrix.
============
*/
bool arcMatX::SVD_Factor( arcVecX &w, arcMatX &V ) {
	int flag, i, its, j, jj, k, l, nm;
	double c, f, h, s, x, y, z, r, g = 0.0f;
	float anorm = 0.0f;
	arcVecX rv1;

	if ( numRows < numColumns ) {
		return false;
	}

	rv1.SetData( numColumns, VECX_ALLOCA( numColumns ) );
	rv1.Zero();
	w.Zero( numColumns );
	V.Zero( numColumns, numColumns );

	SVD_BiDiag( w, rv1, anorm );
	SVD_InitialWV( w, V, rv1 );

	for ( k = numColumns - 1; k >= 0; k-- ) {
		for ( its = 1; its <= 30; its++ ) {
			flag = 1;
			nm = 0;
			for ( l = k; l >= 0; l-- ) {
				nm = l - 1;
				if ( ( arcMath::Fabs( rv1[l] ) + anorm ) == anorm /* arcMath::Fabs( rv1[l] ) < arcMath::FLT_EPSILON */ ) {
					flag = 0;
					break;
				}
				if ( ( arcMath::Fabs( w[nm] ) + anorm ) == anorm /* arcMath::Fabs( w[nm] ) < arcMath::FLT_EPSILON */ ) {
					break;
				}
			}
			if ( flag ) {
				c = 0.0f;
				s = 1.0f;
				for ( i = l; i <= k; i++ ) {
					f = s * rv1[i];

					if ( ( arcMath::Fabs( f ) + anorm ) != anorm /* arcMath::Fabs( f ) > arcMath::FLT_EPSILON */ ) {
						g = w[i];
						h = Pythag( f, g );
						w[i] = h;
						h = 1.0f / h;
						c = g * h;
						s = -f * h;
						for ( j = 0; j < numRows; j++ ) {
							y = (*this)[j][nm];
							z = (*this)[j][i];
							(*this)[j][nm] = y * c + z * s;
							(*this)[j][i] = z * c - y * s;
						}
					}
				}
			}
			z = w[k];
			if ( l == k ) {
				if ( z < 0.0f ) {
					w[k] = -z;
					for ( j = 0; j < numColumns; j++ ) {
						V[j][k] = -V[j][k];
					}
				}
				break;
			}
			if ( its == 30 ) {
				return false;		// no convergence
			}
			x = w[l];
			nm = k - 1;
			y = w[nm];
			g = rv1[nm];
			h = rv1[k];
			f = ( ( y - z ) * ( y + z ) + ( g - h ) * ( g + h ) ) / ( 2.0f * h * y );
			g = Pythag( f, 1.0f );
			r = ( f >= 0.0f ? g : - g );
			f= ( ( x - z ) * ( x + z ) + h * ( ( y / ( f + r ) ) - h ) ) / x;
			c = s = 1.0f;
			for ( j = l; j <= nm; j++ ) {
				i = j + 1;
				g = rv1[i];
				y = w[i];
				h = s * g;
				g = c * g;
				z = Pythag( f, h );
				rv1[j] = z;
				c = f / z;
				s = h / z;
				f = x * c + g * s;
				g = g * c - x * s;
				h = y * s;
				y = y * c;
				for ( jj = 0; jj < numColumns; jj++ ) {
					x = V[jj][j];
					z = V[jj][i];
					V[jj][j] = x * c + z * s;
					V[jj][i] = z * c - x * s;
				}
				z = Pythag( f, h );
				w[j] = z;
				if ( z ) {
					z = 1.0f / z;
					c = f * z;
					s = h * z;
				}
				f = ( c * g ) + ( s * y );
				x = ( c * y ) - ( s * g );
				for ( jj = 0; jj < numRows; jj++ ) {
					y = (*this)[jj][j];
					z = (*this)[jj][i];
					(*this)[jj][j] = y * c + z * s;
					(*this)[jj][i] = z * c - y * s;
				}
			}
			rv1[l] = 0.0f;
			rv1[k] = f;
			w[k] = x;
		}
	}
	return true;
}

/*
============
arcMatX::SVD_Solve

  Solve Ax = b with A factored as: U * Diag(w) * V.Transpose()
============
*/
void arcMatX::SVD_Solve( arcVecX &x, const arcVecX &b, const arcVecX &w, const arcMatX &V ) const {
	int i, j;
	double sum;
	arcVecX tmp;

	assert( x.GetSize() >= numColumns );
	assert( b.GetSize() >= numColumns );
	assert( w.GetSize() == numColumns );
	assert( V.GetNumRows() == numColumns && V.GetNumColumns() == numColumns );

	tmp.SetData( numColumns, VECX_ALLOCA( numColumns ) );

	for ( i = 0; i < numColumns; i++ ) {
		sum = 0.0f;
		if ( w[i] >= arcMath::FLT_EPSILON ) {
			for ( j = 0; j < numRows; j++ ) {
				sum += (*this)[j][i] * b[j];
			}
			sum /= w[i];
		}
		tmp[i] = sum;
	}
	for ( i = 0; i < numColumns; i++ ) {
		sum = 0.0f;
		for ( j = 0; j < numColumns; j++ ) {
			sum += V[i][j] * tmp[j];
		}
		x[i] = sum;
	}
}

/*
============
arcMatX::SVD_Inverse

  Calculates the inverse of the matrix which is factored in-place as: U * Diag(w) * V.Transpose()
============
*/
void arcMatX::SVD_Inverse( arcMatX &inv, const arcVecX &w, const arcMatX &V ) const {
	int i, j, k;
	double wi, sum;
	arcMatX V2;

	assert( numRows == numColumns );

	V2 = V;

	// V * [diag(1/w[i] )]
	for ( i = 0; i < numRows; i++ ) {
		wi = w[i];
		wi = ( wi < arcMath::FLT_EPSILON ) ? 0.0f : 1.0f / wi;
		for ( j = 0; j < numColumns; j++ ) {
			V2[j][i] *= wi;
		}
	}

	// V * [diag(1/w[i] )] * Ut
	for ( i = 0; i < numRows; i++ ) {
		for ( j = 0; j < numColumns; j++ ) {
			sum = V2[i][0] * (*this)[j][0];
			for ( k = 1; k < numColumns; k++ ) {
				sum += V2[i][k] * (*this)[j][k];
			}
			inv[i][j] = sum;
		}
	}
}

/*
============
arcMatX::SVD_MultiplyFactors

  Multiplies the factors of the in-place SVD factorization to form the original matrix.
============
*/
void arcMatX::SVD_MultiplyFactors( arcMatX &m, const arcVecX &w, const arcMatX &V ) const {
	int r, i, j;
	double sum;

	m.SetSize( numRows, V.GetNumRows() );

	for ( r = 0; r < numRows; r++ ) {
		// calculate row of matrix
		if ( w[r] >= arcMath::FLT_EPSILON ) {
			for ( i = 0; i < V.GetNumRows(); i++ ) {
				sum = 0.0f;
				for ( j = 0; j < numColumns; j++ ) {
					sum += (*this)[r][j] * V[i][j];
				}
				m[r][i] = sum * w[r];
			}
		} else {
			for ( i = 0; i < V.GetNumRows(); i++ ) {
				m[r][i] = 0.0f;
			}
		}
	}
}

/*
============
arcMatX::Cholesky_Factor

  in-place Cholesky factorization: LL'
  L is a triangular matrix stored in the lower triangle.
  The upper triangle is not cleared.
  The initial matrix has to be symmetric positive definite.
============
*/
bool arcMatX::Cholesky_Factor( void ) {
	int i, j, k;
	float *invSqrt;
	double sum;

	assert( numRows == numColumns );

	invSqrt = (float *) _alloca16( numRows * sizeof( float ) );

	for ( i = 0; i < numRows; i++ ) {

		for ( j = 0; j < i; j++ ) {

			sum = (*this)[i][j];
			for ( k = 0; k < j; k++ ) {
				sum -= (*this)[i][k] * (*this)[j][k];
			}
			(*this)[i][j] = sum * invSqrt[j];
		}

		sum = (*this)[i][i];
		for ( k = 0; k < i; k++ ) {
			sum -= (*this)[i][k] * (*this)[i][k];
		}

		if ( sum <= 0.0f ) {
			return false;
		}

		invSqrt[i] = arcMath::InvSqrt( sum );
		(*this)[i][i] = invSqrt[i] * sum;
	}
	return true;
}

/*
============
arcMatX::Cholesky_UpdateRankOne

  Updates the in-place Cholesky factorization to obtain the factors for the matrix: LL' + alpha * v * v'
  If offset > 0 only the lower right corner starting at (offset, offset) is updated.
============
*/
bool arcMatX::Cholesky_UpdateRankOne( const arcVecX &v, float alpha, int offset ) {
	int i, j;
	float *y;
	double diag, invDiag, diagSqr, newDiag, newDiagSqr, beta, p, d;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( offset >= 0 && offset < numRows );

	y = (float *) _alloca16( v.GetSize() * sizeof( float ) );
	memcpy( y, v.ToFloatPtr(), v.GetSize() * sizeof( float ) );

	for ( i = offset; i < numColumns; i++ ) {
		p = y[i];
		diag = (*this)[i][i];
		invDiag = 1.0f / diag;
		diagSqr = diag * diag;
		newDiagSqr = diagSqr + alpha * p * p;

		if ( newDiagSqr <= 0.0f ) {
			return false;
		}

		(*this)[i][i] = newDiag = arcMath::Sqrt( newDiagSqr );

		alpha /= newDiagSqr;
		beta = p * alpha;
		alpha *= diagSqr;

		for ( j = i+1; j < numRows; j++ ) {

			d = (*this)[j][i] * invDiag;

			y[j] -= p * d;
			d += beta * y[j];

			(*this)[j][i] = d * newDiag;
		}
	}
	return true;
}

/*
============
arcMatX::Cholesky_UpdateRowColumn

  Updates the in-place Cholesky factorization to obtain the factors for the matrix:

        [ 0  a  0 ]
  LL' + [ a  b  c ]
        [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1]
============
*/
bool arcMatX::Cholesky_UpdateRowColumn( const arcVecX &v, int r ) {
	int i, j;
	double sum;
	float *original, *y;
	arcVecX addSub;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( r >= 0 && r < numRows );

	addSub.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );

	if ( r == 0 ) {

		if ( numColumns == 1 ) {
			double v0 = v[0];
			sum = (*this)[0][0];
			sum = sum * sum;
			sum = sum + v0;
			if ( sum <= 0.0f ) {
				return false;
			}
			(*this)[0][0] = arcMath::Sqrt( sum );
			return true;
		}
		for ( i = 0; i < numColumns; i++ ) {
			addSub[i] = v[i];
		}

	} else {

		original = (float *) _alloca16( numColumns * sizeof( float ) );
		y = (float *) _alloca16( numColumns * sizeof( float ) );

		// calculate original row/column of matrix
		for ( i = 0; i < numRows; i++ ) {
			sum = 0.0f;
			for ( j = 0; j <= i; j++ ) {
				sum += (*this)[r][j] * (*this)[i][j];
			}
			original[i] = sum;
		}

		// solve for y in L * y = original + v
		for ( i = 0; i < r; i++ ) {
			sum = original[i] + v[i];
			for ( j = 0; j < i; j++ ) {
				sum -= (*this)[r][j] * (*this)[i][j];
			}
			(*this)[r][i] = sum / (*this)[i][i];
		}

		// if the last row/column of the matrix is updated
		if ( r == numColumns - 1 ) {
			// only calculate new diagonal
			sum = original[r] + v[r];
			for ( j = 0; j < r; j++ ) {
				sum -= (*this)[r][j] * (*this)[r][j];
			}
			if ( sum <= 0.0f ) {
				return false;
			}
			(*this)[r][r] = arcMath::Sqrt( sum );
			return true;
		}

		// calculate the row/column to be added to the lower right sub matrix starting at (r, r)
		for ( i = r; i < numColumns; i++ ) {
			sum = 0.0f;
			for ( j = 0; j <= r; j++ ) {
				sum += (*this)[r][j] * (*this)[i][j];
			}
			addSub[i] = v[i] - ( sum - original[i] );
		}
	}

	// add row/column to the lower right sub matrix starting at (r, r)

#if 0

	arcVecX v1, v2;
	double d;

	v1.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );
	v2.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );

	d = arcMath::SQRT_1OVER2;
	v1[r] = ( 0.5f * addSub[r] + 1.0f ) * d;
	v2[r] = ( 0.5f * addSub[r] - 1.0f ) * d;
	for ( i = r+1; i < numColumns; i++ ) {
		v1[i] = v2[i] = addSub[i] * d;
	}

	// update
	if ( !Cholesky_UpdateRankOne( v1, 1.0f, r ) ) {
		return false;
	}
	// downdate
	if ( !Cholesky_UpdateRankOne( v2, -1.0f, r ) ) {
		return false;
	}

#else

	float *v1, *v2;
	double diag, invDiag, diagSqr, newDiag, newDiagSqr;
	double alpha1, alpha2, beta1, beta2, p1, p2, d;

	v1 = (float *) _alloca16( numColumns * sizeof( float ) );
	v2 = (float *) _alloca16( numColumns * sizeof( float ) );

	d = arcMath::SQRT_1OVER2;
	v1[r] = ( 0.5f * addSub[r] + 1.0f ) * d;
	v2[r] = ( 0.5f * addSub[r] - 1.0f ) * d;
	for ( i = r+1; i < numColumns; i++ ) {
		v1[i] = v2[i] = addSub[i] * d;
	}

	alpha1 = 1.0f;
	alpha2 = -1.0f;

	// simultaneous update/downdate of the sub matrix starting at (r, r)
	for ( i = r; i < numColumns; i++ ) {
		p1 = v1[i];
		diag = (*this)[i][i];
		invDiag = 1.0f / diag;
		diagSqr = diag * diag;
		newDiagSqr = diagSqr + alpha1 * p1 * p1;

		if ( newDiagSqr <= 0.0f ) {
			return false;
		}

		alpha1 /= newDiagSqr;
		beta1 = p1 * alpha1;
		alpha1 *= diagSqr;

		p2 = v2[i];
		diagSqr = newDiagSqr;
		newDiagSqr = diagSqr + alpha2 * p2 * p2;

		if ( newDiagSqr <= 0.0f ) {
			return false;
		}

		(*this)[i][i] = newDiag = arcMath::Sqrt( newDiagSqr );

		alpha2 /= newDiagSqr;
		beta2 = p2 * alpha2;
		alpha2 *= diagSqr;

		for ( j = i+1; j < numRows; j++ ) {

			d = (*this)[j][i] * invDiag;

			v1[j] -= p1 * d;
			d += beta1 * v1[j];

			v2[j] -= p2 * d;
			d += beta2 * v2[j];

			(*this)[j][i] = d * newDiag;
		}
	}

#endif

	return true;
}

/*
============
arcMatX::Cholesky_UpdateIncrement

  Updates the in-place Cholesky factorization to obtain the factors for the matrix:

  [ A  a ]
  [ a  b ]

  where: a = v[0,numRows-1], b = v[numRows]
============
*/
bool arcMatX::Cholesky_UpdateIncrement( const arcVecX &v ) {
	int i, j;
	float *x;
	double sum;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );

	ChangeSize( numRows+1, numColumns+1, false );

	x = (float *) _alloca16( numRows * sizeof( float ) );

	// solve for x in L * x = v
	for ( i = 0; i < numRows - 1; i++ ) {
		sum = v[i];
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = sum / (*this)[i][i];
	}

	// calculate new row of L and calculate the square of the diagonal entry
	sum = v[numRows - 1];
	for ( i = 0; i < numRows - 1; i++ ) {
		(*this)[numRows - 1][i] = x[i];
		sum -= x[i] * x[i];
	}

	if ( sum <= 0.0f ) {
		return false;
	}

	// store the diagonal entry
	(*this)[numRows - 1][numRows - 1] = arcMath::Sqrt( sum );

	return true;
}

/*
============
arcMatX::Cholesky_UpdateDecrement

  Updates the in-place Cholesky factorization to obtain the factors for the matrix with row r and column r removed.
  v should store the row of the original matrix.
============
*/
bool arcMatX::Cholesky_UpdateDecrement( const arcVecX &v, int r ) {
	arcVecX v1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( r >= 0 && r < numRows );

	v1.SetData( numRows, VECX_ALLOCA( numRows ) );

	// update the row and column to identity
	v1 = -v;
	v1[r] += 1.0f;

	// NOTE:	msvc compiler bug: the this pointer stored in edi is expected to stay
	//			untouched when calling Cholesky_UpdateRowColumn in the if statement
#if 0
	if ( !Cholesky_UpdateRowColumn( v1, r ) ) {
#else
	bool ret = Cholesky_UpdateRowColumn( v1, r );
	if ( !ret ) {
#endif
		return false;
	}

	// physically remove the row and column
	Update_Decrement( r );

	return true;
}

/*
============
arcMatX::Cholesky_Solve

  Solve Ax = b with A factored in-place as: LL'
============
*/
void arcMatX::Cholesky_Solve( arcVecX &x, const arcVecX &b ) const {
	int i, j;
	double sum;

	assert( numRows == numColumns );
	assert( x.GetSize() >= numRows && b.GetSize() >= numRows );

	// solve L
	for ( i = 0; i < numRows; i++ ) {
		sum = b[i];
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = sum / (*this)[i][i];
	}

	// solve Lt
	for ( i = numRows - 1; i >= 0; i-- ) {
		sum = x[i];
		for ( j = i + 1; j < numRows; j++ ) {
			sum -= (*this)[j][i] * x[j];
		}
		x[i] = sum / (*this)[i][i];
	}
}

/*
============
arcMatX::Cholesky_Inverse

  Calculates the inverse of the matrix which is factored in-place as: LL'
============
*/
void arcMatX::Cholesky_Inverse( arcMatX &inv ) const {
	int i, j;
	arcVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for ( i = 0; i < numRows; i++ ) {

		b[i] = 1.0f;
		Cholesky_Solve( x, b );
		for ( j = 0; j < numRows; j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
arcMatX::Cholesky_MultiplyFactors

  Multiplies the factors of the in-place Cholesky factorization to form the original matrix.
============
*/
void arcMatX::Cholesky_MultiplyFactors( arcMatX &m ) const {
	int r, i, j;
	double sum;

	m.SetSize( numRows, numColumns );

	for ( r = 0; r < numRows; r++ ) {

		// calculate row of matrix
		for ( i = 0; i < numRows; i++ ) {
			sum = 0.0f;
			for ( j = 0; j <= i && j <= r; j++ ) {
				sum += (*this)[r][j] * (*this)[i][j];
			}
			m[r][i] = sum;
		}
	}
}

/*
============
arcMatX::LDLT_Factor

  in-place factorization: LDL'
  L is a triangular matrix stored in the lower triangle.
  L has ones on the diagonal that are not stored.
  D is a diagonal matrix stored on the diagonal.
  The upper triangle is not cleared.
  The initial matrix has to be symmetric.
============
*/
bool arcMatX::LDLT_Factor( void ) {
	int i, j, k;
	float *v;
	double d, sum;

	assert( numRows == numColumns );

	v = (float *) _alloca16( numRows * sizeof( float ) );

	for ( i = 0; i < numRows; i++ ) {

		sum = (*this)[i][i];
		for ( j = 0; j < i; j++ ) {
			d = (*this)[i][j];
		    v[j] = (*this)[j][j] * d;
		    sum -= v[j] * d;
		}

		if ( sum == 0.0f ) {
			return false;
		}

		(*this)[i][i] = sum;
		d = 1.0f / sum;

		for ( j = i + 1; j < numRows; j++ ) {
		    sum = (*this)[j][i];
			for ( k = 0; k < i; k++ ) {
				sum -= (*this)[j][k] * v[k];
			}
		    (*this)[j][i] = sum * d;
		}
	}

	return true;
}

/*
============
arcMatX::LDLT_UpdateRankOne

  Updates the in-place LDL' factorization to obtain the factors for the matrix: LDL' + alpha * v * v'
  If offset > 0 only the lower right corner starting at (offset, offset) is updated.
============
*/
bool arcMatX::LDLT_UpdateRankOne( const arcVecX &v, float alpha, int offset ) {
	int i, j;
	float *y;
	double diag, newDiag, beta, p, d;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( offset >= 0 && offset < numRows );

	y = (float *) _alloca16( v.GetSize() * sizeof( float ) );
	memcpy( y, v.ToFloatPtr(), v.GetSize() * sizeof( float ) );

	for ( i = offset; i < numColumns; i++ ) {
		p = y[i];
		diag = (*this)[i][i];
		(*this)[i][i] = newDiag = diag + alpha * p * p;

		if ( newDiag == 0.0f ) {
			return false;
		}

		alpha /= newDiag;
		beta = p * alpha;
		alpha *= diag;

		for ( j = i+1; j < numRows; j++ ) {

			d = (*this)[j][i];

			y[j] -= p * d;
			d += beta * y[j];

			(*this)[j][i] = d;
		}
	}

	return true;
}

/*
============
arcMatX::LDLT_UpdateRowColumn

  Updates the in-place LDL' factorization to obtain the factors for the matrix:

         [ 0  a  0 ]
  LDL' + [ a  b  c ]
         [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1]
============
*/
bool arcMatX::LDLT_UpdateRowColumn( const arcVecX &v, int r ) {
	int i, j;
	double sum;
	float *original, *y;
	arcVecX addSub;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( r >= 0 && r < numRows );

	addSub.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );

	if ( r == 0 ) {

		if ( numColumns == 1 ) {
			(*this)[0][0] += v[0];
			return true;
		}
		for ( i = 0; i < numColumns; i++ ) {
			addSub[i] = v[i];
		}

	} else {

		original = (float *) _alloca16( numColumns * sizeof( float ) );
		y = (float *) _alloca16( numColumns * sizeof( float ) );

		// calculate original row/column of matrix
		for ( i = 0; i < r; i++ ) {
			y[i] = (*this)[r][i] * (*this)[i][i];
		}
		for ( i = 0; i < numColumns; i++ ) {
			if ( i < r ) {
				sum = (*this)[i][i] * (*this)[r][i];
			} else if ( i == r ) {
				sum = (*this)[r][r];
			} else {
				sum = (*this)[r][r] * (*this)[i][r];
			}
			for ( j = 0; j < i && j < r; j++ ) {
				sum += (*this)[i][j] * y[j];
			}
			original[i] = sum;
		}

		// solve for y in L * y = original + v
		for ( i = 0; i < r; i++ ) {
			sum = original[i] + v[i];
			for ( j = 0; j < i; j++ ) {
				sum -= (*this)[i][j] * y[j];
			}
			y[i] = sum;
		}

		// calculate new row of L
		for ( i = 0; i < r; i++ ) {
			(*this)[r][i] = y[i] / (*this)[i][i];
		}

		// if the last row/column of the matrix is updated
		if ( r == numColumns - 1 ) {
			// only calculate new diagonal
			sum = original[r] + v[r];
			for ( j = 0; j < r; j++ ) {
				sum -= (*this)[r][j] * y[j];
			}
			if ( sum == 0.0f ) {
				return false;
			}
			(*this)[r][r] = sum;
			return true;
		}

		// calculate the row/column to be added to the lower right sub matrix starting at (r, r)
		for ( i = 0; i < r; i++ ) {
			y[i] = (*this)[r][i] * (*this)[i][i];
		}
		for ( i = r; i < numColumns; i++ ) {
			if ( i == r ) {
				sum = (*this)[r][r];
			} else {
				sum = (*this)[r][r] * (*this)[i][r];
			}
			for ( j = 0; j < r; j++ ) {
				sum += (*this)[i][j] * y[j];
			}
			addSub[i] = v[i] - ( sum - original[i] );
		}
	}

	// add row/column to the lower right sub matrix starting at (r, r)

#if 0

	arcVecX v1, v2;
	double d;

	v1.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );
	v2.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );

	d = arcMath::SQRT_1OVER2;
	v1[r] = ( 0.5f * addSub[r] + 1.0f ) * d;
	v2[r] = ( 0.5f * addSub[r] - 1.0f ) * d;
	for ( i = r+1; i < numColumns; i++ ) {
		v1[i] = v2[i] = addSub[i] * d;
	}

	// update
	if ( !LDLT_UpdateRankOne( v1, 1.0f, r ) ) {
		return false;
	}
	// downdate
	if ( !LDLT_UpdateRankOne( v2, -1.0f, r ) ) {
		return false;
	}

#else

	float *v1, *v2;
	double d, diag, newDiag, p1, p2, alpha1, alpha2, beta1, beta2;

	v1 = (float *) _alloca16( numColumns * sizeof( float ) );
	v2 = (float *) _alloca16( numColumns * sizeof( float ) );

	d = arcMath::SQRT_1OVER2;
	v1[r] = ( 0.5f * addSub[r] + 1.0f ) * d;
	v2[r] = ( 0.5f * addSub[r] - 1.0f ) * d;
	for ( i = r+1; i < numColumns; i++ ) {
		v1[i] = v2[i] = addSub[i] * d;
	}

	alpha1 = 1.0f;
	alpha2 = -1.0f;

	// simultaneous update/downdate of the sub matrix starting at (r, r)
	for ( i = r; i < numColumns; i++ ) {

		diag = (*this)[i][i];
		p1 = v1[i];
		newDiag = diag + alpha1 * p1 * p1;

		if ( newDiag == 0.0f ) {
			return false;
		}

		alpha1 /= newDiag;
		beta1 = p1 * alpha1;
		alpha1 *= diag;

		diag = newDiag;
		p2 = v2[i];
		newDiag = diag + alpha2 * p2 * p2;

		if ( newDiag == 0.0f ) {
			return false;
		}

		alpha2 /= newDiag;
		beta2 = p2 * alpha2;
		alpha2 *= diag;

		(*this)[i][i] = newDiag;

		for ( j = i+1; j < numRows; j++ ) {

			d = (*this)[j][i];

			v1[j] -= p1 * d;
			d += beta1 * v1[j];

			v2[j] -= p2 * d;
			d += beta2 * v2[j];

			(*this)[j][i] = d;
		}
	}

#endif

	return true;
}

/*
============
arcMatX::LDLT_UpdateIncrement

  Updates the in-place LDL' factorization to obtain the factors for the matrix:

  [ A  a ]
  [ a  b ]

  where: a = v[0,numRows-1], b = v[numRows]
============
*/
bool arcMatX::LDLT_UpdateIncrement( const arcVecX &v ) {
	int i, j;
	float *x;
	double sum, d;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );

	ChangeSize( numRows+1, numColumns+1, false );

	x = (float *) _alloca16( numRows * sizeof( float ) );

	// solve for x in L * x = v
	for ( i = 0; i < numRows - 1; i++ ) {
		sum = v[i];
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = sum;
	}

	// calculate new row of L and calculate the diagonal entry
	sum = v[numRows - 1];
	for ( i = 0; i < numRows - 1; i++ ) {
		(*this)[numRows - 1][i] = d = x[i] / (*this)[i][i];
		sum -= d * x[i];
	}

	if ( sum == 0.0f ) {
		return false;
	}

	// store the diagonal entry
	(*this)[numRows - 1][numRows - 1] = sum;

	return true;
}

/*
============
arcMatX::LDLT_UpdateDecrement

  Updates the in-place LDL' factorization to obtain the factors for the matrix with row r and column r removed.
  v should store the row of the original matrix.
============
*/
bool arcMatX::LDLT_UpdateDecrement( const arcVecX &v, int r ) {
	arcVecX v1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( r >= 0 && r < numRows );

	v1.SetData( numRows, VECX_ALLOCA( numRows ) );

	// update the row and column to identity
	v1 = -v;
	v1[r] += 1.0f;

	// NOTE:	msvc compiler bug: the this pointer stored in edi is expected to stay
	//			untouched when calling LDLT_UpdateRowColumn in the if statement
#if 0
	if ( !LDLT_UpdateRowColumn( v1, r ) ) {
#else
	bool ret = LDLT_UpdateRowColumn( v1, r );
	if ( !ret ) {
#endif
		return false;
	}

	// physically remove the row and column
	Update_Decrement( r );

	return true;
}

/*
============
arcMatX::LDLT_Solve

  Solve Ax = b with A factored in-place as: LDL'
============
*/
void arcMatX::LDLT_Solve( arcVecX &x, const arcVecX &b ) const {
	int i, j;
	double sum;

	assert( numRows == numColumns );
	assert( x.GetSize() >= numRows && b.GetSize() >= numRows );

	// solve L
	for ( i = 0; i < numRows; i++ ) {
		sum = b[i];
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = sum;
	}

	// solve D
	for ( i = 0; i < numRows; i++ ) {
		x[i] /= (*this)[i][i];
	}

	// solve Lt
	for ( i = numRows - 2; i >= 0; i-- ) {
		sum = x[i];
		for ( j = i + 1; j < numRows; j++ ) {
			sum -= (*this)[j][i] * x[j];
		}
		x[i] = sum;
	}
}

/*
============
arcMatX::LDLT_Inverse

  Calculates the inverse of the matrix which is factored in-place as: LDL'
============
*/
void arcMatX::LDLT_Inverse( arcMatX &inv ) const {
	int i, j;
	arcVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for ( i = 0; i < numRows; i++ ) {

		b[i] = 1.0f;
		LDLT_Solve( x, b );
		for ( j = 0; j < numRows; j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
arcMatX::LDLT_UnpackFactors

  Unpacks the in-place LDL' factorization.
============
*/
void arcMatX::LDLT_UnpackFactors( arcMatX &L, arcMatX &D ) const {
	int i, j;

	L.Zero( numRows, numColumns );
	D.Zero( numRows, numColumns );
	for ( i = 0; i < numRows; i++ ) {
		for ( j = 0; j < i; j++ ) {
			L[i][j] = (*this)[i][j];
		}
		L[i][i] = 1.0f;
		D[i][i] = (*this)[i][i];
	}
}

/*
============
arcMatX::LDLT_MultiplyFactors

  Multiplies the factors of the in-place LDL' factorization to form the original matrix.
============
*/
void arcMatX::LDLT_MultiplyFactors( arcMatX &m ) const {
	int r, i, j;
	float *v;
	double sum;

	v = (float *) _alloca16( numRows * sizeof( float ) );
	m.SetSize( numRows, numColumns );

	for ( r = 0; r < numRows; r++ ) {

		// calculate row of matrix
		for ( i = 0; i < r; i++ ) {
			v[i] = (*this)[r][i] * (*this)[i][i];
		}
		for ( i = 0; i < numColumns; i++ ) {
			if ( i < r ) {
				sum = (*this)[i][i] * (*this)[r][i];
			} else if ( i == r ) {
				sum = (*this)[r][r];
			} else {
				sum = (*this)[r][r] * (*this)[i][r];
			}
			for ( j = 0; j < i && j < r; j++ ) {
				sum += (*this)[i][j] * v[j];
			}
			m[r][i] = sum;
		}
	}
}

/*
============
arcMatX::TriDiagonal_ClearTriangles
============
*/
void arcMatX::TriDiagonal_ClearTriangles( void ) {
	int i, j;

	assert( numRows == numColumns );
	for ( i = 0; i < numRows-2; i++ ) {
		for ( j = i+2; j < numColumns; j++ ) {
			(*this)[i][j] = 0.0f;
			(*this)[j][i] = 0.0f;
		}
	}
}

/*
============
arcMatX::TriDiagonal_Solve

  Solve Ax = b with A being tridiagonal.
============
*/
bool arcMatX::TriDiagonal_Solve( arcVecX &x, const arcVecX &b ) const {
	int i;
	float d;
	arcVecX tmp;

	assert( numRows == numColumns );
	assert( x.GetSize() >= numRows && b.GetSize() >= numRows );

	tmp.SetData( numRows, VECX_ALLOCA( numRows ) );

	d = (*this)[0][0];
	if ( d == 0.0f ) {
		return false;
	}
	d = 1.0f / d;
	x[0] = b[0] * d;
	for ( i = 1; i < numRows; i++ ) {
		tmp[i] = (*this)[i-1][i] * d;
		d = (*this)[i][i] - (*this)[i][i-1] * tmp[i];
		if ( d == 0.0f ) {
			return false;
		}
		d = 1.0f / d;
		x[i] = ( b[i] - (*this)[i][i-1] * x[i-1] ) * d;
	}
	for ( i = numRows - 2; i >= 0; i-- ) {
		x[i] -= tmp[i+1] * x[i+1];
	}
	return true;
}

/*
============
arcMatX::TriDiagonal_Inverse

  Calculates the inverse of a tri-diagonal matrix.
============
*/
void arcMatX::TriDiagonal_Inverse( arcMatX &inv ) const {
	int i, j;
	arcVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for ( i = 0; i < numRows; i++ ) {

		b[i] = 1.0f;
		TriDiagonal_Solve( x, b );
		for ( j = 0; j < numRows; j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
arcMatX::HouseholderReduction

  Householder reduction to symmetric tri-diagonal form.
  The original matrix is replaced by an orthogonal matrix effecting the accumulated householder transformations.
  The diagonal elements of the diagonal matrix are stored in diag.
  The off-diagonal elements of the diagonal matrix are stored in subd.
  The initial matrix has to be symmetric.
============
*/
void arcMatX::HouseholderReduction( arcVecX &diag, arcVecX &subd ) {
	int i0, i1, i2, i3;
	float h, f, g, invH, halfFdivH, scale, invScale, sum;

	assert( numRows == numColumns );

	diag.SetSize( numRows );
	subd.SetSize( numRows );

	for ( i0 = numRows-1, i3 = numRows-2; i0 >= 1; i0--, i3-- ) {
		h = 0.0f;
		scale = 0.0f;

		if ( i3 > 0 ) {
			for ( i2 = 0; i2 <= i3; i2++ ) {
				scale += arcMath::Fabs( (*this)[i0][i2] );
			}
			if ( scale == 0 ) {
				subd[i0] = (*this)[i0][i3];
			} else {
				invScale = 1.0f / scale;
				for (i2 = 0; i2 <= i3; i2++ )
				{
					(*this)[i0][i2] *= invScale;
					h += (*this)[i0][i2] * (*this)[i0][i2];
				}
				f = (*this)[i0][i3];
				g = arcMath::Sqrt( h );
				if ( f > 0.0f ) {
					g = -g;
				}
				subd[i0] = scale * g;
				h -= f * g;
				(*this)[i0][i3] = f - g;
				f = 0.0f;
				invH = 1.0f / h;
				for (i1 = 0; i1 <= i3; i1++ ) {
					(*this)[i1][i0] = (*this)[i0][i1] * invH;
					g = 0.0f;
					for (i2 = 0; i2 <= i1; i2++ ) {
						g += (*this)[i1][i2] * (*this)[i0][i2];
					}
					for (i2 = i1+1; i2 <= i3; i2++ ) {
						g += (*this)[i2][i1] * (*this)[i0][i2];
					}
					subd[i1] = g * invH;
					f += subd[i1] * (*this)[i0][i1];
				}
				halfFdivH = 0.5f * f * invH;
				for ( i1 = 0; i1 <= i3; i1++ ) {
					f = (*this)[i0][i1];
					g = subd[i1] - halfFdivH * f;
					subd[i1] = g;
					for ( i2 = 0; i2 <= i1; i2++ ) {
						(*this)[i1][i2] -= f * subd[i2] + g * (*this)[i0][i2];
					}
				}
            }
		} else {
			subd[i0] = (*this)[i0][i3];
		}

		diag[i0] = h;
	}

	diag[0] = 0.0f;
	subd[0] = 0.0f;
	for ( i0 = 0, i3 = -1; i0 <= numRows-1; i0++, i3++ ) {
		if ( diag[i0] ) {
			for ( i1 = 0; i1 <= i3; i1++ ) {
				sum = 0.0f;
				for (i2 = 0; i2 <= i3; i2++ ) {
					sum += (*this)[i0][i2] * (*this)[i2][i1];
				}
				for ( i2 = 0; i2 <= i3; i2++ ) {
					(*this)[i2][i1] -= sum * (*this)[i2][i0];
				}
			}
		}
		diag[i0] = (*this)[i0][i0];
		(*this)[i0][i0] = 1.0f;
		for ( i1 = 0; i1 <= i3; i1++ ) {
			(*this)[i1][i0] = 0.0f;
			(*this)[i0][i1] = 0.0f;
		}
	}

	// re-order
	for ( i0 = 1, i3 = 0; i0 < numRows; i0++, i3++ ) {
		subd[i3] = subd[i0];
	}
	subd[numRows-1] = 0.0f;
}

/*
============
arcMatX::QL

  QL algorithm with implicit shifts to determine the eigenvalues and eigenvectors of a symmetric tri-diagonal matrix.
  diag contains the diagonal elements of the symmetric tri-diagonal matrix on input and is overwritten with the eigenvalues.
  subd contains the off-diagonal elements of the symmetric tri-diagonal matrix and is destroyed.
  This matrix has to be either the identity matrix to determine the eigenvectors for a symmetric tri-diagonal matrix,
  or the matrix returned by the Householder reduction to determine the eigenvalues for the original symmetric matrix.
============
*/
bool arcMatX::QL( arcVecX &diag, arcVecX &subd ) {
    const int maxIter = 32;
	int i0, i1, i2, i3;
	float a, b, f, g, r, p, s, c;

	assert( numRows == numColumns );

	for ( i0 = 0; i0 < numRows; i0++ ) {
		for ( i1 = 0; i1 < maxIter; i1++ ) {
			for ( i2 = i0; i2 <= numRows - 2; i2++ ) {
				a = arcMath::Fabs( diag[i2] ) + arcMath::Fabs( diag[i2+1] );
				if ( arcMath::Fabs( subd[i2] ) + a == a ) {
					break;
				}
			}
			if ( i2 == i0 ) {
				break;
			}

			g = ( diag[i0+1] - diag[i0] ) / ( 2.0f * subd[i0] );
			r = arcMath::Sqrt( g * g + 1.0f );
			if ( g < 0.0f ) {
				g = diag[i2] - diag[i0] + subd[i0] / ( g - r );
			} else {
				g = diag[i2] - diag[i0] + subd[i0] / ( g + r );
			}
			s = 1.0f;
			c = 1.0f;
			p = 0.0f;
			for ( i3 = i2 - 1; i3 >= i0; i3-- ) {
				f = s * subd[i3];
				b = c * subd[i3];
				if ( arcMath::Fabs( f ) >= arcMath::Fabs( g ) ) {
					c = g / f;
					r = arcMath::Sqrt( c * c + 1.0f );
					subd[i3+1] = f * r;
					s = 1.0f / r;
					c *= s;
				} else {
					s = f / g;
					r = arcMath::Sqrt( s * s + 1.0f );
					subd[i3+1] = g * r;
					c = 1.0f / r;
					s *= c;
				}
				g = diag[i3+1] - p;
				r = ( diag[i3] - g ) * s + 2.0f * b * c;
				p = s * r;
				diag[i3+1] = g + p;
				g = c * r - b;

				for ( int i4 = 0; i4 < numRows; i4++ ) {
					f = (*this)[i4][i3+1];
					(*this)[i4][i3+1] = s * (*this)[i4][i3] + c * f;
					(*this)[i4][i3] = c * (*this)[i4][i3] - s * f;
				}
			}
			diag[i0] -= p;
			subd[i0] = g;
			subd[i2] = 0.0f;
		}
		if ( i1 == maxIter ) {
			return false;
		}
	}
	return true;
}

/*
============
arcMatX::Eigen_SolveSymmetricTriDiagonal

  Determine eigen values and eigen vectors for a symmetric tri-diagonal matrix.
  The eigen values are stored in 'eigenValues'.
  Column i of the original matrix will store the eigen vector corresponding to the eigenValues[i].
  The initial matrix has to be symmetric tri-diagonal.
============
*/
bool arcMatX::Eigen_SolveSymmetricTriDiagonal( arcVecX &eigenValues ) {
	int i;
	arcVecX subd;

	assert( numRows == numColumns );

	subd.SetData( numRows, VECX_ALLOCA( numRows ) );
	eigenValues.SetSize( numRows );

	for ( i = 0; i < numRows-1; i++ ) {
		eigenValues[i] = (*this)[i][i];
		subd[i] = (*this)[i+1][i];
	}
	eigenValues[numRows-1] = (*this)[numRows-1][numRows-1];

	Identity();

	return QL( eigenValues, subd );
}

/*
============
arcMatX::Eigen_SolveSymmetric

  Determine eigen values and eigen vectors for a symmetric matrix.
  The eigen values are stored in 'eigenValues'.
  Column i of the original matrix will store the eigen vector corresponding to the eigenValues[i].
  The initial matrix has to be symmetric.
============
*/
bool arcMatX::Eigen_SolveSymmetric( arcVecX &eigenValues ) {
	arcVecX subd;

	assert( numRows == numColumns );

	subd.SetData( numRows, VECX_ALLOCA( numRows ) );
	eigenValues.SetSize( numRows );

	HouseholderReduction( eigenValues, subd );
	return QL( eigenValues, subd );
}

/*
============
arcMatX::HessenbergReduction

  Reduction to Hessenberg form.
============
*/
void arcMatX::HessenbergReduction( arcMatX &H ) {
	int i, j, m;
	int low = 0;
	int high = numRows - 1;
	float scale, f, g, h;
	arcVecX v;

	v.SetData( numRows, VECX_ALLOCA( numRows ) );

	for ( m = low + 1; m <= high - 1; m++ ) {

		scale = 0.0f;
		for ( i = m; i <= high; i++ ) {
			scale = scale + arcMath::Fabs( H[i][m-1] );
		}
		if ( scale != 0.0f ) {

			// compute Householder transformation.
			h = 0.0f;
			for ( i = high; i >= m; i-- ) {
				v[i] = H[i][m-1] / scale;
				h += v[i] * v[i];
			}
			g = arcMath::Sqrt( h );
			if ( v[m] > 0.0f ) {
				g = -g;
			}
			h = h - v[m] * g;
			v[m] = v[m] - g;

			// apply Householder similarity transformation
			// H = (I-u*u'/h)*H*(I-u*u')/h)
			for ( j = m; j < numRows; j++ ) {
				f = 0.0f;
				for ( i = high; i >= m; i-- ) {
					f += v[i] * H[i][j];
				}
				f = f / h;
				for ( i = m; i <= high; i++ ) {
					H[i][j] -= f * v[i];
				}
			}

			for ( i = 0; i <= high; i++ ) {
				f = 0.0f;
				for ( j = high; j >= m; j-- ) {
					f += v[j] * H[i][j];
				}
				f = f / h;
				for ( j = m; j <= high; j++ ) {
					H[i][j] -= f * v[j];
				}
			}
			v[m] = scale * v[m];
			H[m][m-1] = scale * g;
		}
	}

	// accumulate transformations
	Identity();
	for ( int m = high - 1; m >= low + 1; m-- ) {
		if ( H[m][m-1] != 0.0f ) {
			for ( i = m + 1; i <= high; i++ ) {
				v[i] = H[i][m-1];
			}
			for ( j = m; j <= high; j++ ) {
				g = 0.0f;
				for ( i = m; i <= high; i++ ) {
					g += v[i] * (*this)[i][j];
				}
				// float division to avoid possible underflow
				g = ( g / v[m] ) / H[m][m-1];
				for ( i = m; i <= high; i++ ) {
					(*this)[i][j] += g * v[i];
				}
			}
		}
	}
}

/*
============
arcMatX::ComplexDivision

  Complex scalar division.
============
*/
void arcMatX::ComplexDivision( float xr, float xi, float yr, float yi, float &cdivr, float &cdivi ) {
	float r, d;
	if ( arcMath::Fabs( yr ) > arcMath::Fabs( yi ) ) {
		r = yi / yr;
		d = yr + r * yi;
		cdivr = ( xr + r * xi ) / d;
		cdivi = ( xi - r * xr ) / d;
	} else {
		r = yr / yi;
		d = yi + r * yr;
		cdivr = ( r * xr + xi ) / d;
		cdivi = ( r * xi - xr ) / d;
	}
}

/*
============
arcMatX::HessenbergToRealSchur

  Reduction from Hessenberg to real Schur form.
============
*/
bool arcMatX::HessenbergToRealSchur( arcMatX &H, arcVecX &realEigenValues, arcVecX &imaginaryEigenValues ) {
	int i, j, k;
	int n = numRows - 1;
	int low = 0;
	int high = numRows - 1;
	float eps = 2e-16f, exshift = 0.0f;
	float p = 0.0f, q = 0.0f, r = 0.0f, s = 0.0f, z = 0.0f, t, w, x, y;

	// store roots isolated by balanc and compute matrix norm
	float norm = 0.0f;
	for ( i = 0; i < numRows; i++ ) {
		if ( i < low || i > high ) {
			realEigenValues[i] = H[i][i];
			imaginaryEigenValues[i] = 0.0f;
		}
		for ( j = Max( i - 1, 0 ); j < numRows; j++ ) {
			norm = norm + arcMath::Fabs( H[i][j] );
		}
	}

	int iter = 0;
	while( n >= low ) {

		// look for single small sub-diagonal element
		int l = n;
		while ( l > low ) {
			s = arcMath::Fabs( H[l-1][l-1] ) + arcMath::Fabs( H[l][l] );
			if ( s == 0.0f ) {
				s = norm;
			}
			if ( arcMath::Fabs( H[l][l-1] ) < eps * s ) {
				break;
			}
			l--;
		}

		// check for convergence
		if ( l == n ) {			// one root found
			H[n][n] = H[n][n] + exshift;
			realEigenValues[n] = H[n][n];
			imaginaryEigenValues[n] = 0.0f;
			n--;
			iter = 0;
		} else if ( l == n-1 ) {	// two roots found
			w = H[n][n-1] * H[n-1][n];
			p = ( H[n-1][n-1] - H[n][n] ) / 2.0f;
			q = p * p + w;
			z = arcMath::Sqrt( arcMath::Fabs( q ) );
			H[n][n] = H[n][n] + exshift;
			H[n-1][n-1] = H[n-1][n-1] + exshift;
			x = H[n][n];

			if ( q >= 0.0f ) {		// real pair
				if ( p >= 0.0f ) {
					z = p + z;
				} else {
					z = p - z;
				}
				realEigenValues[n-1] = x + z;
				realEigenValues[n] = realEigenValues[n-1];
				if ( z != 0.0f ) {
					realEigenValues[n] = x - w / z;
				}
				imaginaryEigenValues[n-1] = 0.0f;
				imaginaryEigenValues[n] = 0.0f;
				x = H[n][n-1];
				s = arcMath::Fabs( x ) + arcMath::Fabs( z );
				p = x / s;
				q = z / s;
				r = arcMath::Sqrt( p * p + q * q );
				p = p / r;
				q = q / r;

				// modify row
				for ( j = n-1; j < numRows; j++ ) {
					z = H[n-1][j];
					H[n-1][j] = q * z + p * H[n][j];
					H[n][j] = q * H[n][j] - p * z;
				}

				// modify column
				for ( i = 0; i <= n; i++ ) {
					z = H[i][n-1];
					H[i][n-1] = q * z + p * H[i][n];
					H[i][n] = q * H[i][n] - p * z;
				}

				// accumulate transformations
				for ( i = low; i <= high; i++ ) {
					z = (*this)[i][n-1];
					(*this)[i][n-1] = q * z + p * (*this)[i][n];
					(*this)[i][n] = q * (*this)[i][n] - p * z;
				}
			} else {		// complex pair
				realEigenValues[n-1] = x + p;
				realEigenValues[n] = x + p;
				imaginaryEigenValues[n-1] = z;
				imaginaryEigenValues[n] = -z;
			}
			n = n - 2;
			iter = 0;

		} else {	// no convergence yet

			// form shift
			x = H[n][n];
			y = 0.0f;
			w = 0.0f;
			if ( l < n ) {
				y = H[n-1][n-1];
				w = H[n][n-1] * H[n-1][n];
			}

			// Wilkinson's original ad hoc shift
			if ( iter == 10 ) {
				exshift += x;
				for ( i = low; i <= n; i++ ) {
					H[i][i] -= x;
				}
				s = arcMath::Fabs( H[n][n-1] ) + arcMath::Fabs( H[n-1][n-2] );
				x = y = 0.75f * s;
				w = -0.4375f * s * s;
			}

			// new ad hoc shift
			if ( iter == 30 ) {
				s = ( y - x ) / 2.0f;
				s = s * s + w;
				if ( s > 0 ) {
					s = arcMath::Sqrt( s );
					if ( y < x ) {
						s = -s;
					}
					s = x - w / ( ( y - x ) / 2.0f + s );
					for ( i = low; i <= n; i++ ) {
						H[i][i] -= s;
					}
					exshift += s;
					x = y = w = 0.964f;
				}
			}

			iter = iter + 1;

			// look for two consecutive small sub-diagonal elements
			int m;
			for ( m = n-2; m >= l; m-- ) {
				z = H[m][m];
				r = x - z;
				s = y - z;
				p = ( r * s - w ) / H[m+1][m] + H[m][m+1];
				q = H[m+1][m+1] - z - r - s;
				r = H[m+2][m+1];
				s = arcMath::Fabs( p ) + arcMath::Fabs( q ) + arcMath::Fabs( r );
				p = p / s;
				q = q / s;
				r = r / s;
				if ( m == l ) {
					break;
				}
				if ( arcMath::Fabs( H[m][m-1] ) * ( arcMath::Fabs( q ) + arcMath::Fabs( r ) ) <
						eps * ( arcMath::Fabs( p ) * ( arcMath::Fabs( H[m-1][m-1] ) + arcMath::Fabs( z ) + arcMath::Fabs( H[m+1][m+1] ) ) ) ) {
					break;
				}
			}

			for ( i = m+2; i <= n; i++ ) {
				H[i][i-2] = 0.0f;
				if ( i > m+2 ) {
					H[i][i-3] = 0.0f;
				}
			}

			// double QR step involving rows l:n and columns m:n
			for ( k = m; k <= n-1; k++ ) {
				bool notlast = ( k != n-1 );
				if ( k != m ) {
					p = H[k][k-1];
					q = H[k+1][k-1];
					r = ( notlast ? H[k+2][k-1] : 0.0f );
					x = arcMath::Fabs( p ) + arcMath::Fabs( q ) + arcMath::Fabs( r );
					if ( x != 0.0f ) {
						p = p / x;
						q = q / x;
						r = r / x;
					}
				}
				if ( x == 0.0f ) {
					break;
				}
				s = arcMath::Sqrt( p * p + q * q + r * r );
				if ( p < 0.0f ) {
					s = -s;
				}
				if ( s != 0.0f ) {
					if ( k != m ) {
						H[k][k-1] = -s * x;
					} else if ( l != m ) {
						H[k][k-1] = -H[k][k-1];
					}
					p = p + s;
					x = p / s;
					y = q / s;
					z = r / s;
					q = q / p;
					r = r / p;

					// modify row
					for ( j = k; j < numRows; j++ ) {
						p = H[k][j] + q * H[k+1][j];
						if ( notlast ) {
							p = p + r * H[k+2][j];
							H[k+2][j] = H[k+2][j] - p * z;
						}
						H[k][j] = H[k][j] - p * x;
						H[k+1][j] = H[k+1][j] - p * y;
					}

					// modify column
					for ( i = 0; i <= Min( n, k + 3 ); i++ ) {
						p = x * H[i][k] + y * H[i][k+1];
						if ( notlast ) {
							p = p + z * H[i][k+2];
							H[i][k+2] = H[i][k+2] - p * r;
						}
						H[i][k] = H[i][k] - p;
						H[i][k+1] = H[i][k+1] - p * q;
					}

					// accumulate transformations
					for ( i = low; i <= high; i++ ) {
						p = x * (*this)[i][k] + y * (*this)[i][k+1];
						if ( notlast ) {
							p = p + z * (*this)[i][k+2];
							(*this)[i][k+2] = (*this)[i][k+2] - p * r;
						}
						(*this)[i][k] = (*this)[i][k] - p;
						(*this)[i][k+1] = (*this)[i][k+1] - p * q;
					}
				}
			}
		}
	}

	// backsubstitute to find vectors of upper triangular form
	if ( norm == 0.0f ) {
		return false;
	}

	for ( n = numRows-1; n >= 0; n-- ) {
		p = realEigenValues[n];
		q = imaginaryEigenValues[n];

		if ( q == 0.0f ) {		// real vector
			int l = n;
			H[n][n] = 1.0f;
			for ( i = n-1; i >= 0; i-- ) {
				w = H[i][i] - p;
				r = 0.0f;
				for ( j = l; j <= n; j++ ) {
					r = r + H[i][j] * H[j][n];
				}
				if ( imaginaryEigenValues[i] < 0.0f ) {
					z = w;
					s = r;
				} else {
					l = i;
					if ( imaginaryEigenValues[i] == 0.0f ) {
						if ( w != 0.0f ) {
							H[i][n] = -r / w;
						} else {
							H[i][n] = -r / ( eps * norm );
						}
					} else {		// solve real equations
						x = H[i][i+1];
						y = H[i+1][i];
						q = ( realEigenValues[i] - p ) * ( realEigenValues[i] - p ) + imaginaryEigenValues[i] * imaginaryEigenValues[i];
						t = ( x * s - z * r ) / q;
						H[i][n] = t;
						if ( arcMath::Fabs(x) > arcMath::Fabs( z ) ) {
							H[i+1][n] = ( -r - w * t ) / x;
						} else {
							H[i+1][n] = ( -s - y * t ) / z;
						}
					}

					// overflow control
					t = arcMath::Fabs(H[i][n] );
					if ( ( eps * t ) * t > 1 ) {
						for ( j = i; j <= n; j++ ) {
							H[j][n] = H[j][n] / t;
						}
					}
				}
			}
		} else if ( q < 0.0f ) {	// complex vector
			int l = n-1;

			// last vector component imaginary so matrix is triangular
			if ( arcMath::Fabs( H[n][n-1] ) > arcMath::Fabs( H[n-1][n] ) ) {
				H[n-1][n-1] = q / H[n][n-1];
				H[n-1][n] = -( H[n][n] - p ) / H[n][n-1];
			} else {
				ComplexDivision( 0.0f, -H[n-1][n], H[n-1][n-1]-p, q, H[n-1][n-1], H[n-1][n] );
			}
			H[n][n-1] = 0.0f;
			H[n][n] = 1.0f;
			for ( i = n-2; i >= 0; i-- ) {
				float ra, sa, vr, vi;
				ra = 0.0f;
				sa = 0.0f;
				for ( j = l; j <= n; j++ ) {
					ra = ra + H[i][j] * H[j][n-1];
					sa = sa + H[i][j] * H[j][n];
				}
				w = H[i][i] - p;

				if ( imaginaryEigenValues[i] < 0.0f ) {
					z = w;
					r = ra;
					s = sa;
				} else {
					l = i;
					if ( imaginaryEigenValues[i] == 0.0f ) {
						ComplexDivision( -ra, -sa, w, q, H[i][n-1], H[i][n] );
					} else {
						// solve complex equations
						x = H[i][i+1];
						y = H[i+1][i];
						vr = ( realEigenValues[i] - p ) * ( realEigenValues[i] - p ) + imaginaryEigenValues[i] * imaginaryEigenValues[i] - q * q;
						vi = ( realEigenValues[i] - p ) * 2.0f * q;
						if ( vr == 0.0f && vi == 0.0f ) {
							vr = eps * norm * ( arcMath::Fabs( w ) + arcMath::Fabs( q ) + arcMath::Fabs( x ) + arcMath::Fabs( y ) + arcMath::Fabs( z ) );
						}
						ComplexDivision( x * r - z * ra + q * sa, x * s - z * sa - q * ra, vr, vi, H[i][n-1], H[i][n] );
						if ( arcMath::Fabs( x ) > ( arcMath::Fabs( z ) + arcMath::Fabs( q ) ) ) {
							H[i+1][n-1] = ( -ra - w * H[i][n-1] + q * H[i][n] ) / x;
							H[i+1][n] = ( -sa - w * H[i][n] - q * H[i][n-1] ) / x;
						} else {
							ComplexDivision( -r - y * H[i][n-1], -s - y * H[i][n], z, q, H[i+1][n-1], H[i+1][n] );
						}
					}

					// overflow control
					t = Max( arcMath::Fabs( H[i][n-1] ), arcMath::Fabs( H[i][n] ) );
					if ( ( eps * t ) * t > 1 ) {
						for ( j = i; j <= n; j++ ) {
							H[j][n-1] = H[j][n-1] / t;
							H[j][n] = H[j][n] / t;
						}
					}
				}
			}
		}
	}

	// vectors of isolated roots
	for ( i = 0; i < numRows; i++ ) {
		if ( i < low || i > high ) {
			for ( j = i; j < numRows; j++ ) {
				(*this)[i][j] = H[i][j];
			}
		}
	}

	// back transformation to get eigenvectors of original matrix
	for ( j = numRows - 1; j >= low; j-- ) {
		for ( i = low; i <= high; i++ ) {
			z = 0.0f;
			for ( k = low; k <= Min( j, high ); k++ ) {
				z = z + (*this)[i][k] * H[k][j];
			}
			(*this)[i][j] = z;
		}
	}

	return true;
}

/*
============
arcMatX::Eigen_Solve

  Determine eigen values and eigen vectors for a square matrix.
  The eigen values are stored in 'realEigenValues' and 'imaginaryEigenValues'.
  Column i of the original matrix will store the eigen vector corresponding to the realEigenValues[i] and imaginaryEigenValues[i].
============
*/
bool arcMatX::Eigen_Solve( arcVecX &realEigenValues, arcVecX &imaginaryEigenValues ) {
    arcMatX H;

	assert( numRows == numColumns );

	realEigenValues.SetSize( numRows );
	imaginaryEigenValues.SetSize( numRows );

	H = *this;

    // reduce to Hessenberg form
    HessenbergReduction( H );

    // reduce Hessenberg to real Schur form
    return HessenbergToRealSchur( H, realEigenValues, imaginaryEigenValues );
}

/*
============
arcMatX::Eigen_SortIncreasing
============
*/
void arcMatX::Eigen_SortIncreasing( arcVecX &eigenValues ) {
	int i, j, k;
	float min;

	for ( i = 0, j; i <= numRows - 2; i++ ) {
		j = i;
		min = eigenValues[j];
		for ( k = i + 1; k < numRows; k++ ) {
			if ( eigenValues[k] < min ) {
				j = k;
				min = eigenValues[j];
			}
		}
		if ( j != i ) {
			eigenValues.SwapElements( i, j );
			SwapColumns( i, j );
		}
	}
}

/*
============
arcMatX::Eigen_SortDecreasing
============
*/
void arcMatX::Eigen_SortDecreasing( arcVecX &eigenValues ) {
	int i, j, k;
	float max;

	for ( i = 0, j; i <= numRows - 2; i++ ) {
		j = i;
		max = eigenValues[j];
		for ( k = i + 1; k < numRows; k++ ) {
			if ( eigenValues[k] > max ) {
				j = k;
				max = eigenValues[j];
			}
		}
		if ( j != i ) {
			eigenValues.SwapElements( i, j );
			SwapColumns( i, j );
		}
	}
}

/*
============
arcMatX::DeterminantGeneric
============
*/
float arcMatX::DeterminantGeneric( void ) const {
	int *index;
	float det;
	arcMatX tmp;

	index = ( int * ) _alloca16( numRows * sizeof( int ) );
	tmp.SetData( numRows, numColumns, MATX_ALLOCA( numRows * numColumns ) );
	tmp = *this;

	if ( !tmp.LU_Factor( index, &det ) ) {
		return 0.0f;
	}

	return det;
}

/*
============
arcMatX::InverseSelfGeneric
============
*/
bool arcMatX::InverseSelfGeneric( void ) {
	int i, j, *index;
	arcMatX tmp;
	arcVecX x, b;

	index = ( int * ) _alloca16( numRows * sizeof( int ) );
	tmp.SetData( numRows, numColumns, MATX_ALLOCA( numRows * numColumns ) );
	tmp = *this;

	if ( !tmp.LU_Factor( index ) ) {
		return false;
	}

	x.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.SetData( numRows, VECX_ALLOCA( numRows ) );
	b.Zero();

	for ( i = 0; i < numRows; i++ ) {

		b[i] = 1.0f;
		tmp.LU_Solve( x, b, index );
		for ( j = 0; j < numRows; j++ ) {
			(*this)[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
	return true;
}

/*
============
arcMatX::Test
============
*/
void arcMatX::Test( void ) {
	arcMatX original, m1, m2, m3, q1, q2, r1, r2;
	arcVecX v, w, u, c, d;
	int offset, size, *index1, *index2;

	size = 6;
	original.Random( size, size, 0 );
	original = original * original.Transpose();

	index1 = ( int * ) _alloca16( ( size + 1 ) * sizeof( index1[0] ) );
	index2 = ( int * ) _alloca16( ( size + 1 ) * sizeof( index2[0] ) );

	/*
		arcMatX::LowerTriangularInverse
	*/

	m1 = original;
	m1.ClearUpperTriangle();
	m2 = m1;

	m2.InverseSelf();
	m1.LowerTriangularInverse();

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LowerTriangularInverse failed" );
	}

	/*
		arcMatX::UpperTriangularInverse
	*/

	m1 = original;
	m1.ClearLowerTriangle();
	m2 = m1;

	m2.InverseSelf();
	m1.UpperTriangularInverse();

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::UpperTriangularInverse failed" );
	}

	/*
		arcMatX::Inverse_GaussJordan
	*/

	m1 = original;

	m1.Inverse_GaussJordan();
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Inverse_GaussJordan failed" );
	}

	/*
		arcMatX::Inverse_UpdateRankOne
	*/

	m1 = original;
	m2 = original;

	w.Random( size, 1 );
	v.Random( size, 2 );

	// invert m1
	m1.Inverse_GaussJordan();

	// modify and invert m2
	m2.Update_RankOne( v, w, 1.0f );
	if ( !m2.Inverse_GaussJordan() ) {
		assert( 0 );
	}

	// update inverse of m1
	m1.Inverse_UpdateRankOne( v, w, 1.0f );

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Inverse_UpdateRankOne failed" );
	}

	/*
		arcMatX::Inverse_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.Random( size, 1 );
		w.Random( size, 2 );
		w[offset] = 0.0f;

		// invert m1
		m1.Inverse_GaussJordan();

		// modify and invert m2
		m2.Update_RowColumn( v, w, offset );
		if ( !m2.Inverse_GaussJordan() ) {
			assert( 0 );
		}

		// update inverse of m1
		m1.Inverse_UpdateRowColumn( v, w, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::Inverse_UpdateRowColumn failed" );
		}
	}

	/*
		arcMatX::Inverse_UpdateIncrement
	*/

	m1 = original;
	m2 = original;

	v.Random( size + 1, 1 );
	w.Random( size + 1, 2 );
	w[size] = 0.0f;

	// invert m1
	m1.Inverse_GaussJordan();

	// modify and invert m2
	m2.Update_Increment( v, w );
	if ( !m2.Inverse_GaussJordan() ) {
		assert( 0 );
	}

	// update inverse of m1
	m1.Inverse_UpdateIncrement( v, w );

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Inverse_UpdateIncrement failed" );
	}

	/*
		arcMatX::Inverse_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		w.SetSize( 6 );
		for ( int i = 0; i < size; i++ ) {
			v[i] = original[i][offset];
			w[i] = original[offset][i];
		}

		// invert m1
		m1.Inverse_GaussJordan();

		// modify and invert m2
		m2.Update_Decrement( offset );
		if ( !m2.Inverse_GaussJordan() ) {
			assert( 0 );
		}

		// update inverse of m1
		m1.Inverse_UpdateDecrement( v, w, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::Inverse_UpdateDecrement failed" );
		}
	}

	/*
		arcMatX::LU_Factor
	*/

	m1 = original;

	m1.LU_Factor( NULL );	// no pivoting
	m1.LU_UnpackFactors( m2, m3 );
	m1 = m2 * m3;

	if ( !original.Compare( m1, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LU_Factor failed" );
	}

	/*
		arcMatX::LU_UpdateRankOne
	*/

	m1 = original;
	m2 = original;

	w.Random( size, 1 );
	v.Random( size, 2 );

	// factor m1
	m1.LU_Factor( index1 );

	// modify and factor m2
	m2.Update_RankOne( v, w, 1.0f );
	if ( !m2.LU_Factor( index2 ) ) {
		assert( 0 );
	}
	m2.LU_MultiplyFactors( m3, index2 );
	m2 = m3;

	// update factored m1
	m1.LU_UpdateRankOne( v, w, 1.0f, index1 );
	m1.LU_MultiplyFactors( m3, index1 );
	m1 = m3;

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LU_UpdateRankOne failed" );
	}

	/*
		arcMatX::LU_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.Random( size, 1 );
		w.Random( size, 2 );
		w[offset] = 0.0f;

		// factor m1
		m1.LU_Factor( index1 );

		// modify and factor m2
		m2.Update_RowColumn( v, w, offset );
		if ( !m2.LU_Factor( index2 ) ) {
			assert( 0 );
		}
		m2.LU_MultiplyFactors( m3, index2 );
		m2 = m3;

		// update m1
		m1.LU_UpdateRowColumn( v, w, offset, index1  );
		m1.LU_MultiplyFactors( m3, index1 );
		m1 = m3;

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::LU_UpdateRowColumn failed" );
		}
	}

	/*
		arcMatX::LU_UpdateIncrement
	*/

	m1 = original;
	m2 = original;

	v.Random( size + 1, 1 );
	w.Random( size + 1, 2 );
	w[size] = 0.0f;

	// factor m1
	m1.LU_Factor( index1 );

	// modify and factor m2
	m2.Update_Increment( v, w );
	if ( !m2.LU_Factor( index2 ) ) {
		assert( 0 );
	}
	m2.LU_MultiplyFactors( m3, index2 );
	m2 = m3;

	// update factored m1
	m1.LU_UpdateIncrement( v, w, index1 );
	m1.LU_MultiplyFactors( m3, index1 );
	m1 = m3;

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LU_UpdateIncrement failed" );
	}

	/*
		arcMatX::LU_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		w.SetSize( 6 );
		for ( int i = 0; i < size; i++ ) {
			v[i] = original[i][offset];
			w[i] = original[offset][i];
		}

		// factor m1
		m1.LU_Factor( index1 );

		// modify and factor m2
		m2.Update_Decrement( offset );
		if ( !m2.LU_Factor( index2 ) ) {
			assert( 0 );
		}
		m2.LU_MultiplyFactors( m3, index2 );
		m2 = m3;

		u.SetSize( 6 );
		for ( int i = 0; i < size; i++ ) {
			u[i] = original[index1[offset]][i];
		}

		// update factors of m1
		m1.LU_UpdateDecrement( v, w, u, offset, index1 );
		m1.LU_MultiplyFactors( m3, index1 );
		m1 = m3;

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::LU_UpdateDecrement failed" );
		}
	}

	/*
		arcMatX::LU_Inverse
	*/

	m2 = original;

	m2.LU_Factor( NULL );
	m2.LU_Inverse( m1, NULL );
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LU_Inverse failed" );
	}

	/*
		arcMatX::QR_Factor
	*/

	c.SetSize( size );
	d.SetSize( size );

	m1 = original;

	m1.QR_Factor( c, d );
	m1.QR_UnpackFactors( q1, r1, c, d );
	m1 = q1 * r1;

	if ( !original.Compare( m1, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::QR_Factor failed" );
	}

	/*
		arcMatX::QR_UpdateRankOne
	*/

	c.SetSize( size );
	d.SetSize( size );

	m1 = original;
	m2 = original;

	w.Random( size, 0 );
	v = w;

	// factor m1
	m1.QR_Factor( c, d );
	m1.QR_UnpackFactors( q1, r1, c, d );

	// modify and factor m2
	m2.Update_RankOne( v, w, 1.0f );
	if ( !m2.QR_Factor( c, d ) ) {
		assert( 0 );
	}
	m2.QR_UnpackFactors( q2, r2, c, d );
	m2 = q2 * r2;

	// update factored m1
	q1.QR_UpdateRankOne( r1, v, w, 1.0f );
	m1 = q1 * r1;

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::QR_UpdateRankOne failed" );
	}

	/*
		arcMatX::QR_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		c.SetSize( size );
		d.SetSize( size );

		m1 = original;
		m2 = original;

		v.Random( size, 1 );
		w.Random( size, 2 );
		w[offset] = 0.0f;

		// factor m1
		m1.QR_Factor( c, d );
		m1.QR_UnpackFactors( q1, r1, c, d );

		// modify and factor m2
		m2.Update_RowColumn( v, w, offset );
		if ( !m2.QR_Factor( c, d ) ) {
			assert( 0 );
		}
		m2.QR_UnpackFactors( q2, r2, c, d );
		m2 = q2 * r2;

		// update m1
		q1.QR_UpdateRowColumn( r1, v, w, offset );
		m1 = q1 * r1;

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::QR_UpdateRowColumn failed" );
		}
	}

	/*
		arcMatX::QR_UpdateIncrement
	*/

	c.SetSize( size+1 );
	d.SetSize( size+1 );

	m1 = original;
	m2 = original;

	v.Random( size + 1, 1 );
	w.Random( size + 1, 2 );
	w[size] = 0.0f;

	// factor m1
	m1.QR_Factor( c, d );
	m1.QR_UnpackFactors( q1, r1, c, d );

	// modify and factor m2
	m2.Update_Increment( v, w );
	if ( !m2.QR_Factor( c, d ) ) {
		assert( 0 );
	}
	m2.QR_UnpackFactors( q2, r2, c, d );
	m2 = q2 * r2;

	// update factored m1
	q1.QR_UpdateIncrement( r1, v, w );
	m1 = q1 * r1;

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::QR_UpdateIncrement failed" );
	}

	/*
		arcMatX::QR_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset++ ) {
		c.SetSize( size+1 );
		d.SetSize( size+1 );

		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		w.SetSize( 6 );
		for ( int i = 0; i < size; i++ ) {
			v[i] = original[i][offset];
			w[i] = original[offset][i];
		}

		// factor m1
		m1.QR_Factor( c, d );
		m1.QR_UnpackFactors( q1, r1, c, d );

		// modify and factor m2
		m2.Update_Decrement( offset );
		if ( !m2.QR_Factor( c, d ) ) {
			assert( 0 );
		}
		m2.QR_UnpackFactors( q2, r2, c, d );
		m2 = q2 * r2;

		// update factors of m1
		q1.QR_UpdateDecrement( r1, v, w, offset );
		m1 = q1 * r1;

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::QR_UpdateDecrement failed" );
		}
	}

	/*
		arcMatX::QR_Inverse
	*/

	m2 = original;

	m2.QR_Factor( c, d );
	m2.QR_Inverse( m1, c, d );
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::QR_Inverse failed" );
	}

	/*
		arcMatX::SVD_Factor
	*/

	m1 = original;
	m3.Zero( size, size );
	w.Zero( size );

	m1.SVD_Factor( w, m3 );
	m2.Diag( w );
	m3.TransposeSelf();
	m1 = m1 * m2 * m3;

	if ( !original.Compare( m1, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::SVD_Factor failed" );
	}

	/*
		arcMatX::SVD_Inverse
	*/

	m2 = original;

	m2.SVD_Factor( w, m3 );
	m2.SVD_Inverse( m1, w, m3 );
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::SVD_Inverse failed" );
	}

	/*
		arcMatX::Cholesky_Factor
	*/

	m1 = original;

	m1.Cholesky_Factor();
	m1.Cholesky_MultiplyFactors( m2 );

	if ( !original.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Cholesky_Factor failed" );
	}

	/*
		arcMatX::Cholesky_UpdateRankOne
	*/

	m1 = original;
	m2 = original;

	w.Random( size, 0 );

	// factor m1
	m1.Cholesky_Factor();
	m1.ClearUpperTriangle();

	// modify and factor m2
	m2.Update_RankOneSymmetric( w, 1.0f );
	if ( !m2.Cholesky_Factor() ) {
		assert( 0 );
	}
	m2.ClearUpperTriangle();

	// update factored m1
	m1.Cholesky_UpdateRankOne( w, 1.0f, 0 );

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Cholesky_UpdateRankOne failed" );
	}

	/*
		arcMatX::Cholesky_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		// factor m1
		m1.Cholesky_Factor();
		m1.ClearUpperTriangle();

		int pdtable[] = { 1, 0, 1, 0, 0, 0 };
		w.Random( size, pdtable[offset] );
		w *= 0.1f;

		// modify and factor m2
		m2.Update_RowColumnSymmetric( w, offset );
		if ( !m2.Cholesky_Factor() ) {
			assert( 0 );
		}
		m2.ClearUpperTriangle();

		// update m1
		m1.Cholesky_UpdateRowColumn( w, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::Cholesky_UpdateRowColumn failed" );
		}
	}

	/*
		arcMatX::Cholesky_UpdateIncrement
	*/

	m1.Random( size + 1, size + 1, 0 );
	m3 = m1 * m1.Transpose();

	m1.SquareSubMatrix( m3, size );
	m2 = m1;

	w.SetSize( size + 1 );
	for ( int i = 0; i < size + 1; i++ ) {
		w[i] = m3[size][i];
	}

	// factor m1
	m1.Cholesky_Factor();

	// modify and factor m2
	m2.Update_IncrementSymmetric( w );
	if ( !m2.Cholesky_Factor() ) {
		assert( 0 );
	}

	// update factored m1
	m1.Cholesky_UpdateIncrement( w );

	m1.ClearUpperTriangle();
	m2.ClearUpperTriangle();

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Cholesky_UpdateIncrement failed" );
	}

	/*
		arcMatX::Cholesky_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset += size - 1 ) {
		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		for ( int i = 0; i < size; i++ ) {
			v[i] = original[i][offset];
		}

		// factor m1
		m1.Cholesky_Factor();

		// modify and factor m2
		m2.Update_Decrement( offset );
		if ( !m2.Cholesky_Factor() ) {
			assert( 0 );
		}

		// update factors of m1
		m1.Cholesky_UpdateDecrement( v, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::Cholesky_UpdateDecrement failed" );
		}
	}

	/*
		arcMatX::Cholesky_Inverse
	*/

	m2 = original;

	m2.Cholesky_Factor();
	m2.Cholesky_Inverse( m1 );
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Cholesky_Inverse failed" );
	}

	/*
		arcMatX::LDLT_Factor
	*/

	m1 = original;

	m1.LDLT_Factor();
	m1.LDLT_MultiplyFactors( m2 );

	if ( !original.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LDLT_Factor failed" );
	}

	m1.LDLT_UnpackFactors( m2, m3 );
	m2 = m2 * m3 * m2.Transpose();

	if ( !original.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LDLT_Factor failed" );
	}

	/*
		arcMatX::LDLT_UpdateRankOne
	*/

	m1 = original;
	m2 = original;

	w.Random( size, 0 );

	// factor m1
	m1.LDLT_Factor();
	m1.ClearUpperTriangle();

	// modify and factor m2
	m2.Update_RankOneSymmetric( w, 1.0f );
	if ( !m2.LDLT_Factor() ) {
		assert( 0 );
	}
	m2.ClearUpperTriangle();

	// update factored m1
	m1.LDLT_UpdateRankOne( w, 1.0f, 0 );

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LDLT_UpdateRankOne failed" );
	}

	/*
		arcMatX::LDLT_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		w.Random( size, 0 );

		// factor m1
		m1.LDLT_Factor();
		m1.ClearUpperTriangle();

		// modify and factor m2
		m2.Update_RowColumnSymmetric( w, offset );
		if ( !m2.LDLT_Factor() ) {
			assert( 0 );
		}
		m2.ClearUpperTriangle();

		// update m1
		m1.LDLT_UpdateRowColumn( w, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::LDLT_UpdateRowColumn failed" );
		}
	}

	/*
		arcMatX::LDLT_UpdateIncrement
	*/

	m1.Random( size + 1, size + 1, 0 );
	m3 = m1 * m1.Transpose();

	m1.SquareSubMatrix( m3, size );
	m2 = m1;

	w.SetSize( size + 1 );
	for ( int i = 0; i < size + 1; i++ ) {
		w[i] = m3[size][i];
	}

	// factor m1
	m1.LDLT_Factor();

	// modify and factor m2
	m2.Update_IncrementSymmetric( w );
	if ( !m2.LDLT_Factor() ) {
		assert( 0 );
	}

	// update factored m1
	m1.LDLT_UpdateIncrement( w );

	m1.ClearUpperTriangle();
	m2.ClearUpperTriangle();

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LDLT_UpdateIncrement failed" );
	}

	/*
		arcMatX::LDLT_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		for ( int i = 0; i < size; i++ ) {
			v[i] = original[i][offset];
		}

		// factor m1
		m1.LDLT_Factor();

		// modify and factor m2
		m2.Update_Decrement( offset );
		if ( !m2.LDLT_Factor() ) {
			assert( 0 );
		}

		// update factors of m1
		m1.LDLT_UpdateDecrement( v, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			arcLibrary::common->Warning( "arcMatX::LDLT_UpdateDecrement failed" );
		}
	}

	/*
		arcMatX::LDLT_Inverse
	*/

	m2 = original;

	m2.LDLT_Factor();
	m2.LDLT_Inverse( m1 );
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::LDLT_Inverse failed" );
	}

	/*
		arcMatX::Eigen_SolveSymmetricTriDiagonal
	*/

	m3 = original;
	m3.TriDiagonal_ClearTriangles();
	m1 = m3;

	v.SetSize( size );

	m1.Eigen_SolveSymmetricTriDiagonal( v );

	m3.TransposeMultiply( m2, m1 );

	for ( int i = 0; i < size; i++ ) {
		for ( int j = 0; j < size; j++ ) {
			m1[i][j] *= v[j];
		}
	}

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Eigen_SolveSymmetricTriDiagonal failed" );
	}

	/*
		arcMatX::Eigen_SolveSymmetric
	*/

	m3 = original;
	m1 = m3;

	v.SetSize( size );

	m1.Eigen_SolveSymmetric( v );

	m3.TransposeMultiply( m2, m1 );

	for ( int i = 0; i < size; i++ ) {
		for ( int j = 0; j < size; j++ ) {
			m1[i][j] *= v[j];
		}
	}

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Eigen_SolveSymmetric failed" );
	}

	/*
		arcMatX::Eigen_Solve
	*/

	m3 = original;
	m1 = m3;

	v.SetSize( size );
	w.SetSize( size );

	m1.Eigen_Solve( v, w );

	m3.TransposeMultiply( m2, m1 );

	for ( int i = 0; i < size; i++ ) {
		for ( int j = 0; j < size; j++ ) {
			m1[i][j] *= v[j];
		}
	}

	if ( !m1.Compare( m2, 1e-4f ) ) {
		arcLibrary::common->Warning( "arcMatX::Eigen_Solve failed" );
	}
}

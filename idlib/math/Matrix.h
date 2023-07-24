#ifndef __MATH_MATRIX_H__
#define __MATH_MATRIX_H__

/*
===============================================================================

  Matrix classes, all matrices are row-major except arcMat3

===============================================================================
*/
#include "/home/fzcpro/Documents/codename-FUBARBROKENARROW/sys/precompiled.h"
#include "Vector.h"
#define MATRIX_INVERSE_EPSILON		1e-14
#define MATRIX_EPSILON				1e-6

class arcAngles;
class arcQuats;
class arcCQuats;
class arcRotate;
class arcMat4;

//===============================================================
//
//	arcMat2 - 2x2 matrix
//
//===============================================================

class arcMat2 {
public:
					aRcMat2Table();
					arcMat2( void );
					explicit arcMat2( const arcVec2 &x, const arcVec2 &y );
					explicit arcMat2( const float xx, const float xy, const float yx, const float yy );
					explicit arcMat2( const float src[ 2 ][ 2 ] );

	const arcVec2 &	operator[]( int index ) const;
	arcVec2 &		operator[]( int index );
	arcMat2			operator-() const;
	arcMat2			operator*( const float a ) const;
	arcVec2			operator*( const arcVec2 &vec ) const;
	arcMat2			operator*( const arcMat2 &a ) const;
	arcMat2			operator+( const arcMat2 &a ) const;
	arcMat2			operator-( const arcMat2 &a ) const;
	arcMat2 & 		operator=( const swfMatrix_t & a ) { xx = a.xx; yy = a.yy; xy = a.xy; yx = a.yx; tx = a.tx; ty = a.ty; return *this; }
	arcMat2 &		operator*=( const float a );
	arcMat2 &		operator*=( const arcMat2 &a );
	arcMat2 &		operator+=( const arcMat2 &a );
	arcMat2 &		operator-=( const arcMat2 &a );

	friend arcMat2	operator*( const float a, const arcMat2 &mat );
	friend arcVec2	operator*( const arcVec2 &vec, const arcMat2 &mat );
	friend arcVec2 &operator*=( arcVec2 &vec, const arcMat2 &mat );

	bool			Compare( const arcMat2 &a ) const;						// exact compare, no epsilon
	bool			Compare( const arcMat2 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const arcMat2 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const arcMat2 &a ) const;					// exact compare, no epsilon

	void			Zero( void );
	void			Identity( void );
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;

	float			Trace( void ) const;
	float			Determinant( void ) const;

	arcVec2			Scale( const arcVec2 & in ) const;
	arcVec2			Transform( const arcVec2 & in ) const;

	arcMat2			Transpose( void ) const;	// returns transpose
	arcMat2 &		TransposeSelf( void );

	swfMatrix_t 	Multiply( const swfMatrix_t & a ) const;
	swfMatrix_t		Inverse() const;
	arcMat2			Inverse( void ) const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf( void );		// returns false if determinant is zero
	arcMat2			InverseFast( void ) const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf( void );	// returns false if determinant is zero

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;
private:
	arcVec2			mat[ 2 ];
	float			xx, yy;
	float			xy, yx;
	float 			tx, ty;
};

extern arcMat2 mat2_zero;
extern arcMat2 mat2_identity;
#define mat2_default	mat2_identity

ARC_INLINE arcMat2::aRcMat2Table() : xx( 1.0f ), yy( 1.0f ), yx( 0.0f ), xy( 0.0f ), tx( 0.0f ), ty( 0.0f ){}

ARC_INLINE arcMat2::arcMat2( void ) {
}

ARC_INLINE arcMat2::arcMat2( const arcVec2 &x, const arcVec2 &y ) {
	mat[ 0 ].x = x.x; mat[ 0 ].y = x.y;
	mat[ 1 ].x = y.x; mat[ 1 ].y = y.y;
}

ARC_INLINE arcMat2::arcMat2( const float xx, const float xy, const float yx, const float yy ) {
	mat[ 0 ].x = xx; mat[ 0 ].y = xy;
	mat[ 1 ].x = yx; mat[ 1 ].y = yy;
}

ARC_INLINE arcMat2::arcMat2( const float src[ 2 ][ 2 ] ) {
	memcpy( mat, src, 2 * 2 * sizeof( float ) );
}

ARC_INLINE const arcVec2 &arcMat2::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 2 ) );
	return mat[index];
}

ARC_INLINE arcVec2 &arcMat2::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 2 ) );
	return mat[index];
}

ARC_INLINE arcMat2 arcMat2::operator-() const {
	return arcMat2(	-mat[0][0], -mat[0][1],
					-mat[1][0], -mat[1][1] );
}

ARC_INLINE arcVec2 arcMat2::operator*( const arcVec2 &vec ) const {
	return arcVec2(
		mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y,
		mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y );
}

ARC_INLINE arcMat2 arcMat2::operator*( const arcMat2 &a ) const {
	return arcMat2(
		mat[0].x * a[0].x + mat[0].y * a[1].x,
		mat[0].x * a[0].y + mat[0].y * a[1].y,
		mat[1].x * a[0].x + mat[1].y * a[1].x,
		mat[1].x * a[0].y + mat[1].y * a[1].y );
}

ARC_INLINE arcMat2 arcMat2::operator*( const float a ) const {
	return arcMat2(
		mat[0].x * a, mat[0].y * a,
		mat[1].x * a, mat[1].y * a );
}

ARC_INLINE arcMat2 arcMat2::operator+( const arcMat2 &a ) const {
	return arcMat2(
		mat[0].x + a[0].x, mat[0].y + a[0].y,
		mat[1].x + a[1].x, mat[1].y + a[1].y );
}

ARC_INLINE arcMat2 arcMat2::operator-( const arcMat2 &a ) const {
	return arcMat2(
		mat[0].x - a[0].x, mat[0].y - a[0].y,
		mat[1].x - a[1].x, mat[1].y - a[1].y );
}

ARC_INLINE arcMat2 &arcMat2::operator*=( const float a ) {
	mat[0].x *= a; mat[0].y *= a;
	mat[1].x *= a; mat[1].y *= a;

    return *this;
}

ARC_INLINE arcMat2 &arcMat2::operator*=( const arcMat2 &a ) {
	float x, y;
	x = mat[0].x; y = mat[0].y;
	mat[0].x = x * a[0].x + y * a[1].x;
	mat[0].y = x * a[0].y + y * a[1].y;
	x = mat[1].x; y = mat[1].y;
	mat[1].x = x * a[0].x + y * a[1].x;
	mat[1].y = x * a[0].y + y * a[1].y;
	return *this;
}

ARC_INLINE arcMat2 &arcMat2::operator+=( const arcMat2 &a ) {
	mat[0].x += a[0].x; mat[0].y += a[0].y;
	mat[1].x += a[1].x; mat[1].y += a[1].y;

    return *this;
}

ARC_INLINE arcMat2 &arcMat2::operator-=( const arcMat2 &a ) {
	mat[0].x -= a[0].x; mat[0].y -= a[0].y;
	mat[1].x -= a[1].x; mat[1].y -= a[1].y;

    return *this;
}

ARC_INLINE arcVec2 operator*( const arcVec2 &vec, const arcMat2 &mat ) {
	return mat * vec;
}

ARC_INLINE arcMat2 operator*( const float a, arcMat2 const &mat ) {
	return mat * a;
}

ARC_INLINE arcVec2 &operator*=( arcVec2 &vec, const arcMat2 &mat ) {
	vec = mat * vec;
	return vec;
}

ARC_INLINE bool arcMat2::Compare( const arcMat2 &a ) const {
	if ( mat[0].Compare( a[0] ) &&
		mat[1].Compare( a[1] ) ) {
		return true;
	}
	return false;
}

ARC_INLINE bool arcMat2::Compare( const arcMat2 &a, const float epsilon ) const {
	if ( mat[0].Compare( a[0], epsilon ) &&
		mat[1].Compare( a[1], epsilon ) ) {
		return true;
	}
	return false;
}

ARC_INLINE bool arcMat2::operator==( const arcMat2 &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcMat2::operator!=( const arcMat2 &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcMat2::Zero( void ) {
	mat[0].Zero();
	mat[1].Zero();
}

ARC_INLINE void arcMat2::Identity( void ) {
	*this = mat2_identity;
}

ARC_INLINE bool arcMat2::IsIdentity( const float epsilon ) const {
	return Compare( mat2_identity, epsilon );
}

ARC_INLINE bool arcMat2::IsSymmetric( const float epsilon ) const {
	return ( arcMath::Fabs( mat[0][1] - mat[1][0] ) < epsilon );
}

ARC_INLINE bool arcMat2::IsDiagonal( const float epsilon ) const {
	if ( arcMath::Fabs( mat[0][1] ) > epsilon ||
		arcMath::Fabs( mat[1][0] ) > epsilon ) {
		return false;
	}
	return true;
}

ARC_INLINE float arcMat2::Trace( void ) const {
	return ( mat[0][0] + mat[1][1] );
}

ARC_INLINE float arcMat2::Determinant( void ) const {
	return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
}

ARC_INLINE arcVec2 arcMat2::Scale( const arcVec2 & in ) const {
	return arcVec2( ( in.x * xx ) + ( in.y * xy ),
					( in.y * yy ) + ( in.x * yx ) );
}

ARC_INLINE arcMat2 arcMat2::Transpose( void ) const {
	return arcMat2(	mat[0][0], mat[1][0],
					mat[0][1], mat[1][1] );
}

ARC_INLINE arcMat2 &arcMat2::TransposeSelf( void ) {
	float tmp;

	tmp = mat[0][1];
	mat[0][1] = mat[1][0];
	mat[1][0] = tmp;

	return *this;
}

ARC_INLINE arcVec2 arcMat2::Transform( const arcVec2 & in ) const {
	return arcVec2( ( in.x * xx ) + ( in.y * xy ) + tx,
					( in.y * yy ) + ( in.x * yx ) + ty );
}

ARC_INLINE arcMat2 arcMat2::Inverse() const {
	arcMat2 inverse;
	float det = ( ( xx * yy ) - ( yx * xy ) );
	if ( arcMath::Fabs( det ) < arcMath::FLT_SMALLEST_NON_DENORMAL ) {
		return *this;
	}
	float invDet = 1.0f / det;
	inverse.xx = invDet *  yy;
	inverse.yx = invDet * -yx;
	inverse.xy = invDet * -xy;
	inverse.yy = invDet *  xx;
	//inverse.tx = invDet * ( xy * ty ) - ( yy * tx );
	//inverse.ty = invDet * ( yx * tx ) - ( xx * ty );
	return inverse;
}

ARC_INLINE arcMat2 arcMat2::Inverse( void ) const {
	arcMat2 invMat;

	invMat = *this;
	int r = invMat.InverseSelf();
	assert( r );
	return invMat;
}

ARC_INLINE arcMat2 arcMat2::InverseFast( void ) const {
	arcMat2 invMat;

	invMat = *this;
	int r = invMat.InverseFastSelf();
	assert( r );
	return invMat;
}

ARC_INLINE arcMat2 arcMat2::Multiply( const swfMatrix_t & a ) const {
	arcMat2 result;
    result.xx = xx * a.xx + yx * a.xy;
    result.yx = xx * a.yx + yx * a.yy;
    result.xy = xy * a.xx + yy * a.xy;
    result.yy = xy * a.yx + yy * a.yy;
    result.tx = tx * a.xx + ty * a.xy + a.tx;
    result.ty = tx * a.yx + ty * a.yy + a.ty;
	return result;
}

ARC_INLINE int arcMat2::GetDimension( void ) const {
	return 4;
}

ARC_INLINE const float *arcMat2::ToFloatPtr( void ) const {
	return mat[0].ToFloatPtr();
}

ARC_INLINE float *arcMat2::ToFloatPtr( void ) {
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	arcMat3 - 3x3 matrix
//
//	NOTE:	matrix is column-major
//
//===============================================================

class arcMat3 {
public:
					arcMat3( void );
					explicit arcMat3( const arcVec3 &x, const arcVec3 &y, const arcVec3 &z );
					explicit arcMat3( const float xx, const float xy, const float xz, const float yx, const float yy, const float yz, const float zx, const float zy, const float zz );
					explicit arcMat3( const float src[ 3 ][ 3 ] );

	const arcVec3 &	operator[]( int index ) const;
	arcVec3 &		operator[]( int index );
	arcMat3			operator-() const;
	arcMat3			operator*( const float a ) const;
	arcVec3			operator*( const arcVec3 &vec ) const;
	arcMat3			operator*( const arcMat3 &a ) const;
	arcVec3			operator/( const arcVec3 &vec ) const;
	arcMat3			operator/( const arcMat3 &a ) const;
	arcMat3			operator+( const arcMat3 &a ) const;
	arcMat3			operator-( const arcMat3 &a ) const;
	arcMat3 &		operator*=( const float a );
	arcMat3 &		operator*=( const arcMat3 &a );
	arcMat3 &		operator+=( const arcMat3 &a );
	arcMat3 &		operator-=( const arcMat3 &a );

	friend arcMat3	operator*( const float a, const arcMat3 &mat );
	friend arcVec3	operator*( const arcVec3 &vec, const arcMat3 &mat );
	friend arcVec3 &operator*=( arcVec3 &vec, const arcMat3 &mat );

	bool			Compare( const arcMat3 &a ) const;						// exact compare, no epsilon
	bool			Compare( const arcMat3 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const arcMat3 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const arcMat3 &a ) const;					// exact compare, no epsilon

	void			Zero( void );
	void			Identity( void );
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsRotated( void ) const;

	void			ProjectVector( const arcVec3 &src, arcVec3 &dst ) const;
	void			UnprojectVector( const arcVec3 &src, arcVec3 &dst ) const;

	bool			FixDegeneracies( void );	// fix degenerate axial cases
	bool			FixDenormals( void );		// change tiny numbers to zero

	float			Trace( void ) const;

	// uniformed scale version to all axes.
	arcVec3			UniformScale( const float &scaleFactor ) const;
	// seperate scaling for each individual axis
	arcVec3 		IndividualScale( const arcVec3 &in ) const;

	// non-uniform scaling with individual axes
	arcVec3 		NonUniformScale( const float &scaleX, const float &scaleY, const float &scaleZ ) const;

	arcVec3			Scale( const arcVec3 &in ) const;

	float			Determinant( void ) const;
	arcMat3			OrthoNormalize( void ) const;
	arcMat3 &		OrthoNormalizeSelf( void );
	arcMat3			Transpose( void ) const;	// returns transpose
	arcMat3 &		TransposeSelf( void );
	arcMat3			Inverse( void ) const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf( void );		// returns false if determinant is zero
	arcMat3			InverseFast( void ) const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf( void );	// returns false if determinant is zero
	arcMat3			TransposeMultiply( const arcMat3 &b ) const;

	arcMat3			InertiaTranslate( const float mass, const arcVec3 &centerOfMass, const arcVec3 &translation ) const;
	arcMat3 &		InertiaTranslateSelf( const float mass, const arcVec3 &centerOfMass, const arcVec3 &translation );
	arcMat3			InertiaRotate( const arcMat3 &rotation ) const;
	arcMat3 &		InertiaRotateSelf( const arcMat3 &rotation );

	int				GetDimension( void ) const;

	arcAngles		ToAngles( void ) const;
	arcQuats		ToQuat( void ) const;
	arcCQuats		ToCQuat( void ) const;
	arcRotate		ToRotation( void ) const;
	arcMat4			ToMat4( void ) const;
	arcVec3			ToAngularVelocity( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	friend void		TransposeMultiply( const arcMat3 &inv, const arcMat3 &b, arcMat3 &dst );
	friend arcMat3	SkewSymmetric( arcVec3 const &src );

private:
	arcVec3			mat[ 3 ];
};

extern arcMat3 mat3_zero;
extern arcMat3 mat3_identity;
#define mat3_default	mat3_identity

ARC_INLINE arcMat3::arcMat3( void ) {
}

ARC_INLINE arcMat3::arcMat3( const arcVec3 &x, const arcVec3 &y, const arcVec3 &z ) {
	mat[ 0 ].x = x.x; mat[ 0 ].y = x.y; mat[ 0 ].z = x.z;
	mat[ 1 ].x = y.x; mat[ 1 ].y = y.y; mat[ 1 ].z = y.z;
	mat[ 2 ].x = z.x; mat[ 2 ].y = z.y; mat[ 2 ].z = z.z;
}

ARC_INLINE arcMat3::arcMat3( const float xx, const float xy, const float xz, const float yx, const float yy, const float yz, const float zx, const float zy, const float zz ) {
	mat[ 0 ].x = xx; mat[ 0 ].y = xy; mat[ 0 ].z = xz;
	mat[ 1 ].x = yx; mat[ 1 ].y = yy; mat[ 1 ].z = yz;
	mat[ 2 ].x = zx; mat[ 2 ].y = zy; mat[ 2 ].z = zz;
}

ARC_INLINE arcMat3::arcMat3( const float src[ 3 ][ 3 ] ) {
	memcpy( mat, src, 3 * 3 * sizeof( float ) );
}

ARC_INLINE const arcVec3 &arcMat3::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 3 ) );
	return mat[index];
}

ARC_INLINE arcVec3 &arcMat3::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 3 ) );
	return mat[index];
}

ARC_INLINE arcMat3 arcMat3::operator-() const {
	return arcMat3(	-mat[0][0], -mat[0][1], -mat[0][2], -mat[1][0], -mat[1][1], -mat[1][2], -mat[2][0], -mat[2][1], -mat[2][2] );
}

ARC_INLINE arcVec3 arcMat3::operator*( const arcVec3 &vec ) const {
	return arcVec3( mat[ 0 ].x * vec.x + mat[ 1 ].x * vec.y + mat[ 2 ].x * vec.z,
		mat[ 0 ].y * vec.x + mat[ 1 ].y * vec.y + mat[ 2 ].y * vec.z,
		mat[ 0 ].z * vec.x + mat[ 1 ].z * vec.y + mat[ 2 ].z * vec.z );
}

ARC_INLINE arcMat3 arcMat3::operator*( const arcMat3 &a ) const {;
	arcMat3 dst;

	const float *m1Ptr = reinterpret_cast<const float *>( this );
	const float *m2Ptr = reinterpret_cast<const float *>( & a );
	float dstPtr = reinterpret_cast<float *>( & dst );

	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 3 + j ] + m1Ptr[1] * m2Ptr[ 1 * 3 + j ] + m1Ptr[2] * m2Ptr[ 2 * 3 + j ];
			dstPtr++;
		}
		m1Ptr += 3;
	}
	return dst;
}

ARC_INLINE arcMat3 arcMat3::operator*( const float a ) const {
	return arcMat3( mat[0].x * a, mat[0].y * a, mat[0].z * a,
		mat[1].x * a, mat[1].y * a, mat[1].z * a,
		mat[2].x * a, mat[2].y * a, mat[2].z * a );
}

ARC_INLINE arcVec3 arcMat3::operator/( const arcVec3 &vec ) const {
	return( arcVec3( mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z,
		mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z,
		mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z ) );
}

ARC_INLINE arcMat3& arcMat3::operator=( const arcMat3 &a ) {
	memcpy( mat[ 0 ].ToFloatPtr(), a.mat[ 0 ].ToFloatPtr(), sizeof( mat ) );
	return *this;
}

ARC_INLINE arcMat3 arcMat3::operator/( const arcMat3 &a ) const {
	arcMat3 dst;
	dst[0].x = mat[0].x * a.mat[0].x + mat[0].y * a.mat[0].y + mat[0].z * a.mat[0].z;
	dst[0].y = mat[0].x * a.mat[1].x + mat[0].y * a.mat[1].y + mat[0].z * a.mat[1].z;
	dst[0].z = mat[0].x * a.mat[2].x + mat[0].y * a.mat[2].y + mat[0].z * a.mat[2].z;

	dst[1].x = mat[1].x * a.mat[0].x + mat[1].y * a.mat[0].y + mat[1].z * a.mat[0].z;
	dst[1].y = mat[1].x * a.mat[1].x + mat[1].y * a.mat[1].y + mat[1].z * a.mat[1].z;
	dst[1].z = mat[1].x * a.mat[2].x + mat[1].y * a.mat[2].y + mat[1].z * a.mat[2].z;

	dst[2].x = mat[2].x * a.mat[0].x + mat[2].y * a.mat[0].y + mat[2].z * a.mat[0].z;
	dst[2].y = mat[2].x * a.mat[1].x + mat[2].y * a.mat[1].y + mat[2].z * a.mat[1].z;
	dst[2].z = mat[2].x * a.mat[2].x + mat[2].y * a.mat[2].y + mat[2].z * a.mat[2].z;

	return( dst );
}

ARC_INLINE arcMat3 arcMat3::operator+( const arcMat3 &a ) const {
	return arcMat3( mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z,
		mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z,
		mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z );
}

ARC_INLINE arcMat3 arcMat3::operator-( const arcMat3 &a ) const {
	return arcMat3( mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z,
		mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z,
		mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z );
}

ARC_INLINE arcMat3 &arcMat3::operator*=( const float a ) {
	mat[0].x *= a; mat[0].y *= a; mat[0].z *= a;
	mat[1].x *= a; mat[1].y *= a; mat[1].z *= a;
	mat[2].x *= a; mat[2].y *= a; mat[2].z *= a;

    return *this;
}

ARC_INLINE arcMat3 &arcMat3::operator*=( const arcMat3 &a ) {
	float dst[3];
	float *m1Ptr = reinterpret_cast<float *>( this );
	const float *m2Ptr = reinterpret_cast<const float *>( & a );
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			dst[j] = m1Ptr[0] * m2Ptr[0 * 3 + j] + m1Ptr[1] * m2Ptr[1 * 3 + j] + m1Ptr[2] * m2Ptr[2 * 3 + j];
		}
		m1Ptr[0] = dst[0]; m1Ptr[1] = dst[1]; m1Ptr[2] = dst[2];
		m1Ptr += 3;
	}
	return *this;
}

ARC_INLINE arcMat3 &arcMat3::operator+=( const arcMat3 &a ) {
	mat[0].x += a[0].x; mat[0].y += a[0].y; mat[0].z += a[0].z;
	mat[1].x += a[1].x; mat[1].y += a[1].y; mat[1].z += a[1].z;
	mat[2].x += a[2].x; mat[2].y += a[2].y; mat[2].z += a[2].z;

    return *this;
}

ARC_INLINE arcMat3 &arcMat3::operator-=( const arcMat3 &a ) {
	mat[0].x -= a[0].x; mat[0].y -= a[0].y; mat[0].z -= a[0].z;
	mat[1].x -= a[1].x; mat[1].y -= a[1].y; mat[1].z -= a[1].z;
	mat[2].x -= a[2].x; mat[2].y -= a[2].y; mat[2].z -= a[2].z;

    return *this;
}

ARC_INLINE arcVec3 operator*( const arcVec3 &vec, const arcMat3 &mat ) {
	return mat * vec;
}

ARC_INLINE arcMat3 operator*( const float a, const arcMat3 &mat ) {
	return mat * a;
}

ARC_INLINE arcVec3 &operator*=( arcVec3 &vec, const arcMat3 &mat ) {
	float x = mat[ 0 ].x * vec.x + mat[ 1 ].x * vec.y + mat[ 2 ].x * vec.z;
	float y = mat[ 0 ].y * vec.x + mat[ 1 ].y * vec.y + mat[ 2 ].y * vec.z;
	vec.z = mat[ 0 ].z * vec.x + mat[ 1 ].z * vec.y + mat[ 2 ].z * vec.z;
	vec.x = x;
	vec.y = y;
	return vec;
}

ARC_INLINE bool arcMat3::Compare( const arcMat3 &a ) const {
	if ( mat[0].Compare( a[0] ) &&
		mat[1].Compare( a[1] ) &&
		mat[2].Compare( a[2] ) ) {
		return true;
	}
	return false;
}

ARC_INLINE bool arcMat3::Compare( const arcMat3 &a, const float epsilon ) const {
	if ( mat[0].Compare( a[0], epsilon ) &&
		mat[1].Compare( a[1], epsilon ) &&
		mat[2].Compare( a[2], epsilon ) ) {
		return true;
	}
	return false;
}

ARC_INLINE bool arcMat3::operator==( const arcMat3 &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcMat3::operator!=( const arcMat3 &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcMat3::Zero( void ) {
	memset( mat, 0, sizeof( arcMat3 ) );
}

ARC_INLINE void arcMat3::Identity( void ) {
	*this = mat3_identity;
}

ARC_INLINE bool arcMat3::IsIdentity( const float epsilon ) const {
	return Compare( mat3_identity, epsilon );
}

ARC_INLINE bool arcMat3::IsSymmetric( const float epsilon ) const {
	if ( arcMath::Fabs( mat[0][1] - mat[1][0] ) > epsilon ) {
		return false;
	}
	if ( arcMath::Fabs( mat[0][2] - mat[2][0] ) > epsilon ) {
		return false;
	}
	if ( arcMath::Fabs( mat[1][2] - mat[2][1] ) > epsilon ) {
		return false;
	}
	return true;
}

ARC_INLINE bool arcMat3::IsDiagonal( const float epsilon ) const {
	if ( arcMath::Fabs( mat[0][1] ) > epsilon ||
		arcMath::Fabs( mat[0][2] ) > epsilon ||
		arcMath::Fabs( mat[1][0] ) > epsilon ||
		arcMath::Fabs( mat[1][2] ) > epsilon ||
		arcMath::Fabs( mat[2][0] ) > epsilon ||
		arcMath::Fabs( mat[2][1] ) > epsilon ) {
		return false;
	}
	return true;
}

ARC_INLINE bool arcMat3::IsRotated( void ) const {
	return !Compare( mat3_identity );
}

ARC_INLINE void arcMat3::ProjectVector( const arcVec3 &src, arcVec3 &dst ) const {
	dst.x = src * mat[ 0 ];
	dst.y = src * mat[ 1 ];
	dst.z = src * mat[ 2 ];
}

ARC_INLINE void arcMat3::UnprojectVector( const arcVec3 &src, arcVec3 &dst ) const {
	dst = mat[ 0 ] * src.x + mat[ 1 ] * src.y + mat[ 2 ] * src.z;
}

ARC_INLINE bool arcMat3::FixDegeneracies( void ) {
	bool r = mat[0].FixDegenerateNormal();
	r |= mat[1].FixDegenerateNormal();
	r |= mat[2].FixDegenerateNormal();
	return r;
}

ARC_INLINE bool arcMat3::FixDenormals( void ) {
	bool r = mat[0].FixDenormals();
	r |= mat[1].FixDenormals();
	r |= mat[2].FixDenormals();
	return r;
}

ARC_INLINE float arcMat3::Trace( void ) const {
	return ( mat[0][0] + mat[1][1] + mat[2][2] );
}

ARC_INLINE arcMat3 arcMat3::OrthoNormalize( void ) const {
	arcMat3 ortho = *this;
	ortho[ 0 ].Normalize();
	ortho[ 2 ].Cross( mat[ 0 ], mat[ 1 ] );
	ortho[ 2 ].Normalize();
	ortho[ 1 ].Cross( mat[ 2 ], mat[ 0 ] );
	ortho[ 1 ].Normalize();
	return ortho;
}

ARC_INLINE arcMat3 &arcMat3::OrthoNormalizeSelf( void ) {
	mat[ 0 ].Normalize();
	mat[ 2 ].Cross( mat[ 0 ], mat[ 1 ] );
	mat[ 2 ].Normalize();
	mat[ 1 ].Cross( mat[ 2 ], mat[ 0 ] );
	mat[ 1 ].Normalize();
	return *this;
}

ARC_INLINE arcVec3 arcMat3::Scale( const arcVec3 &in ) const {
	return arcVec3( ( in.x * xx ) + ( in.y * xy ),
					( in.y * yy ) + ( in.x * yx ),
					( in.y * zz ) + ( in.x * zx ) );
}

// uniformed scale version to all axes.
ARC_INLINE arcVec3 arcMat3::UniformScale( const float &scaleFactor ) const {
    return arcVec3( xx * scaleFactor, yy * scaleFactor, zz * scaleFactor );
}
// seperate scaling for each individual axis
ARC_INLINE arcVec3 arcMat3::IndividualScale( const arcVec3 &in ) const {
    return arcVec3( in.x * xx, in.y * yy, in.z * zz );
}
// non-uniform scaling with individual axes
ARC_INLINE arcVec3 arcMat3::NonUniformScale( const float &scaleX, const float &scaleY, const float &scaleZ ) const {
    return arcVec3( xx * scaleX, yy * scaleY, zz * scaleZ );
}

ARC_INLINE arcMat3 arcMat3::Transpose( void ) const {
	return arcMat3(	mat[0][0], mat[1][0], mat[2][0],
					mat[0][1], mat[1][1], mat[2][1],
					mat[0][2], mat[1][2], mat[2][2] );
}

ARC_INLINE arcMat3 &arcMat3::TransposeSelf( void ) {
	float tmp0 = mat[0][1];
	mat[0][1] = mat[1][0];
	mat[1][0] = tmp0;
	float tmp1 = mat[0][2];
	mat[0][2] = mat[2][0];
	mat[2][0] = tmp1;
	float tmp2 = mat[1][2];
	mat[1][2] = mat[2][1];
	mat[2][1] = tmp2;

	return *this;
}

ARC_INLINE arcMat3 arcMat3::Inverse( void ) const {
	arcMat3 invMat = *this;
	int r = invMat.InverseSelf();
	assert( r );
	return invMat;
}

ARC_INLINE arcMat3 arcMat3::InverseFast( void ) const {
	arcMat3 invMat = *this;
	int r = invMat.InverseFastSelf();
	assert( r );
	return invMat;
}

ARC_INLINE arcMat3 arcMat3::TransposeMultiply( const arcMat3 &b ) const {
	return arcMat3(	mat[0].x * b[0].x + mat[1].x * b[1].x + mat[2].x * b[2].x,
					mat[0].x * b[0].y + mat[1].x * b[1].y + mat[2].x * b[2].y,
					mat[0].x * b[0].z + mat[1].x * b[1].z + mat[2].x * b[2].z,
					mat[0].y * b[0].x + mat[1].y * b[1].x + mat[2].y * b[2].x,
					mat[0].y * b[0].y + mat[1].y * b[1].y + mat[2].y * b[2].y,
					mat[0].y * b[0].z + mat[1].y * b[1].z + mat[2].y * b[2].z,
					mat[0].z * b[0].x + mat[1].z * b[1].x + mat[2].z * b[2].x,
					mat[0].z * b[0].y + mat[1].z * b[1].y + mat[2].z * b[2].y,
					mat[0].z * b[0].z + mat[1].z * b[1].z + mat[2].z * b[2].z );
}

ARC_INLINE void arcMat3::TransposeMultiply( const arcMat3 &transpose, const arcMat3 &b, arcMat3 &dst ) {
	dst[0].x = transpose[0].x * b[0].x + transpose[1].x * b[1].x + transpose[2].x * b[2].x;
	dst[0].y = transpose[0].x * b[0].y + transpose[1].x * b[1].y + transpose[2].x * b[2].y;
	dst[0].z = transpose[0].x * b[0].z + transpose[1].x * b[1].z + transpose[2].x * b[2].z;
	dst[1].x = transpose[0].y * b[0].x + transpose[1].y * b[1].x + transpose[2].y * b[2].x;
	dst[1].y = transpose[0].y * b[0].y + transpose[1].y * b[1].y + transpose[2].y * b[2].y;
	dst[1].z = transpose[0].y * b[0].z + transpose[1].y * b[1].z + transpose[2].y * b[2].z;
	dst[2].x = transpose[0].z * b[0].x + transpose[1].z * b[1].x + transpose[2].z * b[2].x;
	dst[2].y = transpose[0].z * b[0].y + transpose[1].z * b[1].y + transpose[2].z * b[2].y;
	dst[2].z = transpose[0].z * b[0].z + transpose[1].z * b[1].z + transpose[2].z * b[2].z;
}

ARC_INLINE arcMat3 arcMat3::SkewSymmetric( arcVec3 const &src ) {
	return arcMat3( 0.0f, -src.z,  src.y, src.z,   0.0f, -src.x, -src.y,  src.x,   0.0f );
}

ARC_INLINE int arcMat3::GetDimension( void ) const {
	return 9;
}

ARC_INLINE const float *arcMat3::ToFloatPtr( void ) const {
	return mat[0].ToFloatPtr();
}

ARC_INLINE float *arcMat3::ToFloatPtr( void ) {
	return mat[0].ToFloatPtr();
}

ARC_INLINE arcMat3 arcMat3::ToMaya( void ) const {
	arcMat3 matMaya = *this;
	matMaya.ToMayaSelf();
	return matMaya;
}

ARC_INLINE arcMat3 &arcMat3::ToMayaSelf( void ) {
	Swap( mat[ 0 ][ 2 ], mat[ 0 ][ 1 ] );
	mat[ 0 ][ 2 ] = -mat[ 0 ][ 2 ];

	Swap( mat[ 1 ][ 2 ], mat[ 1 ][ 1 ] );
	mat[ 1 ][ 2 ] = -mat[ 1 ][ 2 ];

	Swap( mat[ 2 ][ 1 ], mat[ 2 ][ 2 ] );
	mat[ 2 ][ 2 ] = -mat[ 2 ][ 2 ];

	return (*this);
}

ARC_INLINE arcMat3 arcMat3::FromMaya( void ) const {
	arcMat3 matId = *this;
	matId.FromMayaSelf();
	return matId;
}

ARC_INLINE arcMat3 &arcMat3::FromMayaSelf( void ) {
	Swap( mat[ 0 ][ 2 ], mat[ 0 ][ 1 ] );
	mat[ 0 ][ 1 ] = -mat[ 0 ][ 1 ];

	Swap( mat[ 1 ][ 2 ], mat[ 1 ][ 1 ] );
	mat[ 1 ][ 1 ] = -mat[ 1 ][ 1 ];

	Swap( mat[ 2 ][ 1 ], mat[ 2 ][ 2 ] );
	mat[ 2 ][ 1 ] = -mat[ 2 ][ 1 ];

	return (* this);
}


/*
===============================================================================

  row-major 3x4 matrix

  arcMat3 m;
  arcVec3 t;

  m[0][0], m[1][0], m[2][0], t[0]
  m[0][1], m[1][1], m[2][1], t[1]
  m[0][2], m[1][2], m[2][2], t[2]

===============================================================================
*/

class aRcMat3x4 {
public:

	void			SetRotation( const arcMat3 &m );
	void			SetTranslation( const arcVec3 &t );

	bool			Compare( const aRcMat3x4 &a ) const;							// exact compare, no epsilon
	bool			Compare( const aRcMat3x4 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const aRcMat3x4 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const aRcMat3x4 &a ) const;						// exact compare, no epsilon

	void			Identity( void );
	void			Invert( void );

	void			LeftMultiply( const aRcMat3x4 &m );
	void			LeftMultiply( const arcMat3 &m );
	void			RightMultiply( const aRcMat3x4 &m );
	void			RightMultiply( const arcMat3 &m );

	void			Transform( arcVec3 &result, const arcVec3 &v ) const;
	void			Rotate( arcVec3 &result, const arcVec3 &v ) const;

	arcMat3			ToMat3( void ) const;
	arcVec3			ToVec3( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	float			mat[3*4];
};


/*
=============
aRcMat3x4::SetRotation
=============
*/
ARC_INLINE void aRcMat3x4::SetRotation( const arcMat3 &m ) {
	// NOTE: arcMat3 is transposed because it is column-major
	mat[0 * 4 + 0] = m[0][0];
	mat[0 * 4 + 1] = m[1][0];
	mat[0 * 4 + 2] = m[2][0];
	mat[1 * 4 + 0] = m[0][1];
	mat[1 * 4 + 1] = m[1][1];
	mat[1 * 4 + 2] = m[2][1];
	mat[2 * 4 + 0] = m[0][2];
	mat[2 * 4 + 1] = m[1][2];
	mat[2 * 4 + 2] = m[2][2];
}

/*
=============
aRcMat3x4::SetTranslation
=============
*/
ARC_INLINE void aRcMat3x4::SetTranslation( const arcVec3 &t ) {
	mat[0 * 4 + 3] = t[0];
	mat[1 * 4 + 3] = t[1];
	mat[2 * 4 + 3] = t[2];
}

/*
=============
aRcMat3x4::Compare
=============
*/
ARC_INLINE bool aRcMat3x4::Compare( const aRcMat3x4 &a ) const {
	int i;

	for ( i = 0; i < 12; i++ ) {
		if ( mat[i] != a.mat[i] ) {
			return false;
		}
	}
	return true;
}

/*
=============
aRcMat3x4::Compare
=============
*/
ARC_INLINE bool aRcMat3x4::Compare( const aRcMat3x4 &a, const float epsilon ) const {
	int i;

	for ( i = 0; i < 12; i++ ) {
		if ( arcMath::Fabs( mat[i] - a.mat[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool aRcMat3x4::operator==( const aRcMat3x4 &a ) const {
	return Compare( a );
}

ARC_INLINE bool aRcMat3x4::operator!=( const aRcMat3x4 &a ) const {
	return !Compare( a );
}

/*
=============
aRcMat3x4::Identity
=============
*/
ARC_INLINE void aRcMat3x4::Identity( void ) {
	mat[0 * 4 + 0] = 1.0f; mat[0 * 4 + 1] = 0.0f; mat[0 * 4 + 2] = 0.0f; mat[0 * 4 + 3] = 0.0f;
	mat[0 * 4 + 0] = 0.0f; mat[0 * 4 + 1] = 1.0f; mat[0 * 4 + 2] = 0.0f; mat[0 * 4 + 3] = 0.0f;
	mat[0 * 4 + 0] = 0.0f; mat[0 * 4 + 1] = 0.0f; mat[0 * 4 + 2] = 1.0f; mat[0 * 4 + 3] = 0.0f;
}

/*
=============
aRcMat3x4::Invert
=============
*/
ARC_INLINE void aRcMat3x4::Invert( void ) {
	float tmp[3];

	// negate inverse rotated translation part
	tmp[0] = mat[0 * 4 + 0] * mat[0 * 4 + 3] + mat[1 * 4 + 0] * mat[1 * 4 + 3] + mat[2 * 4 + 0] * mat[2 * 4 + 3];
	tmp[1] = mat[0 * 4 + 1] * mat[0 * 4 + 3] + mat[1 * 4 + 1] * mat[1 * 4 + 3] + mat[2 * 4 + 1] * mat[2 * 4 + 3];
	tmp[2] = mat[0 * 4 + 2] * mat[0 * 4 + 3] + mat[1 * 4 + 2] * mat[1 * 4 + 3] + mat[2 * 4 + 2] * mat[2 * 4 + 3];
	mat[0 * 4 + 3] = -tmp[0];
	mat[1 * 4 + 3] = -tmp[1];
	mat[2 * 4 + 3] = -tmp[2];

	// transpose rotation part
	tmp[0] = mat[0 * 4 + 1];
	mat[0 * 4 + 1] = mat[1 * 4 + 0];
	mat[1 * 4 + 0] = tmp[0];
	tmp[1] = mat[0 * 4 + 2];
	mat[0 * 4 + 2] = mat[2 * 4 + 0];
	mat[2 * 4 + 0] = tmp[1];
	tmp[2] = mat[1 * 4 + 2];
	mat[1 * 4 + 2] = mat[2 * 4 + 1];
	mat[2 * 4 + 1] = tmp[2];
}

/*
=============
aRcMat3x4::LeftMultiply
=============
*/
ARC_INLINE void aRcMat3x4::LeftMultiply( const aRcMat3x4 &m ) {
	float t0 = m.mat[0 * 4 + 0] * mat[0 * 4 + 0] + m.mat[0 * 4 + 1] * mat[1 * 4 + 0] + m.mat[0 * 4 + 2] * mat[2 * 4 + 0];
	float t1 = m.mat[1 * 4 + 0] * mat[0 * 4 + 0] + m.mat[1 * 4 + 1] * mat[1 * 4 + 0] + m.mat[1 * 4 + 2] * mat[2 * 4 + 0];
	mat[2 * 4 + 0]	= m.mat[2 * 4 + 0] * mat[0 * 4 + 0] + m.mat[2 * 4 + 1] * mat[1 * 4 + 0] + m.mat[2 * 4 + 2] * mat[2 * 4 + 0];

	mat[1 * 4 + 0] = t1;
	mat[0 * 4 + 0] = t0;

	float t0 = m.mat[0 * 4 + 0] * mat[0 * 4 + 1] + m.mat[0 * 4 + 1] * mat[1 * 4 + 1] + m.mat[0 * 4 + 2] * mat[2 * 4 + 1];
	float t1 = m.mat[1 * 4 + 0] * mat[0 * 4 + 1] + m.mat[1 * 4 + 1] * mat[1 * 4 + 1] + m.mat[1 * 4 + 2] * mat[2 * 4 + 1];
	mat[2 * 4 + 1]	= m.mat[2 * 4 + 0] * mat[0 * 4 + 1] + m.mat[2 * 4 + 1] * mat[1 * 4 + 1] + m.mat[2 * 4 + 2] * mat[2 * 4 + 1];

	mat[1 * 4 + 1] = t1;
	mat[0 * 4 + 1] = t0;

	float t0 = m.mat[0 * 4 + 0] * mat[0 * 4 + 2] + m.mat[0 * 4 + 1] * mat[1 * 4 + 2] + m.mat[0 * 4 + 2] * mat[2 * 4 + 2];
	float t1 = m.mat[1 * 4 + 0] * mat[0 * 4 + 2] + m.mat[1 * 4 + 1] * mat[1 * 4 + 2] + m.mat[1 * 4 + 2] * mat[2 * 4 + 2];
	mat[2 * 4 + 2]	= m.mat[2 * 4 + 0] * mat[0 * 4 + 2] + m.mat[2 * 4 + 1] * mat[1 * 4 + 2] + m.mat[2 * 4 + 2] * mat[2 * 4 + 2];

	mat[1 * 4 + 2] = t1;
	mat[0 * 4 + 2] = t0;

	float t0 = m.mat[0 * 4 + 0] * mat[0 * 4 + 3] + m.mat[0 * 4 + 1] * mat[1 * 4 + 3] + m.mat[0 * 4 + 2] * mat[2 * 4 + 3] + m.mat[0 * 4 + 3];
	float t1 = m.mat[1 * 4 + 0] * mat[0 * 4 + 3] + m.mat[1 * 4 + 1] * mat[1 * 4 + 3] + m.mat[1 * 4 + 2] * mat[2 * 4 + 3] + m.mat[1 * 4 + 3];
	mat[2 * 4 + 3]	= m.mat[2 * 4 + 0] * mat[0 * 4 + 3] + m.mat[2 * 4 + 1] * mat[1 * 4 + 3] + m.mat[2 * 4 + 2] * mat[2 * 4 + 3] + m.mat[2 * 4 + 3];

	mat[1 * 4 + 3] = t1;
	mat[0 * 4 + 3] = t0;
}

/*
=============
aRcMat3x4::LeftMultiply
=============
*/
ARC_INLINE void aRcMat3x4::LeftMultiply( const arcMat3 &m ) {
	// NOTE: arcMat3 is column-major
	float t0 = m[0][0] * mat[0 * 4 + 0] + m[1][0] * mat[1 * 4 + 0] + m[2][0] * mat[2 * 4 + 0];
	float t1 = m[0][1] * mat[0 * 4 + 0] + m[1][1] * mat[1 * 4 + 0] + m[2][1] * mat[2 * 4 + 0];
	mat[2 * 4 + 0]	= m[0][2] * mat[0 * 4 + 0] + m[1][2] * mat[1 * 4 + 0] + m[2][2] * mat[2 * 4 + 0];

	mat[1 * 4 + 0] = t1;
	mat[0 * 4 + 0] = t0;

	float t0 = m[0][0] * mat[0 * 4 + 1] + m[1][0] * mat[1 * 4 + 1] + m[2][0] * mat[2 * 4 + 1];
	float t1 = m[0][1] * mat[0 * 4 + 1] + m[1][1] * mat[1 * 4 + 1] + m[2][1] * mat[2 * 4 + 1];
	mat[2 * 4 + 1]	= m[0][2] * mat[0 * 4 + 1] + m[1][2] * mat[1 * 4 + 1] + m[2][2] * mat[2 * 4 + 1];

	mat[1 * 4 + 1] = t1;
	mat[0 * 4 + 1] = t0;

	float t0 = m[0][0] * mat[0 * 4 + 2] + m[1][0] * mat[1 * 4 + 2] + m[2][0] * mat[2 * 4 + 2];
	float t1 = m[0][1] * mat[0 * 4 + 2] + m[1][1] * mat[1 * 4 + 2] + m[2][1] * mat[2 * 4 + 2];
	mat[2 * 4 + 2]	= m[0][2] * mat[0 * 4 + 2] + m[1][2] * mat[1 * 4 + 2] + m[2][2] * mat[2 * 4 + 2];

	mat[1 * 4 + 2] = t1;
	mat[0 * 4 + 2] = t0;

	float t0 = m[0][0] * mat[0 * 4 + 3] + m[1][0] * mat[1 * 4 + 3] + m[2][0] * mat[2 * 4 + 3];
	float t1 = m[0][1] * mat[0 * 4 + 3] + m[1][1] * mat[1 * 4 + 3] + m[2][1] * mat[2 * 4 + 3];
	mat[2 * 4 + 3]	= m[0][2] * mat[0 * 4 + 3] + m[1][2] * mat[1 * 4 + 3] + m[2][2] * mat[2 * 4 + 3];

	mat[1 * 4 + 3] = t1;
	mat[0 * 4 + 3] = t0;
}

/*
=============
arcMat3::RightMultiply
=============
*/
ARC_INLINE void aRcMat3x4::RightMultiply( const aRcMat3x4 &m ) {

	float t0 = mat[0 * 4 + 0] * m.mat[0 * 4 + 0] + mat[0 * 4 + 1] * m.mat[1 * 4 + 0] + mat[0 * 4 + 2] * m.mat[2 * 4 + 0];
	float t1 = mat[0 * 4 + 0] * m.mat[0 * 4 + 1] + mat[0 * 4 + 1] * m.mat[1 * 4 + 1] + mat[0 * 4 + 2] * m.mat[2 * 4 + 1];
	float t2 = mat[0 * 4 + 0] * m.mat[0 * 4 + 2] + mat[0 * 4 + 1] * m.mat[1 * 4 + 2] + mat[0 * 4 + 2] * m.mat[2 * 4 + 2];
	mat[0 * 4 + 3]	= mat[0 * 4 + 0] * m.mat[0 * 4 + 3] + mat[0 * 4 + 1] * m.mat[1 * 4 + 3] + mat[0 * 4 + 2] * m.mat[2 * 4 + 3] + mat[0 * 4 + 3];

	mat[0 * 4 + 0] = t0;
	mat[0 * 4 + 1] = t1;
	mat[0 * 4 + 2] = t2;

	float t0 = mat[1 * 4 + 0] * m.mat[0 * 4 + 0] + mat[1 * 4 + 1] * m.mat[1 * 4 + 0] + mat[1 * 4 + 2] * m.mat[2 * 4 + 0];
	float t1 = mat[1 * 4 + 0] * m.mat[0 * 4 + 1] + mat[1 * 4 + 1] * m.mat[1 * 4 + 1] + mat[1 * 4 + 2] * m.mat[2 * 4 + 1];
	float t2 = mat[1 * 4 + 0] * m.mat[0 * 4 + 2] + mat[1 * 4 + 1] * m.mat[1 * 4 + 2] + mat[1 * 4 + 2] * m.mat[2 * 4 + 2];
	mat[1 * 4 + 3]	= mat[1 * 4 + 0] * m.mat[0 * 4 + 3] + mat[1 * 4 + 1] * m.mat[1 * 4 + 3] + mat[1 * 4 + 2] * m.mat[2 * 4 + 3] + mat[1 * 4 + 3];

	mat[1 * 4 + 0] = t0;
	mat[1 * 4 + 1] = t1;
	mat[1 * 4 + 2] = t2;

	float t0 = mat[2 * 4 + 0] * m.mat[0 * 4 + 0] + mat[2 * 4 + 1] * m.mat[1 * 4 + 0] + mat[2 * 4 + 2] * m.mat[2 * 4 + 0];
	float t1 = mat[2 * 4 + 0] * m.mat[0 * 4 + 1] + mat[2 * 4 + 1] * m.mat[1 * 4 + 1] + mat[2 * 4 + 2] * m.mat[2 * 4 + 1];
	float t2 = mat[2 * 4 + 0] * m.mat[0 * 4 + 2] + mat[2 * 4 + 1] * m.mat[1 * 4 + 2] + mat[2 * 4 + 2] * m.mat[2 * 4 + 2];
	mat[2 * 4 + 3]	= mat[2 * 4 + 0] * m.mat[0 * 4 + 3] + mat[2 * 4 + 1] * m.mat[1 * 4 + 3] + mat[2 * 4 + 2] * m.mat[2 * 4 + 3] + mat[2 * 4 + 3];

	mat[2 * 4 + 0] = t0;
	mat[2 * 4 + 1] = t1;
	mat[2 * 4 + 2] = t2;
}

/*
=============
aRcMat3x4::RightMultiply
=============
*/
ARC_INLINE void aRcMat3x4::RightMultiply( const arcMat3 &m ) {
	// NOTE: arcMat3 is column-major
	float t0 = mat[0 * 4 + 0] * m[0][0] + mat[0 * 4 + 1] * m[0][1] + mat[0 * 4 + 2] * m[0][2];
	float t1 = mat[0 * 4 + 0] * m[1][0] + mat[0 * 4 + 1] * m[1][1] + mat[0 * 4 + 2] * m[1][2];
	float t2 = mat[0 * 4 + 0] * m[2][0] + mat[0 * 4 + 1] * m[2][1] + mat[0 * 4 + 2] * m[2][2];

	mat[0 * 4 + 0] = t0;
	mat[0 * 4 + 1] = t1;
	mat[0 * 4 + 2] = t2;

	float t0 = mat[1 * 4 + 0] * m[0][0] + mat[1 * 4 + 1] * m[0][1] + mat[1 * 4 + 2] * m[0][2];
	float t1 = mat[1 * 4 + 0] * m[1][0] + mat[1 * 4 + 1] * m[1][1] + mat[1 * 4 + 2] * m[1][2];
	float t2 = mat[1 * 4 + 0] * m[2][0] + mat[1 * 4 + 1] * m[2][1] + mat[1 * 4 + 2] * m[2][2];

	mat[1 * 4 + 0] = t0;
	mat[1 * 4 + 1] = t1;
	mat[1 * 4 + 2] = t2;

	float t0 = mat[2 * 4 + 0] * m[0][0] + mat[2 * 4 + 1] * m[0][1] + mat[2 * 4 + 2] * m[0][2];
	float t1 = mat[2 * 4 + 0] * m[1][0] + mat[2 * 4 + 1] * m[1][1] + mat[2 * 4 + 2] * m[1][2];
	float t2 = mat[2 * 4 + 0] * m[2][0] + mat[2 * 4 + 1] * m[2][1] + mat[2 * 4 + 2] * m[2][2];

	mat[2 * 4 + 0] = t0;
	mat[2 * 4 + 1] = t1;
	mat[2 * 4 + 2] = t2;
}

/*
=============
aRcMat3x4::Transform
=============
*/
ARC_INLINE void aRcMat3x4::Transform( arcVec3 &result, const arcVec3 &v ) const {
	result.x = mat[0 * 4 + 0] * v.x + mat[0 * 4 + 1] * v.y + mat[0 * 4 + 2] * v.z + mat[0 * 4 + 3];
	result.y = mat[1 * 4 + 0] * v.x + mat[1 * 4 + 1] * v.y + mat[1 * 4 + 2] * v.z + mat[1 * 4 + 3];
	result.z = mat[2 * 4 + 0] * v.x + mat[2 * 4 + 1] * v.y + mat[2 * 4 + 2] * v.z + mat[2 * 4 + 3];
}

/*
=============
aRcMat3x4::Rotate
=============
*/
ARC_INLINE void aRcMat3x4::Rotate( arcVec3 &result, const arcVec3 &v ) const {
	result.x = mat[0 * 4 + 0] * v.x + mat[0 * 4 + 1] * v.y + mat[0 * 4 + 2] * v.z;
	result.y = mat[1 * 4 + 0] * v.x + mat[1 * 4 + 1] * v.y + mat[1 * 4 + 2] * v.z;
	result.z = mat[2 * 4 + 0] * v.x + mat[2 * 4 + 1] * v.y + mat[2 * 4 + 2] * v.z;
}

/*
=============
aRcMat3x4::ToMat3
=============
*/
ARC_INLINE arcMat3 aRcMat3x4::ToMat3( void ) const {
	return arcMat3(	mat[0 * 4 + 0], mat[1 * 4 + 0], mat[2 * 4 + 0],
					mat[0 * 4 + 1], mat[1 * 4 + 1], mat[2 * 4 + 1],
					mat[0 * 4 + 2], mat[1 * 4 + 2], mat[2 * 4 + 2] );
}

/*
=============
aRcMat3x4::ToVec3
=============
*/
ARC_INLINE arcVec3 aRcMat3x4::ToVec3( void ) const {
	return arcVec3( mat[0 * 4 + 3], mat[1 * 4 + 3], mat[2 * 4 + 3] );
}

/*
=============
aRcMat3x4::ToFloatPtr
=============
*/
ARC_INLINE const float *aRcMat3x4::ToFloatPtr( void ) const {
	return mat;
}

ARC_INLINE float *aRcMat3x4::ToFloatPtr( void ) {
	return mat;
}

//===============================================================
//
//	arcMat4 - 4x4 matrix
//
//===============================================================

class arcMat4 {
public:
					arcMat4( void );
					explicit arcMat4( const arcVec4 &x, const arcVec4 &y, const arcVec4 &z, const arcVec4 &w );
					explicit arcMat4(const float xx, const float xy, const float xz, const float xw,
									const float yx, const float yy, const float yz, const float yw,
									const float zx, const float zy, const float zz, const float zw,
									const float wx, const float wy, const float wz, const float ww );
					explicit arcMat4( const arcMat3 &rotation, const arcVec3 &translation );
					explicit arcMat4( const float src[ 4 ][ 4 ] );

	const arcVec4 &	operator[]( int index ) const;
	arcVec4 &		operator[]( int index );
	arcMat4			operator*( const float a ) const;
	arcVec4			operator*( const arcVec4 &vec ) const;
	arcVec3			operator*( const arcVec3 &vec ) const;
	arcMat4			operator*( const arcMat4 &a ) const;
	arcMat4			operator+( const arcMat4 &a ) const;
	arcMat4			operator-( const arcMat4 &a ) const;
	arcMat4 &		operator*=( const float a );
	arcMat4 &		operator*=( const arcMat4 &a );
	arcMat4 &		operator+=( const arcMat4 &a );
	arcMat4 &		operator-=( const arcMat4 &a );

	friend arcMat4	operator*( const float a, const arcMat4 &mat );
	friend arcVec4	operator*( const arcVec4 &vec, const arcMat4 &mat );
	friend arcVec3	operator*( const arcVec3 &vec, const arcMat4 &mat );
	friend arcVec4 &	operator*=( arcVec4 &vec, const arcMat4 &mat );
	friend arcVec3 &	operator*=( arcVec3 &vec, const arcMat4 &mat );

	bool			Compare( const arcMat4 &a ) const;						// exact compare, no epsilon
	bool			Compare( const arcMat4 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const arcMat4 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const arcMat4 &a ) const;					// exact compare, no epsilon

	void			Zero( void );
	void			Identity( void );
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsRotated( void ) const;

	void			ProjectVector( const arcVec4 &src, arcVec4 &dst ) const;
	void			UnprojectVector( const arcVec4 &src, arcVec4 &dst ) const;

	float			Trace( void ) const;
	float			Determinant( void ) const;
	arcMat4			Transpose( void ) const;	// returns transpose
	arcMat4 &		TransposeSelf( void );
	arcMat4			Inverse( void ) const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf( void );		// returns false if determinant is zero
	arcMat4			InverseFast( void ) const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf( void );	// returns false if determinant is zero
	arcMat4			TransposeMultiply( const arcMat4 &b ) const;

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	arcVec4			mat[ 4 ];
};

extern arcMat4 mat4_zero;
extern arcMat4 mat4_identity;
#define mat4_default	mat4_identity

ARC_INLINE arcMat4::arcMat4( void ) {
}

ARC_INLINE arcMat4::arcMat4( const arcVec4 &x, const arcVec4 &y, const arcVec4 &z, const arcVec4 &w ) {
	mat[ 0 ] = x;
	mat[ 1 ] = y;
	mat[ 2 ] = z;
	mat[ 3 ] = w;
}

ARC_INLINE arcMat4::arcMat4( const float xx, const float xy, const float xz, const float xw,
							const float yx, const float yy, const float yz, const float yw,
							const float zx, const float zy, const float zz, const float zw,
							const float wx, const float wy, const float wz, const float ww ) {
	mat[0][0] = xx; mat[0][1] = xy; mat[0][2] = xz; mat[0][3] = xw;
	mat[1][0] = yx; mat[1][1] = yy; mat[1][2] = yz; mat[1][3] = yw;
	mat[2][0] = zx; mat[2][1] = zy; mat[2][2] = zz; mat[2][3] = zw;
	mat[3][0] = wx; mat[3][1] = wy; mat[3][2] = wz; mat[3][3] = ww;
}

ARC_INLINE arcMat4::arcMat4( const arcMat3 &rotation, const arcVec3 &translation ) {
	// NOTE: arcMat3 is transposed because it is column-major
	mat[ 0 ][ 0 ] = rotation[0][0];
	mat[ 0 ][ 1 ] = rotation[1][0];
	mat[ 0 ][ 2 ] = rotation[2][0];
	mat[ 0 ][ 3 ] = translation[0];
	mat[ 1 ][ 0 ] = rotation[0][1];
	mat[ 1 ][ 1 ] = rotation[1][1];
	mat[ 1 ][ 2 ] = rotation[2][1];
	mat[ 1 ][ 3 ] = translation[1];
	mat[ 2 ][ 0 ] = rotation[0][2];
	mat[ 2 ][ 1 ] = rotation[1][2];
	mat[ 2 ][ 2 ] = rotation[2][2];
	mat[ 2 ][ 3 ] = translation[2];
	mat[ 3 ][ 0 ] = 0.0f;
	mat[ 3 ][ 1 ] = 0.0f;
	mat[ 3 ][ 2 ] = 0.0f;
	mat[ 3 ][ 3 ] = 1.0f;
}

ARC_INLINE arcMat4::arcMat4( const float src[ 4 ][ 4 ] ) {
	memcpy( mat, src, 4 * 4 * sizeof( float ) );
}

ARC_INLINE const arcVec4 &arcMat4::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 4 ) );
	return mat[index];
}

ARC_INLINE arcVec4 &arcMat4::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 4 ) );
	return mat[index];
}

ARC_INLINE arcMat4 arcMat4::operator*( const float a ) const {
	return arcMat4(
		mat[0].x * a, mat[0].y * a, mat[0].z * a, mat[0].w * a,
		mat[1].x * a, mat[1].y * a, mat[1].z * a, mat[1].w * a,
		mat[2].x * a, mat[2].y * a, mat[2].z * a, mat[2].w * a,
		mat[3].x * a, mat[3].y * a, mat[3].z * a, mat[3].w * a );
}

ARC_INLINE arcVec4 arcMat4::operator*( const arcVec4 &vec ) const {
	return arcVec4(
		mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w * vec.w,
		mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w * vec.w,
		mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w * vec.w,
		mat[ 3 ].x * vec.x + mat[ 3 ].y * vec.y + mat[ 3 ].z * vec.z + mat[ 3 ].w * vec.w );
}

ARC_INLINE arcVec3 arcMat4::operator*( const arcVec3 &vec ) const {
	float s = mat[ 3 ].x * vec.x + mat[ 3 ].y * vec.y + mat[ 3 ].z * vec.z + mat[ 3 ].w;
	if ( s == 0.0f ) {
		return arcVec3( 0.0f, 0.0f, 0.0f );
	}
	if ( s == 1.0f ) {
		return arcVec3(
			mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w,
			mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w,
			mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w );
	}
	else {
		float invS = 1.0f / s;
		return arcVec3(
			(mat[ 0 ].x * vec.x + mat[ 0 ].y * vec.y + mat[ 0 ].z * vec.z + mat[ 0 ].w) * invS,
			(mat[ 1 ].x * vec.x + mat[ 1 ].y * vec.y + mat[ 1 ].z * vec.z + mat[ 1 ].w) * invS,
			(mat[ 2 ].x * vec.x + mat[ 2 ].y * vec.y + mat[ 2 ].z * vec.z + mat[ 2 ].w) * invS );
	}
}

ARC_INLINE arcMat4 arcMat4::operator*( const arcMat4 &a ) const {
	int i, j;
	const float *m1Ptr, *m2Ptr;
	float *dstPtr;
	arcMat4 dst;

	m1Ptr = reinterpret_cast<const float *>( this );
	m2Ptr = reinterpret_cast<const float *>(&a);
	dstPtr = reinterpret_cast<float *>(&dst);

	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 4 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 4 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 4 + j ]
					+ m1Ptr[3] * m2Ptr[ 3 * 4 + j ];
			dstPtr++;
		}
		m1Ptr += 4;
	}
	return dst;
}

ARC_INLINE arcMat4 arcMat4::operator+( const arcMat4 &a ) const {
	return arcMat4(
		mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z, mat[0].w + a[0].w,
		mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z, mat[1].w + a[1].w,
		mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z, mat[2].w + a[2].w,
		mat[3].x + a[3].x, mat[3].y + a[3].y, mat[3].z + a[3].z, mat[3].w + a[3].w );
}

ARC_INLINE arcMat4 arcMat4::operator-( const arcMat4 &a ) const {
	return arcMat4(
		mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z, mat[0].w - a[0].w,
		mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z, mat[1].w - a[1].w,
		mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z, mat[2].w - a[2].w,
		mat[3].x - a[3].x, mat[3].y - a[3].y, mat[3].z - a[3].z, mat[3].w - a[3].w );
}

ARC_INLINE arcMat4 &arcMat4::operator*=( const float a ) {
	mat[0].x *= a; mat[0].y *= a; mat[0].z *= a; mat[0].w *= a;
	mat[1].x *= a; mat[1].y *= a; mat[1].z *= a; mat[1].w *= a;
	mat[2].x *= a; mat[2].y *= a; mat[2].z *= a; mat[2].w *= a;
	mat[3].x *= a; mat[3].y *= a; mat[3].z *= a; mat[3].w *= a;
    return *this;
}

ARC_INLINE arcMat4 &arcMat4::operator*=( const arcMat4 &a ) {
	*this = (*this) * a;
	return *this;
}

ARC_INLINE arcMat4 &arcMat4::operator+=( const arcMat4 &a ) {
	mat[0].x += a[0].x; mat[0].y += a[0].y; mat[0].z += a[0].z; mat[0].w += a[0].w;
	mat[1].x += a[1].x; mat[1].y += a[1].y; mat[1].z += a[1].z; mat[1].w += a[1].w;
	mat[2].x += a[2].x; mat[2].y += a[2].y; mat[2].z += a[2].z; mat[2].w += a[2].w;
	mat[3].x += a[3].x; mat[3].y += a[3].y; mat[3].z += a[3].z; mat[3].w += a[3].w;
    return *this;
}

ARC_INLINE arcMat4 &arcMat4::operator-=( const arcMat4 &a ) {
	mat[0].x -= a[0].x; mat[0].y -= a[0].y; mat[0].z -= a[0].z; mat[0].w -= a[0].w;
	mat[1].x -= a[1].x; mat[1].y -= a[1].y; mat[1].z -= a[1].z; mat[1].w -= a[1].w;
	mat[2].x -= a[2].x; mat[2].y -= a[2].y; mat[2].z -= a[2].z; mat[2].w -= a[2].w;
	mat[3].x -= a[3].x; mat[3].y -= a[3].y; mat[3].z -= a[3].z; mat[3].w -= a[3].w;
    return *this;
}

ARC_INLINE arcMat4 operator*( const float a, const arcMat4 &mat ) {
	return mat * a;
}

ARC_INLINE arcVec4 operator*( const arcVec4 &vec, const arcMat4 &mat ) {
	return mat * vec;
}

ARC_INLINE arcVec3 operator*( const arcVec3 &vec, const arcMat4 &mat ) {
	return mat * vec;
}

ARC_INLINE arcVec4 &operator*=( arcVec4 &vec, const arcMat4 &mat ) {
	vec = mat * vec;
	return vec;
}

ARC_INLINE arcVec3 &operator*=( arcVec3 &vec, const arcMat4 &mat ) {
	vec = mat * vec;
	return vec;
}

ARC_INLINE bool arcMat4::Compare( const arcMat4 &a ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 4*4; i++ ) {
		if ( ptr1[i] != ptr2[i] ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcMat4::Compare( const arcMat4 &a, const float epsilon ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 4*4; i++ ) {
		if ( arcMath::Fabs( ptr1[i] - ptr2[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcMat4::operator==( const arcMat4 &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcMat4::operator!=( const arcMat4 &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcMat4::Zero( void ) {
	memset( mat, 0, sizeof( arcMat4 ) );
}

ARC_INLINE void arcMat4::Identity( void ) {
	*this = mat4_identity;
}

ARC_INLINE bool arcMat4::IsIdentity( const float epsilon ) const {
	return Compare( mat4_identity, epsilon );
}

ARC_INLINE bool arcMat4::IsSymmetric( const float epsilon ) const {
	for ( int i = 1; i < 4; i++ ) {
		for ( int j = 0; j < i; j++ ) {
			if ( arcMath::Fabs( mat[i][j] - mat[j][i] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE bool arcMat4::IsDiagonal( const float epsilon ) const {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			if ( i != j && arcMath::Fabs( mat[i][j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE bool arcMat4::IsRotated( void ) const {
	if ( !mat[ 0 ][ 1 ] && !mat[ 0 ][ 2 ] &&
		!mat[ 1 ][ 0 ] && !mat[ 1 ][ 2 ] &&
		!mat[ 2 ][ 0 ] && !mat[ 2 ][ 1 ] ) {
		return false;
	}
	return true;
}

ARC_INLINE void arcMat4::ProjectVector( const arcVec4 &src, arcVec4 &dst ) const {
	dst.x = src * mat[ 0 ];
	dst.y = src * mat[ 1 ];
	dst.z = src * mat[ 2 ];
	dst.w = src * mat[ 3 ];
}

ARC_INLINE void arcMat4::UnprojectVector( const arcVec4 &src, arcVec4 &dst ) const {
	dst = mat[ 0 ] * src.x + mat[ 1 ] * src.y + mat[ 2 ] * src.z + mat[ 3 ] * src.w;
}

ARC_INLINE float arcMat4::Trace( void ) const {
	return ( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] );
}

ARC_INLINE arcMat4 arcMat4::Inverse( void ) const {
	arcMat4 invMat;

	invMat = *this;
	int r = invMat.InverseSelf();
	assert( r );
	return invMat;
}

ARC_INLINE arcMat4 arcMat4::InverseFast( void ) const {
	arcMat4 invMat;

	invMat = *this;
	int r = invMat.InverseFastSelf();
	assert( r );
	return invMat;
}

ARC_INLINE arcMat4 arcMat3::ToMat4( void ) const {
	// NOTE: arcMat3 is transposed because it is column-major
	return arcMat4(	mat[0][0],	mat[1][0],	mat[2][0],	0.0f,
					mat[0][1],	mat[1][1],	mat[2][1],	0.0f,
					mat[0][2],	mat[1][2],	mat[2][2],	0.0f,
					0.0f,		0.0f,		0.0f,		1.0f );
}

ARC_INLINE int arcMat4::GetDimension( void ) const {
	return 16;
}

ARC_INLINE const float *arcMat4::ToFloatPtr( void ) const {
	return mat[0].ToFloatPtr();
}

ARC_INLINE float *arcMat4::ToFloatPtr( void ) {
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	arcMat5 - 5x5 matrix
//
//===============================================================

class arcMat5 {
public:
					arcMat5( void );
					explicit arcMat5( const arcVec5 &v0, const arcVec5 &v1, const arcVec5 &v2, const arcVec5 &v3, const arcVec5 &v4 );
					explicit arcMat5( const float src[ 5 ][ 5 ] );

	const arcVec5 &	operator[]( int index ) const;
	arcVec5 &		operator[]( int index );
	arcMat5			operator*( const float a ) const;
	arcVec5			operator*( const arcVec5 &vec ) const;
	arcMat5			operator*( const arcMat5 &a ) const;
	arcMat5			operator+( const arcMat5 &a ) const;
	arcMat5			operator-( const arcMat5 &a ) const;
	arcMat5 &		operator*=( const float a );
	arcMat5 &		operator*=( const arcMat5 &a );
	arcMat5 &		operator+=( const arcMat5 &a );
	arcMat5 &		operator-=( const arcMat5 &a );

	friend arcMat5	operator*( const float a, const arcMat5 &mat );
	friend arcVec5	operator*( const arcVec5 &vec, const arcMat5 &mat );
	friend arcVec5 &	operator*=( arcVec5 &vec, const arcMat5 &mat );

	bool			Compare( const arcMat5 &a ) const;						// exact compare, no epsilon
	bool			Compare( const arcMat5 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const arcMat5 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const arcMat5 &a ) const;					// exact compare, no epsilon

	void			Zero( void );
	void			Identity( void );
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;

	float			Trace( void ) const;
	float			Determinant( void ) const;
	arcMat5			Transpose( void ) const;	// returns transpose
	arcMat5 &		TransposeSelf( void );
	arcMat5			Inverse( void ) const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf( void );		// returns false if determinant is zero
	arcMat5			InverseFast( void ) const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf( void );	// returns false if determinant is zero

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	arcVec5			mat[ 5 ];
};

extern arcMat5 mat5_zero;
extern arcMat5 mat5_identity;
#define mat5_default	mat5_identity

ARC_INLINE arcMat5::arcMat5( void ) {
}

ARC_INLINE arcMat5::arcMat5( const float src[ 5 ][ 5 ] ) {
	memcpy( mat, src, 5 * 5 * sizeof( float ) );
}

ARC_INLINE arcMat5::arcMat5( const arcVec5 &v0, const arcVec5 &v1, const arcVec5 &v2, const arcVec5 &v3, const arcVec5 &v4 ) {
	mat[0] = v0;
	mat[1] = v1;
	mat[2] = v2;
	mat[3] = v3;
	mat[4] = v4;
}

ARC_INLINE const arcVec5 &arcMat5::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 5 ) );
	return mat[index];
}

ARC_INLINE arcVec5 &arcMat5::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 5 ) );
	return mat[index];
}

ARC_INLINE arcMat5 arcMat5::operator*( const arcMat5 &a ) const {
	int i, j;
	const float *m1Ptr, *m2Ptr;
	float *dstPtr;
	arcMat5 dst;

	m1Ptr = reinterpret_cast<const float *>( this );
	m2Ptr = reinterpret_cast<const float *>(&a);
	dstPtr = reinterpret_cast<float *>(&dst);

	for ( i = 0; i < 5; i++ ) {
		for ( j = 0; j < 5; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 5 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 5 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 5 + j ]
					+ m1Ptr[3] * m2Ptr[ 3 * 5 + j ]
					+ m1Ptr[4] * m2Ptr[ 4 * 5 + j ];
			dstPtr++;
		}
		m1Ptr += 5;
	}
	return dst;
}

ARC_INLINE arcMat5 arcMat5::operator*( const float a ) const {
	return arcMat5(
		arcVec5( mat[0][0] * a, mat[0][1] * a, mat[0][2] * a, mat[0][3] * a, mat[0][4] * a ),
		arcVec5( mat[1][0] * a, mat[1][1] * a, mat[1][2] * a, mat[1][3] * a, mat[1][4] * a ),
		arcVec5( mat[2][0] * a, mat[2][1] * a, mat[2][2] * a, mat[2][3] * a, mat[2][4] * a ),
		arcVec5( mat[3][0] * a, mat[3][1] * a, mat[3][2] * a, mat[3][3] * a, mat[3][4] * a ),
		arcVec5( mat[4][0] * a, mat[4][1] * a, mat[4][2] * a, mat[4][3] * a, mat[4][4] * a ) );
}

ARC_INLINE arcVec5 arcMat5::operator*( const arcVec5 &vec ) const {
	return arcVec5(
		mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3] + mat[0][4] * vec[4],
		mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3] + mat[1][4] * vec[4],
		mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3] + mat[2][4] * vec[4],
		mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3] * vec[3] + mat[3][4] * vec[4],
		mat[4][0] * vec[0] + mat[4][1] * vec[1] + mat[4][2] * vec[2] + mat[4][3] * vec[3] + mat[4][4] * vec[4] );
}

ARC_INLINE arcMat5 arcMat5::operator+( const arcMat5 &a ) const {
	return arcMat5(
		arcVec5( mat[0][0] + a[0][0], mat[0][1] + a[0][1], mat[0][2] + a[0][2], mat[0][3] + a[0][3], mat[0][4] + a[0][4] ),
		arcVec5( mat[1][0] + a[1][0], mat[1][1] + a[1][1], mat[1][2] + a[1][2], mat[1][3] + a[1][3], mat[1][4] + a[1][4] ),
		arcVec5( mat[2][0] + a[2][0], mat[2][1] + a[2][1], mat[2][2] + a[2][2], mat[2][3] + a[2][3], mat[2][4] + a[2][4] ),
		arcVec5( mat[3][0] + a[3][0], mat[3][1] + a[3][1], mat[3][2] + a[3][2], mat[3][3] + a[3][3], mat[3][4] + a[3][4] ),
		arcVec5( mat[4][0] + a[4][0], mat[4][1] + a[4][1], mat[4][2] + a[4][2], mat[4][3] + a[4][3], mat[4][4] + a[4][4] ) );
}

ARC_INLINE arcMat5 arcMat5::operator-( const arcMat5 &a ) const {
	return arcMat5(
		arcVec5( mat[0][0] - a[0][0], mat[0][1] - a[0][1], mat[0][2] - a[0][2], mat[0][3] - a[0][3], mat[0][4] - a[0][4] ),
		arcVec5( mat[1][0] - a[1][0], mat[1][1] - a[1][1], mat[1][2] - a[1][2], mat[1][3] - a[1][3], mat[1][4] - a[1][4] ),
		arcVec5( mat[2][0] - a[2][0], mat[2][1] - a[2][1], mat[2][2] - a[2][2], mat[2][3] - a[2][3], mat[2][4] - a[2][4] ),
		arcVec5( mat[3][0] - a[3][0], mat[3][1] - a[3][1], mat[3][2] - a[3][2], mat[3][3] - a[3][3], mat[3][4] - a[3][4] ),
		arcVec5( mat[4][0] - a[4][0], mat[4][1] - a[4][1], mat[4][2] - a[4][2], mat[4][3] - a[4][3], mat[4][4] - a[4][4] ) );
}

ARC_INLINE arcMat5 &arcMat5::operator*=( const float a ) {
	mat[0][0] *= a; mat[0][1] *= a; mat[0][2] *= a; mat[0][3] *= a; mat[0][4] *= a;
	mat[1][0] *= a; mat[1][1] *= a; mat[1][2] *= a; mat[1][3] *= a; mat[1][4] *= a;
	mat[2][0] *= a; mat[2][1] *= a; mat[2][2] *= a; mat[2][3] *= a; mat[2][4] *= a;
	mat[3][0] *= a; mat[3][1] *= a; mat[3][2] *= a; mat[3][3] *= a; mat[3][4] *= a;
	mat[4][0] *= a; mat[4][1] *= a; mat[4][2] *= a; mat[4][3] *= a; mat[4][4] *= a;
	return *this;
}

ARC_INLINE arcMat5 &arcMat5::operator*=( const arcMat5 &a ) {
	*this = *this * a;
	return *this;
}

ARC_INLINE arcMat5 &arcMat5::operator+=( const arcMat5 &a ) {
	mat[0][0] += a[0][0]; mat[0][1] += a[0][1]; mat[0][2] += a[0][2]; mat[0][3] += a[0][3]; mat[0][4] += a[0][4];
	mat[1][0] += a[1][0]; mat[1][1] += a[1][1]; mat[1][2] += a[1][2]; mat[1][3] += a[1][3]; mat[1][4] += a[1][4];
	mat[2][0] += a[2][0]; mat[2][1] += a[2][1]; mat[2][2] += a[2][2]; mat[2][3] += a[2][3]; mat[2][4] += a[2][4];
	mat[3][0] += a[3][0]; mat[3][1] += a[3][1]; mat[3][2] += a[3][2]; mat[3][3] += a[3][3]; mat[3][4] += a[3][4];
	mat[4][0] += a[4][0]; mat[4][1] += a[4][1]; mat[4][2] += a[4][2]; mat[4][3] += a[4][3]; mat[4][4] += a[4][4];
	return *this;
}

ARC_INLINE arcMat5 &arcMat5::operator-=( const arcMat5 &a ) {
	mat[0][0] -= a[0][0]; mat[0][1] -= a[0][1]; mat[0][2] -= a[0][2]; mat[0][3] -= a[0][3]; mat[0][4] -= a[0][4];
	mat[1][0] -= a[1][0]; mat[1][1] -= a[1][1]; mat[1][2] -= a[1][2]; mat[1][3] -= a[1][3]; mat[1][4] -= a[1][4];
	mat[2][0] -= a[2][0]; mat[2][1] -= a[2][1]; mat[2][2] -= a[2][2]; mat[2][3] -= a[2][3]; mat[2][4] -= a[2][4];
	mat[3][0] -= a[3][0]; mat[3][1] -= a[3][1]; mat[3][2] -= a[3][2]; mat[3][3] -= a[3][3]; mat[3][4] -= a[3][4];
	mat[4][0] -= a[4][0]; mat[4][1] -= a[4][1]; mat[4][2] -= a[4][2]; mat[4][3] -= a[4][3]; mat[4][4] -= a[4][4];
	return *this;
}

ARC_INLINE arcVec5 operator*( const arcVec5 &vec, const arcMat5 &mat ) {
	return mat * vec;
}

ARC_INLINE arcMat5 operator*( const float a, arcMat5 const &mat ) {
	return mat * a;
}

ARC_INLINE arcVec5 &operator*=( arcVec5 &vec, const arcMat5 &mat ) {
	vec = mat * vec;
	return vec;
}

ARC_INLINE bool arcMat5::Compare( const arcMat5 &a ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 5*5; i++ ) {
		if ( ptr1[i] != ptr2[i] ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcMat5::Compare( const arcMat5 &a, const float epsilon ) const {
	dword i;
	const float *ptr1, *ptr2;

	ptr1 = reinterpret_cast<const float *>(mat);
	ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( i = 0; i < 5*5; i++ ) {
		if ( arcMath::Fabs( ptr1[i] - ptr2[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcMat5::operator==( const arcMat5 &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcMat5::operator!=( const arcMat5 &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcMat5::Zero( void ) {
	memset( mat, 0, sizeof( arcMat5 ) );
}

ARC_INLINE void arcMat5::Identity( void ) {
	*this = mat5_identity;
}

ARC_INLINE bool arcMat5::IsIdentity( const float epsilon ) const {
	return Compare( mat5_identity, epsilon );
}

ARC_INLINE bool arcMat5::IsSymmetric( const float epsilon ) const {
	for ( int i = 1; i < 5; i++ ) {
		for ( int j = 0; j < i; j++ ) {
			if ( arcMath::Fabs( mat[i][j] - mat[j][i] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE bool arcMat5::IsDiagonal( const float epsilon ) const {
	for ( int i = 0; i < 5; i++ ) {
		for ( int j = 0; j < 5; j++ ) {
			if ( i != j && arcMath::Fabs( mat[i][j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE float arcMat5::Trace( void ) const {
	return ( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] + mat[4][4] );
}

ARC_INLINE arcMat5 arcMat5::Inverse( void ) const {
	arcMat5 invMat;

	invMat = *this;
	int r = invMat.InverseSelf();
	assert( r );
	return invMat;
}

ARC_INLINE arcMat5 arcMat5::InverseFast( void ) const {
	arcMat5 invMat;

	invMat = *this;
	int r = invMat.InverseFastSelf();
	assert( r );
	return invMat;
}

ARC_INLINE int arcMat5::GetDimension( void ) const {
	return 25;
}

ARC_INLINE const float *arcMat5::ToFloatPtr( void ) const {
	return mat[0].ToFloatPtr();
}

ARC_INLINE float *arcMat5::ToFloatPtr( void ) {
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	arcMat6 - 6x6 matrix
//
//===============================================================

class arcMat6 {
public:
					arcMat6( void );
					explicit arcMat6( const arcVec6 &v0, const arcVec6 &v1, const arcVec6 &v2, const arcVec6 &v3, const arcVec6 &v4, const arcVec6 &v5 );
					explicit arcMat6( const arcMat3 &m0, const arcMat3 &m1, const arcMat3 &m2, const arcMat3 &m3 );
					explicit arcMat6( const float src[ 6 ][ 6 ] );

	const arcVec6 &	operator[]( int index ) const;
	arcVec6 &		operator[]( int index );
	arcMat6			operator*( const float a ) const;
	arcVec6			operator*( const arcVec6 &vec ) const;
	arcMat6			operator*( const arcMat6 &a ) const;
	arcMat6			operator+( const arcMat6 &a ) const;
	arcMat6			operator-( const arcMat6 &a ) const;
	arcMat6 &		operator*=( const float a );
	arcMat6 &		operator*=( const arcMat6 &a );
	arcMat6 &		operator+=( const arcMat6 &a );
	arcMat6 &		operator-=( const arcMat6 &a );

	friend arcMat6	operator*( const float a, const arcMat6 &mat );
	friend arcVec6	operator*( const arcVec6 &vec, const arcMat6 &mat );
	friend arcVec6 &	operator*=( arcVec6 &vec, const arcMat6 &mat );

	bool			Compare( const arcMat6 &a ) const;						// exact compare, no epsilon
	bool			Compare( const arcMat6 &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const arcMat6 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const arcMat6 &a ) const;					// exact compare, no epsilon

	void			Zero( void );
	void			Identity( void );
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;

	arcMat3			SubMat3( int n ) const;
	float			Trace( void ) const;
	float			Determinant( void ) const;
	arcMat6			Transpose( void ) const;	// returns transpose
	arcMat6 &		TransposeSelf( void );
	arcMat6			Inverse( void ) const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf( void );		// returns false if determinant is zero
	arcMat6			InverseFast( void ) const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf( void );	// returns false if determinant is zero

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	arcVec6			mat[ 6 ];
};

extern arcMat6 mat6_zero;
extern arcMat6 mat6_identity;
#define mat6_default	mat6_identity

ARC_INLINE arcMat6::arcMat6( void ) {
}

ARC_INLINE arcMat6::arcMat6( const arcMat3 &m0, const arcMat3 &m1, const arcMat3 &m2, const arcMat3 &m3 ) {
	mat[0] = arcVec6( m0[0][0], m0[0][1], m0[0][2], m1[0][0], m1[0][1], m1[0][2] );
	mat[1] = arcVec6( m0[1][0], m0[1][1], m0[1][2], m1[1][0], m1[1][1], m1[1][2] );
	mat[2] = arcVec6( m0[2][0], m0[2][1], m0[2][2], m1[2][0], m1[2][1], m1[2][2] );
	mat[3] = arcVec6( m2[0][0], m2[0][1], m2[0][2], m3[0][0], m3[0][1], m3[0][2] );
	mat[4] = arcVec6( m2[1][0], m2[1][1], m2[1][2], m3[1][0], m3[1][1], m3[1][2] );
	mat[5] = arcVec6( m2[2][0], m2[2][1], m2[2][2], m3[2][0], m3[2][1], m3[2][2] );
}

ARC_INLINE arcMat6::arcMat6( const arcVec6 &v0, const arcVec6 &v1, const arcVec6 &v2, const arcVec6 &v3, const arcVec6 &v4, const arcVec6 &v5 ) {
	mat[0] = v0;
	mat[1] = v1;
	mat[2] = v2;
	mat[3] = v3;
	mat[4] = v4;
	mat[5] = v5;
}

ARC_INLINE arcMat6::arcMat6( const float src[ 6 ][ 6 ] ) {
	memcpy( mat, src, 6 * 6 * sizeof( float ) );
}

ARC_INLINE const arcVec6 &arcMat6::operator[]( int index ) const {
	//assert( ( index >= 0 ) && ( index < 6 ) );
	return mat[index];
}

ARC_INLINE arcVec6 &arcMat6::operator[]( int index ) {
	//assert( ( index >= 0 ) && ( index < 6 ) );
	return mat[index];
}

ARC_INLINE arcMat6 arcMat6::operator*( const arcMat6 &a ) const {
	int i, j;
	const float *m1Ptr, *m2Ptr;
	float *dstPtr;
	arcMat6 dst;

	m1Ptr = reinterpret_cast<const float *>( this );
	m2Ptr = reinterpret_cast<const float *>(&a);
	dstPtr = reinterpret_cast<float *>(&dst);

	for ( i = 0; i < 6; i++ ) {
		for ( j = 0; j < 6; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 6 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 6 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 6 + j ]
					+ m1Ptr[3] * m2Ptr[ 3 * 6 + j ]
					+ m1Ptr[4] * m2Ptr[ 4 * 6 + j ]
					+ m1Ptr[5] * m2Ptr[ 5 * 6 + j ];
			dstPtr++;
		}
		m1Ptr += 6;
	}
	return dst;
}

ARC_INLINE arcMat6 arcMat6::operator*( const float a ) const {
	return arcMat6(
		arcVec6( mat[0][0] * a, mat[0][1] * a, mat[0][2] * a, mat[0][3] * a, mat[0][4] * a, mat[0][5] * a ),
		arcVec6( mat[1][0] * a, mat[1][1] * a, mat[1][2] * a, mat[1][3] * a, mat[1][4] * a, mat[1][5] * a ),
		arcVec6( mat[2][0] * a, mat[2][1] * a, mat[2][2] * a, mat[2][3] * a, mat[2][4] * a, mat[2][5] * a ),
		arcVec6( mat[3][0] * a, mat[3][1] * a, mat[3][2] * a, mat[3][3] * a, mat[3][4] * a, mat[3][5] * a ),
		arcVec6( mat[4][0] * a, mat[4][1] * a, mat[4][2] * a, mat[4][3] * a, mat[4][4] * a, mat[4][5] * a ),
		arcVec6( mat[5][0] * a, mat[5][1] * a, mat[5][2] * a, mat[5][3] * a, mat[5][4] * a, mat[5][5] * a ) );
}

ARC_INLINE arcVec6 arcMat6::operator*( const arcVec6 &vec ) const {
	return arcVec6(
		mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3] + mat[0][4] * vec[4] + mat[0][5] * vec[5],
		mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3] + mat[1][4] * vec[4] + mat[1][5] * vec[5],
		mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3] + mat[2][4] * vec[4] + mat[2][5] * vec[5],
		mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3] * vec[3] + mat[3][4] * vec[4] + mat[3][5] * vec[5],
		mat[4][0] * vec[0] + mat[4][1] * vec[1] + mat[4][2] * vec[2] + mat[4][3] * vec[3] + mat[4][4] * vec[4] + mat[4][5] * vec[5],
		mat[5][0] * vec[0] + mat[5][1] * vec[1] + mat[5][2] * vec[2] + mat[5][3] * vec[3] + mat[5][4] * vec[4] + mat[5][5] * vec[5] );
}

ARC_INLINE arcMat6 arcMat6::operator+( const arcMat6 &a ) const {
	return arcMat6(
		arcVec6( mat[0][0] + a[0][0], mat[0][1] + a[0][1], mat[0][2] + a[0][2], mat[0][3] + a[0][3], mat[0][4] + a[0][4], mat[0][5] + a[0][5] ),
		arcVec6( mat[1][0] + a[1][0], mat[1][1] + a[1][1], mat[1][2] + a[1][2], mat[1][3] + a[1][3], mat[1][4] + a[1][4], mat[1][5] + a[1][5] ),
		arcVec6( mat[2][0] + a[2][0], mat[2][1] + a[2][1], mat[2][2] + a[2][2], mat[2][3] + a[2][3], mat[2][4] + a[2][4], mat[2][5] + a[2][5] ),
		arcVec6( mat[3][0] + a[3][0], mat[3][1] + a[3][1], mat[3][2] + a[3][2], mat[3][3] + a[3][3], mat[3][4] + a[3][4], mat[3][5] + a[3][5] ),
		arcVec6( mat[4][0] + a[4][0], mat[4][1] + a[4][1], mat[4][2] + a[4][2], mat[4][3] + a[4][3], mat[4][4] + a[4][4], mat[4][5] + a[4][5] ),
		arcVec6( mat[5][0] + a[5][0], mat[5][1] + a[5][1], mat[5][2] + a[5][2], mat[5][3] + a[5][3], mat[5][4] + a[5][4], mat[5][5] + a[5][5] ) );
}

ARC_INLINE arcMat6 arcMat6::operator-( const arcMat6 &a ) const {
	return arcMat6(
		arcVec6( mat[0][0] - a[0][0], mat[0][1] - a[0][1], mat[0][2] - a[0][2], mat[0][3] - a[0][3], mat[0][4] - a[0][4], mat[0][5] - a[0][5] ),
		arcVec6( mat[1][0] - a[1][0], mat[1][1] - a[1][1], mat[1][2] - a[1][2], mat[1][3] - a[1][3], mat[1][4] - a[1][4], mat[1][5] - a[1][5] ),
		arcVec6( mat[2][0] - a[2][0], mat[2][1] - a[2][1], mat[2][2] - a[2][2], mat[2][3] - a[2][3], mat[2][4] - a[2][4], mat[2][5] - a[2][5] ),
		arcVec6( mat[3][0] - a[3][0], mat[3][1] - a[3][1], mat[3][2] - a[3][2], mat[3][3] - a[3][3], mat[3][4] - a[3][4], mat[3][5] - a[3][5] ),
		arcVec6( mat[4][0] - a[4][0], mat[4][1] - a[4][1], mat[4][2] - a[4][2], mat[4][3] - a[4][3], mat[4][4] - a[4][4], mat[4][5] - a[4][5] ),
		arcVec6( mat[5][0] - a[5][0], mat[5][1] - a[5][1], mat[5][2] - a[5][2], mat[5][3] - a[5][3], mat[5][4] - a[5][4], mat[5][5] - a[5][5] ) );
}

ARC_INLINE arcMat6 &arcMat6::operator*=( const float a ) {
	mat[0][0] *= a; mat[0][1] *= a; mat[0][2] *= a; mat[0][3] *= a; mat[0][4] *= a; mat[0][5] *= a;
	mat[1][0] *= a; mat[1][1] *= a; mat[1][2] *= a; mat[1][3] *= a; mat[1][4] *= a; mat[1][5] *= a;
	mat[2][0] *= a; mat[2][1] *= a; mat[2][2] *= a; mat[2][3] *= a; mat[2][4] *= a; mat[2][5] *= a;
	mat[3][0] *= a; mat[3][1] *= a; mat[3][2] *= a; mat[3][3] *= a; mat[3][4] *= a; mat[3][5] *= a;
	mat[4][0] *= a; mat[4][1] *= a; mat[4][2] *= a; mat[4][3] *= a; mat[4][4] *= a; mat[4][5] *= a;
	mat[5][0] *= a; mat[5][1] *= a; mat[5][2] *= a; mat[5][3] *= a; mat[5][4] *= a; mat[5][5] *= a;
	return *this;
}

ARC_INLINE arcMat6 &arcMat6::operator*=( const arcMat6 &a ) {
	*this = *this * a;
	return *this;
}

ARC_INLINE arcMat6 &arcMat6::operator+=( const arcMat6 &a ) {
	mat[0][0] += a[0][0]; mat[0][1] += a[0][1]; mat[0][2] += a[0][2]; mat[0][3] += a[0][3]; mat[0][4] += a[0][4]; mat[0][5] += a[0][5];
	mat[1][0] += a[1][0]; mat[1][1] += a[1][1]; mat[1][2] += a[1][2]; mat[1][3] += a[1][3]; mat[1][4] += a[1][4]; mat[1][5] += a[1][5];
	mat[2][0] += a[2][0]; mat[2][1] += a[2][1]; mat[2][2] += a[2][2]; mat[2][3] += a[2][3]; mat[2][4] += a[2][4]; mat[2][5] += a[2][5];
	mat[3][0] += a[3][0]; mat[3][1] += a[3][1]; mat[3][2] += a[3][2]; mat[3][3] += a[3][3]; mat[3][4] += a[3][4]; mat[3][5] += a[3][5];
	mat[4][0] += a[4][0]; mat[4][1] += a[4][1]; mat[4][2] += a[4][2]; mat[4][3] += a[4][3]; mat[4][4] += a[4][4]; mat[4][5] += a[4][5];
	mat[5][0] += a[5][0]; mat[5][1] += a[5][1]; mat[5][2] += a[5][2]; mat[5][3] += a[5][3]; mat[5][4] += a[5][4]; mat[5][5] += a[5][5];
	return *this;
}

ARC_INLINE arcMat6 &arcMat6::operator-=( const arcMat6 &a ) {
	mat[0][0] -= a[0][0]; mat[0][1] -= a[0][1]; mat[0][2] -= a[0][2]; mat[0][3] -= a[0][3]; mat[0][4] -= a[0][4]; mat[0][5] -= a[0][5];
	mat[1][0] -= a[1][0]; mat[1][1] -= a[1][1]; mat[1][2] -= a[1][2]; mat[1][3] -= a[1][3]; mat[1][4] -= a[1][4]; mat[1][5] -= a[1][5];
	mat[2][0] -= a[2][0]; mat[2][1] -= a[2][1]; mat[2][2] -= a[2][2]; mat[2][3] -= a[2][3]; mat[2][4] -= a[2][4]; mat[2][5] -= a[2][5];
	mat[3][0] -= a[3][0]; mat[3][1] -= a[3][1]; mat[3][2] -= a[3][2]; mat[3][3] -= a[3][3]; mat[3][4] -= a[3][4]; mat[3][5] -= a[3][5];
	mat[4][0] -= a[4][0]; mat[4][1] -= a[4][1]; mat[4][2] -= a[4][2]; mat[4][3] -= a[4][3]; mat[4][4] -= a[4][4]; mat[4][5] -= a[4][5];
	mat[5][0] -= a[5][0]; mat[5][1] -= a[5][1]; mat[5][2] -= a[5][2]; mat[5][3] -= a[5][3]; mat[5][4] -= a[5][4]; mat[5][5] -= a[5][5];
	return *this;
}

ARC_INLINE arcVec6 operator*( const arcVec6 &vec, const arcMat6 &mat ) {
	return mat * vec;
}

ARC_INLINE arcMat6 operator*( const float a, arcMat6 const &mat ) {
	return mat * a;
}

ARC_INLINE arcVec6 &operator*=( arcVec6 &vec, const arcMat6 &mat ) {
	vec = mat * vec;
	return vec;
}

ARC_INLINE bool arcMat6::Compare( const arcMat6 &a ) const {
	const float *ptr1 = reinterpret_cast<const float *>(mat);
	const float *ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( dword i = 0; i < 6*6; i++ ) {
		if ( ptr1[i] != ptr2[i] ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcMat6::Compare( const arcMat6 &a, const float epsilon ) const {
	const float *ptr1 = reinterpret_cast<const float *>(mat);
	const float *ptr2 = reinterpret_cast<const float *>(a.mat);
	for ( dword i = 0; i < 6*6; i++ ) {
		if ( arcMath::Fabs( ptr1[i] - ptr2[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcMat6::operator==( const arcMat6 &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcMat6::operator!=( const arcMat6 &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcMat6::Zero( void ) {
	memset( mat, 0, sizeof( arcMat6 ) );
}

ARC_INLINE void arcMat6::Identity( void ) {
	*this = mat6_identity;
}

ARC_INLINE bool arcMat6::IsIdentity( const float epsilon ) const {
	return Compare( mat6_identity, epsilon );
}

ARC_INLINE bool arcMat6::IsSymmetric( const float epsilon ) const {
	for ( int i = 1; i < 6; i++ ) {
		for ( int j = 0; j < i; j++ ) {
			if ( arcMath::Fabs( mat[i][j] - mat[j][i] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE bool arcMat6::IsDiagonal( const float epsilon ) const {
	for ( int i = 0; i < 6; i++ ) {
		for ( int j = 0; j < 6; j++ ) {
			if ( i != j && arcMath::Fabs( mat[i][j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE arcMat3 arcMat6::SubMat3( int n ) const {
	assert( n >= 0 && n < 4 );
	int b0 = ( ( n & 2 ) >> 1 ) * 3;
	int b1 = ( n & 1 ) * 3;
	return arcMat3(
		mat[b0 + 0][b1 + 0], mat[b0 + 0][b1 + 1], mat[b0 + 0][b1 + 2],
		mat[b0 + 1][b1 + 0], mat[b0 + 1][b1 + 1], mat[b0 + 1][b1 + 2],
		mat[b0 + 2][b1 + 0], mat[b0 + 2][b1 + 1], mat[b0 + 2][b1 + 2] );
}

ARC_INLINE float arcMat6::Trace( void ) const {
	return ( mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3] + mat[4][4] + mat[5][5] );
}

ARC_INLINE arcMat6 arcMat6::Inverse( void ) const {
	arcMat6 invMat;

	invMat = *this;
	int r = invMat.InverseSelf();
	assert( r );
	return invMat;
}

ARC_INLINE arcMat6 arcMat6::InverseFast( void ) const {
	arcMat6 invMat;

	invMat = *this;
	int r = invMat.InverseFastSelf();
	assert( r );
	return invMat;
}

ARC_INLINE int arcMat6::GetDimension( void ) const {
	return 36;
}

ARC_INLINE const float *arcMat6::ToFloatPtr( void ) const {
	return mat[0].ToFloatPtr();
}

ARC_INLINE float *arcMat6::ToFloatPtr( void ) {
	return mat[0].ToFloatPtr();
}


//===============================================================
//
//	arcMatX - arbitrary sized dense real matrix
//
//  The matrix lives on 16 byte aligned and 16 byte padded memory.
//
//	NOTE: due to the temporary memory pool arcMatX cannot be used by multiple threads.
//
//===============================================================

#define MATX_MAX_TEMP		1024
#define MATX_QUAD( x )		( ( ( ( x ) + 3 ) & ~3 ) * sizeof( float ) )
#define MATX_CLEAREND()		int s = numRows * numColumns; while( s < ( ( s + 3 ) & ~3 ) ) { mat[s++] = 0.0f; }
#define MATX_ALLOCA( n )	( (float *) _alloca16( MATX_QUAD( n ) ) )
#define MATX_SIMD

class arcMatX {
public:
					arcMatX( void );
					explicit arcMatX( int rows, int columns );
					explicit arcMatX( int rows, int columns, float *src );
					~arcMatX( void );

	void			Set( int rows, int columns, const float *src );
	void			Set( const arcMat3 &m1, const arcMat3 &m2 );
	void			Set( const arcMat3 &m1, const arcMat3 &m2, const arcMat3 &m3, const arcMat3 &m4 );

	const float *	operator[]( int index ) const;
	float *			operator[]( int index );
	arcMatX &		operator=( const arcMatX &a );
	arcMatX			operator*( const float a ) const;
	arcVecX			operator*( const arcVecX &vec ) const;
	arcMatX			operator*( const arcMatX &a ) const;
	arcMatX			operator+( const arcMatX &a ) const;
	arcMatX			operator-( const arcMatX &a ) const;
	arcMatX &		operator*=( const float a );
	arcMatX &		operator*=( const arcMatX &a );
	arcMatX &		operator+=( const arcMatX &a );
	arcMatX &		operator-=( const arcMatX &a );

	friend arcMatX	operator*( const float a, const arcMatX &m );
	friend arcVecX	operator*( const arcVecX &vec, const arcMatX &m );
	friend arcVecX &	operator*=( arcVecX &vec, const arcMatX &m );

	bool			Compare( const arcMatX &a ) const;								// exact compare, no epsilon
	bool			Compare( const arcMatX &a, const float epsilon ) const;			// compare with epsilon
	bool			operator==( const arcMatX &a ) const;							// exact compare, no epsilon
	bool			operator!=( const arcMatX &a ) const;							// exact compare, no epsilon

	void			SetSize( int rows, int columns );								// set the number of rows/columns
	void			ChangeSize( int rows, int columns, bool makeZero = false );		// change the size keeping data intact where possible
	int				GetNumRows( void ) const { return numRows; }					// get the number of rows
	int				GetNumColumns( void ) const { return numColumns; }				// get the number of columns
	void			SetData( int rows, int columns, float *data );					// set float array pointer
	void			Zero( void );													// clear matrix
	void			Zero( int rows, int columns );									// set size and clear matrix
	void			Identity( void );												// clear to identity matrix
	void			Identity( int rows, int columns );								// set size and clear to identity matrix
	void			Diag( const arcVecX &v );										// create diagonal matrix from vector
	void			Random( int seed, float l = 0.0f, float u = 1.0f );				// fill matrix with random values
	void			Random( int rows, int columns, int seed, float l = 0.0f, float u = 1.0f );
	void			Negate( void );													// (*this) = - (*this)
	void			Clamp( float min, float max );									// clamp all values
	arcMatX &		SwapRows( int r1, int r2 );										// swap rows
	arcMatX &		SwapColumns( int r1, int r2 );									// swap columns
	arcMatX &		SwapRowsColumns( int r1, int r2 );								// swap rows and columns
	arcMatX &		RemoveRow( int r );												// remove a row
	arcMatX &		RemoveColumn( int r );											// remove a column
	arcMatX &		RemoveRowColumn( int r );										// remove a row and column
	void			ClearUpperTriangle( void );										// clear the upper triangle
	void			ClearLowerTriangle( void );										// clear the lower triangle
	void			SquareSubMatrix( const arcMatX &m, int size );					// get square sub-matrix from 0,0 to size,size
	float			MaxDifference( const arcMatX &m ) const;							// return maximum element difference between this and m

	bool			IsSquare( void ) const { return ( numRows == numColumns ); }
	bool			IsZero( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsIdentity( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsTriDiagonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsOrthogonal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsOrthonormal( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsPMatrix( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsZMatrix( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsPositiveDefinite( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetricPositiveDefinite( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsPositiveSemiDefinite( const float epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetricPositiveSemiDefinite( const float epsilon = MATRIX_EPSILON ) const;

	float			Trace( void ) const;											// returns product of diagonal elements
	float			Determinant( void ) const;										// returns determinant of matrix
	arcMatX			Transpose( void ) const;										// returns transpose
	arcMatX &		TransposeSelf( void );											// transposes the matrix itself
	arcMatX			Inverse( void ) const;											// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf( void );											// returns false if determinant is zero
	arcMatX			InverseFast( void ) const;										// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf( void );										// returns false if determinant is zero

	bool			LowerTriangularInverse( void );									// in-place inversion, returns false if determinant is zero
	bool			UpperTriangularInverse( void );									// in-place inversion, returns false if determinant is zero

	arcVecX			Multiply( const arcVecX &vec ) const;							// (*this) * vec
	arcVecX			TransposeMultiply( const arcVecX &vec ) const;					// this->Transpose() * vec

	arcMatX			Multiply( const arcMatX &a ) const;								// (*this) * a
	arcMatX			TransposeMultiply( const arcMatX &a ) const;						// this->Transpose() * a

	void			Multiply( arcVecX &dst, const arcVecX &vec ) const;				// dst = (*this) * vec
	void			MultiplyAdd( arcVecX &dst, const arcVecX &vec ) const;			// dst += (*this) * vec
	void			MultiplySub( arcVecX &dst, const arcVecX &vec ) const;			// dst -= (*this) * vec
	void			TransposeMultiply( arcVecX &dst, const arcVecX &vec ) const;		// dst = this->Transpose() * vec
	void			TransposeMultiplyAdd( arcVecX &dst, const arcVecX &vec ) const;	// dst += this->Transpose() * vec
	void			TransposeMultiplySub( arcVecX &dst, const arcVecX &vec ) const;	// dst -= this->Transpose() * vec

	void			Multiply( arcMatX &dst, const arcMatX &a ) const;					// dst = (*this) * a
	void			TransposeMultiply( arcMatX &dst, const arcMatX &a ) const;		// dst = this->Transpose() * a

	int				GetDimension( void ) const;										// returns total number of values in matrix

	const arcVec6 &	SubVec6( int row ) const;										// interpret beginning of row as a const arcVec6
	arcVec6 &		SubVec6( int row );												// interpret beginning of row as an arcVec6
	const arcVecX	SubVecX( int row ) const;										// interpret complete row as a const arcVecX
	arcVecX			SubVecX( int row );												// interpret complete row as an arcVecX
	const float *	ToFloatPtr( void ) const;										// pointer to const matrix float array
	float *			ToFloatPtr( void );												// pointer to matrix float array
	const char *	ToString( int precision = 2 ) const;

	void			Update_RankOne( const arcVecX &v, const arcVecX &w, float alpha );
	void			Update_RankOneSymmetric( const arcVecX &v, float alpha );
	void			Update_RowColumn( const arcVecX &v, const arcVecX &w, int r );
	void			Update_RowColumnSymmetric( const arcVecX &v, int r );
	void			Update_Increment( const arcVecX &v, const arcVecX &w );
	void			Update_IncrementSymmetric( const arcVecX &v );
	void			Update_Decrement( int r );

	bool			Inverse_GaussJordan( void );					// invert in-place with Gauss-Jordan elimination
	bool			Inverse_UpdateRankOne( const arcVecX &v, const arcVecX &w, float alpha );
	bool			Inverse_UpdateRowColumn( const arcVecX &v, const arcVecX &w, int r );
	bool			Inverse_UpdateIncrement( const arcVecX &v, const arcVecX &w );
	bool			Inverse_UpdateDecrement( const arcVecX &v, const arcVecX &w, int r );
	void			Inverse_Solve( arcVecX &x, const arcVecX &b ) const;

	bool			LU_Factor( int *index, float *det = NULL );		// factor in-place: L * U
	bool			LU_UpdateRankOne( const arcVecX &v, const arcVecX &w, float alpha, int *index );
	bool			LU_UpdateRowColumn( const arcVecX &v, const arcVecX &w, int r, int *index );
	bool			LU_UpdateIncrement( const arcVecX &v, const arcVecX &w, int *index );
	bool			LU_UpdateDecrement( const arcVecX &v, const arcVecX &w, const arcVecX &u, int r, int *index );
	void			LU_Solve( arcVecX &x, const arcVecX &b, const int *index ) const;
	void			LU_Inverse( arcMatX &inv, const int *index ) const;
	void			LU_UnpackFactors( arcMatX &L, arcMatX &U ) const;
	void			LU_MultiplyFactors( arcMatX &m, const int *index ) const;

	bool			QR_Factor( arcVecX &c, arcVecX &d );				// factor in-place: Q * R
	bool			QR_UpdateRankOne( arcMatX &R, const arcVecX &v, const arcVecX &w, float alpha );
	bool			QR_UpdateRowColumn( arcMatX &R, const arcVecX &v, const arcVecX &w, int r );
	bool			QR_UpdateIncrement( arcMatX &R, const arcVecX &v, const arcVecX &w );
	bool			QR_UpdateDecrement( arcMatX &R, const arcVecX &v, const arcVecX &w, int r );
	void			QR_Solve( arcVecX &x, const arcVecX &b, const arcVecX &c, const arcVecX &d ) const;
	void			QR_Solve( arcVecX &x, const arcVecX &b, const arcMatX &R ) const;
	void			QR_Inverse( arcMatX &inv, const arcVecX &c, const arcVecX &d ) const;
	void			QR_UnpackFactors( arcMatX &Q, arcMatX &R, const arcVecX &c, const arcVecX &d ) const;
	void			QR_MultiplyFactors( arcMatX &m, const arcVecX &c, const arcVecX &d ) const;

	bool			SVD_Factor( arcVecX &w, arcMatX &V );				// factor in-place: U * Diag(w) * V.Transpose()
	void			SVD_Solve( arcVecX &x, const arcVecX &b, const arcVecX &w, const arcMatX &V ) const;
	void			SVD_Inverse( arcMatX &inv, const arcVecX &w, const arcMatX &V ) const;
	void			SVD_MultiplyFactors( arcMatX &m, const arcVecX &w, const arcMatX &V ) const;

	bool			Cholesky_Factor( void );						// factor in-place: L * L.Transpose()
	bool			Cholesky_UpdateRankOne( const arcVecX &v, float alpha, int offset = 0 );
	bool			Cholesky_UpdateRowColumn( const arcVecX &v, int r );
	bool			Cholesky_UpdateIncrement( const arcVecX &v );
	bool			Cholesky_UpdateDecrement( const arcVecX &v, int r );
	void			Cholesky_Solve( arcVecX &x, const arcVecX &b ) const;
	void			Cholesky_Inverse( arcMatX &inv ) const;
	void			Cholesky_MultiplyFactors( arcMatX &m ) const;

	bool			LDLT_Factor( void );							// factor in-place: L * D * L.Transpose()
	bool			LDLT_UpdateRankOne( const arcVecX &v, float alpha, int offset = 0 );
	bool			LDLT_UpdateRowColumn( const arcVecX &v, int r );
	bool			LDLT_UpdateIncrement( const arcVecX &v );
	bool			LDLT_UpdateDecrement( const arcVecX &v, int r );
	void			LDLT_Solve( arcVecX &x, const arcVecX &b ) const;
	void			LDLT_Inverse( arcMatX &inv ) const;
	void			LDLT_UnpackFactors( arcMatX &L, arcMatX &D ) const;
	void			LDLT_MultiplyFactors( arcMatX &m ) const;

	void			TriDiagonal_ClearTriangles( void );
	bool			TriDiagonal_Solve( arcVecX &x, const arcVecX &b ) const;
	void			TriDiagonal_Inverse( arcMatX &inv ) const;

	bool			Eigen_SolveSymmetricTriDiagonal( arcVecX &eigenValues );
	bool			Eigen_SolveSymmetric( arcVecX &eigenValues );
	bool			Eigen_Solve( arcVecX &realEigenValues, arcVecX &imaginaryEigenValues );
	void			Eigen_SortIncreasing( arcVecX &eigenValues );
	void			Eigen_SortDecreasing( arcVecX &eigenValues );

	static void		Test( void );

private:
	int				numRows;				// number of rows
	int				numColumns;				// number of columns
	int				alloced;				// floats allocated, if -1 then mat points to data set with SetData
	float *			mat;					// memory the matrix is stored

	static float	temp[MATX_MAX_TEMP+4];	// used to store intermediate results
	static float *	tempPtr;				// pointer to 16 byte aligned temporary memory
	static int		tempIndex;				// index into memory pool, wraps around

private:
	void			SetTempSize( int rows, int columns );
	float			DeterminantGeneric( void ) const;
	bool			InverseSelfGeneric( void );
	void			QR_Rotate( arcMatX &R, int i, float a, float b );
	float			Pythag( float a, float b ) const;
	void			SVD_BiDiag( arcVecX &w, arcVecX &rv1, float &anorm );
	void			SVD_InitialWV( arcVecX &w, arcMatX &V, arcVecX &rv1 );
	void			HouseholderReduction( arcVecX &diag, arcVecX &subd );
	bool			QL( arcVecX &diag, arcVecX &subd );
	void			HessenbergReduction( arcMatX &H );
	void			ComplexDivision( float xr, float xi, float yr, float yi, float &cdivr, float &cdivi );
	bool			HessenbergToRealSchur( arcMatX &H, arcVecX &realEigenValues, arcVecX &imaginaryEigenValues );
};

ARC_INLINE arcMatX::arcMatX( void ) {
	numRows = numColumns = alloced = 0;
	mat = NULL;
}

ARC_INLINE arcMatX::~arcMatX( void ) {
	// if not temp memory
	if ( mat != NULL && ( mat < arcMatX::tempPtr || mat > arcMatX::tempPtr + MATX_MAX_TEMP ) && alloced != -1 ) {
		Mem_Free16( mat );
	}
}

ARC_INLINE arcMatX::arcMatX( int rows, int columns ) {
	numRows = numColumns = alloced = 0;
	mat = NULL;
	SetSize( rows, columns );
}

ARC_INLINE arcMatX::arcMatX( int rows, int columns, float *src ) {
	numRows = numColumns = alloced = 0;
	mat = NULL;
	SetData( rows, columns, src );
}

ARC_INLINE void arcMatX::Set( int rows, int columns, const float *src ) {
	SetSize( rows, columns );
	memcpy( this->mat, src, rows * columns * sizeof( float ) );
}

ARC_INLINE void arcMatX::Set( const arcMat3 &m1, const arcMat3 &m2 ) {
	SetSize( 3, 6 );
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			mat[( i+0 ) * numColumns + (j+0 )] = m1[i][j];
			mat[( i+0 ) * numColumns + (j+3)] = m2[i][j];
		}
	}
}

ARC_INLINE void arcMatX::Set( const arcMat3 &m1, const arcMat3 &m2, const arcMat3 &m3, const arcMat3 &m4 ) {
	SetSize( 6, 6 );
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			mat[( i+0 ) * numColumns + (j+0 )] = m1[i][j];
			mat[( i+0 ) * numColumns + (j+3)] = m2[i][j];
			mat[( i+3) * numColumns + (j+0 )] = m3[i][j];
			mat[( i+3) * numColumns + (j+3)] = m4[i][j];
		}
	}
}

ARC_INLINE const float *arcMatX::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < numRows ) );
	return mat + index * numColumns;
}

ARC_INLINE float *arcMatX::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < numRows ) );
	return mat + index * numColumns;
}

ARC_INLINE arcMatX &arcMatX::operator=( const arcMatX &a ) {
	SetSize( a.numRows, a.numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->Copy16( mat, a.mat, a.numRows * a.numColumns );
#else
	memcpy( mat, a.mat, a.numRows * a.numColumns * sizeof( float ) );
#endif
	arcMatX::tempIndex = 0;
	return *this;
}

ARC_INLINE arcMatX arcMatX::operator*( const float a ) const {
	arcMatX m;

	m.SetTempSize( numRows, numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->Mul16( m.mat, mat, a, numRows * numColumns );
#else
	int i, s;
	s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		m.mat[i] = mat[i] * a;
	}
#endif
	return m;
}

ARC_INLINE arcVecX arcMatX::operator*( const arcVecX &vec ) const {
	arcVecX dst;

	assert( numColumns == vec.GetSize() );

	dst.SetTempSize( numRows );
#ifdef MATX_SIMD
	SIMDProcessor->MatX_MultiplyVecX( dst, *this, vec );
#else
	Multiply( dst, vec );
#endif
	return dst;
}

ARC_INLINE arcMatX arcMatX::operator*( const arcMatX &a ) const {
	arcMatX dst;

	assert( numColumns == a.numRows );

	dst.SetTempSize( numRows, a.numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->MatX_MultiplyMatX( dst, *this, a );
#else
	Multiply( dst, a );
#endif
	return dst;
}

ARC_INLINE arcMatX arcMatX::operator+( const arcMatX &a ) const {
	arcMatX m;

	assert( numRows == a.numRows && numColumns == a.numColumns );
	m.SetTempSize( numRows, numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->Add16( m.mat, mat, a.mat, numRows * numColumns );
#else
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		m.mat[i] = mat[i] + a.mat[i];
	}
#endif
	return m;
}

ARC_INLINE arcMatX arcMatX::operator-( const arcMatX &a ) const {
	arcMatX m;

	assert( numRows == a.numRows && numColumns == a.numColumns );
	m.SetTempSize( numRows, numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->Sub16( m.mat, mat, a.mat, numRows * numColumns );
#else
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		m.mat[i] = mat[i] - a.mat[i];
	}
#endif
	return m;
}

ARC_INLINE arcMatX &arcMatX::operator*=( const float a ) {
#ifdef MATX_SIMD
	SIMDProcessor->MulAssign16( mat, a, numRows * numColumns );
#else
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		mat[i] *= a;
	}
#endif
	arcMatX::tempIndex = 0;
	return *this;
}

ARC_INLINE arcMatX &arcMatX::operator*=( const arcMatX &a ) {
	*this = *this * a;
	arcMatX::tempIndex = 0;
	return *this;
}

ARC_INLINE arcMatX &arcMatX::operator+=( const arcMatX &a ) {
	assert( numRows == a.numRows && numColumns == a.numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->AddAssign16( mat, a.mat, numRows * numColumns );
#else
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		mat[i] += a.mat[i];
	}
#endif
	arcMatX::tempIndex = 0;
	return *this;
}

ARC_INLINE arcMatX &arcMatX::operator-=( const arcMatX &a ) {
	assert( numRows == a.numRows && numColumns == a.numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->SubAssign16( mat, a.mat, numRows * numColumns );
#else
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		mat[i] -= a.mat[i];
	}
#endif
	arcMatX::tempIndex = 0;
	return *this;
}

ARC_INLINE arcMatX operator*( const float a, arcMatX const &m ) {
	return m * a;
}

ARC_INLINE arcVecX operator*( const arcVecX &vec, const arcMatX &m ) {
	return m * vec;
}

ARC_INLINE arcVecX &operator*=( arcVecX &vec, const arcMatX &m ) {
	vec = m * vec;
	return vec;
}

ARC_INLINE bool arcMatX::Compare( const arcMatX &a ) const {
	assert( numRows == a.numRows && numColumns == a.numColumns );

	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		if ( mat[i] != a.mat[i] ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcMatX::Compare( const arcMatX &a, const float epsilon ) const {
	assert( numRows == a.numRows && numColumns == a.numColumns );

	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		if ( arcMath::Fabs( mat[i] - a.mat[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcMatX::operator==( const arcMatX &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcMatX::operator!=( const arcMatX &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcMatX::SetSize( int rows, int columns ) {
	assert( mat < arcMatX::tempPtr || mat > arcMatX::tempPtr + MATX_MAX_TEMP );
	int alloc = ( rows * columns + 3 ) & ~3;
	if ( alloc > alloced && alloced != -1 ) {
		if ( mat != NULL ) {
			Mem_Free16( mat );
		}
		mat = (float *) Mem_Alloc16( alloc * sizeof( float ) );
		alloced = alloc;
	}
	numRows = rows;
	numColumns = columns;
	MATX_CLEAREND();
}

ARC_INLINE void arcMatX::SetTempSize( int rows, int columns ) {
	int newSize;

	newSize = ( rows * columns + 3 ) & ~3;
	assert( newSize < MATX_MAX_TEMP );
	if ( arcMatX::tempIndex + newSize > MATX_MAX_TEMP ) {
		arcMatX::tempIndex = 0;
	}
	mat = arcMatX::tempPtr + arcMatX::tempIndex;
	arcMatX::tempIndex += newSize;
	alloced = newSize;
	numRows = rows;
	numColumns = columns;
	MATX_CLEAREND();
}

ARC_INLINE void arcMatX::SetData( int rows, int columns, float *data ) {
	assert( mat < arcMatX::tempPtr || mat > arcMatX::tempPtr + MATX_MAX_TEMP );
	if ( mat != NULL && alloced != -1 ) {
		Mem_Free16( mat );
	}
	assert( ( ( ( int ) data ) & 15 ) == 0 ); // data must be 16 byte aligned
	mat = data;
	alloced = -1;
	numRows = rows;
	numColumns = columns;
	MATX_CLEAREND();
}

ARC_INLINE void arcMatX::Zero( void ) {
#ifdef MATX_SIMD
	SIMDProcessor->Zero16( mat, numRows * numColumns );
#else
	memset( mat, 0, numRows * numColumns * sizeof( float ) );
#endif
}

ARC_INLINE void arcMatX::Zero( int rows, int columns ) {
	SetSize( rows, columns );
#ifdef MATX_SIMD
	SIMDProcessor->Zero16( mat, numRows * numColumns );
#else
	memset( mat, 0, rows * columns * sizeof( float ) );
#endif
}

ARC_INLINE void arcMatX::Identity( void ) {
	assert( numRows == numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->Zero16( mat, numRows * numColumns );
#else
	memset( mat, 0, numRows * numColumns * sizeof( float ) );
#endif
	for ( int i = 0; i < numRows; i++ ) {
		mat[i * numColumns + i] = 1.0f;
	}
}

ARC_INLINE void arcMatX::Identity( int rows, int columns ) {
	assert( rows == columns );
	SetSize( rows, columns );
	arcMatX::Identity();
}

ARC_INLINE void arcMatX::Diag( const arcVecX &v ) {
	Zero( v.GetSize(), v.GetSize() );
	for ( int i = 0; i < v.GetSize(); i++ ) {
		mat[i * numColumns + i] = v[i];
	}
}

ARC_INLINE void arcMatX::Random( int seed, float l, float u ) {
	arcRandom rnd(seed);

	float c = u - l;
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		mat[i] = l + rnd.RandomFloat() * c;
	}
}

ARC_INLINE void arcMatX::Random( int rows, int columns, int seed, float l, float u ) {
	arcRandom rnd(seed);

	SetSize( rows, columns );
	float c = u - l;
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		mat[i] = l + rnd.RandomFloat() * c;
	}
}

ARC_INLINE void arcMatX::Negate( void ) {
#ifdef MATX_SIMD
	SIMDProcessor->Negate16( mat, numRows * numColumns );
#else
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		mat[i] = -mat[i];
	}
#endif
}

ARC_INLINE void arcMatX::Clamp( float min, float max ) {
	int s = numRows * numColumns;
	for ( int i = 0; i < s; i++ ) {
		if ( mat[i] < min ) {
			mat[i] = min;
		} else if ( mat[i] > max ) {
			mat[i] = max;
		}
	}
}

ARC_INLINE arcMatX &arcMatX::SwapRows( int r1, int r2 ) {
	float *ptr;

	ptr = (float *) _alloca16( numColumns * sizeof( float ) );
	memcpy( ptr, mat + r1 * numColumns, numColumns * sizeof( float ) );
	memcpy( mat + r1 * numColumns, mat + r2 * numColumns, numColumns * sizeof( float ) );
	memcpy( mat + r2 * numColumns, ptr, numColumns * sizeof( float ) );

	return *this;
}

ARC_INLINE arcMatX &arcMatX::SwapColumns( int r1, int r2 ) {
	for ( int i = 0; i < numRows; i++ ) {
		float *ptr = mat + i * numColumns;
		float tmp = ptr[r1];
		ptr[r1] = ptr[r2];
		ptr[r2] = tmp;
	}

	return *this;
}

ARC_INLINE arcMatX &arcMatX::SwapRowsColumns( int r1, int r2 ) {
	SwapRows( r1, r2 );
	SwapColumns( r1, r2 );
	return *this;
}

ARC_INLINE void arcMatX::ClearUpperTriangle( void ) {
	assert( numRows == numColumns );
	for ( int i = numRows-2; i >= 0; i-- ) {
		memset( mat + i * numColumns + i + 1, 0, (numColumns - 1 - i) * sizeof( float ) );
	}
}

ARC_INLINE void arcMatX::ClearLowerTriangle( void ) {
	assert( numRows == numColumns );
	for ( int i = 1; i < numRows; i++ ) {
		memset( mat + i * numColumns, 0, i * sizeof( float ) );
	}
}

ARC_INLINE void arcMatX::SquareSubMatrix( const arcMatX &m, int size ) {
	assert( size <= m.numRows && size <= m.numColumns );
	SetSize( size, size );
	for ( int i = 0; i < size; i++ ) {
		memcpy( mat + i * numColumns, m.mat + i * m.numColumns, size * sizeof( float ) );
	}
}

ARC_INLINE float arcMatX::MaxDifference( const arcMatX &m ) const {
	assert( numRows == m.numRows && numColumns == m.numColumns );

	float maxDiff = -1.0f;
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			float diff = arcMath::Fabs( mat[ i * numColumns + j ] - m[i][j] );
			if ( maxDiff < 0.0f || diff > maxDiff ) {
				maxDiff = diff;
			}
		}
	}
	return maxDiff;
}

ARC_INLINE bool arcMatX::IsZero( const float epsilon ) const {
	// returns true if (*this) == Zero
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			if ( arcMath::Fabs( mat[i * numColumns + j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE bool arcMatX::IsIdentity( const float epsilon ) const {
	// returns true if (*this) == Identity
	assert( numRows == numColumns );
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			if ( arcMath::Fabs( mat[i * numColumns + j] - ( float )( i == j ) ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE bool arcMatX::IsDiagonal( const float epsilon ) const {
	// returns true if all elements are zero except for the elements on the diagonal
	assert( numRows == numColumns );
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			if ( i != j && arcMath::Fabs( mat[i * numColumns + j] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE bool arcMatX::IsTriDiagonal( const float epsilon ) const {
	// returns true if all elements are zero except for the elements on the diagonal plus or minus one column
	if ( numRows != numColumns ) {
		return false;
	}
	for ( int i = 0; i < numRows-2; i++ ) {
		for ( int j = i+2; j < numColumns; j++ ) {
			if ( arcMath::Fabs( (*this)[i][j] ) > epsilon ) {
				return false;
			}
			if ( arcMath::Fabs( (*this)[j][i] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE bool arcMatX::IsSymmetric( const float epsilon ) const {
	// (*this)[i][j] == (*this)[j][i]
	if ( numRows != numColumns ) {
		return false;
	}
	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			if ( arcMath::Fabs( mat[ i * numColumns + j ] - mat[ j * numColumns + i ] ) > epsilon ) {
				return false;
			}
		}
	}
	return true;
}

ARC_INLINE float arcMatX::Trace( void ) const {
	float trace = 0.0f;

	assert( numRows == numColumns );

	// sum of elements on the diagonal
	for ( int i = 0; i < numRows; i++ ) {
		trace += mat[i * numRows + i];
	}
	return trace;
}

ARC_INLINE float arcMatX::Determinant( void ) const {
	assert( numRows == numColumns );

	switch( numRows ) {
		case 1:
			return mat[0];
		case 2:
			return reinterpret_cast<const arcMat2 *>(mat)->Determinant();
		case 3:
			return reinterpret_cast<const arcMat3 *>(mat)->Determinant();
		case 4:
			return reinterpret_cast<const arcMat4 *>(mat)->Determinant();
		case 5:
			return reinterpret_cast<const arcMat5 *>(mat)->Determinant();
		case 6:
			return reinterpret_cast<const arcMat6 *>(mat)->Determinant();
		default:
			return DeterminantGeneric();
	}
	return 0.0f;
}

ARC_INLINE arcMatX arcMatX::Transpose( void ) const {
	arcMatX transpose.SetTempSize( numColumns, numRows );

	for ( int i = 0; i < numRows; i++ ) {
		for ( int j = 0; j < numColumns; j++ ) {
			transpose.mat[j * transpose.numColumns + i] = mat[i * numColumns + j];
		}
	}

	return transpose;
}

ARC_INLINE arcMatX &arcMatX::TransposeSelf( void ) {
	*this = Transpose();
	return *this;
}

ARC_INLINE arcMatX arcMatX::Inverse( void ) const {
	arcMatX invMat.SetTempSize( numRows, numColumns );
	memcpy( invMat.mat, mat, numRows * numColumns * sizeof( float ) );
	int r = invMat.InverseSelf();
	assert( r );
	return invMat;
}

ARC_INLINE bool arcMatX::InverseSelf( void ) {
	assert( numRows == numColumns );
	switch( numRows ) {
		case 1:
			if ( arcMath::Fabs( mat[0] ) < MATRIX_INVERSE_EPSILON ) {
				return false;
			}
			mat[0] = 1.0f / mat[0];
			return true;
		case 2:
			return reinterpret_cast<arcMat2 *>(mat)->InverseSelf();
		case 3:
			return reinterpret_cast<arcMat3 *>(mat)->InverseSelf();
		case 4:
			return reinterpret_cast<arcMat4 *>(mat)->InverseSelf();
		case 5:
			return reinterpret_cast<arcMat5 *>(mat)->InverseSelf();
		case 6:
			return reinterpret_cast<arcMat6 *>(mat)->InverseSelf();
		default:
			return InverseSelfGeneric();
	}
}

ARC_INLINE arcMatX arcMatX::InverseFast( void ) const {
	arcMatX invMat.SetTempSize( numRows, numColumns );
	memcpy( invMat.mat, mat, numRows * numColumns * sizeof( float ) );
	int r = invMat.InverseFastSelf();
	assert( r );
	return invMat;
}

ARC_INLINE bool arcMatX::InverseFastSelf( void ) {
	assert( numRows == numColumns );
	switch( numRows ) {
		case 1:
			if ( arcMath::Fabs( mat[0] ) < MATRIX_INVERSE_EPSILON ) {
				return false;
			}
			mat[0] = 1.0f / mat[0];
			return true;
		case 2:
			return reinterpret_cast<arcMat2 *>(mat)->InverseFastSelf();
		case 3:
			return reinterpret_cast<arcMat3 *>(mat)->InverseFastSelf();
		case 4:
			return reinterpret_cast<arcMat4 *>(mat)->InverseFastSelf();
		case 5:
			return reinterpret_cast<arcMat5 *>(mat)->InverseFastSelf();
		case 6:
			return reinterpret_cast<arcMat6 *>(mat)->InverseFastSelf();
		default:
			return InverseSelfGeneric();
	}
	return false;
}

ARC_INLINE arcVecX arcMatX::Multiply( const arcVecX &vec ) const {
	arcVecX dst;

	assert( numColumns == vec.GetSize() );

	dst.SetTempSize( numRows );
#ifdef MATX_SIMD
	SIMDProcessor->MatX_MultiplyVecX( dst, *this, vec );
#else
	Multiply( dst, vec );
#endif
	return dst;
}

ARC_INLINE arcMatX arcMatX::Multiply( const arcMatX &a ) const {
	arcMatX dst;

	assert( numColumns == a.numRows );

	dst.SetTempSize( numRows, a.numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->MatX_MultiplyMatX( dst, *this, a );
#else
	Multiply( dst, a );
#endif
	return dst;
}

ARC_INLINE arcVecX arcMatX::TransposeMultiply( const arcVecX &vec ) const {
	arcVecX dst;

	assert( numRows == vec.GetSize() );

	dst.SetTempSize( numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->MatX_TransposeMultiplyVecX( dst, *this, vec );
#else
	TransposeMultiply( dst, vec );
#endif
	return dst;
}

ARC_INLINE arcMatX arcMatX::TransposeMultiply( const arcMatX &a ) const {
	arcMatX dst;

	assert( numRows == a.numRows );

	dst.SetTempSize( numColumns, a.numColumns );
#ifdef MATX_SIMD
	SIMDProcessor->MatX_TransposeMultiplyMatX( dst, *this, a );
#else
	TransposeMultiply( dst, a );
#endif
	return dst;
}

ARC_INLINE void arcMatX::Multiply( arcVecX &dst, const arcVecX &vec ) const {
#ifdef MATX_SIMD
	SIMDProcessor->MatX_MultiplyVecX( dst, *this, vec );
#else
	const float *mPtr = mat;
	const float *vPtr = vec.ToFloatPtr();
	float *dstPtr = dst.ToFloatPtr();
	for ( int i = 0; i < numRows; i++ ) {
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numColumns; j++ ) {
			sum += mPtr[j] * vPtr[j];
		}
		dstPtr[i] = sum;
		mPtr += numColumns;
	}
#endif
}

ARC_INLINE void arcMatX::MultiplyAdd( arcVecX &dst, const arcVecX &vec ) const {
#ifdef MATX_SIMD
	SIMDProcessor->MatX_MultiplyAddVecX( dst, *this, vec );
#else
	const float *mPtr = mat;
	const float *vPtr = vec.ToFloatPtr();
	float *dstPtr = dst.ToFloatPtr();
	for ( int i = 0; i < numRows; i++ ) {
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numColumns; j++ ) {
			sum += mPtr[j] * vPtr[j];
		}
		dstPtr[i] += sum;
		mPtr += numColumns;
	}
#endif
}

ARC_INLINE void arcMatX::MultiplySub( arcVecX &dst, const arcVecX &vec ) const {
#ifdef MATX_SIMD
	SIMDProcessor->MatX_MultiplySubVecX( dst, *this, vec );
#else
	const float *mPtr = mat;
	const float *vPtr = vec.ToFloatPtr();
	float *dstPtr = dst.ToFloatPtr();
	for ( int i = 0; i < numRows; i++ ) {
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numColumns; j++ ) {
			sum += mPtr[j] * vPtr[j];
		}
		dstPtr[i] -= sum;
		mPtr += numColumns;
	}
#endif
}

ARC_INLINE void arcMatX::TransposeMultiply( arcVecX &dst, const arcVecX &vec ) const {
#ifdef MATX_SIMD
	SIMDProcessor->MatX_TransposeMultiplyVecX( dst, *this, vec );
#else
	const float *vPtr = vec.ToFloatPtr();
	float *dstPtr = dst.ToFloatPtr();
	for ( int i = 0; i < numColumns; i++ ) {
		mPtr = mat + i;
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numRows; j++ ) {
			const float *mPtr += numColumns;
			sum += mPtr[0] * vPtr[j];
		}
		dstPtr[i] = sum;
	}
#endif
}

ARC_INLINE void arcMatX::TransposeMultiplyAdd( arcVecX &dst, const arcVecX &vec ) const {
#ifdef MATX_SIMD
	SIMDProcessor->MatX_TransposeMultiplyAddVecX( dst, *this, vec );
#else
	const float *vPtr = vec.ToFloatPtr();
	float *dstPtr = dst.ToFloatPtr();
	for ( int i = 0; i < numColumns; i++ ) {
		mPtr = mat + i;
		float sum = mPtr[0] * vPtr[0];
		for ( int j = 1; j < numRows; j++ ) {
			const float *mPtr += numColumns;
			sum += mPtr[0] * vPtr[j];
		}
		dstPtr[i] += sum;
	}
#endif
}

ARC_INLINE void arcMatX::TransposeMultiplySub( arcVecX &dst, const arcVecX &vec ) const {
#ifdef MATX_SIMD
	SIMDProcessor->MatX_TransposeMultiplySubVecX( dst, *this, vec );
#else
	int i, j;
	const float *mPtr, *vPtr;
	float *dstPtr;

	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	for ( i = 0; i < numColumns; i++ ) {
		mPtr = mat + i;
		float sum = mPtr[0] * vPtr[0];
		for ( j = 1; j < numRows; j++ ) {
			mPtr += numColumns;
			sum += mPtr[0] * vPtr[j];
		}
		dstPtr[i] -= sum;
	}
#endif
}

ARC_INLINE void arcMatX::Multiply( arcMatX &dst, const arcMatX &a ) const {
#ifdef MATX_SIMD
	SIMDProcessor->MatX_MultiplyMatX( dst, *this, a );
#else
	assert( numColumns == a.numRows );

	float *dstPtr = dst.ToFloatPtr();
	const float *m1Ptr = ToFloatPtr();
	const float *m2Ptr = a.ToFloatPtr();
	int k = numRows;
	int l = a.GetNumColumns();

	for ( int i = 0; i < k; i++ ) {
		for ( int j = 0; j < l; j++ ) {
			m2Ptr = a.ToFloatPtr() + j;
			double sum = m1Ptr[0] * m2Ptr[0];
			for ( int n = 1; n < numColumns; n++ ) {
				const float *m2Ptr += l;
				double sum += m1Ptr[n] * m2Ptr[0];
			}
			*dstPtr++ = sum;
		}
		m1Ptr += numColumns;
	}
#endif
}

ARC_INLINE void arcMatX::TransposeMultiply( arcMatX &dst, const arcMatX &a ) const {
#ifdef MATX_SIMD
	SIMDProcessor->MatX_TransposeMultiplyMatX( dst, *this, a );
#else
	assert( numRows == a.numRows );

	float *dstPtr = dst.ToFloatPtr();
	const float *m1Ptr = ToFloatPtr();
	int k = numColumns;
	int l = a.numColumns;

	for ( int i = 0; i < k; i++ ) {
		for ( int j = 0; j < l; j++ ) {
			const float *m1Ptr = ToFloatPtr() + i;
			const float *m2Ptr = a.ToFloatPtr() + j;
			double sum = m1Ptr[0] * m2Ptr[0];
			for (  intn = 1; n < numRows; n++ ) {
				const float *m1Ptr += numColumns;
				const float *m2Ptr += a.numColumns;
				double sum += m1Ptr[0] * m2Ptr[0];
			}
			*dstPtr++ = sum;
		}
	}
#endif
}

ARC_INLINE int arcMatX::GetDimension( void ) const {
	return numRows * numColumns;
}

ARC_INLINE const arcVec6 &arcMatX::SubVec6( int row ) const {
	assert( numColumns >= 6 && row >= 0 && row < numRows );
	return *reinterpret_cast<const arcVec6 *>(mat + row * numColumns);
}

ARC_INLINE arcVec6 &arcMatX::SubVec6( int row ) {
	assert( numColumns >= 6 && row >= 0 && row < numRows );
	return *reinterpret_cast<arcVec6 *>(mat + row * numColumns);
}

ARC_INLINE const arcVecX arcMatX::SubVecX( int row ) const {
	arcVecX v;
	assert( row >= 0 && row < numRows );
	v.SetData( numColumns, mat + row * numColumns );
	return v;
}

ARC_INLINE arcVecX arcMatX::SubVecX( int row ) {
	arcVecX v;
	assert( row >= 0 && row < numRows );
	v.SetData( numColumns, mat + row * numColumns );
	return v;
}

ARC_INLINE const float *arcMatX::ToFloatPtr( void ) const {
	return mat;
}

ARC_INLINE float *arcMatX::ToFloatPtr( void ) {
	return mat;
}

#endif /* !__MATH_MATRIX_H__ */

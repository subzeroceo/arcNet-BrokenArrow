#ifndef __RENDERMATRIX_H__
#define __RENDERMATRIX_H__

static const int NUM_FRUSTUM_CORNERS	= 8;

struct frustumCorners_t {
	float	x[NUM_FRUSTUM_CORNERS];
	float	y[NUM_FRUSTUM_CORNERS];
	float	z[NUM_FRUSTUM_CORNERS];
};

enum frustumCull_t {
	FRUSTUM_CULL_FRONT		= 1,
	FRUSTUM_CULL_BACK		= 2,
	FRUSTUM_CULL_CROSS		= 3
};

/*
================================================================================================

anGLMatrix

This is a row-major matrix and transforms are applied with left-multiplication.

================================================================================================
*/
class anGLMatrix {
public:
							anGLMatrix() {}
	inline				anGLMatrix(	float a0, float a1, float a2, float a3,
											float b0, float b1, float b2, float b3,
											float c0, float c1, float c2, float c3,
											float d0, float d1, float d2, float d3 );

	const float *			operator[]( int index ) const { assert( index >= 0 && index < 4 ); return &m[index*4]; }
	float *					operator[]( int index ) { assert( index >= 0 && index < 4 ); return &m[index*4]; }

	void					Zero() { memset( m, 0, sizeof( m ) ); }
	inline void			Identity();

	// Matrix classification (only meant to be used for asserts).
	inline bool			IsZero( float epsilon ) const;
	inline bool			IsIdentity( float epsilon ) const;
	inline bool			IsAffineTransform( float epsilon ) const;
	inline bool			IsUniformScale( float epsilon ) const;

	// Transform a point.
	// NOTE: the anVec3 out variant does not divide by W.
	inline void			TransformPoint( const anVec3 & in, anVec3 & out ) const;
	inline void			TransformPoint( const anVec3 & in, anVec4 & out ) const;
	inline void			TransformPoint( const anVec4 & in, anVec4 & out ) const;

	// These assume the matrix has no non-uniform scaling or shearing.
	// NOTE: a direction will only stay normalized if the matrix has no skewing or scaling.
	inline void			TransformDir( const anVec3 & in, anVec3 & out, bool normalize ) const;
	inline void			TransformPlane( const anPlane & in, anPlane & out, bool normalize ) const;

	// These transforms work with non-uniform scaling and shearing by multiplying
	// with 'transpose(inverse(M))' where this matrix is assumed to be 'inverse(M)'.
	inline void			InverseTransformDir( const anVec3 & in, anVec3 & out, bool normalize ) const;
	inline void			InverseTransformPlane( const anPlane & in, anPlane & out, bool normalize ) const;

	// Project a point.
	static inline void	TransformModelToClip( const anVec3 & src, const anGLMatrix & modelMatrix, const anGLMatrix & projectionMatrix, anVec4 & eye, anVec4 & clip );
	static inline void	TransformClipToDevice( const anVec4 & clip, anVec3 & ndc );

	// Create a matrix that goes from local space to the space defined by the 'origin' and 'axis'.
	static void				CreateFromOriginAxis( const anVec3 & origin, const anMat3 & axis, anGLMatrix & out );
	static void				CreateFromOriginAxisScale( const anVec3 & origin, const anMat3 & axis, const anVec3 & scale, anGLMatrix & out );

	// Create a matrix that goes from a global coordinate to a view coordinate (OpenGL looking down -Z) based on the given view origin/axis.
	static void				CreateViewMatrix( const anVec3 & origin, const anMat3 & axis, anGLMatrix & out );

	// Create a projection matrix.
	static void				CreateProjectionMatrix( float xMin, float xMax, float yMin, float yMax, float zNear, float zFar, anGLMatrix & out );
	static void				CreateProjectionMatrixFov( float xFovDegrees, float yFovDegrees, float zNear, float zFar, float xOffset, float yOffset, anGLMatrix & out );

	// Apply depth hacks to a projection matrix.
	static inline void	ApplyDepthHack( anGLMatrix & src );
	static inline void	ApplyModelDepthHack( anGLMatrix & src, float value );

	// Offset and scale the given matrix such that the result matrix transforms the unit-cube to exactly cover the given bounds (and the inverse).
	static void				OffsetScaleForBounds( const anGLMatrix & src, const anBounds & bounds, anGLMatrix & out );
	static void				InverseOffsetScaleForBounds( const anGLMatrix & src, const anBounds & bounds, anGLMatrix & out );

	// Basic matrix operations.
	static void				Transpose( const anGLMatrix & src, anGLMatrix & out );
	static void				Multiply( const anGLMatrix & a, const anGLMatrix & b, anGLMatrix & out );
	static bool				Inverse( const anGLMatrix & src, anGLMatrix & out );
	static void				InverseByTranspose( const anGLMatrix & src, anGLMatrix & out );
	static bool				InverseByDoubles( const anGLMatrix & src, anGLMatrix & out );

	// Copy or create a matrix that is stored directly into four float4 vectors which is useful for directly setting vertex program uniforms.
	static void				CopyMatrix( const anGLMatrix & matrix, anVec4 & row0, anVec4 & row1, anVec4 & row2, anVec4 & row3 );

	// Cull to a Model-View-Projection (MVP) matrix.
	static bool				CullPointToMVP( const anGLMatrix & mvp, const anVec3 & point, bool zeroToOne = false );
	static bool				CullPointToMVPbits( const anGLMatrix & mvp, const anVec3 & point, byte * outBits, bool zeroToOne = false );
	static bool				CullBoundsToMVP( const anGLMatrix & mvp, const anBounds & bounds, bool zeroToOne = false );
	static bool				CullBoundsToMVPbits( const anGLMatrix & mvp, const anBounds & bounds, byte * outBits, bool zeroToOne = false );
	static bool				CullExtrudedBoundsToMVP( const anGLMatrix & mvp, const anBounds & bounds, const anVec3 & extrudeDirection, const anPlane & clipPlane, bool zeroToOne = false );
	static bool				CullExtrudedBoundsToMVPbits( const anGLMatrix & mvp, const anBounds & bounds, const anVec3 & extrudeDirection, const anPlane & clipPlane, byte * outBits, bool zeroToOne = false );

	// Create frustum planes and corners from a matrix.
	static void				GetFrustumPlanes( anPlane planes[6], const anGLMatrix & frustum, bool zeroToOne, bool normalize );
	static void				GetFrustumCorners( frustumCorners_t & corners, const anGLMatrix & frustumTransform, const anBounds & frustumBounds );
	static frustumCull_t	CullFrustumCornersToPlane( const frustumCorners_t & corners, const anPlane & plane );

private:
	float					m[16];
};

extern const anGLMatrix renderMatrix_identity;
extern const anGLMatrix renderMatrix_flipToOpenGL;
extern const anGLMatrix renderMatrix_windowSpaceToClipSpace;

/*
========================
anGLMatrix::anGLMatrix
========================
*/
inline anGLMatrix::anGLMatrix(	float a0, float a1, float a2, float a3,
											float b0, float b1, float b2, float b3,
											float c0, float c1, float c2, float c3,
											float d0, float d1, float d2, float d3 ) {
	m[0*4+0] = a0; m[0*4+1] = a1; m[0*4+2] = a2; m[0*4+3] = a3;
	m[1*4+0] = b0; m[1*4+1] = b1; m[1*4+2] = b2; m[1*4+3] = b3;
	m[2*4+0] = c0; m[2*4+1] = c1; m[2*4+2] = c2; m[2*4+3] = c3;
	m[3*4+0] = d0; m[3*4+1] = d1; m[3*4+2] = d2; m[3*4+3] = d3;
}

/*
========================
anGLMatrix::Identity
========================
*/
inline void anGLMatrix::Identity() {
	m[0*4+0] = 1.0f;
	m[0*4+1] = 0.0f;
	m[0*4+2] = 0.0f;
	m[0*4+3] = 0.0f;

	m[1*4+0] = 0.0f;
	m[1*4+1] = 1.0f;
	m[1*4+2] = 0.0f;
	m[1*4+3] = 0.0f;

	m[2*4+0] = 0.0f;
	m[2*4+1] = 0.0f;
	m[2*4+2] = 1.0f;
	m[2*4+3] = 0.0f;

	m[3*4+0] = 0.0f;
	m[3*4+1] = 0.0f;
	m[3*4+2] = 0.0f;
	m[3*4+3] = 1.0f;
}

/*
========================
anGLMatrix::IsZero
========================
*/
inline bool anGLMatrix::IsZero( float epsilon ) const {
	for ( int i = 0; i < 16; i++ ) {
		if ( anMath::Fabs( m[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

/*
========================
anGLMatrix::IsIdentity
========================
*/
inline bool anGLMatrix::IsIdentity( float epsilon ) const {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			if ( i == j ) {
				if ( anMath::Fabs( m[i * 4 + j] - 1.0f ) > epsilon ) {
					return false;
				}
			} else {
				if ( anMath::Fabs( m[i * 4 + j] ) > epsilon ) {
					return false;
				}
			}
		}
	}
	return true;
}

/*
========================
anGLMatrix::IsAffineTransform
========================
*/
inline bool anGLMatrix::IsAffineTransform( float epsilon ) const {
	if ( anMath::Fabs( m[3 * 4 + 0] ) > epsilon ||
			anMath::Fabs( m[3 * 4 + 1] ) > epsilon ||
				anMath::Fabs( m[3 * 4 + 2] ) > epsilon ||
					anMath::Fabs( m[3 * 4 + 3] - 1.0f ) > epsilon ) {
		return false;
	}
	return true;
}

/*
========================
anGLMatrix::IsUniformScale
========================
*/
inline bool anGLMatrix::IsUniformScale( float epsilon ) const {
	float d0 = anMath::InvSqrt( m[0*4+0] * m[0*4+0] + m[1*4+0] * m[1*4+0] + m[2*4+0] * m[2*4+0] );
	float d1 = anMath::InvSqrt( m[0*4+1] * m[0*4+1] + m[1*4+1] * m[1*4+1] + m[2*4+1] * m[2*4+1] );
	float d2 = anMath::InvSqrt( m[0*4+2] * m[0*4+2] + m[1*4+2] * m[1*4+2] + m[2*4+2] * m[2*4+2] );
	if ( anMath::Fabs( d0 - d1 ) > epsilon ) { return false; }
	if ( anMath::Fabs( d1 - d2 ) > epsilon ) { return false; }
	if ( anMath::Fabs( d0 - d2 ) > epsilon ) { return false; }
	return true;
}

/*
========================
anGLMatrix::TransformPoint
========================
*/
inline void anGLMatrix::TransformPoint( const anVec3 & in, anVec3 & out ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const anGLMatrix & matrix = *this;
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2] + matrix[0][3];
	out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2] + matrix[1][3];
	out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2] + matrix[2][3];
	assert( anMath::Fabs( in[0] * matrix[3][0] + in[1] * matrix[3][1] + in[2] * matrix[3][2] + matrix[3][3] - 1.0f ) < 0.01f );
}

/*
========================
anGLMatrix::TransformPoint
========================
*/
inline void anGLMatrix::TransformPoint( const anVec3 & in, anVec4 & out ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const anGLMatrix & matrix = *this;
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2] + matrix[0][3];
	out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2] + matrix[1][3];
	out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2] + matrix[2][3];
	out[3] = in[0] * matrix[3][0] + in[1] * matrix[3][1] + in[2] * matrix[3][2] + matrix[3][3];
}

/*
========================
anGLMatrix::TransformPoint
========================
*/
inline void anGLMatrix::TransformPoint( const anVec4 & in, anVec4 & out ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const anGLMatrix & matrix = *this;
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2] + in[3] * matrix[0][3];
	out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2] + in[3] * matrix[1][3];
	out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2] + in[3] * matrix[2][3];
	out[3] = in[0] * matrix[3][0] + in[1] * matrix[3][1] + in[2] * matrix[3][2] + in[3] * matrix[3][3];
}

/*
========================
anGLMatrix::TransformDir
========================
*/
inline void anGLMatrix::TransformDir( const anVec3 & in, anVec3 & out, bool normalize ) const {
	const anGLMatrix & matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2];
	float p1 = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2];
	float p2 = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2];
	if ( normalize ) {
		float r = anMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
}

/*
========================
anGLMatrix::TransformPlane
========================
*/
inline void anGLMatrix::TransformPlane( const anPlane & in, anPlane & out, bool normalize ) const {
	assert( IsUniformScale( 0.01f ) );
	const anGLMatrix & matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2];
	float p1 = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2];
	float p2 = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2];
	float d0 = matrix[0][3] - p0 * in[3];
	float d1 = matrix[1][3] - p1 * in[3];
	float d2 = matrix[2][3] - p2 * in[3];
	if ( normalize ) {
		float r = anMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
	out[3] = - p0 * d0 - p1 * d1 - p2 * d2;
}

/*
========================
anGLMatrix::InverseTransformDir
========================
*/
inline void anGLMatrix::InverseTransformDir( const anVec3 & in, anVec3 & out, bool normalize ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const anGLMatrix & matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0];
	float p1 = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1];
	float p2 = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2];
	if ( normalize ) {
		float r = anMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
}

/*
========================
anGLMatrix::InverseTransformPlane
========================
*/
inline void anGLMatrix::InverseTransformPlane( const anPlane & in, anPlane & out, bool normalize ) const {
	assert( in.ToFloatPtr() != out.ToFloatPtr() );
	const anGLMatrix & matrix = *this;
	float p0 = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0] + in[3] * matrix[3][0];
	float p1 = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1] + in[3] * matrix[3][1];
	float p2 = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2] + in[3] * matrix[3][2];
	float p3 = in[0] * matrix[0][3] + in[1] * matrix[1][3] + in[2] * matrix[2][3] + in[3] * matrix[3][3];
	if ( normalize ) {
		float r = anMath::InvSqrt( p0 * p0 + p1 * p1 + p2 * p2 );
		p0 *= r;
		p1 *= r;
		p2 *= r;
		p3 *= r;
	}
	out[0] = p0;
	out[1] = p1;
	out[2] = p2;
	out[3] = p3;
}

/*
========================
anGLMatrix::TransformModelToClip
========================
*/
inline void anGLMatrix::TransformModelToClip( const anVec3 & src, const anGLMatrix & modelMatrix, const anGLMatrix & projectionMatrix, anVec4 & eye, anVec4 & clip ) {
	for ( int i = 0; i < 4; i++ ) {
		eye[i] =	modelMatrix[i][0] * src[0] +
					modelMatrix[i][1] * src[1] +
					modelMatrix[i][2] * src[2] +
					modelMatrix[i][3];
	}
	for ( int i = 0; i < 4; i++ ) {
		clip[i] =	projectionMatrix[i][0] * eye[0] +
					projectionMatrix[i][1] * eye[1] +
					projectionMatrix[i][2] * eye[2] +
					projectionMatrix[i][3] * eye[3];
	}
}

/*
========================
anGLMatrix::TransformClipToDevice

Clip to normalized device coordinates.
========================
*/
inline void anGLMatrix::TransformClipToDevice( const anVec4 & clip, anVec3 & ndc ) {
	assert( anMath::Fabs( clip[3] ) > anMath::FLT_SMALLEST_NON_DENORMAL );
	float r = 1.0f / clip[3];
	ndc[0] = clip[0] * r;
	ndc[1] = clip[1] * r;
	ndc[2] = clip[2] * r;
}

/*
========================
anGLMatrix::ApplyDepthHack
========================
*/
inline void anGLMatrix::ApplyDepthHack( anGLMatrix & src ) {
	// scale projected z by 25%
	src.m[2*4+0] *= 0.25f;
	src.m[2*4+1] *= 0.25f;
	src.m[2*4+2] *= 0.25f;
	src.m[2*4+3] *= 0.25f;
}

/*
========================
anGLMatrix::ApplyModelDepthHack
========================
*/
inline void anGLMatrix::ApplyModelDepthHack( anGLMatrix & src, float value ) {
	// offset projected z
	src.m[2*4+3] -= value;
}

/*
========================
anGLMatrix::CullPointToMVP
========================
*/
inline bool anGLMatrix::CullPointToMVP( const anGLMatrix & mvp, const anVec3 & point, bool zeroToOne ) {
	byte bits;
	return CullPointToMVPbits( mvp, point, &bits, zeroToOne );
}

/*
========================
anGLMatrix::CullBoundsToMVP
========================
*/
inline bool anGLMatrix::CullBoundsToMVP( const anGLMatrix & mvp, const anBounds & bounds, bool zeroToOne ) {
	byte bits;
	return CullBoundsToMVPbits( mvp, bounds, &bits, zeroToOne );
}

/*
========================
anGLMatrix::CullExtrudedBoundsToMVP
========================
*/
inline bool anGLMatrix::CullExtrudedBoundsToMVP( const anGLMatrix & mvp, const anBounds & bounds, const anVec3 & extrudeDirection, const anPlane & clipPlane, bool zeroToOne ) {
	byte bits;
	return CullExtrudedBoundsToMVPbits( mvp, bounds, extrudeDirection, clipPlane, &bits, zeroToOne );
}

#endif // !__RENDERMATRIX_H__

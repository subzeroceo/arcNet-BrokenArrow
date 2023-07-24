#include "ARCRenderMatrixes.h"

#undef min
#undef max

const ARCRenderMatrixes ARCRenderMatrixes::arcEntity;

int ARCRenderMatrixes::GetPixelsPerMeter( int pixelWidth ) const {
	arcVec3 result = xform( vec3_origin( 1, 0, -1 ) );
	return int( ( result.x * 0.5f + 0.5f ) * pixelWidth );
}

bool ARCRenderMatrixes::IsOrthogonal() const {
	return columns[3][3] == 1.0f;
}
/*
================
ARCRenderMatrixes::CreateOrthoProjectionMatrix

just to have somethng slightly different if needed to save on repetitive calculations.=)
================
*/
const float *ARCRenderMatrixes::CreateOrthoProjectionMatrix( const float left, const float right, const float bottom, const float top, const float znear, const float zfar ) {
	static float m[ 16 ] = { 0 };

	m[0]	= 2.0f / ( right - left );
	m[5]	= 2.0f / ( top - bottom );
	m[10]	= - 2.0f / ( zfar - znear );
	m[12]	= - ( right + left )/( right - left );
	m[13]	= - ( top + bottom ) / ( top - bottom );
	m[14]	= - ( zfar + znear ) / ( zfar - znear );
	m[15]	= 1.0f;

	return m;
}

/*
================
ARCRenderMatrixes::CreateProjectionMatrix
================
*/
ARCRenderMatrixes ARCRenderMatrixes::CreateProjectionMatrix( float fov, float aspect, float nearClip, float farClip ) {
	const float deg = arcMath::PI / 180.0f;
	const float scale = 1.0f / arcMath::Tan( deg * fov / 2.0f );
	const float nearmfar = nearClip - farClip;
	ARCRenderMatrixes m[0] = scale;
	ARCRenderMatrixes m[5] = scale;
	ARCRenderMatrixes m[10] = ( farClip + nearClip ) / nearmfar;
	ARCRenderMatrixes m[11] = -1;
	ARCRenderMatrixes m[14] = 2 * farClip*nearClip / nearmfar;
	m[15] = 0.0f;
	return m;
}

/*
================
ARCRenderMatrixes::CreateInfiniteProjectionMatrix
================
*/
ARCRenderMatrixes ARCRenderMatrixes::CreateInfiniteProjectionMatrix( float fov, float aspect, float nearClip ) {
	const float yMax = nearClip * arcMath::Tan( fov * arcMath::PI / 360.0f );
	const float yMin = -yMax;
	const float xMax = nearClip * arcMath::Tan( fov * arcMath::PI / 360.0f );
	const float xMin = -xMax;
	const float width = xMax - xMin;
	const float height = yMax - yMin;

	ARCRenderMatrixes m;
	memset( &m, 0, sizeof( m ) );

	m[0] = 2 * nearClip / width;
	m[5] = 2 * nearClip / height;

	// this is the far-plane-at-infinity formulation, and
	// crunches the Z range slightly so w=0 vertexes do not
	// rasterize right at the wraparound point
	m[10] = -0.999f;
	m[11] = -1;
	m[14] = -2.0f*nearClip;
	return m;
}

/*
================
ARCRenderMatrixes::CreateLookAtMatrix
================
*/
ARCRenderMatrixes ARCRenderMatrixes::CreateLookAtMatrix( const arcVec3& viewOrigin, const arcVec3& at, const arcVec3& up ) {
	ARCRenderMatrixes rot = CreateLookAtMatrix( at - viewOrigin, up );
	ARCRenderMatrixes translate[12] = -viewOrigin.x;
	ARCRenderMatrixes translate[13] = -viewOrigin.y;
	ARCRenderMatrixes translate[14] = -viewOrigin.z;
	return rot * translate;
}

/*
================
ARCRenderMatrixes::CreateLookAtMatrix
================
*/
ARCRenderMatrixes ARCRenderMatrixes::CreateLookAtMatrix( const arcVec3& dir, const arcVec3& up ) {
	arcVec3 zAxis = ( dir * -1 ).Normalized();
	arcVec3 xAxis = up.Cross( zAxis ).Normalized();
	arcVec3 yAxis = zAxis.Cross( xAxis );

	ARCRenderMatrixes m;
	m[0] = xAxis.x;
	m[1] = yAxis.x;
	m[2] = zAxis.x;

	m[4] = xAxis.y;
	m[5] = yAxis.y;
	m[6] = zAxis.y;

	m[8] = xAxis.z;
	m[9] = yAxis.z;
	m[10] = zAxis.z;
	return m;
}

/*
================
ARCRenderMatrixes::CreateViewMatrix
================
*/
ARCRenderMatrixes ARCRenderMatrixes::CreateViewMatrix( const arcVec3& origin ) {
	ARCRenderMatrixes m;
	m[12] = -origin.x;
	m[13] = -origin.y;
	m[14] = -origin.z;
	return m;
}

/*
================
ARCRenderMatrixes::FlipMatrix
================
*/
ARCRenderMatrixes ARCRenderMatrixes::FlipMatrix() {
	static float flipMatrix[16] = {
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};
	static const ARCRenderMatrixes m( flipMatrix );
	return m;
}

/*
================
ARCRenderMatrixes::MatrixMultiply
================
*/
void ARCRenderMatrixes::MatrixMultiply( const float a[16], const float b[16], float out[16] ) {
	out[0*4+0] = a[0*4+0]*b[0*4+0] + a[0*4+1]*b[1*4+0] + a[0*4+2]*b[2*4+0] + a[0*4+3]*b[3*4+0];
	out[0*4+1] = a[0*4+0]*b[0*4+1] + a[0*4+1]*b[1*4+1] + a[0*4+2]*b[2*4+1] + a[0*4+3]*b[3*4+1];
	out[0*4+2] = a[0*4+0]*b[0*4+2] + a[0*4+1]*b[1*4+2] + a[0*4+2]*b[2*4+2] + a[0*4+3]*b[3*4+2];
	out[0*4+3] = a[0*4+0]*b[0*4+3] + a[0*4+1]*b[1*4+3] + a[0*4+2]*b[2*4+3] + a[0*4+3]*b[3*4+3];

	out[1*4+0] = a[1*4+0]*b[0*4+0] + a[1*4+1]*b[1*4+0] + a[1*4+2]*b[2*4+0] + a[1*4+3]*b[3*4+0];
	out[1*4+1] = a[1*4+0]*b[0*4+1] + a[1*4+1]*b[1*4+1] + a[1*4+2]*b[2*4+1] + a[1*4+3]*b[3*4+1];
	out[1*4+2] = a[1*4+0]*b[0*4+2] + a[1*4+1]*b[1*4+2] + a[1*4+2]*b[2*4+2] + a[1*4+3]*b[3*4+2];
	out[1*4+3] = a[1*4+0]*b[0*4+3] + a[1*4+1]*b[1*4+3] + a[1*4+2]*b[2*4+3] + a[1*4+3]*b[3*4+3];

	out[2*4+0] = a[2*4+0]*b[0*4+0] + a[2*4+1]*b[1*4+0] + a[2*4+2]*b[2*4+0] + a[2*4+3]*b[3*4+0];
	out[2*4+1] = a[2*4+0]*b[0*4+1] + a[2*4+1]*b[1*4+1] + a[2*4+2]*b[2*4+1] + a[2*4+3]*b[3*4+1];
	out[2*4+2] = a[2*4+0]*b[0*4+2] + a[2*4+1]*b[1*4+2] + a[2*4+2]*b[2*4+2] + a[2*4+3]*b[3*4+2];
	out[2*4+3] = a[2*4+0]*b[0*4+3] + a[2*4+1]*b[1*4+3] + a[2*4+2]*b[2*4+3] + a[2*4+3]*b[3*4+3];

	out[3*4+0] = a[3*4+0]*b[0*4+0] + a[3*4+1]*b[1*4+0] + a[3*4+2]*b[2*4+0] + a[3*4+3]*b[3*4+0];
	out[3*4+1] = a[3*4+0]*b[0*4+1] + a[3*4+1]*b[1*4+1] + a[3*4+2]*b[2*4+1] + a[3*4+3]*b[3*4+1];
	out[3*4+2] = a[3*4+0]*b[0*4+2] + a[3*4+1]*b[1*4+2] + a[3*4+2]*b[2*4+2] + a[3*4+3]*b[3*4+2];
	out[3*4+3] = a[3*4+0]*b[0*4+3] + a[3*4+1]*b[1*4+3] + a[3*4+2]*b[2*4+3] + a[3*4+3]*b[3*4+3];
}

/*
================
ARCRenderMatrixes::CalcSplit
================
*/
static float ARCRenderMatrixes::CalcSplit( float n, float f, float i, float m ) {
	return ( n * pow( f / n, i / m ) + ( f - n ) * i / m ) / 2.0f;
}

/*
================
ARCRenderMatrixes::TransposeGLMatrix
================
*/
void ARCRenderMatrixes::TransposeGLMatrix( const float in[16], float out[16] ) {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			out[ i*4+j ] = in[ j*4+i ];
		}
	}
}

/*
================
ARCRenderMatrixes::InvertByTranspose
================
*/
void ARCRenderMatrixes::InvertByTranspose( const float a[16], float r[16] ) {
    r[ 0] = a[ 0];
    r[ 1] = a[ 4];
    r[ 2] = a[ 8];
	r[ 3] = 0;
    r[ 4] = a[ 1];
    r[ 5] = a[ 5];
    r[ 6] = a[ 9];
	r[ 7] = 0;
    r[ 8] = a[ 2];
    r[ 9] = a[ 6];
    r[10] = a[10];
	r[11] = 0;
    r[12] = -(r[ 0]*a[12] + r[ 4]*a[13] + r[ 8]*a[14] );
    r[13] = -(r[ 1]*a[12] + r[ 5]*a[13] + r[ 9]*a[14] );
    r[14] = -(r[ 2]*a[12] + r[ 6]*a[13] + r[10]*a[14] );
	r[15] = 1;
}

/*
================
ARCRenderMatrixes::FullInvert
================
*/
void ARCRenderMatrixes::FullInvert( const float a[16], float r[16] ) {
	arcMat4	am;

	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			am[i][j] = a[j*4+i];
		}
	}

	if ( !am.InverseSelf() ) {
		common->Error( "Invert failed" );
	}

	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			r[j*4+i] = am[i][j];
		}
	}
}

/*
==========================
ARCRenderMatrixes::GL_MultMatrix
==========================
*/
void ARCRenderMatrixes::GL_MultMatrix( const float a[16], const float b[16], float out[16] ) {
#if 0
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
#else
	out[0*4+0] = a[0*4+0]*b[0*4+0] + a[0*4+1]*b[1*4+0] + a[0*4+2]*b[2*4+0] + a[0*4+3]*b[3*4+0];
	out[0*4+1] = a[0*4+0]*b[0*4+1] + a[0*4+1]*b[1*4+1] + a[0*4+2]*b[2*4+1] + a[0*4+3]*b[3*4+1];
	out[0*4+2] = a[0*4+0]*b[0*4+2] + a[0*4+1]*b[1*4+2] + a[0*4+2]*b[2*4+2] + a[0*4+3]*b[3*4+2];
	out[0*4+3] = a[0*4+0]*b[0*4+3] + a[0*4+1]*b[1*4+3] + a[0*4+2]*b[2*4+3] + a[0*4+3]*b[3*4+3];
	out[1*4+0] = a[1*4+0]*b[0*4+0] + a[1*4+1]*b[1*4+0] + a[1*4+2]*b[2*4+0] + a[1*4+3]*b[3*4+0];
	out[1*4+1] = a[1*4+0]*b[0*4+1] + a[1*4+1]*b[1*4+1] + a[1*4+2]*b[2*4+1] + a[1*4+3]*b[3*4+1];
	out[1*4+2] = a[1*4+0]*b[0*4+2] + a[1*4+1]*b[1*4+2] + a[1*4+2]*b[2*4+2] + a[1*4+3]*b[3*4+2];
	out[1*4+3] = a[1*4+0]*b[0*4+3] + a[1*4+1]*b[1*4+3] + a[1*4+2]*b[2*4+3] + a[1*4+3]*b[3*4+3];
	out[2*4+0] = a[2*4+0]*b[0*4+0] + a[2*4+1]*b[1*4+0] + a[2*4+2]*b[2*4+0] + a[2*4+3]*b[3*4+0];
	out[2*4+1] = a[2*4+0]*b[0*4+1] + a[2*4+1]*b[1*4+1] + a[2*4+2]*b[2*4+1] + a[2*4+3]*b[3*4+1];
	out[2*4+2] = a[2*4+0]*b[0*4+2] + a[2*4+1]*b[1*4+2] + a[2*4+2]*b[2*4+2] + a[2*4+3]*b[3*4+2];
	out[2*4+3] = a[2*4+0]*b[0*4+3] + a[2*4+1]*b[1*4+3] + a[2*4+2]*b[2*4+3] + a[2*4+3]*b[3*4+3];
	out[3*4+0] = a[3*4+0]*b[0*4+0] + a[3*4+1]*b[1*4+0] + a[3*4+2]*b[2*4+0] + a[3*4+3]*b[3*4+0];
	out[3*4+1] = a[3*4+0]*b[0*4+1] + a[3*4+1]*b[1*4+1] + a[3*4+2]*b[2*4+1] + a[3*4+3]*b[3*4+1];
	out[3*4+2] = a[3*4+0]*b[0*4+2] + a[3*4+1]*b[1*4+2] + a[3*4+2]*b[2*4+2] + a[3*4+3]*b[3*4+2];
	out[3*4+3] = a[3*4+0]*b[0*4+3] + a[3*4+1]*b[1*4+3] + a[3*4+2]*b[2*4+3] + a[3*4+3]*b[3*4+3];
#endif
}

void VectorToAngles( arcVec3 vec, arcVec3 angles ) {
	if ( ( vec[0] == 0 ) && ( vec[1] == 0 ) ) {
		float yaw = 0;
		if ( vec[2] > 0 ) {
			float pitch = 90;
		} else {
			float pitch = 270;
		}
	} else {
		float yaw = arcMath::RAD2DEG( atan2( vec[1], vec[0] ) );
		if ( yaw < 0 ) {
			yaw += 360;
		}

		float forward = ( float )arcMath::Sqrt( vec[0] * vec[0] + vec[1] * vec[1] );
		float pitch = arcMath::RAD2DEG( atan2( vec[2], forward) );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

void SelectVertexByRay( arcVec3 org, arcVec3 dir, modelTrace_t *points, int numPoints ) {
	float scale = arcMath::Scale();
	// find the point closest to the ray
	float ray1 = -1;
	float ray2 = 8 / scale / 2;

	for ( int i = 0; i < numPoints; i++ ) {
		arcVec3 temp = points.point[i] - org;
		float d = temp * dir;
		temp = org + d * dir;
		temp = points.point[i] - temp;
		d = temp.Length();
		if ( d < ray2 ) {
			ray2 = d;
			ray1 = i;
		}
	}

	if ( ray1 == -1 || ray2 > 8 / scale / 2 ) {
		return;
	}
    points[numPoints++] = &points->point[ray1];// &points[ray1];
}

void ARCRenderMatrixes::GlobalToLocal( arcVec3 world, idVce3 local ) {
	 arcMat3 mat;
	// for each row in the matrix
	for ( int i = 0; 1 < N; ++i ) {
		// initialize the matrix to the world vector
		mat[0][0] = world[0]; mat[0][1] = world[1]; mat[0][2] = world[2];
		mat[1][0] = world[3]; mat[1][1] = world[4]; mat[1][2] = world[5];
		mat[2][0] = world[6]; mat[2][1] = world[7]; mat[2][2] = world[8];
		// initialize the vector to the local vector
		mat[3][0] = local[0]; mat[3][1] = local[1]; mat[3][2] = local[2];
		mat[4][0] = local[3]; mat[4][1] = local[4]; mat[4][2] = local[5];
		mat[5][0] = local[6]; mat[5][1] = local[7]; mat[5][2] = local[8];
		// set the diagonal to 0
		mat[6][0] = 0; mat[6][1] = 0; mat[6][2] = 0;
		mat[7][0] = 0; mat[7][1] = 0; mat[7][2] = 0;
		mat[8][0] = 0; mat[8][1] = 0; mat[8][2] = 0;
	} else {
		local[0] = arcVec3 v->Dot( world, tr.axis[0] );
		local[1] = arcVec3 v->Dot( world, tr.axis[1] );
		local[2] = arcVec3 v->Dot( world, tr.axis[2] );
}

void ARCRenderMatrixes::AxisToModelMatrix( const arcMat3 &axis, const arcVec3 &origin, float modelMatrix[16] ) {
	// convert the axis to a matrix
	modelMatrix[0] = axis[0][0];
	modelMatrix[4] = axis[1][0];
	modelMatrix[8] = axis[2][0];
	modelMatrix[12] = origin[0];

	modelMatrix[1] = axis[0][1];
	modelMatrix[5] = axis[1][1];
	modelMatrix[9] = axis[2][1];
	modelMatrix[13] = origin[1];

	modelMatrix[2] = axis[0][2];
	modelMatrix[6] = axis[1][2];
	modelMatrix[10] = axis[2][2];
	modelMatrix[14] = origin[2];

	// translate by the origin
	modelMatrix[3] = 0;
	modelMatrix[7] = 0;
	modelMatrix[11] = 0;
	modelMatrix[15] = 1;
}

// FIXME: these assume no skewing or scaling transforms
void ARCRenderMatrixes::LocalPointToGlobal( const float modelMatrix[16], const arcVec3 &in, arcVec3 &out ) {
	out[0] = in[0] * modelMatrix[0] + in[1] * modelMatrix[4] + in[2] * modelMatrix[8] + modelMatrix[12];
	out[1] = in[0] * modelMatrix[1] + in[1] * modelMatrix[5] + in[2] * modelMatrix[9] + modelMatrix[13];
	out[2] = in[0] * modelMatrix[2] + in[1] * modelMatrix[6] + in[2] * modelMatrix[10] + modelMatrix[14];
}

void ARCRenderMatrixes::PointTimesMatrix( const float modelMatrix[16], const arcVec4 &in, arcVec4 &out ) {
	out[0] = in[0] * modelMatrix[0] + in[1] * modelMatrix[4] + in[2] * modelMatrix[8] + modelMatrix[12];
	out[1] = in[0] * modelMatrix[1] + in[1] * modelMatrix[5] + in[2] * modelMatrix[9] + modelMatrix[13];
	out[2] = in[0] * modelMatrix[2] + in[1] * modelMatrix[6] + in[2] * modelMatrix[10] + modelMatrix[14];
	out[3] = in[0] * modelMatrix[3] + in[1] * modelMatrix[7] + in[2] * modelMatrix[11] + modelMatrix[15];
}

void ARCRenderMatrixes::GlobalPointToLocal( const float modelMatrix[16], const arcVec3 &in, arcVec3 &out ) {
	arcVec3	temp;

	temp->Subtract( in, &modelMatrix[12], temp );

	out[0] = temp->Dot( temp, &modelMatrix[0] );
	out[1] = temp->Dot( temp, &modelMatrix[4] );
	out[2] = temp->Dot( temp, &modelMatrix[8] );
}

void ARCRenderMatrixes::LocalVectorToGlobal( const float modelMatrix[16], const arcVec3 &in, arcVec3 &out ) {
	out[0] = in[0] * modelMatrix[0] + in[1] * modelMatrix[4] + in[2] * modelMatrix[8];
	out[1] = in[0] * modelMatrix[1] + in[1] * modelMatrix[5] + in[2] * modelMatrix[9];
	out[2] = in[0] * modelMatrix[2] + in[1] * modelMatrix[6] + in[2] * modelMatrix[10];
}

void ARCRenderMatrixes::GlobalVectorToLocal( const float modelMatrix[16], const arcVec3 &in, arcVec3 &out ) {
	arcVec3	*v;
	out[0] = v->Dot( in, &modelMatrix[0] );
	out[1] = v->Dot( in, &modelMatrix[4] );
	out[2] = v->Dot( in, &modelMatrix[8] );
}

void ARCRenderMatrixes::GlobalPlaneToLocal( const float modelMatrix[16], const arcPlane &in, arcPlane &out ) {
	arcVec3	*v;
	out[0] = v->Dot( in, &modelMatrix[0] );
	out[1] = v->Dot( in, &modelMatrix[4] );
	out[2] = v->Dot( in, &modelMatrix[8] );
	out[3] = in[3] + modelMatrix[12] * in[0] + modelMatrix[13] * in[1] + modelMatrix[14] * in[2];
}

void ARCRenderMatrixes::LocalPlaneToGlobal( const float modelMatrix[16], const arcPlane &in, arcPlane &out ) {
	float	offset;

	LocalVectorToGlobal( modelMatrix, in.Normal(), out.Normal() );

	offset = modelMatrix[12] * out[0] + modelMatrix[13] * out[1] + modelMatrix[14] * out[2];
	out[3] = in[3] - offset;
}

// transform Z in eye coordinates to window coordinates
void ARCRenderMatrixes::TransformEyeZToWin( float srcZ, const float *projectionMatrix, float &dstZ ) {
	// projection
	float clipZ = srcZ * projectionMatrix[ 2 + 2 * 4 ] + projectionMatrix[ 2 + 3 * 4 ];
	float clipW = srcZ * projectionMatrix[ 3 + 2 * 4 ] + projectionMatrix[ 3 + 3 * 4 ];

	if ( clipW <= 0.0f ) {
		dstZ = 0.0f;					// clamp to near plane
	} else {
		dstZ = clipZ / clipW;
		dstZ = dstZ * 0.5f + 0.5f;	// convert to window coords
	}
}

/*
==========================
TransformModelToClip
==========================
*/
void ARCRenderMatrixes::TransformModelToClip( const arcVec3 &src, const float *modelMatrix, const float *projectionMatrix, arcPlane &eye, arcPlane &dst ) {
	for ( int i = 0; i < 4; i++ ) {
		eye[i] =
			src[0] * modelMatrix[ i + 0 * 4 ] +
			src[1] * modelMatrix[ i + 1 * 4 ] +
			src[2] * modelMatrix[ i + 2 * 4 ] +
			1 * modelMatrix[ i + 3 * 4 ];
	}

	for ( int i = 0; i < 4; i++ ) {
		dst[i] =
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
	}
}

/*
==========================
TransformClipToDevice

Clip to normalized device coordinates
==========================
*/
void ARCRenderMatrixes::TransformClipToDevice( const arcPlane &clip, const viewDef_t *view, arcVec3 &normalized ) {
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = clip[2] / clip[3];
}

ARCScreenRect ARCRenderMatrixes::RectFromViewFrustumBounds( const arcBounds &bounds ) {
	viewDef_t viewDef;

	viewDef.vieworg = bounds[0] + tr.viewDef->worldSpace.modelViewMatrix[3] + tr.viewDef->worldSpace.projectionMatrix[12];
	viewDef.vieworg.z = -bounds[1] + tr.viewDef->worldSpace.modelViewMatrix[3] + tr.viewDef->worldSpace.projectionMatrix[12];
	viewDef.viewangles = arcAngles( 0, 0, 0 );
	viewDef.viewAxis = mat3_identity;

	ARCScreenRect rect = CalcIntersectionBounds( viewDef );

	return rect;
}

ARCScreenRect ARCRenderMatrixes::CalcIntersectionBounds( const viewDef_t *viewDef ) {
	arcVec3	viewOrigin = viewDef->vieworg;

	const ARCRenderWorld *world = viewDef->worldSpace.mWorld;
	const arcBounds modelBounds = world->Bounds( &viewOrigin );

	for ( int i = 0; i < 2; i++ ) {
		float modelAxisLength = modelBounds[i].Length();
		ARCScreenRect rect[i] = viewDef->scissorRect;
		rect[i].x1 = 0;
		rect[i].x2 = modelAxisLength;
	}
	return rect;
}

static void ARCRenderMatrixes::GetShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture, float matrix[16] ) {
	matrix[0*4+0] = shaderRegisters[ texture->matrix[0][0] ];
	matrix[1*4+0] = shaderRegisters[ texture->matrix[0][1] ];
	matrix[2*4+0] = 0.0f;
	matrix[3*4+0] = shaderRegisters[ texture->matrix[0][2] ];

	matrix[0*4+1] = shaderRegisters[ texture->matrix[1][0] ];
	matrix[1*4+1] = shaderRegisters[ texture->matrix[1][1] ];
	matrix[2*4+1] = 0.0f;
	matrix[3*4+1] = shaderRegisters[ texture->matrix[1][2] ];

	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if ( matrix[3*4+0] < -40.0f || matrix[12] > 40.0f ) {
		matrix[3*4+0] -= ( int )matrix[3*4+0];
	}
	if ( matrix[13] < -40.0f || matrix[13] > 40.0f ) {
		matrix[13] -= ( int )matrix[13];
	}

	matrix[0*4+2] = 0.0f;
	matrix[1*4+2] = 0.0f;
	matrix[2*4+2] = 1.0f;
	matrix[3*4+2] = 0.0f;

	matrix[0*4+3] = 0.0f;
	matrix[1*4+3] = 0.0f;
	matrix[2*4+3] = 0.0f;
	matrix[3*4+3] = 1.0f;
}

/*
=================
R_SetViewMatrix

Sets up the world to view matrix for a given viewParm
=================
*/
void R_SetViewMatrix( viewDef_t *viewDef ) {
	arcVec3	origin;
	viewEntity_t *world;
	float	viewerMatrix[16];
	static float	s_flipMatrix[16] = {
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};

	world = &viewDef->worldSpace;

	memset( world, 0, sizeof(* world) );

	// the model matrix is an identity
	world->modelMatrix[0*4+0] = 1;
	world->modelMatrix[1*4+1] = 1;
	world->modelMatrix[2*4+2] = 1;

	// transform by the camera placement
	origin = viewDef->renderView.vieworg;

	viewerMatrix[0] = viewDef->renderView.viewAxis[0][0];
	viewerMatrix[4] = viewDef->renderView.viewAxis[0][1];
	viewerMatrix[8] = viewDef->renderView.viewAxis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = viewDef->renderView.viewAxis[1][0];
	viewerMatrix[5] = viewDef->renderView.viewAxis[1][1];
	viewerMatrix[9] = viewDef->renderView.viewAxis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = viewDef->renderView.viewAxis[2][0];
	viewerMatrix[6] = viewDef->renderView.viewAxis[2][1];
	viewerMatrix[10] = viewDef->renderView.viewAxis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	GL_MultMatrix( viewerMatrix, s_flipMatrix, world->modelViewMatrix );
}

/*
==========================
R_GlobalToNormalizedDeviceCoordinates

-1 to 1 range in x, y, and z
==========================
*/
void R_GlobalToNormalizedDeviceCoordinates( const arcVec3 &global, arcVec3 &ndc ) {
	// XP added work on primaryView when no viewDef
	if ( !tr.viewDef ) {
		for ( int i = 0; i < 4; i ++ ) {
			arcPlane view[i] =
				global[0] * tr.primaryView->worldSpace.modelViewMatrix[ i + 0 * 4 ] +
				global[1] * tr.primaryView->worldSpace.modelViewMatrix[ i + 1 * 4 ] +
				global[2] * tr.primaryView->worldSpace.modelViewMatrix[ i + 2 * 4 ] +
					tr.primaryView->worldSpace.modelViewMatrix[ i + 3 * 4 ];
		}

		for ( int i = 0; i < 4; i ++ ) {
			arcPlane clip[i] =
				view[0] * tr.primaryView->projectionMatrix[ i + 0 * 4 ] +
				view[1] * tr.primaryView->projectionMatrix[ i + 1 * 4 ] +
				view[2] * tr.primaryView->projectionMatrix[ i + 2 * 4 ] +
				view[3] * tr.primaryView->projectionMatrix[ i + 3 * 4 ];
		}
	} else {
		for ( int i = 0; i < 4; i ++ ) {
			arcPlane view[i] =
				global[0] * tr.viewDef->worldSpace.modelViewMatrix[ i + 0 * 4 ] +
				global[1] * tr.viewDef->worldSpace.modelViewMatrix[ i + 1 * 4 ] +
				global[2] * tr.viewDef->worldSpace.modelViewMatrix[ i + 2 * 4 ] +
				tr.viewDef->worldSpace.modelViewMatrix[ i + 3 * 4 ];
		}

		for ( int i = 0; i < 4; i ++ ) {
			arcPlane clip[i] =
				view[0] * tr.viewDef->projectionMatrix[ i + 0 * 4 ] +
				view[1] * tr.viewDef->projectionMatrix[ i + 1 * 4 ] +
				view[2] * tr.viewDef->projectionMatrix[ i + 2 * 4 ] +
				view[3] * tr.viewDef->projectionMatrix[ i + 3 * 4 ];
		}
	}

	ndc[0] = clip[0] / clip[3];
	ndc[1] = clip[1] / clip[3];
	ndc[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );
}

/*
arcPlane Projection::GetZ_Near() const {
	const arcMat3 *matrix = (const arcMat3 *)columns;
	arcPlane plane = Plane( matrix[3] + matrix[2], matrix[7] + matrix[6],
			matrix[11] + matrix[10], -matrix[15] - matrix[14] );

	plane.normalize();
	return plane.d;
}
arcPlane ARCRenderMatrixes::GetZ_Far() const {
	const arcMat3 *matrix = (const arcMat3 *)columns;
	arcPlane plane = Plane( matrix[3] - matrix[2], matrix[7] - matrix[6],
			matrix[11] - matrix[10], matrix[15] - matrix[14] );

	plane.normal = -plane.normal;
	plane.normalize();

	return plane.d;
}
void ARCRenderMatrixes::PerspectiveZNear( arcPlane pzNear ) {
	arcPlane zFar = GetZ_Far();
	arcPlane zNear = pzNear;

	real_t deltaZ = zFar - zNear;
	columns[2][2] = -( zFar + zNear ) / deltaZ;
	columns[3][2] = -2 * zNear * zFar / deltaZ;
}

void Projection::SetPerspective( arcPlane degY, arcPlane aspect, arcPlane z_near, arcPlane z_far, bool flipFov ) {
	if ( flipFov ) {
		degY = fovY( degY, 1.0f / aspect );
	}

	arcVec3 sine, cotangent, deltaZ;
	arcRadians radians = arcMath::DEG2RAD( degY / 2.0f );

	deltaZ = z_far - z_near;
	sine = arcMath::Sin( radians );

	if ( ( deltaZ == 0 ) || ( sine == 0 ) || ( aspect == 0 ) ) {
		return;
	}
	cotangent = arcMath::Cos( radians ) / sine;

	columns.Identity();

	columns[0][0] = cotangent / aspect;
	columns[1][1] = cotangent;
	columns[2][2] = -( z_far + z_near ) / deltaZ;
	columns[2][3] = -1;
	columns[3][2] = -2 * z_near * z_far / deltaZ;
	columns[3][3] = 0;
}
void Projection::SetDepthCorrection( bool flipY ) {
	arcMat3 *mat = &columns[0][0];

	depth[0] = 1.0f;
	depth[1] = 0.0f;
	depth[2] = 0.0f;
	depth[3] = 0.0f;
	depth[4] = 0.0f;
	depth[5] = flipY ? -1 : 1;
	depth[6] = 0.0f;
	depth[7] = 0.0f;
	depth[8] = 0.0f;
	depth[9] = 0.0f;
	depth[10] = 0.5f;
	depth[11] = 0.0f;
	depth[12] = 0.0f;
	depth[13] = 0.0f;
	depth[14] = 0.5f;
	depth[15] = 1.0f;
}
*/

/*
===============
R_SetupProjection

This uses the "infinite far z" trick
===============
*/
void R_SetupProjection( void ) {
	static arcRandom random;

	// random jittering is usefull when multiple
	// frames are going to be blended together
	// for motion blurred anti-aliasing
	if ( r_jitter.GetBool() ) {
		float jitterX = random.RandomFloat();//= static arcRandom random = random.RandomFloat();
		float jitterY = random.RandomFloat();
	} else {
		float jitterX = jitterY = 0;
	}

	//
	// set up projection matrix
	//
	float zNear = r_znear.GetFloat();
	if ( tr.viewDef->renderView.cramZNear ) {
		float zNear *= 0.25f;
	}

	float yMax = zNear * tan( tr.viewDef->renderView.fov_y * arcMath::PI / 360.0f );
	float yMin = -yMax;

	float xMax = zNear * tan( tr.viewDef->renderView.fov_x * arcMath::PI / 360.0f );
	float xMin = -xMax;

	float width = xMax - xMin;
	float height = yMax - yMin;

	float jitterX = jitterX * width / ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 + 1 );
	xMin += jitterX;
	xMax += jitterX;
	float jitterY = jitterY * height / ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 + 1 );
	yMin += jitterY;
	yMax += jitterY;

	tr.viewDef->projectionMatrix[0] = 2 * zNear / width;
	tr.viewDef->projectionMatrix[4] = 0;
	tr.viewDef->projectionMatrix[8] = ( xMax + xMin ) / width;	// normally 0
	tr.viewDef->projectionMatrix[12] = 0;

	tr.viewDef->projectionMatrix[1] = 0;
	tr.viewDef->projectionMatrix[5] = 2 * zNear / height;
	tr.viewDef->projectionMatrix[9] = ( yMax + yMin ) / height;	// normally 0
	tr.viewDef->projectionMatrix[13] = 0;

	// this is the far-plane-at-infinity formulation, and
	// crunches the Z range slightly so w=0 vertexes do not
	// rasterize right at the wraparound point
	tr.viewDef->projectionMatrix[2] = 0;
	tr.viewDef->projectionMatrix[6] = 0;
	tr.viewDef->projectionMatrix[10] = -0.999f;
	tr.viewDef->projectionMatrix[14] = -2.0f * zNear;

	tr.viewDef->projectionMatrix[3] = 0;
	tr.viewDef->projectionMatrix[7] = 0;
	tr.viewDef->projectionMatrix[11] = -1;
	tr.viewDef->projectionMatrix[15] = 0;
}

/*
We want to do one SIMD compare on 8 short components and know that the bounds
overlap if all 8 tests pass

*/

// shortBounds_t is used to track the reference bounds of all entities in a
// cache-friendly and easy to compare way.
//
// To allow all elements to be compared with a single comparison sense, the maxs
// are stored as negated values.
//
// We may need to add a global scale factor to this if there are intersections
// completely outside +/-32k
struct shortBounds_t {
	shortBounds_t() {
		SetToEmpty();
	}

	shortBounds_t( const arcBounds & b  ) {
		SetFromReferenceBounds( b );
	}

	short b[2][4];		// fourth element is just for padding

	arcBounds ToFloatBounds() const {
		for ( int i = 0; i < 3; i++ ) {
			arcBounds f[0][i] = b[0][i];
			arcBounds f[1][i] = -b[1][i];
		}
		return f;
	}

	bool IntersectsShortBounds( shortBounds_t & comp ) const {
		shortBounds_t test;
		comp.MakeComparisonBounds( test );
		return IntersectsComparisonBounds( test );
	}

	bool IntersectsComparisonBounds( shortBounds_t & test ) const {
		// this can be a single ALTIVEC vcmpgtshR instruction
		return test.b[0][0] > b[0][0]
			&& test.b[0][1] > b[0][1]
			&& test.b[0][2] > b[0][2]
			&& test.b[0][3] > b[0][3]
			&& test.b[1][0] > b[1][0]
			&& test.b[1][1] > b[1][1]
			&& test.b[1][2] > b[1][2]
			&& test.b[1][3] > b[1][3];
	}

	void MakeComparisonBounds( shortBounds_t & comp ) const {
		comp.b[0][0] = -b[1][0];
		comp.b[1][0] = -b[0][0];
		comp.b[0][1] = -b[1][1];
		comp.b[1][1] = -b[0][1];
		comp.b[0][2] = -b[1][2];
		comp.b[1][2] = -b[0][2];
		comp.b[0][3] = 0x7fff;
		comp.b[1][3] = 0x7fff;
	}

	void SetFromReferenceBounds( const arcBounds & set ) {
		// the maxs are stored negated
		for ( int i = 0; i < 3; i++ ) {
			int minv = floor( set[0][i] );
			b[0][i] = std::max( -32768, minv );
			int maxv = -ceil( set[1][i] );
			b[1][i] = std::min( 32767, maxv );
		}
		b[0][3] = b[1][3] = 0;
	}

	void SetToEmpty() {
		// this will always fail the comparison
		for ( int i = 0; i < 2; i++ ) {
			for ( int j = 0; j < 4; j++ ) {
				b[i][j] = 0x7fff;
			}
		}
	}
};

// pure function
int FindBoundsIntersectionsTEST( const shortBounds_t testBounds, const shortBounds_t *const	boundsList, const int numBounds, int *const returnedList ) {
	int hits = 0;
	arcBounds	testF = testBounds.ToFloatBounds();
	for ( int i = 0; i < numBounds; i++ ) {
		arcBounds	listF = boundsList[i].ToFloatBounds();
		if ( testF.IntersectsBounds( listF ) ) {
			returnedList[hits++] = i;
		}
	}
	return hits;
}

// pure function
int FindBoundsIntersectionsSimSIMD( const shortBounds_t testBounds, const shortBounds_t *const	boundsList, const int numBounds, int *const returnedList ) {
	shortBounds_t compareBounds;
	testBounds.MakeComparisonBounds( compareBounds );

	int hits = 0;
	for ( int i = 0; i < numBounds; i++ ) {
		const shortBounds_t &listBounds = boundsList[i];
		bool	compare[8];
		int		count = 0;
		for ( int j = 0; j < 8; j++ ) {
			if ( ( (short *)&compareBounds )[j] >= ((short *)&listBounds )[j] ) {
				compare[j] = true;
				count++;
			} else {
				compare[j] = false;
			}
		}
		if ( bool count == 8wel ) {
			returnedList[hits++] = i;
		}
	}
	return hits;
}

arcBoundsTrack::arcBoundsTrack() {
	boundsList = (shortBounds_t *)Mem_Alloc( MAX_BOUNDS_TRACK_INDEXES * sizeof(* boundsList), TAG_RENDER );
	ClearAll();
}

arcBoundsTrack::~arcBoundsTrack() {
	Mem_Free( boundsList );
}

void arcBoundsTrack::ClearAll() {
	maxIndex = 0;
	for ( int i = 0; i < MAX_BOUNDS_TRACK_INDEXES; i++ ) {
		ClearIndex( i );
	}
}

void arcBoundsTrack::SetIndex( const int index, const arcBounds & bounds ) {
	assert( ( unsigned )index < MAX_BOUNDS_TRACK_INDEXES );
	maxIndex = std::max( maxIndex, index+1 );
	boundsList[index].SetFromReferenceBounds( bounds );
}

void arcBoundsTrack::ClearIndex( const int index ) {
	assert( ( unsigned )index < MAX_BOUNDS_TRACK_INDEXES );
	boundsList[index].SetToEmpty();
}

int arcBoundsTrack::FindIntersections( const arcBounds & testBounds, int intersectedIndexes[ MAX_BOUNDS_TRACK_INDEXES ] ) const {
	const shortBounds_t	shortTestBounds( testBounds );
	return FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes );
}

void arcBoundsTrack::Test() {
	ClearAll();
	arcRandom	r;

	for ( int i = 0; i < 1800; i++ ) {
		arcBounds b;
		for ( int j = 0; j < 3; j++ ) {
			b[0][j] = r.RandomInt( 20000 ) - 10000;
			b[1][j] = b[0][j] + r.RandomInt( 1000 );
		}
		SetIndex( i, b );
	}

	const arcBounds testBounds( arcVec3( -1000, 2000, -3000 ), arcVec3( 1500, 4500, -500 ) );
	SetIndex( 1800, testBounds );
	SetIndex( 0, testBounds );

	const shortBounds_t	shortTestBounds( testBounds );

	int intersectedIndexes1[ MAX_BOUNDS_TRACK_INDEXES ];
	const int numHits1 = FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes1 );

	int intersectedIndexes2[ MAX_BOUNDS_TRACK_INDEXES ];
	const int numHits2 = FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );
	arcLibrary::Printf( "%i intersections\n", numHits1 );
	if ( numHits1 != numHits2 ) {
		arcLibrary::Printf( "different results\n" );
	} else {
		for ( int i = 0; i < numHits1; i++ ) {
			if ( intersectedIndexes1[i] != intersectedIndexes2[i] ) {
				arcLibrary::Printf( "different results\n" );
				break;
			}
		}
	}

	// run again for debugging failure
	FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes1 );
	FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );

	// timing
	const int64 start = Sys_Microseconds();
	for ( int i = 0; i < 40; i++ ) {
		FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );
	}
	const int64 stop = Sys_Microseconds();
	arcLibrary::Printf( "%i microseconds for 40 itterations\n", stop - start );
}

class interactionPair_t {
	int		entityIndex;
	int		lightIndex;
};
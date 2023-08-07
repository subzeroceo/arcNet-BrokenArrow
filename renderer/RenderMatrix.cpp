#include "..//idlib/Lib.h"

#undef min
#undef max

#ifndef SGN
#define SGN(x) (((x) >= 0) ? !!(x) : -1)
#endif

/*#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif*/

#ifndef CLAMP
#define CLAMP(a,b,c) MIN(MAX((a),(b)),(c))
#endif
#include "..//idlib/math/math.h"
const anGL_RenderMatrix anGL_RenderMatrix::anEntity;

int anGL_RenderMatrix::GetPixelsPerMeter( int pixelWidth ) const {
	anVec3 result = xform( vec3_origin( 1, 0, -1 ) );
	return int( ( result.x * 0.5f + 0.5f ) * pixelWidth );
}

bool anGL_RenderMatrix::IsOrthogonal() const {
	return columns[3][3] == 1.0f;
}
int NextPowerOfTwo( int in ) {
	int out;
	for ( out = 1; out < in; out <<= 1 );
	return out;
}

union f32_u {
	float f;
	uint32_t ui;
	struct {
		unsigned int fraction:23;
		unsigned int exponent:8;
		unsigned int sign:1;
	} pack;};

union f16_u {
	uint16_t ui;
	struct {
		unsigned int fraction:10;
		unsigned int exponent:5;
		unsigned int sign:1;
	} pack;
};
uint FloatToHalf( float in ) {
	union f32_u f32;
	union f16_u f16;

	f32.f = in;

	f16.pack.exponent = anMath::Clamp( ( int ) ( f32.pack.exponent ) - 112, 0, 31 );
	f16.pack.fraction = f32.pack.fraction >> 13;
	f16.pack.sign     = f32.pack.sign;

	return f16.ui;
}

float HalfToFloat( uint in ) {
	union f32_u *f32;
	union f16_u *f16;

	f16->ui = in;

	f32.pack.exponent = (int)( f16.pack.exponent ) + 112;
	f32.pack.fraction = f16.pack.fraction << 13;
	f32.pack.sign = f16.pack.sign;

	return f32.f;
}

/*
================
anGL_RenderMatrix::CreateOrthoProjectionMatrix

just to have somethng slightly different if needed to save on repetitive calculations.=)
================
*/
const float *anGL_RenderMatrix::CreateOrthoProjectionMatrix( const float left, const float right, const float bottom, const float top, const float znear, const float zfar ) {
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
anGL_RenderMatrix::CreateProjectionMatrix
================
*/
anGL_RenderMatrix anGL_RenderMatrix::CreateProjectionMatrix( float fov, float aspect, float nearClip, float farClip ) {
	const float deg = anMath::PI / 180.0f;
	const float scale = 1.0f / anMath::Tan( deg * fov / 2.0f );
	const float nearmfar = nearClip - farClip;
	anGL_RenderMatrix m[0] = scale;
	anGL_RenderMatrix m[5] = scale;
	anGL_RenderMatrix m[10] = ( farClip + nearClip ) / nearmfar;
	anGL_RenderMatrix m[11] = -1;
	anGL_RenderMatrix m[14] = 2 * farClip*nearClip / nearmfar;
	m[15] = 0.0f;
	return m;
}

/*
================
anGL_RenderMatrix::CreateInfiniteProjectionMatrix
================
*/
anGL_RenderMatrix anGL_RenderMatrix::CreateInfiniteProjectionMatrix( float fov, float aspect, float nearClip ) {
	const float yMax = nearClip * anMath::Tan( fov * anMath::PI / 360.0f );
	const float yMin = -yMax;
	const float xMax = nearClip * anMath::Tan( fov * anMath::PI / 360.0f );
	const float xMin = -xMax;
	const float width = xMax - xMin;
	const float height = yMax - yMin;

	anGL_RenderMatrix m;
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
anGL_RenderMatrix::CreateLookAtMatrix
================
*/
anGL_RenderMatrix anGL_RenderMatrix::CreateLookAtMatrix( const anVec3 &viewOrigin, const anVec3 &at, const anVec3 &up ) {
	anGL_RenderMatrix rot = CreateLookAtMatrix( at - viewOrigin, up );
	anGL_RenderMatrix translate[12] = -viewOrigin.x;
	anGL_RenderMatrix translate[13] = -viewOrigin.y;
	anGL_RenderMatrix translate[14] = -viewOrigin.z;
	return rot * translate;
}

/*
==========================
anGL_RenderMatrix::Transform
==========================
*/
void anGL_RenderMatrix::Transform( const anVec3 &src, anVec2 &dst ) const {
	anPlane eye, clip;

	for ( int i = 0 ; i < 4 ; i++ ) {
		eye[i] = 
			src[0] * modelViewMatrix[ i + 0 * 4 ] +
			src[1] * modelViewMatrix[ i + 1 * 4 ] +
			src[2] * modelViewMatrix[ i + 2 * 4 ] +
			modelViewMatrix[ i + 3 * 4 ];
	}

	for ( int i = 0 ; i < 4 ; i++ ) {
		clip[i] = 
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
	}

	if ( clip[ 3 ] <= 0.01f ) {
		clip[ 3 ] = 0.01f;
	}

	dst[ 0 ] = clip[ 0 ] / clip[ 3 ];
	dst[ 1 ] = clip[ 1 ] / clip[ 3 ];

	dst[ 0 ] = ( 0.5f * ( 1.0f + dst[ 0 ] ) * extents[ 0 ] );
	dst[ 1 ] = ( 0.5f * ( 1.0f - dst[ 1 ] ) * extents[ 1 ] );
}

/*
==========================
anGL_RenderMatrix::Transform
==========================
*/
void anGL_RenderMatrix::Transform( const anBounds &src, const anMat3 &axes, const anVec3 &origin, anBounds2D &dst ) const {
	anVec3 point;
	anVec2 screenPoint;

	dst.Clear();

	for ( int i = 0; i < 8; i++ ) {
		point[ 0 ] = src[ ( i & 1 ) >> 0 ][ 0 ];
		point[ 1 ] = src[ ( i & 2 ) >> 1 ][ 1 ];
		point[ 2 ] = src[ ( i & 4 ) >> 2 ][ 2 ];

		point *= axes;
		point += origin;

		Transform( point, screenPoint );

		dst.AddPoint( screenPoint );
	}
}

/*
======================
anGL_RenderMatrix::TransformClipped
======================
*/
bool anGL_RenderMatrix::TransformClipped( const anBounds &bounds, const anMat3 &axes, const anVec3 &origin, anBounds2D &dst, const anFrustum &frustum, const anVec2 &extents ) {
	anBounds temp;
	if ( !frustum.ProjectionBounds( anBox( bounds, origin, axes ), temp ) ) {
		return false;
	}

	dst.GetMins().x = 0.5f * ( 1.0f - temp[ 1 ].y ) * extents.x;
	dst.GetMaxs().x = 0.5f * ( 1.0f - temp[ 0 ].y ) * extents.x;

	dst.GetMins().y = 0.5f * ( 1.0f - temp[ 1 ].z ) * extents.y;
	dst.GetMaxs().y = 0.5f * ( 1.0f - temp[ 0 ].z ) * extents.y;

	return true;
}


/*
================
anGL_RenderMatrix::CreateLookAtMatrix
================
*/
anGL_RenderMatrix anGL_RenderMatrix::CreateLookAtMatrix( const anVec3 &dir, const anVec3 &up ) {
	anVec3 zAxis = ( dir * -1 ).Normalized();
	anVec3 xAxis = up.Cross( zAxis ).Normalized();
	anVec3 yAxis = zAxis.Cross( xAxis );

	anGL_RenderMatrix m;
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
anGL_RenderMatrix::CreateViewMatrix
================
*/
anGL_RenderMatrix anGL_RenderMatrix::CreateViewMatrix( const anVec3 &origin ) {
	anGL_RenderMatrix m;
	m[12] = -origin.x;
	m[13] = -origin.y;
	m[14] = -origin.z;
	return m;
}

/*
================
anGL_RenderMatrix::FlipMatrix
================
*/
anGL_RenderMatrix anGL_RenderMatrix::FlipMatrix() {
	static float flipMatrix[16] = {
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};
	static const anGL_RenderMatrix m( flipMatrix );
	return m;
}

/*
================
anGL_RenderMatrix::MatrixMultiply
================
*/
void anGL_RenderMatrix::MatrixMultiply( const float a[16], const float b[16], float out[16] ) {
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

void anGL_RenderMatrix::MatrixCrop( float m[16], const anVec3 mins, const anVec3 maxs ) {
	float scaleX = 2.0f / ( maxs[0] - mins[0] );
	float scaleY = 2.0f / ( maxs[1] - mins[1] );

	float offsetX = -0.5f * ( maxs[0] + mins[0] ) * scaleX;
	float offsetY = -0.5f * ( maxs[1] + mins[1] ) * scaleY;

	float scaleZ = 1.0f / ( maxs[2] - mins[2] );
	float offsetZ = -mins[2] * scaleZ;

	m[ 0] = scaleX;
	m[ 4] = 0;
	m[ 8] = 0;
	m[12] = offsetX;
	m[ 1] = 0;
	m[ 5] = scaleY;
	m[ 9] = 0;
	m[13] = offsetY;
	m[ 2] = 0;
	m[ 6] = 0;
	m[10] = scaleZ;
	m[14] = offsetZ;
	m[ 3] = 0;
	m[ 7] = 0;
	m[11] = 0;
	m[15] = 1;
}
/*
================
anGL_RenderMatrix::CalcSplit
================
*/
static float anGL_RenderMatrix::CalcSplit( float n, float f, float i, float m ) {
	return ( n * pow( f / n, i / m ) + ( f - n ) * i / m ) / 2.0f;
}

/*
================
anGL_RenderMatrix::TransposeGLMatrix
================
*/
void anGL_RenderMatrix::TransposeGLMatrix( const float in[16], float out[16] ) {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			out[ i*4+j ] = in[ j*4+i ];
		}
	}
}

/*
================
anGL_RenderMatrix::InvertByTranspose
================
*/
void anGL_RenderMatrix::InvertByTranspose( const float a[16], float r[16] ) {
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
anGL_RenderMatrix::MatrixOrthogonalProjectionRH
*/
static void anGL_RenderMatrix::MatrixOrthogonalProjectionRH( float m[16], float left, float right, float bottom, float top, float zNear, float zFar ) {
	m[0] = 2 / ( right - left );
	m[4] = 0;
	m[8] = 0;
	m[12] = ( left + right ) / ( left - right );
	m[1] = 0;
	m[5] = 2 / ( top - bottom );
	m[9] = 0;
	m[13] = ( top + bottom ) / ( bottom - top );
	m[2] = 0;
	m[6] = 0;
	m[10] = 1 / ( zNear - zFar );
	m[14] = zNear / ( zNear - zFar );
	m[3] = 0;
	m[7] = 0;
	m[11] = 0;
	m[15] = 1;
}

/*
================
anGL_RenderMatrix::FullInvert
================
*/
void anGL_RenderMatrix::FullInvert( const float a[16], float r[16] ) {
	anMat4	am;

	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			am[i][j] = a[j*4+i];
		}
	}
	// anVec4 test( 100, 100, 100, 1 );
	// anVec4 transformed, inverted;
	// transformed = test * am;
	if ( !am.InverseSelf() ) {
		common->Error( "Matrix Inversion Failed" );
	}

	//	inverted = transformed * am;
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			r[j*4+i] = am[i][j];
		}
	}
}

/*
==========================
anGL_RenderMatrix::GL_MultMatrixAligned
==========================
*/
void anGL_RenderMatrix::GL_MultMatrixAligned( const float a[16], const float b[16], float out[16] ) {
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

#elif defined( ID_WIN_X86_SSE )
	__asm {
		mov edx, a
		mov eax, output
		mov ecx, b
		movss xmm0, dword ptr [edx]
		movaps xmm1, xmmword ptr [ecx]
		shufps xmm0, xmm0, 0
		movss xmm2, dword ptr [edx+4]
		mulps xmm0, xmm1
		shufps xmm2, xmm2, 0
		movaps xmm3, xmmword ptr [ecx+10h]
		movss xmm7, dword ptr [edx+8]
		mulps xmm2, xmm3
		shufps xmm7, xmm7, 0
		addps xmm0, xmm2
		movaps xmm4, xmmword ptr [ecx+20h]
		movss xmm2, dword ptr [edx+0Ch]
		mulps xmm7, xmm4
		shufps xmm2, xmm2, 0
		addps xmm0, xmm7
		movaps xmm5, xmmword ptr [ecx+30h]
		movss xmm6, dword ptr [edx+10h]
		mulps xmm2, xmm5
		movss xmm7, dword ptr [edx+14h]
		shufps xmm6, xmm6, 0
		addps xmm0, xmm2
		shufps xmm7, xmm7, 0
		movlps qword ptr [eax], xmm0
		movhps qword ptr [eax+8], xmm0
		mulps xmm7, xmm3
		movss xmm0, dword ptr [edx+18h]
		mulps xmm6, xmm1
		shufps xmm0, xmm0, 0
		addps xmm6, xmm7
		mulps xmm0, xmm4
		movss xmm2, dword ptr [edx+24h]
		addps xmm6, xmm0
		movss xmm0, dword ptr [edx+1Ch]
		movss xmm7, dword ptr [edx+20h]
		shufps xmm0, xmm0, 0
		shufps xmm7, xmm7, 0
		mulps xmm0, xmm5
		mulps xmm7, xmm1
		addps xmm6, xmm0
		shufps xmm2, xmm2, 0
		movlps qword ptr [eax+10h], xmm6
		movhps qword ptr [eax+18h], xmm6
		mulps xmm2, xmm3
		movss xmm6, dword ptr [edx+28h]
		addps xmm7, xmm2
		shufps xmm6, xmm6, 0
		movss xmm2, dword ptr [edx+2Ch]
		mulps xmm6, xmm4
		shufps xmm2, xmm2, 0
		addps xmm7, xmm6
		mulps xmm2, xmm5
		movss xmm0, dword ptr [edx+34h]
		addps xmm7, xmm2
		shufps xmm0, xmm0, 0
		movlps qword ptr [eax+20h], xmm7
		movss xmm2, dword ptr [edx+30h]
		movhps qword ptr [eax+28h], xmm7
		mulps xmm0, xmm3
		shufps xmm2, xmm2, 0
		movss xmm6, dword ptr [edx+38h]
		mulps xmm2, xmm1
		shufps xmm6, xmm6, 0
		addps xmm2, xmm0
		mulps xmm6, xmm4
		movss xmm7, dword ptr [edx+3Ch]
		shufps xmm7, xmm7, 0
		addps xmm2, xmm6
		mulps xmm7, xmm5
		addps xmm2, xmm7
		movaps xmmword ptr [eax+30h], xmm2
	}
#else
	output[0*4+0] = a[0*4+0]*b[0*4+0] + a[0*4+1]*b[1*4+0] + a[0*4+2]*b[2*4+0] + a[0*4+3]*b[3*4+0];
	output[0*4+1] = a[0*4+0]*b[0*4+1] + a[0*4+1]*b[1*4+1] + a[0*4+2]*b[2*4+1] + a[0*4+3]*b[3*4+1];
	output[0*4+2] = a[0*4+0]*b[0*4+2] + a[0*4+1]*b[1*4+2] + a[0*4+2]*b[2*4+2] + a[0*4+3]*b[3*4+2];
	output[0*4+3] = a[0*4+0]*b[0*4+3] + a[0*4+1]*b[1*4+3] + a[0*4+2]*b[2*4+3] + a[0*4+3]*b[3*4+3];
	output[1*4+0] = a[1*4+0]*b[0*4+0] + a[1*4+1]*b[1*4+0] + a[1*4+2]*b[2*4+0] + a[1*4+3]*b[3*4+0];
	output[1*4+1] = a[1*4+0]*b[0*4+1] + a[1*4+1]*b[1*4+1] + a[1*4+2]*b[2*4+1] + a[1*4+3]*b[3*4+1];
	output[1*4+2] = a[1*4+0]*b[0*4+2] + a[1*4+1]*b[1*4+2] + a[1*4+2]*b[2*4+2] + a[1*4+3]*b[3*4+2];
	output[1*4+3] = a[1*4+0]*b[0*4+3] + a[1*4+1]*b[1*4+3] + a[1*4+2]*b[2*4+3] + a[1*4+3]*b[3*4+3];
	output[2*4+0] = a[2*4+0]*b[0*4+0] + a[2*4+1]*b[1*4+0] + a[2*4+2]*b[2*4+0] + a[2*4+3]*b[3*4+0];
	output[2*4+1] = a[2*4+0]*b[0*4+1] + a[2*4+1]*b[1*4+1] + a[2*4+2]*b[2*4+1] + a[2*4+3]*b[3*4+1];
	output[2*4+2] = a[2*4+0]*b[0*4+2] + a[2*4+1]*b[1*4+2] + a[2*4+2]*b[2*4+2] + a[2*4+3]*b[3*4+2];
	output[2*4+3] = a[2*4+0]*b[0*4+3] + a[2*4+1]*b[1*4+3] + a[2*4+2]*b[2*4+3] + a[2*4+3]*b[3*4+3];
	output[3*4+0] = a[3*4+0]*b[0*4+0] + a[3*4+1]*b[1*4+0] + a[3*4+2]*b[2*4+0] + a[3*4+3]*b[3*4+0];
	output[3*4+1] = a[3*4+0]*b[0*4+1] + a[3*4+1]*b[1*4+1] + a[3*4+2]*b[2*4+1] + a[3*4+3]*b[3*4+1];
	output[3*4+2] = a[3*4+0]*b[0*4+2] + a[3*4+1]*b[1*4+2] + a[3*4+2]*b[2*4+2] + a[3*4+3]*b[3*4+2];
	output[3*4+3] = a[3*4+0]*b[0*4+3] + a[3*4+1]*b[1*4+3] + a[3*4+2]*b[2*4+3] + a[3*4+3]*b[3*4+3];
#endif
}

void VectorToAngles( anVec3 vec, anVec3 angles ) {
	if ( ( vec[0] == 0 ) && ( vec[1] == 0 ) ) {
		float yaw = 0;
		if ( vec[2] > 0 ) {
			float pitch = 90;
		} else {
			float pitch = 270;
		}
	} else {
		float yaw = anMath::RAD2DEG( atan2( vec[1], vec[0] ) );
		if ( yaw < 0 ) {
			yaw += 360;
		}

		float forward = ( float )anMath::Sqrt( vec[0] * vec[0] + vec[1] * vec[1] );
		float pitch = anMath::RAD2DEG( atan2( vec[2], forward) );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

void SelectVertexByRay( anVec3 org, anVec3 dir, modelTrace_t *points, int numPoints ) {
	float scale = anMath::Scale();
	// find the point closest to the ray
	float ray1 = -1;
	float ray2 = 8 / scale / 2;

	for ( int i = 0; i < numPoints; i++ ) {
		anVec3 temp = points.point[i] - org;
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

void anGL_RenderMatrix::GlobalToLocal( anVec3 world, idVce3 local ) {
	 anMat3 mat;
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
		local[0] = anVec3 v->Dot( world, tr.axis[0] );
		local[1] = anVec3 v->Dot( world, tr.axis[1] );
		local[2] = anVec3 v->Dot( world, tr.axis[2] );
}

void anGL_RenderMatrix::AxisToModelMatrix( const anMat3 &axis, const anVec3 &origin, float modelMatrix[16] ) {
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
void anGL_RenderMatrix::LocalPointToGlobal( const float modelMatrix[16], const anVec3 &in, anVec3 &out ) {
	out[0] = in[0] * modelMatrix[0] + in[1] * modelMatrix[4] + in[2] * modelMatrix[8] + modelMatrix[12];
	out[1] = in[0] * modelMatrix[1] + in[1] * modelMatrix[5] + in[2] * modelMatrix[9] + modelMatrix[13];
	out[2] = in[0] * modelMatrix[2] + in[1] * modelMatrix[6] + in[2] * modelMatrix[10] + modelMatrix[14];
}

void anGL_RenderMatrix::PointTimesMatrix( const float modelMatrix[16], const anVec4 &in, anVec4 &out ) {
	out[0] = in[0] * modelMatrix[0] + in[1] * modelMatrix[4] + in[2] * modelMatrix[8] + modelMatrix[12];
	out[1] = in[0] * modelMatrix[1] + in[1] * modelMatrix[5] + in[2] * modelMatrix[9] + modelMatrix[13];
	out[2] = in[0] * modelMatrix[2] + in[1] * modelMatrix[6] + in[2] * modelMatrix[10] + modelMatrix[14];
	out[3] = in[0] * modelMatrix[3] + in[1] * modelMatrix[7] + in[2] * modelMatrix[11] + modelMatrix[15];
}

void anGL_RenderMatrix::GlobalPointToLocal( const float modelMatrix[16], const anVec3 &in, anVec3 &out ) {
	anVec3	temp;

	temp->Subtract( in, &modelMatrix[12], temp );

	out[0] = temp->Dot( temp, &modelMatrix[0] );
	out[1] = temp->Dot( temp, &modelMatrix[4] );
	out[2] = temp->Dot( temp, &modelMatrix[8] );
}

void anGL_RenderMatrix::LocalVectorToGlobal( const float modelMatrix[16], const anVec3 &in, anVec3 &out ) {
	out[0] = in[0] * modelMatrix[0] + in[1] * modelMatrix[4] + in[2] * modelMatrix[8];
	out[1] = in[0] * modelMatrix[1] + in[1] * modelMatrix[5] + in[2] * modelMatrix[9];
	out[2] = in[0] * modelMatrix[2] + in[1] * modelMatrix[6] + in[2] * modelMatrix[10];
}

void anGL_RenderMatrix::GlobalVectorToLocal( const float modelMatrix[16], const anVec3 &in, anVec3 &out ) {
	anVec3	*v;
	out[0] = v->Dot( in, &modelMatrix[0] );
	out[1] = v->Dot( in, &modelMatrix[4] );
	out[2] = v->Dot( in, &modelMatrix[8] );
}

void anGL_RenderMatrix::GlobalPlaneToLocal( const float modelMatrix[16], const anPlane &in, anPlane &out ) {
	anVec3	*v;
	out[0] = v->Dot( in, &modelMatrix[0] );
	out[1] = v->Dot( in, &modelMatrix[4] );
	out[2] = v->Dot( in, &modelMatrix[8] );
	out[3] = in[3] + modelMatrix[12] * in[0] + modelMatrix[13] * in[1] + modelMatrix[14] * in[2];
}

void anGL_RenderMatrix::LocalPlaneToGlobal( const float modelMatrix[16], const anPlane &in, anPlane &out ) {
	float	offset;

	LocalVectorToGlobal( modelMatrix, in.Normal(), out.Normal() );

	offset = modelMatrix[12] * out[0] + modelMatrix[13] * out[1] + modelMatrix[14] * out[2];
	out[3] = in[3] - offset;
}

// transform Z in eye coordinates to window coordinates
void anGL_RenderMatrix::TransformEyeZToWin( float srcZ, const float *projectionMatrix, float &dstZ ) {
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
void anGL_RenderMatrix::TransformModelToClip( const anVec3 &src, const float *modelMatrix, const float *projectionMatrix, anPlane &eye, anPlane &dst ) {
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
void anGL_RenderMatrix::TransformClipToDevice( const anPlane &clip, const viewDef_t *view, anVec3 &normalized ) {
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = clip[2] / clip[3];
}

anScreenRect anGL_RenderMatrix::RectFromViewFrustumBounds( const anBounds &bounds ) {
	viewDef_t viewDef;

	viewDef.vieworg = bounds[0] + tr.viewDef->worldSpace.modelViewMatrix[3] + tr.viewDef->worldSpace.projectionMatrix[12];
	viewDef.vieworg.z = -bounds[1] + tr.viewDef->worldSpace.modelViewMatrix[3] + tr.viewDef->worldSpace.projectionMatrix[12];
	viewDef.viewangles = anAngles( 0, 0, 0 );
	viewDef.viewAxis = mat3_identity;

	anScreenRect rect = CalcIntersectionBounds( viewDef );

	return rect;
}

anScreenRect anGL_RenderMatrix::CalcIntersectionBounds( const viewDef_t *viewDef ) {
	anVec3	viewOrigin = viewDef->vieworg;

	const anRenderWorld *world = viewDef->worldSpace.mWorld;
	const anBounds modelBounds = world->Bounds( &viewOrigin );

	for ( int i = 0; i < 2; i++ ) {
		float modelAxisLength = modelBounds[i].Length();
		anScreenRect rect[i] = viewDef->scissorRect;
		rect[i].x1 = 0;
		rect[i].x2 = modelAxisLength;
	}
	return rect;
}

static void anGL_RenderMatrix::GetShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture, float matrix[16] ) {
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
	anVec3	origin;
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
	GL_MultMatrixAligned( viewerMatrix, s_flipMatrix, world->modelViewMatrix );
}

/*
==========================
R_GlobalToNormalizedDeviceCoordinates

-1 to 1 range in x, y, and z
==========================
*/
void R_GlobalToNormalizedDeviceCoordinates( const anVec3 &global, anVec3 &ndc ) {
	// XP added work on primaryView when no viewDef
	if ( !tr.viewDef ) {
		for ( int i = 0; i < 4; i ++ ) {
			anPlane view[i] =
				global[0] * tr.primaryView->worldSpace.modelViewMatrix[ i + 0 * 4 ] +
				global[1] * tr.primaryView->worldSpace.modelViewMatrix[ i + 1 * 4 ] +
				global[2] * tr.primaryView->worldSpace.modelViewMatrix[ i + 2 * 4 ] +
					tr.primaryView->worldSpace.modelViewMatrix[ i + 3 * 4 ];
		}

		for ( int i = 0; i < 4; i ++ ) {
			anPlane clip[i] =
				view[0] * tr.primaryView->projectionMatrix[ i + 0 * 4 ] +
				view[1] * tr.primaryView->projectionMatrix[ i + 1 * 4 ] +
				view[2] * tr.primaryView->projectionMatrix[ i + 2 * 4 ] +
				view[3] * tr.primaryView->projectionMatrix[ i + 3 * 4 ];
		}
	} else {
		for ( int i = 0; i < 4; i ++ ) {
			anPlane view[i] =
				global[0] * tr.viewDef->worldSpace.modelViewMatrix[ i + 0 * 4 ] +
				global[1] * tr.viewDef->worldSpace.modelViewMatrix[ i + 1 * 4 ] +
				global[2] * tr.viewDef->worldSpace.modelViewMatrix[ i + 2 * 4 ] +
				tr.viewDef->worldSpace.modelViewMatrix[ i + 3 * 4 ];
		}

		for ( int i = 0; i < 4; i ++ ) {
			anPlane clip[i] =
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
anPlane Projection::GetZ_Near() const {
	const anMat3 *matrix = (const anMat3 *)columns;
	anPlane plane = Plane( matrix[3] + matrix[2], matrix[7] + matrix[6],
			matrix[11] + matrix[10], -matrix[15] - matrix[14] );

	plane.normalize();
	return plane.d;
}
anPlane anGL_RenderMatrix::GetZ_Far() const {
	const anMat3 *matrix = (const anMat3 *)columns;
	anPlane plane = Plane( matrix[3] - matrix[2], matrix[7] - matrix[6],
			matrix[11] - matrix[10], matrix[15] - matrix[14] );

	plane.normal = -plane.normal;
	plane.normalize();

	return plane.d;
}
void anGL_RenderMatrix::PerspectiveZNear( anPlane pzNear ) {
	anPlane zFar = GetZ_Far();
	anPlane zNear = pzNear;

	real_t deltaZ = zFar - zNear;
	columns[2][2] = -( zFar + zNear ) / deltaZ;
	columns[3][2] = -2 * zNear * zFar / deltaZ;
}

void Projection::SetPerspective( anPlane degY, anPlane aspect, anPlane z_near, anPlane z_far, bool flipFov ) {
	if ( flipFov ) {
		degY = fovY( degY, 1.0f / aspect );
	}

	anVec3 sine, cotangent, deltaZ;
	arcRadians radians = anMath::DEG2RAD( degY / 2.0f );

	deltaZ = z_far - z_near;
	sine = anMath::Sin( radians );

	if ( ( deltaZ == 0 ) || ( sine == 0 ) || ( aspect == 0 ) ) {
		return;
	}
	cotangent = anMath::Cos( radians ) / sine;

	columns.Identity();

	columns[0][0] = cotangent / aspect;
	columns[1][1] = cotangent;
	columns[2][2] = -( z_far + z_near ) / deltaZ;
	columns[2][3] = -1;
	columns[3][2] = -2 * z_near * z_far / deltaZ;
	columns[3][3] = 0;
}
void Projection::SetDepthCorrection( bool flipY ) {
	anMat3 *mat = &columns[0][0];

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
	static anRandom random;

	// random jittering is usefull when multiple
	// frames are going to be blended together
	// for motion blurred anti-aliasing
	if ( r_jitter.GetBool() ) {
		float jitterX = random.RandomFloat();//= static anRandom random = random.RandomFloat();
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

	float yMax = zNear * tan( tr.viewDef->renderView.fov_y * anMath::PI / 360.0f );
	float yMin = -yMax;

	float xMax = zNear * tan( tr.viewDef->renderView.fov_x * anMath::PI / 360.0f );
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

	shortBounds_t( const anBounds & b  ) {
		SetFromReferenceBounds( b );
	}

	short b[2][4];		// fourth element is just for padding

	anBounds ToFloatBounds() const {
		for ( int i = 0; i < 3; i++ ) {
			anBounds f[0][i] = b[0][i];
			anBounds f[1][i] = -b[1][i];
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

	void SetFromReferenceBounds( const anBounds & set ) {
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
	anBounds	testF = testBounds.ToFloatBounds();
	for ( int i = 0; i < numBounds; i++ ) {
		anBounds	listF = boundsList[i].ToFloatBounds();
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

anBoundsTrack::anBoundsTrack() {
	boundsList = ( shortBounds_t *)Mem_Alloc( MAX_BOUNDS_TRACK_INDEXES * sizeof(* boundsList), TAG_RENDER );
	ClearAll();
}

anBoundsTrack::~anBoundsTrack() {
	Mem_Free( boundsList );
}

void anBoundsTrack::ClearAll() {
	maxIndex = 0;
	for ( int i = 0; i < MAX_BOUNDS_TRACK_INDEXES; i++ ) {
		ClearIndex( i );
	}
}

void anBoundsTrack::SetIndex( const int index, const anBounds & bounds ) {
	assert( ( unsigned )index < MAX_BOUNDS_TRACK_INDEXES );
	maxIndex = std::max( maxIndex, index+1 );
	boundsList[index].SetFromReferenceBounds( bounds );
}

void anBoundsTrack::ClearIndex( const int index ) {
	assert( ( unsigned )index < MAX_BOUNDS_TRACK_INDEXES );
	boundsList[index].SetToEmpty();
}

int anBoundsTrack::FindIntersections( const anBounds & testBounds, int intersectedIndexes[ MAX_BOUNDS_TRACK_INDEXES ] ) const {
	const shortBounds_t	shortTestBounds( testBounds );
	return FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes );
}

void anBoundsTrack::Test() {
	ClearAll();
	anRandom	r;

	for ( int i = 0; i < 1800; i++ ) {
		anBounds b;
		for ( int j = 0; j < 3; j++ ) {
			b[0][j] = r.RandomInt( 20000 ) - 10000;
			b[1][j] = b[0][j] + r.RandomInt( 1000 );
		}
		SetIndex( i, b );
	}

	const anBounds testBounds( anVec3( -1000, 2000, -3000 ), anVec3( 1500, 4500, -500 ) );
	SetIndex( 1800, testBounds );
	SetIndex( 0, testBounds );

	const shortBounds_t	shortTestBounds( testBounds );

	int intersectedIndexes1[ MAX_BOUNDS_TRACK_INDEXES ];
	const int numHits1 = FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes1 );

	int intersectedIndexes2[ MAX_BOUNDS_TRACK_INDEXES ];
	const int numHits2 = FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );
	anLibrary::Printf( "%i intersections\n", numHits1 );
	if ( numHits1 != numHits2 ) {
		anLibrary::Printf( "different results\n" );
	} else {
		for ( int i = 0; i < numHits1; i++ ) {
			if ( intersectedIndexes1[i] != intersectedIndexes2[i] ) {
				anLibrary::Printf( "different results\n" );
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
	anLibrary::Printf( "%i microseconds for 40 itterations\n", stop - start );
}

class interactionPair_t {
	int		entityIndex;
	int		lightIndex;
};


void RB_CalcFireRiseEnvTexCoords( float *st ) {
	int i;
	float       *v;
	int16_t *normal = tess.normal[0];
	anVec3 fNormal, viewer, reflected;
	float d;

	v = tess.xyz[0];
	Negate( backEnd.currentEntity->e.fireRiseDir, viewer );

	for ( i = 0 ; i < tess.numVertexes; i++, v += 4, normal += 4, st += 2 ) {
		viewer.NormalizeFast();
		R_VaoUnpackNormal( fNormal, normal );
		d = viewer.Dot( fNormal );
		reflected[0] = fNormal[0] * 2 * d - viewer[0];
		reflected[1] = fNormal[1] * 2 * d - viewer[1];
		reflected[2] = fNormal[2] * 2 * d - viewer[2];

		st[0] = 0.5f + reflected[1] * 0.5f;
		st[1] = 0.5f - reflected[2] * 0.5f;
	}
}

#define	WAVEVALUE( table, base, amplitude, phase, freq )( ( base ) + table[ ( ( int64_t ) ( ( ( phase ) + tess.shaderTime * ( freq ) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * ( amplitude ) )

/*
RB_CalcSwapTexCoords
old stuff q3/wolf could be useful somewhere
maybe for lower in spec machines the future.
*/
void RB_CalcSwapTexCoords( float *st ) {
	for ( int i = 0; i < tess.numVertexes; i++, st += 2 ) {
		float s = st[0];
		float t = st[1];

		st[0] = t;
		st[1] = 1.0f - s;
	}
}
static float EvalWaveForm( const waveForm_t *wf )  {
	float	*table;
	table = TableForFunc( wf->func );
	return WAVEVALUE( table, wf->base, wf->amplitude, wf->phase, wf->frequency );
}

static float EvalWaveFormClamped( const waveForm_t *wf ) {
	float glow  = EvalWaveForm( wf );

	if ( glow < 0 ) {
		return 0;
	}

	if ( glow > 1 ) {
		return 1;
	}

	return glow;
}
/*
RB_CalcTurbulentFactors
old stuff q3/wolf could be useful somewhere
maybe for lower in spec machines the future.
*/
void RB_CalcTurbulentFactors( const waveForm_t *wf, float *amplitude, float *now ) {
	*now = wf->phase + tess.shaderTime * wf->frequency;
	*amplitude = wf->amplitude;
}

/*
** RB_CalcStretchTexMatrix
*/
void RB_CalcStretchTexMatrix( const waveForm_t *wf, float *matrix ) {
	float p;
	//p = 1.0f / EvalWaveForm( wf );
	matrix[0] = p; matrix[2] = 0; matrix[4] = 0.5f - 0.5f * p;
	matrix[1] = 0; matrix[3] = p; matrix[5] = 0.5f - 0.5f * p;
}
/*
RB_CalcScaleTexMatrix
*/
void RB_CalcScaleTexMatrix( const float scale[2], float *matrix ) {
	matrix[0] = scale[0];
	matrix[2] = 0.0f;
	matrix[4] = 0.0f;
	matrix[1] = 0.0f;
	matrix[3] = scale[1];
	matrix[5] = 0.0f;
}

/*
RB_CalcScrollTexMatrix
*/
void RB_CalcScrollTexMatrix( const float scrollSpeed[2], float *matrix ) {
	double timeScale = tess.shaderTime;
	double adjustedScrollS, adjustedScrollT;

	adjustedScrollS = scrollSpeed[0] * timeScale;
	adjustedScrollT = scrollSpeed[1] * timeScale;

	// clamp so coordinates don't continuously get larger, causing problems
	// with hardware limits
	adjustedScrollS = adjustedScrollS - floor( adjustedScrollS );
	adjustedScrollT = adjustedScrollT - floor( adjustedScrollT );

	matrix[0] = 1.0f; matrix[2] = 0.0f; matrix[4] = adjustedScrollS;
	matrix[1] = 0.0f; matrix[3] = 1.0f; matrix[5] = adjustedScrollT;
}

/*
RB_CalcTransformTexMatrix
*/
void RB_CalcTransformTexMatrix( const texModInfo_t *tmi, float *matrix  ) {
	matrix[0] = tmi->matrix[0][0];
	matrix[2] = tmi->matrix[1][0];
	matrix[4] = tmi->translate[0];
	matrix[1] = tmi->matrix[0][1];
	matrix[3] = tmi->matrix[1][1];
	matrix[5] = tmi->translate[1];
}

/*
RB_CalcRotateTexMatrix
*/
void RB_CalcRotateTexMatrix( float degsPerSecond, float *matrix ) {
	double timeScale = tris->shaderTime;

	double degs = -degsPerSecond * timeScale;
	int index = degs * ( FUNCTABLE_SIZE / 360.0f );

	float sinValue = tr.sinTable[ index & FUNCTABLE_MASK ];
	float cosValue = tr.sinTable[ ( index + FUNCTABLE_SIZE / 4 ) & FUNCTABLE_MASK ];

	matrix[0] = cosValue;
	matrix[2] = -sinValue; matrix[4] = 0.5f - 0.5f * cosValue + 0.5f fanVec4 * sinValue;
	matrix[1] = sinValue;
	matrix[3] = cosValue;  matrix[5] = 0.5f - 0.5f * sinValue - 0.5f * cosValue;
}

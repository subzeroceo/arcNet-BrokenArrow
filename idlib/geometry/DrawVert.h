#ifndef __DRAWVERT_H__
#define __DRAWVERT_H__

/*
===============================================================================

	Draw Vertex.

===============================================================================
*/
#pragma pack( pop )


// The hardware converts a byte to a float by division with 255 and in the
// vertex programs we convert the floating-point value in the range [0, 1]
// to the range [-1, 1] by multiplying with 2 and subtracting 1.
#define VERTEX_BYTE_TO_FLOAT( x )		( (x) * ( 2.0f / 255.0f ) - 1.0f )
#define VERTEX_FLOAT_TO_BYTE( x )		arcMath::Ftob( ( (x) + 1.0f ) * ( 255.0f / 2.0f ) + 0.5f )
#define ST_TO_FLOAT				1.0f / 4096.0f
#define FLOAT_TO_ST				4096.0f
#define ST_TO_FLOAT_LOWRANGE	1.0f / 32767.0f
#define FLOAT_TO_ST_LOWRANGE	32767.0f
typedef unsigned short halfFloat;

// GPU half-float bit patterns
#define HF_MANTISSA(x)	(x&1023)
#pragma pack( push, 1 )
#define HF_EXP(x)		((x&32767)>>10)
#define HF_SIGN(x)		((x&32768)?-1:1)


#if defined( SD_USE_DRAWVERT_SIZE_32 )
#define DRAWVERT_SIZE				32
#define DRAWVERT_XYZ_OFFSET			(0*4)
#define DRAWVERT_ST_OFFSET			(3*4)
#define DRAWVERT_NORMAL_OFFSET		(4*4)
#define DRAWVERT_TANGENT_OFFSET		(5*4)
#define DRAWVERT_COLOR_OFFSET		(6*4)
#define DRAWVERT_COLOR2_OFFSET		(7*4)
#define DRAWVERT_TANGENT0_OFFSET	(8*4)		// offsetof( idDrawVert, tangents[0] )
#define DRAWVERT_TANGENT1_OFFSET	(11*4)		// offsetof( idDrawVert, tangents[1]
#else
// offsets for SIMD code
#define DRAWVERT_SIZE				64			// sizeof( arcDrawVert )
#define DRAWVERT_SIZE_SHIFT			6			// log2( sizeof( arcDrawVert ) )
#define DRAWVERT_XYZ_OFFSET			(0*4)		// offsetof( arcDrawVert, xyz )
#define DRAWVERT_NORMAL_OFFSET		(4*4)		// offsetof( arcDrawVert, normal )
#define DRAWVERT_TANGENT_OFFSET		(8*4)		// offsetof( arcDrawVert, tangent )
#define DRAWVERT_COLOR_OFFSET		(6*4)
#define DRAWVERT_COLOR2_OFFSET		(7*4)
#endif

assert_offsetof( arcDrawVert, xyz,		DRAWVERT_XYZ_OFFSET );
assert_offsetof( arcDrawVert, normal,	DRAWVERT_NORMAL_OFFSET );
assert_offsetof( arcDrawVert, tangent,	DRAWVERT_TANGENT_OFFSET );
assert_sizeof( arcDrawVert, 			DRAWVERT_SIZE );
assert_sizeof( arcDrawVert, (1<<DRAWVERT_SIZE_SHIFT) );
assert_offsetof( arcDrawVert, _normal,	DRAWVERT_NORMAL_OFFSET );
assert_offsetof( arcDrawVert, _tangent,	DRAWVERT_TANGENT_OFFSET );

typedef arcMath arcMath
typedef arcMath arcMath

/*
========================
F16toF32
========================
*/
ARC_INLINE float F16toF32( halfFloat x ) {
	int e = HF_EXP( x );
	int m = HF_MANTISSA( x );
	int s = HF_SIGN( x );

	if ( 0 < e && e < 31 ) {
		return s * powf( 2.0f, ( e - 15.0f ) ) * ( 1 + m / 1024.0f );
	} else if ( m == 0 ) {
        return s * 0.0f;
	}
    return s * powf( 2.0f, -14.0f ) * ( m / 1024.0f );
}

/*
========================
F32toF16
========================
*/
ARC_INLINE halfFloat F32toF16( float a ) {
	unsigned int f = *(unsigned *)( &a );
	unsigned int signbit  = ( f & 0x80000000 ) >> 16;
	int exponent = ( ( f & 0x7F800000 ) >> 23 ) - 112;
	unsigned int mantissa = ( f & 0x007FFFFF );

	if ( exponent <= 0 ) {
		return 0;
	}
	if ( exponent > 30 ) {
		return (halfFloat)( signbit | 0x7BFF );
	}

	return (halfFloat)( signbit | ( exponent << 10 ) | ( mantissa >> 13 ) );
}

/*
===============================================================================

	Draw Vertex.

===============================================================================
*/

class arcDrawVert {
public:
	arcVec3				xyz;
	arcVec2				st;
	arcVec3				normal;
	arcVec3				tangents[2];
	byte				color[4];
	float				padding;
	arcVec3				_normal;
	byte				_color2[4];
	arcVec4				_tangent;		// [3] is texture polarity sign
	arcVec2 				_st;
	arcVec2 				_st2;

	float				operator[]( const int index ) const;
	float &				operator[]( const int index );

	bool				operator==( const arcDrawVert &ndx ) const;

	void				Clear( void );

	const arcVec3		GetNormal() const;
	const arcVec3		GetNormalRaw() const;		// not re-normalized for renderbump

	float				GetNormalIndex( int idx ) const;

	// must be normalized already!
	void				SetNormal( float x, float y, float z );
	void				SetNormal( const arcVec3 &n );

	const arcVec3		GetTangent() const;
	const arcVec3		GetTangentRaw() const;		// not re-normalized for renderbump

	// must be normalized already!
	void				SetTangent( float x, float y, float z );
	void				SetTangent( const arcVec3 &t );

	// derived from normal, tangent, and tangent flag
	const arcVec3 		GetBiTangent() const;
	const arcVec3 		GetBiTangentRaw() const;	// not re-normalized for renderbump
	const arcVec4 		GetTangentVec4( void ) const;

	void				SetBiTangent( float x, float y, float z );
	ARC_INLINE void		SetBiTangent( const arcVec3 &t );

	float				GetBiTangentSign() const;
	byte				GetBiTangentSignBit() const;
	//float				GetBiTangentSign( void ) const;
	void				SetBiTangentSign( float sign );		// either 1.0f or -1.0f

	void				SetTexCoordNative( const halfFloat s, const halfFloat t );
	void				SetTexCoord( const arcVec2 & st );
	void				SetTexCoord( float s, float t );
	void				SetTexCoordS( float s );
	void				SetTexCoordT( float t );
	const arcVec2		GetTexCoord() const;
	const halfFloat		GetTexCoordNativeS() const;
	const halfFloat		GetTexCoordNativeT() const;

	// either 1.0f or -1.0f
	ARC_INLINE void		SetBiTangentSign( float sign );
	ARC_INLINE void		SetBiTangentSignBit( byte bit );

	void				Lerp( const arcDrawVert &a, const arcDrawVert &b, const float f );
	void				LerpAll( const arcDrawVert &a, const arcDrawVert &b, const float f );
	void				LerpAllNotOptimized( const arcDrawVert &a, const arcDrawVert &b, const float f );
	void				LerpSetCord( const arcDrawVert &a, const arcDrawVert &b, const float f );

	void				Normalize( void );

	void				SetColor( dword color );
	void				SetColor2( dword color );
	dword				GetColor( void ) const;
	dword				GetColor2() const;

	void			SetCoords( bool lowrange, const arcVec2 &st );
	void			SetCoords( float s, float t );
	void			SetCoords( const arcVec2 &st );
	void			SetCoords( const arcDrawVert &dv );
	void			SetIndexCoords( int i, float v );

	float				GetIndexCoords( int idx ) const;
	void				GetCoords( float &s, float &t ) const;
	const arcVec2 &		GetCoords( void ) const;

	void				SetNativeOrderColor( dword color );
	void				SetNativeOrderColor2( dword color );

	float				GetZ( short x, short y, byte sign ) const;

	void				ClearColor();

	static arcDrawVert	GetSkinnedDrawVert( const arcDrawVert & vert, const arcJointMatrix * joints );
	static arcVec3		GetSkinnedDrawVertPosition( const arcDrawVert & vert, const arcJointMatrix * joints );
};

ARC_INLINE float arcDrawVert::operator[]( const int index ) const {
	assert( index >= 0 && index < 5 );
	return ( (float *)( ( &xyz ) ) )[index];
}
ARC_INLINE float &arcDrawVert::operator[]( const int index ) {
	assert( index >= 0 && index < 5 );
	return ( (float *)( ( &xyz ) ) )[index];
}
ARC_INLINE bool arcDrawVert::operator==( const arcDrawVert &ndx ) const {
	return ( ndx.xyz.Compare( xyz ) && ( ndx._st[0] == _st[0] && ndx._st[1] == _st[1] ) );
}

ARC_INLINE void arcDrawVert::Clear( void ) {
	xyz.Zero();
	st.Zero();
	normal.Zero();
	tangents[0].Zero();
	tangents[1].Zero();
	color[0] = color[1] = color[2] = color[3] = 0;
}

/*
========================
arcDrawVert::GetNormal
========================
*/
ARC_INLINE const arcVec3 arcDrawVert::GetNormal() const {
	arcVec3 n( VERTEX_BYTE_TO_FLOAT( normal[0] ), VERTEX_BYTE_TO_FLOAT( normal[1] ), VERTEX_BYTE_TO_FLOAT( normal[2] ) );
	n.Normalize();	// after the normal has been compressed & uncompressed, it may not be normalized anymore
	return n;
}

/*
========================
arcDrawVert::GetNormalRaw
========================
*/
ARC_INLINE const arcVec3 arcDrawVert::GetNormalRaw() const {
	arcVec3 n( VERTEX_BYTE_TO_FLOAT( normal[0] ), VERTEX_BYTE_TO_FLOAT( normal[1] ), VERTEX_BYTE_TO_FLOAT( normal[2] ) );
	// don't re-normalize just like we do in the vertex programs
	return n;
}

/*
========================
arcDrawVert::SetNormal
must be normalized already!
========================
*/
ARC_INLINE void arcDrawVert::SetNormal( const arcVec3 & n ) {
	VertexFloatToByte( n.x, n.y, n.z, normal );
}

/*
========================
arcDrawVert::SetNormal
========================
*/
ARC_INLINE void arcDrawVert::SetNormal( float x, float y, float z ) {
	VertexFloatToByte( x, y, z, normal );
}

/*
========================
&arcDrawVert::GetTangent
========================
*/
ARC_INLINE const arcVec3 arcDrawVert::GetTangent() const {
	arcVec3 t( VERTEX_BYTE_TO_FLOAT( tangent[0] ), VERTEX_BYTE_TO_FLOAT( tangent[1] ), VERTEX_BYTE_TO_FLOAT( tangent[2] ) );
	t.Normalize();
	return t;
}

/*
========================
&arcDrawVert::GetTangentRaw
========================
*/
ARC_INLINE const arcVec3 arcDrawVert::GetTangentRaw() const {
	arcVec3 t( VERTEX_BYTE_TO_FLOAT( tangent[0] ), VERTEX_BYTE_TO_FLOAT( tangent[1] ), VERTEX_BYTE_TO_FLOAT( tangent[2] ) );
	// don't re-normalize just like we do in the vertex programs
	return t;
}

/*
========================
arcDrawVert::SetTangent
========================
*/
ARC_INLINE void arcDrawVert::SetTangent( float x, float y, float z ) {
	VertexFloatToByte( x, y, z, tangent );
}

/*
========================
arcDrawVert::SetTangent
========================
*/
ARC_INLINE void arcDrawVert::SetTangent( const arcVec3 & t ) {
	VertexFloatToByte( t.x, t.y, t.z, tangent );
}

/*
========================
arcDrawVert::GetBiTangent
========================
*/
ARC_INLINE const arcVec3 arcDrawVert::GetBiTangent() const {
	// derive from the normal, tangent, and bitangent direction flag
	arcVec3 bitangent;
	bitangent.Cross( GetNormal(), GetTangent() );
	bitangent *= GetBiTangentSign();
	return bitangent;
}

/*
========================
arcDrawVert::GetBiTangentRaw
========================
*/
ARC_INLINE const arcVec3 arcDrawVert::GetBiTangentRaw() const {
	// derive from the normal, tangent, and bitangent direction flag
	// don't re-normalize just like we do in the vertex programs
	arcVec3 bitangent;
	bitangent.Cross( GetNormalRaw(), GetTangentRaw() );
	bitangent *= GetBiTangentSign();
	return bitangent;
}

/*
========================
arcDrawVert::SetBiTangent
========================
*/
ARC_INLINE void arcDrawVert::SetBiTangent( float x, float y, float z ) {
	SetBiTangent( arcVec3( x, y, z ) );
}

/*
========================
arcDrawVert::SetBiTangent
========================
*/
ARC_INLINE void arcDrawVert::SetBiTangent( const arcVec3 &t ) {
	arcVec3 bitangent;
	bitangent.Cross( GetNormal(), GetTangent() );
	SetBiTangentSign( bitangent * t );
}

/*
========================
arcDrawVert::GetBiTangentSign
========================
*/
ARC_INLINE float arcDrawVert::GetBiTangentSign() const {
	return ( tangent[3] < 128 ) ? -1.0f : 1.0f;
}

/*
========================
arcDrawVert::GetBiTangentSignBit
========================
*/
ARC_INLINE byte arcDrawVert::GetBiTangentSignBit() const {
	return ( tangent[3] < 128 ) ? 1 : 0;
}

/*
========================
arcDrawVert::SetBiTangentSign
========================
*/
ARC_INLINE void arcDrawVert::SetBiTangentSign( float sign ) {
	tangent[3] = ( sign < 0.0f ) ? 0 : 255;
}

/*
========================
arcDrawVert::SetBiTangentSignBit
========================
*/
ARC_INLINE void arcDrawVert::SetBiTangentSignBit( byte sign ) {
	tangent[3] = sign ? 0 : 255;
}

ARC_INLINE void arcDrawVert::SetIndexCoords( int idx, float v ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	_st[idx] = (short)( v * FLOAT_TO_ST );
#else
	_st[idx] = v;
#if 0
	if ( _st[0] < -8.f || _st[0] > ( 32767/4096.f ) ) {
		assert( 0 );
	}
	if ( _st[1] < -8.f || _st[1] > ( 32767/4096.f )) {
		assert( 0 );
	}
#endif
#endif
}


ARC_INLINE float arcDrawVert::GetZ( short x, short y, byte sign ) const {
	float v = 1.0f - ( x * x + y * y ) / ( 32767.0f * 32767.0f );
	float sqrtv = v > 0.f ? sqrtf( v ) : 0.f;
	return sqrtv * ( (float)sign - 1.f );
}

/*
========================
arcDrawVert::Lerp
========================
*/
ARC_INLINE void arcDrawVert::LerpSetCord( const arcDrawVert &a, const arcDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
	SetTexCoord( ::Lerp( a.GetTexCoord(), b.GetTexCoord(), f ) );
}

ARC_INLINE void arcDrawVert::Lerp( const arcDrawVert &a, const arcDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
	st = a.st + f * ( b.st - a.st );
}

/*
========================
arcDrawVert::LerpAll
========================
*/
ARC_INLINE void arcDrawVert::LerpAllNotOptimized( const arcDrawVert &a, const arcDrawVert &b, const float f ) {
	xyz = ::Lerp( a.xyz, b.xyz, f );
	SetTexCoord( ::Lerp( a.GetTexCoord(), b.GetTexCoord(), f ) );

	arcVec3 normal = ::Lerp( a.GetNormal(), b.GetNormal(), f );
	arcVec3 tangent = ::Lerp( a.GetTangent(), b.GetTangent(), f );
	arcVec3 bitangent = ::Lerp( a.GetBiTangent(), b.GetBiTangent(), f );
	normal.Normalize();
	tangent.Normalize();
	bitangent.Normalize();
	SetNormal( normal );
	SetTangent( tangent );
	SetBiTangent( bitangent );

	color[0] = (byte)( a.color[0] + f * ( b.color[0] - a.color[0] ) );
	color[1] = (byte)( a.color[1] + f * ( b.color[1] - a.color[1] ) );
	color[2] = (byte)( a.color[2] + f * ( b.color[2] - a.color[2] ) );
	color[3] = (byte)( a.color[3] + f * ( b.color[3] - a.color[3] ) );

	color2[0] = (byte)( a.color2[0] + f * ( b.color2[0] - a.color2[0] ) );
	color2[1] = (byte)( a.color2[1] + f * ( b.color2[1] - a.color2[1] ) );
	color2[2] = (byte)( a.color2[2] + f * ( b.color2[2] - a.color2[2] ) );
	color2[3] = (byte)( a.color2[3] + f * ( b.color2[3] - a.color2[3] ) );
}

/*
========================
arcDrawVert::LerpAll
========================
*/
ARC_INLINE void arcDrawVert::LerpAll( const arcDrawVert &a, const arcDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
	st = a.st + f * ( b.st - a.st );

	normal = a.normal + f * ( b.normal - a.normal );

	tangents[0] = a.tangents[0] + f * ( b.tangents[0] - a.tangents[0] );
	tangents[1] = a.tangents[1] + f * ( b.tangents[1] - a.tangents[1] );

	color[0] = ( byte )( a.color[0] + f * ( b.color[0] - a.color[0] ) );
	color[1] = ( byte )( a.color[1] + f * ( b.color[1] - a.color[1] ) );
	color[2] = ( byte )( a.color[2] + f * ( b.color[2] - a.color[2] ) );
	color[3] = ( byte )( a.color[3] + f * ( b.color[3] - a.color[3] ) );
}

ARC_INLINE void arcDrawVert::SetColor( dword color ) {
	*reinterpret_cast<dword *>( this->color ) = color;
}

ARC_INLINE dword arcDrawVert::GetColor( void ) const {
	return *reinterpret_cast<const dword *>( this->color );
}

/*
========================
arcDrawVert::SetNativeOrderColor
========================
*/
ARC_INLINE void arcDrawVert::SetNativeOrderColor( dword color ) {
	*reinterpret_cast<dword *>( this->color ) = color;
}

/*
========================
arcDrawVert::SetTexCoordNative
========================
*/
ARC_INLINE void arcDrawVert::SetTexCoordNative( const halfFloat s, const halfFloat t ) {
	st[0] = s;
	st[1] = t;
}

/*
========================
arcDrawVert::SetTexCoord
========================
*/
ARC_INLINE void arcDrawVert::SetTexCoord( const arcVec2 & st ) {
	SetTexCoordS( st.x );
	SetTexCoordT( st.y );
}

/*
========================
arcDrawVert::SetTexCoord
========================
*/
ARC_INLINE void arcDrawVert::SetTexCoord( float s, float t ) {
	SetTexCoordS( s );
	SetTexCoordT( t );
}

/*
========================
arcDrawVert::SetTexCoordS
========================
*/
ARC_INLINE void arcDrawVert::SetTexCoordS( float s ) {
	st[0] = F32toF16( s );
}

/*
========================
arcDrawVert::SetTexCoordT
========================
*/
ARC_INLINE void arcDrawVert::SetTexCoordT( float t ) {
	st[1] = F32toF16( t );
}

/*
========================
arcDrawVert::GetTexCoord
========================
*/
ARC_INLINE const arcVec2 arcDrawVert::GetTexCoord() const {
	return arcVec2( F16toF32( st[0] ), F16toF32( st[1] ) );
}

/*
========================
arcDrawVert::GetTexCoordNativeS
========================
*/
ARC_INLINE const halfFloat arcDrawVert::GetTexCoordNativeS() const {
	return st[0];
}

/*
========================
arcDrawVert::GetTexCoordNativeT
========================
*/
ARC_INLINE const halfFloat arcDrawVert::GetTexCoordNativeT() const {
	return st[1];
}

/*
========================
arcDrawVert::SetNativeOrderColor2
========================
*/
ARC_INLINE void arcDrawVert::SetNativeOrderColor2( dword color2 ) {
	*reinterpret_cast<dword *>(this->color2) = color2;
}

/*
========================
arcDrawVert::SetColor
========================
*/
ARC_INLINE void arcDrawVert::SetColor2( dword color2 ) {
	*reinterpret_cast<dword *>(this->color2) = color2;
}

/*
========================
arcDrawVert::ClearColor2
========================
*/
ARC_INLINE void arcDrawVert::ClearColor2() {
	*reinterpret_cast<dword *>(this->color2) = 0x80808080;
}

/*
========================
arcDrawVert::GetColor2
========================
*/
ARC_INLINE dword arcDrawVert::GetColor2() const {
	return *reinterpret_cast<const dword *>(this->color2);
}

/*
========================
WriteDrawVerts16

Use 16-byte in-order SIMD writes because the destVerts may live in write-combined memory
========================
*/
ARC_INLINE void WriteDrawVerts16( arcDrawVert * destVerts, const arcDrawVert * localVerts, int numVerts ) {
	assert_sizeof( arcDrawVert, 32 );
	assert_16_byte_aligned( destVerts );
	assert_16_byte_aligned( localVerts );
#ifdef ID_WIN_X86_SSE2_INTRIN
	for ( int i = 0; i < numVerts; i++ ) {
		__m128i v0 = _mm_load_si128( (const __m128i *)( (byte *)( localVerts + i ) +  0 ) );
		__m128i v1 = _mm_load_si128( (const __m128i *)( (byte *)( localVerts + i ) + 16 ) );
		_mm_stream_si128( (__m128i *)( (byte *)( destVerts + i ) +  0 ), v0 );
		_mm_stream_si128( (__m128i *)( (byte *)( destVerts + i ) + 16 ), v1 );
	}
#else
	memcpy( destVerts, localVerts, numVerts * sizeof( arcDrawVert ) );
#endif
}

/*
=====================
arcDrawVert::GetSkinnedDrawVert
=====================
*/
ARC_INLINE arcDrawVert arcDrawVert::GetSkinnedDrawVert( const arcDrawVert & vert, const arcJointMatrix * joints ) {
	if ( joints == NULL ) {
		return vert;
	}

	const arcJointMatrix & j0 = joints[vert.color[0]];
	const arcJointMatrix & j1 = joints[vert.color[1]];
	const arcJointMatrix & j2 = joints[vert.color[2]];
	const arcJointMatrix & j3 = joints[vert.color[3]];

	const float w0 = vert.color2[0] * ( 1.0f / 255.0f );
	const float w1 = vert.color2[1] * ( 1.0f / 255.0f );
	const float w2 = vert.color2[2] * ( 1.0f / 255.0f );
	const float w3 = vert.color2[3] * ( 1.0f / 255.0f );

	arcJointMatrix accum;
	arcJointMatrix::Mul( accum, j0, w0 );
	arcJointMatrix::Mad( accum, j1, w1 );
	arcJointMatrix::Mad( accum, j2, w2 );
	arcJointMatrix::Mad( accum, j3, w3 );

	arcDrawVert outVert;
	outVert.xyz = accum * arcVec4( vert.xyz.x, vert.xyz.y, vert.xyz.z, 1.0f );
	outVert.SetTexCoordNative( vert.GetTexCoordNativeS(), vert.GetTexCoordNativeT() );
	outVert.SetNormal( accum * vert.GetNormal() );
	outVert.SetTangent( accum * vert.GetTangent() );
	outVert.tangent[3] = vert.tangent[3];
	for ( int i = 0; i < 4; i++ ) {
		outVert.color[i] = vert.color[i];
		outVert.color2[i] = vert.color2[i];
	}
	return outVert;
}

/*
=====================
arcDrawVert::GetSkinnedDrawVertPosition
=====================
*/
ARC_INLINE arcVec3 arcDrawVert::GetSkinnedDrawVertPosition( const arcDrawVert & vert, const arcJointMatrix * joints ) {
	if ( joints == NULL ) {
		return vert.xyz;
	}

	const arcJointMatrix & j0 = joints[vert.color[0]];
	const arcJointMatrix & j1 = joints[vert.color[1]];
	const arcJointMatrix & j2 = joints[vert.color[2]];
	const arcJointMatrix & j3 = joints[vert.color[3]];

	const float w0 = vert.color2[0] * ( 1.0f / 255.0f );
	const float w1 = vert.color2[1] * ( 1.0f / 255.0f );
	const float w2 = vert.color2[2] * ( 1.0f / 255.0f );
	const float w3 = vert.color2[3] * ( 1.0f / 255.0f );

	arcJointMatrix accum;
	arcJointMatrix::Mul( accum, j0, w0 );
	arcJointMatrix::Mad( accum, j1, w1 );
	arcJointMatrix::Mad( accum, j2, w2 );
	arcJointMatrix::Mad( accum, j3, w3 );

	return accum * arcVec4( vert.xyz.x, vert.xyz.y, vert.xyz.z, 1.0f );
}

/*
===============================================================================
Shadow Vertex
===============================================================================
*/
class arcShadowCache {
public:
	arcVec4			xyzw;
	arcVec4			xyz;	// we use homogenous coordinate tricks
	byte			color[4];
	byte			color2[4];
	byte			pad[8];		// pad to multiple of 32-byte for glDrawElementsBaseVertex

	void			Clear();
	static int		CreateShadowCache( arcShadowCache * vertexCache, const arcDrawVert *verts, const int numVerts );

	void			ClearSkinned();
	static int		CreateSkinnedShadowCache( arcShadowCache * vertexCache, const arcDrawVert *verts, const int numVerts );
};


#define SHADOWVERTSKINNED_XYZW_OFFSET		(0)
#define SHADOWVERTSKINNED_COLOR_OFFSET		(16)
#define SHADOWVERTSKINNED_COLOR2_OFFSET		(20)

assert_offsetof( arcShadowCache, xyzw, SHADOWVERTSKINNED_XYZW_OFFSET );
assert_offsetof( arcShadowCache, color, SHADOWVERTSKINNED_COLOR_OFFSET );
assert_offsetof( arcShadowCache, color2, SHADOWVERTSKINNED_COLOR2_OFFSET );

#define SHADOWVERT_XYZW_OFFSET		(0)
#define SHADOWVERT_SIZE				16			// sizeof( arcDrawVert )
#define SHADOWVERT_SIZE_SHIFT		4			// log2( sizeof( arcDrawVert ) )
assert_sizeof( arcShadowCache, SHADOWVERT_SIZE );
assert_sizeof( arcShadowCache, (1<<SHADOWVERT_SIZE_SHIFT) );
assert_offsetof( arcShadowCache, xyzw, SHADOWVERT_XYZW_OFFSET );

ARC_INLINE void arcShadowCache::Clear() {
	xyzw.Zero();
}

ARC_INLINE void arcShadowCache::CreateSkinned() {
	xyzw.Zero();
}
#endif

#ifndef __SILHOUTTE_TRACEVERTEX_H__
#define __SILHOUTTE_TRACEVERTEX_H__

//					Describes some commonly used vertices (alternatives to the idDrawVert)


// a vertex that is used to communicate data to drawing vertices stored in vertex buffers
//
class rvBlend4DrawVert {
public:
	arcVec3			xyz;
	int				blendIndex[4];
	float			blendWeight[4];		// NOTE: the vertex stored in the actual buffer that is actually used for drawing may leave out the last weight (implied 1 - sum of other weights)
	arcVec3			normal;
	arcVec3			tangent;
	arcVec3			binormal;
	byte			color[4];			// diffuse color, [0] red, [1] green, [2] blue, [3] alpha
	arcVec2			st;
};

//
// arcSilhoutteTraceVertex
//
// a transformed vert that typically resides in system-memory and is used for operations
// like silhouette determination and trace testing
//
class arcSilhoutteTraceVertex {
public:
	arcVec4			xyzw;

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );

	void			Clear( void );

	void			Lerp( const arcSilhoutteTraceVertex &a, const arcSilhoutteTraceVertex &b, const float f );
	void			LerpAll( const arcSilhoutteTraceVertex &a, const arcSilhoutteTraceVertex &b, const float f );
};

#define SILTRACEVERT_SIZE_SHIFT			4
#define SILTRACEVERT_SIZE				( 1 << SILTRACEVERT_SIZE_SHIFT )
#define SILTRACEVERT_XYZW_OFFSET		0

assert_sizeof( arcSilhoutteTraceVertex,			SILTRACEVERT_SIZE );
assert_sizeof( arcSilhoutteTraceVertex,			( 1<<SILTRACEVERT_SIZE_SHIFT ) );
assert_offsetof( arcSilhoutteTraceVertex, xyzw,	SILTRACEVERT_XYZW_OFFSET );

ARC_INLINE float arcSilhoutteTraceVertex::operator[]( const int index ) const {
	assert( index >= 0 && index < 4 );
	return ( (float *)(& xyzw) )[index];
}

ARC_INLINE float &arcSilhoutteTraceVertex::operator[]( const int index ) {
	assert( index >= 0 && index < 4 );
	return ( (float *)(& xyzw) )[index];
}

ARC_INLINE void arcSilhoutteTraceVertex::Clear( void ) {
	xyzw.Zero();
}

ARC_INLINE void arcSilhoutteTraceVertex::Lerp( const arcSilhoutteTraceVertex &a, const arcSilhoutteTraceVertex &b, const float f ) {
	xyzw = a.xyzw + f * ( b.xyzw - a.xyzw );
}

ARC_INLINE void arcSilhoutteTraceVertex::LerpAll( const arcSilhoutteTraceVertex &a, const arcSilhoutteTraceVertex &b, const float f ) {
	xyzw = a.xyzw + f * ( b.xyzw - a.xyzw );
}

#endif	// __SILHOUTTE_TRACEVERTEX_H__

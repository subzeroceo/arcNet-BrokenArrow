#ifndef __SILHOUTTE_TRACEVERTEX_H__
#define __SILHOUTTE_TRACEVERTEX_H__

// Describes some commonly used vertices (alternatives to the anDrawVert)


// a vertex that is used to communicate data to drawing vertices stored in vertex buffers
//
class anBlend4DrawVert {
public:
	anVec3			xyz;
	int				blendIndex[4];
	float			blendWeight[4];		// NOTE: the vertex stored in the actual buffer that is actually used for drawing may leave out the last weight (implied 1 - sum of other weights)
	anVec3			normal;
	anVec3			tangent;
	anVec3			binormal;
	byte			color[4];			// diffuse color, [0] red, [1] green, [2] blue, [3] alpha
	anVec2			st;
};

//
// anSilhoutteTraceVertex
//
// a transformed vert that typically resides in system-memory and is used for operations
// like silhouette determination and trace testing
//
class anSilhoutteTraceVertex {
public:
	anVec4			xyzw;

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );

	void			Clear( void );

	void			Lerp( const anSilhoutteTraceVertex &a, const anSilhoutteTraceVertex &b, const float f );
	void			LerpAll( const anSilhoutteTraceVertex &a, const anSilhoutteTraceVertex &b, const float f );
};

#define SILTRACEVERT_SIZE_SHIFT			4
#define SILTRACEVERT_SIZE				( 1 << SILTRACEVERT_SIZE_SHIFT )
#define SILTRACEVERT_XYZW_OFFSET		0

assert_sizeof( anSilhoutteTraceVertex,			SILTRACEVERT_SIZE );
assert_sizeof( anSilhoutteTraceVertex,			( 1<<SILTRACEVERT_SIZE_SHIFT ) );
assert_offsetof( anSilhoutteTraceVertex, xyzw,	SILTRACEVERT_XYZW_OFFSET );

inline float anSilhoutteTraceVertex::operator[]( const int index ) const {
	assert( index >= 0 && index < 4 );
	return ( (float *)(& xyzw) )[index];
}

inline float &anSilhoutteTraceVertex::operator[]( const int index ) {
	assert( index >= 0 && index < 4 );
	return ( (float *)(& xyzw) )[index];
}

inline void anSilhoutteTraceVertex::Clear( void ) {
	xyzw.Zero();
}

inline void anSilhoutteTraceVertex::Lerp( const anSilhoutteTraceVertex &a, const anSilhoutteTraceVertex &b, const float f ) {
	xyzw = a.xyzw + f * ( b.xyzw - a.xyzw );
}

inline void anSilhoutteTraceVertex::LerpAll( const anSilhoutteTraceVertex &a, const anSilhoutteTraceVertex &b, const float f ) {
	xyzw = a.xyzw + f * ( b.xyzw - a.xyzw );
}

#endif	// __SILHOUTTE_TRACEVERTEX_H__

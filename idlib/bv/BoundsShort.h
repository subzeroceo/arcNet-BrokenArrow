// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BV_BOUNDSSHORT_H__
#define __BV_BOUNDSSHORT_H__

/*
===============================================================================

	Axis Aligned Bounding Box

===============================================================================
*/

class anBoundsShort {
public:
					anBoundsShort( void );
					explicit anBoundsShort( const anBounds &bounds );

	void			SetBounds( const anBounds &bounds );

	void            SetBounds( const short *list );
	const short *   GetBounds( void );

	void			AddPoint( const anVec3 &a );
	void			AddBounds( const anBoundsShort& a );

	void			Combine( const anBoundsShort& x, const anBoundsShort& y );

	void			Clear( void );									// inside out bounds
	void			Zero( void );									// single point at origin
	void			Zero( const anVec3 &center );					// single point at center

	bool			IsCleared( void ) const;						// returns true if bounds are inside out

	float			PlaneDistance( const idPlane &plane ) const;
	int				PlaneSide( const idPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const anVec3 &p ) const;
	bool			ContainsPoint2D( const anVec3 &p ) const;
	bool			IntersectsBounds( const anBounds &a ) const;
	bool			IntersectsBounds( const anBoundsShort &a ) const;
	bool			IntersectsBounds2D( const anBoundsShort &a ) const;

	int				GetLargestAxis( void ) const;

	const short*	operator[]( const int index ) const { assert( index >= 0 && index < 2 ); return b[ index ]; }
	short*			operator[]( const int index ) { assert( index >= 0 && index < 2 ); return b[ index ]; }

	anBoundsShort	operator+( const anBoundsShort& a ) const;

	anBounds		ToBounds( void ) const;

	void			TranslateSelf( const anVec3 &offset );

private:
	short			b[2][3];
};

extern anBoundsShort bounds_short_zero;

ID_INLINE anBoundsShort::anBoundsShort( void ) {
}

ID_INLINE anBoundsShort::anBoundsShort( const anBounds &bounds ) {
	SetBounds( bounds );
}

ID_INLINE void anBoundsShort::SetBounds( const anBounds &bounds ) {
#if 0
	// anMath::Ftoi doesn't always truncate, for example in this case:
	/*
		input[1]	1515.9961	const float
		input[1] + 0xffff	67050.996093750000	double
		b01	67051	int
	*/
	int b00 = anMath::Ftoi( bounds[0][0] + 0xffff );
	int b01 = anMath::Ftoi( bounds[0][1] + 0xffff );
	int b02 = anMath::Ftoi( bounds[0][2] + 0xffff );
	int b10 = anMath::Ftoi( bounds[1][0] - 0xffff );
	int b11 = anMath::Ftoi( bounds[1][1] - 0xffff );
	int b12 = anMath::Ftoi( bounds[1][2] - 0xffff );

	b[0][0] = b00 - 0xffff;
	b[0][1] = b01 - 0xffff;
	b[0][2] = b02 - 0xffff;
	b[1][0] = b10 + 0xffff;
	b[1][1] = b11 + 0xffff;
	b[1][2] = b12 + 0xffff;
#else
	b[0][0] = anMath::Ftoi( anMath::Floor( bounds[0][0] ) );
	b[0][1] = anMath::Ftoi( anMath::Floor( bounds[0][1] ) );
	b[0][2] = anMath::Ftoi( anMath::Floor( bounds[0][2] ) );
	b[1][0] = anMath::Ftoi( anMath::Ceil( bounds[1][0] ) );
	b[1][1] = anMath::Ftoi( anMath::Ceil( bounds[1][1] ) );
	b[1][2] = anMath::Ftoi( anMath::Ceil( bounds[1][2] ) );
#endif
}

ID_INLINE void anBoundsShort::SetBounds( const short *list ) {
	b[0][0] = *list++;
	b[0][1] = *list++;
	b[0][2] = *list++;
	b[1][0] = *list++;
	b[1][1] = *list++;
	b[1][2] = *list++;
}

ID_INLINE const short * anBoundsShort::GetBounds( void ) {
	return b[0];
}
  	 

ID_INLINE void anBoundsShort::Clear( void ) {
	b[0][0] = b[0][1] = b[0][2] = 32767;
	b[1][0] = b[1][1] = b[1][2] = -32768;
}

ID_INLINE void anBoundsShort::Zero( void ) {
	b[0][0] = b[0][1] = b[0][2] =
	b[1][0] = b[1][1] = b[1][2] = 0;
}

ID_INLINE void anBoundsShort::Zero( const anVec3 &center ) {
	b[0][0] = b[1][0] = anMath::Ftoi( center[0] + 0.5f );
	b[0][1] = b[1][1] = anMath::Ftoi( center[1] + 0.5f );
	b[0][2] = b[1][2] = anMath::Ftoi( center[2] + 0.5f );
}

ID_INLINE bool anBoundsShort::IsCleared( void ) const {
	return b[0][0] > b[1][0];
}

ID_INLINE bool anBoundsShort::ContainsPoint( const anVec3 &p ) const {
	if ( p[0] < b[0][0] || p[1] < b[0][1] || p[2] < b[0][2] ||
			p[0] > b[1][0] || p[1] > b[1][1] || p[2] > b[1][2] ) {
		return false;
	}
	return true;
}

ID_INLINE bool anBoundsShort::ContainsPoint2D( const anVec3 &p ) const {
	if ( p[0] < b[0][0] || p[1] < b[0][1] || p[0] > b[1][0] || p[1] > b[1][1] ) {
		return false;
	}
	return true;
}

ID_INLINE bool anBoundsShort::IntersectsBounds( const anBounds &a ) const {
	if ( a.GetMaxs()[0] < b[0][0] || a.GetMaxs()[1] < b[0][1] || a.GetMaxs()[2] < b[0][2]
		|| a.GetMins()[0] > b[1][0] || a.GetMins()[1] > b[1][1] || a.GetMins()[2] > b[1][2] ) {
		return false;
	}
	return true;
}

ID_INLINE bool anBoundsShort::IntersectsBounds( const anBoundsShort &a ) const {
	if ( a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[1][2] < b[0][2]
		|| a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] || a.b[0][2] > b[1][2] ) {
		return false;
	}
	return true;
}

ID_INLINE bool anBoundsShort::IntersectsBounds2D( const anBoundsShort &a ) const {
	if ( a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] ) {
		return false;
	}
	return true;
}

ID_INLINE anBounds anBoundsShort::ToBounds( void ) const {
	return anBounds( anVec3( b[0][0], b[0][1], b[0][2] ), anVec3( b[1][0], b[1][1], b[1][2] ) );
}

ID_INLINE void anBoundsShort::AddBounds( const anBoundsShort& a ) {
	if ( a.b[0][0] < b[0][0] ) {
		b[0][0] = a.b[0][0];
	}
	if ( a.b[0][1] < b[0][1] ) {
		b[0][1] = a.b[0][1];
	}
	if ( a.b[0][2] < b[0][2] ) {
		b[0][2] = a.b[0][2];
	}
	if ( a.b[1][0] > b[1][0] ) {
		b[1][0] = a.b[1][0];
	}
	if ( a.b[1][1] > b[1][1] ) {
		b[1][1] = a.b[1][1];
	}
	if ( a.b[1][2] > b[1][2] ) {
		b[1][2] = a.b[1][2];
	}
}

ID_INLINE  anBoundsShort anBoundsShort::operator+( const anBoundsShort& a ) const {
	anBoundsShort other;
	other = *this;
	other.AddBounds( a );
	return other;
}

ID_INLINE int anBoundsShort::GetLargestAxis( void ) const {
	short work[ 3 ];
	work[ 0 ] = b[ 1 ][ 0 ] - b[ 0 ][ 0 ];
	work[ 1 ] = b[ 1 ][ 1 ] - b[ 0 ][ 1 ];
	work[ 2 ] = b[ 1 ][ 2 ] - b[ 0 ][ 2 ];

	int axis = 0;

	if ( work[ 1 ] > work[ 0 ] ) {
		axis = 1;
	}

	if ( work[ 2 ] > work[ axis ] ) {
		axis = 2;
	}

	return axis;
}

ID_INLINE void anBoundsShort::TranslateSelf( const anVec3 &offset ) {
	b[ 0 ][ 0 ] += anMath::Ftoi( anMath::Floor( offset[ 0 ] ) );
	b[ 0 ][ 1 ] += anMath::Ftoi( anMath::Floor( offset[ 1 ] ) );
	b[ 0 ][ 2 ] += anMath::Ftoi( anMath::Floor( offset[ 2 ] ) );
	b[ 1 ][ 0 ] += anMath::Ftoi( anMath::Ceil( offset[ 0 ] ) );
	b[ 1 ][ 1 ] += anMath::Ftoi( anMath::Ceil( offset[ 1 ] ) );
	b[ 1 ][ 2 ] += anMath::Ftoi( anMath::Ceil( offset[ 2 ] ) );
}

ID_INLINE void anBoundsShort::Combine( const anBoundsShort& x, const anBoundsShort& y ) {
	b[0][0] = ( x.b[0][0] < y.b[0][0] ) ? x.b[0][0] : y.b[0][0];
	b[0][1] = ( x.b[0][1] < y.b[0][1] ) ? x.b[0][1] : y.b[0][1];
	b[0][2] = ( x.b[0][2] < y.b[0][2] ) ? x.b[0][2] : y.b[0][2];
	b[1][0] = ( x.b[1][0] > y.b[1][0] ) ? x.b[1][0] : y.b[1][0];
	b[1][1] = ( x.b[1][1] > y.b[1][1] ) ? x.b[1][1] : y.b[1][1];
	b[1][2] = ( x.b[1][2] > y.b[1][2] ) ? x.b[1][2] : y.b[1][2];
}

ID_INLINE void anBoundsShort::AddPoint( const anVec3 &x ) {
	anBoundsShort temp;
	temp.b[ 0 ][ 0 ] = anMath::Ftoi( anMath::Floor( x[ 0 ] ) );
	temp.b[ 0 ][ 1 ] = anMath::Ftoi( anMath::Floor( x[ 1 ] ) );
	temp.b[ 0 ][ 2 ] = anMath::Ftoi( anMath::Floor( x[ 2 ] ) );
	temp.b[ 1 ][ 0 ] = anMath::Ftoi( anMath::Ceil( x[ 0 ] ) );
	temp.b[ 1 ][ 1 ] = anMath::Ftoi( anMath::Ceil( x[ 1 ] ) );
	temp.b[ 1 ][ 2 ] = anMath::Ftoi( anMath::Ceil( x[ 2 ] ) );
	AddBounds( temp );
}


#endif /* !__BV_BOUNDSSHORT_H__ */

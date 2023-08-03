#ifndef __BV_BOUNDS2D_H__
#define __BV_BOUNDS2D_H__

class an2DBounds {
public:

	enum eSide {
		SIDE_LEFT		= BITT< 0 >::VALUE,
		SIDE_RIGHT		= BITT< 1 >::VALUE,
		SIDE_TOP		= BITT< 2 >::VALUE,
		SIDE_BOTTOM		= BITT< 3 >::VALUE,
		SIDE_INTERIOR	= BITT< 4 >::VALUE
	};

	enum eScreenSpace {
		ARIAL_SPACE_INCREASAE,
		ARIAL_SPACE_DECLINE
	};

					an2DBounds();
					an2DBounds( const anBounds &rhs, size_t ignoreAxis = 2 );
					an2DBounds( const float x, const float y, const float w, const float h );
					an2DBounds( const anVec2 &mins, const anVec2 &maxs );
	explicit		an2DBounds( const anVec4 &vec );
	void			Zero();

	const anVec2 &	operator[]( int index ) const;
	anVec2 &			operator[]( int index );
	an2DBounds		operator+( const an2DBounds &rhs ) const;
	an2DBounds &	operator+=( const an2DBounds &rhs );
	an2DBounds		operator*=( const anVec2 &s ) const;
	an2DBounds &	operator*=( const anVec2 &s );
	bool			operator==( const an2DBounds &rhs ) const;
	bool			operator!=( const an2DBounds &rhs ) const;

	anVec4			ToVec4();
	void			FromRectangle( const anVec4 &rect );
	void			FromRectangle( const float x, const float y, const float w, const float h );
	void			Clear();

	bool			AddPoint( const anVec2 &p );
	bool			AddBounds( const an2DBounds &rhs );

	int				GetLargestAxis( void ) const;

	anVec2 &		GetMins();
	const anVec2 &	GetMins() const;

	anVec2 &		GetMaxs();
	const anVec2 &	GetMaxs() const;

	bool			Compare( const an2DBounds& rhs ) const;

	anVec2			GetCenter() const;

	bool			IsCleared() const;
	bool			IsCollapsed( float epsilon = VECTOR_EPSILON ) const;

	bool			ContainsPoint( const anVec2 &point ) const;
	bool			ContainsBounds( const an2DBounds &bounds ) const;
	int				SideForPoint( const anVec2 &point, eScreenSpace space = ARIAL_SPACE_INCREASAE ) const;
	bool			IntersectsBounds( const an2DBounds &other ) const;
	void			IntersectBounds( const an2DBounds &other, an2DBounds &result ) const;

	float			GetWidth() const;
	float			GetHeight() const;

	float&			GetLeft();
	float&			GetRight();

	float&			GetTop( eScreenSpace space = ARIAL_SPACE_INCREASAE );
	float&			GetBottom( eScreenSpace space = ARIAL_SPACE_INCREASAE );

	float			GetLeft() const;
	float			GetRight() const;

	float			GetTop( eScreenSpace space = ARIAL_SPACE_INCREASAE ) const;
	float			GetBottom( eScreenSpace space = ARIAL_SPACE_INCREASAE ) const;

	void			TranslateSelf( const float xOffset, const float yOffset );
	void			TranslateSelf( const anVec2 &offset );

	anVec2			GetSize( void ) const;

	void			ExpandSelf( const float xOffset, const float yOffset );
	void			ExpandSelf( const anVec2 &offset );

    void			MakeValid();

private:
	anVec2 bounds[2];
};

extern an2DBounds bounds2d_zero;

/*
============
an2DBounds::an2DBounds
============
*/
ARC_INLINE an2DBounds::an2DBounds( const float x, const float y, const float w, const float h ) {
	FromRectangle( x, y, w, h );
}

/*
============
an2DBounds::an2DBounds
============
*/
ARC_INLINE an2DBounds::an2DBounds( const anVec4& vec ) {
	FromRectangle( vec.x, vec.y, vec.z, vec.w );
}

/*
============
an2DBounds::FromRectangle
============
*/
ARC_INLINE void an2DBounds::FromRectangle( const float x, const float y, const float w, const float h ) {
	bounds[0][0] = x;
	bounds[1][0] = x + w;
	bounds[0][1] = y;
	bounds[1][1] = y + h;
}

/*
============
an2DBounds::GetWidth
============
*/
ARC_INLINE float	an2DBounds::GetWidth() const {
	return bounds[1][0] - bounds[0][0];
}

/*
============
an2DBounds::GetHeight
============
*/
ARC_INLINE float	an2DBounds::GetHeight() const {
	return bounds[1][1] - bounds[0][1];
}

/*
============
an2DBounds::operator[]
============
*/
ARC_INLINE const anVec2& an2DBounds::operator[]( int index ) const {
	return bounds[ index ];
}

/*
============
an2DBounds::operator[]
============
*/
ARC_INLINE anVec2&	an2DBounds::operator[]( int index ) {
	return bounds[index];
}

/*
============
an2DBounds::Zero
============
*/
ARC_INLINE void an2DBounds::Zero() {
	bounds[0] = bounds[1] = vec2_zero;
}

/*
============
an2DBounds::GetMins
============
*/
ARC_INLINE const anVec2& an2DBounds::GetMins() const {
	return bounds[0];
}

/*
============
an2DBounds::GetMaxs
============
*/
ARC_INLINE const anVec2& an2DBounds::GetMaxs() const {
	return bounds[1];
}

/*
============
an2DBounds::GetMins
============
*/
ARC_INLINE anVec2& an2DBounds::GetMins() {
	return bounds[0];
}

/*
============
an2DBounds::GetMaxs
============
*/
ARC_INLINE anVec2& an2DBounds::GetMaxs() {
	return bounds[1];
}

/*
============
an2DBounds::GetCenter
============
*/
ARC_INLINE anVec2 an2DBounds::GetCenter() const {
	return bounds[0] + ( ( bounds[1] - bounds[0] ) * 0.5f );
}

/*
============
an2DBounds::operator+
============
*/
ARC_INLINE an2DBounds an2DBounds::operator+( const an2DBounds &rhs ) const {
	an2DBounds newBounds;
	newBounds = *this;
	newBounds.AddBounds( rhs );
	return newBounds;
}

/*
============
an2DBounds::operator+=
============
*/
ARC_INLINE an2DBounds &an2DBounds::operator+=( const an2DBounds &rhs ) {
	an2DBounds::AddBounds( rhs );
	return *this;
}

/*
============
an2DBounds::AddBounds
============
*/
ARC_INLINE bool an2DBounds::AddBounds( const an2DBounds &rhs ) {
	bool expanded = false;
	if ( rhs.bounds[0][0] < bounds[0][0] ) {
		bounds[0][0] = rhs.bounds[0][0];
		expanded = true;
	}
	if ( rhs.bounds[0][1] < bounds[0][1] ) {
		bounds[0][1] = rhs.bounds[0][1];
		expanded = true;
	}
	if ( rhs.bounds[1][0] > bounds[1][0] ) {
		bounds[1][0] = rhs.bounds[1][0];
		expanded = true;
	}
	if ( rhs.bounds[1][1] > bounds[1][1] ) {
		bounds[1][1] = rhs.bounds[1][1];
		expanded = true;
	}
	return expanded;
}

/*
============
an2DBounds::Clear
============
*/
ARC_INLINE void an2DBounds::Clear(){
	bounds[0].Set( anMath::INFINITY, anMath::INFINITY );
	bounds[1].Set( -anMath::INFINITY, -anMath::INFINITY );
}

/*
============
an2DBounds::IsCleared
============
*/
ARC_INLINE bool an2DBounds::IsCleared() const {
	return( bounds[0][0] > bounds[1][0] );
}

/*
============
an2DBounds::IsCollapsed
============
*/
ARC_INLINE bool an2DBounds::IsCollapsed( float epsilon ) const {
	return(	( anMath::Fabs( bounds[1][0] - bounds[0][0] ) < epsilon )  ||
			( anMath::Fabs( bounds[1][1] - bounds[0][1] ) < epsilon ) );
}

/*
============
an2DBounds::operator*=
============
*/
ARC_INLINE an2DBounds an2DBounds::operator*=( const anVec2& s ) const {
	return an2DBounds( anVec2( bounds[0][0] * s[0], bounds[0][1] * s[1] ), anVec2( bounds[1][0] * s[0], bounds[1][1] * s[1] ) );
}

/*
============
an2DBounds::Compare
============
*/
ARC_INLINE bool an2DBounds::Compare( const an2DBounds& rhs ) const {
	return ( bounds[0].Compare( rhs.bounds[0] ) && bounds[1].Compare( rhs.bounds[1] ) );
}

/*
============
an2DBounds::operator*=
============
*/
ARC_INLINE an2DBounds & an2DBounds::operator*=( const anVec2& s ) {
	this->bounds[0][0] *= s[0];
	this->bounds[0][1] *= s[1];
	this->bounds[1][0] *= s[0];
	this->bounds[1][1] *= s[1];
	return *this;
}

/*
============
an2DBounds::operator==
============
*/
ARC_INLINE bool an2DBounds::operator==( const an2DBounds& rhs ) const {
	return Compare( rhs );
}

/*
============
an2DBounds::operator!=
============
*/
ARC_INLINE bool an2DBounds::operator!=( const an2DBounds& rhs ) const {
	return !Compare( rhs );
}

/*
============
an2DBounds::GetLargestAxis
============
*/
ARC_INLINE int an2DBounds::GetLargestAxis( void ) const {
	anVec2 work = bounds[1] - bounds[0];
	int axis = 0;

	if ( work[1] > work[0] ) {
		axis = 1;
	}
	return( axis );
}

/*
============
an2DBounds::MakeValid
============
*/
ARC_INLINE void an2DBounds::MakeValid() {
	if ( bounds[0].x > bounds[1].x ) {
		anSwap(bounds[0].x, bounds[1].x );
	}
	if ( bounds[0].y > bounds[1].y ) {
		anSwap(bounds[0].y, bounds[1].y );
	}
}

/*
============
an2DBounds::SideForPoint
============
*/
ARC_INLINE int an2DBounds::SideForPoint( const anVec2& point, eScreenSpace space ) const {
	int sides = 0;
	if ( point.x < bounds[0].x ) {
		sides |= SIDE_LEFT;
	} else if ( point.x > bounds[1].x ) {
		sides |= SIDE_RIGHT;
	}

	if ( point.y < bounds[0].y ) {
		sides |= ( space == ARIAL_SPACE_INCREASAE ) ? SIDE_TOP : SIDE_BOTTOM;
	} else if ( point.y > bounds[1].y ) {
		sides |= ( space == ARIAL_SPACE_INCREASAE ) ? SIDE_BOTTOM : SIDE_TOP;
	}

	if ( sides == 0 ) {
		sides = SIDE_INTERIOR;
	}

	return sides;
}

/*
============
an2DBounds::an2DBounds
============
*/
ARC_INLINE an2DBounds::an2DBounds() {
}

/*
============
an2DBounds::an2DBounds
============
*/
ARC_INLINE an2DBounds::an2DBounds( const anBounds& rhs, size_t ignoreAxis ) {
	const anVec3 &mins = rhs.GetMins();
	const anVec3 &maxs = rhs.GetMaxs();

	switch ( ignoreAxis ) {
		case 0:
			bounds[0][0] = mins[1];
			bounds[1][0] = maxs[1];

			bounds[0][1] = mins[2];
			bounds[1][1] = maxs[2];
			break;
		case 1:
			bounds[0][0] = mins[0];
			bounds[1][0] = maxs[0];

			bounds[0][1] = mins[2];
			bounds[1][1] = maxs[2];
			break;
		case 2:
			bounds[0][0] = mins[0];
			bounds[1][0] = maxs[0];

			bounds[0][1] = mins[1];
			bounds[1][1] = maxs[1];
			break;
	}
}

/*
============
an2DBounds::an2DBounds
============
*/
ARC_INLINE an2DBounds::an2DBounds( const anVec2& mins, const anVec2& maxs ) {
	bounds[0] = mins;
	bounds[1] = maxs;
}

/*
============
an2DBounds::AddPoint
============
*/
ARC_INLINE bool an2DBounds::AddPoint( const anVec2& p ) {
	bool expanded = false;
	for ( int j = 0; j < 2; j++ ) {
		if ( p[j] < bounds[0][j] ) {
			bounds[0][j] = p[j];
			expanded = true;
		}
		if ( p[j] > bounds[1][j] ) {
			bounds[1][j] = p[j];
			expanded = true;
		}
	}
	return expanded;
}

/*
============
an2DBounds::ContainsPoint
============
*/
ARC_INLINE bool an2DBounds::ContainsPoint( const anVec2& point ) const {
	bool contained = true;
	for (  int i = 0; i < 2; i++ ) {
		if ( point[i] < bounds[0][i] ) {
			contained = false;
			break;
		}
		if ( point[i] > bounds[1][i] ) {
			contained = false;
			break;
		}
	}
	return contained;
}

/*
============
an2DBounds::ContainsBounds
============
*/
ARC_INLINE bool an2DBounds::ContainsBounds( const an2DBounds& bounds ) const {
	return ( ( bounds.GetMins().x >= GetMins().x ) &&
			( bounds.GetMins().y >= GetMins().y ) &&
			( bounds.GetMaxs().x <= GetMaxs().x ) &&
			( bounds.GetMaxs().y <= GetMaxs().y ) );
}

/*
============
an2DBounds::IntersectsBounds
============
*/
ARC_INLINE bool an2DBounds::IntersectsBounds( const an2DBounds& other ) const {
	if ( &other == this ) {
		return true;
	}

	if ( other.bounds[1][0] < bounds[0][0] || other.bounds[1][1] < bounds[0][1]
	|| other.bounds[0][0] > bounds[1][0] || other.bounds[0][1] > bounds[1][1] ) {
		return false;
	}

	return true;
}

/*
============
an2DBounds::IntersectBounds
============
*/
ARC_INLINE void an2DBounds::IntersectBounds( const an2DBounds& other, an2DBounds& result ) const {
	if ( &other == this ) {
		result = *this;
		return;
	}

	result.Clear();

	for ( int i = 0; i < 2; i++ ) {
		if ( other.bounds[0][i] < bounds[0][i] ) {
			result[0][i] = bounds[0][i];
		} else {
			result[0][i] = other.bounds[0][i];
		}

		if ( other.bounds[1][i] > bounds[1][i] ) {
			result[1][i] = bounds[1][i];
		} else {
			result[1][i] = other.bounds[1][i];
		}
	}
}

/*
============
an2DBounds::TranslateSelf
============
*/
ARC_INLINE void an2DBounds::TranslateSelf( const anVec2& offset ) {
	TranslateSelf( offset.x, offset.y );
}

/*
============
an2DBounds::TranslateSelf
============
*/
ARC_INLINE void an2DBounds::TranslateSelf( const float xOffset, const float yOffset ) {
	GetMins().x += xOffset;
	GetMaxs().x += xOffset;

	GetMins().y += yOffset;
	GetMaxs().y += yOffset;
}

/*
============
an2DBounds::ExpandSelf
============
*/
ARC_INLINE anVec2 an2DBounds::GetSize( void ) const {
	return GetMaxs() - GetMins();
}

/*
============
an2DBounds::ExpandSelf
============
*/
ARC_INLINE void an2DBounds::ExpandSelf( const float xOffset, const float yOffset ) {
	GetMins().x -= xOffset;
	GetMaxs().x += xOffset;

	GetMins().y -= yOffset;
	GetMaxs().y += yOffset;
}

/*
============
an2DBounds::ExpandSelf
============
*/
ARC_INLINE void an2DBounds::ExpandSelf( const anVec2& offset ) {
	ExpandSelf( offset.x, offset.y );
}

/*
============
an2DBounds::GetLeft
============
*/
ARC_INLINE float an2DBounds::GetLeft() const {
	return GetMins().x;
}

/*
============
an2DBounds::Right
============
*/
ARC_INLINE float an2DBounds::GetRight() const {
	return GetMaxs().x;
}

/*
============
an2DBounds::GetTop
============
*/
ARC_INLINE float an2DBounds::GetTop( eScreenSpace space ) const {
	return ( space == ARIAL_SPACE_INCREASAE ) ? GetMins().y : GetMaxs().y;
}

/*
============
an2DBounds::GetBottom
============
*/
ARC_INLINE float an2DBounds::GetBottom( eScreenSpace space ) const {
	return ( space == ARIAL_SPACE_INCREASAE ) ? GetMaxs().y : GetMins().y;
}

/*
============
an2DBounds::GetLeft
============
*/
ARC_INLINE float &an2DBounds::GetLeft() {
	return GetMins().x;
}

/*
============
an2DBounds::GetRight
============
*/
ARC_INLINE float &an2DBounds::GetRight() {
	return GetMaxs().x;
}

/*
============
an2DBounds::GetTop
============
*/
ARC_INLINE float &an2DBounds::GetTop( eScreenSpace space ) {
	return ( space == ARIAL_SPACE_INCREASAE ) ? GetMins().y : GetMaxs().y;
}

/*
============
an2DBounds::GetBottom
============
*/
ARC_INLINE float &an2DBounds::GetBottom( eScreenSpace space ) {
	return ( space == ARIAL_SPACE_INCREASAE ) ? GetMaxs().y : GetMins().y;
}


/*
============
an2DBounds::ToVec4
============
*/
ARC_INLINE anVec4 an2DBounds::ToVec4() {
	return anVec4( GetMins().x, GetMins().y, GetWidth(), GetHeight() );
}

/*
============
an2DBounds::FromRectangle
============
*/
ARC_INLINE void an2DBounds::FromRectangle( const anVec4& rect ) {
	FromRectangle( rect.x, rect.y, rect.z, rect.w );
}

#endif // ! __BV_BOUNDS2D_H__
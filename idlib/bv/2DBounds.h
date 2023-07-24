#ifndef __BV_BOUNDS2D_H__
#define __BV_BOUNDS2D_H__

class arcNet2DBounds {
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

					arcNet2DBounds();
					arcNet2DBounds( const arcBounds &rhs, size_t ignoreAxis = 2 );
					arcNet2DBounds( const float x, const float y, const float w, const float h );
					arcNet2DBounds( const arcVec2 &mins, const arcVec2 &maxs );
	explicit		arcNet2DBounds( const arcVec4 &vec );
	void			Zero();

	const arcVec2 &	operator[]( int index ) const;
	arcVec2 &			operator[]( int index );
	arcNet2DBounds		operator+( const arcNet2DBounds &rhs ) const;
	arcNet2DBounds &	operator+=( const arcNet2DBounds &rhs );
	arcNet2DBounds		operator*=( const arcVec2 &s ) const;
	arcNet2DBounds &		operator*=( const arcVec2 &s );
	bool			operator==( const arcNet2DBounds &rhs ) const;
	bool			operator!=( const arcNet2DBounds &rhs ) const;

	arcVec4			ToVec4();
	void			FromRectangle( const arcVec4 &rect );
	void			FromRectangle( const float x, const float y, const float w, const float h );
	void			Clear();

	bool			AddPoint( const arcVec2 &p );
	bool			AddBounds( const arcNet2DBounds &rhs );

	int				GetLargestAxis( void ) const;

	arcVec2 &			GetMins();
	const arcVec2 &	GetMins() const;

	arcVec2 &			GetMaxs();
	const arcVec2 &	GetMaxs() const;

	bool			Compare( const arcNet2DBounds& rhs ) const;

	arcVec2			GetCenter() const;

	bool			IsCleared() const;
	bool			IsCollapsed( float epsilon = VECTOR_EPSILON ) const;

	bool			ContainsPoint( const arcVec2 &point ) const;
	bool			ContainsBounds( const arcNet2DBounds &bounds ) const;
	int				SideForPoint( const arcVec2 &point, eScreenSpace space = ARIAL_SPACE_INCREASAE ) const;
	bool			IntersectsBounds( const arcNet2DBounds &other ) const;
	void			IntersectBounds( const arcNet2DBounds &other, arcNet2DBounds &result ) const;

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
	void			TranslateSelf( const arcVec2 &offset );

	arcVec2			GetSize( void ) const;

	void			ExpandSelf( const float xOffset, const float yOffset );
	void			ExpandSelf( const arcVec2 &offset );

    void			MakeValid();

private:
	arcVec2 bounds[2];
};

extern arcNet2DBounds bounds2d_zero;


/*
============
arcNet2DBounds::arcNet2DBounds
============
*/
ARC_INLINE arcNet2DBounds::arcNet2DBounds( const float x, const float y, const float w, const float h ) {
	FromRectangle( x, y, w, h );
}


/*
============
arcNet2DBounds::arcNet2DBounds
============
*/
ARC_INLINE arcNet2DBounds::arcNet2DBounds( const arcVec4& vec ) {
	FromRectangle( vec.x, vec.y, vec.z, vec.w );
}


/*
============
arcNet2DBounds::FromRectangle
============
*/
ARC_INLINE void arcNet2DBounds::FromRectangle( const float x, const float y, const float w, const float h ) {
	bounds[0][0] = x;
	bounds[1][0] = x + w;
	bounds[0][1] = y;
	bounds[1][1] = y + h;
}

/*
============
arcNet2DBounds::GetWidth
============
*/
ARC_INLINE float	arcNet2DBounds::GetWidth() const {
	return bounds[1][0] - bounds[0][0];
}

/*
============
arcNet2DBounds::GetHeight
============
*/
ARC_INLINE float	arcNet2DBounds::GetHeight() const {
	return bounds[1][1] - bounds[0][1];
}

/*
============
arcNet2DBounds::operator[]
============
*/
ARC_INLINE const arcVec2& arcNet2DBounds::operator[]( int index ) const {
	return bounds[ index ];
}

/*
============
arcNet2DBounds::operator[]
============
*/
ARC_INLINE arcVec2&	arcNet2DBounds::operator[]( int index ) {
	return bounds[index];
}

/*
============
arcNet2DBounds::Zero
============
*/
ARC_INLINE void arcNet2DBounds::Zero() {
	bounds[0] = bounds[1] = vec2_zero;
}

/*
============
arcNet2DBounds::GetMins
============
*/
ARC_INLINE const arcVec2& arcNet2DBounds::GetMins() const {
	return bounds[0];
}

/*
============
arcNet2DBounds::GetMaxs
============
*/
ARC_INLINE const arcVec2& arcNet2DBounds::GetMaxs() const {
	return bounds[1];
}

/*
============
arcNet2DBounds::GetMins
============
*/
ARC_INLINE arcVec2& arcNet2DBounds::GetMins() {
	return bounds[0];
}

/*
============
arcNet2DBounds::GetMaxs
============
*/
ARC_INLINE arcVec2& arcNet2DBounds::GetMaxs() {
	return bounds[1];
}

/*
============
arcNet2DBounds::GetCenter
============
*/
ARC_INLINE arcVec2 arcNet2DBounds::GetCenter() const {
	return bounds[0] + ( ( bounds[1] - bounds[0] ) * 0.5f );
}

/*
============
arcNet2DBounds::operator+
============
*/
ARC_INLINE arcNet2DBounds arcNet2DBounds::operator+( const arcNet2DBounds &rhs ) const {
	arcNet2DBounds newBounds;
	newBounds = *this;
	newBounds.AddBounds( rhs );
	return newBounds;
}

/*
============
arcNet2DBounds::operator+=
============
*/
ARC_INLINE arcNet2DBounds &arcNet2DBounds::operator+=( const arcNet2DBounds &rhs ) {
	arcNet2DBounds::AddBounds( rhs );
	return *this;
}

/*
============
arcNet2DBounds::AddBounds
============
*/
ARC_INLINE bool arcNet2DBounds::AddBounds( const arcNet2DBounds &rhs ) {
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
arcNet2DBounds::Clear
============
*/
ARC_INLINE void arcNet2DBounds::Clear(){
	bounds[0].Set( arcMath::INFINITY, arcMath::INFINITY );
	bounds[1].Set( -arcMath::INFINITY, -arcMath::INFINITY );
}

/*
============
arcNet2DBounds::IsCleared
============
*/
ARC_INLINE bool arcNet2DBounds::IsCleared() const {
	return( bounds[0][0] > bounds[1][0] );
}

/*
============
arcNet2DBounds::IsCollapsed
============
*/
ARC_INLINE bool arcNet2DBounds::IsCollapsed( float epsilon ) const {
	return(	( arcMath::Fabs( bounds[1][0] - bounds[0][0] ) < epsilon )  ||
			( arcMath::Fabs( bounds[1][1] - bounds[0][1] ) < epsilon ) );
}

/*
============
arcNet2DBounds::operator*=
============
*/
ARC_INLINE arcNet2DBounds arcNet2DBounds::operator*=( const arcVec2& s ) const {
	return arcNet2DBounds( arcVec2( bounds[0][0] * s[0], bounds[0][1] * s[1] ), arcVec2( bounds[1][0] * s[0], bounds[1][1] * s[1] ) );
}

/*
============
arcNet2DBounds::Compare
============
*/
ARC_INLINE bool arcNet2DBounds::Compare( const arcNet2DBounds& rhs ) const {
	return ( bounds[0].Compare( rhs.bounds[0] ) && bounds[1].Compare( rhs.bounds[1] ) );
}

/*
============
arcNet2DBounds::operator*=
============
*/
ARC_INLINE arcNet2DBounds & arcNet2DBounds::operator*=( const arcVec2& s ) {
	this->bounds[0][0] *= s[0];
	this->bounds[0][1] *= s[1];
	this->bounds[1][0] *= s[0];
	this->bounds[1][1] *= s[1];
	return *this;
}


/*
============
arcNet2DBounds::operator==
============
*/
ARC_INLINE bool arcNet2DBounds::operator==( const arcNet2DBounds& rhs ) const {
	return Compare( rhs );
}

/*
============
arcNet2DBounds::operator!=
============
*/
ARC_INLINE bool arcNet2DBounds::operator!=( const arcNet2DBounds& rhs ) const {
	return !Compare( rhs );
}

/*
============
arcNet2DBounds::GetLargestAxis
============
*/
ARC_INLINE int arcNet2DBounds::GetLargestAxis( void ) const
{
	arcVec2 work = bounds[1] - bounds[0];
	int axis = 0;

	if ( work[1] > work[0] ) {
		axis = 1;
	}
	return( axis );
}


/*
============
arcNet2DBounds::MakeValid
============
*/
ARC_INLINE void arcNet2DBounds::MakeValid() {
	if ( bounds[0].x > bounds[1].x ) {
		idSwap(bounds[0].x, bounds[1].x );
	}
	if ( bounds[0].y > bounds[1].y ) {
		idSwap(bounds[0].y, bounds[1].y );
	}
}


/*
============
arcNet2DBounds::SideForPoint
============
*/
ARC_INLINE int arcNet2DBounds::SideForPoint( const arcVec2& point, eScreenSpace space ) const {
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
arcNet2DBounds::arcNet2DBounds
============
*/
ARC_INLINE arcNet2DBounds::arcNet2DBounds () {
}

/*
============
arcNet2DBounds::arcNet2DBounds
============
*/
ARC_INLINE arcNet2DBounds::arcNet2DBounds( const arcBounds& rhs, size_t ignoreAxis ) {
	const arcVec3 &mins = rhs.GetMins();
	const arcVec3 &maxs = rhs.GetMaxs();

	switch( ignoreAxis ) {
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
arcNet2DBounds::arcNet2DBounds
============
*/
ARC_INLINE arcNet2DBounds::arcNet2DBounds( const arcVec2& mins, const arcVec2& maxs ) {
	bounds[0] = mins;
	bounds[1] = maxs;
}

/*
============
arcNet2DBounds::AddPoint
============
*/
ARC_INLINE bool arcNet2DBounds::AddPoint( const arcVec2& p ) {
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
arcNet2DBounds::ContainsPoint
============
*/
ARC_INLINE bool arcNet2DBounds::ContainsPoint( const arcVec2& point ) const {
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
arcNet2DBounds::ContainsBounds
============
*/
ARC_INLINE bool arcNet2DBounds::ContainsBounds( const arcNet2DBounds& bounds ) const {
	return (( bounds.GetMins().x >= GetMins().x ) &&
			( bounds.GetMins().y >= GetMins().y ) &&
			( bounds.GetMaxs().x <= GetMaxs().x ) &&
			( bounds.GetMaxs().y <= GetMaxs().y ) );
}

/*
============
arcNet2DBounds::IntersectsBounds
============
*/
ARC_INLINE bool arcNet2DBounds::IntersectsBounds( const arcNet2DBounds& other ) const {
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
arcNet2DBounds::IntersectBounds
============
*/
ARC_INLINE void arcNet2DBounds::IntersectBounds( const arcNet2DBounds& other, arcNet2DBounds& result ) const {
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
arcNet2DBounds::TranslateSelf
============
*/
ARC_INLINE void arcNet2DBounds::TranslateSelf( const arcVec2& offset ) {
	TranslateSelf( offset.x, offset.y );
}

/*
============
arcNet2DBounds::TranslateSelf
============
*/
ARC_INLINE void arcNet2DBounds::TranslateSelf( const float xOffset, const float yOffset ) {
	GetMins().x += xOffset;
	GetMaxs().x += xOffset;

	GetMins().y += yOffset;
	GetMaxs().y += yOffset;
}

/*
============
arcNet2DBounds::ExpandSelf
============
*/
ARC_INLINE arcVec2 arcNet2DBounds::GetSize( void ) const {
	return GetMaxs() - GetMins();
}

/*
============
arcNet2DBounds::ExpandSelf
============
*/
ARC_INLINE void arcNet2DBounds::ExpandSelf( const float xOffset, const float yOffset ) {
	GetMins().x -= xOffset;
	GetMaxs().x += xOffset;

	GetMins().y -= yOffset;
	GetMaxs().y += yOffset;
}

/*
============
arcNet2DBounds::ExpandSelf
============
*/
ARC_INLINE void arcNet2DBounds::ExpandSelf( const arcVec2& offset ) {
	ExpandSelf( offset.x, offset.y );
}

/*
============
arcNet2DBounds::GetLeft
============
*/
ARC_INLINE float	arcNet2DBounds::GetLeft() const {
	return GetMins().x;
}

/*
============
arcNet2DBounds::Right
============
*/
ARC_INLINE float	arcNet2DBounds::GetRight() const {
	return GetMaxs().x;
}

/*
============
arcNet2DBounds::GetTop
============
*/
ARC_INLINE float	arcNet2DBounds::GetTop( eScreenSpace space ) const {
	return ( space == ARIAL_SPACE_INCREASAE ) ? GetMins().y : GetMaxs().y;
}

/*
============
arcNet2DBounds::GetBottom
============
*/
ARC_INLINE float	arcNet2DBounds::GetBottom( eScreenSpace space ) const {
	return ( space == ARIAL_SPACE_INCREASAE ) ? GetMaxs().y : GetMins().y;
}

/*
============
arcNet2DBounds::GetLeft
============
*/
ARC_INLINE float& arcNet2DBounds::GetLeft() {
	return GetMins().x;
}

/*
============
arcNet2DBounds::GetRight
============
*/
ARC_INLINE float& arcNet2DBounds::GetRight() {
	return GetMaxs().x;
}

/*
============
arcNet2DBounds::GetTop
============
*/
ARC_INLINE float& arcNet2DBounds::GetTop( eScreenSpace space ) {
	return ( space == ARIAL_SPACE_INCREASAE ) ? GetMins().y : GetMaxs().y;
}

/*
============
arcNet2DBounds::GetBottom
============
*/
ARC_INLINE float& arcNet2DBounds::GetBottom( eScreenSpace space ) {
	return ( space == ARIAL_SPACE_INCREASAE ) ? GetMaxs().y : GetMins().y;
}


/*
============
arcNet2DBounds::ToVec4
============
*/
ARC_INLINE arcVec4 arcNet2DBounds::ToVec4() {
	return arcVec4( GetMins().x, GetMins().y, GetWidth(), GetHeight() );
}

/*
============
arcNet2DBounds::FromRectangle
============
*/
ARC_INLINE void arcNet2DBounds::FromRectangle( const arcVec4& rect ) {
	FromRectangle( rect.x, rect.y, rect.z, rect.w );
}

#endif /* ! __BV_BOUNDS2D_H__ */

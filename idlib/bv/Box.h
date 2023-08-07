#ifndef __BV_BOX_H__
#define __BV_BOX_H__

/*
===============================================================================

	Oriented Bounding Box

===============================================================================
*/

class anBox {
public:
					anBox( void );
					explicit anBox( const anVec3 &center, const anVec3 &extents, const anMat3 &axis );
					explicit anBox( const anVec3 &point );
					explicit anBox( const anBounds &bounds );
					explicit anBox( const anBounds &bounds, const anVec3 &origin, const anMat3 &axis );

	anBox			operator+( const anVec3 &t ) const;				// returns translated box
	anBox &			operator+=( const anVec3 &t );					// translate the box
	anBox			operator*( const anMat3 &r ) const;				// returns rotated box
	anBox &			operator*=( const anMat3 &r );					// rotate the box
	anBox			operator+( const anBox &a ) const;
	anBox &			operator+=( const anBox &a );
	anBox			operator-( const anBox &a ) const;
	anBox &			operator-=( const anBox &a );

	bool			Compare( const anBox &a ) const;						// exact compare, no epsilon
	bool			Compare( const anBox &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const anBox &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const anBox &a ) const;						// exact compare, no epsilon

	void			Clear( void );									// inside out box
	void			Zero( void );									// single point at origin

	const anVec3 &	GetCenter( void ) const;						// returns center of the box
	const anVec3 &	GetExtents( void ) const;						// returns extents of the box
	const anMat3 &	GetAxis( void ) const;							// returns the axis of the box
	float			GetVolume( void ) const;						// returns the volume of the box
	bool			IsCleared( void ) const;						// returns true if box are inside out

	bool			AddPoint( const anVec3 &v );					// add the point, returns true if the box expanded
	bool			AddBox( const anBox &a );						// add the box, returns true if the box expanded
	anBox			Expand( const float d ) const;					// return box expanded in all directions with the given value
	anBox &			ExpandSelf( const float d );					// expand box in all directions with the given value
	anBox			Translate( const anVec3 &translation ) const;	// return translated box
	anBox &			TranslateSelf( const anVec3 &translation );		// translate this box
	anBox			Rotate( const anMat3 &rotation ) const;			// return rotated box
	anBox &			RotateSelf( const anMat3 &rotation );			// rotate this box

	float			PlaneDistance( const anPlane &plane ) const;
	int				PlaneSide( const anPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const anVec3 &p ) const;			// includes touching
	bool			IntersectsBox( const anBox &a ) const;			// includes touching
	bool			LineIntersection( const anVec3 &start, const anVec3 &end ) const;
					// intersection points are ( start + dir * scale1) and ( start + dir * scale2)
	bool			RayIntersection( const anVec3 &start, const anVec3 &dir, float &scale1, float &scale2 ) const;

					// tight box for a collection of points
	void			FromPoints( const anVec3 *points, const int numPoints );
					// most tight box for a translation
	void			FromPointTranslation( const anVec3 &point, const anVec3 &translation );
	void			FromBoxTranslation( const anBox &box, const anVec3 &translation );
					// most tight box for a rotation
	void			FromPointRotation( const anVec3 &point, const anRotation &rotation );
	void			FromBoxRotation( const anBox &box, const anRotation &rotation );

	void			ToPoints( anVec3 points[8] ) const;
	anSphere		ToSphere( void ) const;

					// calculates the projection of this box onto the given axis
	void			AxisProjection( const anVec3 &dir, float &min, float &max ) const;
	void			AxisProjection( const anMat3 &ax, anBounds &bounds ) const;

					// calculates the silhouette of the box
	int				GetProjectionSilhouetteVerts( const anVec3 &projectionOrigin, anVec3 silVerts[6] ) const;
	int				ParallelProjSilhouetteVerts( const anVec3 &projectionDir, anVec3 silVerts[6] ) const;

private:
	anVec3			center;
	anVec3			extents;
	anMat3			axis;
};

extern anBox	box_zero;

inline anBox::anBox( void ) {
}

inline anBox::anBox( const anVec3 &center, const anVec3 &extents, const anMat3 &axis ) {
	this->center = center;
	this->extents = extents;
	this->axis = axis;
}

inline anBox::anBox( const anVec3 &point ) {
	this->center = point;
	this->extents.Zero();
	this->axis.Identity();
}

inline anBox::anBox( const anBounds &bounds ) {
	this->center = ( bounds[0] + bounds[1] ) * 0.5f;
	this->extents = bounds[1] - this->center;
	this->axis.Identity();
}

inline anBox::anBox( const anBounds &bounds, const anVec3 &origin, const anMat3 &axis ) {
	this->center = ( bounds[0] + bounds[1] ) * 0.5f;
	this->extents = bounds[1] - this->center;
	this->center = origin + this->center * axis;
	this->axis = axis;
}

inline anBox anBox::operator+( const anVec3 &t ) const {
	return anBox( center + t, extents, axis );
}

inline anBox &anBox::operator+=( const anVec3 &t ) {
	center += t;
	return *this;
}

inline anBox anBox::operator*( const anMat3 &r ) const {
	return anBox( center * r, extents, axis * r );
}

inline anBox &anBox::operator*=( const anMat3 &r ) {
	center *= r;
	axis *= r;
	return *this;
}

inline anBox anBox::operator+( const anBox &a ) const {
	anBox newBox;
	newBox = *this;
	newBox.AddBox( a );
	return newBox;
}

inline anBox &anBox::operator+=( const anBox &a ) {
	anBox::AddBox( a );
	return *this;
}

inline anBox anBox::operator-( const anBox &a ) const {
	return anBox( center, extents - a.extents, axis );
}

inline anBox &anBox::operator-=( const anBox &a ) {
	extents -= a.extents;
	return *this;
}

inline bool anBox::Compare( const anBox &a ) const {
	return ( center.Compare( a.center ) && extents.Compare( a.extents ) && axis.Compare( a.axis ) );
}

inline bool anBox::Compare( const anBox &a, const float epsilon ) const {
	return ( center.Compare( a.center, epsilon ) && extents.Compare( a.extents, epsilon ) && axis.Compare( a.axis, epsilon ) );
}

inline bool anBox::operator==( const anBox &a ) const {
	return Compare( a );
}

inline bool anBox::operator!=( const anBox &a ) const {
	return !Compare( a );
}

inline void anBox::Clear( void ) {
	center.Zero();
	extents[0] = extents[1] = extents[2] = -anMath::INFINITY;
	axis.Identity();
}

inline void anBox::Zero( void ) {
	center.Zero();
	extents.Zero();
	axis.Identity();
}

inline const anVec3 &anBox::GetCenter( void ) const {
	return center;
}

inline const anVec3 &anBox::GetExtents( void ) const {
	return extents;
}

inline const anMat3 &anBox::GetAxis( void ) const {
	return axis;
}

inline float anBox::GetVolume( void ) const {
	return ( extents * 2.0f ).LengthSqr();
}

inline bool anBox::IsCleared( void ) const {
	return extents[0] < 0.0f;
}

inline anBox anBox::Expand( const float d ) const {
	return anBox( center, extents + anVec3( d, d, d ), axis );
}

inline anBox &anBox::ExpandSelf( const float d ) {
	extents[0] += d;
	extents[1] += d;
	extents[2] += d;
	return *this;
}

inline anBox anBox::Translate( const anVec3 &translation ) const {
	return anBox( center + translation, extents, axis );
}

inline anBox &anBox::TranslateSelf( const anVec3 &translation ) {
	center += translation;
	return *this;
}

inline anBox anBox::Rotate( const anMat3 &rotation ) const {
	return anBox( center * rotation, extents, axis * rotation );
}

inline anBox &anBox::RotateSelf( const anMat3 &rotation ) {
	center *= rotation;
	axis *= rotation;
	return *this;
}

inline bool anBox::ContainsPoint( const anVec3 &p ) const {
	anVec3 lp = p - center;
	if ( anMath::Fabs( lp * axis[0] ) > extents[0] ||
			anMath::Fabs( lp * axis[1] ) > extents[1] ||
				anMath::Fabs( lp * axis[2] ) > extents[2] ) {
		return false;
	}
	return true;
}

inline anSphere anBox::ToSphere( void ) const {
	return anSphere( center, extents.Length() );
}

inline void anBox::AxisProjection( const anVec3 &dir, float &min, float &max ) const {
	float d1 = dir * center;
	float d2 = anMath::Fabs( extents[0] * ( dir * axis[0] ) ) +
				anMath::Fabs( extents[1] * ( dir * axis[1] ) ) +
				anMath::Fabs( extents[2] * ( dir * axis[2] ) );
	min = d1 - d2;
	max = d1 + d2;
}

inline void anBox::AxisProjection( const anMat3 &ax, anBounds &bounds ) const {
	for ( int i = 0; i < 3; i++ ) {
		float d1 = ax[i] * center;
		float d2 = anMath::Fabs( extents[0] * ( ax[i] * axis[0] ) ) +
					anMath::Fabs( extents[1] * ( ax[i] * axis[1] ) ) +
					anMath::Fabs( extents[2] * ( ax[i] * axis[2] ) );
		bounds[0][i] = d1 - d2;
		bounds[1][i] = d1 + d2;
	}
}

// Calculate the volume of a bounding box
inline float CalcBoundingBoxVolume( const anBounds &bounds ) {
    anVec3 dimensions = bounds.maxPoint - bounds.minPoint;
    return dimensions.x * dimensions.y * dimensions.z;
}

inline anBounds CalcBoundingBox( const anVec3 &points ) {
    anVec3 MinPoint( float numeric_limits::max() );
    anVec3 MaxPoint( float numeric_limits::lowest() );

    for ( const anVec3 &point : points ) {
        minPoint = anVec3::Min( minPoint, point );
        maxPoint = anVec3::Max( maxPoint, point );
    }

    Bounds BoundingBox( minPoint, maxPoint );
    return boundingBox;
}

#endif // !__BV_BOX_H__

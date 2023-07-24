#ifndef __BV_BOX_H__
#define __BV_BOX_H__

/*
===============================================================================

	Oriented Bounding Box

===============================================================================
*/

class ARCBox {
public:
					ARCBox( void );
					explicit ARCBox( const arcVec3 &center, const arcVec3 &extents, const arcMat3 &axis );
					explicit ARCBox( const arcVec3 &point );
					explicit ARCBox( const arcBounds &bounds );
					explicit ARCBox( const arcBounds &bounds, const arcVec3 &origin, const arcMat3 &axis );

	ARCBox			operator+( const arcVec3 &t ) const;				// returns translated box
	ARCBox &			operator+=( const arcVec3 &t );					// translate the box
	ARCBox			operator*( const arcMat3 &r ) const;				// returns rotated box
	ARCBox &			operator*=( const arcMat3 &r );					// rotate the box
	ARCBox			operator+( const ARCBox &a ) const;
	ARCBox &			operator+=( const ARCBox &a );
	ARCBox			operator-( const ARCBox &a ) const;
	ARCBox &			operator-=( const ARCBox &a );

	bool			Compare( const ARCBox &a ) const;						// exact compare, no epsilon
	bool			Compare( const ARCBox &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const ARCBox &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const ARCBox &a ) const;						// exact compare, no epsilon

	void			Clear( void );									// inside out box
	void			Zero( void );									// single point at origin

	const arcVec3 &	GetCenter( void ) const;						// returns center of the box
	const arcVec3 &	GetExtents( void ) const;						// returns extents of the box
	const arcMat3 &	GetAxis( void ) const;							// returns the axis of the box
	float			GetVolume( void ) const;						// returns the volume of the box
	bool			IsCleared( void ) const;						// returns true if box are inside out

	bool			AddPoint( const arcVec3 &v );					// add the point, returns true if the box expanded
	bool			AddBox( const ARCBox &a );						// add the box, returns true if the box expanded
	ARCBox			Expand( const float d ) const;					// return box expanded in all directions with the given value
	ARCBox &			ExpandSelf( const float d );					// expand box in all directions with the given value
	ARCBox			Translate( const arcVec3 &translation ) const;	// return translated box
	ARCBox &			TranslateSelf( const arcVec3 &translation );		// translate this box
	ARCBox			Rotate( const arcMat3 &rotation ) const;			// return rotated box
	ARCBox &			RotateSelf( const arcMat3 &rotation );			// rotate this box

	float			PlaneDistance( const arcPlane &plane ) const;
	int				PlaneSide( const arcPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const arcVec3 &p ) const;			// includes touching
	bool			IntersectsBox( const ARCBox &a ) const;			// includes touching
	bool			LineIntersection( const arcVec3 &start, const arcVec3 &end ) const;
					// intersection points are (start + dir * scale1) and (start + dir * scale2)
	bool			RayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale1, float &scale2 ) const;

					// tight box for a collection of points
	void			FromPoints( const arcVec3 *points, const int numPoints );
					// most tight box for a translation
	void			FromPointTranslation( const arcVec3 &point, const arcVec3 &translation );
	void			FromBoxTranslation( const ARCBox &box, const arcVec3 &translation );
					// most tight box for a rotation
	void			FromPointRotation( const arcVec3 &point, const arcRotate &rotation );
	void			FromBoxRotation( const ARCBox &box, const arcRotate &rotation );

	void			ToPoints( arcVec3 points[8] ) const;
	ARCSphere		ToSphere( void ) const;

					// calculates the projection of this box onto the given axis
	void			AxisProjection( const arcVec3 &dir, float &min, float &max ) const;
	void			AxisProjection( const arcMat3 &ax, arcBounds &bounds ) const;

					// calculates the silhouette of the box
	int				GetProjectionSilhouetteVerts( const arcVec3 &projectionOrigin, arcVec3 silVerts[6] ) const;
	int				GetParallelProjectionSilhouetteVerts( const arcVec3 &projectionDir, arcVec3 silVerts[6] ) const;

private:
	arcVec3			center;
	arcVec3			extents;
	arcMat3			axis;
};

extern ARCBox	box_zero;

ARC_INLINE ARCBox::ARCBox( void ) {
}

ARC_INLINE ARCBox::ARCBox( const arcVec3 &center, const arcVec3 &extents, const arcMat3 &axis ) {
	this->center = center;
	this->extents = extents;
	this->axis = axis;
}

ARC_INLINE ARCBox::ARCBox( const arcVec3 &point ) {
	this->center = point;
	this->extents.Zero();
	this->axis.Identity();
}

ARC_INLINE ARCBox::ARCBox( const arcBounds &bounds ) {
	this->center = ( bounds[0] + bounds[1] ) * 0.5f;
	this->extents = bounds[1] - this->center;
	this->axis.Identity();
}

ARC_INLINE ARCBox::ARCBox( const arcBounds &bounds, const arcVec3 &origin, const arcMat3 &axis ) {
	this->center = ( bounds[0] + bounds[1] ) * 0.5f;
	this->extents = bounds[1] - this->center;
	this->center = origin + this->center * axis;
	this->axis = axis;
}

ARC_INLINE ARCBox ARCBox::operator+( const arcVec3 &t ) const {
	return ARCBox( center + t, extents, axis );
}

ARC_INLINE ARCBox &ARCBox::operator+=( const arcVec3 &t ) {
	center += t;
	return *this;
}

ARC_INLINE ARCBox ARCBox::operator*( const arcMat3 &r ) const {
	return ARCBox( center * r, extents, axis * r );
}

ARC_INLINE ARCBox &ARCBox::operator*=( const arcMat3 &r ) {
	center *= r;
	axis *= r;
	return *this;
}

ARC_INLINE ARCBox ARCBox::operator+( const ARCBox &a ) const {
	ARCBox newBox;
	newBox = *this;
	newBox.AddBox( a );
	return newBox;
}

ARC_INLINE ARCBox &ARCBox::operator+=( const ARCBox &a ) {
	ARCBox::AddBox( a );
	return *this;
}

ARC_INLINE ARCBox ARCBox::operator-( const ARCBox &a ) const {
	return ARCBox( center, extents - a.extents, axis );
}

ARC_INLINE ARCBox &ARCBox::operator-=( const ARCBox &a ) {
	extents -= a.extents;
	return *this;
}

ARC_INLINE bool ARCBox::Compare( const ARCBox &a ) const {
	return ( center.Compare( a.center ) && extents.Compare( a.extents ) && axis.Compare( a.axis ) );
}

ARC_INLINE bool ARCBox::Compare( const ARCBox &a, const float epsilon ) const {
	return ( center.Compare( a.center, epsilon ) && extents.Compare( a.extents, epsilon ) && axis.Compare( a.axis, epsilon ) );
}

ARC_INLINE bool ARCBox::operator==( const ARCBox &a ) const {
	return Compare( a );
}

ARC_INLINE bool ARCBox::operator!=( const ARCBox &a ) const {
	return !Compare( a );
}

ARC_INLINE void ARCBox::Clear( void ) {
	center.Zero();
	extents[0] = extents[1] = extents[2] = -arcMath::INFINITY;
	axis.Identity();
}

ARC_INLINE void ARCBox::Zero( void ) {
	center.Zero();
	extents.Zero();
	axis.Identity();
}

ARC_INLINE const arcVec3 &ARCBox::GetCenter( void ) const {
	return center;
}

ARC_INLINE const arcVec3 &ARCBox::GetExtents( void ) const {
	return extents;
}

ARC_INLINE const arcMat3 &ARCBox::GetAxis( void ) const {
	return axis;
}

ARC_INLINE float ARCBox::GetVolume( void ) const {
	return ( extents * 2.0f ).LengthSqr();
}

ARC_INLINE bool ARCBox::IsCleared( void ) const {
	return extents[0] < 0.0f;
}

ARC_INLINE ARCBox ARCBox::Expand( const float d ) const {
	return ARCBox( center, extents + arcVec3( d, d, d ), axis );
}

ARC_INLINE ARCBox &ARCBox::ExpandSelf( const float d ) {
	extents[0] += d;
	extents[1] += d;
	extents[2] += d;
	return *this;
}

ARC_INLINE ARCBox ARCBox::Translate( const arcVec3 &translation ) const {
	return ARCBox( center + translation, extents, axis );
}

ARC_INLINE ARCBox &ARCBox::TranslateSelf( const arcVec3 &translation ) {
	center += translation;
	return *this;
}

ARC_INLINE ARCBox ARCBox::Rotate( const arcMat3 &rotation ) const {
	return ARCBox( center * rotation, extents, axis * rotation );
}

ARC_INLINE ARCBox &ARCBox::RotateSelf( const arcMat3 &rotation ) {
	center *= rotation;
	axis *= rotation;
	return *this;
}

ARC_INLINE bool ARCBox::ContainsPoint( const arcVec3 &p ) const {
	arcVec3 lp = p - center;
	if ( arcMath::Fabs( lp * axis[0] ) > extents[0] ||
			arcMath::Fabs( lp * axis[1] ) > extents[1] ||
				arcMath::Fabs( lp * axis[2] ) > extents[2] ) {
		return false;
	}
	return true;
}

ARC_INLINE ARCSphere ARCBox::ToSphere( void ) const {
	return ARCSphere( center, extents.Length() );
}

ARC_INLINE void ARCBox::AxisProjection( const arcVec3 &dir, float &min, float &max ) const {
	float d1 = dir * center;
	float d2 = arcMath::Fabs( extents[0] * ( dir * axis[0] ) ) +
				arcMath::Fabs( extents[1] * ( dir * axis[1] ) ) +
				arcMath::Fabs( extents[2] * ( dir * axis[2] ) );
	min = d1 - d2;
	max = d1 + d2;
}

ARC_INLINE void ARCBox::AxisProjection( const arcMat3 &ax, arcBounds &bounds ) const {
	for ( int i = 0; i < 3; i++ ) {
		float d1 = ax[i] * center;
		float d2 = arcMath::Fabs( extents[0] * ( ax[i] * axis[0] ) ) +
					arcMath::Fabs( extents[1] * ( ax[i] * axis[1] ) ) +
					arcMath::Fabs( extents[2] * ( ax[i] * axis[2] ) );
		bounds[0][i] = d1 - d2;
		bounds[1][i] = d1 + d2;
	}
}

#endif /* !__BV_BOX_H__ */

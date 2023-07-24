#ifndef __BV_FRUSTUM_H__
#define __BV_FRUSTUM_H__

/*
===============================================================================

	Orthogonal Frustum

===============================================================================
*/

class ARCFrustum {
public:
					ARCFrustum( void );

	void			SetOrigin( const arcVec3 &origin );
	void			SetAxis( const arcMat3 &axis );
	void			SetSize( float dNear, float dFar, float dLeft, float dUp );
	void			SetPyramid( float dNear, float dFar );
	void			MoveNearDistance( float dNear );
	void			MoveFarDistance( float dFar );

	const arcVec3 &	GetOrigin( void ) const;						// returns frustum origin
	const arcMat3 &	GetAxis( void ) const;							// returns frustum orientation
	arcVec3			GetCenter( void ) const;						// returns center of frustum

	bool			IsValid( void ) const;							// returns true if the frustum is valid
	float			GetNearDistance( void ) const;					// returns distance to near plane
	float			GetFarDistance( void ) const;					// returns distance to far plane
	float			GetLeft( void ) const;							// returns left vector length
	float			GetUp( void ) const;							// returns up vector length

	ARCFrustum		Expand( const float d ) const;					// returns frustum expanded in all directions with the given value
	ARCFrustum &	ExpandSelf( const float d );					// expands frustum in all directions with the given value
	ARCFrustum		Translate( const arcVec3 &translation ) const;	// returns translated frustum
	ARCFrustum &	TranslateSelf( const arcVec3 &translation );		// translates frustum
	ARCFrustum		Rotate( const arcMat3 &rotation ) const;			// returns rotated frustum
	ARCFrustum &	RotateSelf( const arcMat3 &rotation );			// rotates frustum

	float			PlaneDistance( const arcPlane &plane ) const;
	int				PlaneSide( const arcPlane &plane, const float epsilon = ON_EPSILON ) const;

					// fast culling but might not cull everything outside the frustum
	bool			CullPoint( const arcVec3 &point ) const;
	bool			CullBounds( const arcBounds &bounds ) const;
	bool			CullBox( const ARCBox &box ) const;
	bool			CullSphere( const ARCSphere &sphere ) const;
	bool			CullFrustum( const ARCFrustum &frustum ) const;
	bool			CullWinding( const class arcWinding &winding ) const;

					// exact intersection tests
	bool			ContainsPoint( const arcVec3 &point ) const;
	bool			IntersectsBounds( const arcBounds &bounds ) const;
	bool			IntersectsBox( const ARCBox &box ) const;
	bool			IntersectsSphere( const ARCSphere &sphere ) const;
	bool			IntersectsFrustum( const ARCFrustum &frustum ) const;
	bool			IntersectsWinding( const arcWinding &winding ) const;
	bool			LineIntersection( const arcVec3 &start, const arcVec3 &end ) const;
	bool			RayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale1, float &scale2 ) const;

					// returns true if the projection origin is far enough away from the bounding volume to create a valid frustum
	bool			FromProjection( const arcBounds &bounds, const arcVec3 &projectionOrigin, const float dFar );
	bool			FromProjection( const ARCBox &box, const arcVec3 &projectionOrigin, const float dFar );
	bool			FromProjection( const ARCSphere &sphere, const arcVec3 &projectionOrigin, const float dFar );

					// moves the far plane so it extends just beyond the bounding volume
	bool			ConstrainToBounds( const arcBounds &bounds );
	bool			ConstrainToBox( const ARCBox &box );
	bool			ConstrainToSphere( const ARCSphere &sphere );
	bool			ConstrainToFrustum( const ARCFrustum &frustum );

	void			ToPlanes( arcPlane planes[6] ) const;			// planes point outwards
	void			ToPoints( arcVec3 points[8] ) const;				// 8 corners of the frustum

					// calculates the projection of this frustum onto the given axis
	void			AxisProjection( const arcVec3 &dir, float &min, float &max ) const;
	void			AxisProjection( const arcMat3 &ax, arcBounds &bounds ) const;

					// calculates the bounds for the projection in this frustum
	bool			ProjectionBounds( const arcBounds &bounds, arcBounds &projectionBounds ) const;
	bool			ProjectionBounds( const ARCBox &box, arcBounds &projectionBounds ) const;
	bool			ProjectionBounds( const ARCSphere &sphere, arcBounds &projectionBounds ) const;
	bool			ProjectionBounds( const ARCFrustum &frustum, arcBounds &projectionBounds ) const;
	bool			ProjectionBounds( const arcWinding &winding, arcBounds &projectionBounds ) const;

					// calculates the bounds for the projection in this frustum of the given frustum clipped to the given box
	bool			ClippedProjectionBounds( const ARCFrustum &frustum, const ARCBox &clipBox, arcBounds &projectionBounds ) const;

private:
	arcVec3			origin;		// frustum origin
	arcMat3			axis;		// frustum orientation
	float			dNear;		// distance of near plane, dNear >= 0.0f
	float			dFar;		// distance of far plane, dFar > dNear
	float			dLeft;		// half the width at the far plane
	float			dUp;		// half the height at the far plane
	float			invFar;		// 1.0f / dFar

private:
	bool			CullLocalBox( const arcVec3 &localOrigin, const arcVec3 &extents, const arcMat3 &localAxis ) const;
	bool			CullLocalFrustum( const ARCFrustum &localFrustum, const arcVec3 indexPoints[8], const arcVec3 cornerVecs[4] ) const;
	bool			CullLocalWinding( const arcVec3 *points, const int numPoints, int *pointCull ) const;
	bool			BoundsCullLocalFrustum( const arcBounds &bounds, const ARCFrustum &localFrustum, const arcVec3 indexPoints[8], const arcVec3 cornerVecs[4] ) const;
	bool			LocalLineIntersection( const arcVec3 &start, const arcVec3 &end ) const;
	bool			LocalRayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale1, float &scale2 ) const;
	bool			LocalFrustumIntersectsFrustum( const arcVec3 points[8], const bool testFirstSide ) const;
	bool			LocalFrustumIntersectsBounds( const arcVec3 points[8], const arcBounds &bounds ) const;
	void			ToClippedPoints( const float fractions[4], arcVec3 points[8] ) const;
	void			ToIndexPoints( arcVec3 indexPoints[8] ) const;
	void			ToIndexPointsAndCornerVecs( arcVec3 indexPoints[8], arcVec3 cornerVecs[4] ) const;
	void			AxisProjection( const arcVec3 indexPoints[8], const arcVec3 cornerVecs[4], const arcVec3 &dir, float &min, float &max ) const;
	void			AddLocalLineToProjectionBoundsSetCull( const arcVec3 &start, const arcVec3 &end, int &startCull, int &endCull, arcBounds &bounds ) const;
	void			AddLocalLineToProjectionBoundsUseCull( const arcVec3 &start, const arcVec3 &end, int startCull, int endCull, arcBounds &bounds ) const;
	bool			AddLocalCapsToProjectionBounds( const arcVec3 endPoints[4], const int endPointCull[4], const arcVec3 &point, int pointCull, int pointClip, arcBounds &projectionBounds ) const;
	bool			BoundsRayIntersection( const arcBounds &bounds, const arcVec3 &start, const arcVec3 &dir, float &scale1, float &scale2 ) const;
	void			ClipFrustumToBox( const ARCBox &box, float clipFractions[4], int clipPlanes[4] ) const;
	bool			ClipLine( const arcVec3 localPoints[8], const arcVec3 points[8], int startIndex, int endIndex, arcVec3 &start, arcVec3 &end, int &startClip, int &endClip ) const;
};


ARC_INLINE ARCFrustum::ARCFrustum( void ) {
	dNear = dFar = 0.0f;
}

ARC_INLINE void ARCFrustum::SetOrigin( const arcVec3 &origin ) {
	this->origin = origin;
}

ARC_INLINE void ARCFrustum::SetAxis( const arcMat3 &axis ) {
	this->axis = axis;
}

ARC_INLINE void ARCFrustum::SetSize( float dNear, float dFar, float dLeft, float dUp ) {
	assert( dNear >= 0.0f && dFar > dNear && dLeft > 0.0f && dUp > 0.0f );
	this->dNear = dNear;
	this->dFar = dFar;
	this->dLeft = dLeft;
	this->dUp = dUp;
	this->invFar = 1.0f / dFar;
}

ARC_INLINE void ARCFrustum::SetPyramid( float dNear, float dFar ) {
	assert( dNear >= 0.0f && dFar > dNear );
	this->dNear = dNear;
	this->dFar = dFar;
	this->dLeft = dFar;
	this->dUp = dFar;
	this->invFar = 1.0f / dFar;
}

ARC_INLINE void ARCFrustum::MoveNearDistance( float dNear ) {
	assert( dNear >= 0.0f );
	this->dNear = dNear;
}

ARC_INLINE void ARCFrustum::MoveFarDistance( float dFar ) {
	assert( dFar > this->dNear );
	float scale = dFar / this->dFar;
	this->dFar = dFar;
	this->dLeft *= scale;
	this->dUp *= scale;
	this->invFar = 1.0f / dFar;
}

ARC_INLINE const arcVec3 &ARCFrustum::GetOrigin( void ) const {
	return origin;
}

ARC_INLINE const arcMat3 &ARCFrustum::GetAxis( void ) const {
	return axis;
}

ARC_INLINE arcVec3 ARCFrustum::GetCenter( void ) const {
	return ( origin + axis[0] * ( ( dFar - dNear ) * 0.5f ) );
}

ARC_INLINE bool ARCFrustum::IsValid( void ) const {
	return ( dFar > dNear );
}

ARC_INLINE float ARCFrustum::GetNearDistance( void ) const {
	return dNear;
}

ARC_INLINE float ARCFrustum::GetFarDistance( void ) const {
	return dFar;
}

ARC_INLINE float ARCFrustum::GetLeft( void ) const {
	return dLeft;
}

ARC_INLINE float ARCFrustum::GetUp( void ) const {
	return dUp;
}

ARC_INLINE ARCFrustum ARCFrustum::Expand( const float d ) const {
	ARCFrustum f = *this;
	f.origin -= d * f.axis[0];
	f.dFar += 2.0f * d;
	f.dLeft = f.dFar * dLeft * invFar;
	f.dUp = f.dFar * dUp * invFar;
	f.invFar = 1.0f / dFar;
	return f;
}

ARC_INLINE ARCFrustum &ARCFrustum::ExpandSelf( const float d ) {
	origin -= d * axis[0];
	dFar += 2.0f * d;
	dLeft = dFar * dLeft * invFar;
	dUp = dFar * dUp * invFar;
	invFar = 1.0f / dFar;
	return *this;
}

ARC_INLINE ARCFrustum ARCFrustum::Translate( const arcVec3 &translation ) const {
	ARCFrustum f = *this;
	f.origin += translation;
	return f;
}

ARC_INLINE ARCFrustum &ARCFrustum::TranslateSelf( const arcVec3 &translation ) {
	origin += translation;
	return *this;
}

ARC_INLINE ARCFrustum ARCFrustum::Rotate( const arcMat3 &rotation ) const {
	ARCFrustum f = *this;
	f.axis *= rotation;
	return f;
}

ARC_INLINE ARCFrustum &ARCFrustum::RotateSelf( const arcMat3 &rotation ) {
	axis *= rotation;
	return *this;
}

#endif /* !__BV_FRUSTUM_H__ */

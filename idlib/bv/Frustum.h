#ifndef __BV_FRUSTUM_H__
#define __BV_FRUSTUM_H__

/*
===============================================================================

	Orthogonal Frustum

===============================================================================
*/

class anFrustum {
public:
					anFrustum( void );

	void			SetOrigin( const anVec3 &origin );
	void			SetAxis( const anMat3 &axis );
	void			SetSize( float dNear, float dFar, float dLeft, float dUp );
	void			SetPyramid( float dNear, float dFar );
	void			MoveNearDistance( float dNear );
	void			MoveFarDistance( float dFar );

	const anVec3 &	GetOrigin( void ) const;						// returns frustum origin
	const anMat3 &	GetAxis( void ) const;							// returns frustum orientation
	anVec3			GetCenter( void ) const;						// returns center of frustum

	bool			IsValid( void ) const;							// returns true if the frustum is valid
	float			GetNearDistance( void ) const;					// returns distance to near plane
	float			GetFarDistance( void ) const;					// returns distance to far plane
	float			GetLeft( void ) const;							// returns left vector length
	float			GetUp( void ) const;							// returns up vector length

	anFrustum		Expand( const float d ) const;					// returns frustum expanded in all directions with the given value
	anFrustum &	ExpandSelf( const float d );					// expands frustum in all directions with the given value
	anFrustum		Translate( const anVec3 &translation ) const;	// returns translated frustum
	anFrustum &	TranslateSelf( const anVec3 &translation );		// translates frustum
	anFrustum		Rotate( const anMat3 &rotation ) const;			// returns rotated frustum
	anFrustum &	RotateSelf( const anMat3 &rotation );			// rotates frustum

	float			PlaneDistance( const anPlane &plane ) const;
	int				PlaneSide( const anPlane &plane, const float epsilon = ON_EPSILON ) const;

					// fast culling but might not cull everything outside the frustum
	bool			CullPoint( const anVec3 &point ) const;
	bool			CullBounds( const anBounds &bounds ) const;
	bool			CullBox( const anBox &box ) const;
	bool			CullSphere( const anSphere &sphere ) const;
	bool			CullFrustum( const anFrustum &frustum ) const;
	bool			CullWinding( const class anWinding &winding ) const;

					// exact intersection tests
	bool			ContainsPoint( const anVec3 &point ) const;
	bool			IntersectsBounds( const anBounds &bounds ) const;
	bool			IntersectsBox( const anBox &box ) const;
	bool			IntersectsSphere( const anSphere &sphere ) const;
	bool			IntersectsFrustum( const anFrustum &frustum ) const;
	bool			IntersectsWinding( const anWinding &winding ) const;
	bool			LineIntersection( const anVec3 &start, const anVec3 &end ) const;
	bool			RayIntersection( const anVec3 &start, const anVec3 &dir, float &scale1, float &scale2 ) const;

					// returns true if the projection origin is far enough away from the bounding volume to create a valid frustum
	bool			FromProjection( const anBounds &bounds, const anVec3 &projectionOrigin, const float dFar );
	bool			FromProjection( const anBox &box, const anVec3 &projectionOrigin, const float dFar );
	bool			FromProjection( const anSphere &sphere, const anVec3 &projectionOrigin, const float dFar );

					// moves the far plane so it extends just beyond the bounding volume
	bool			ConstrainToBounds( const anBounds &bounds );
	bool			ConstrainToBox( const anBox &box );
	bool			ConstrainToSphere( const anSphere &sphere );
	bool			ConstrainToFrustum( const anFrustum &frustum );

	void			ToPlanes( anPlane planes[6] ) const;			// planes point outwards
	void			ToPoints( anVec3 points[8] ) const;				// 8 corners of the frustum

					// calculates the projection of this frustum onto the given axis
	void			AxisProjection( const anVec3 &dir, float &min, float &max ) const;
	void			AxisProjection( const anMat3 &ax, anBounds &bounds ) const;

					// calculates the bounds for the projection in this frustum
	bool			ProjectionBounds( const anBounds &bounds, anBounds &projectionBounds ) const;
	bool			ProjectionBounds( const anBox &box, anBounds &projectionBounds ) const;
	bool			ProjectionBounds( const anSphere &sphere, anBounds &projectionBounds ) const;
	bool			ProjectionBounds( const anFrustum &frustum, anBounds &projectionBounds ) const;
	bool			ProjectionBounds( const anWinding &winding, anBounds &projectionBounds ) const;

					// calculates the bounds for the projection in this frustum of the given frustum clipped to the given box
	bool			ClippedProjectionBounds( const anFrustum &frustum, const anBox &clipBox, anBounds &projectionBounds ) const;

private:
	anVec3			origin;		// frustum origin
	anMat3			axis;		// frustum orientation
	float			dNear;		// distance of near plane, dNear >= 0.0f
	float			dFar;		// distance of far plane, dFar > dNear
	float			dLeft;		// half the width at the far plane
	float			dUp;		// half the height at the far plane
	float			invFar;		// 1.0f / dFar

private:
	bool			CullLocalBox( const anVec3 &localOrigin, const anVec3 &extents, const anMat3 &localAxis ) const;
	bool			CullLocalFrustum( const anFrustum &localFrustum, const anVec3 indexPoints[8], const anVec3 cornerVecs[4] ) const;
	bool			CullLocalWinding( const anVec3 *points, const int numPoints, int *pointCull ) const;
	bool			BoundsCullLocalFrustum( const anBounds &bounds, const anFrustum &localFrustum, const anVec3 indexPoints[8], const anVec3 cornerVecs[4] ) const;
	bool			LocalLineIntersection( const anVec3 &start, const anVec3 &end ) const;
	bool			LocalRayIntersection( const anVec3 &start, const anVec3 &dir, float &scale1, float &scale2 ) const;
	bool			LocalFrustumIntersectsFrustum( const anVec3 points[8], const bool testFirstSide ) const;
	bool			LocalFrustumIntersectsBounds( const anVec3 points[8], const anBounds &bounds ) const;
	void			ToClippedPoints( const float fractions[4], anVec3 points[8] ) const;
	void			ToIndexPoints( anVec3 indexPoints[8] ) const;
	void			ToIndexPointsAndCornerVecs( anVec3 indexPoints[8], anVec3 cornerVecs[4] ) const;
	void			AxisProjection( const anVec3 indexPoints[8], const anVec3 cornerVecs[4], const anVec3 &dir, float &min, float &max ) const;
	void			AddLocalLineToProjectionBoundsSetCull( const anVec3 &start, const anVec3 &end, int &startCull, int &endCull, anBounds &bounds ) const;
	void			AddLocalLineToProjectionBoundsUseCull( const anVec3 &start, const anVec3 &end, int startCull, int endCull, anBounds &bounds ) const;
	bool			AddLocalCapsToProjectionBounds( const anVec3 endPoints[4], const int endPointCull[4], const anVec3 &point, int pointCull, int pointClip, anBounds &projectionBounds ) const;
	bool			BoundsRayIntersection( const anBounds &bounds, const anVec3 &start, const anVec3 &dir, float &scale1, float &scale2 ) const;
	void			ClipFrustumToBox( const anBox &box, float clipFractions[4], int clipPlanes[4] ) const;
	bool			ClipLine( const anVec3 localPoints[8], const anVec3 points[8], int startIndex, int endIndex, anVec3 &start, anVec3 &end, int &startClip, int &endClip ) const;
};

inline anFrustum::anFrustum( void ) {
	dNear = dFar = 0.0f;
}

inline void anFrustum::SetOrigin( const anVec3 &origin ) {
	this->origin = origin;
}

inline void anFrustum::SetAxis( const anMat3 &axis ) {
	this->axis = axis;
}

inline void anFrustum::SetSize( float dNear, float dFar, float dLeft, float dUp ) {
	assert( dNear >= 0.0f && dFar > dNear && dLeft > 0.0f && dUp > 0.0f );
	this->dNear = dNear;
	this->dFar = dFar;
	this->dLeft = dLeft;
	this->dUp = dUp;
	this->invFar = 1.0f / dFar;
}

inline void anFrustum::SetPyramid( float dNear, float dFar ) {
	assert( dNear >= 0.0f && dFar > dNear );
	this->dNear = dNear;
	this->dFar = dFar;
	this->dLeft = dFar;
	this->dUp = dFar;
	this->invFar = 1.0f / dFar;
}

inline void anFrustum::MoveNearDistance( float dNear ) {
	assert( dNear >= 0.0f );
	this->dNear = dNear;
}

inline void anFrustum::MoveFarDistance( float dFar ) {
	assert( dFar > this->dNear );
	float scale = dFar / this->dFar;
	this->dFar = dFar;
	this->dLeft *= scale;
	this->dUp *= scale;
	this->invFar = 1.0f / dFar;
}

inline const anVec3 &anFrustum::GetOrigin( void ) const {
	return origin;
}

inline const anMat3 &anFrustum::GetAxis( void ) const {
	return axis;
}

inline anVec3 anFrustum::GetCenter( void ) const {
	return ( origin + axis[0] * ( ( dFar - dNear ) * 0.5f ) );
}

inline bool anFrustum::IsValid( void ) const {
	return ( dFar > dNear );
}

inline float anFrustum::GetNearDistance( void ) const {
	return dNear;
}

inline float anFrustum::GetFarDistance( void ) const {
	return dFar;
}

inline float anFrustum::GetLeft( void ) const {
	return dLeft;
}

inline float anFrustum::GetUp( void ) const {
	return dUp;
}

inline anFrustum anFrustum::Expand( const float d ) const {
	anFrustum f = *this;
	f.origin -= d * f.axis[0];
	f.dFar += 2.0f * d;
	f.dLeft = f.dFar * dLeft * invFar;
	f.dUp = f.dFar * dUp * invFar;
	f.invFar = 1.0f / dFar;
	return f;
}

inline anFrustum &anFrustum::ExpandSelf( const float d ) {
	origin -= d * axis[0];
	dFar += 2.0f * d;
	dLeft = dFar * dLeft * invFar;
	dUp = dFar * dUp * invFar;
	invFar = 1.0f / dFar;
	return *this;
}

inline anFrustum anFrustum::Translate( const anVec3 &translation ) const {
	anFrustum f = *this;
	f.origin += translation;
	return f;
}

inline anFrustum &anFrustum::TranslateSelf( const anVec3 &translation ) {
	origin += translation;
	return *this;
}

inline anFrustum anFrustum::Rotate( const anMat3 &rotation ) const {
	anFrustum f = *this;
	f.axis *= rotation;
	return f;
}

inline anFrustum &anFrustum::RotateSelf( const anMat3 &rotation ) {
	axis *= rotation;
	return *this;
}

#endif // !__BV_FRUSTUM_H__
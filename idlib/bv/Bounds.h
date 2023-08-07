#ifndef __BV_BOUNDS_H__
#define __BV_BOUNDS_H__

/*
===============================================================================

        Axis Aligned Bounding Box

===============================================================================
*/

class anBounds {
public:
	anBounds( void );
	explicit anBounds( const anVec3 &mins, const anVec3 &maxs );
	explicit anBounds( const anVec3 &point );

	const anVec3 &			operator[]( const int index) const;
	anVec3 &				operator[]( const int index );
	anBounds 				operator+( const anVec3 &r ) const; // returns translated bounds
	anBounds &				operator+=( const anVec3 &r );     // translate the bounds
	anBounds 				operator*( const anMat3 &r ) const; // returns rotated bounds
	anBounds &				operator*=( const anMat3 &r );     // rotate the bounds
	anBounds 				operator+( const anBounds &a ) const;
	anBounds &				operator+=( const anBounds &a );
	anBounds 				operator-( const anBounds &a ) const;
	anBounds &				operator-=( const anBounds &a );

	bool 				Compare( const anBounds &a ) const; // exact compare, no epsilon
	bool 				Compare( const anBounds &a, const float epsilon) const;   // compare with epsilon
	bool 				operator==( const anBounds &a ) const; // exact compare, no epsilon
	bool 				operator!=( const anBounds &a ) const; // exact compare, no epsilon

	void 				Clear(); // inside out bounds
	void 				Zero();  // single point at origin

	//float				ShortestDistance( const anVec3 &point ) const;
	float 				ShortestDistance( const anVec3 &point ) const;
	float				GetMaxDistFromOrigin( void ) const;				// returns the maximum distance from the origin

	anVec3 				GetCenter() const; // returns center of bounds
	float 				GetRadius() const;   // returns the radius relative to the bounds origin
	float				GetRadius( const anVec3 &center ) const;               // returns the radius relative to the given center
	float 				GetVolume() const; // returns the volume of the bounds
	bool 				IsCleared() const;  // returns true if bounds are inside out

	void 				AddPointToBounds( const anVec3 &v, anVec3 &mins, anVec3 &maxs );

	bool 				AddPoint( const anVec3 &v ); // add the point, returns true if the bounds expanded
	bool 				AddBounds( const anBounds &a ); // add the bounds, returns true if the bounds expanded
	anBounds 			Intersect( const anBounds &a ) const; // return intersection of this bounds with the given bounds
	anBounds &			IntersectSelf( const anBounds &a );					// intersect this bounds with the given bounds
	anBounds 			Expand( const float d ) const;						// return bounds expanded in all directions with the given value
	anBounds &			ExpandSelf( const anVec3 &d );						// expand bounds in all directions by the given vector
	anBounds &			ExpandSelf( const float d );						// expand bounds in all directions with the given value
	anBounds 			Translate( const anVec3 &translation ) const;		// return translated bounds
	anBounds &			TranslateSelf( const anVec3 &translation );			// translate this bounds
	anBounds 			Rotate( const anMat3 &rotation ) const;				// return rotated bounds
	anBounds &			RotateSelf( const anMat3 &rotation );				// rotate this bounds

	anBounds &			DirectedExpandSelf( const abVec3 &d );				// expand bounds in direction of vec

	anVec3					Size( void ) const;
	bool					Contains( const anBounds &a ) const;
	int						GetLargestAxis( void ) const;

	float 					PlaneDistance( const anPlane &plane ) const;
	int 					PlaneSide( const anPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool 					ContainsPoint( const anVec3 &p ) const;      // includes touching
	bool 					IntersectsBounds( const anBounds &a ) const; // includes touching
	bool 					LineIntersection( const anVec3 &start, const anVec3 &end ) const;
	// intersection point is start + dir * scale
	bool 					RayIntersection( const anVec3 &start, const anVec3 &dir, float &scale ) const;

	// most tight bounds for the given transformed bounds
	void 					FromTransformedBounds( const anBounds &bounds, const anVec3 &origin, const anMat3 &axis );
	// most tight bounds for a point set
	void					FromPoints( const anVec3 *points, const int numPoints );
	// most tight bounds for a translation
	void 					FromPointTranslation( const anVec3 &point, const anVec3 &translation );
	void 					FromBoundsTranslation( const anBounds &bounds, const anVec3 &origin, const anMat3 &axis, const anVec3 &translation );
	// most tight bounds for a rotation
	void 					FromPointRotation( const anVec3 &point, const anRotation &rotation );
	void					FromBoundsRotation( const anBounds &bounds, const anVec3 &origin, const anMat3 &axis, const anRotation &rotation );

	void 					ToPoints( anVec3 points[8] ) const;
	anSphere 				ToSphere( void ) const;

	void 					AxisProjection( const anVec3 &dir, float &min, float &max ) const;
	void 					AxisProjection( const anVec3 &origin, const anMat3 &axis, const anVec3 &dir, float &min, float &max ) const;

	int 					GetDimension() const;

	void					ToPoints( anVec3 points[8] ) const;
	idSphere				ToSphere( void ) const;
	void					ToPlanes( idPlane planes[6] ) const;

	const float *			ToFloatPtr() const;
	float *					ToFloatPtr();
	const char *			ToString( int precision = 2 ) const;

	// Calculate the projected bounds.
	static void				ProjectedBounds( anBounds & projected, const anGLMatrix & mvp, const anBounds & bounds, bool windowSpace = true );
	static void				ProjectedNearClippedBounds( anBounds & projected, const anGLMatrix & mvp, const anBounds & bounds, bool windowSpace = true );
	static void				ProjectedFullyClippedBounds( anBounds & projected, const anGLMatrix & mvp, const anBounds & bounds, bool windowSpace = true );

	// Calculate the projected depth bounds.
	static void				DepthBoundsForBounds( float & min, float & max, const anGLMatrix & mvp, const anBounds & bounds, bool windowSpace = true );
	static void				DepthBoundsForExtrudedBounds( float & min, float & max, const anGLMatrix & mvp, const anBounds & bounds, const anVec3 & extrudeDirection, const anPlane & clipPlane, bool windowSpace = true );
	static void				DepthBoundsForShadowBounds( float & min, float & max, const anGLMatrix & mvp, const anBounds & bounds, const anVec3 & localLightOrigin, bool windowSpace = true );

	anBounds				ToMaya( void ) const;
	anBounds&				ToMayaSelf( void );
	anBounds				FromMaya( void ) const;
	anBounds&				FromMayaSelf( void );

	const anVec3 &			GetMins( void ) const;
	anVec3&					GetMins( void );
	const anVec3 &			GetMaxs( void ) const;
	anVec3 &				GetMaxs( void );

private:
	anVec3					b[2];
};

extern anBounds bounds_zero;
extern anBounds bounds_zeroOneCube;
extern anBounds bounds_unitCube;

inline anBounds::anBounds( void ) {}

inline anBounds::anBounds( const anVec3 &mins, const anVec3 &maxs ) {
  b[0] = mins;
  b[1] = maxs;
}

inline anBounds::anBounds( const anVec3 &point ) {
  b[0] = point;
  b[1] = point;
}

inline const anVec3 &anBounds::operator[]( const int index ) const {
  return b[index];
}

inline anVec3 &anBounds::operator[]( const int index) { return b[index]; }

inline anBounds anBounds::operator+( const anVec3 &r ) const {
  return anBounds( b[0] + t, b[1] + t );
}

inline anBounds &anBounds::operator+=( const anVec3 &r ) {
  b[0] += t;
  b[1] += t;
  return *this;
}

inline anBounds anBounds::operator*( const anMat3 &r ) const {
	anBounds bounds.FromTransformedBounds( *this, vec3_origin, r );
	return bounds;
}

inline anBounds &anBounds::operator*=( const anMat3 &r ) {
	this->FromTransformedBounds( *this, vec3_origin, r );
	return *this;
}

inline anBounds anBounds::operator+( const anBounds &a ) const {
	anBounds newBounds = *this;
	newBounds.AddBounds( a );
	return newBounds;
}

inline anBounds &anBounds::operator+=( const anBounds &a ) {
	anBounds::AddBounds( a );
	return *this;
}

inline anBounds anBounds::operator-( const anBounds &a ) const {
  assert( b[1][0] - b[0][0] > a.b[1][0] - a.b[0][0] &&
         b[1][1] - b[0][1] > a.b[1][1] - a.b[0][1] &&
         b[1][2] - b[0][2] > a.b[1][2] - a.b[0][2] );
  return anBounds(
      anVec3( b[0][0] + a.b[1][0], b[0][1] + a.b[1][1], b[0][2] + a.b[1][2] ),
      anVec3( b[1][0] + a.b[0][0], b[1][1] + a.b[0][1], b[1][2] + a.b[0][2] ) );
}

inline anBounds &anBounds::operator-=( const anBounds &a ) {
  assert( b[1][0] - b[0][0] > a.b[1][0] - a.b[0][0] &&
         b[1][1] - b[0][1] > a.b[1][1] - a.b[0][1] &&
         b[1][2] - b[0][2] > a.b[1][2] - a.b[0][2] );
  b[0] += a.b[1];
  b[1] += a.b[0];
  return *this;
}

inline bool anBounds::Compare( const anBounds &a ) const {
return ( b[0].Compare( a.b[0] ) && b[1].Compare( a.b[1] ) );
}

inline bool anBounds::Compare( const anBounds &a,
       const float epsilon) const {
return ( b[0].Compare( a.b[0], epsilon ) && b[1].Compare( a.b[1], epsilon ) );
}

inline bool anBounds::operator==( const anBounds &a ) const {
return Compare( a );
}

inline bool anBounds::operator!=( const anBounds &a ) const {
return !Compare( a );
}

inline void anBounds::Clear() {
b[0][0] = b[0][1] = b[0][2] = anMath::INFINITY;
b[1][0] = b[1][1] = b[1][2] = -anMath::INFINITY;
}

inline void ClearBounds( anVec3 &mins, anVec3 &maxs ) {
mins[0] = mins[1] = mins[2] = 99999;  //= anMath::INFINITY;
maxs[0] = maxs[1] = maxs[2] = -99999; //= -anMath::INFINITY;
}

inline void anBounds::Zero() {
b[0][0] = b[0][1] = b[0][2] = b[1][0] = b[1][1] = b[1][2] = 0;
}

inline anVec3 anBounds::GetCenter() const {
	return anVec3( ( b[1][0] + b[0][0] ) * 0.5f, ( b[1][1] + b[0][1] ) * 0.5f, ( b[1][2] + b[0][2] ) * 0.5f );
}

inline float anBounds::GetVolume() const {
	if ( b[0][0] >= b[1][0] || b[0][1] >= b[1][1] || b[0][2] >= b[1][2] ) {
		return 0.0f;
	}
	return ( ( b[1][0] - b[0][0] ) * ( b[1][1] - b[0][1] ) * ( b[1][2] - b[0][2] ) );
}

inline bool anBounds::IsCleared() const {
	return b[0][0] > b[1][0]; 
}

inline float anBounds::GetMaxDistFromOrigin( void ) const {
	float r1, r2;
	r1 = b[0].LengthSqr();
	r2 = b[1].LengthSqr();
	if ( r2 > r1 ) {
		r1 = r2;
	}
	return anMath::Sqrt( r1 );
}

ARCC_INLINE void anBounds::AddPointToBounds( const anVec3 &v, anVec3 &mins, anVec3 &maxs ) {
	for ( int i = 0; i < 3; i++ ) {
	float val = v[i];
	if ( val < mins[i] ) {
		mins[i] = val;
	}

	if ( val > maxs[i] ) {
		maxs[i] = val;
	}
	}
	}

	ARCC_INLINE bool anBounds::AddPoint( const anVec3 &v ) {
	bool expanded = false;
	if ( v[0] < b[0][0] ) {
		b[0][0] = v[0];
		expanded = true;
	}
	if ( v[0] > b[1][0] ) {
		b[1][0] = v[0];
		expanded = true;
	}
	if ( v[1] < b[0][1] ) {
	b[0][1] = v[1];
	expanded = true;
	}
	if ( v[1] > b[1][1] ) {
	b[1][1] = v[1];
	expanded = true;
	}
	if ( v[2] < b[0][2] ) {
	b[0][2] = v[2];
	expanded = true;
	}
	if ( v[2] > b[1][2] ) {
		b[1][2] = v[2];
		expanded = true;
	}
	return expanded;
	}

inline bool anBounds::AddBounds( const anBounds &a ) {
bool expanded = false;
if ( a.b[0][0] < b[0][0] ) {
b[0][0] = a.b[0][0];
expanded = true;
}
if ( a.b[0][1] < b[0][1] ) {
b[0][1] = a.b[0][1];
expanded = true;
}
if ( a.b[0][2] < b[0][2] ) {
b[0][2] = a.b[0][2];
expanded = true;
}
if ( a.b[1][0] > b[1][0] ) {
b[1][0] = a.b[1][0];
expanded = true;
}
if ( a.b[1][1] > b[1][1] ) {
b[1][1] = a.b[1][1];
expanded = true;
}
if ( a.b[1][2] > b[1][2] ) {
b[1][2] = a.b[1][2];
expanded = true;
}
return expanded;
}

inline anBounds anBounds::Intersect( const anBounds &a ) const {
anBounds n;
n.b[0][0] = ( a.b[0][0] > b[0][0] ) ? a.b[0][0] : b[0][0];
n.b[0][1] = ( a.b[0][1] > b[0][1] ) ? a.b[0][1] : b[0][1];
n.b[0][2] = ( a.b[0][2] > b[0][2] ) ? a.b[0][2] : b[0][2];
n.b[1][0] = ( a.b[1][0] < b[1][0] ) ? a.b[1][0] : b[1][0];
n.b[1][1] = ( a.b[1][1] < b[1][1] ) ? a.b[1][1] : b[1][1];
n.b[1][2] = ( a.b[1][2] < b[1][2] ) ? a.b[1][2] : b[1][2];
return n;
}

inline anBounds &anBounds::IntersectSelf( const anBounds &a ) {
	if ( a.b[0][0] > b[0][0] ) {
		b[0][0] = a.b[0][0];
	}
	if ( a.b[0][1] > b[0][1] ) {
		b[0][1] = a.b[0][1];
	}
	if ( a.b[0][2] > b[0][2] ) {
		b[0][2] = a.b[0][2];
	}
	if ( a.b[1][0] < b[1][0] ) {
		b[1][0] = a.b[1][0];
	}
	if ( a.b[1][1] < b[1][1] ) {
		b[1][1] = a.b[1][1];
	}
	if ( a.b[1][2] < b[1][2] ) {
		b[1][2] = a.b[1][2];
	}
	return *this;
}

inline anBounds anBounds::Expand( const float d) const {
	return anBounds( anVec3( b[0][0] - d, b[0][1] - d, b[0][2] - d ),
	anVec3( b[1][0] + d, b[1][1] + d, b[1][2] + d ) );
}

inline anBounds &anBounds::ExpandSelf( const float d) {
b[0][0] -= d;
b[0][1] -= d;
b[0][2] -= d;
b[1][0] += d;
b[1][1] += d;
b[1][2] += d;
return *this;
}

inline anBounds anBounds::Translate( const anVec3 &translation ) const {
return anBounds( b[0] + translation, b[1] + translation );
}

inline anBounds &anBounds::TranslateSelf( const anVec3 &translation ) {
	b[0] += translation;
	b[1] += translation;
	return *this;
}

inline anBounds anBounds::Rotate( const anMat3 &rotation ) const {
	anBounds bounds.FromTransformedBounds( *this, vec3_origin, rotation );
	return bounds;
}

inline anBounds &anBounds::RotateSelf( const anMat3 &rotation ) {
	FromTransformedBounds( *this, vec3_origin, rotation );
	return *this;
}

inline bool anBounds::ContainsPoint( const anVec3 &p ) const {
	if ( p[0] < b[0][0] || p[1] < b[0][1] || p[2] < b[0][2] || p[0] > b[1][0] ||
	p[1] > b[1][1] || p[2] > b[1][2] ) {
	return false;
	}
	return true;
}

inline bool anBounds::IntersectsBounds( const anBounds &a ) const {
if ( a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[1][2] < b[0][2] ||
a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] || a.b[0][2] > b[1][2] ) {
return false;
}
return true;
}

inline anSphere anBounds::ToSphere( void) const {
anSphere sphere.SetOrigin( ( b[0] + b[1] ) * 0.5f );
sphere.SetRadius( ( b[1] - sphere.GetOrigin()).Length() );
return sphere;
}

inline void anBounds::AxisProjection( const anVec3 &dir, float &min, float &max ) const {
	center, extents;

 	anVec3 center = ( b[0] + b[1] ) * 0.5f;
	anVec3 extents = b[1] - center;

	float d1 = dir * center;
	float d2 = anMath::Fabs( extents[0] * dir[0] ) +
				anMath::Fabs( extents[1] * dir[1] ) +
				anMath::Fabs( extents[2] * dir[2] );

	min = d1 - d2;
	max = d1 + d2;
}

inline void anBounds::AxisProjection( const anVec3 &origin, const anMat3 &axis, const anVec3 &dir, float &min, float &max ) const {
	anVec3 center = ( b[0] + b[1] ) * 0.5f;
	anVec3 extents = b[1] - center;
	center = origin + center * axis;

	float d1 = dir * center;
	float d2 = anMath::Fabs( extents[0] * ( dir * axis[0] ) ) +
             anMath::Fabs( extents[1] * ( dir * axis[1] ) ) +
             anMath::Fabs( extents[2] * ( dir * axis[2] ) );

	min = d1 - d2;
	max = d1 + d2;
}

inline int anBounds::GetDimension() const { return 6; }

inline const float *anBounds::ToFloatPtr() const { return &b[0].x; }

inline float *anBounds::ToFloatPtr() { return &b[0].x; }

#endif
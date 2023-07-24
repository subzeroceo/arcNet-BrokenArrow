
#ifndef __SURFACE_POLYTOPE_H__
#define __SURFACE_POLYTOPE_H__

/*
===============================================================================

	Polytope surface.

	NOTE: vertexes are not duplicated for texture coordinates.

===============================================================================
*/

class arcSurface_Polytope : public arcSurface {
public:
						arcSurface_Polytope( void );

	void				FromPlanes( const arcPlane *planes, const int numPlanes );

	void				SetupTetrahedron( const arcBounds &bounds );
	void				SetupHexahedron( const arcBounds &bounds );
	void				SetupOctahedron( const arcBounds &bounds );
	void				SetupDodecahedron( const arcBounds &bounds );
	void				SetupIcosahedron( const arcBounds &bounds );
	void				SetupCylinder( const arcBounds &bounds, const int numSides );
	void				SetupCone( const arcBounds &bounds, const int numSides );

	int					SplitPolytope( const arcPlane &plane, const float epsilon, arcSurface_Polytope **front, arcSurface_Polytope **back ) const;

protected:

};

/*
====================
arcSurface_Polytope::arcSurface_Polytope
====================
*/
ARC_INLINE arcSurface_Polytope::arcSurface_Polytope( void ) {
}

#endif /* !__SURFACE_POLYTOPE_H__ */

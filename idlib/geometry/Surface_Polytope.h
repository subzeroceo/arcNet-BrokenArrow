
#ifndef __SURFACE_POLYTOPE_H__
#define __SURFACE_POLYTOPE_H__

/*
===============================================================================

	Polytope surface.

	NOTE: vertexes are not duplicated for texture coordinates.

===============================================================================
*/

class anSurface_Polytope : public anSurface {
public:
						anSurface_Polytope( void );

	void				FromPlanes( const anPlane *planes, const int numPlanes );

	void				SetupTetrahedron( const anBounds &bounds );
	void				SetupHexahedron( const anBounds &bounds );
	void				SetupOctahedron( const anBounds &bounds );
	void				SetupDodecahedron( const anBounds &bounds );
	void				SetupIcosahedron( const anBounds &bounds );
	void				SetupCylinder( const anBounds &bounds, const int numSides );
	void				SetupCone( const anBounds &bounds, const int numSides );

	int					SplitPolytope( const anPlane &plane, const float epsilon, anSurface_Polytope **front, anSurface_Polytope **back ) const;

protected:

};

/*
====================
anSurface_Polytope::anSurface_Polytope
====================
*/
inline anSurface_Polytope::anSurface_Polytope( void ) {
}

#endif // !__SURFACE_POLYTOPE_H__

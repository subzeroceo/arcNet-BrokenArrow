#ifndef __SURFACE_PATCH_H__
#define __SURFACE_PATCH_H__

/*
===============================================================================

	Bezier patch surface.

===============================================================================
*/

class anSurface_Patch : public anSurface {

public:
						anSurface_Patch( void );
						anSurface_Patch( int maxPatchWidth, int maxPatchHeight );
						anSurface_Patch( const anSurface_Patch &patch );
						~anSurface_Patch( void );

	void				SetSize( int patchWidth, int patchHeight );
	int					GetWidth( void ) const;
	int					GetHeight( void ) const;

						// subdivide the patch mesh based on error
	void				Subdivide( float maxHorizontalError, float maxVerticalError, float maxLength, bool genNormals = false );
						// subdivide the patch up to an explicit number of horizontal and vertical subdivisions
	void				SubdivideExplicit( int horzSubdivisions, int vertSubdivisions, bool genNormals, bool removeLinear = false );

protected:
	int					width;			// width of patch
	int					height;			// height of patch
	int					maxWidth;		// maximum width allocated for
	int					maxHeight;		// maximum height allocated for
	bool				expanded;		// true if vertices are spaced out

private:
						// put the approximation points on the curve
	void				PutOnCurve( void );
						// remove columns and rows with all points on one line
	void				RemoveLinearColumnsRows( void );
						// resize verts buffer
	void				ResizeExpanded( int height, int width );
						// space points out over maxWidth * maxHeight buffer
	void				Expand( void );
						// move all points to the start of the verts buffer
	void				Collapse( void );
						// project a point onto a vector to calculate maximum curve error
	void				ProjectPointOntoVector( const anVec3 &point, const anVec3 &vStart, const anVec3 &vEnd, anVec3 &vProj );
						// generate normals
	void				GenerateNormals( void );
						// generate triangle indexes
	void				GenerateIndexes( void );
						// lerp point from two patch point
	void				LerpVert( const anDrawVertex &a, const anDrawVertex &b, anDrawVertex &out ) const;
						// sample a single 3x3 patch
	void				SampleSinglePatchPoint( const anDrawVertex ctrl[3][3], float u, float v, anDrawVertex *out ) const;
	void				SampleSinglePatch( const anDrawVertex ctrl[3][3], int baseCol, int baseRow, int width, int horzSub, int vertSub, anDrawVertex *outVerts ) const;
};

/*
=================
anSurface_Patch::anSurface_Patch
=================
*/
inline anSurface_Patch::anSurface_Patch( void ) {
	height = width = maxHeight = maxWidth = 0;
	expanded = false;
}

/*
=================
anSurface_Patch::anSurface_Patch
=================
*/
inline anSurface_Patch::anSurface_Patch( int maxPatchWidth, int maxPatchHeight ) {
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}

/*
=================
anSurface_Patch::anSurface_Patch
=================
*/
inline anSurface_Patch::anSurface_Patch( const anSurface_Patch &patch ) {
	(*this) = patch;
}

/*
=================
anSurface_Patch::~anSurface_Patch
=================
*/
inline anSurface_Patch::~anSurface_Patch() {
}

/*
=================
anSurface_Patch::GetWidth
=================
*/
inline int anSurface_Patch::GetWidth( void ) const {
	return width;
}

/*
=================
anSurface_Patch::GetHeight
=================
*/
inline int anSurface_Patch::GetHeight( void ) const {
	return height;
}

#endif /* !__SURFACE_PATCH_H__ */

#ifndef __SURFACE_PATCH_H__
#define __SURFACE_PATCH_H__

/*
===============================================================================

	Bezier patch surface.

===============================================================================
*/

class arcSurface_Patch : public arcSurface {

public:
						arcSurface_Patch( void );
						arcSurface_Patch( int maxPatchWidth, int maxPatchHeight );
						arcSurface_Patch( const arcSurface_Patch &patch );
						~arcSurface_Patch( void );

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
	void				ProjectPointOntoVector( const arcVec3 &point, const arcVec3 &vStart, const arcVec3 &vEnd, arcVec3 &vProj );
						// generate normals
	void				GenerateNormals( void );
						// generate triangle indexes
	void				GenerateIndexes( void );
						// lerp point from two patch point
	void				LerpVert( const arcDrawVert &a, const arcDrawVert &b, arcDrawVert &out ) const;
						// sample a single 3x3 patch
	void				SampleSinglePatchPoint( const arcDrawVert ctrl[3][3], float u, float v, arcDrawVert *out ) const;
	void				SampleSinglePatch( const arcDrawVert ctrl[3][3], int baseCol, int baseRow, int width, int horzSub, int vertSub, arcDrawVert *outVerts ) const;
};

/*
=================
arcSurface_Patch::arcSurface_Patch
=================
*/
ARC_INLINE arcSurface_Patch::arcSurface_Patch( void ) {
	height = width = maxHeight = maxWidth = 0;
	expanded = false;
}

/*
=================
arcSurface_Patch::arcSurface_Patch
=================
*/
ARC_INLINE arcSurface_Patch::arcSurface_Patch( int maxPatchWidth, int maxPatchHeight ) {
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}

/*
=================
arcSurface_Patch::arcSurface_Patch
=================
*/
ARC_INLINE arcSurface_Patch::arcSurface_Patch( const arcSurface_Patch &patch ) {
	(*this) = patch;
}

/*
=================
arcSurface_Patch::~arcSurface_Patch
=================
*/
ARC_INLINE arcSurface_Patch::~arcSurface_Patch() {
}

/*
=================
arcSurface_Patch::GetWidth
=================
*/
ARC_INLINE int arcSurface_Patch::GetWidth( void ) const {
	return width;
}

/*
=================
arcSurface_Patch::GetHeight
=================
*/
ARC_INLINE int arcSurface_Patch::GetHeight( void ) const {
	return height;
}

#endif /* !__SURFACE_PATCH_H__ */

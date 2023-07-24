
#ifndef __SURFACE_SWEPTSPLINE_H__
#define __SURFACE_SWEPTSPLINE_H__

/*
===============================================================================

	Swept Spline surface.

===============================================================================
*/

class arcSurface_SweptSpline : public arcSurface {
public:
							arcSurface_SweptSpline( void );
							~arcSurface_SweptSpline( void );

	void					SetSpline( tecKCurveSpline<arcVec4> *spline );
	void					SetSweptSpline( tecKCurveSpline<arcVec4> *sweptSpline );
	void					SetSweptCircle( const float radius );

	void					Tessellate( const int splineSubdivisions, const int sweptSplineSubdivisions );

	void					Clear( void );

protected:
	tecKCurveSpline<arcVec4> *spline;
	tecKCurveSpline<arcVec4> *sweptSpline;

	void					GetFrame( const arcMat3 &previousFrame, const arcVec3 dir, arcMat3 &newFrame );
};

/*
====================
arcSurface_SweptSpline::arcSurface_SweptSpline
====================
*/
ARC_INLINE arcSurface_SweptSpline::arcSurface_SweptSpline( void ) {
	spline = NULL;
	sweptSpline = NULL;
}

/*
====================
arcSurface_SweptSpline::~arcSurface_SweptSpline
====================
*/
ARC_INLINE arcSurface_SweptSpline::~arcSurface_SweptSpline( void ) {
	delete spline;
	delete sweptSpline;
}

/*
====================
arcSurface_SweptSpline::Clear
====================
*/
ARC_INLINE void arcSurface_SweptSpline::Clear( void ) {
	arcSurface::Clear();
	delete spline;
	spline = NULL;
	delete sweptSpline;
	sweptSpline = NULL;
}

#endif /* !__SURFACE_SWEPTSPLINE_H__ */

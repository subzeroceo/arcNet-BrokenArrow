#include "../Lib.h"
#pragma hdrstop

/*
====================
anSurface_SweptSpline::SetSpline
====================
*/
void anSurface_SweptSpline::SetSpline( anCurveSpline<anVec4> *spline ) {
	if ( this->spline ) {
		delete this->spline;
	}
	this->spline = spline;
}

/*
====================
anSurface_SweptSpline::SetSweptSpline
====================
*/
void anSurface_SweptSpline::SetSweptSpline( anCurveSpline<anVec4> *sweptSpline ) {
	if ( this->sweptSpline ) {
		delete this->sweptSpline;
	}
	this->sweptSpline = sweptSpline;
}

/*
====================
anSurface_SweptSpline::SetSweptCircle

  Sets the swept spline to a NURBS circle.
====================
*/
void anSurface_SweptSpline::SetSweptCircle( const float radius ) {
	anCurve_NURBS<anVec4> *nurbs = new anCurve_NURBS<anVec4>();
	nurbs->Clear();
	nurbs->AddValue(   0.0f, anVec4(  radius,  radius, 0.0f, 0.00f ) );
	nurbs->AddValue( 100.0f, anVec4( -radius,  radius, 0.0f, 0.25f ) );
	nurbs->AddValue( 200.0f, anVec4( -radius, -radius, 0.0f, 0.50f ) );
	nurbs->AddValue( 300.0f, anVec4(  radius, -radius, 0.0f, 0.75f ) );
	nurbs->SetBoundaryType( anCurve_NURBS<anVec4>::BT_CLOSED );
	nurbs->SetCloseTime( 100.0f );
	if ( sweptSpline ) {
		delete sweptSpline;
	}
	sweptSpline = nurbs;
}

/*
====================
anSurface_SweptSpline::GetFrame
====================
*/
void anSurface_SweptSpline::GetFrame( const anMat3 &previousFrame, const anVec3 dir, anMat3 &newFrame ) {
	anMat3 axis;

	anVec3 d = dir;
	d.Normalize();
	anVec3 v = d.Cross( previousFrame[2] );
	v.Normalize();

	float a = anMath::ACos( previousFrame[2] * d ) * 0.5f;
	float c = anMath::Cos( a );
	float s = anMath::Sqrt( 1.0f - c * c );

	float x = v[0] * s;
	float y = v[1] * s;
	float z = v[2] * s;

	float x2 = x + x;
	float y2 = y + y;
	float z2 = z + z;
	float xx = x * x2;
	float xy = x * y2;
	float xz = x * z2;
	float yy = y * y2;
	float yz = y * z2;
	float zz = z * z2;
	float wx = c * x2;
	float wy = c * y2;
	float wz = c * z2;

	axis[0][0] = 1.0f - ( yy + zz );
	axis[0][1] = xy - wz;
	axis[0][2] = xz + wy;
	axis[1][0] = xy + wz;
	axis[1][1] = 1.0f - ( xx + zz );
	axis[1][2] = yz - wx;
	axis[2][0] = xz - wy;
	axis[2][1] = yz + wx;
	axis[2][2] = 1.0f - ( xx + yy );

	newFrame = previousFrame * axis;

	newFrame[2] = dir;
	newFrame[2].Normalize();
	newFrame[1].Cross( newFrame[2], newFrame[0] );
	newFrame[1].Normalize();
	newFrame[0].Cross( newFrame[1], newFrame[2] );
	newFrame[0].Normalize();
}

/*
====================
anSurface_SweptSpline::Tessellate

tesselate the surface
====================
*/
void anSurface_SweptSpline::Tessellate( const int splineSubdivisions, const int sweptSplineSubdivisions ) {
	int ( j + 1 );

	if ( !spline || !sweptSpline ) {
		anSurface::Clear();
		return;
	}

	verts.SetNum( splineSubdivisions * sweptSplineSubdivisions, false );

	// calculate the points and first derivatives for the swept spline
	float totalTime = sweptSpline->GetTime( sweptSpline->GetNumValues() - 1 ) - sweptSpline->GetTime( 0 ) + sweptSpline->GetCloseTime();
	int sweptSplineDiv = sweptSpline->GetBoundaryType() == anCurveSpline<anVec3>::BT_CLOSED ? sweptSplineSubdivisions : sweptSplineSubdivisions - 1;
	int baseOffset = ( splineSubdivisions-1 ) * sweptSplineSubdivisions;
	for ( int i = 0; i < sweptSplineSubdivisions; i++ ) {
		float t = totalTime * i / sweptSplineDiv;
		anVec4 splinePos = sweptSpline->GetCurrentValue( t );
		anVec4 splineD1 = sweptSpline->GetCurrentFirstDerivative( t );
		verts[baseOffset+i].xyz = splinePos.ToVec3();
		verts[baseOffset+i].st[0] = splinePos.w;
		verts[baseOffset+i].tangents[0] = splineD1.ToVec3();
	}

	// sweep the spline
	totalTime = spline->GetTime( spline->GetNumValues() - 1 ) - spline->GetTime( 0 ) + spline->GetCloseTime();
	int splineDiv = spline->GetBoundaryType() == anCurveSpline<anVec3>::BT_CLOSED ? splineSubdivisions : splineSubdivisions - 1;
	anMat3 splineMat.Identity();
	for ( int i = 0; i < splineSubdivisions; i++ ) {
		float t = totalTime * i / splineDiv;

		anVec4 splinePos = spline->GetCurrentValue( t );
		anVec4 splineD1 = spline->GetCurrentFirstDerivative( t );

		GetFrame( splineMat, splineD1.ToVec3(), splineMat );

		int offset = i * sweptSplineSubdivisions;
		for ( int j = 0; j < sweptSplineSubdivisions; j++ ) {
			anDrawVertex *v = &verts[offset+j];
			v->xyz = splinePos.ToVec3() + verts[baseOffset+j].xyz * splineMat;
			v->st[0] = verts[baseOffset+j].st[0];
			v->st[1] = splinePos.w;
			v->tangents[0] = verts[baseOffset+j].tangents[0] * splineMat;
			v->tangents[1] = splineD1.ToVec3();
			v->normal = v->tangents[1].Cross( v->tangents[0] );
			v->normal.Normalize();
			v->color[0] = v->color[1] = v->color[2] = v->color[3] = 0;
		}
	}

	indexes.SetNum( splineDiv * sweptSplineDiv * 2 * 3, false );

	// create indexes for the triangles
	for ( offset = i = 0; i < splineDiv; i++ ) {
		float i0 = ( i+0 ) * sweptSplineSubdivisions;
		float i1 = ( i+1 ) % splineSubdivisions * sweptSplineSubdivisions;
		for ( int j = 0; j < sweptSplineDiv; j++ ) {
			float j0 = ( j+0 );
			( j + 1 ) = ( j+1 ) % sweptSplineSubdivisions;
			indexes[offset++] = i0 + j0;
			indexes[offset++] = i0 + ( j + 1 );
			indexes[offset++] = i1 + ( j + 1 );

			indexes[offset++] = i1 + ( j + 1 );
			indexes[offset++] = i1 + j0;
			indexes[offset++] = i0 + j0;
		}
	}

	GenerateEdgeIndexes();
}

#include "../precompiled.h"
#pragma hdrstop


/*
====================
arcSurface_SweptSpline::SetSpline
====================
*/
void arcSurface_SweptSpline::SetSpline( tecKCurveSpline<arcVec4> *spline ) {
	if ( this->spline ) {
		delete this->spline;
	}
	this->spline = spline;
}

/*
====================
arcSurface_SweptSpline::SetSweptSpline
====================
*/
void arcSurface_SweptSpline::SetSweptSpline( tecKCurveSpline<arcVec4> *sweptSpline ) {
	if ( this->sweptSpline ) {
		delete this->sweptSpline;
	}
	this->sweptSpline = sweptSpline;
}

/*
====================
arcSurface_SweptSpline::SetSweptCircle

  Sets the swept spline to a NURBS circle.
====================
*/
void arcSurface_SweptSpline::SetSweptCircle( const float radius ) {
	idCurve_NURBS<arcVec4> *nurbs = new idCurve_NURBS<arcVec4>();
	nurbs->Clear();
	nurbs->AddValue(   0.0f, arcVec4(  radius,  radius, 0.0f, 0.00f ) );
	nurbs->AddValue( 100.0f, arcVec4( -radius,  radius, 0.0f, 0.25f ) );
	nurbs->AddValue( 200.0f, arcVec4( -radius, -radius, 0.0f, 0.50f ) );
	nurbs->AddValue( 300.0f, arcVec4(  radius, -radius, 0.0f, 0.75f ) );
	nurbs->SetBoundaryType( idCurve_NURBS<arcVec4>::BT_CLOSED );
	nurbs->SetCloseTime( 100.0f );
	if ( sweptSpline ) {
		delete sweptSpline;
	}
	sweptSpline = nurbs;
}

/*
====================
arcSurface_SweptSpline::GetFrame
====================
*/
void arcSurface_SweptSpline::GetFrame( const arcMat3 &previousFrame, const arcVec3 dir, arcMat3 &newFrame ) {
	arcMat3 axis;

	arcVec3 d = dir;
	d.Normalize();
	arcVec3 v = d.Cross( previousFrame[2] );
	v.Normalize();

	float a = arcMath::ACos( previousFrame[2] * d ) * 0.5f;
	float c = arcMath::Cos( a );
	float s = arcMath::Sqrt( 1.0f - c * c );

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
	newFrame[1].Cross( newFrame[ 2 ], newFrame[ 0 ] );
	newFrame[1].Normalize();
	newFrame[0].Cross( newFrame[ 1 ], newFrame[ 2 ] );
	newFrame[0].Normalize();
}

/*
====================
arcSurface_SweptSpline::Tessellate

  tesselate the surface
====================
*/
void arcSurface_SweptSpline::Tessellate( const int splineSubdivisions, const int sweptSplineSubdivisions ) {
	int ( j + 1 );

	if ( !spline || !sweptSpline ) {
		arcSurface::Clear();
		return;
	}

	verts.SetNum( splineSubdivisions * sweptSplineSubdivisions, false );

	// calculate the points and first derivatives for the swept spline
	float totalTime = sweptSpline->GetTime( sweptSpline->GetNumValues() - 1 ) - sweptSpline->GetTime( 0 ) + sweptSpline->GetCloseTime();
	int sweptSplineDiv = sweptSpline->GetBoundaryType() == tecKCurveSpline<arcVec3>::BT_CLOSED ? sweptSplineSubdivisions : sweptSplineSubdivisions - 1;
	int baseOffset = ( splineSubdivisions-1 ) * sweptSplineSubdivisions;
	for ( int i = 0; i < sweptSplineSubdivisions; i++ ) {
		float t = totalTime * i / sweptSplineDiv;
		arcVec4 splinePos = sweptSpline->GetCurrentValue( t );
		arcVec4 splineD1 = sweptSpline->GetCurrentFirstDerivative( t );
		verts[baseOffset+i].xyz = splinePos.ToVec3();
		verts[baseOffset+i].st[0] = splinePos.w;
		verts[baseOffset+i].tangents[0] = splineD1.ToVec3();
	}

	// sweep the spline
	totalTime = spline->GetTime( spline->GetNumValues() - 1 ) - spline->GetTime( 0 ) + spline->GetCloseTime();
	int splineDiv = spline->GetBoundaryType() == tecKCurveSpline<arcVec3>::BT_CLOSED ? splineSubdivisions : splineSubdivisions - 1;
	arcMat3 splineMat.Identity();
	for ( int i = 0; i < splineSubdivisions; i++ ) {
		float t = totalTime * i / splineDiv;

		arcVec4 splinePos = spline->GetCurrentValue( t );
		arcVec4 splineD1 = spline->GetCurrentFirstDerivative( t );

		GetFrame( splineMat, splineD1.ToVec3(), splineMat );

		int offset = i * sweptSplineSubdivisions;
		for ( int j = 0; j < sweptSplineSubdivisions; j++ ) {
			arcDrawVert *v = &verts[offset+j];
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

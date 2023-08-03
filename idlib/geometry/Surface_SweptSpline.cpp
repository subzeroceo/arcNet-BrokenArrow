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
	newFrame[1].Cross( newFrame[ 2 ], newFrame[ 0 ] );
	newFrame[1].Normalize();
	newFrame[0].Cross( newFrame[ 1 ], newFrame[ 2 ] );
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

abVec3 anSplineList::zero( 0, 0, 0 );
void splineTest() {
	//g_splineList->load( "p:/doom/base/maps/test_base1.camera" );
}

void splineDraw() {
	//g_splineList->addToRenderer();
}

//const char *idCameraPosition::positionStr[] = {
	//"Fixed",
	//"Interpolated",
	//"Spline",
//};

void anSplineList::addToRenderer() {

	if (controlPoints.Num() == 0) {
		return;
	}

	anVec3_t mins, maxs;
	anVec3_t yellow(1.0, 1.0, 0);
	anVec3_t white(1.0, 1.0, 1.0);
        int i;

	for ( i = 0; i < controlPoints.Num(); i++ ) {
		VectorCopy(*controlPoints[i], mins);
		VectorCopy(mins, maxs);
		mins[0] -= 8;
		mins[1] += 8;
		mins[2] -= 8;
		maxs[0] += 8;
		maxs[1] -= 8;
		maxs[2] += 8;
		debugLine( yellow, mins[0], mins[1], mins[2], maxs[0], mins[1], mins[2]);
		debugLine( yellow, maxs[0], mins[1], mins[2], maxs[0], maxs[1], mins[2]);
		debugLine( yellow, maxs[0], maxs[1], mins[2], mins[0], maxs[1], mins[2]);
		debugLine( yellow, mins[0], maxs[1], mins[2], mins[0], mins[1], mins[2]);

		debugLine( yellow, mins[0], mins[1], maxs[2], maxs[0], mins[1], maxs[2]);
		debugLine( yellow, maxs[0], mins[1], maxs[2], maxs[0], maxs[1], maxs[2]);
		debugLine( yellow, maxs[0], maxs[1], maxs[2], mins[0], maxs[1], maxs[2]);
		debugLine( yellow, mins[0], maxs[1], maxs[2], mins[0], mins[1], maxs[2]);

	}

	int step = 0;
	anVec3_t step1;
	for ( i = 3; i < controlPoints.Num(); i++ ) {
		for (float tension = 0.0f; tension < 1.001f; tension += 0.1f) {
			float x = 0;
			float y = 0;
			float z = 0;
			for ( intj = 0; j < 4; j++ ) {
				x += controlPoints[i - (3 - j)]->x * calcSpline(j, tension);
				y += controlPoints[i - (3 - j)]->y * calcSpline(j, tension);
				z += controlPoints[i - (3 - j)]->z * calcSpline(j, tension);
			}
			if ( step == 0) {
				step1[0] = x;
				step1[1] = y;
				step1[2] = z;
				step = 1;
			} else {
				debugLine( white, step1[0], step1[1], step1[2], x, y, z);
				step = 0;
			}

		}
	}
}

void anSplineList::buildSpline() {
	//int start = Sys_Milliseconds();
	clearSpline();
	for ( int i = 3; i < controlPoints.Num(); i++ ) {
		for (float tension = 0.0f; tension < 1.001f; tension += granularity) {
			float x = 0;
			float y = 0;
			float z = 0;
			for ( intj = 0; j < 4; j++ ) {
				x += controlPoints[i - (3 - j)]->x * calcSpline(j, tension);
				y += controlPoints[i - (3 - j)]->y * calcSpline(j, tension);
				z += controlPoints[i - (3 - j)]->z * calcSpline(j, tension);
			}
			splinePoints.Append(new anVec3_t(x, y, z));
		}
	}
	dirty = false;
	//Com_Printf( "Spline build took %f seconds\n", ( float )(Sys_Milliseconds() - start) / 1000);
}


void anSplineList::draw(bool editMode) {
	int i;
	vec4_t yellow(1, 1, 0, 1);

	if (controlPoints.Num() == 0) {
		return;
	}

	if (dirty) {
		buildSpline();
	}


	qglColor3fv(controlColor);
	qglPointSize(5);

	qglBegin(GL_POINTS);
	for ( i = 0; i < controlPoints.Num(); i++ ) {
		qglVertex3fv(*controlPoints[i]);
	}
	qglEnd();

	if (editMode) {
		for ( i = 0; i < controlPoints.Num(); i++ ) {
			glBox(activeColor, *controlPoints[i], 4);
		}
	}

	//Draw the curve
	qglColor3fv(pathColor);
	qglBegin(GL_LINE_STRIP);
	int count = splinePoints.Num();
	for ( i = 0; i < count; i++ ) {
		qglVertex3fv(*splinePoints[i]);
	}
	qglEnd();

	if (editMode) {
		qglColor3fv( segmentColor);
		qglPointSize(3);
		qglBegin(GL_POINTS);
		for ( i = 0; i < count; i++ ) {
			qglVertex3fv(*splinePoints[i]);
		}
		qglEnd();
	}
	if (count > 0) {
		//assert(activeSegment >=0 && activeSegment < count);
		if (activeSegment >=0 && activeSegment < count) {
			glBox(activeColor, *splinePoints[activeSegment], 6);
			glBox(yellow, *splinePoints[activeSegment], 8);
		}
	}

}

float anSplineList::totalDistance() {

	if (controlPoints.Num() == 0) {
		return 0.0;
	}

	if (dirty) {
		buildSpline();
	}

	float dist = 0.0;
	anVec3_t temp;
	int count = splinePoints.Num();
	for ( int i = 1; i < count; i++ ) {
		temp = *splinePoints[i-1];
		temp -= *splinePoints[i];
		dist += temp.Length();
	}
	return dist;
}

void anSplineList::initPosition(long bt, long totalTime) {

	if (dirty) {
		buildSpline();
	}

	if ( splinePoints.Num() == 0) {
		return;
	}

	baseTime = bt;
	time = totalTime;

	// calc distance to travel ( this will soon be broken into time segments )
	splineTime.Clear();
	splineTime.Append(0);
	float dist = totalDistance();
	float distSoFar = 0.0;
	anVec3_t temp;
	int count = splinePoints.Num();
	//for ( int i = 2; i < count - 1; i++ ) {
	for ( int i = 1; i < count; i++ ) {
		temp = *splinePoints[i-1];
		temp -= *splinePoints[i];
		distSoFar += temp.Length();
		float percent = distSoFar / dist;
		percent *= totalTime;
		splineTime.Append(percent + bt);
	}
	assert( splineTime.Num() == splinePoints.Num());
	activeSegment = 0;
}



float anSplineList::calcSpline( intstep, float tension) {
	switch ( step) {
		case 0:	return (pow(1 - tension, 3)) / 6;
		case 1:	return (3 * pow(tension, 3) - 6 * pow(tension, 2) + 4) / 6;
		case 2:	return (-3 * pow(tension, 3) + 3 * pow(tension, 2) + 3 * tension + 1) / 6;
		case 3:	return pow(tension, 3) / 6;
	}
	return 0.0;
}

void anSplineList::updateSelection(const anVec3_t &move) {
	if ( selected) {
		dirty = true;
		VectorAdd(*selected, move, *selected);
	}
}


void anSplineList::setSelectedPoint(anVec3_t *p) {
	if (p) {
		p->Snap();
		for ( int i = 0; i < controlPoints.Num(); i++ ) {
			if (*p == *controlPoints[i]) {
				selected = controlPoints[i];
			}
		}
	} else {
		selected = nullptr;
	}
}
const anVec3_t *anSplineList::getPosition(long t) {
	static anVec3_t interpolatedPos;

	int count = splineTime.Num();
	if (count == 0) {
		return &zero;
	}

	assert( splineTime.Num() == splinePoints.Num());

	while (activeSegment < count) {
		if ( splineTime[activeSegment] >= t) {
			if (activeSegment > 0 && activeSegment < count - 1) {
				float timeHi = splineTime[activeSegment + 1];
				float timeLo = splineTime[activeSegment - 1];
				//float percent = ( float )(baseTime + time - t) / time;
				float percent = (timeHi - t) / (timeHi - timeLo);
				// pick two bounding points
				anVec3_t v1 = *splinePoints[activeSegment-1];
				anVec3_t v2 = *splinePoints[activeSegment+1];
				v2 *= (1.0 - percent);
				v1 *= percent;
				v2 += v1;
				interpolatedPos = v2;
				return &interpolatedPos;
			}
			return splinePoints[activeSegment];
		} else {
			activeSegment++;
		}
	}
	return splinePoints[count-1];
}

const anVec3 *anInterpolatedPosition::GetPosition( long t ) {
	static anVec3 interpolatedPos;

	float velocity = GetVelocity( t );
	float timePassed = t - lastTime;
	lastTime = t;

	// convert to seconds
	timePassed /= 1000;

	float distToTravel = timePassed *= velocity;

	anVec3 temp = startPos;
	temp -= endPos;
	float distance = temp.Length();

	distSoFar += distToTravel;
	float percent = ( float )( distSoFar ) / distance;

	if ( percent > 1.0f ) {
		percent = 1.0f;
	} else if ( percent < 0.0f ) {
		percent = 0.0f;
	}

	// the following line does a straigt calc on percentage of time
	// float percent = ( float )( startTime + time - t) / time;

	anVec3 v1 = startPos;
	anVec3 v2 = endPos;
	v1 *= ( 1.0f - percent );
	v2 *= percent;
	v1 += v2;
	interpolatedPos = v1;
	return &interpolatedPos;
}


void idInterpolatedPosition::parse(const char *(*text)  ) {
	const char *token;
	Com_MatchToken( text, "{" );
	do {
		token = Com_Parse( text );

		if ( !token[0] ) {
			break;
		}
		if ( !strcmp (token, "}" ) ) {
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do {
			// if token is not a brace, it is a key for a key/value pair
			if ( !token[0] || !strcmp (token, "( " ) || !strcmp(token, "}" ) ) {
				break;
			}

			Com_UngetToken();
			anString key = Com_ParseOnLine(text);

			const char *token = Com_Parse(text);
			if (Q_stricmp(key.c_str(), "startPos" ) == 0) {
				Com_UngetToken();
				Com_Parse1DMatrix( text, 3, startPos );
			} else if (Q_stricmp(key.c_str(), "endPos" ) == 0) {
				Com_UngetToken();
				Com_Parse1DMatrix( text, 3, endPos );
			} else {
				Com_UngetToken();
				idCameraPosition::parseToken(key.c_str(), text);
			}
			token = Com_Parse(text);

		} while ( 1 );

		if ( !strcmp (token, "}" ) ) {
			break;
		}

	} while ( 1 );

	Com_UngetToken();
	Com_MatchToken( text, "}" );
}


void idSplinePosition::parse(const char *(*text)  ) {
	const char *token;
	Com_MatchToken( text, "{" );
	do {
		token = Com_Parse( text );

		if ( !token[0] ) {
			break;
		}
		if ( !strcmp (token, "}" ) ) {
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do {
			// if token is not a brace, it is a key for a key/value pair
			if ( !token[0] || !strcmp (token, "( " ) || !strcmp(token, "}" ) ) {
				break;
			}

			Com_UngetToken();
			anString key = Com_ParseOnLine(text);

			const char *token = Com_Parse(text);
			if (Q_stricmp(key.c_str(), "target" ) == 0) {
				target.parse(text);
			} else {
				Com_UngetToken();
				idCameraPosition::parseToken(key.c_str(), text);
			}
			token = Com_Parse(text);

		} while ( 1 );

		if ( !strcmp (token, "}" ) ) {
			break;
		}

	} while ( 1 );

	Com_UngetToken();
	Com_MatchToken( text, "}" );
}

void idFixedPosition::write(fileHandle_t file, const char *p) {
	anString s = va( "\t%s {\n", p);
	FS_Write( s.c_str(), s.length(), file);
	idCameraPosition::write(file, p);
	s = va( "\t\tpos ( %f %f %f )\n", pos.x, pos.y, pos.z);
	FS_Write( s.c_str(), s.length(), file);
	s = "\t}\n";
	FS_Write( s.c_str(), s.length(), file);
}

void idInterpolatedPosition::write(fileHandle_t file, const char *p) {
	anString s = va( "\t%s {\n", p);
	FS_Write( s.c_str(), s.length(), file);
	idCameraPosition::write(file, p);
	s = va( "\t\tstartPos ( %f %f %f )\n", startPos.x, startPos.y, startPos.z);
	FS_Write( s.c_str(), s.length(), file);
	s = va( "\t\tendPos ( %f %f %f )\n", endPos.x, endPos.y, endPos.z);
	FS_Write( s.c_str(), s.length(), file);
	s = "\t}\n";
	FS_Write( s.c_str(), s.length(), file);
}

void idSplinePosition::write(fileHandle_t file, const char *p) {
	anString s = va( "\t%s {\n", p);
	FS_Write( s.c_str(), s.length(), file);
	idCameraPosition::write(file, p);
	target.write(file, "target" );
	s = "\t}\n";
	FS_Write( s.c_str(), s.length(), file);
}

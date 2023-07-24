#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "splines.h"

aRCCameraDef splineList;
aRCCameraDef *g_splineList = &splineList;

/*
================
glLabeledPoint
================
*/
void glLabeledPoint(arcVec4 &color, arcVec3 &point, float size, const char *label) {
	qglColor3fv( color.ToFloatPtr() );
	qglPointSize( size );
	qglBegin( GL_POINTS );
	qglVertex3fv( point.ToFloatPtr() );
	qglEnd();
	arcVec3 v = point;
	v.x += 1;
	v.y += 1;
	v.z += 1;
	qglRasterPos3fv( v.ToFloatPtr() );
	qglCallLists( strlen(label), GL_UNSIGNED_BYTE, label );
}

/*
================
QGLBox
================
*/
void QGLBox( arcVec4 &color, arcVec3 &point, float size ) {
	arcVec3 mins( point );
	arcVec3 maxs( point );
	mins[0] -= size;
	mins[1] += size;
	mins[2] -= size;
	maxs[0] += size;
	maxs[1] -= size;
	maxs[2] += size;
	arcVec4	saveColor;
	qglGetFloatv( GL_CURRENT_COLOR, saveColor.ToFloatPtr() );
	qglColor3fv( color.ToFloatPtr() );
	qglBegin( GL_LINE_LOOP );
	qglVertex3f( mins[0],mins[1],mins[2] );
	qglVertex3f(maxs[0],mins[1],mins[2] );
	qglVertex3f(maxs[0],maxs[1],mins[2] );
	qglVertex3f( mins[0],maxs[1],mins[2] );
	qglEnd();
	qglBegin( GL_LINE_LOOP );
	qglVertex3f( mins[0],mins[1],maxs[2] );
	qglVertex3f(maxs[0],mins[1],maxs[2] );
	qglVertex3f(maxs[0],maxs[1],maxs[2] );
	qglVertex3f( mins[0],maxs[1],maxs[2] );
	qglEnd();

	qglBegin( GL_LINES );
  	qglVertex3f( mins[0],mins[1],mins[2] );
	qglVertex3f( mins[0],mins[1],maxs[2] );
	qglVertex3f( mins[0],maxs[1],maxs[2] );
	qglVertex3f( mins[0],maxs[1],mins[2] );
	qglVertex3f(maxs[0],mins[1],mins[2] );
	qglVertex3f(maxs[0],mins[1],maxs[2] );
	qglVertex3f(maxs[0],maxs[1],maxs[2] );
	qglVertex3f(maxs[0],maxs[1],mins[2] );
	qglEnd();
	qglColor4fv( saveColor.ToFloatPtr() );
}

/*
================
splineTest
================
*/
void splineTest() {
	//g_splineList->load( "file://base/maps/test_base1.cam" );
}

/*
================
splineDraw
================
*/
void splineDraw() {
	//g_splineList->addToRenderer();
}

/*
================
debugLine
================
*/
void debugLine(arcVec4 &color, float x, float y, float z, float x2, float y2, float z2) {
	arcVec3 from( x, y, z );
	arcVec3 to( x2, y2, z2 );
	session->rw->DebugLine( color, from, to );
}


/*
=================================================================================

idPointListInterface

=================================================================================
*/

/*
================
idPointListInterface::selectPointByRay
================
*/
int idPointListInterface::selectPointByRay( const arcVec3 &origin, const arcVec3 &direction, bool single ) {
	// find the point closest to the ray
	float besti = -1;
	float bestd = 8;
	float count = numPoints();

	for ( int i = 0; i < count; i++ ) {
		arcVec3 temp = *getPoint( i );
		arcVec3 temp2 = temp;
		temp -= origin;
		float d = arcVec3::Dot( temp, direction );
		arcVec3::MA( origin, d, direction, temp );
		temp2 -= temp;
		d = temp2.Length();
		if ( d <= bestd ) {
			bestd = d;
			besti = i;
		}
	}

	if ( besti >= 0 ) {
		selectPoint( besti, single );
	}

	return besti;
}

/*
================
idPointListInterface::isPointSelected
================
*/
int idPointListInterface::isPointSelected( int index ) {
	int count = selectedPoints.Num();
	for ( int i = 0; i < count; i++ ) {
		if (selectedPoints[i] == index) {
			return i;
		}
	}
	return -1;
}

/*
================
idPointListInterface::selectPoint
================
*/
int idPointListInterface::selectPoint( int index, bool single) {
	if (index >= 0 && index < numPoints() ) {
		if (single) {
			deselectAll();
		} else {
			if (isPointSelected( index ) >= 0 ) {
				selectedPoints.Remove( index );
			}
		}
		return selectedPoints.Append( index );
	}
	return -1;
}

/*
================
idPointListInterface::selectAll
================
*/
void idPointListInterface::selectAll() {
	selectedPoints.Clear();
	for ( int i = 0; i < numPoints(); i++ ) {
		selectedPoints.Append( i );
	}
}

/*
================
idPointListInterface::deselectAll
================
*/
void idPointListInterface::deselectAll() {
	selectedPoints.Clear();
}

/*
================
idPointListInterface::getSelectedPoint
================
*/
arcVec3 *idPointListInterface::getSelectedPoint( int index ) {
	assert(index >= 0 && index < numSelectedPoints() );
	return getPoint(selectedPoints[index] );
}

/*
================
idPointListInterface::UpdateSelection
================
*/
void idPointListInterface::UpdateSelection(const arcVec3 &move) {
	int count = selectedPoints.Num();
	for ( int i = 0; i < count; i++ ) {
		*getPoint(selectedPoints[i] ) += move;
	}
}

/*
================
idPointListInterface::drawSelection
================
*/
void idPointListInterface::drawSelection() {
	int count = selectedPoints.Num();
	for ( int i = 0; i < count; i++ ) {
		QGLBox(colorRed, *getPoint(selectedPoints[i] ), 4);
	}
}

/*
=================================================================================

idSplineList

=================================================================================
*/

/*
================
idSplineList::clearControl
================
*/
void idSplineList::clearControl() {
	for ( int i = 0; i < controlPoints.Num(); i++ ) {
		delete controlPoints[i];
	}
	controlPoints.Clear();
}

/*
================
idSplineList::clearSpline
================
*/
void idSplineList::clearSpline() {
	for ( int i = 0; i < splinePoints.Num(); i++ ) {
		delete splinePoints[i];
	}
	splinePoints.Clear();
}

/*
================
idSplineList::clear
================
*/
void idSplineList::clear() {
	clearControl();
	clearSpline();
	splineTime.Clear();
	selected = NULL;
	dirty = true;
	activeSegment = 0;
	granularity = 0.025f;
	pathColor = arcVec4(1.0f, 0.5f, 0.0f, 1.0f);
	controlColor = arcVec4(0.7f, 0.0f, 1.0f, 1.0f);
	segmentColor = arcVec4(0.0f, 0.0f, 1.0f, 1.0 );
	activeColor = arcVec4(1.0f, 0.0f, 0.0f, 1.0f);
}

/*
================
idSplineList::setColors
================
*/
void idSplineList::setColors(arcVec4 &path, arcVec4 &segment, arcVec4 &control, arcVec4 &active) {
	pathColor = path;
	segmentColor = segment;
	controlColor = control;
	activeColor = active;
}

/*
================
idSplineList::validTime
================
*/
bool idSplineList::validTime() {
	if ( dirty ) {
		buildSpline();
	}
	// gcc doesn't allow static casting away from bools
	// why?  I've no idea...
	return (bool)(splineTime.Num() > 0 && splineTime.Num() == splinePoints.Num() );
}

/*
================
idSplineList::addToRenderer
================
*/
void idSplineList::addToRenderer() {
	int i;
	arcVec3 mins, maxs;

	if (controlPoints.Num() == 0 ) {
		return;
	}

	for ( i = 0; i < controlPoints.Num(); i++ ) {
		VectorCopy(*controlPoints[i], mins);
		VectorCopy( mins, maxs);
		mins[0] -= 8;
		mins[1] += 8;
		mins[2] -= 8;
		maxs[0] += 8;
		maxs[1] -= 8;
		maxs[2] += 8;
		debugLine( colorYellow, mins[0], mins[1], mins[2], maxs[0], mins[1], mins[2] );
		debugLine( colorYellow, maxs[0], mins[1], mins[2], maxs[0], maxs[1], mins[2] );
		debugLine( colorYellow, maxs[0], maxs[1], mins[2], mins[0], maxs[1], mins[2] );
		debugLine( colorYellow, mins[0], maxs[1], mins[2], mins[0], mins[1], mins[2] );

		debugLine( colorYellow, mins[0], mins[1], maxs[2], maxs[0], mins[1], maxs[2] );
		debugLine( colorYellow, maxs[0], mins[1], maxs[2], maxs[0], maxs[1], maxs[2] );
		debugLine( colorYellow, maxs[0], maxs[1], maxs[2], mins[0], maxs[1], maxs[2] );
		debugLine( colorYellow, mins[0], maxs[1], maxs[2], mins[0], mins[1], maxs[2] );

	}

	int step = 0;
	arcVec3 step1;
	for ( i = 3; i < controlPoints.Num(); i++ ) {
		for (float tension = 0.0f; tension < 1.001f; tension += 0.1f) {
			float x = 0;
			float y = 0;
			float z = 0;
			for ( int j = 0; j < 4; j++ ) {
				x += controlPoints[i - (3 - j)]->x * CalcSpline(j, tension);
				y += controlPoints[i - (3 - j)]->y * CalcSpline(j, tension);
				z += controlPoints[i - (3 - j)]->z * CalcSpline(j, tension);
			}
			if (step == 0 ) {
				step1[0] = x;
				step1[1] = y;
				step1[2] = z;
				step = 1;
			} else {
				debugLine( colorWhite, step1[0], step1[1], step1[2], x, y, z);
				step = 0;
			}

		}
	}
}

/*
================
idSplineList::buildSpline
================
*/
void idSplineList::buildSpline() {
	int start = Sys_Milliseconds();
	clearSpline();
	for ( int i = 3; i < controlPoints.Num(); i++ ) {
		for (float tension = 0.0f; tension < 1.001f; tension += granularity) {
			float x = 0;
			float y = 0;
			float z = 0;
			for ( int j = 0; j < 4; j++ ) {
				x += controlPoints[i - (3 - j)]->x * CalcSpline(j, tension);
				y += controlPoints[i - (3 - j)]->y * CalcSpline(j, tension);
				z += controlPoints[i - (3 - j)]->z * CalcSpline(j, tension);
			}
			splinePoints.Append(new arcVec3( x, y, z ) );
		}
	}
	dirty = false;
	//common->Printf( "Spline build took %f seconds\n", ( float )(Sys_Milliseconds() - start) / 1000);
}

/*
================
idSplineList::draw
================
*/
void idSplineList::draw(bool editMode) {
        int i;

	if (controlPoints.Num() == 0 ) {
		return;
	}

	if ( dirty ) {
		buildSpline();
	}

	qglColor3fv( controlColor.ToFloatPtr() );
	qglPointSize( 5 );

	qglBegin( GL_POINTS );
	for ( i = 0; i < controlPoints.Num(); i++ ) {
		qglVertex3fv( (*controlPoints[i] ).ToFloatPtr() );
	}
	qglEnd();

	if ( editMode ) {
		for ( i = 0; i < controlPoints.Num(); i++ ) {
			QGLBox(activeColor, *controlPoints[i], 4);
		}
	}

	//Draw the curve
	qglColor3fv( pathColor.ToFloatPtr() );
	qglBegin( GL_LINE_STRIP );
	int count = splinePoints.Num();
	for ( i = 0; i < count; i++ ) {
		qglVertex3fv( (*splinePoints[i] ).ToFloatPtr() );
	}
	qglEnd();

	if ( editMode ) {
		qglColor3fv( segmentColor.ToFloatPtr() );
		qglPointSize( 3 );
		qglBegin( GL_POINTS );
		for ( i = 0; i < count; i++ ) {
			qglVertex3fv( ( *splinePoints[i] ).ToFloatPtr() );
		}
		qglEnd();
	}
	if (count > 0 ) {
		//assert(activeSegment >=0 && activeSegment < count);
		if ( activeSegment >=0 && activeSegment < count ) {
			QGLBox( activeColor, *splinePoints[activeSegment], 6);
			QGLBox( colorYellow, *splinePoints[activeSegment], 8);
		}
	}

}

/*
================
idSplineList::totalDistance
================
*/
float idSplineList::totalDistance() {
	// FIXME: save dist and return
	//
	if (controlPoints.Num() == 0 ) {
		return 0.0f;
	}

	if ( dirty ) {
		buildSpline();
	}

	float dist = 0.0f;
	arcVec3 temp;
	int count = splinePoints.Num();
	for ( int i = 1; i < count; i++ ) {
		temp = *splinePoints[i-1];
		temp -= *splinePoints[i];
		dist += temp.Length();
	}
	return dist;
}

/*
================
idSplineList::initPosition
================
*/
void idSplineList::initPosition(long bt, long totalTime) {
	if ( dirty ) {
		buildSpline();
	}

	if (splinePoints.Num() == 0 ) {
		return;
	}

	baseTime = bt;
	time = totalTime;

	// calc distance to travel ( this will soon be broken into time segments )
	splineTime.Clear();
	splineTime.Append(bt);
	double dist = totalDistance();
	double distSoFar = 0.0;
	arcVec3 temp;
	int count = splinePoints.Num();
	//for ( int i = 2; i < count - 1; i++ ) {
	for ( int i = 1; i < count; i++ ) {
		temp = *splinePoints[i-1];
		temp -= *splinePoints[i];
		distSoFar += temp.Length();
		double percent = distSoFar / dist;
		percent *= totalTime;
		splineTime.Append(percent + bt);
	}
	assert(splineTime.Num() == splinePoints.Num() );
	activeSegment = 0;
}

/*
================
idSplineList::CalcSpline
================
*/
float idSplineList::CalcSpline( int step, float tension) {
	switch(step) {
		case 0:	return (pow(1 - tension, 3) ) / 6;
		case 1:	return (3 * pow(tension, 3) - 6 * pow(tension, 2) + 4) / 6;
		case 2:	return (-3 * pow(tension, 3) + 3 * pow(tension, 2) + 3 * tension + 1 ) / 6;
		case 3:	return pow(tension, 3) / 6;
	}
	return 0.0f;
}

/*
================
idSplineList::UpdateSelection
================
*/
void idSplineList::UpdateSelection(const arcVec3 &move) {
	if (selected) {
		dirty = true;
		VectorAdd(*selected, move, *selected);
	}
}

/*
================
idSplineList::SetSelectedPoint
================
*/
void idSplineList::SetSelectedPoint(arcVec3 *p) {
	if (p) {
		p->SnapInt();
		for ( int i = 0; i < controlPoints.Num(); i++ ) {
			if ( ( *p ).Compare( *controlPoints[i], VECTOR_EPSILON ) ) {
				selected = controlPoints[i];
			}
		}
	} else {
		selected = NULL;
	}
}

/*
================
idSplineList::GetPosition
================
*/
const arcVec3 *idSplineList::GetPosition(long t) {
	static arcVec3 interpolatedPos;

	int count = splineTime.Num();
	if (count == 0 ) {
		return &vec3_zero;
	}

	assert(splineTime.Num() == splinePoints.Num() );

#if 0
	float velocity = getVelocity(t);
	float timePassed = t - lastTime;
	lastTime = t;

	// convert to seconds
	timePassed /= 1000;

	float distToTravel = timePassed * velocity;

	distSoFar += distToTravel;
	float tempDistance = 0;

	arcVec3 temp;
	int count = splinePoints.Num();
	//for ( int i = 2; i < count - 1; i++ ) {
	for ( int i = 1; i < count; i++ ) {
		temp = *splinePoints[i-1];
		temp -= *splinePoints[i];
		tempDistance += temp.Length();
		if (tempDistance >= distSoFar) {
			break;
		}
	}

	if ( i == count) {
		interpolatedPos = splinePoints[i-1];
	} else {
		double timeHi = splineTime[i + 1];
		double timeLo = splineTime[i - 1];
		double percent = (timeHi - t) / (timeHi - timeLo);
		arcVec3 v1 = *splinePoints[i - 1];
		arcVec3 v2 = *splinePoints[i + 1];
		v2 *= ( 1.0f - percent );
		v1 *= percent;
		v2 += v1;
		interpolatedPos = v2;
	}
	return &interpolatedPos;
#else
	while (activeSegment < count) {
		if (splineTime[activeSegment] >= t) {
			if (activeSegment > 0 && activeSegment < count - 1 ) {
				double timeHi = splineTime[activeSegment + 1];
				double timeLo = splineTime[activeSegment - 1];
				//float percent = ( float )(baseTime + time - t) / time;
				double percent = (timeHi - t) / (timeHi - timeLo);
				// pick two bounding points
				arcVec3 v1 = *splinePoints[activeSegment-1];
				arcVec3 v2 = *splinePoints[activeSegment+1];
				v2 *= ( 1.0f - percent );
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
#endif
}

/*
================
idSplineList::parse
================
*/
void idSplineList::Parse( ARCParser *src ) {
	arcNetToken token;
	arcNetString key;

	src->ExpectTokenString( "{" );

	while ( 1 ) {
		if ( !src->ExpectAnyToken( &token ) ) {
			break;
		}
		if ( token == "}" ) {
			break;
		}
		// if token is not a brace, it is a key for a key/value pair
		if ( token == "( " ) {
			src->UnreadToken( &token );
			// read the control point
			arcVec3 point;
			src->Parse1DMatrix( 3, point.ToFloatPtr() );
			addPoint(point.x, point.y, point.z);
		} else {
			key = token;
			src->ReadTokenOnLine( &token );
			if ( !key.Icmp( "granularity" ) ) {
				granularity = atof(token.c_str() );
			} else if ( !key.Icmp( "name" ) ) {
				name = token;
			} else {
				src->Error( "unknown spline list key: %s", key.c_str() );
				break;
			}
		}
	}
	dirty = true;
}

/*
================
idSplineList::write
================
*/
void idSplineList::write( arcNetFile *f, const char *p) {
	f->Printf( "\t\t%s {\n", p );

	//f->Printf( "\t\tname %s\n", name.c_str() );
	f->Printf( "\t\t\tgranularity %f\n", granularity );
	int count = controlPoints.Num();
	for ( int i = 0; i < count; i++ ) {
		f->Printf( "\t\t\t( %f %f %f )\n", controlPoints[i]->x, controlPoints[i]->y, controlPoints[i]->z );
	}
	f->Printf( "\t\t}\n" );
}

/*
=================================================================================

idCamaraDef

=================================================================================
*/

/*
================
aRCCameraDef::clear
================
*/
void aRCCameraDef::clear() {
	currentCameraPosition = 0;
	cameraRunning = false;
	lastDirection.Zero();
	baseTime = 30;
	activeTarget = 0;
	name = "camera01";
	fov.SetFOV(90);
	int i;
	for ( i = 0; i < targetPositions.Num(); i++ ) {
		delete targetPositions[i];
	}
	for ( i = 0; i < events.Num(); i++ ) {
		delete events[i];
	}
	delete cameraPosition;
	cameraPosition = NULL;
	events.Clear();
	targetPositions.Clear();
}

/*
================
aRCCameraDef::startNewCamera
================
*/
ARCCameraPos *aRCCameraDef::startNewCamera( ARCCameraPos::positionType type ) {
	clear();
	if (type == ARCCameraPos::SPLINE) {
		cameraPosition = new ARCSplinePos();
	} else if (type == ARCCameraPos::INTERPOLATED) {
		cameraPosition = new idInterpolatedPosition();
	} else {
		cameraPosition = new ARCFixedPos();
	}
	return cameraPosition;
}

/*
================
aRCCameraDef::addTarget
================
*/
void aRCCameraDef::addTarget(const char *name, ARCCameraPos::positionType type) {
	const char *text = (name == NULL) ? va( "target0%d", numTargets()+1 ) : name;
	ARCCameraPos *pos = newFromType(type);
	if ( pos ) {
		pos->setName( name );
		targetPositions.Append( pos );
		activeTarget = numTargets()-1;
		if (activeTarget == 0 ) {
			// first one
			addEvent(aRCCameraEvent::EVENT_TARGET, name, 0 );
		}
	}
}

/*
================
aRCCameraDef::getActiveTarget
================
*/
ARCCameraPos *aRCCameraDef::getActiveTarget() {
	if (targetPositions.Num() == 0 ) {
		addTarget(NULL, ARCCameraPos::FIXED);
	}
	return targetPositions[activeTarget];
}

/*
================
aRCCameraDef::getActiveTarget
================
*/
ARCCameraPos *aRCCameraDef::getActiveTarget( int index ) {
	if (targetPositions.Num() == 0 ) {
		addTarget(NULL, ARCCameraPos::FIXED);
		return targetPositions[0];
	}
	return targetPositions[index];
}

/*
================
aRCCameraDef::setActiveTargetByName
================
*/
void aRCCameraDef::setActiveTargetByName( const char *name ) {
	for ( int i = 0; i < targetPositions.Num(); i++ ) {
		if (arcNetString::Icmp(name, targetPositions[i]->getName() ) == 0 ) {
			setActiveTarget( i );
			return;
		}
	}
}

/*
================
aRCCameraDef::setActiveTarget
================
*/
void aRCCameraDef::setActiveTarget( int index ) {
	assert(index >= 0 && index < targetPositions.Num() );
	activeTarget = index;
}

/*
================
aRCCameraDef::draw
================
*/
void aRCCameraDef::draw( bool editMode ) {
            // gcc doesn't allow casting away from bools
            // why?  I've no idea...
	if (cameraPosition) {
		cameraPosition->draw((bool)((editMode || cameraRunning) && camEdit) );
		int count = targetPositions.Num();
		for ( int i = 0; i < count; i++ ) {
			targetPositions[i]->draw((bool)((editMode || cameraRunning) && i == activeTarget && !camEdit) );
		}
	}
}

/*
================
aRCCameraDef::numPoints
================
*/
int aRCCameraDef::numPoints() {
	if ( camEdit ) {
		return cameraPosition->numPoints();
	}
	return getActiveTarget()->numPoints();
}

/*
================
aRCCameraDef::getPoint
================
*/
const arcVec3 *aRCCameraDef::getPoint( int index ) {
	if ( camEdit ) {
		return cameraPosition->getPoint( index );
	}
	return getActiveTarget()->getPoint( index );
}

/*
================
aRCCameraDef::stopEdit
================
*/
void aRCCameraDef::stopEdit() {
	editMode = false;
	if ( camEdit ) {
		cameraPosition->stopEdit();
	} else {
		getActiveTarget()->stopEdit();
	}
}

/*
================
aRCCameraDef::startEdit
================
*/
void aRCCameraDef::startEdit(bool camera) {
	camEdit = camera;
	if ( camera ) {
		cameraPosition->startEdit();
		for ( int i = 0; i < targetPositions.Num(); i++ ) {
			targetPositions[i]->stopEdit();
		}
	} else {
		getActiveTarget()->startEdit();
		cameraPosition->stopEdit();
	}
	editMode = true;
}

/*
================
aRCCameraDef::getPositionObj
================
*/
ARCCameraPos *aRCCameraDef::getPositionObj() {
	if (cameraPosition == NULL) {
		cameraPosition = new ARCFixedPos();
	}
	return cameraPosition;
}

/*
================
aRCCameraDef::getActiveSegmentInfo
================
*/
void aRCCameraDef::getActiveSegmentInfo( int segment, arcVec3 &origin, arcVec3 &direction, float *fov) {
#if 0
	if ( !camSpline.validTime() ) {
		buildCamera();
	}
	double d = ( double )segment / numSegments();
	getCameraInfo(d * totalTime * 1000, origin, direction, fov);
#endif
/*
	if ( !camSpline.validTime() ) {
		buildCamera();
	}
	origin = *camSpline.getSegmentPoint(segment);


	arcVec3 temp;

	int numTargets = getTargetSpline()->controlPoints.Num();
	int count = camSpline.splineTime.Num();
	if (numTargets == 0 ) {
		// follow the path
		if (camSpline.getActiveSegment() < count - 1 ) {
			temp = *camSpline.splinePoints[camSpline.getActiveSegment()+1];
		}
	} else if (numTargets == 1 ) {
		temp = *getTargetSpline()->controlPoints[0];
	} else {
		temp = *getTargetSpline()->getSegmentPoint(segment);
	}

	temp -= origin;
	temp.Normalize();
	direction = temp;
*/
}

/*
================
aRCCameraDef::getCameraInfo
================
*/
bool aRCCameraDef::getCameraInfo(long time, arcVec3 &origin, arcVec3 &direction, float *fv) {
	char	buff[ 1024 ];
	int		i;

	if ((time - startTime) / 1000 <= totalTime) {

		for ( i = 0; i < events.Num(); i++ ) {
			if (time >= startTime + events[i]->getTime() && !events[i]->getTriggered() ) {
				events[i]->setTriggered(true);
				if (events[i]->getType() == aRCCameraEvent::EVENT_TARGET) {
					setActiveTargetByName(events[i]->getParam() );
					getActiveTarget()->start(startTime + events[i]->getTime() );
					//common->Printf( "Triggered event switch to target: %s\n",events[i]->getParam() );
				} else if (events[i]->getType() == aRCCameraEvent::EVENT_TRIGGER) {
#if 0
//FIXME: seperate game and editor spline code
					arcEntity *ent;
					ent = gameLocal.FindEntity( events[i]->getParam() );
					if (ent) {
						ent->Signal( SIG_TRIGGER );
						ent->ProcessEvent( &EV_Activate, gameLocal.world );
					}
#endif
				} else if (events[i]->getType() == aRCCameraEvent::EVENT_FOV) {
					memset(buff, 0, sizeof(buff) );
					strcpy(buff, events[i]->getParam() );
					const char *param1 = strtok(buff, " \t,\0" );
					const char *param2 = strtok(NULL, " \t,\0" );
					fov.reset(fov.GetFOV( time ), atof(param1), time, atoi(param2) );
					//*fv = fov = atof(events[i]->getParam() );
				} else if (events[i]->getType() == aRCCameraEvent::EVENT_CAMERA) {
				} else if (events[i]->getType() == aRCCameraEvent::EVENT_STOP) {
					return false;
				}
			}
		}
	} else {
	}

	origin = *cameraPosition->GetPosition( time );

	*fv = fov.GetFOV( time );

	arcVec3 temp = origin;

	int numTargets = targetPositions.Num();
	if (numTargets == 0 ) {
/*		// follow the path
		if (camSpline.getActiveSegment() < count - 1 ) {
			temp = *camSpline.splinePoints[camSpline.getActiveSegment()+1];
			if (temp == origin) {
				int index = camSpline.getActiveSegment() + 2;
				while (temp == origin && index < count - 1 ) {
					temp = *camSpline.splinePoints[index++];
				}
			}
		}*/
	} else {
		temp = *getActiveTarget()->GetPosition( time );
	}

	temp -= origin;
	temp.Normalize();
	direction = temp;

	return true;
}

/*
================
aRCCameraDef::waitEvent
================
*/
bool aRCCameraDef::waitEvent( int index ) {
	//for ( int i = 0; i < events.Num(); i++ ) {
	//	if (events[i]->getSegment() == index && events[i]->getType() == aRCCameraEvent::EVENT_WAIT) {
	//		return true;
	//	}
    //}
	return false;
}

/*
================
aRCCameraDef::buildCamera
================
*/
#define NUM_CCELERATION_SEGS 10
#define CELL_AMT 5

void aRCCameraDef::buildCamera() {
	int i;
	int lastSwitch = 0;
	arcNetList<float> waits;
	arcNetList<int> targets;

	totalTime = baseTime;
	cameraPosition->setTime(totalTime * 1000);
	// we have a base time layout for the path and the target path
	// now we need to layer on any wait or speed changes
	for ( i = 0; i < events.Num(); i++ ) {
		aRCCameraEvent *ev = events[i];
		events[i]->setTriggered(false);
		switch (events[i]->getType() ) {
			case aRCCameraEvent::EVENT_TARGET : {
				targets.Append( i );
				break;
			}
			case aRCCameraEvent::EVENT_FEATHER : {
				long startTime = 0;
				float speed = 0;
				long loopTime = 10;
				float stepGoal = cameraPosition->getBaseVelocity() / (1000 / loopTime);
				while (startTime <= 1000) {
					cameraPosition->addVelocity(startTime, loopTime, speed);
					speed += stepGoal;
					if (speed > cameraPosition->getBaseVelocity() ) {
						speed = cameraPosition->getBaseVelocity();
					}
					startTime += loopTime;
				}

				startTime = totalTime * 1000 - 1000;
				long endTime = startTime + 1000;
				speed = cameraPosition->getBaseVelocity();
				while (startTime < endTime) {
					speed -= stepGoal;
					if (speed < 0 ) {
						speed = 0;
					}
					cameraPosition->addVelocity(startTime, loopTime, speed);
					startTime += loopTime;
				}
				break;

			}
			case aRCCameraEvent::EVENT_WAIT : {
				waits.Append(atof(events[i]->getParam() ));

				//FIXME: this is quite hacky for Wolf E3, accel and decel needs
				// do be parameter based etc..
				long startTime = events[i]->getTime() - 1000;
				if (startTime < 0 ) {
					startTime = 0;
				}
				float speed = cameraPosition->getBaseVelocity();
				long loopTime = 10;
				float steps = speed / ((events[i]->getTime() - startTime) / loopTime);
				while (startTime <= events[i]->getTime() - loopTime) {
					cameraPosition->addVelocity(startTime, loopTime, speed);
					speed -= steps;
					startTime += loopTime;
				}
				cameraPosition->addVelocity(events[i]->getTime(), atof(events[i]->getParam() ) * 1000, 0 );

				startTime = events[i]->getTime() + atof(events[i]->getParam() ) * 1000;
				long endTime = startTime + 1000;
				speed = 0;
				while (startTime <= endTime) {
					cameraPosition->addVelocity(startTime, loopTime, speed);
					speed += steps;
					startTime += loopTime;
				}
				break;
			}
			case aRCCameraEvent::EVENT_TARGETWAIT : {
				//targetWaits.Append( i );
				break;
			}
			case aRCCameraEvent::EVENT_SPEED : {
/*
				// take the average delay between up to the next five segments
				float adjust = atof(events[i]->getParam() );
				int index = events[i]->getSegment();
				total = 0;
				count = 0;

				// get total amount of time over the remainder of the segment
				for (j = index; j < camSpline.numSegments() - 1; j++ ) {
					total += camSpline.getSegmentTime(j + 1 ) - camSpline.getSegmentTime(j);
					count++;
				}

				// multiply that by the adjustment
				double newTotal = total * adjust;
				// what is the difference..
				newTotal -= total;
				totalTime += newTotal / 1000;

				// per segment difference
				newTotal /= count;
				int additive = newTotal;

				// now propogate that difference out to each segment
				for (j = index; j < camSpline.numSegments(); j++ ) {
					camSpline.addSegmentTime(j, additive);
					additive += newTotal;
				}
				break;
*/
			}
		}
	}


	for ( i = 0; i < waits.Num(); i++ ) {
		totalTime += waits[i];
	}

	// on a new target switch, we need to take time to this point ( since last target switch )
	// and allocate it across the active target, then reset time to this point
	long timeSoFar = 0;
	long total = totalTime * 1000;
	for ( i = 0; i < targets.Num(); i++ ) {
		long t;
		if ( i < targets.Num() - 1 ) {
			t = events[targets[i+1]]->getTime();
		} else {
			t = total - timeSoFar;
		}
		// t is how much time to use for this target
		setActiveTargetByName(events[targets[i]]->getParam() );
		getActiveTarget()->setTime(t);
		timeSoFar += t;
	}
}

/*
================
aRCCameraDef::startCamera
================
*/
void aRCCameraDef::startCamera(long t) {
	cameraPosition->clearVelocities();
	cameraPosition->start(t);
	buildCamera();
	//for ( int i = 0; i < targetPositions.Num(); i++ ) {
	//	targetPositions[i]->
	//}
	startTime = t;
	cameraRunning = true;
}

/*
================
aRCCameraDef::parse
================
*/
void aRCCameraDef::Parse( ARCParser *src  ) {
	arcNetToken token;

	src->ReadToken(&token);
	src->ExpectTokenString( "{" );
	while ( 1 ) {
		src->ExpectAnyToken( &token );
		if ( token == "}" ) {
			break;
		} else if ( !token.Icmp( "time" ) ) {
			baseTime = src->ParseFloat();
		} else if ( !token.Icmp( "camera_fixed" ) ) {
			cameraPosition = new ARCFixedPos();
			cameraPosition->Parse( src );
		} else if ( !token.Icmp( "camera_interpolated" ) ) {
			cameraPosition = new idInterpolatedPosition();
			cameraPosition->Parse( src );
		} else if ( !token.Icmp( "camera_spline" ) ) {
			cameraPosition = new ARCSplinePos();
			cameraPosition->Parse( src );
		} else if ( !token.Icmp( "target_fixed" ) ) {
			ARCFixedPos *pos = new ARCFixedPos();
			pos->Parse( src );
			targetPositions.Append( pos );
		} else if ( !token.Icmp( "target_interpolated" ) ) {
			idInterpolatedPosition *pos = new idInterpolatedPosition();
			pos->Parse( src );
			targetPositions.Append( pos );
		} else if ( !token.Icmp( "target_spline" ) ) {
			ARCSplinePos *pos = new ARCSplinePos();
			pos->Parse( src );
			targetPositions.Append( pos );
		} else if ( !token.Icmp( "fov" ) ) {
			fov.Parse( src );
		} else if ( !token.Icmp( "event" ) ) {
			aRCCameraEvent *event = new aRCCameraEvent();
			event->Parse( src );
			addEvent(event);
		} else {
			src->Error( "unknown camera def: %s", token.c_str() );
			break;
		}
	}

	if ( !cameraPosition ) {
		common->Printf( "no camera position specified\n" );
		// prevent a crash later on
		cameraPosition = new ARCFixedPos();
	}
}

/*
================
aRCCameraDef::load
================
*/
bool aRCCameraDef::load( const char *filename ) {
	ARCParser *src;

	src = new ARCParser( filename, LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
	if ( !src->IsLoaded() ) {
		common->Printf( "couldn't load %s\n", filename );
		delete src;
		return false;
	}

	clear();
	Parse( src );

	delete src;

	return true;
}

/*
================
aRCCameraDef::save
================
*/
void aRCCameraDef::save(const char *filename) {
	arcNetFile *f = fileSystem->OpenFileWrite( filename, "fs_devpath" );
	if ( f ) {
		int i;
		f->Printf( "cameraPathDef { \n" );
		f->Printf( "\ttime %f\n", baseTime );

		cameraPosition->write( f, va( "camera_%s",cameraPosition->typeStr() ) );

		for ( i = 0; i < numTargets(); i++ ) {
			targetPositions[i]->write( f, va( "target_%s", targetPositions[i]->typeStr() ) );
		}

		for ( i = 0; i < events.Num(); i++ ) {
			events[i]->write( f, "event" );
		}

		fov.write( f, "fov" );

		f->Printf( "}\n" );
	}
	fileSystem->CloseFile( f );
}

/*
================
aRCCameraDef::sortEvents
================
*/
int aRCCameraDef::sortEvents( const void *p1, const void *p2 ) {
	aRCCameraEvent *ev1 =  (aRCCameraEvent* )( p1 );
	aRCCameraEvent *ev2 = ( aRCCameraEvent* )( p2 );

	if ( ev1->getTime() > ev2->getTime() ) {
		return -1;
	}
	if ( ev1->getTime() < ev2->getTime() ) {
		return 1;
	}
	return 0;
}

/*
================
aRCCameraDef::addEvent
================
*/
void aRCCameraDef::addEvent( aRCCameraEvent *event ) {
	events.Append(event);
	//events.Sort(&sortEvents);

}

/*
================
aRCCameraDef::addEvent
================
*/
void aRCCameraDef::addEvent(aRCCameraEvent::eventType t, const char *param, long time) {
	addEvent(new aRCCameraEvent(t, param, time) );
	buildCamera();
}

/*
================
aRCCameraDef::newFromType
================
*/
ARCCameraPos *aRCCameraDef::newFromType( ARCCameraPos::positionType t ) {
	switch (t) {
		case ARCCameraPos::FIXED : return new ARCFixedPos();
		case ARCCameraPos::INTERPOLATED : return new idInterpolatedPosition();
		case ARCCameraPos::SPLINE : return new ARCSplinePos();
	};
	return NULL;
}

/*
=================================================================================

aRCCameraEvent

=================================================================================
*/

/*
================
aRCCameraEvent::eventStr
================
*/
const char *aRCCameraEvent::eventStr[] = {
	"NA",
	"WAIT",
	"TARGETWAIT",
	"SPEED",
	"TARGET",
	"SNAPTARGET",
	"FOV",
	"CMD",
	"TRIGGER",
	"STOP",
	"CAMERA",
	"FADEOUT",
	"FADEIN",
	"FEATHER"
};

/*
================
aRCCameraEvent::parse
================
*/
void aRCCameraEvent::Parse( ARCParser *src ) {
	arcNetToken token;
	arcNetString key;

	src->ExpectTokenString( "{" );

	while ( 1 ) {

		if ( !src->ExpectAnyToken( &token ) ) {
			break;
		}
		if ( token == "}" ) {
			break;
		}

		key = token;
		src->ReadTokenOnLine( &token );
		if ( !key.Icmp( "type" ) ) {
			type = static_cast<aRCCameraEvent::eventType>(atoi(token.c_str() ));
		}
		else if ( !key.Icmp( "param" ) ) {
			paramStr = token;
		}
		else if ( !key.Icmp( "time" ) ) {
			time = atoi(token.c_str() );
		}
		else {
			src->Error( "unknown camera event key: %s", key.c_str() );
			break;
		}
	}
}

/*
================
aRCCameraEvent::write
================
*/
void aRCCameraEvent::write( arcNetFile *f, const char *name) {
	f->Printf( "\t%s {\n", name );
	f->Printf( "\t\ttype %d\n", static_cast<int>(type) );
	f->Printf( "\t\tparam \"%s\"\n", paramStr.c_str() );
	f->Printf( "\t\ttime %d\n", time );
	f->Printf( "\t}\n" );
}

/*
=================================================================================

idCamaraPosition

=================================================================================
*/

/*
================
ARCCameraPos::positionStr
================
*/
const char *ARCCameraPos::positionStr[] = {
	"Fixed",
	"Interpolated",
	"Spline",
};

/*
================
ARCCameraPos::positionStr
================
*/
void ARCCameraPos::clearVelocities() {
	for ( int i = 0; i < velocities.Num(); i++ ) {
		delete velocities[i];
		velocities[i] = NULL;
	}
	velocities.Clear();
}

/*
================
ARCCameraPos::positionStr
================
*/
float ARCCameraPos::getVelocity( long t ) {
	long check = t - startTime;
	for ( int i = 0; i < velocities.Num(); i++ ) {
		if (check >= velocities[i]->startTime && check <= velocities[i]->startTime + velocities[i]->time) {
			return velocities[i]->speed;
		}
	}
	return baseVelocity;
}

/*
================
ARCCameraPos::parseToken
================
*/
bool ARCCameraPos::parseToken( const arcNetString &key, ARCParser *src ) {
	arcNetToken token;

	if ( !key.Icmp( "time" ) ) {
		time = src->ParseInt();
		return true;
	}else if ( !key.Icmp( "type" ) ) {
		type = static_cast<ARCCameraPos::positionType> ( src->ParseInt() );
		return true;
	}else if ( !key.Icmp( "velocity" ) ) {
		long t = atol( token );
		long d = src->ParseInt();
		float s = src->ParseFloat();
		addVelocity(t, d, s);
		return true;
	}else if ( !key.Icmp( "baseVelocity" ) ) {
		baseVelocity = src->ParseFloat();
		return true;
	}else if ( !key.Icmp( "name" ) ) {
		src->ReadToken( &token );
		name = token;
		return true;
	}else if ( !key.Icmp( "time" ) ) {
		time = src->ParseInt();
		return true;
	}else {
		src->Error( "unknown camera position key: %s", key.c_str() );
		return false;
	}
}

/*
================
ARCCameraPos::write
================
*/
void ARCCameraPos::write( arcNetFile *f, const char *p ) {
	f->Printf( "\t\ttime %i\n", time );
	f->Printf( "\t\ttype %i\n", static_cast<int>(type) );
	f->Printf( "\t\tname %s\n", name.c_str() );
	f->Printf( "\t\tbaseVelocity %f\n", baseVelocity );
	for ( int i = 0; i < velocities.Num(); i++ ) {
		f->Printf( "\t\tvelocity %i %i %f\n", velocities[i]->startTime, velocities[i]->time, velocities[i]->speed );
	}
}

/*
=================================================================================

idInterpolatedPosition

=================================================================================
*/

/*
================
idInterpolatedPosition::getPoint
================
*/
arcVec3 *idInterpolatedPosition::getPoint( int index ) {
	assert( index >= 0 && index < 2 );
	if ( index == 0 ) {
		return &startPos;
	}
	return &endPos;
}

/*
================
idInterpolatedPosition::addPoint
================
*/
void idInterpolatedPosition::addPoint( const float x, const float y, const float z ) {
	if (first) {
		startPos.Set( x, y, z );
		first = false;
	} else {
		endPos.Set( x, y, z );
		first = true;
	}
}

/*
================
idInterpolatedPosition::addPoint
================
*/
void idInterpolatedPosition::addPoint( const arcVec3 &v ) {
	if (first) {
		startPos = v;
		first = false;
	}else {
		endPos = v;
		first = true;
	}
}

/*
================
idInterpolatedPosition::draw
================
*/
void idInterpolatedPosition::draw( bool editMode ) {
	glLabeledPoint( colorBlue, startPos, ( editMode ) ? 5 : 3, "Start interpolated" );
	glLabeledPoint( colorBlue, endPos, ( editMode ) ? 5 : 3, "End interpolated" );
	qglBegin( GL_LINES );
	qglVertex3fv( startPos.ToFloatPtr() );
	qglVertex3fv( endPos.ToFloatPtr() );
	qglEnd();
}

/*
================
idInterpolatedPosition::start
================
*/
void idInterpolatedPosition::start( long t ) {
	ARCCameraPos::start(t);
	lastTime = startTime;
	distSoFar = 0.0f;
	arcVec3 temp = startPos;
	temp -= endPos;
	calcVelocity(temp.Length() );
}

/*
================
idInterpolatedPosition::GetPosition
================
*/
const arcVec3 *idInterpolatedPosition::GetPosition( long t ) {
	static arcVec3 interpolatedPos;

	if (t - startTime > 6000) {
		int i = 0;
	}

	float velocity = getVelocity(t);
	float timePassed = t - lastTime;
	lastTime = t;

	// convert to seconds
	timePassed /= 1000;

	if (velocity != getBaseVelocity() ) {
		int i = 0;
	}

	float distToTravel = timePassed * velocity;

	arcVec3 temp = startPos;
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
	// float percent = ( float )(startTime + time - t) / time;

	arcVec3 v1 = startPos;
	arcVec3 v2 = endPos;
	v1 *= ( 1.0f - percent );
	v2 *= percent;
	v1 += v2;
	interpolatedPos = v1;
	return &interpolatedPos;
}

/*
================
idInterpolatedPosition::parse
================
*/
void idInterpolatedPosition::Parse( ARCParser *src ) {
	arcNetToken token;

	src->ExpectTokenString( "{" );
	while ( 1 ) {
		if ( !src->ExpectAnyToken( &token ) ) {
			break;
		}
		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "startPos" ) ) {
			src->Parse1DMatrix( 3, startPos.ToFloatPtr() );
		}else if ( !token.Icmp( "endPos" ) ) {
			src->Parse1DMatrix( 3, endPos.ToFloatPtr() );
		}else {
			ARCCameraPos::parseToken( token, src);
		}
	}
}

/*
================
idInterpolatedPosition::write
================
*/
void idInterpolatedPosition::write( arcNetFile *f, const char *p ) {
	f->Printf( "\t%s {\n", p );
	ARCCameraPos::write( f, p );
	f->Printf( "\t\tstartPos ( %f %f %f )\n", startPos.x, startPos.y, startPos.z );
	f->Printf( "\t\tendPos ( %f %f %f )\n", endPos.x, endPos.y, endPos.z );
	f->Printf( "\t}\n" );
}

/*
=================================================================================

ARCCamFOV

=================================================================================
*/

/*
================
ARCCamFOV::GetFOV
================
*/
float ARCCamFOV::GetFOV( long t ) {
	if ( time ) {
		assert(startTime);
		float percent = (t - startTime) / length;
		if ( percent < 0.0f ) {
			percent = 0.0f;
		} else if ( percent > 1.0f ) {
			percent = 1.0f;
		}
		float temp = endFOV - startFOV;
		temp *= percent;
		fov = startFOV + temp;
	}
	return fov;
}

/*
================
ARCCamFOV::reset
================
*/
void ARCCamFOV::reset( float startfov, float endfov, int start, int len ) {
	startFOV = startfov;
	endFOV = endfov;
	startTime = start;
	length = len;
}

/*
================
ARCCamFOV::parse
================
*/
void ARCCamFOV::Parse( ARCParser *src ) {
	arcNetToken token;

	src->ExpectTokenString( "{" );
	while ( 1 ) {
		if ( !src->ExpectAnyToken( &token ) ) {
			break;
		}
		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "fov" ) ) {
			fov = src->ParseFloat();
		}else if ( !token.Icmp( "startFOV" ) ) {
			startFOV = src->ParseFloat();
		}else if ( !token.Icmp( "endFOV" ) ) {
			endFOV = src->ParseFloat();
		}else if ( !token.Icmp( "time" ) ) {
			time = src->ParseInt();
		}else {
			src->Error( "unknown camera FOV key: %s", token.c_str() );
			break;
		}
	}
}

/*
================
ARCCamFOV::write
================
*/
void ARCCamFOV::write( arcNetFile *f, const char *p ) {
	f->Printf( "\t%s {\n", p );
	f->Printf( "\t\tfov %f\n", fov );
	f->Printf( "\t\tstartFOV %f\n", startFOV );
	f->Printf( "\t\tendFOV %f\n", endFOV );
	f->Printf( "\t\ttime %i\n", time );
	f->Printf( "\t}\n" );
}

/*
=================================================================================

ARCFixedPos

=================================================================================
*/

/*
================
ARCFixedPos::parse
================
*/
void ARCFixedPos::Parse( ARCParser *src ) {
	arcNetToken token;

	src->ExpectTokenString( "{" );
	while ( 1 ) {
		if ( !src->ExpectAnyToken( &token ) ) {
			break;
		}
		if ( token == "}" ) {
			break;
		}
		if ( !token.Icmp( "pos" ) ) {
			src->Parse1DMatrix( 3, pos.ToFloatPtr() );
		}else {
			ARCCameraPos::parseToken( token, src );
		}
	}
}

/*
================
ARCFixedPos::write
================
*/
void ARCFixedPos::write( arcNetFile *f, const char *p ) {
	f->Printf( "\t%s {\n", p );
	ARCCameraPos::write( f, p );
	f->Printf( "\t\tpos ( %f %f %f )\n", pos.x, pos.y, pos.z );
	f->Printf( "\t}\n" );
}

/*
=================================================================================

ARCSplinePos

=================================================================================
*/

/*
================
ARCSplinePos::start
================
*/
void ARCSplinePos::start( long t ) {
	ARCCameraPos::start( t );
	target.initPosition(t, time);
	lastTime = startTime;
	distSoFar = 0.0f;
	calcVelocity(target.totalDistance() );
}

/*
================
ARCSplinePos::parse
================
*/
void ARCSplinePos::Parse( ARCParser *src ) {
	arcNetToken token;

	src->ExpectTokenString( "{" );
	while ( 1 ) {
		if ( !src->ExpectAnyToken( &token ) ) {
			break;
		}
		if ( token == "}" ) {
			break;
		}
		if ( !token.Icmp( "target" ) ) {
			target.Parse( src );
		} else {
			ARCCameraPos::parseToken( token, src );
		}
	}
}

/*
================
ARCSplinePos::write
================
*/
void ARCSplinePos::write( arcNetFile *f, const char *p ) {
	f->Printf( "\t%s {\n", p );
	ARCCameraPos::write( f, p );
	target.write( f, "target" );
	f->Printf( "\t}\n" );
}

/*
================
ARCSplinePos::GetPosition
================
*/
const arcVec3 *ARCSplinePos::GetPosition(long t) {
	static arcVec3 interpolatedPos;

	float velocity = getVelocity(t);
	float timePassed = t - lastTime;
	lastTime = t;

	// convert to seconds
	timePassed /= 1000;

	float distToTravel = timePassed * velocity;

	distSoFar += distToTravel;
	double tempDistance = target.totalDistance();

	double percent = ( double )( distSoFar ) / tempDistance;

	double targetDistance = percent * tempDistance;
	tempDistance = 0;

	double lastDistance1,lastDistance2;
	lastDistance1 = lastDistance2 = 0;
	//FIXME: calc distances on spline build
	arcVec3 temp;
	int count = target.numSegments();
	//for ( int i = 2; i < count - 1; i++ ) {
	int i;
	for ( i = 1; i < count; i++ ) {
		temp = *target.getSegmentPoint( i-1 );
		temp -= *target.getSegmentPoint( i );
		tempDistance += temp.Length();
		if ( i & 1 ) {
			lastDistance1 = tempDistance;
		} else {
			lastDistance2 = tempDistance;
		}
		if (tempDistance >= targetDistance) {
			break;
		}
	}

	if ( i >= count - 1 ) {
		interpolatedPos = *target.getSegmentPoint( i-1 );
	} else {
#if 0
		double timeHi = target.getSegmentTime( i + 1 );
		double timeLo = target.getSegmentTime( i - 1 );
		double percent = (timeHi - t) / (timeHi - timeLo);
		arcVec3 v1 = *target.getSegmentPoint( i - 1 );
		arcVec3 v2 = *target.getSegmentPoint( i + 1 );
		v2 *= ( 1.0f - percent );
		v1 *= percent;
		v2 += v1;
		interpolatedPos = v2;
#else
		if (lastDistance1 > lastDistance2) {
			double d = lastDistance2;
			lastDistance2 = lastDistance1;
			lastDistance1 = d;
		}

		arcVec3 v1 = *target.getSegmentPoint( i - 1 );
		arcVec3 v2 = *target.getSegmentPoint( i );
		double percent = (lastDistance2 - targetDistance) / (lastDistance2 - lastDistance1);
		v2 *= ( 1.0f - percent );
		v1 *= percent;
		v2 += v1;
		interpolatedPos = v2;
#endif
	}
	return &interpolatedPos;

}

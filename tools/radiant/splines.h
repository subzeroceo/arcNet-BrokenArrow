/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __SPLINES_H__
#define __SPLINES_H__

extern void QGLBox(arcVec4 &color, arcVec3 &point, float size);
extern void glLabeledPoint(arcVec4 &color, arcVec3 &point, float size, const char *label);


class idPointListInterface {
public:
						idPointListInterface() { selectedPoints.Clear(); };
						~idPointListInterface() {};

	virtual int			numPoints() { return 0; }
	virtual void		addPoint( const float x, const float y, const float z ) {}
	virtual void		addPoint( const arcVec3 &v ) {}
	virtual void		removePoint( int index ) {}
	virtual arcVec3 *	getPoint( int index ) { return NULL; }

	int					numSelectedPoints() { return selectedPoints.Num(); }
	arcVec3 *			getSelectedPoint( int index );
	int					selectPointByRay( const arcVec3 &origin, const arcVec3 &direction, bool single );
	int					isPointSelected( int index );
	int					selectPoint( int index, bool single );
	void				selectAll();
	void				deselectAll();
	virtual void		UpdateSelection( const arcVec3 &move );
	void				drawSelection();

protected:
	arcNetList<int>			selectedPoints;
};


class idSplineList {
	friend class		idCamera;

public:

						idSplineList() { clear(); }
						idSplineList( const char *p ) { clear(); name = p; }
						~idSplineList() { clear(); }

	void				clearControl();
	void				clearSpline();
	void				Parse( ARCParser *src );
	void				write( arcNetFile *f, const char *name );

	void				clear();
	void				initPosition( long startTime, long totalTime );
	const arcVec3 *		GetPosition( long time );

	void				draw( bool editMode );
	void				addToRenderer();

	void				SetSelectedPoint( arcVec3 *p );
	arcVec3 *			getSelectedPoint() { return selected; }

	void				addPoint( const arcVec3 &v ) { controlPoints.Append(new arcVec3( v) ); dirty = true; }
	void				addPoint( float x, float y, float z ) { controlPoints.Append(new arcVec3( x, y, z ) ); dirty = true; }

	void				UpdateSelection(const arcVec3 &move);
	void				startEdit() { editMode = true; }
	void				stopEdit() { editMode = false; }
	void				buildSpline();
	void				setGranularity( float f ) { granularity = f; }
	float				getGranularity() { return granularity; }

	int					numPoints() { return controlPoints.Num(); }
	arcVec3 *			getPoint( int index ) { assert(index >= 0 && index < controlPoints.Num() ); return controlPoints[index]; }
	arcVec3 *			getSegmentPoint( int index ) { assert(index >= 0 && index < splinePoints.Num() ); return splinePoints[index]; }
	void				setSegmentTime( int index, int time) { assert(index >= 0 && index < splinePoints.Num() ); splineTime[index] = time; }
	int					getSegmentTime( int index ) { assert(index >= 0 && index < splinePoints.Num() ); return splineTime[index]; }
	void				addSegmentTime( int index, int time) { assert(index >= 0 && index < splinePoints.Num() ); splineTime[index] += time; }
	float				totalDistance();

	int					getActiveSegment() { return activeSegment; }
	void				setActiveSegment( int i ) { /* assert( i >= 0 && (splinePoints.Num() > 0 && i < splinePoints.Num() )); */ activeSegment = i; }
	int					numSegments() { return splinePoints.Num(); }

	void				setColors(arcVec4 &path, arcVec4 &segment, arcVec4 &control, arcVec4 &active);

	const char *		getName() { return name.c_str(); }
	void				setName( const char *p ) { name = p; }

	bool				validTime();
	void				setTime( long t ) { time = t; }
	void				setBaseTime( long t ) { baseTime = t; }

protected:
	arcNetString				name;
	float				CalcSpline( int step, float tension);
	arcNetList<arcVec3*>		controlPoints;
	arcNetList<arcVec3*>		splinePoints;
	arcNetList<double>		splineTime;
	arcVec3 *			selected;
	arcVec4				pathColor, segmentColor, controlColor, activeColor;
	float				granularity;
	bool				editMode;
	bool				dirty;
	int					activeSegment;
	long				baseTime;
	long				time;
};

// time in milliseconds
// velocity where 1.0 equal rough walking speed
struct idVelocity {
						idVelocity( long start, long duration, float s ) { startTime = start; time = duration; speed = s; }
	long				startTime;
	long				time;
	float				speed;
};

// can either be a look at or origin position for a camera
class ARCCameraPos : public idPointListInterface {
public:

						ARCCameraPos() { time = 0; name = "position"; }
						ARCCameraPos( const char *p ) { name = p; }
						ARCCameraPos( long t ) { time = t; }
	virtual				~ARCCameraPos() { clear(); }

	// this can be done with RTTI syntax but i like the derived classes setting a type
	// makes serialization a bit easier to see
	//
	enum				positionType {
							FIXED = 0x00,
							INTERPOLATED,
							SPLINE,
							POSITION_COUNT
						};

	virtual void		clearVelocities();
	virtual void		clear() { editMode = false; time = 5000; clearVelocities(); }
	virtual void		start( long t ) { startTime = t; }
	long				getTime() { return time; }
	virtual void		setTime(long t) { time = t; }
	float				getVelocity( long t );
	float				getBaseVelocity() { return baseVelocity; }
	void				addVelocity( long start, long duration, float speed ) { velocities.Append(new idVelocity(start, duration, speed) ); }
	virtual const arcVec3 *GetPosition( long t ) { return NULL; }
	virtual void		draw( bool editMode ) {};
	virtual void		Parse( ARCParser *src ) {};
	virtual void		write( arcNetFile *f, const char *name);
	virtual bool		parseToken( const arcNetString &key, ARCParser *src );
	const char *		getName() { return name.c_str(); }
	void				setName( const char *p ) { name = p; }
	virtual void		startEdit() { editMode = true; }
	virtual void		stopEdit() { editMode = false; }
	virtual void		draw() {};
	const char *		typeStr() { return positionStr[static_cast<int>(type)]; }
	void				calcVelocity( float distance ) { float secs = ( float )time / 1000; baseVelocity = distance / secs; }

protected:
	static const char *	positionStr[POSITION_COUNT];
	long				startTime;
	long				time;
	positionType		type;
	arcNetString				name;
	bool				editMode;
	arcNetList<idVelocity*> velocities;
	float				baseVelocity;
};

class ARCFixedPos : public ARCCameraPos {
public:

						ARCFixedPos() : ARCCameraPos() { init(); }
						ARCFixedPos(arcVec3 p) : ARCCameraPos() { init(); pos = p; }
						~ARCFixedPos() { }

	void				init() { pos.Zero(); type = ARCCameraPos::FIXED; }

	virtual void		addPoint( const arcVec3 &v ) { pos = v; }
	virtual void		addPoint( const float x, const float y, const float z ) { pos.Set( x, y, z ); }
	virtual const arcVec3 *GetPosition( long t ) { return &pos; }
	void				Parse( ARCParser *src );
	void				write( arcNetFile *f, const char *name );
	virtual int			numPoints() { return 1; }
	virtual arcVec3 *	getPoint( int index ) { assert( index == 0 ); return &pos; }
	virtual void		draw( bool editMode ) { glLabeledPoint(colorBlue, pos, ( editMode ) ? 5 : 3, "Fixed point" ); }

protected:
	arcVec3				pos;
};

class idInterpolatedPosition : public ARCCameraPos {
public:
						idInterpolatedPosition() : ARCCameraPos() { init(); }
						idInterpolatedPosition( arcVec3 start, arcVec3 end, long time ) : ARCCameraPos( time ) { init(); startPos = start; endPos = end; }
						~idInterpolatedPosition() { }

	void				init() { type = ARCCameraPos::INTERPOLATED; first = true; startPos.Zero(); endPos.Zero(); }

	virtual const arcVec3 *GetPosition(long t);
	void				Parse( ARCParser *src );
	void				write( arcNetFile *f, const char *name );
	virtual int			numPoints() { return 2; }
	virtual arcVec3 *	getPoint( int index );
	virtual void		addPoint( const float x, const float y, const float z );
	virtual void		addPoint( const arcVec3 &v );
	virtual void		draw( bool editMode );
	virtual void		start( long t );

protected:
	bool				first;
	arcVec3				startPos;
	arcVec3				endPos;
	long				lastTime;
	float				distSoFar;
};

class ARCSplinePos : public ARCCameraPos {
public:

						ARCSplinePos() : ARCCameraPos() { init(); }
						ARCSplinePos( long time ) : ARCCameraPos( time ) { init(); }
						~ARCSplinePos() { }

	void				init() { type = ARCCameraPos::SPLINE; }
	virtual void		start( long t );
	virtual const arcVec3 *GetPosition( long t );
	void				addControlPoint( arcVec3 &v ) { target.addPoint( v); }
	void				Parse( ARCParser *src );
	void				write( arcNetFile *f, const char *name );
	virtual int			numPoints() { return target.numPoints(); }
	virtual arcVec3 *	getPoint( int index ) { return target.getPoint( index ); }
	virtual void		addPoint( const arcVec3 &v ) { target.addPoint( v ); }
	virtual void		draw( bool editMode ) { target.draw( editMode ); }
	virtual void		UpdateSelection( const arcVec3 &move ) { ARCCameraPos::UpdateSelection(move); target.buildSpline(); }

protected:
	idSplineList		target;
	long				lastTime;
	float				distSoFar;
};

class ARCCamFOV {
public:
						ARCCamFOV() { time = 0; fov = 90; }
						ARCCamFOV( int v ) { time = 0; fov = v; }
						ARCCamFOV( int s, int e, long t ) { startFOV = s; endFOV = e; time = t; }
						~ARCCamFOV() { }

	void				SetFOV( float f ) { fov = f; }
	float				GetFOV( long t );
	void				start( long t ) { startTime = t; }
	void				reset( float startfov, float endfov, int start, int len );
	void				Parse( ARCParser *src );
	void				write( arcNetFile *f, const char *name );

protected:
	float				fov;
	float				startFOV;
	float				endFOV;
	int					startTime;
	int					time;
	int					length;
};

class aRCCameraEvent {
public:
	enum				eventType {
							EVENT_NA = 0x00,
							EVENT_WAIT,
							EVENT_TARGETWAIT,
							EVENT_SPEED,
							EVENT_TARGET,
							EVENT_SNAPTARGET,
							EVENT_FOV,
							EVENT_CMD,
							EVENT_TRIGGER,
							EVENT_STOP,
							EVENT_CAMERA,
							EVENT_FADEOUT,
							EVENT_FADEIN,
							EVENT_FEATHER,
							EVENT_COUNT
						};

						aRCCameraEvent() { paramStr = ""; type = EVENT_NA; time = 0; }
						aRCCameraEvent( eventType t, const char *param, long n ) { type = t; paramStr = param; time = n; }
						~aRCCameraEvent() { }

	eventType			getType() { return type; }
	const char *		typeStr() { return eventStr[static_cast<int>(type)]; }
	const char *		getParam() { return paramStr.c_str(); }
	long				getTime() { return time; }
	void				setTime(long n) { time = n; }
	void				Parse( ARCParser *src );
	void				write( arcNetFile *f, const char *name );
	void				setTriggered( bool b ) { triggered = b; }
	bool				getTriggered() { return triggered; }

	static const char *	eventStr[EVENT_COUNT];

protected:
	eventType			type;
	arcNetString				paramStr;
	long				time;
	bool				triggered;

};

class aRCCameraDef {
public:
						aRCCameraDef() { cameraPosition = NULL; clear(); }
						~aRCCameraDef() { clear(); }

	void				clear();
	ARCCameraPos *	startNewCamera(ARCCameraPos::positionType type);
	void				addEvent( aRCCameraEvent::eventType t, const char *param, long time );
	void				addEvent( aRCCameraEvent *event );
	static int			sortEvents( const void *p1, const void *p2 );
	int					numEvents() { return events.Num(); }
	aRCCameraEvent *		getEvent( int index ) { assert(index >= 0 && index < events.Num() ); return events[index]; }
	void				Parse( ARCParser *src );
	bool				load( const char *filename );
	void				save( const char *filename );
	void				buildCamera();

	void				addTarget( const char *name, ARCCameraPos::positionType type );

	ARCCameraPos *	getActiveTarget();
	ARCCameraPos *	getActiveTarget( int index );
	int					numTargets() { return targetPositions.Num(); }
	void				setActiveTargetByName(const char *name);
	void				setActiveTarget( int index );
	void				setRunning( bool b ) { cameraRunning = b; }
	void				setBaseTime( float f ) { baseTime = f; }
	float				getBaseTime() { return baseTime; }
	float				getTotalTime() { return totalTime; }
	void				startCamera( long t );
	void				stopCamera() { cameraRunning = true; }
	void				getActiveSegmentInfo( int segment, arcVec3 &origin, arcVec3 &direction, float *fv);
	bool				getCameraInfo(long time, arcVec3 &origin, arcVec3 &direction, float *fv);
	void				draw( bool editMode );
	int					numPoints();
	const arcVec3 *		getPoint( int index );
	void				stopEdit();
	void				startEdit( bool camera );
	bool				waitEvent( int index );
	const char *		getName() { return name.c_str(); }
	void				setName( const char *p ) { name = p; }
	ARCCameraPos *	getPositionObj();

	static ARCCameraPos *newFromType( ARCCameraPos::positionType t );

protected:
	arcNetString				name;
	int					currentCameraPosition;
	arcVec3				lastDirection;
	bool				cameraRunning;
	ARCCameraPos *	cameraPosition;
	arcNetList<ARCCameraPos*> targetPositions;
	arcNetList<aRCCameraEvent*> events;
	ARCCamFOV			fov;
	int					activeTarget;
	float				totalTime;
	float				baseTime;
	long				startTime;

	bool				camEdit;
	bool				editMode;
};

extern bool g_splineMode;

extern aRCCameraDef *g_splineList;

#endif /* !__SPLINES_H__ */

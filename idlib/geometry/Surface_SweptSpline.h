
#ifndef __SURFACE_SWEPTSPLINE_H__
#define __SURFACE_SWEPTSPLINE_H__

/*
===============================================================================

	Swept Spline surface.

===============================================================================
*/

class anSurface_SweptSpline : public anSurface {
public:
							anSurface_SweptSpline( void );
							~anSurface_SweptSpline( void );

	void					SetSpline( anCurveSpline<anVec4> *spline );
	void					SetSweptSpline( anCurveSpline<anVec4> *sweptSpline );
	void					SetSweptCircle( const float radius );

	void					Tessellate( const int splineSubdivisions, const int sweptSplineSubdivisions );

	void					Clear( void );

protected:
	anCurveSpline<anVec4> *	spline;
	anCurveSpline<anVec4> *	sweptSpline;

	void					GetFrame( const anMat3 &previousFrame, const anVec3 dir, anMat3 &newFrame );
};

/*
====================
anSurface_SweptSpline::anSurface_SweptSpline
====================
*/
inline anSurface_SweptSpline::anSurface_SweptSpline( void ) {
	spline = nullptr;
	sweptSpline = nullptr;
}

/*
====================
anSurface_SweptSpline::~anSurface_SweptSpline
====================
*/
inline anSurface_SweptSpline::~anSurface_SweptSpline( void ) {
	delete spline;
	delete sweptSpline;
}

/*
====================
anSurface_SweptSpline::Clear
====================
*/
inline void anSurface_SweptSpline::Clear( void ) {
	anSurface::Clear();
	delete spline;
	spline = nullptr;
	delete sweptSpline;
	sweptSpline = nullptr;
}



class anSplineList {

public:

	anSplineList() {
		Clear();
	}

	anSplineList( const char *p ) {
		Clear();
		name = p;
	};

	~anSplineList() {
		Clear();
	};

	void ClearControl() {
		for ( int i = 0; i < controlPoints.Num(); i++ ) {
			delete controlPoints[i];
		}
		controlPoints.Clear();
	}

	void ClearSpline() {
		for ( int i = 0; i < splinePoints.Num(); i++ ) {
			delete splinePoints[i];
		}
		splinePoints.Clear();
	}

	void Parse( const char *(* text) );
	void Write( fileHandle_t file, const char *name );

	void Clear() {
		ClearControl();
		ClearSpline();
		splineTime.Clear();
		selected = nullptr;
		dirty = true;
		activeSegment = 0;
		granularity = 0.025f;
		pathColor.set( 1.0f, 0.5f, 0.0f );
		controlColor.set( 0.7f, 0.0f, 1.0f );
		segmentColor.set( 0.0f, 0.0f, 1.0f );
		activeColor.set( 1.0f, 0.0f, 0.0f );
	}

	void InitPosition( long startTime, long totalTime );
	const anVec3 *GetPosition( long time );


	void Draw( bool editMode );
	void AddToRenderer();

	void SetSelectedPoint( anVec3 *p );
	anVec3 *GetSelectedPoint() {return selected;}

	void AddPoint( const anVec3 &v) {
		controlPoints.Append( new anVec3( v ) );
		dirty = true;
	}

	void AddPoint( float x, float y, float z ) {
		controlPoints.Append( new anVec3( x, y, z ) );
		dirty = true;
	}

	void UpdateSelection( const anVec3 &move );
	void BuildSpline();

	void SetGranularity(float f) { granularity = f;}

	float GetGranularity() {return granularity;}

	int NumPoints() {return controlPoints.Num();}

	anVec3 *GetPoint( int index ) {
		assert( index >= 0 && index < controlPoints.Num() );
		return controlPoints[index];
	}

	anVec3 *GetSegmentPoint( int index ) {
		assert( index >= 0 && index < splinePoints.Num() );
		return splinePoints[index];
	}


	void SetSegmentTime( int index, int time ) {
		assert( index >= 0 && index < splinePoints.Num() );
		splineTime[index] = time;
	}

	int GetSegmentTime( int index) {
		assert( index >= 0 && index < splinePoints.Num() );
		return splineTime[index];
	}
	void AddSegmentTime( int index, int time ) {
		assert( index >= 0 && index < splinePoints.Num() );
		splineTime[index] += time;
	}

	float TotalDistance();

	static anVec3 zero;

	int GetActiveSegment() {return activeSegment;}

	void SetActiveSegment( int i ) {
		//assert( i >= 0 && ( splinePoints.Num() > 0 && i < splinePoints.Num() ) );
		activeSegment = i;
	}

	int NumSegments() {return splinePoints.Num();}

	void SetColors( anVec3_t &path, anVec3_t &segment, anVec3_t &control, anVec3_t &active ) {
		pathColor = path;
		segmentColor = segment;
		controlColor = control;
		activeColor = active;
	}

	const char *GetName() {return name.c_str();}

	void SetName( const char *p ) { name = p;}

	bool ValidTime() {if ( dirty ) { buildSpline();}
		return ( bool )( splineTime.Num() > 0 && splineTime.Num() == splinePoints.Num() );}

	void SetTime(long t) {time = t;}

	void SetBaseTime(long t) {baseTime = t;}

protected:
	anStr name;
	float CalcSpline( int step, float tension );
	anList<anVec3 *> controlPoints;
	anList<anVec3 *> splinePoints;
	anList<float> splineTime;
	anVec3 *selected;
	anVec3 pathColor, segmentColor, controlColor, activeColor;
	float granularity;
	bool editMode;
	bool dirty;
	int activeSegment;
	long baseTime;
	long time;
	friend class idCamera;
};

class anSplinePosition : public anCameraPosition {
public:
	anSplinePosition( void );
	~anSplinePosition( void );

protected:
	anSplineList target;
};

#endif // !__SURFACE_SWEPTSPLINE_H__

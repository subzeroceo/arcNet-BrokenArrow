#ifndef __MATH_INTERPOLATE_H__
#define __MATH_INTERPOLATE_H__

/*
==============================================================================================

	Linear interpolation.

==============================================================================================
*/

template< class type >
class aRcInterpolation {
	friend class aRcTypeTools;
public:
						aRcInterpolation();
						virtual ~aRcInterpolation() { }

	void				Init( const float startTime, const float duration, const type &startValue, const type &endValue );
	void				SetStartTime( float time ) { this->startTime = time; }
	void				SetDuration( float duration ) { this->duration = duration; }
	void				SetStartValue( const type &startValue ) { this->startValue = startValue; }
	void				SetEndValue( const type &endValue ) { this->endValue = endValue; }

	type				GetCurrentValue( float time ) const;
	//virtual type		GetCurrentValue( float time ) const;
	virtual type		GetDeltaValue( float startTime, float endTime ) const;

	bool				IsDone( float time ) const { return ( time >= startTime + duration ); }

	float				GetStartTime( void ) const { return startTime; }
	float				GetEndTime( void ) const { return startTime + duration; }
	float				GetDuration( void ) const { return duration; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }

private:
	float				startTime;
	float				duration;
	type				startValue;
	type				endValue;
	mutable float		currentTime;
	mutable type		currentValue;
};

/*
====================
aRcInterpolation::aRcInterpolation
====================
*/
template< class type >
ARC_INLINE aRcInterpolation<type>::aRcInterpolation() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
}

/*
====================
aRcInterpolation::Init
====================
*/
template< class type >
ARC_INLINE void aRcInterpolation<type>::Init( const float startTime, const float duration, const type &startValue, const type &endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

/*
====================
aRcInterpolation::GetCurrentValue
====================
*/
template< class type >
ARC_INLINE type aRcInterpolation<type>::GetCurrentValue( float time ) const {
	float deltaTime;

	deltaTime = time - startTime;
	if ( time != currentTime ) {
		currentTime = time;
		if ( deltaTime <= 0 ) {
			currentValue = startValue;
		} else if ( deltaTime >= duration ) {
			currentValue = endValue;
		} else {
			currentValue = startValue + ( endValue - startValue ) * ( ( float ) deltaTime / duration );
		}
	}
	return currentValue;
}
template< class type >
ARC_INLINE type aRcInterpolation<type>::GetDeltaValue( float startTime, float endTime ) const {
	return GetCurrentValue(endTime) - GetCurrentValue(startTime);
}

ARC_INLINE float Linear( float frac ) {
	return frac;
}

ARC_INLINE float SinusoidalMidPoint( float frac ) {
	return arcMath::Sin( DEG2RAD(arcMath::MidPointLerp(0.0f, 60.0f, 90.0f, frac) ) );
}

/*
==============================================================================================
	Spheric Interloper--Spheric Interloper--Spheric Interloper--Spheric Interloper--Spheric
Spheric Interloper--Spheric Interloper--Spheric Interloper--Spheric Interloper--Spheric Interloper
==============================================================================================
*/
typedef float (*TimeManipFunc) ( float );
class aRcSphericalInterpolation : public aRcInterpolation<arcQuats> {
public:
						aRcSphericalInterpolation();
	virtual arcQuats GetCurrentValue( float time ) const;

	void				SetTimeFunction( TimeManipFunc func ) { timeFunc = func; }

protected:
	TimeManipFunc		timeFunc;
};

/*
====================
aRcSphericalInterpolation::aRcSphericalInterpolation
====================
*/
ARC_INLINE aRcSphericalInterpolation::aRcSphericalInterpolation() :
	aRcInterpolation<arcQuats>() {
	SetTimeFunction( SinusoidalMidPoint );
}

/*
====================
aRcSphericalInterpolation::GetCurrentValue
====================
*/
ARC_INLINE arcQuats aRcSphericalInterpolation::GetCurrentValue( float time ) const {
	float deltaTime;

	deltaTime = time - startTime;

	if ( time != currentTime ) {
		currentTime = time;
		if ( duration == 0.0f ) {
			currentValue = endValue;
		} else {
			currentValue.Slerp( startValue, endValue, timeFunc( ( float )deltaTime / duration) );
		}
	}
	return currentValue;
}

/*
==============================================================================================

	Continuous interpolation with linear acceleration and deceleration phase.
	The velocity is continuous but the acceleration is not.

ill finish mathclass later for not thats all there is bad connection and need to get work done
none of the below is finsiehd and all broken up

					theory of five-axis linkage motion control
The theory of five-axis motion linkage control, also known as five-axis kinematics,
	involves the movement and control of a five-axial system.
· first x% of the time it will be acceleration from Δ speed to v speed ·
· for deceleration is a = Δ v Δ t = v − v 0 t − t 0 ·

Trajectory Planning: Trajectory planning involves generating a smooth and feasible path for the
end effector to follow. This can be done using techniques such as cubic splines,
 B-splines, or trajectory interpolation. The trajectory planning algorithm takes into account
 factors like acceleration, velocity, and jerk limits to ensure smooth and controlled motion.

It's important to note that the specific equations and algorithms for five-axis motion linkage
control can be quite involved and may depend on the specific kinematic structure of the system
(e.g., articulated arm, robot manipulator, etc.) as well as the desired motion characteristics.

EXAMPLE:
import numpy as np

def interpolate(start, end, steps):
    result = []
    for i in range(steps):
        t = i / (steps - 1 )  # Interpolation parameter between 0 and 1
        interpolated_point = (1 - t) * start + t * end
        result.append(interpolated_point)
    return result

# Example usage
start_point = np.array([0, 0, 0, 0, 0])
end_point = np.array([10, 5, 3, 90, 45])
interpolated_points = interpolate(start_point, end_point, steps=10)

==============================================================================================
*/

template< class type >
class aRcLinearKinematicInterpolation  {
	friend class aRcTypeTools;
public:
						aRcLinearKinematicInterpolation();

	void				Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue );
	void				SetStartTime( float time ) { startTime = time; Invalidate(); }
	void				SetStartValue( const type &startValue ) { this->startValue = startValue; Invalidate(); }
	void				SetEndValue( const type &endValue ) { this->endValue = endValue; Invalidate(); }

	type				GetCurrentValue( float time ) const;
	type				GetCurrentSpeed( float time ) const;
	bool				IsDone( float time ) const { return ( time >= startTime + accelTime + linearTime + decelTime ); }

	float				GetStartTime( void ) const { return startTime; }
	float				GetEndTime( void ) const { return startTime + accelTime + linearTime + decelTime; }
	float				GetDuration( void ) const { return accelTime + linearTime + decelTime; }
	float				GetAcceleration( void ) const { return accelTime; }
	float				GetDeceleration( void ) const { return decelTime; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }

private:
	float				startTime;
	float				accelTime;
	float				linearTime;
	float				decelTime;
	type				startValue;
	type				endValue;
	mutable aRcExtrapolation<type> extrapolate;

	void				Invalidate( void );
	void				SetPhase( float time ) const;
};

/*
====================
aRcLinearKinematicInterpolation::aRcLinearKinematicInterpolation
====================
*/
template< class type >
ARC_INLINE aRcLinearKinematicInterpolation<type>::aRcLinearKinematicInterpolation() {
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	endValue = startValue;
}

/*
====================
aRcLinearKinematicInterpolation::Init
====================
*/
template< class type >
ARC_INLINE void aRcLinearKinematicInterpolation<type>::Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue ) {
	type speed;

	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if ( duration <= 0.0f ) {
		return;
	}

	if ( this->accelTime + this->decelTime > duration ) {
		this->accelTime = this->accelTime * duration / ( this->accelTime + this->decelTime );
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	speed = ( endValue - startValue ) * ( 1000.0f / ( ( float ) this->linearTime + ( this->accelTime + this->decelTime ) * 0.5f ) );

	if ( this->accelTime ) {
		extrapolate.Init( startTime, this->accelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_ACCELLINEAR );
	} else if ( this->linearTime ) {
		extrapolate.Init( startTime, this->linearTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_LINEAR );
	} else {
		extrapolate.Init( startTime, this->decelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_DECELLINEAR );
	}
}

/*
====================
aRcLinearKinematicInterpolation::Invalidate
====================
*/
template< class type >
ARC_INLINE void aRcLinearKinematicInterpolation<type>::Invalidate( void ) {
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
aRcLinearKinematicInterpolation::SetPhase
====================
*/
template< class type >
ARC_INLINE void aRcLinearKinematicInterpolation<type>::SetPhase( float time ) const {
	float deltaTime;

	deltaTime = time - startTime;
	if ( deltaTime < accelTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELLINEAR ) {
			extrapolate.Init( startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELLINEAR );
		}
	} else if ( deltaTime < accelTime + linearTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR ) {
			extrapolate.Init( startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * ( accelTime * 0.001f * 0.5f ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR );
		}
	} else {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELLINEAR ) {
			extrapolate.Init( startTime + accelTime + linearTime, decelTime, endValue - ( extrapolate.GetSpeed() * ( decelTime * 0.001f * 0.5f ) ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELLINEAR );
		}
	}
}

/*
====================
aRcLinearKinematicInterpolation::GetCurrentValue
====================
*/
template< class type >
ARC_INLINE type aRcLinearKinematicInterpolation<type>::GetCurrentValue( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
aRcLinearKinematicInterpolation::GetCurrentSpeed
====================
*/
template< class type >
ARC_INLINE type aRcLinearKinematicInterpolation<type>::GetCurrentSpeed( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}

/*
==============================================================================================

	Continuous interpolation with sinusoidal acceleration and deceleration phase.
	Both the velocity and acceleration are continuous.

==============================================================================================
*/

template< class type >
class aRcInterpolateSineKinematics  {
	friend class aRcTypeTools;
public:
						aRcInterpolateSineKinematics();

	void				Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue );
	void				SetStartTime( float time ) { startTime = time; Invalidate(); }
	void				SetStartValue( const type &startValue ) { this->startValue = startValue; Invalidate(); }
	void				SetEndValue( const type &endValue ) { this->endValue = endValue; Invalidate(); }

	type				GetCurrentValue( float time ) const;
	type				GetCurrentSpeed( float time ) const;
	bool				IsDone( float time ) const { return ( time >= startTime + accelTime + linearTime + decelTime ); }

	float				GetStartTime( void ) const { return startTime; }
	float				GetEndTime( void ) const { return startTime + accelTime + linearTime + decelTime; }
	float				GetDuration( void ) const { return accelTime + linearTime + decelTime; }
	float				GetAcceleration( void ) const { return accelTime; }
	float				GetDeceleration( void ) const { return decelTime; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }

private:
	float				startTime;
	float				accelTime;
	float				linearTime;
	float				decelTime;
	type				startValue;
	type				endValue;
	mutable aRcExtrapolation<type> extrapolate;

	void				Invalidate( void );
	void				SetPhase( float time ) const;
};

/*
====================
aRcInterpolateSineKinematics::aRcInterpolateSineKinematics
====================
*/
template< class type >
ARC_INLINE aRcInterpolateSineKinematics<type>::aRcInterpolateSineKinematics() {
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	endValue = startValue;
}

/*
====================
aRcInterpolateSineKinematics::Init
====================
*/
template< class type >
ARC_INLINE void aRcInterpolateSineKinematics<type>::Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue ) {
	type speed;

	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if ( duration <= 0.0f ) {
		return;
	}

	if ( this->accelTime + this->decelTime > duration ) {
		this->accelTime = this->accelTime * duration / ( this->accelTime + this->decelTime );
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	speed = ( endValue - startValue ) * ( 1000.0f / ( ( float ) this->linearTime + ( this->accelTime + this->decelTime ) * arcMath::SQRT_1OVER2 ) );

	if ( this->accelTime ) {
		extrapolate.Init( startTime, this->accelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_ACCELSINE );
	} else if ( this->linearTime ) {
		extrapolate.Init( startTime, this->linearTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_LINEAR );
	} else {
		extrapolate.Init( startTime, this->decelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_DECELSINE );
	}
}
ARC_INLINE void aRcInterpolateSineKinematics<type>::Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type& startValue, const type& endValue ) {
    this->startTime = startTime;
    this->startValue = startValue;
    this->endValue = endValue;

    if ( duration <= 0.0f ) {
        return;
    }

    // Calculate the linear time as the difference between duration, acceleration time, and deceleration time
    float linearTime = duration - accelTime - decelTime;

    if ( accelTime + decelTime > duration ) {
        // Adjust the acceleration and deceleration times proportionally to fit within the provided duration
        float factor = accelTime / ( accelTime + decelTime );
        accelTime *= factor;
        decelTime *= ( 1 - factor );
    }

    // Calculate the speed based on the difference between the end and start values
    // The linear time and (acceleration time + deceleration time) are used to adjust the speed calculation
    type speed = ( endValue - startValue ) *  (1000.0f / (linearTime + ( accelTime + decelTime ) * arcMath::SQRT_1OVER2 ) );

    ExtrapolationType extrapolationType = EXTRAPOLATION_DECELSINE;
    if ( accelTime > 0 ) {
        // If acceleration time is greater than 0, use ACCELSINE extrapolation type
        extrapolationType = EXTRAPOLATION_ACCELSINE;
    } else if ( linearTime > 0 ) {
        // If linear time is greater than 0, use LINEAR extrapolation type
        extrapolationType = EXTRAPOLATION_LINEAR;
    }

    // Initialize the extrapolate object with the appropriate parameters based on the extrapolation type
    extrapolate.Init( startTime, accelTime > 0 ? accelTime : ( linearTime > 0 ? linearTime : decelTime ), startValue, ( s tartValue - startValue ), speed, extrapolationType );
}

/*
====================
aRcInterpolateSineKinematics::Invalidate
====================
*/
template< class type >
ARC_INLINE void aRcInterpolateSineKinematics<type>::Invalidate( void ) {
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
aRcInterpolateSineKinematics::SetPhase
====================
*/
template< class type >
ARC_INLINE void aRcInterpolateSineKinematics<type>::SetPhase( float time ) const {
	float deltaTime;

	deltaTime = time - startTime;
	if ( deltaTime < accelTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELSINE ) {
			extrapolate.Init( startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELSINE );
		}
	} else if ( deltaTime < accelTime + linearTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR ) {
			extrapolate.Init( startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * ( accelTime * 0.001f * arcMath::SQRT_1OVER2 ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR );
		}
	} else {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELSINE ) {
			extrapolate.Init( startTime + accelTime + linearTime, decelTime, endValue - ( extrapolate.GetSpeed() * ( decelTime * 0.001f * arcMath::SQRT_1OVER2 ) ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELSINE );
		}
	}
}

/*
====================
aRcInterpolateSineKinematics::GetCurrentValue
====================
*/
template< class type >
ARC_INLINE type aRcInterpolateSineKinematics<type>::GetCurrentValue( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
aRcInterpolateSineKinematics::GetCurrentSpeed
====================
*/
template< class type >
ARC_INLINE type aRcInterpolateSineKinematics<type>::GetCurrentSpeed( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}

//==============================================================================================
//
//	Hermite interpolation.
//
//==============================================================================================

template< class type >
class aRcHermiteInterpolator {
public:
						aRcHermiteInterpolator();
	void				Init( const int startTime, const int duration, const type startValue, const type endValue, float S1, float S2 );
	void				Init( const int startTime, const int duration, const type startValue, const type endValue );
	void				SetStartTime( int time ) { this->startTime = time; }
	void				SetDuration( int duration ) { this->duration = duration; }
	void				SetStartValue( const type &start ) { this->startValue = start; }
	void				SetEndValue( const type &end ) { this->endValue = end; }
	void				SetHermiteParms( float S1, float S2 ) { this->S1 = S1; this->S2 = S2; }
	type				GetCurrentValue( int time ) const;
	bool				IsDone( int time ) const { return ( time >= startTime + duration ); }
	int					GetStartTime( void ) const { return startTime; }
	int					GetDuration( void ) const { return duration; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }
	float				GetS1( void ) const { return S1; }
	float				GetS2( void ) const { return S2; }
	float				HermiteAlpha(float t) const;

private:
	float				S1;				// Slope of curve leaving start point
	float				S2;				// Slope of curve arriving at end point
	int					startTime;
	int					duration;
	type				startValue;
	type				endValue;
	mutable int			currentTime;
	mutable type		currentValue;
};

/*
====================
aRcHermiteInterpolator::aRcHermiteInterpolator
====================
*/
template< class type >
ARC_INLINE aRcHermiteInterpolator<type>::aRcHermiteInterpolator() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
	S1 = S2 = 1;
}

/*
====================
aRcHermiteInterpolator::Init
====================
*/
template< class type >
ARC_INLINE void aRcHermiteInterpolator<type>::Init( const int startTime, const int duration, const type startValue, const type endValue, const float S1, const float S2 ) {
	this->S1 = S1;
	this->S2 = S2;
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

/*
====================
aRcHermiteInterpolator::Init
====================
*/
template< class type >
ARC_INLINE void aRcHermiteInterpolator<type>::Init( const int startTime, const int duration, const type startValue, const type endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

/*
====================
aRcHermiteInterpolator::GetCurrentValue
====================
*/
template< class type >
ARC_INLINE type aRcHermiteInterpolator<type>::GetCurrentValue( int time ) const {
	int deltaTime;

	deltaTime = time - startTime;
	if ( time != currentTime ) {
		currentTime = time;
		if ( deltaTime <= 0 ) {
			currentValue = startValue;
		} else if ( deltaTime >= duration ) {
			currentValue = endValue;
		} else {
			currentValue = startValue + ( endValue - startValue ) * HermiteAlpha( ( float ) deltaTime / duration );
		}
	}
	return currentValue;
}

// Returns an alpha value [0..1] based on Hermite Parameters N1, N2, S1, S2 and an input alpha 't'
template< class type >
ARC_INLINE float aRcHermiteInterpolator<type>::HermiteAlpha(const float t) const {
	float N1 = 0.0f;
	float N2 = 1.0f;
	float tSquared = t * t;
	float tCubed = tSquared * t;
	return	( 2 * tCubed - 3 * tSquared + 1 ) * N1 +
			( -2 * tCubed + 3 * tSquared ) * N2 +
			( tCubed - 2 * tSquared + t ) * S1 +
			( tCubed - tSquared )*S2;
}

//==============================================================================================
//
// TCB Spline Interpolation
//
// Defines a Kochanek-Bartels spline, in short a Hermite spline with formulae to calculate the tangents
// Requires extra points at the ends, try duplicating first and last
//==============================================================================================
class aRcTCBSpline {
	//TODO: Make a template like the others so it can handle something other than vec3 types
public:
					aRcTCBSpline() { Clear(); }
	void			Clear();
	void			AddPoint( const arcVec3 &point );
	void			SetControls( float tension, float continuity, float bias );
	arcVec3			GetValue( float alpha );

	float			tension;		// How much of an increase in sharpness does the curve bend?
	float			continuity;		// How rapid is the change in speed and direction?
	float			bias;			// What is the direction of the curve as it passes through the key point?
	arcNetList<arcVec3> nodes;			// control points

protected:
	arcVec3			GetNode( int i );
	arcVec3			IncomingTangent( int i );
	arcVec3			OutgoingTangent( int i );
};

ARC_INLINE void aRcTCBSpline::Clear() {
	tension = continuity = bias = 0.0f;
	nodes.Clear();
}

ARC_INLINE void aRcTCBSpline::AddPoint( const arcVec3 &point ) {
	nodes.Append( point );
}

ARC_INLINE void aRcTCBSpline::SetControls( float tension, float continuity, float bias ) {
	this->tension = arcMath::ClampFloat( 0.0f, 1.0f, tension );
	this->continuity = arcMath::ClampFloat( 0.0f, 1.0f, continuity );
	this->bias = arcMath::ClampFloat( 0.0f, 1.0f, bias );
}

ARC_INLINE arcVec3 aRcTCBSpline::GetNode( int i ) {
	// Clamping has the effect of having duplicate nodes beyond the array boundaries
	int index = arcMath::ClampInt( 0, nodes.Num()-1, i );
	return nodes[index];
}

ARC_INLINE arcVec3 aRcTCBSpline::IncomingTangent( int i ) {
	return	( ( 1.0f - tension )*( 1.0f - continuity )*( 1.0f + bias ) * 0.5f) * (GetNode(i) - GetNode(i-1)) +
			( ( 1.0f - tension )*( 1.0f + continuity )*( 1.0f - bias ) * 0.5f) * (GetNode(i+1) - GetNode(i));
}

ARC_INLINE arcVec3 aRcTCBSpline::OutgoingTangent( int i ) {
	return	((1.0f-tension)*( 1.0f + continuity )*( 1.0f + bias ) * 0.5f) * (GetNode(i) - GetNode(i-1)) +
			((1.0f-tension)*( 1.0f - continuity )*( 1.0f - bias ) * 0.5f) * (GetNode(i+1) - GetNode(i));
}

ARC_INLINE arcVec3 aRcTCBSpline::GetValue( float alpha ) {
	float t = arcMath::ClampFloat(0.0f, 1.0f, alpha);
	int numNodes = nodes.Num();
	int numSegments = numNodes-1;
	int startNode = t * numSegments;
	t = ( t * numSegments ) - startNode;		// t = alpha within this segment

	// Calculate hermite parameters
	arcVec3 N1 = GetNode( startNode );
	arcVec3 N2 = GetNode( startNode + 1 );
	arcVec3 S1 = OutgoingTangent( startNode );
	arcVec3 S2 = IncomingTangent( startNode + 1 );

	float tSquared = t*t;
	float tCubed = tSquared*t;
	return	( 2 * tCubed - 3 * tSquared + 1 ) * N1 +
			( -2 * tCubed + 3 * tSquared ) * N2 +
			( tCubed - 2 * tSquared + t ) * S1 +
			( tCubed - tSquared )*S2;
}


//==============================================================================================
//
//	Saw interpolation.
//
//	Interpolates from startValue to endValue to startValue over duration
//==============================================================================================

template< class type >
class aRcSawInterpolation {
public:
						aRcSawInterpolation();
	void				Init( const int startTime, const int duration, const type startValue, const type endValue );
	void				SetStartTime( int time ) { this->startTime = time; }
	void				SetDuration( int duration ) { this->duration = duration; }
	void				SetStartValue( const type &start ) { this->startValue = start; }
	void				SetEndValue( const type &end ) { this->endValue = end; }
	type				GetCurrentValue( int time ) const;
	bool				IsDone( int time ) const { return ( time >= startTime + duration ); }
	int					GetStartTime( void ) const { return startTime; }
	int					GetDuration( void ) const { return duration; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }

private:
	int					startTime;
	int					duration;
	type				startValue;
	type				endValue;
	mutable int			currentTime;
	mutable type		currentValue;
};

template< class type >
ARC_INLINE aRcSawInterpolation<type>::aRcSawInterpolation() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
}

template< class type >
ARC_INLINE void aRcSawInterpolation<type>::Init( const int startTime, const int duration, const type startValue, const type endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

template< class type >
ARC_INLINE type aRcSawInterpolation<type>::GetCurrentValue( int time ) const {
	int deltaTime;

	deltaTime = time - startTime;
	if ( time != currentTime ) {
		currentTime = time;
		if ( deltaTime <= 0 ) {
			currentValue = startValue;
		}
		else if ( deltaTime >= duration ) {
			currentValue = startValue;
		}
		else {
			float frac = (( float ) deltaTime / duration );
			if (frac < 0.5f) {
				currentValue = startValue + ( endValue - startValue ) * frac * 2.0f;
			}
			else {
				currentValue = startValue + ( endValue - startValue ) * (1.0f - frac) * 2.0f;
			}
		}
	}
	return currentValue;
}


//==============================================================================================
//
//	Sine wave oscillator
//
//	Oscillates between min and max over given period
//==============================================================================================

template< class type >
class aRcSinOscillator {
public:
						aRcSinOscillator();
	void				Init( const int startTime, const int period, const type min, const type max );
	void				SetStartTime( int time ) { this->startTime = time; }
	void				SetPeriod( int period ) { this->period = period; }
	void				SetMinValue( const type &min ) { this->minValue = min; }
	void				SetMaxValue( const type &max ) { this->MaxValue = max; }
	type				GetCurrentValue( int time ) const;
	int					GetStartTime( void ) const { return startTime; }
	int					GetPeriod( void ) const { return period; }
	const type &		GetMinValue( void ) const { return minValue; }
	const type &		GetMaxValue( void ) const { return maxValue; }

private:
	int					startTime;
	int					period;
	type				minValue;
	type				maxValue;
	mutable int			currentTime;
	mutable type		currentValue;
};

template< class type >
ARC_INLINE aRcSinOscillator<type>::aRcSinOscillator() {
	currentTime = startTime = period = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	minValue = maxValue = currentValue;
}

template< class type >
ARC_INLINE void aRcSinOscillator<type>::Init( const int startTime, const int period, const type minValue, const type maxValue ) {
	this->startTime = startTime;
	this->period = period;
	this->minValue = minValue;
	this->maxValue = maxValue;
	this->currentTime = startTime - 1;
	this->currentValue = minValue;
}

template< class type >
ARC_INLINE type aRcSinOscillator<type>::GetCurrentValue( int time ) const {

	if ( time != currentTime ) {
		currentTime = time;

		float deltaTime = period == 0 ? 0.0f : ( time - startTime ) / ( float ) period;
		float s = (1.0f + ( float ) sin(deltaTime * arcMath::TWO_PI)) * 0.5f;
		currentValue = minValue + s * (maxValue - minValue);
	}
	return currentValue;
}

#endif

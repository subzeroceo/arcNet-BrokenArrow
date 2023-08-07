#ifndef __MATH_INTERPOLATE_H__
#define __MATH_INTERPOLATE_H__

/*
==============================================================================================

	Linear interpolation.

==============================================================================================
*/

template<class type>
class anInterpolation {
	friend class anTypeTools;
public:
						anInterpolation();
						virtual ~anInterpolation() { }

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
anInterpolation::anInterpolation
====================
*/
template<class type>
inline anInterpolation<type>::anInterpolation() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
}

/*
====================
anInterpolation::Init
====================
*/
template<class type>
inline void anInterpolation<type>::Init( const float startTime, const float duration, const type &startValue, const type &endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

/*
====================
anInterpolation::GetCurrentValue
====================
*/
template<class type>
inline type anInterpolation<type>::GetCurrentValue( float time ) const {
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
template<class type>
inline type anInterpolation<type>::GetDeltaValue( float startTime, float endTime ) const {
	return GetCurrentValue(endTime) - GetCurrentValue( startTime);
}

inline float Linear( float frac ) {
	return frac;
}

inline float SinusoidalMidPoint( float frac ) {
	return anMath::Sin( DEG2RAD(anMath::MidPointLerp(0.0f, 60.0f, 90.0f, frac) ) );
}

/*
==============================================================================================
	Spheric Interloper--Spheric Interloper--Spheric Interloper--Spheric Interloper--Spheric
Spheric Interloper--Spheric Interloper--Spheric Interloper--Spheric Interloper--Spheric Interloper
==============================================================================================
*/
typedef float (*TimeManipFunc) ( float );
class anSphericalInterpolation : public anInterpolation<anQuats> {
public:
						anSphericalInterpolation();
	virtual anQuats GetCurrentValue( float time ) const;

	void				SetTimeFunction( TimeManipFunc func ) { timeFunc = func; }

protected:
	TimeManipFunc		timeFunc;
};

/*
====================
anSphericalInterpolation::anSphericalInterpolation
====================
*/
inline anSphericalInterpolation::anSphericalInterpolation() :
	anInterpolation<anQuats>() {
	SetTimeFunction( SinusoidalMidPoint );
}

/*
====================
anSphericalInterpolation::GetCurrentValue
====================
*/
inline anQuats anSphericalInterpolation::GetCurrentValue( float time ) const {
	float deltaTime = time - startTime;

	if ( time != currentTime ) {
		currentTime = time;
		if ( duration == 0.0f ) {
			currentValue = endValue;
		} else {
			currentValue.Slerp( startValue, endValue, timeFunc( ( float )deltaTime / duration ) );
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

def interpolate( start, end, steps):
    result = []
    for i in range( steps):
        t = i / ( steps - 1 )  # Interpolation parameter between 0 and 1
        interpolated_point = (1 - t) * start + t * end
        result.append(interpolated_point)
    return result

# Example usage
start_point = np.array([0, 0, 0, 0, 0])
end_point = np.array([10, 5, 3, 90, 45])
interpolated_points = interpolate( start_point, end_point, steps=10)

==============================================================================================
*/

template<class type>
class anLinearKinematicInterpolation  {
	friend class anTypeTools;
public:
						anLinearKinematicInterpolation();

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
	mutable anExtrapolation<type> extrapolate;

	void				Invalidate( void );
	void				SetPhase( float time ) const;
};

/*
====================
anLinearKinematicInterpolation::anLinearKinematicInterpolation
====================
*/
template<class type>
inline anLinearKinematicInterpolation<type>::anLinearKinematicInterpolation() {
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	endValue = startValue;
}

/*
====================
anLinearKinematicInterpolation::Init
====================
*/
template<class type>
inline void anLinearKinematicInterpolation<type>::Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue ) {
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
anLinearKinematicInterpolation::Invalidate
====================
*/
template<class type>
inline void anLinearKinematicInterpolation<type>::Invalidate( void ) {
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
anLinearKinematicInterpolation::SetPhase
====================
*/
template<class type>
inline void anLinearKinematicInterpolation<type>::SetPhase( float time ) const {
	float deltaTime = time - startTime;
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
anLinearKinematicInterpolation::GetCurrentValue
====================
*/
template<class type>
inline type anLinearKinematicInterpolation<type>::GetCurrentValue( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
anLinearKinematicInterpolation::GetCurrentSpeed
====================
*/
template<class type>
inline type anLinearKinematicInterpolation<type>::GetCurrentSpeed( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}

/*
==============================================================================================

	Continuous interpolation with sinusoidal acceleration and deceleration phase.
	Both the velocity and acceleration are continuous.

==============================================================================================
*/

template<class type>
class anInterpolateSineKinematics  {
	friend class anTypeTools;
public:
						anInterpolateSineKinematics();

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
	mutable anExtrapolation<type> extrapolate;

	void				Invalidate( void );
	void				SetPhase( float time ) const;
};

/*
====================
anInterpolateSineKinematics::anInterpolateSineKinematics
====================
*/
template<class type>
inline anInterpolateSineKinematics<type>::anInterpolateSineKinematics() {
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	endValue = startValue;
}

/*
====================
anInterpolateSineKinematics::Init
====================
*/
template<class type>
inline void anInterpolateSineKinematics<type>::Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue ) {
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
	speed = ( endValue - startValue ) * ( 1000.0f / ( ( float ) this->linearTime + ( this->accelTime + this->decelTime ) * anMath::SQRT_1OVER2 ) );

	if ( this->accelTime ) {
		extrapolate.Init( startTime, this->accelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_ACCELSINE );
	} else if ( this->linearTime ) {
		extrapolate.Init( startTime, this->linearTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_LINEAR );
	} else {
		extrapolate.Init( startTime, this->decelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_DECELSINE );
	}
}
inline void anInterpolateSineKinematics<type>::Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type& startValue, const type& endValue ) {
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
    type speed = ( endValue - startValue ) *  (1000.0f / (linearTime + ( accelTime + decelTime ) * anMath::SQRT_1OVER2 ) );

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
anInterpolateSineKinematics::Invalidate
====================
*/
template<class type>
inline void anInterpolateSineKinematics<type>::Invalidate( void ) {
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
anInterpolateSineKinematics::SetPhase
====================
*/
template<class type>
inline void anInterpolateSineKinematics<type>::SetPhase( float time ) const {
	float deltaTime = time - startTime;
	if ( deltaTime < accelTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELSINE ) {
			extrapolate.Init( startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELSINE );
		}
	} else if ( deltaTime < accelTime + linearTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR ) {
			extrapolate.Init( startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * ( accelTime * 0.001f * anMath::SQRT_1OVER2 ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR );
		}
	} else {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELSINE ) {
			extrapolate.Init( startTime + accelTime + linearTime, decelTime, endValue - ( extrapolate.GetSpeed() * ( decelTime * 0.001f * anMath::SQRT_1OVER2 ) ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELSINE );
		}
	}
}

/*
====================
anInterpolateSineKinematics::GetCurrentValue
====================
*/
template<class type>
inline type anInterpolateSineKinematics<type>::GetCurrentValue( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
anInterpolateSineKinematics::GetCurrentSpeed
====================
*/
template<class type>
inline type anInterpolateSineKinematics<type>::GetCurrentSpeed( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}
/*
=============================================================================

	Lanczos Interpolation - AKA Lanczos Kernel

=============================================================================
*/

template <class type>
class anLanczosInterpolator {
public:
    static				anLanczosInterpolator();
    void			Init( const int startTime, const int duration, const type startValue, const type endValue, const int kernelSize );
    void			SetStartTime( int time ) { this->startTime = time; }
    void			SetDuration( int duration ) { this->duration = duration; }
    void			SetStartValue( const type& start ) { this->startValue = start; }
    void			SetEndValue( const type& end ) { this->endValue = end; }
    type			GetCurrentValue( int time ) const;
    bool			IsDone( int time ) const { return ( time >= startTime + duration ); }
    int				GetStartTime( void ) const { return startTime; }
    int				GetDuration( void)  const { return duration; }
    const type& 	GetStartValue(void ) const { return startValue; }
    const type&		GetEndValue( void ) const { return endValue; }
    int				GetKernelSize( void ) const { return kernelSize; }

    float			LanczosKernel(float x ) const;
    float			KernelScale(float x ) const;

private:
    int 			startTime;
    int				duration;
    type			startValue;
    type			endValue;
    int				kernelSize;
    mutable int		currentTime;
    mutable type	currentValue;

private:

		anLanczosInterpolator() {
		// Initialize lookup table
		for ( int i = 0; i < KERNEL_SIZE * 2; i++ ) {
			float x = (float)i - KERNEL_SIZE;
		kernelTable[i] = sinc( x ) * sinc( x / KERNEL_SIZE );
		}
	}

	// receieves the val, calculates the Lanczos kernel formula with the table
	// returns with the index table lookup
	float GetKernelValue( float x ) {
		x = anMath::Clamp( x, -KERNEL_SIZE, KERNEL_SIZE );
		int index = ( int ) x + KERNEL_SIZE;
		return kernelTable[index];
	}

	 // we want to ensure the kernel is encapsulated securely hidden no FKC jokes..
 	float kernelTable[KERNEL_SIZE*2];

};

/*
====================
anLanczosInterpolator::anLanczosInterpolator
====================
*/
template<class type>
inline anLanczosInterpolator<type>::anLanczosInterpolator() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
	kernelSize = currentValue = 1;
}

/*
====================
anLanczosInterpolator::Init
====================
*/
template<class type>
inline void anLanczosInterpolator<type>::Init( const int startTime, const int duration, const type startValue, const type endValue, const int kernelSize ) {
	this->kernelSize = kernelSize;
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

template<class type>
inline anLanczosInterpolator::GetCurrentValue( int time ) const {
	int lanczosTime = time - startTime;
	if ( time != currentTime ) {
		currentTime = time;
		if ( lanczosTime <= 0 ) {
			currentValue = startValue;
		} else if ( lanczosTime >= duration ) {
			currentValue = endValue;
		} else {
            // Calculate the interpolation value using the Lanczos kernel
            type fraction = static_cast<type>( lanczosTime ) / duration;
            type interpolatedValue = 0.0f;
            for ( int i = 0; i < kernelSize; i++ ) {
                type weight = LanczosKernel( static_cast<type>( i ) - fraction );
				interpolatedValue += weight * ( endValue - startValue ) * fraction + startValue;
				//interpolatedValue = startValue + ( endValue - startValue ) * LanczosKernel( ( float ) lanczosTime / duration );
            }
            currentValue = interpolatedValue;
        }
    }
    return currentValue;
}

template<class type>
inline const float anLanczosInterpolator::LanczosKernel( const float x ) const {
    if ( x == 0.0f ) {
        return 1.0f;
    } else if ( anMath::Abs( x ) < kernelSize ) {
        float piX = anMath::PI * x;
        return kernelSize * anMath::Sin( piX ) * anMath::Sin( piX / kernelSize ) / ( piX * piX );
    } else {
        return 0.0f;
    }
}


// Lanczos kernel function
template<class type>
inline  const float anLanczosInterpolator::KernelScale( float x ) const {
    const float radius = 2.0f;
    if ( x > -radius && x < radius ) {
        return sinc( x ) * sinc( x / radius );
    }
    return 0.0f;
}


// Sinc function
float sinc( loat x ) {
    if ( x == 0.0f ) {
        return 1.0f;
    }
    return sin(x * M_PI) / (x * M_PI);
}

//==============================================================================================
//
//	Hermite interpolation.
//
//==============================================================================================

template<class type>
class anHermiteInterpolator {
public:
						anHermiteInterpolator();
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
anHermiteInterpolator::anHermiteInterpolator
====================
*/
template<class type>
inline anHermiteInterpolator<type>::anHermiteInterpolator() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
	S1 = S2 = 1;
}

/*
====================
anHermiteInterpolator::Init
====================
*/
template<class type>
inline void anHermiteInterpolator<type>::Init( const int startTime, const int duration, const type startValue, const type endValue, const float S1, const float S2 ) {
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
anHermiteInterpolator::Init
====================
*/
template<class type>
inline void anHermiteInterpolator<type>::Init( const int startTime, const int duration, const type startValue, const type endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

/*
====================
anHermiteInterpolator::GetCurrentValue
====================
*/
template<class type>
inline type anHermiteInterpolator<type>::GetCurrentValue( int time ) const {
	int deltaTime = time - startTime;
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
template<class type>
inline float anHermiteInterpolator<type>::HermiteAlpha(const float t) const {
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
// CatMull-Rom-Spline Interpolation
// this class wouldnt ever gotten implemented if it wasnt for Cody. Simple
// and i love interpolation mathematics but i overlooked it. Thanks
//==============================================================================================


class anCatMullROMSpline {
public:
					anCatMullROMSpline() { Clear(); }
	void			Clear();

					// add your nodes, updates apply with adding numNodes.
					// This avoids having to call nodes.Num() repeatedly.
					// Otherwise, the nodes/tangent functions are sufficient and fully interpolate Catmull-Rom splines
	void			AddPoint( const anVec3 &point );
	anVec3			GetValue( float alpha );

	float			tension;
	float			continuity;
	float			bias;			
	anList<anVec3>	nodes;			// control points

protected:
	anVec3			GetNode( int i );

					// This implements cubic Catmull-Rom spline interpolation using the incoming
					// and outgoing tangents.
	anVec3			IncomingTangent( int i );
	anVec3			OutgoingTangent( int i );
};

inline void anCatMullROMSpline::Clear() {
	tension = continuity = bias = 0.0f;
	nodes.Clear();
}

inline void anCatMullROMSpline::AddPoint( const anVec3 &point ) {
	nodes.Append( point );
	numNodes++;
}

inline anVec3 anCatMullROMSpline::GetNode( int i ) {
	// Clamping has the effect of having duplicate nodes beyond the array boundaries
	int index = anMath::ClampInt( 0, nodes.Num()-1, i );
	return nodes[index];
}

inline anVec3 anCatMullROMSpline::IncomingTangent( int i ) {
	anVec3 p0 = GetNode( i - 1 );
	anVec3 p1 = GetNode( i );
	return ( p1 - p0 ).Normalize();
}

inline anVec3 anCatMullROMSpline::OutgoingTangent( int i ) {
	anVec3 p1 = GetNode( i );
	anVec3 p2 = GetNode( i + 1 );
	return ( p2 - p1 ).Normalize();
}

inline anVec3 anCatMullROMSpline::GetValue( float t ) {
	int p0 = Floor( t );
	int p1 = p0 + 1;
	float t = t - ( float )p0;

	anVec3 a = GetNode( p0 );
	anVec3 b = GetNode( p1 );

	anVec3 tangent0 = IncomingTangent( p0 );
	anVec3 tangent1 = OutgoingTangent( p1 );

	anVec3 result = a * ( 2 * t * t * t - 3 * t * t + 1 ) + 
					b * ( -2 * t * t * t + 3 * t * t ) +
			tangent0 * ( t * t * t - 2 * t * t + t ) +
			tangent1 * ( t * t * t - t * t );

  return result;
}



//==============================================================================================
//
// TCB Spline Interpolation
//
// Defines a Kochanek-Bartels spline, in short a Hermite spline with formulae to calculate the tangents
// Requires extra points at the ends, try duplicating first and last
//==============================================================================================


class anTCBSpline {
	//TODO: Make a template like the others so it can handle something other than vec3 types
public:
					anTCBSpline() { Clear(); }
	void			Clear();
	void			AddPoint( const anVec3 &point );
	void			SetControls( float tension, float continuity, float bias );
	anVec3			GetValue( float alpha );

	float			tension;		// How much of an increase in sharpness does the curve bend?
	float			continuity;		// How rapid is the change in speed and direction?
	float			bias;			// What is the direction of the curve as it passes through the key point?
	anList<anVec3> nodes;			// control points

protected:
	anVec3			GetNode( int i );
	anVec3			IncomingTangent( int i );
	anVec3			OutgoingTangent( int i );
};

inline void anTCBSpline::Clear() {
	tension = continuity = bias = 0.0f;
	nodes.Clear();
}

inline void anTCBSpline::AddPoint( const anVec3 &point ) {
	nodes.Append( point );
}

inline void anTCBSpline::SetControls( float tension, float continuity, float bias ) {
	this->tension = anMath::ClampFloat( 0.0f, 1.0f, tension );
	this->continuity = anMath::ClampFloat( 0.0f, 1.0f, continuity );
	this->bias = anMath::ClampFloat( 0.0f, 1.0f, bias );
}

inline anVec3 anTCBSpline::GetNode( int i ) {
	// Clamping has the effect of having duplicate nodes beyond the array boundaries
	int index = anMath::ClampInt( 0, nodes.Num()-1, i );
	return nodes[index];
}

inline anVec3 anTCBSpline::IncomingTangent( int i ) {
	return	( ( 1.0f - tension )*( 1.0f - continuity )*( 1.0f + bias ) * 0.5f) * (GetNode( i ) - GetNode(i-1)) +
			( ( 1.0f - tension )*( 1.0f + continuity )*( 1.0f - bias ) * 0.5f) * (GetNode(i+1) - GetNode( i ));
}

inline anVec3 anTCBSpline::OutgoingTangent( int i ) {
	return	((1.0f-tension)*( 1.0f + continuity )*( 1.0f + bias ) * 0.5f) * (GetNode( i ) - GetNode(i-1)) +
			((1.0f-tension)*( 1.0f - continuity )*( 1.0f - bias ) * 0.5f) * (GetNode(i+1) - GetNode( i ));
}

inline anVec3 anTCBSpline::GetValue( float alpha ) {
	float t = anMath::ClampFloat(0.0f, 1.0f, alpha);
	int numNodes = nodes.Num();
	int numSegments = numNodes-1;
	int startNode = t * numSegments;
	t = ( t * numSegments ) - startNode;		// t = alpha within this segment

	// Calculate hermite parameters
	anVec3 N1 = GetNode( startNode );
	anVec3 N2 = GetNode( startNode + 1 );
	anVec3 S1 = OutgoingTangent( startNode );
	anVec3 S2 = IncomingTangent( startNode + 1 );

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

template<class type>
class anSawInterpolation {
public:
						anSawInterpolation();
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

template<class type>
inline anSawInterpolation<type>::anSawInterpolation() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
}

template<class type>
inline void anSawInterpolation<type>::Init( const int startTime, const int duration, const type startValue, const type endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

template<class type>
inline type anSawInterpolation<type>::GetCurrentValue( int time ) const {
	int deltaTime = time - startTime;
	if ( time != currentTime ) {
		currentTime = time;
		if ( deltaTime <= 0 ) {
			currentValue = startValue;
		} else if ( deltaTime >= duration ) {
			currentValue = startValue;
		} else {
			float frac = ( ( float ) deltaTime / duration );
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

template<class type>
class anSinOscillator {
public:
						anSinOscillator();
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

template<class type>
inline anSinOscillator<type>::anSinOscillator() {
	currentTime = startTime = period = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	minValue = maxValue = currentValue;
}

template<class type>
inline void anSinOscillator<type>::Init( const int startTime, const int period, const type minValue, const type maxValue ) {
	this->startTime = startTime;
	this->period = period;
	this->minValue = minValue;
	this->maxValue = maxValue;
	this->currentTime = startTime - 1;
	this->currentValue = minValue;
}

template<class type>
inline type anSinOscillator<type>::GetCurrentValue( int time ) const {
	if ( time != currentTime ) {
		currentTime = time;

		float deltaTime = period == 0 ? 0.0f : ( time - startTime ) / ( float ) period;
		float s = ( 1.0f + ( float ) sin( deltaTime * anMath::TWO_PI ) ) * 0.5f;
		currentValue = minValue + s * ( maxValue - minValue );
	}
	return currentValue;
}



/*
==============================================================================================

	Bandpass filter styleee, with lerping on the leading and falling edges.
	If OOB flag is set, an additional OOB value will be returned if the
	time is out with the boundaries of the filter.

	        _______
		   /       \
		  /         \
	_____/           \______


	With OOB conditions:
	        _______
		   /       \
		  /         \
	     /           \
	     |           |
	     |           |
	     |           |
	_____|           |______

==============================================================================================
*/

template<typename type>
class anBandPassFilter {
public:
						anBandPassFilter( void );
	void				Init( const float lowPassStart, const float lowPassDuration, const float highPassStart, const float highPassDuration, const type &lowValue, const type &highValue );
	void				SetLowPassStart( float _time ) { lowStartTime = _time; }
	void				SetLowPassDuration( float _duration ) { lowDuration = _duration; }
	void				SetHighPassStart( float _time ) { highStartTime = _time; }
	void				SetHighPassDuration( float _duration ) { highDuration = _duration; }
	void				SetLowValue( const type& _low ) { lowValue = _low; }
	void				SetHighValue( const type& _high ) { highValue = _high; }
	void				SetOOBValue( const type& _value ) { oobFlag = true; oobValue = _value; }
	void				SetOOBFlag( bool _flag ) { oobFlag = _flag; }
	type				GetCurrentValue( float time ) const;
	bool				IsDone( float time ) const { return ( time >= highStartTime + highDuration ); }
	float				GetLowPassStart( void ) const { return lowStartTime; }
	float				GetLowPassDuration( void ) const { return lowDuration; }
	float				GetHighPassStart( void ) const { return highStartTime; }
	float				GetHighPassDuration( void ) const { return highDuration; }
	const type&			GetOOBValue( void ) const { return oobValue; }
	bool				GetOOBFlag( void ) const { return oobFlag; }
	const type&			GetLowValue( void ) const { return lowValue; }
	const type&			GetHighValue( void ) const { return highValue; }

private:
	float				lowStartTime, lowDuration;
	float				highStartTime, highDuration;
	type				oobValue;
	bool				oobFlag;
	type				lowValue, highValue;
};

/*
====================
anBandPassFilter::idInterpolate
====================
*/
template<typename type>
inline anBandPassFilter<type>::anBandPassFilter( void ) {
	lowStartTime	= 0.f;
	lowDuration		= 0.f;
	highStartTime	= 0.f;
	highDuration	= 0.f;

	oobFlag			= false;

	memset( &lowValue, 0, sizeof( lowValue ) );
	highValue = lowValue;
}

/*
====================
anBandPassFilter::Init
====================
*/
template<typename type>
inline void anBandPassFilter<type>::Init( const float lowPassStart, const float lowPassDuration, const float highPassStart, const float highPassDuration, const type& _lowValue, const type& _highValue ) {
	assert( lowPassDuration >= 0 );
	assert( highPassDuration >= 0 );
	assert( lowPassStart + lowPassDuration <= highPassStart );

	lowStartTime		= lowPassStart;
	lowDuration			= lowPassDuration;
	highStartTime		= highPassStart;
	highDuration		= highPassDuration;

	lowValue			= _lowValue;
	highValue			= _highValue;
}

/*
====================
anBandPassFilter::GetCurrentValue
====================
*/
template<typename type>
inline type anBandPassFilter<type>::GetCurrentValue( float time ) const {
	if ( time < lowStartTime ) {
		// pre leading edge condition

		if ( oobFlag ) {
			return oobValue;
		}
		return lowValue;
	}

	if ( time < lowStartTime + lowDuration ) {
		// on the leading edge
		float frac = ( time - lowStartTime ) / lowDuration;
		return Lerp( lowValue, highValue, frac );
	}

	if ( time < highStartTime ) {
		// on the upper level
		return highValue;
	}

	if ( time < highStartTime + highDuration ) {
		// on the trailing edge
		float frac = ( time - highStartTime ) / highDuration;
		return Lerp( highValue, lowValue, frac );
	}

	if ( oobFlag ) {
		return oobValue;
	}
	return lowValue;
}

#endif

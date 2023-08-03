#ifndef __MATH_CURVE_H__
#define __MATH_CURVE_H__

/*
===============================================================================

	Curve base template.

===============================================================================
*/

template<class type>
class anCurve {
public:
						anCurve( void );
	virtual				~anCurve( void );

	virtual int			AddValue( const float time, const type &value );
	virtual void		RemoveIndex( const int index ) { values.RemoveIndex( index ); times.RemoveIndex( index ); changed = true; }
	virtual void		Clear( void ) { values.Clear(); times.Clear(); currentIndex = -1; changed = true; }

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

	virtual bool		IsDone( const float time ) const;

	int					GetNumValues( void ) const { return values.Num(); }
	void				SetValue( const int index, const type &value ) { values[index] = value; changed = true; }
	type				GetValue( const int index ) const { return values[index]; }
	type *				GetValueAddress( const int index ) { return &values[index]; }
	float				GetTime( const int index ) const { return times[index]; }

	float				GetLengthForTime( const float time ) const;
	float				GetTimeForLength( const float length, const float epsilon = 0.1f ) const;
	float				GetLengthBetweenKnots( const int i0, const int i1 ) const;

	void				MakeUniform( const float totalTime );
	void				SetConstantSpeed( const float totalTime );
	void				ShiftTime( const float deltaTime );
	void				Translate( const type &translation );

protected:

	anList<float>		times;			// knots
	anList<type>		values;			// knot values

	mutable int			currentIndex;	// cached index for fast lookup
	mutable bool		changed;		// set whenever the curve changes

	int					IndexForTime( const float time ) const;
	float				TimeForIndex( const int index ) const;
	type				ValueForIndex( const int index ) const;

	float				GetSpeed( const float time ) const;
	float				RombergIntegral( const float t0, const float t1, const int order ) const;
};

/*
====================
anCurve::anCurve
====================
*/
template<class type>
ARC_INLINE anCurve<type>::anCurve( void ) {
	currentIndex = -1;
	changed = false;
}

/*
====================
anCurve::~anCurve
====================
*/
template<class type>
ARC_INLINE anCurve<type>::~anCurve( void ) {
}

/*
====================
anCurve::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template<class type>
ARC_INLINE int anCurve<type>::AddValue( const float time, const type &value ) {
	int i;

	i = IndexForTime( time );
	times.Insert( time, i );
	values.Insert( value, i );
	changed = true;
	return i;
}

/*
====================
anCurve::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve<type>::GetCurrentValue( const float time ) const {
	int i;

	i = IndexForTime( time );
	if ( i >= values.Num() ) {
		return values[values.Num() - 1];
	} else {
		return values[i];
	}
}

/*
====================
anCurve::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve<type>::GetCurrentFirstDerivative( const float time ) const {
	return ( values[0] - values[0] );
}

/*
====================
anCurve::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve<type>::GetCurrentSecondDerivative( const float time ) const {
	return ( values[0] - values[0] );
}

/*
====================
anCurve::IsDone
====================
*/
template<class type>
ARC_INLINE bool anCurve<type>::IsDone( const float time ) const {
	return ( time >= times[ times.Num() - 1 ] );
}

/*
====================
anCurve::GetSpeed
====================
*/
template<class type>
ARC_INLINE float anCurve<type>::GetSpeed( const float time ) const {
	int i;
	float speed;
	type value;

	value = GetCurrentFirstDerivative( time );
	for ( speed = 0.0f, i = 0; i < value.GetDimension(); i++ ) {
		speed += value[i] * value[i];
	}
	return anMath::Sqrt( speed );
}

/*
====================
anCurve::RombergIntegral
====================
*/
template<class type>
ARC_INLINE float anCurve<type>::RombergIntegral( const float t0, const float t1, const int order ) const {
	int i, j, k, m, n;
	float sum, delta;
	float *temp[2];

	temp[0] = (float *) _alloca16( order * sizeof( float ) );
	temp[1] = (float *) _alloca16( order * sizeof( float ) );

	delta = t1 - t0;
	temp[0][0] = 0.5f * delta * ( GetSpeed( t0 ) + GetSpeed( t1 ) );

	for ( i = 2, m = 1; i <= order; i++, m *= 2, delta *= 0.5f ) {

		// approximate using the trapezoid rule
		sum = 0.0f;
		for ( j = 1; j <= m; j++ ) {
			sum += GetSpeed( t0 + delta * ( j - 0.5f ) );
		}

		// Richardson extrapolation
		temp[1][0] = 0.5f * ( temp[0][0] + delta * sum );
		for ( k = 1, n = 4; k < i; k++, n *= 4 ) {
			temp[1][k] = ( n * temp[1][k-1] - temp[0][k-1] ) / ( n - 1 );
		}

		for ( j = 0; j < i; j++ ) {
			temp[0][j] = temp[1][j];
		}
	}
	return temp[0][order-1];
}

/*
====================
anCurve::GetLengthBetweenKnots
====================
*/
template<class type>
ARC_INLINE float anCurve<type>::GetLengthBetweenKnots( const int i0, const int i1 ) const {
	float length = 0.0f;
	for ( int i = i0; i < i1; i++ ) {
		length += RombergIntegral( times[i], times[i+1], 5 );
	}
	return length;
}

/*
====================
anCurve::GetLengthForTime
====================
*/
template<class type>
ARC_INLINE float anCurve<type>::GetLengthForTime( const float time ) const {
	float length = 0.0f;
	int index = IndexForTime( time );
	for ( int i = 0; i < index; i++ ) {
		length += RombergIntegral( times[i], times[i+1], 5 );
	}
	length += RombergIntegral( times[index], time, 5 );
	return length;
}

/*
====================
anCurve::GetTimeForLength
====================
*/
template<class type>
ARC_INLINE float anCurve<type>::GetTimeForLength( const float length, const float epsilon ) const {
	int i, index;
	float *accumLength, totalLength, len0, len1, t, diff;

	if ( length <= 0.0f ) {
		return times[0];
	}

	accumLength = (float *) _alloca16( values.Num() * sizeof( float ) );
	totalLength = 0.0f;
	for ( index = 0; index < values.Num() - 1; index++ ) {
		totalLength += GetLengthBetweenKnots( index, index + 1 );
		accumLength[index] = totalLength;
		if ( length < accumLength[index] ) {
			break;
		}
	}

	if ( index >= values.Num() - 1 ) {
		return times[times.Num() - 1];
	}

	if ( index == 0 ) {
		len0 = length;
		len1 = accumLength[0];
	} else {
		len0 = length - accumLength[index-1];
		len1 = accumLength[index] - accumLength[index-1];
	}

	// invert the anLexer length integral using Newton's method
	t = ( times[index+1] - times[index] ) * len0 / len1;
	for ( i = 0; i < 32; i++ ) {
		diff = RombergIntegral( times[index], times[index] + t, 5 ) - len0;
		if ( anMath::Fabs( diff ) <= epsilon ) {
			return times[index] + t;
		}
		t -= diff / GetSpeed( times[index] + t );
	}
	return times[index] + t;
}

/*
====================
anCurve::MakeUniform
====================
*/
template<class type>
ARC_INLINE void anCurve<type>::MakeUniform( const float totalTime ) {
	int i, n;

	n = times.Num() - 1;
	for ( i = 0; i <= n; i++ ) {
		times[i] = i * totalTime / n;
	}
	changed = true;
}

/*
====================
anCurve::SetConstantSpeed
====================
*/
template<class type>
ARC_INLINE void anCurve<type>::SetConstantSpeed( const float totalTime ) {
	int i, j;
	float *length, totalLength, scale, t;

	length = (float *) _alloca16( values.Num() * sizeof( float ) );
	totalLength = 0.0f;
	for ( i = 0; i < values.Num() - 1; i++ ) {
		length[i] = GetLengthBetweenKnots( i, i + 1 );
		totalLength += length[i];
	}
	scale = totalTime / totalLength;
	for ( t = 0.0f, i = 0; i < times.Num() - 1; i++ ) {
		times[i] = t;
		t += scale * length[i];
	}
	times[times.Num() - 1] = totalTime;
	changed = true;
}

/*
====================
anCurve::ShiftTime
====================
*/
template<class type>
ARC_INLINE void anCurve<type>::ShiftTime( const float deltaTime ) {
	for ( int i = 0; i < times.Num(); i++ ) {
		times[i] += deltaTime;
	}
	changed = true;
}

/*
====================
anCurve::Translate
====================
*/
template<class type>
ARC_INLINE void anCurve<type>::Translate( const type &translation ) {
	for ( int i = 0; i < values.Num(); i++ ) {
		values[i] += translation;
	}
	changed = true;
}

/*
====================
anCurve::IndexForTime

  find the index for the first time greater than or equal to the given time
====================
*/
template<class type>
ARC_INLINE int anCurve<type>::IndexForTime( const float time ) const {
	int len, mid, offset, res;

	if ( currentIndex >= 0 && currentIndex <= times.Num() ) {
		// use the cached index if it is still valid
		if ( currentIndex == 0 ) {
			if ( time <= times[currentIndex] ) {
				return currentIndex;
			}
		} else if ( currentIndex == times.Num() ) {
			if ( time > times[currentIndex-1] ) {
				return currentIndex;
			}
		} else if ( time > times[currentIndex-1] && time <= times[currentIndex] ) {
			return currentIndex;
		} else if ( time > times[currentIndex] && ( currentIndex+1 == times.Num() || time <= times[currentIndex+1] ) ) {
			// use the next index
			currentIndex++;
			return currentIndex;
		}
	}

	// use binary search to find the index for the given time
	len = times.Num();
	mid = len;
	offset = 0;
	res = 0;
	while( mid > 0 ) {
		mid = len >> 1;
		if ( time == times[offset+mid] ) {
			return offset+mid;
		} else if ( time > times[offset+mid] ) {
			offset += mid;
			len -= mid;
			res = 1;
		} else {
			len -= mid;
			res = 0;
		}
	}
	currentIndex = offset+res;
	return currentIndex;
}

/*
====================
anCurve::ValueForIndex

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve<type>::ValueForIndex( const int index ) const {
	int n = values.Num()-1;

	if ( index < 0 ) {
		return values[0] + index * ( values[1] - values[0] );
	} else if ( index > n ) {
		return values[n] + ( index - n ) * ( values[n] - values[n-1] );
	}
	return values[index];
}

/*
====================
anCurve::TimeForIndex

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE float anCurve<type>::TimeForIndex( const int index ) const {
	int n = times.Num()-1;

	if ( index < 0 ) {
		return times[0] + index * ( times[1] - times[0] );
	} else if ( index > n ) {
		return times[n] + ( index - n ) * ( times[n] - times[n-1] );
	}
	return times[index];
}


/*
===============================================================================

	Bezier Curve template.
	The degree of the polynomial equals the number of knots minus one.

===============================================================================
*/

template<class type>
class anCurve_Bezier : public anCurve<type> {
public:
						anCurve_Bezier( void );

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const int order, const float t, float *bvals ) const;
	void				BasisFirstDerivative( const int order, const float t, float *bvals ) const;
	void				BasisSecondDerivative( const int order, const float t, float *bvals ) const;
};

/*
====================
anCurve_Bezier::anCurve_Bezier
====================
*/
template<class type>
ARC_INLINE anCurve_Bezier<type>::anCurve_Bezier( void ) {
}

/*
====================
anCurve_Bezier::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_Bezier<type>::GetCurrentValue( const float time ) const {
	int i;
	float *bvals;
	type v;

	bvals = (float *) _alloca16( this->values.Num() * sizeof( float ) );

	Basis( this->values.Num(), time, bvals );
	v = bvals[0] * this->values[0];
	for ( i = 1; i < this->values.Num(); i++ ) {
		v += bvals[i] * this->values[i];
	}
	return v;
}

/*
====================
anCurve_Bezier::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_Bezier<type>::GetCurrentFirstDerivative( const float time ) const {
	int i;
	float *bvals, d;
	type v;

	bvals = (float *) _alloca16( this->values.Num() * sizeof( float ) );

	BasisFirstDerivative( this->values.Num(), time, bvals );
	v = bvals[0] * this->values[0];
	for ( i = 1; i < this->values.Num(); i++ ) {
		v += bvals[i] * this->values[i];
	}
	d = ( this->times[this->times.Num()-1] - this->times[0] );
	return ( ( float ) (this->values.Num()-1 ) / d ) * v;
}

/*
====================
anCurve_Bezier::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_Bezier<type>::GetCurrentSecondDerivative( const float time ) const {
	int i;
	float *bvals, d;
	type v;

	bvals = (float *) _alloca16( this->values.Num() * sizeof( float ) );

	BasisSecondDerivative( this->values.Num(), time, bvals );
	v = bvals[0] * this->values[0];
	for ( i = 1; i < this->values.Num(); i++ ) {
		v += bvals[i] * this->values[i];
	}
	d = ( this->times[this->times.Num()-1] - this->times[0] );
	return ( ( float ) (this->values.Num()-2) * (this->values.Num()-1 ) / ( d * d ) ) * v;
}

/*
====================
anCurve_Bezier::Basis

  bezier basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_Bezier<type>::Basis( const int order, const float t, float *bvals ) const {
	int i, j, d;
	float *c, c1, c2, s, o, ps, po;

	bvals[0] = 1.0f;
	d = order - 1;
	if ( d <= 0 ) {
		return;
	}

	c = (float *) _alloca16( (d+1 ) * sizeof( float ) );
	s = ( float ) ( t - this->times[0] ) / ( this->times[this->times.Num()-1] - this->times[0] );
    o = 1.0f - s;
	ps = s;
	po = o;

	for ( i = 1; i < d; i++ ) {
		c[i] = 1.0f;
	}
	for ( i = 1; i < d; i++ ) {
		c[i-1] = 0.0f;
		c1 = c[i];
		c[i] = 1.0f;
		for ( j = i+1; j <= d; j++ ) {
			c2 = c[j];
			c[j] = c1 + c[j-1];
			c1 = c2;
		}
		bvals[i] = c[d] * ps;
		ps *= s;
	}
	for ( i = d-1; i >= 0; i-- ) {
		bvals[i] *= po;
		po *= o;
	}
	bvals[d] = ps;
}

/*
====================
anCurve_Bezier::BasisFirstDerivative

  first derivative of bezier basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_Bezier<type>::BasisFirstDerivative( const int order, const float t, float *bvals ) const {
	int i;

	Basis( order-1, t, bvals+1 );
	bvals[0] = 0.0f;
	for ( i = 0; i < order-1; i++ ) {
		bvals[i] -= bvals[i+1];
	}
}

/*
====================
anCurve_Bezier::BasisSecondDerivative

  second derivative of bezier basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_Bezier<type>::BasisSecondDerivative( const int order, const float t, float *bvals ) const {
	int i;

	BasisFirstDerivative( order-1, t, bvals+1 );
	bvals[0] = 0.0f;
	for ( i = 0; i < order-1; i++ ) {
		bvals[i] -= bvals[i+1];
	}
}


/*
===============================================================================

	Quadratic Bezier Curve template.
	Should always have exactly three knots.

===============================================================================
*/

template<class type>
class anCurve_QuadraticBezier : public anCurve<type> {

public:
						anCurve_QuadraticBezier( void );

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const float t, float *bvals ) const;
	void				BasisFirstDerivative( const float t, float *bvals ) const;
	void				BasisSecondDerivative( const float t, float *bvals ) const;
};

/*
====================
anCurve_QuadraticBezier::anCurve_QuadraticBezier
====================
*/
template<class type>
ARC_INLINE anCurve_QuadraticBezier<type>::anCurve_QuadraticBezier( void ) {
}


/*
====================
anCurve_QuadraticBezier::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_QuadraticBezier<type>::GetCurrentValue( const float time ) const {
	float bvals[3];
	assert( this->values.Num() == 3 );
	Basis( time, bvals );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] );
}

/*
====================
anCurve_QuadraticBezier::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_QuadraticBezier<type>::GetCurrentFirstDerivative( const float time ) const {
	float bvals[3], d;
	assert( this->values.Num() == 3 );
	BasisFirstDerivative( time, bvals );
	d = ( this->times[2] - this->times[0] );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] ) / d;
}

/*
====================
anCurve_QuadraticBezier::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_QuadraticBezier<type>::GetCurrentSecondDerivative( const float time ) const {
	float bvals[3], d;
	assert( this->values.Num() == 3 );
	BasisSecondDerivative( time, bvals );
	d = ( this->times[2] - this->times[0] );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] ) / ( d * d );
}

/*
====================
anCurve_QuadraticBezier::Basis

  quadratic bezier basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_QuadraticBezier<type>::Basis( const float t, float *bvals ) const {
	float s1 = ( float ) ( t - this->times[0] ) / ( this->times[2] - this->times[0] );
	float s2 = s1 * s1;
	bvals[0] = s2 - 2.0f * s1 + 1.0f;
	bvals[1] = -2.0f * s2 + 2.0f * s1;
	bvals[2] = s2;
}

/*
====================
anCurve_QuadraticBezier::BasisFirstDerivative

  first derivative of quadratic bezier basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_QuadraticBezier<type>::BasisFirstDerivative( const float t, float *bvals ) const {
	float s1 = ( float ) ( t - this->times[0] ) / ( this->times[2] - this->times[0] );
	bvals[0] = 2.0f * s1 - 2.0f;
	bvals[1] = -4.0f * s1 + 2.0f;
	bvals[2] = 2.0f * s1;
}

/*
====================
anCurve_QuadraticBezier::BasisSecondDerivative

  second derivative of quadratic bezier basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_QuadraticBezier<type>::BasisSecondDerivative( const float t, float *bvals ) const {
	float s1 = ( float ) ( t - this->times[0] ) / ( this->times[2] - this->times[0] );
	bvals[0] = 2.0f;
	bvals[1] = -4.0f;
	bvals[2] = 2.0f;
}


/*
===============================================================================

	Cubic Bezier Curve template.
	Should always have exactly four knots.

===============================================================================
*/

template<class type>
class anCurve_CubicBezier : public anCurve<type> {

public:
						anCurve_CubicBezier( void );

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const float t, float *bvals ) const;
	void				BasisFirstDerivative( const float t, float *bvals ) const;
	void				BasisSecondDerivative( const float t, float *bvals ) const;
};

/*
====================
anCurve_CubicBezier::anCurve_CubicBezier
====================
*/
template<class type>
ARC_INLINE anCurve_CubicBezier<type>::anCurve_CubicBezier( void ) {
}


/*
====================
anCurve_CubicBezier::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_CubicBezier<type>::GetCurrentValue( const float time ) const {
	float bvals[4];
	assert( this->values.Num() == 4 );
	Basis( time, bvals );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] + bvals[3] * this->values[3] );
}

/*
====================
anCurve_CubicBezier::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_CubicBezier<type>::GetCurrentFirstDerivative( const float time ) const {
	float bvals[4], d;
	assert( this->values.Num() == 4 );
	BasisFirstDerivative( time, bvals );
	d = ( this->times[3] - this->times[0] );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] + bvals[3] * this->values[3] ) / d;
}

/*
====================
anCurve_CubicBezier::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_CubicBezier<type>::GetCurrentSecondDerivative( const float time ) const {
	float bvals[4], d;
	assert( this->values.Num() == 4 );
	BasisSecondDerivative( time, bvals );
	d = ( this->times[3] - this->times[0] );
	return ( bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] + bvals[3] * this->values[3] ) / ( d * d );
}

/*
====================
anCurve_CubicBezier::Basis

  cubic bezier basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_CubicBezier<type>::Basis( const float t, float *bvals ) const {
	float s1 = ( float ) ( t - this->times[0] ) / ( this->times[3] - this->times[0] );
	float s2 = s1 * s1;
	float s3 = s2 * s1;
	bvals[0] = -s3 + 3.0f * s2 - 3.0f * s1 + 1.0f;
	bvals[1] = 3.0f * s3 - 6.0f * s2 + 3.0f * s1;
	bvals[2] = -3.0f * s3 + 3.0f * s2;
	bvals[3] = s3;
}

/*
====================
anCurve_CubicBezier::BasisFirstDerivative

  first derivative of cubic bezier basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_CubicBezier<type>::BasisFirstDerivative( const float t, float *bvals ) const {
	float s1 = ( float ) ( t - this->times[0] ) / ( this->times[3] - this->times[0] );
	float s2 = s1 * s1;
	bvals[0] = -3.0f * s2 + 6.0f * s1 - 3.0f;
	bvals[1] = 9.0f * s2 - 12.0f * s1 + 3.0f;
	bvals[2] = -9.0f * s2 + 6.0f * s1;
	bvals[3] = 3.0f * s2;
}

/*
====================
anCurve_CubicBezier::BasisSecondDerivative

  second derivative of cubic bezier basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_CubicBezier<type>::BasisSecondDerivative( const float t, float *bvals ) const {
	float s1 = ( float ) ( t - this->times[0] ) / ( this->times[3] - this->times[0] );
	bvals[0] = -6.0f * s1 + 6.0f;
	bvals[1] = 18.0f * s1 - 12.0f;
	bvals[2] = -18.0f * s1 + 6.0f;
	bvals[3] = 6.0f * s1;
}


/*
===============================================================================

	Spline base template.

===============================================================================
*/

template<class type>
class anCurveSpline : public anCurve<type> {

public:
	enum				boundary_t { BT_FREE, BT_CLAMPED, BT_CLOSED };

						anCurveSpline( void );

	virtual bool		IsDone( const float time ) const;

	virtual void		SetBoundaryType( const boundary_t bt ) { boundaryType = bt; this->changed = true; }
	virtual boundary_t	GetBoundaryType( void ) const { return boundaryType; }

	virtual void		SetCloseTime( const float t ) { closeTime = t; this->changed = true; }
	virtual float		GetCloseTime( void ) { return boundaryType == BT_CLOSED ? closeTime : 0.0f; }

protected:
	boundary_t			boundaryType;
	float				closeTime;

	type				ValueForIndex( const int index ) const;
	float				TimeForIndex( const int index ) const;
	float				ClampedTime( const float t ) const;
};

/*
====================
anCurveSpline::anCurveSpline
====================
*/
template<class type>
ARC_INLINE anCurveSpline<type>::anCurveSpline( void ) {
	boundaryType = BT_FREE;
	closeTime = 0.0f;
}

/*
====================
anCurveSpline::ValueForIndex

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurveSpline<type>::ValueForIndex( const int index ) const {
	int n = this->values.Num()-1;

	if ( index < 0 ) {
		if ( boundaryType == BT_CLOSED ) {
			return this->values[ this->values.Num() + index % this->values.Num() ];
		}
		else {
			return this->values[0] + index * ( this->values[1] - this->values[0] );
		}
	}
	else if ( index > n ) {
		if ( boundaryType == BT_CLOSED ) {
			return this->values[ index % this->values.Num() ];
		}
		else {
			return this->values[n] + ( index - n ) * ( this->values[n] - this->values[n-1] );
		}
	}
	return this->values[index];
}

/*
====================
anCurveSpline::TimeForIndex

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE float anCurveSpline<type>::TimeForIndex( const int index ) const {
	int n = this->times.Num()-1;

	if ( index < 0 ) {
		if ( boundaryType == BT_CLOSED ) {
			return ( index / this->times.Num() ) * ( this->times[n] + closeTime ) - ( this->times[n] + closeTime - this->times[this->times.Num() + index % this->times.Num()] );
		}
		else {
			return this->times[0] + index * ( this->times[1] - this->times[0] );
		}
	}
	else if ( index > n ) {
		if ( boundaryType == BT_CLOSED ) {
			return ( index / this->times.Num() ) * ( this->times[n] + closeTime ) + this->times[index % this->times.Num()];
		}
		else {
			return this->times[n] + ( index - n ) * ( this->times[n] - this->times[n-1] );
		}
	}
	return this->times[index];
}

/*
====================
anCurveSpline::ClampedTime

  return the clamped time based on the boundary type
====================
*/
template<class type>
ARC_INLINE float anCurveSpline<type>::ClampedTime( const float t ) const {
	if ( boundaryType == BT_CLAMPED ) {
		if ( t < this->times[0] ) {
			return this->times[0];
		}
		else if ( t >= this->times[this->times.Num()-1] ) {
			return this->times[this->times.Num()-1];
		}
	}
	return t;
}

/*
====================
anCurveSpline::IsDone
====================
*/
template<class type>
ARC_INLINE bool anCurveSpline<type>::IsDone( const float time ) const {
	return ( boundaryType != BT_CLOSED && time >= this->times[ this->times.Num() - 1 ] );
}


/*
===============================================================================

	Cubic Interpolating Spline template.
	The curve goes through all the knots.

===============================================================================
*/

template<class type>
class anCurve_NaturalCubicSpline : public anCurveSpline<type> {
public:
						anCurve_NaturalCubicSpline( void );

	virtual void		Clear( void ) { anCurveSpline<type>::Clear(); this->values.Clear(); b.Clear(); c.Clear(); d.Clear(); }

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	mutable anList<type>b;
	mutable anList<type>c;
	mutable anList<type>d;

	void				Setup( void ) const;
	void				SetupFree( void ) const;
	void				SetupClamped( void ) const;
	void				SetupClosed( void ) const;
};

/*
====================
anCurve_NaturalCubicSpline::anCurve_NaturalCubicSpline
====================
*/
template<class type>
ARC_INLINE anCurve_NaturalCubicSpline<type>::anCurve_NaturalCubicSpline( void ) {
}

/*
====================
anCurve_NaturalCubicSpline::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_NaturalCubicSpline<type>::GetCurrentValue( const float time ) const {
	float clampedTime = this->ClampedTime( time );
	int i = this->IndexForTime( clampedTime );
	float s = time - this->TimeForIndex( i );
	Setup();
	return ( this->values[i] + s * ( b[i] + s * ( c[i] + s * d[i] ) ) );
}

/*
====================
anCurve_NaturalCubicSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_NaturalCubicSpline<type>::GetCurrentFirstDerivative( const float time ) const {
	float clampedTime = this->ClampedTime( time );
	int i = this->IndexForTime( clampedTime );
	float s = time - this->TimeForIndex( i );
	Setup();
	return ( b[i] + s * ( 2.0f * c[i] + 3.0f * s * d[i] ) );
}

/*
====================
anCurve_NaturalCubicSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_NaturalCubicSpline<type>::GetCurrentSecondDerivative( const float time ) const {
	float clampedTime = this->ClampedTime( time );
	int i = this->IndexForTime( clampedTime );
	float s = time - this->TimeForIndex( i );
	Setup();
	return ( 2.0f * c[i] + 6.0f * s * d[i] );
}

/*
====================
anCurve_NaturalCubicSpline::Setup
====================
*/
template<class type>
ARC_INLINE void anCurve_NaturalCubicSpline<type>::Setup( void ) const {
	if ( this->changed ) {
		switch ( this->boundaryType ) {
			case anCurveSpline<type>::BT_FREE:		SetupFree(); break;
			case anCurveSpline<type>::BT_CLAMPED:	SetupClamped(); break;
			case anCurveSpline<type>::BT_CLOSED:		SetupClosed(); break;
		}
		this->changed = false;
	}
}

/*
====================
anCurve_NaturalCubicSpline::SetupFree
====================
*/
template<class type>
ARC_INLINE void anCurve_NaturalCubicSpline<type>::SetupFree( void ) const {
	int i;
	float inv;
	float *d0, *d1, *beta, *gamma;
	type *alpha, *delta;

	d0 = (float *) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	d1 = (float *) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	alpha = (type *) _alloca16( ( this->values.Num() - 1 ) * sizeof( type ) );
	beta = (float *) _alloca16( this->values.Num() * sizeof( float ) );
	gamma = (float *) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	delta = (type *) _alloca16( this->values.Num() * sizeof( type ) );

	for ( i = 0; i < this->values.Num() - 1; i++ ) {
		d0[i] = this->times[i+1] - this->times[i];
	}

	for ( i = 1; i < this->values.Num() - 1; i++ ) {
		d1[i] = this->times[i+1] - this->times[i-1];
	}

	for ( i = 1; i < this->values.Num() - 1; i++ ) {
		type sum = 3.0f * ( d0[i-1] * this->values[i+1] - d1[i] * this->values[i] + d0[i] * this->values[i-1] );
		inv = 1.0f / ( d0[i-1] * d0[i] );
		alpha[i] = inv * sum;
	}

	beta[0] = 1.0f;
	gamma[0] = 0.0f;
	delta[0] = this->values[0] - this->values[0];

	for ( i = 1; i < this->values.Num() - 1; i++ ) {
		beta[i] = 2.0f * d1[i] - d0[i-1] * gamma[i-1];
		inv = 1.0f / beta[i];
		gamma[i] = inv * d0[i];
		delta[i] = inv * ( alpha[i] - d0[i-1] * delta[i-1] );
	}
	beta[this->values.Num() - 1] = 1.0f;
	delta[this->values.Num() - 1] = this->values[0] - this->values[0];

	b.AssureSize( this->values.Num() );
	c.AssureSize( this->values.Num() );
	d.AssureSize( this->values.Num() );

	c[this->values.Num() - 1] = this->values[0] - this->values[0];

	for ( i = this->values.Num() - 2; i >= 0; i-- ) {
		c[i] = delta[i] - gamma[i] * c[i+1];
		inv = 1.0f / d0[i];
		b[i] = inv * ( this->values[i+1] - this->values[i] ) - ( 1.0f / 3.0f ) * d0[i] * ( c[i+1] + 2.0f * c[i] );
		d[i] = ( 1.0f / 3.0f ) * inv * ( c[i+1] - c[i] );
	}
}

/*
====================
anCurve_NaturalCubicSpline::SetupClamped
====================
*/
template<class type>
ARC_INLINE void anCurve_NaturalCubicSpline<type>::SetupClamped( void ) const {
	int i;
	float inv;
	float *d0, *d1, *beta, *gamma;
	type *alpha, *delta;

	d0 = (float *) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	d1 = (float *) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	alpha = (type *) _alloca16( ( this->values.Num() - 1 ) * sizeof( type ) );
	beta = (float *) _alloca16( this->values.Num() * sizeof( float ) );
	gamma = (float *) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	delta = (type *) _alloca16( this->values.Num() * sizeof( type ) );

	for ( i = 0; i < this->values.Num() - 1; i++ ) {
		d0[i] = this->times[i+1] - this->times[i];
	}

	for ( i = 1; i < this->values.Num() - 1; i++ ) {
		d1[i] = this->times[i+1] - this->times[i-1];
	}

	inv = 1.0f / d0[0];
	alpha[0] = 3.0f * ( inv - 1.0f ) * ( this->values[1] - this->values[0] );
	inv = 1.0f / d0[this->values.Num() - 2];
	alpha[this->values.Num() - 1] = 3.0f * ( 1.0f - inv ) * ( this->values[this->values.Num() - 1] - this->values[this->values.Num() - 2] );

	for ( i = 1; i < this->values.Num() - 1; i++ ) {
		type sum = 3.0f * ( d0[i-1] * this->values[i+1] - d1[i] * this->values[i] + d0[i] * this->values[i-1] );
		inv = 1.0f / ( d0[i-1] * d0[i] );
		alpha[i] = inv * sum;
	}

	beta[0] = 2.0f * d0[0];
	gamma[0] = 0.5f;
	inv = 1.0f / beta[0];
	delta[0] = inv * alpha[0];

	for ( i = 1; i < this->values.Num() - 1; i++ ) {
		beta[i] = 2.0f * d1[i] - d0[i-1] * gamma[i-1];
		inv = 1.0f / beta[i];
		gamma[i] = inv * d0[i];
		delta[i] = inv * ( alpha[i] - d0[i-1] * delta[i-1] );
	}

	beta[this->values.Num() - 1] = d0[this->values.Num() - 2] * ( 2.0f - gamma[this->values.Num() - 2] );
	inv = 1.0f / beta[this->values.Num() - 1];
	delta[this->values.Num() - 1] = inv * ( alpha[this->values.Num() - 1] - d0[this->values.Num() - 2] * delta[this->values.Num() - 2] );

	b.AssureSize( this->values.Num() );
	c.AssureSize( this->values.Num() );
	d.AssureSize( this->values.Num() );

	c[this->values.Num() - 1] = delta[this->values.Num() - 1];

	for ( i = this->values.Num() - 2; i >= 0; i-- ) {
		c[i] = delta[i] - gamma[i] * c[i+1];
		inv = 1.0f / d0[i];
		b[i] = inv * ( this->values[i+1] - this->values[i] ) - ( 1.0f / 3.0f ) * d0[i]* ( c[i+1] + 2.0f * c[i] );
		d[i] = ( 1.0f / 3.0f ) * inv * ( c[i+1] - c[i] );
	}
}

/*
====================
anCurve_NaturalCubicSpline::SetupClosed
====================
*/
template<class type>
ARC_INLINE void anCurve_NaturalCubicSpline<type>::SetupClosed( void ) const {
	int i, j;
	float c0, c1;
	float *d0;
	anMatX mat;
	anVecX x;

	d0 = (float *) _alloca16( ( this->values.Num() - 1 ) * sizeof( float ) );
	x.SetData( this->values.Num(), VECX_ALLOCA( this->values.Num() ) );
	mat.SetData( this->values.Num(), this->values.Num(), MATX_ALLOCA( this->values.Num() * this->values.Num() ) );

	b.AssureSize( this->values.Num() );
	c.AssureSize( this->values.Num() );
	d.AssureSize( this->values.Num() );

	for ( i = 0; i < this->values.Num() - 1; i++ ) {
		d0[i] = this->times[i+1] - this->times[i];
	}

	// matrix of system
	mat[0][0] = 1.0f;
	mat[0][this->values.Num() - 1] = -1.0f;
	for ( i = 1; i <= this->values.Num() - 2; i++ ) {
		mat[i][i-1] = d0[i-1];
		mat[i][i  ] = 2.0f * ( d0[i-1] + d0[i] );
		mat[i][i+1] = d0[i];
	}
	mat[this->values.Num() - 1][this->values.Num() - 2] = d0[this->values.Num() - 2];
	mat[this->values.Num() - 1][0] = 2.0f * ( d0[this->values.Num() - 2] + d0[0] );
	mat[this->values.Num() - 1][1] = d0[0];

	// right-hand side
	c[0].Zero();
	for ( i = 1; i <= this->values.Num() - 2; i++ ) {
		c0 = 1.0f / d0[i];
		c1 = 1.0f / d0[i-1];
		c[i] = 3.0f * ( c0 * ( this->values[i + 1] - this->values[i] ) - c1 * ( this->values[i] - this->values[i - 1] ) );
	}
	c0 = 1.0f / d0[0];
	c1 = 1.0f / d0[this->values.Num() - 2];
	c[this->values.Num() - 1] = 3.0f * ( c0 * ( this->values[1] - this->values[0] ) - c1 * ( this->values[0] - this->values[this->values.Num() - 2] ) );

	// solve system for each dimension
	mat.LU_Factor( nullptr );
	for ( i = 0; i < this->values[0].GetDimension(); i++ ) {
		for ( j = 0; j < this->values.Num(); j++ ) {
			x[j] = c[j][i];
		}
		mat.LU_Solve( x, x, nullptr );
		for ( j = 0; j < this->values.Num(); j++ ) {
			c[j][i] = x[j];
		}
	}

	for ( i = 0; i < this->values.Num() - 1; i++ ) {
		c0 = 1.0f / d0[i];
		b[i] = c0 * ( this->values[i + 1] - this->values[i] ) - ( 1.0f / 3.0f ) * ( c[i+1] + 2.0f * c[i] ) * d0[i];
		d[i] = ( 1.0f / 3.0f ) * c0 * ( c[i + 1] - c[i] );
	}
}


/*
===============================================================================

	Uniform Cubic Interpolating Spline template.
	The curve goes through all the knots.

===============================================================================
*/

template<class type>
class anCurve_CatmullRomSpline : public anCurveSpline<type> {

public:
						anCurve_CatmullRomSpline( void );

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const int index, const float t, float *bvals ) const;
	void				BasisFirstDerivative( const int index, const float t, float *bvals ) const;
	void				BasisSecondDerivative( const int index, const float t, float *bvals ) const;
};

/*
====================
anCurve_CatmullRomSpline::anCurve_CatmullRomSpline
====================
*/
template<class type>
ARC_INLINE anCurve_CatmullRomSpline<type>::anCurve_CatmullRomSpline( void ) {
}

/*
====================
anCurve_CatmullRomSpline::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_CatmullRomSpline<type>::GetCurrentValue( const float time ) const {
	int i, j, k;
	float bvals[4], clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	Basis( i-1, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < 4; j++ ) {
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
anCurve_CatmullRomSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_CatmullRomSpline<type>::GetCurrentFirstDerivative( const float time ) const {
	int i, j, k;
	float bvals[4], d, clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return ( this->values[0] - this->values[0] );
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisFirstDerivative( i-1, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < 4; j++ ) {
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i-1 ) );
	return v / d;
}

/*
====================
anCurve_CatmullRomSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_CatmullRomSpline<type>::GetCurrentSecondDerivative( const float time ) const {
	int i, j, k;
	float bvals[4], d, clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return ( this->values[0] - this->values[0] );
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisSecondDerivative( i-1, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < 4; j++ ) {
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i-1 ) );
	return v / ( d * d );
}

/*
====================
anCurve_CatmullRomSpline::Basis

  spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_CatmullRomSpline<type>::Basis( const int index, const float t, float *bvals ) const {
	float s = ( float ) ( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index+1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( ( -s + 2.0f ) * s - 1.0f ) * s * 0.5f;				// -0.5f s * s * s + s * s - 0.5f * s
	bvals[1] = ( ( ( 3.0f * s - 5.0f ) * s ) * s + 2.0f ) * 0.5f;	// 1.5f * s * s * s - 2.5f * s * s + 1.0f
	bvals[2] = ( ( -3.0f * s + 4.0f ) * s + 1.0f ) * s * 0.5f;		// -1.5f * s * s * s - 2.0f * s * s + 0.5f s
	bvals[3] = ( ( s - 1.0f ) * s * s ) * 0.5f;						// 0.5f * s * s * s - 0.5f * s * s
}

/*
====================
anCurve_CatmullRomSpline::BasisFirstDerivative

  first derivative of spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_CatmullRomSpline<type>::BasisFirstDerivative( const int index, const float t, float *bvals ) const {
	float s = ( float ) ( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index+1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( -1.5f * s + 2.0f ) * s - 0.5f;						// -1.5f * s * s + 2.0f * s - 0.5f
	bvals[1] = ( 4.5f * s - 5.0f ) * s;								// 4.5f * s * s - 5.0f * s
	bvals[2] = ( -4.5 * s + 4.0f ) * s + 0.5f;						// -4.5 * s * s + 4.0f * s + 0.5f
	bvals[3] = 1.5f * s * s - s;									// 1.5f * s * s - s
}

/*
====================
anCurve_CatmullRomSpline::BasisSecondDerivative

  second derivative of spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_CatmullRomSpline<type>::BasisSecondDerivative( const int index, const float t, float *bvals ) const {
	float s = ( float ) ( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index+1 ) - this->TimeForIndex( index ) );
	bvals[0] = -3.0f * s + 2.0f;
	bvals[1] = 9.0f * s - 5.0f;
	bvals[2] = -9.0f * s + 4.0f;
	bvals[3] = 3.0f * s - 1.0f;
}


/*
===============================================================================

	Cubic Interpolating Spline template.
	The curve goes through all the knots.
	The curve becomes the Catmull-Rom spline if the tension,
	continuity and bias are all set to zero.

===============================================================================
*/

template<class type>
class anCurve_KochanekBartelsSpline : public anCurveSpline<type> {

public:
						anCurve_KochanekBartelsSpline( void );

	virtual int			AddValue( const float time, const type &value );
	virtual int			AddValue( const float time, const type &value, const float tension, const float continuity, const float bias );
	virtual void		RemoveIndex( const int index ) { this->values.RemoveIndex( index ); this->times.RemoveIndex( index ); tension.RemoveIndex( index ); continuity.RemoveIndex( index ); bias.RemoveIndex( index ); }
	virtual void		Clear( void ) { this->values.Clear(); this->times.Clear(); tension.Clear(); continuity.Clear(); bias.Clear(); this->currentIndex = -1; }

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	anList<float>		tension;
	anList<float>		continuity;
	anList<float>		bias;

	void				TangentsForIndex( const int index, type &t0, type &t1 ) const;

	void				Basis( const int index, const float t, float *bvals ) const;
	void				BasisFirstDerivative( const int index, const float t, float *bvals ) const;
	void				BasisSecondDerivative( const int index, const float t, float *bvals ) const;
};

/*
====================
anCurve_KochanekBartelsSpline::anCurve_KochanekBartelsSpline
====================
*/
template<class type>
ARC_INLINE anCurve_KochanekBartelsSpline<type>::anCurve_KochanekBartelsSpline( void ) {
}

/*
====================
anCurve_KochanekBartelsSpline::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template<class type>
ARC_INLINE int anCurve_KochanekBartelsSpline<type>::AddValue( const float time, const type &value ) {
	int i;

	i = this->IndexForTime( time );
	this->times.Insert( time, i );
	this->values.Insert( value, i );
	tension.Insert( 0.0f, i );
	continuity.Insert( 0.0f, i );
	bias.Insert( 0.0f, i );
	return i;
}

/*
====================
anCurve_KochanekBartelsSpline::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template<class type>
ARC_INLINE int anCurve_KochanekBartelsSpline<type>::AddValue( const float time, const type &value, const float tension, const float continuity, const float bias ) {
	int i;

	i = this->IndexForTime( time );
	this->times.Insert( time, i );
	this->values.Insert( value, i );
	this->tension.Insert( tension, i );
	this->continuity.Insert( continuity, i );
	this->bias.Insert( bias, i );
	return i;
}

/*
====================
anCurve_KochanekBartelsSpline::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_KochanekBartelsSpline<type>::GetCurrentValue( const float time ) const {
	int i;
	float bvals[4], clampedTime;
	type v, t0, t1;

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	TangentsForIndex( i - 1, t0, t1 );
	Basis( i - 1, clampedTime, bvals );
	v = bvals[0] * this->ValueForIndex( i - 1 );
	v += bvals[1] * this->ValueForIndex( i );
	v += bvals[2] * t0;
	v += bvals[3] * t1;
	return v;
}

/*
====================
anCurve_KochanekBartelsSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_KochanekBartelsSpline<type>::GetCurrentFirstDerivative( const float time ) const {
	int i;
	float bvals[4], d, clampedTime;
	type v, t0, t1;

	if ( this->times.Num() == 1 ) {
		return ( this->values[0] - this->values[0] );
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	TangentsForIndex( i - 1, t0, t1 );
	BasisFirstDerivative( i - 1, clampedTime, bvals );
	v = bvals[0] * this->ValueForIndex( i - 1 );
	v += bvals[1] * this->ValueForIndex( i );
	v += bvals[2] * t0;
	v += bvals[3] * t1;
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i-1 ) );
	return v / d;
}

/*
====================
anCurve_KochanekBartelsSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_KochanekBartelsSpline<type>::GetCurrentSecondDerivative( const float time ) const {
	int i;
	float bvals[4], d, clampedTime;
	type v, t0, t1;

	if ( this->times.Num() == 1 ) {
		return ( this->values[0] - this->values[0] );
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	TangentsForIndex( i - 1, t0, t1 );
	BasisSecondDerivative( i - 1, clampedTime, bvals );
	v = bvals[0] * this->ValueForIndex( i - 1 );
	v += bvals[1] * this->ValueForIndex( i );
	v += bvals[2] * t0;
	v += bvals[3] * t1;
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i-1 ) );
	return v / ( d * d );
}

/*
====================
anCurve_KochanekBartelsSpline::TangentsForIndex
====================
*/
template<class type>
ARC_INLINE void anCurve_KochanekBartelsSpline<type>::TangentsForIndex( const int index, type &t0, type &t1 ) const {
	float dt, omt, omc, opc, omb, opb, adj, s0, s1;
	type delta;

	delta = this->ValueForIndex( index + 1 ) - this->ValueForIndex( index );
	dt = this->TimeForIndex( index + 1 ) - this->TimeForIndex( index );

	omt = 1.0f - tension[index];
	omc = 1.0f - continuity[index];
	opc = 1.0f + continuity[index];
	omb = 1.0f - bias[index];
	opb = 1.0f + bias[index];
	adj = 2.0f * dt / ( this->TimeForIndex( index + 1 ) - this->TimeForIndex( index - 1 ) );
	s0 = 0.5f * adj * omt * opc * opb;
	s1 = 0.5f * adj * omt * omc * omb;

	// outgoing tangent at first point
	t0 = s1 * delta + s0 * ( this->ValueForIndex( index ) - this->ValueForIndex( index - 1 ) );

	omt = 1.0f - tension[index + 1];
	omc = 1.0f - continuity[index + 1];
	opc = 1.0f + continuity[index + 1];
	omb = 1.0f - bias[index + 1];
	opb = 1.0f + bias[index + 1];
	adj = 2.0f * dt / ( this->TimeForIndex( index + 2 ) - this->TimeForIndex( index ) );
	s0 = 0.5f * adj * omt * omc * opb;
	s1 = 0.5f * adj * omt * opc * omb;

	// incoming tangent at second point
	t1 = s1 * ( this->ValueForIndex( index + 2 ) - this->ValueForIndex( index + 1 ) ) + s0 * delta;
}

/*
====================
anCurve_KochanekBartelsSpline::Basis

  spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_KochanekBartelsSpline<type>::Basis( const int index, const float t, float *bvals ) const {
	float s = ( float ) ( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index+1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( ( 2.0f * s - 3.0f ) * s ) * s + 1.0f;				// 2.0f * s * s * s - 3.0f * s * s + 1.0f
	bvals[1] = ( ( -2.0f * s + 3.0f ) * s ) * s;					// -2.0f * s * s * s + 3.0f * s * s
	bvals[2] = ( ( s - 2.0f ) * s ) * s + s;						// s * s * s - 2.0f * s * s + s
	bvals[3] = ( ( s - 1.0f ) * s ) * s;							// s * s * s - s * s
}

/*
====================
anCurve_KochanekBartelsSpline::BasisFirstDerivative

  first derivative of spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_KochanekBartelsSpline<type>::BasisFirstDerivative( const int index, const float t, float *bvals ) const {
	float s = ( float ) ( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index+1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( 6.0f * s - 6.0f ) * s;								// 6.0f * s * s - 6.0f * s
	bvals[1] = ( -6.0f * s + 6.0f ) * s;							// -6.0f * s * s + 6.0f * s
	bvals[2] = ( 3.0f * s - 4.0f ) * s + 1.0f;						// 3.0f * s * s - 4.0f * s + 1.0f
	bvals[3] = ( 3.0f * s - 2.0f ) * s;								// 3.0f * s * s - 2.0f * s
}

/*
====================
anCurve_KochanekBartelsSpline::BasisSecondDerivative

  second derivative of spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_KochanekBartelsSpline<type>::BasisSecondDerivative( const int index, const float t, float *bvals ) const {
	float s = ( float ) ( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index+1 ) - this->TimeForIndex( index ) );
	bvals[0] = 12.0f * s - 6.0f;
	bvals[1] = -12.0f * s + 6.0f;
	bvals[2] = 6.0f * s - 4.0f;
	bvals[3] = 6.0f * s - 2.0f;
}


/*
===============================================================================

	B-Spline base template. Uses recursive definition and is slow.
	Use anCurve_UniformCubicBSpline or anCurve_NonUniformBSpline instead.

===============================================================================
*/

template<class type>
class anCurve_BSpline : public anCurveSpline<type> {

public:
						anCurve_BSpline( void );

	virtual int			GetOrder( void ) const { return order; }
	virtual void		SetOrder( const int i ) { assert( i > 0 && i < 10 ); order = i; }

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	int					order;

	float				Basis( const int index, const int order, const float t ) const;
	float				BasisFirstDerivative( const int index, const int order, const float t ) const;
	float				BasisSecondDerivative( const int index, const int order, const float t ) const;
};

/*
====================
anCurve_BSpline::anCurve_NaturalCubicSpline
====================
*/
template<class type>
ARC_INLINE anCurve_BSpline<type>::anCurve_BSpline( void ) {
	order = 4;	// default to cubic
}

/*
====================
anCurve_BSpline::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_BSpline<type>::GetCurrentValue( const float time ) const {
	int i, j, k;
	float clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < order; j++ ) {
		k = i + j - ( order >> 1 );
		v += Basis( k-2, order, clampedTime ) * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
anCurve_BSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_BSpline<type>::GetCurrentFirstDerivative( const float time ) const {
	int i, j, k;
	float clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < order; j++ ) {
		k = i + j - ( order >> 1 );
		v += BasisFirstDerivative( k-2, order, clampedTime ) * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
anCurve_BSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_BSpline<type>::GetCurrentSecondDerivative( const float time ) const {
	int i, j, k;
	float clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < order; j++ ) {
		k = i + j - ( order >> 1 );
		v += BasisSecondDerivative( k-2, order, clampedTime ) * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
anCurve_BSpline::Basis

  spline basis function
====================
*/
template<class type>
ARC_INLINE float anCurve_BSpline<type>::Basis( const int index, const int order, const float t ) const {
	if ( order <= 1 ) {
		if ( this->TimeForIndex( index ) < t && t <= this->TimeForIndex( index + 1 ) ) {
			return 1.0f;
		} else {
			return 0.0f;
		}
	} else {
		float sum = 0.0f;
		float d1 = this->TimeForIndex( index+order-1 ) - this->TimeForIndex( index );
		if ( d1 != 0.0f ) {
			sum += ( float ) ( t - this->TimeForIndex( index ) ) * Basis( index, order-1, t ) / d1;
		}

		float d2 = this->TimeForIndex( index+order ) - this->TimeForIndex( index+1 );
		if ( d2 != 0.0f ) {
			sum += ( float ) ( this->TimeForIndex( index+order ) - t ) * Basis( index+1, order-1, t ) / d2;
		}
		return sum;
	}
}

/*
====================
anCurve_BSpline::BasisFirstDerivative

  first derivative of spline basis function
====================
*/
template<class type>
ARC_INLINE float anCurve_BSpline<type>::BasisFirstDerivative( const int index, const int order, const float t ) const {
	return ( Basis( index, order-1, t ) - Basis( index+1, order-1, t ) ) *
			( float ) ( order - 1 ) / ( this->TimeForIndex( index + ( order - 1 ) - 2 ) - this->TimeForIndex( index - 2 ) );
}

/*
====================
anCurve_BSpline::BasisSecondDerivative

  second derivative of spline basis function
====================
*/
template<class type>
ARC_INLINE float anCurve_BSpline<type>::BasisSecondDerivative( const int index, const int order, const float t ) const {
	return ( BasisFirstDerivative( index, order-1, t ) - BasisFirstDerivative( index+1, order-1, t ) ) *
			( float ) ( order - 1 ) / ( this->TimeForIndex( index + ( order - 1 ) - 2 ) - this->TimeForIndex( index - 2 ) );
}


/*
===============================================================================

	Uniform Non-Rational Cubic B-Spline template.

===============================================================================
*/

template<class type>
class anCurve_UniformCubicBSpline : public anCurve_BSpline<type> {

public:
						anCurve_UniformCubicBSpline( void );

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const int index, const float t, float *bvals ) const;
	void				BasisFirstDerivative( const int index, const float t, float *bvals ) const;
	void				BasisSecondDerivative( const int index, const float t, float *bvals ) const;
};

/*
====================
anCurve_UniformCubicBSpline::anCurve_UniformCubicBSpline
====================
*/
template<class type>
ARC_INLINE anCurve_UniformCubicBSpline<type>::anCurve_UniformCubicBSpline( void ) {
	this->order = 4;	// always cubic
}

/*
====================
anCurve_UniformCubicBSpline::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_UniformCubicBSpline<type>::GetCurrentValue( const float time ) const {
	int i, j, k;
	float bvals[4], clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	Basis( i-1, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < 4; j++ ) {
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
anCurve_UniformCubicBSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_UniformCubicBSpline<type>::GetCurrentFirstDerivative( const float time ) const {
	int i, j, k;
	float bvals[4], d, clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return ( this->values[0] - this->values[0] );
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisFirstDerivative( i-1, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < 4; j++ ) {
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i-1 ) );
	return v / d;
}

/*
====================
anCurve_UniformCubicBSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_UniformCubicBSpline<type>::GetCurrentSecondDerivative( const float time ) const {
	int i, j, k;
	float bvals[4], d, clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return ( this->values[0] - this->values[0] );
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisSecondDerivative( i-1, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < 4; j++ ) {
		k = i + j - 2;
		v += bvals[j] * this->ValueForIndex( k );
	}
	d = ( this->TimeForIndex( i ) - this->TimeForIndex( i-1 ) );
	return v / ( d * d );
}

/*
====================
anCurve_UniformCubicBSpline::Basis

  spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_UniformCubicBSpline<type>::Basis( const int index, const float t, float *bvals ) const {
	float s = ( float ) ( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index+1 ) - this->TimeForIndex( index ) );
	bvals[0] = ( ( ( -s + 3.0f ) * s - 3.0f ) * s + 1.0f ) * ( 1.0f / 6.0f );
	bvals[1] = ( ( ( 3.0f * s - 6.0f ) * s ) * s + 4.0f ) * ( 1.0f / 6.0f );
	bvals[2] = ( ( ( -3.0f * s + 3.0f ) * s + 3.0f ) * s + 1.0f ) * ( 1.0f / 6.0f );
	bvals[3] = ( s * s * s ) * ( 1.0f / 6.0f );
}

/*
====================
anCurve_UniformCubicBSpline::BasisFirstDerivative

  first derivative of spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_UniformCubicBSpline<type>::BasisFirstDerivative( const int index, const float t, float *bvals ) const {
	float s = ( float ) ( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index+1 ) - this->TimeForIndex( index ) );
	bvals[0] = -0.5f * s * s + s - 0.5f;
	bvals[1] = 1.5f * s * s - 2.0f * s;
	bvals[2] = -1.5f * s * s + s + 0.5f;
	bvals[3] = 0.5f * s * s;
}

/*
====================
anCurve_UniformCubicBSpline::BasisSecondDerivative

  second derivative of spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_UniformCubicBSpline<type>::BasisSecondDerivative( const int index, const float t, float *bvals ) const {
	float s = ( float ) ( t - this->TimeForIndex( index ) ) / ( this->TimeForIndex( index+1 ) - this->TimeForIndex( index ) );
	bvals[0] = -s + 1.0f;
	bvals[1] = 3.0f * s - 2.0f;
	bvals[2] = -3.0f * s + 1.0f;
	bvals[3] = s;
}


/*
===============================================================================

	Non-Uniform Non-Rational B-Spline (NUBS) template.

===============================================================================
*/

template<class type>
class anCurve_NonUniformBSpline : public anCurve_BSpline<type> {

public:
						anCurve_NonUniformBSpline( void );

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	void				Basis( const int index, const int order, const float t, float *bvals ) const;
	void				BasisFirstDerivative( const int index, const int order, const float t, float *bvals ) const;
	void				BasisSecondDerivative( const int index, const int order, const float t, float *bvals ) const;
};

/*
====================
anCurve_NonUniformBSpline::anCurve_NonUniformBSpline
====================
*/
template<class type>
ARC_INLINE anCurve_NonUniformBSpline<type>::anCurve_NonUniformBSpline( void ) {
}

/*
====================
anCurve_NonUniformBSpline::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_NonUniformBSpline<type>::GetCurrentValue( const float time ) const {
	int i, j, k;
	float clampedTime;
	type v;
	float *bvals = (float *) _alloca16( this->order * sizeof( float ) );

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	Basis( i-1, this->order, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < this->order; j++ ) {
		k = i + j - ( this->order >> 1 );
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
anCurve_NonUniformBSpline::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_NonUniformBSpline<type>::GetCurrentFirstDerivative( const float time ) const {
	int i, j, k;
	float clampedTime;
	type v;
	float *bvals = (float *) _alloca16( this->order * sizeof( float ) );

	if ( this->times.Num() == 1 ) {
		return ( this->values[0] - this->values[0] );
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisFirstDerivative( i-1, this->order, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < this->order; j++ ) {
		k = i + j - ( this->order >> 1 );
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
anCurve_NonUniformBSpline::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_NonUniformBSpline<type>::GetCurrentSecondDerivative( const float time ) const {
	int i, j, k;
	float clampedTime;
	type v;
	float *bvals = (float *) _alloca16( this->order * sizeof( float ) );

	if ( this->times.Num() == 1 ) {
		return ( this->values[0] - this->values[0] );
	}

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	BasisSecondDerivative( i-1, this->order, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	for ( j = 0; j < this->order; j++ ) {
		k = i + j - ( this->order >> 1 );
		v += bvals[j] * this->ValueForIndex( k );
	}
	return v;
}

/*
====================
anCurve_NonUniformBSpline::Basis

  spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_NonUniformBSpline<type>::Basis( const int index, const int order, const float t, float *bvals ) const {
    int r, s, i;
    float omega;

    bvals[order-1] = 1.0f;
    for ( r = 2; r <= order; r++ ) {
		i = index - r + 1;
		bvals[order - r] = 0.0f;
		for ( s = order - r + 1; s < order; s++ ) {
			i++;
			omega = ( float ) ( t - this->TimeForIndex( i ) ) / ( this->TimeForIndex( i + r - 1 ) - this->TimeForIndex( i ) );
			bvals[s - 1] += ( 1.0f - omega ) * bvals[s];
			bvals[s] *= omega;
		}
    }
}

/*
====================
anCurve_NonUniformBSpline::BasisFirstDerivative

  first derivative of spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_NonUniformBSpline<type>::BasisFirstDerivative( const int index, const int order, const float t, float *bvals ) const {
	int i;

	Basis( index, order-1, t, bvals+1 );
	bvals[0] = 0.0f;
	for ( i = 0; i < order-1; i++ ) {
		bvals[i] -= bvals[i+1];
		bvals[i] *= ( float ) ( order - 1 ) / ( this->TimeForIndex( index + i + (order-1 ) - 2 ) - this->TimeForIndex( index + i - 2 ) );
	}
	bvals[i] *= ( float ) ( order - 1 ) / ( this->TimeForIndex( index + i + (order-1 ) - 2 ) - this->TimeForIndex( index + i - 2 ) );
}

/*
====================
anCurve_NonUniformBSpline::BasisSecondDerivative

  second derivative of spline basis functions
====================
*/
template<class type>
ARC_INLINE void anCurve_NonUniformBSpline<type>::BasisSecondDerivative( const int index, const int order, const float t, float *bvals ) const {
	int i;

	BasisFirstDerivative( index, order-1, t, bvals+1 );
	bvals[0] = 0.0f;
	for ( i = 0; i < order-1; i++ ) {
		bvals[i] -= bvals[i+1];
		bvals[i] *= ( float ) ( order - 1 ) / ( this->TimeForIndex( index + i + (order-1 ) - 2 ) - this->TimeForIndex( index + i - 2 ) );
	}
	bvals[i] *= ( float ) ( order - 1 ) / ( this->TimeForIndex( index + i + (order-1 ) - 2 ) - this->TimeForIndex( index + i - 2 ) );
}


/*
===============================================================================

	Non-Uniform Rational B-Spline (NURBS) template.

===============================================================================
*/

template<class type>
class anCurve_NURBS : public anCurve_NonUniformBSpline<type> {

public:
						anCurve_NURBS( void );

	virtual int			AddValue( const float time, const type &value );
	virtual int			AddValue( const float time, const type &value, const float weight );
	virtual void		RemoveIndex( const int index ) { this->values.RemoveIndex( index ); this->times.RemoveIndex( index ); weights.RemoveIndex( index ); }
	virtual void		Clear( void ) { this->values.Clear(); this->times.Clear(); weights.Clear(); this->currentIndex = -1; }

	virtual type		GetCurrentValue( const float time ) const;
	virtual type		GetCurrentFirstDerivative( const float time ) const;
	virtual type		GetCurrentSecondDerivative( const float time ) const;

protected:
	anList<float>		weights;

	float				WeightForIndex( const int index ) const;
};

/*
====================
anCurve_NURBS::anCurve_NURBS
====================
*/
template<class type>
ARC_INLINE anCurve_NURBS<type>::anCurve_NURBS( void ) {
}

/*
====================
anCurve_NURBS::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template<class type>
ARC_INLINE int anCurve_NURBS<type>::AddValue( const float time, const type &value ) {
	int i;

	i = this->IndexForTime( time );
	this->times.Insert( time, i );
	this->values.Insert( value, i );
	weights.Insert( 1.0f, i );
	return i;
}

/*
====================
anCurve_NURBS::AddValue

  add a timed/value pair to the spline
  returns the index to the inserted pair
====================
*/
template<class type>
ARC_INLINE int anCurve_NURBS<type>::AddValue( const float time, const type &value, const float weight ) {
	int i;

	i = this->IndexForTime( time );
	this->times.Insert( time, i );
	this->values.Insert( value, i );
	weights.Insert( weight, i );
	return i;
}

/*
====================
anCurve_NURBS::GetCurrentValue

  get the value for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_NURBS<type>::GetCurrentValue( const float time ) const {
	int i, j, k;
	float w, b, *bvals, clampedTime;
	type v;

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	bvals = (float *) _alloca16( this->order * sizeof( float ) );

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	this->Basis( i-1, this->order, clampedTime, bvals );
	v = this->values[0] - this->values[0];
	w = 0.0f;
	for ( j = 0; j < this->order; j++ ) {
		k = i + j - ( this->order >> 1 );
		b = bvals[j] * WeightForIndex( k );
		w += b;
		v += b * this->ValueForIndex( k );
	}
	return v / w;
}

/*
====================
anCurve_NURBS::GetCurrentFirstDerivative

  get the first derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_NURBS<type>::GetCurrentFirstDerivative( const float time ) const {
	int i, j, k;
	float w, wb, wd1, b, d1, *bvals, *d1vals, clampedTime;
	type v, vb, vd1;

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	bvals = (float *) _alloca16( this->order * sizeof( float ) );
	d1vals = (float *) _alloca16( this->order * sizeof( float ) );

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	this->Basis( i-1, this->order, clampedTime, bvals );
	this->BasisFirstDerivative( i-1, this->order, clampedTime, d1vals );
	vb = vd1 = this->values[0] - this->values[0];
	wb = wd1 = 0.0f;
	for ( j = 0; j < this->order; j++ ) {
		k = i + j - ( this->order >> 1 );
		w = WeightForIndex( k );
		b = bvals[j] * w;
		d1 = d1vals[j] * w;
		wb += b;
		wd1 += d1;
		v = this->ValueForIndex( k );
		vb += b * v;
		vd1 += d1 * v;
	}
	return ( wb * vd1 - vb * wd1 ) / ( wb * wb );
}

/*
====================
anCurve_NURBS::GetCurrentSecondDerivative

  get the second derivative for the given time
====================
*/
template<class type>
ARC_INLINE type anCurve_NURBS<type>::GetCurrentSecondDerivative( const float time ) const {
	int i, j, k;
	float w, wb, wd1, wd2, b, d1, d2, *bvals, *d1vals, *d2vals, clampedTime;
	type v, vb, vd1, vd2;

	if ( this->times.Num() == 1 ) {
		return this->values[0];
	}

	bvals = (float *) _alloca16( this->order * sizeof( float ) );
	d1vals = (float *) _alloca16( this->order * sizeof( float ) );
	d2vals = (float *) _alloca16( this->order * sizeof( float ) );

	clampedTime = this->ClampedTime( time );
	i = this->IndexForTime( clampedTime );
	this->Basis( i-1, this->order, clampedTime, bvals );
	this->BasisFirstDerivative( i-1, this->order, clampedTime, d1vals );
	this->BasisSecondDerivative( i-1, this->order, clampedTime, d2vals );
	vb = vd1 = vd2 = this->values[0] - this->values[0];
	wb = wd1 = wd2 = 0.0f;
	for ( j = 0; j < this->order; j++ ) {
		k = i + j - ( this->order >> 1 );
		w = WeightForIndex( k );
		b = bvals[j] * w;
		d1 = d1vals[j] * w;
		d2 = d2vals[j] * w;
		wb += b;
		wd1 += d1;
		wd2 += d2;
		v = this->ValueForIndex( k );
		vb += b * v;
		vd1 += d1 * v;
		vd2 += d2 * v;
	}
	return ( ( wb * wb ) * ( wb * vd2 - vb * wd2 ) - ( wb * vd1 - vb * wd1 ) * 2.0f * wb * wd1 ) / ( wb * wb * wb * wb );
}

/*
====================
anCurve_NURBS::WeightForIndex

  get the weight for the given index
====================
*/
template<class type>
ARC_INLINE float anCurve_NURBS<type>::WeightForIndex( const int index ) const {
	int n = weights.Num()-1;

	if ( index < 0 ) {
		if ( this->boundaryType == anCurveSpline<type>::BT_CLOSED ) {
			return weights[ weights.Num() + index % weights.Num() ];
		} else {
			return weights[0] + index * ( weights[1] - weights[0] );
		}
	} else if ( index > n ) {
		if ( this->boundaryType == anCurveSpline<type>::BT_CLOSED ) {
			return weights[ index % weights.Num() ];
		} else {
			return weights[n] + ( index - n ) * ( weights[n] - weights[n-1] );
		}
	}
	return weights[index];
}

#endif // !__MATH_CURVE_H__

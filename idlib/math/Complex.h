#ifndef __MATH_COMPLEX_H__
#define __MATH_COMPLEX_H__

/*
===============================================================================

  Complex number

===============================================================================
*/

class TeckComplex {
public:
	float				r;		// real part
	float				i;		// imaginary part

						TeckComplex( void );
						TeckComplex( const float r, const float i );

	void 				Set( const float r, const float i );
	void				Zero( void );

	float				operator[]( int index ) const;
	float &				operator[]( int index );

	TeckComplex			operator-() const;
	TeckComplex &		operator=( const TeckComplex &a );

	TeckComplex			operator*( const TeckComplex &a ) const;
	TeckComplex			operator/( const TeckComplex &a ) const;
	TeckComplex			operator+( const TeckComplex &a ) const;
	TeckComplex			operator-( const TeckComplex &a ) const;

	TeckComplex &		operator*=( const TeckComplex &a );
	TeckComplex &		operator/=( const TeckComplex &a );
	TeckComplex &		operator+=( const TeckComplex &a );
	TeckComplex &		operator-=( const TeckComplex &a );

	TeckComplex			operator*( const float a ) const;
	TeckComplex			operator/( const float a ) const;
	TeckComplex			operator+( const float a ) const;
	TeckComplex			operator-( const float a ) const;

	TeckComplex &		operator*=( const float a );
	TeckComplex &		operator/=( const float a );
	TeckComplex &		operator+=( const float a );
	TeckComplex &		operator-=( const float a );

	friend TeckComplex	operator*( const float a, const TeckComplex &b );
	friend TeckComplex	operator/( const float a, const TeckComplex &b );
	friend TeckComplex	operator+( const float a, const TeckComplex &b );
	friend TeckComplex	operator-( const float a, const TeckComplex &b );

	bool				Compare( const TeckComplex &a ) const;						// exact compare, no epsilon
	bool				Compare( const TeckComplex &a, const float epsilon ) const;	// compare with epsilon
	bool				operator==(	const TeckComplex &a ) const;						// exact compare, no epsilon
	bool				operator!=(	const TeckComplex &a ) const;						// exact compare, no epsilon

	TeckComplex			Reciprocal( void ) const;
	TeckComplex			Sqrt( void ) const;
	float				Abs( void ) const;

	int					GetDimension( void ) const;

	const float *		ToFloatPtr( void ) const;
	float *				ToFloatPtr( void );
	const char *		ToString( int precision = 2 ) const;
};

extern TeckComplex complexOrigin;
#define complex_zero complexOrigin

ARC_INLINE TeckComplex::TeckComplex( void ) {
}

ARC_INLINE TeckComplex::TeckComplex( const float r, const float i ) {
	//this->r = r;
	//this->i = i;
}

ARC_INLINE void TeckComplex::Set( const float r, const float i ) {
	this->r = r;
	this->i = i;
}

ARC_INLINE void TeckComplex::Zero( void ) {
	r = i = 0.0f;
}

ARC_INLINE float TeckComplex::operator[]( int index ) const {
	assert( index >= 0 && index < 2 );
	return ( &r )[index];
}

ARC_INLINE float& TeckComplex::operator[]( int index ) {
	assert( index >= 0 && index < 2 );
	return ( &r )[index];
}

ARC_INLINE TeckComplex TeckComplex::operator-() const {
	return TeckComplex( -r, -i );
}

ARC_INLINE TeckComplex &TeckComplex::operator=( const TeckComplex &a ) {
	r = a.r;
	i = a.i;
	return *this;
}

ARC_INLINE TeckComplex TeckComplex::operator*( const TeckComplex &a ) const {
	return TeckComplex( r * a.r - i * a.i, i * a.r + r * a.i );
}

ARC_INLINE TeckComplex TeckComplex::operator/( const TeckComplex &a ) const {
	float s, t;
	if ( arcMath::Fabs( a.r ) >= arcMath::Fabs( a.i ) ) {
		s = a.i / a.r;
		t = 1.0f / ( a.r + s * a.i );
		return TeckComplex( ( r + s * i ) * t, ( i - s * r ) * t );
	} else {
		s = a.r / a.i;
		t = 1.0f / ( s * a.r + a.i );
		return TeckComplex( ( r * s + i ) * t, ( i * s - r ) * t );
	}
}

ARC_INLINE TeckComplex TeckComplex::operator+( const TeckComplex &a ) const {
	return TeckComplex( r + a.r, i + a.i );
}

ARC_INLINE TeckComplex TeckComplex::operator-( const TeckComplex &a ) const {
	return TeckComplex( r - a.r, i - a.i );
}

ARC_INLINE TeckComplex &TeckComplex::operator*=( const TeckComplex &a ) {
	*this = TeckComplex( r * a.r - i * a.i, i * a.r + r * a.i );
	return *this;
}

ARC_INLINE TeckComplex &TeckComplex::operator/=( const TeckComplex &a ) {
	float s, t;
	if ( arcMath::Fabs( a.r ) >= arcMath::Fabs( a.i ) ) {
		s = a.i / a.r;
		t = 1.0f / ( a.r + s * a.i );
		*this = TeckComplex( ( r + s * i ) * t, ( i - s * r ) * t );
	} else {
		s = a.r / a.i;
		t = 1.0f / ( s * a.r + a.i );
		*this = TeckComplex( ( r * s + i ) * t, ( i * s - r ) * t );
	}
	return *this;
}

ARC_INLINE TeckComplex &TeckComplex::operator+=( const TeckComplex &a ) {
	r += a.r;
	i += a.i;
	return *this;
}

ARC_INLINE TeckComplex &TeckComplex::operator-=( const TeckComplex &a ) {
	r -= a.r;
	i -= a.i;
	return *this;
}

ARC_INLINE TeckComplex TeckComplex::operator*( const float a ) const {
	return TeckComplex( r * a, i * a );
}

ARC_INLINE TeckComplex TeckComplex::operator/( const float a ) const {
	float s = 1.0f / a;
	return TeckComplex( r * s, i * s );
}

ARC_INLINE TeckComplex TeckComplex::operator+( const float a ) const {
	return TeckComplex( r + a, i );
}

ARC_INLINE TeckComplex TeckComplex::operator-( const float a ) const {
	return TeckComplex( r - a, i );
}

ARC_INLINE TeckComplex &TeckComplex::operator*=( const float a ) {
	r *= a;
	i *= a;
	return *this;
}

ARC_INLINE TeckComplex &TeckComplex::operator/=( const float a ) {
	float s = 1.0f / a;
	r *= s;
	i *= s;
	return *this;
}

ARC_INLINE TeckComplex &TeckComplex::operator+=( const float a ) {
	r += a;
	return *this;
}

ARC_INLINE TeckComplex &TeckComplex::operator-=( const float a ) {
	r -= a;
	return *this;
}

ARC_INLINE TeckComplex operator*( const float a, const TeckComplex &b ) {
	return TeckComplex( a * b.r, a * b.i );
}

ARC_INLINE TeckComplex operator/( const float a, const TeckComplex &b ) {
	float s, t;
	if ( arcMath::Fabs( b.r ) >= arcMath::Fabs( b.i ) ) {
		s = b.i / b.r;
		t = a / ( b.r + s * b.i );
		return TeckComplex( t, - s * t );
	} else {
		s = b.r / b.i;
		t = a / ( s * b.r + b.i );
		return TeckComplex( s * t, - t );
	}
}

ARC_INLINE TeckComplex operator+( const float a, const TeckComplex &b ) {
	return TeckComplex( a + b.r, b.i );
}

ARC_INLINE TeckComplex operator-( const float a, const TeckComplex &b ) {
	return TeckComplex( a - b.r, -b.i );
}

ARC_INLINE TeckComplex TeckComplex::Reciprocal( void ) const {
	float s, t;
	if ( arcMath::Fabs( r ) >= arcMath::Fabs( i ) ) {
		s = i / r;
		t = 1.0f / ( r + s * i );
		return TeckComplex( t, - s * t );
	} else {
		s = r / i;
		t = 1.0f / ( s * r + i );
		return TeckComplex( s * t, - t );
	}
}

ARC_INLINE TeckComplex TeckComplex::Sqrt( void ) const {
	float x, y, w;

	if ( r == 0.0f && i == 0.0f ) {
		return TeckComplex( 0.0f, 0.0f );
	}
	x = arcMath::Fabs( r );
	y = arcMath::Fabs( i );
	if ( x >= y ) {
		w = y / x;
		w = arcMath::Sqrt( x ) * arcMath::Sqrt( 0.5f * ( 1.0f + arcMath::Sqrt( 1.0f + w * w ) ) );
	} else {
		w = x / y;
		w = arcMath::Sqrt( y ) * arcMath::Sqrt( 0.5f * ( w + arcMath::Sqrt( 1.0f + w * w ) ) );
	}
	if ( w == 0.0f ) {
		return TeckComplex( 0.0f, 0.0f );
	}
	if ( r >= 0.0f ) {
		return TeckComplex( w, 0.5f * i / w );
	} else {
		return TeckComplex( 0.5f * y / w, ( i >= 0.0f ) ? w : -w );
	}
}

ARC_INLINE float TeckComplex::Abs( void ) const {
	float x, y, t;
	x = arcMath::Fabs( r );
	y = arcMath::Fabs( i );
	if ( x == 0.0f ) {
		return y;
	} else if ( y == 0.0f ) {
		return x;
	} else if ( x > y ) {
		t = y / x;
		return x * arcMath::Sqrt( 1.0f + t * t );
	} else {
		t = x / y;
		return y * arcMath::Sqrt( 1.0f + t * t );
	}
}

ARC_INLINE bool TeckComplex::Compare( const TeckComplex &a ) const {
	return ( ( r == a.r ) && ( i == a.i ) );
}

ARC_INLINE bool TeckComplex::Compare( const TeckComplex &a, const float epsilon ) const {
	if ( arcMath::Fabs( r - a.r ) > epsilon ) {
		return false;
	}
	if ( arcMath::Fabs( i - a.i ) > epsilon ) {
		return false;
	}
	return true;
}

ARC_INLINE bool TeckComplex::operator==( const TeckComplex &a ) const {
	return Compare( a );
}

ARC_INLINE bool TeckComplex::operator!=( const TeckComplex &a ) const {
	return !Compare( a );
}

ARC_INLINE int TeckComplex::GetDimension( void ) const {
	return 2;
}

ARC_INLINE const float *TeckComplex::ToFloatPtr( void ) const {
	return &r;
}

ARC_INLINE float *TeckComplex::ToFloatPtr( void ) {
	return &r;
}

#endif
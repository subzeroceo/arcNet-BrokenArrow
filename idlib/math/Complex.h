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

inline TeckComplex::TeckComplex( void ) {
}

inline TeckComplex::TeckComplex( const float r, const float i ) {
	//this->r = r;
	//this->i = i;
}

inline void TeckComplex::Set( const float r, const float i ) {
	this->r = r;
	this->i = i;
}

inline void TeckComplex::Zero( void ) {
	r = i = 0.0f;
}

inline float TeckComplex::operator[]( int index ) const {
	assert( index >= 0 && index < 2 );
	return ( &r )[index];
}

inline float& TeckComplex::operator[]( int index ) {
	assert( index >= 0 && index < 2 );
	return ( &r )[index];
}

inline TeckComplex TeckComplex::operator-() const {
	return TeckComplex( -r, -i );
}

inline TeckComplex &TeckComplex::operator=( const TeckComplex &a ) {
	r = a.r;
	i = a.i;
	return *this;
}

inline TeckComplex TeckComplex::operator*( const TeckComplex &a ) const {
	return TeckComplex( r * a.r - i * a.i, i * a.r + r * a.i );
}

inline TeckComplex TeckComplex::operator/( const TeckComplex &a ) const {
	float s, t;
	if ( anMath::Fabs( a.r ) >= anMath::Fabs( a.i ) ) {
		s = a.i / a.r;
		t = 1.0f / ( a.r + s * a.i );
		return TeckComplex( ( r + s * i ) * t, ( i - s * r ) * t );
	} else {
		s = a.r / a.i;
		t = 1.0f / ( s * a.r + a.i );
		return TeckComplex( ( r * s + i ) * t, ( i * s - r ) * t );
	}
}

inline TeckComplex TeckComplex::operator+( const TeckComplex &a ) const {
	return TeckComplex( r + a.r, i + a.i );
}

inline TeckComplex TeckComplex::operator-( const TeckComplex &a ) const {
	return TeckComplex( r - a.r, i - a.i );
}

inline TeckComplex &TeckComplex::operator*=( const TeckComplex &a ) {
	*this = TeckComplex( r * a.r - i * a.i, i * a.r + r * a.i );
	return *this;
}

inline TeckComplex &TeckComplex::operator/=( const TeckComplex &a ) {
	float s, t;
	if ( anMath::Fabs( a.r ) >= anMath::Fabs( a.i ) ) {
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

inline TeckComplex &TeckComplex::operator+=( const TeckComplex &a ) {
	r += a.r;
	i += a.i;
	return *this;
}

inline TeckComplex &TeckComplex::operator-=( const TeckComplex &a ) {
	r -= a.r;
	i -= a.i;
	return *this;
}

inline TeckComplex TeckComplex::operator*( const float a ) const {
	return TeckComplex( r * a, i * a );
}

inline TeckComplex TeckComplex::operator/( const float a ) const {
	float s = 1.0f / a;
	return TeckComplex( r * s, i * s );
}

inline TeckComplex TeckComplex::operator+( const float a ) const {
	return TeckComplex( r + a, i );
}

inline TeckComplex TeckComplex::operator-( const float a ) const {
	return TeckComplex( r - a, i );
}

inline TeckComplex &TeckComplex::operator*=( const float a ) {
	r *= a;
	i *= a;
	return *this;
}

inline TeckComplex &TeckComplex::operator/=( const float a ) {
	float s = 1.0f / a;
	r *= s;
	i *= s;
	return *this;
}

inline TeckComplex &TeckComplex::operator+=( const float a ) {
	r += a;
	return *this;
}

inline TeckComplex &TeckComplex::operator-=( const float a ) {
	r -= a;
	return *this;
}

inline TeckComplex operator*( const float a, const TeckComplex &b ) {
	return TeckComplex( a * b.r, a * b.i );
}

inline TeckComplex operator/( const float a, const TeckComplex &b ) {
	float s, t;
	if ( anMath::Fabs( b.r ) >= anMath::Fabs( b.i ) ) {
		s = b.i / b.r;
		t = a / ( b.r + s * b.i );
		return TeckComplex( t, - s * t );
	} else {
		s = b.r / b.i;
		t = a / ( s * b.r + b.i );
		return TeckComplex( s * t, - t );
	}
}

inline TeckComplex operator+( const float a, const TeckComplex &b ) {
	return TeckComplex( a + b.r, b.i );
}

inline TeckComplex operator-( const float a, const TeckComplex &b ) {
	return TeckComplex( a - b.r, -b.i );
}

inline TeckComplex TeckComplex::Reciprocal( void ) const {
	float s, t;
	if ( anMath::Fabs( r ) >= anMath::Fabs( i ) ) {
		s = i / r;
		t = 1.0f / ( r + s * i );
		return TeckComplex( t, - s * t );
	} else {
		s = r / i;
		t = 1.0f / ( s * r + i );
		return TeckComplex( s * t, - t );
	}
}

inline TeckComplex TeckComplex::Sqrt( void ) const {
	float x, y, w;

	if ( r == 0.0f && i == 0.0f ) {
		return TeckComplex( 0.0f, 0.0f );
	}
	x = anMath::Fabs( r );
	y = anMath::Fabs( i );
	if ( x >= y ) {
		w = y / x;
		w = anMath::Sqrt( x ) * anMath::Sqrt( 0.5f * ( 1.0f + anMath::Sqrt( 1.0f + w * w ) ) );
	} else {
		w = x / y;
		w = anMath::Sqrt( y ) * anMath::Sqrt( 0.5f * ( w + anMath::Sqrt( 1.0f + w * w ) ) );
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

inline float TeckComplex::Abs( void ) const {
	float x, y, t;
	x = anMath::Fabs( r );
	y = anMath::Fabs( i );
	if ( x == 0.0f ) {
		return y;
	} else if ( y == 0.0f ) {
		return x;
	} else if ( x > y ) {
		t = y / x;
		return x * anMath::Sqrt( 1.0f + t * t );
	} else {
		t = x / y;
		return y * anMath::Sqrt( 1.0f + t * t );
	}
}

inline bool TeckComplex::Compare( const TeckComplex &a ) const {
	return ( ( r == a.r ) && ( i == a.i ) );
}

inline bool TeckComplex::Compare( const TeckComplex &a, const float epsilon ) const {
	if ( anMath::Fabs( r - a.r ) > epsilon ) {
		return false;
	}
	if ( anMath::Fabs( i - a.i ) > epsilon ) {
		return false;
	}
	return true;
}

inline bool TeckComplex::operator==( const TeckComplex &a ) const {
	return Compare( a );
}

inline bool TeckComplex::operator!=( const TeckComplex &a ) const {
	return !Compare( a );
}

inline int TeckComplex::GetDimension( void ) const {
	return 2;
}

inline const float *TeckComplex::ToFloatPtr( void ) const {
	return &r;
}

inline float *TeckComplex::ToFloatPtr( void ) {
	return &r;
}

#endif
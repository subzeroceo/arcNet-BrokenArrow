#ifndef __FLOATERRORS_H__
#define __FLOATERRORS_H__

/*
===============================================================================

	Float to int conversions are expensive and should all be made explicit
	in the code using anMath::Ftoi(). Double literals should also never be
	used throughout the engine because automatic promotion of expressions
	to double may come at a performance penalty.

	The define FORCE_FLOAT_ERRORS can be enabled to generate compile errors
	for all C/C++ float to int conversions, both implicit and explicit.
	With this define enabled the compiler also generates errors for all
	double literals.

===============================================================================
*/

#ifndef __TYPE_INFO_GEN__
//#define FORCE_FLOAT_ERRORS
#endif

#ifdef FORCE_FLOAT_ERRORS

struct arcTstFloat;

struct idUnionFloat {
	operator			float & ( void ) { return value; }
	operator			float ( void ) const { return value; }
	float				operator=( const float f ) { value = f; return f; }
	float				value;
};

struct arcTstFloat {
						arcTstFloat( void ) {}
						arcTstFloat( float f ) { value = f; }
						arcTstFloat( int i ) { value = i; }
						arcTstFloat( long i ) { value = i; }
						arcTstFloat( unsigned int i ) { value = i; }
						arcTstFloat( unsigned long i ) { value = i; }
						arcTstFloat( idUnionFloat f ) { value = f.value; }

	operator			float & ( void ) { return value; }
	operator			float ( void ) const { return value; }
	operator			bool ( void ) { return ( value != 0.0f ); }

	arcTstFloat			operator-() const { return arcTstFloat( -value ); }
	arcTstFloat			operator+() const { return *this; }
	bool				operator!() const { return ( value == 0.0f ); }

	bool				operator&&( const arcTstFloat f ) const { return ( value != 0.0f && f.value != 0.0f ); }
	bool				operator||( const arcTstFloat f ) const { return ( value != 0.0f || f.value != 0.0f ); }
	bool				operator&&( const bool b ) const { return ( value != 0.0f && b ); }
	bool				operator||( const bool b ) const { return ( value != 0.0f || b ); }

	arcTstFloat			operator*( const arcTstFloat f ) const { return arcTstFloat( value * f.value ); }
	arcTstFloat			operator/( const arcTstFloat f ) const { return arcTstFloat( value / f.value ); }
	arcTstFloat			operator+( const arcTstFloat f ) const { return arcTstFloat( value + f.value ); }
	arcTstFloat			operator-( const arcTstFloat f ) const { return arcTstFloat( value - f.value ); }
	bool				operator<( const arcTstFloat f ) const { return ( value < f.value ); }
	bool				operator<=( const arcTstFloat f ) const { return ( value <= f.value ); }
	bool				operator>( const arcTstFloat f ) const { return ( value > f.value ); }
	bool				operator>=( const arcTstFloat f ) const { return ( value >= f.value ); }
	bool				operator==( const arcTstFloat f ) const { return ( value == f.value ); }
	bool				operator!=( const arcTstFloat f ) const { return ( value != f.value ); }
	arcTstFloat			operator*=( const arcTstFloat f ) { value *= f.value; return *this; }
	arcTstFloat			operator/=( const arcTstFloat f ) { value /= f.value; return *this; }
	arcTstFloat			operator+=( const arcTstFloat f ) { value += f.value; return *this; }
	arcTstFloat			operator-=( const arcTstFloat f ) { value -= f.value; return *this; }

	arcTstFloat			operator*( const float f ) const { return arcTstFloat( value * f ); }
	arcTstFloat			operator/( const float f ) const { return arcTstFloat( value / f ); }
	arcTstFloat			operator+( const float f ) const { return arcTstFloat( value + f ); }
	arcTstFloat			operator-( const float f ) const { return arcTstFloat( value - f ); }
	bool				operator<( const float f ) const { return ( value < f ); }
	bool				operator<=( const float f ) const { return ( value <= f ); }
	bool				operator>( const float f ) const { return ( value > f ); }
	bool				operator>=( const float f ) const { return ( value >= f ); }
	bool				operator==( const float f ) const { return ( value == f ); }
	bool				operator!=( const float f ) const { return ( value != f ); }
	arcTstFloat			operator*=( const float f ) { value *= f; return *this; }
	arcTstFloat			operator/=( const float f ) { value /= f; return *this; }
	arcTstFloat			operator+=( const float f ) { value += f; return *this; }
	arcTstFloat			operator-=( const float f ) { value -= f; return *this; }
	friend arcTstFloat	operator*( const float i, const arcTstFloat f ) { return arcTstFloat( i * f.value ); }
	friend arcTstFloat	operator/( const float i, const arcTstFloat f ) { return arcTstFloat( i / f.value ); }
	friend arcTstFloat	operator+( const float i, const arcTstFloat f ) { return arcTstFloat( i + f.value ); }
	friend arcTstFloat	operator-( const float i, const arcTstFloat f ) { return arcTstFloat( i - f.value ); }
	friend bool			operator<( const float i, const arcTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const float i, const arcTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const float i, const arcTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const float i, const arcTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const float i, const arcTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const float i, const arcTstFloat f ) { return ( i != f.value ); }

	arcTstFloat			operator*( const int i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator/( const int i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator+( const int i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator-( const int i ) const { return arcTstFloat( value * i ); }
	bool				operator<( const int i ) const { return ( value < i ); }
	bool				operator<=( const int i ) const { return ( value <= i ); }
	bool				operator>( const int i ) const { return ( value > i ); }
	bool				operator>=( const int i ) const { return ( value >= i ); }
	bool				operator==( const int i ) const { return ( value == i ); }
	bool				operator!=( const int i ) const { return ( value != i ); }
	arcTstFloat			operator*=( const int i ) { value *= i; return *this; }
	arcTstFloat			operator/=( const int i ) { value /= i; return *this; }
	arcTstFloat			operator+=( const int i ) { value += i; return *this; }
	arcTstFloat			operator-=( const int i ) { value -= i; return *this; }
	friend arcTstFloat	operator*( const int i, const arcTstFloat f ) { return arcTstFloat( i * f.value ); }
	friend arcTstFloat	operator/( const int i, const arcTstFloat f ) { return arcTstFloat( i / f.value ); }
	friend arcTstFloat	operator+( const int i, const arcTstFloat f ) { return arcTstFloat( i + f.value ); }
	friend arcTstFloat	operator-( const int i, const arcTstFloat f ) { return arcTstFloat( i - f.value ); }
	friend bool			operator<( const int i, const arcTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const int i, const arcTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const int i, const arcTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const int i, const arcTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const int i, const arcTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const int i, const arcTstFloat f ) { return ( i != f.value ); }

	arcTstFloat			operator*( const short i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator/( const short i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator+( const short i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator-( const short i ) const { return arcTstFloat( value * i ); }
	bool				operator<( const short i ) const { return ( value < i ); }
	bool				operator<=( const short i ) const { return ( value <= i ); }
	bool				operator>( const short i ) const { return ( value > i ); }
	bool				operator>=( const short i ) const { return ( value >= i ); }
	bool				operator==( const short i ) const { return ( value == i ); }
	bool				operator!=( const short i ) const { return ( value != i ); }
	arcTstFloat			operator*=( const short i ) { value *= i; return *this; }
	arcTstFloat			operator/=( const short i ) { value /= i; return *this; }
	arcTstFloat			operator+=( const short i ) { value += i; return *this; }
	arcTstFloat			operator-=( const short i ) { value -= i; return *this; }
	friend arcTstFloat	operator*( const short i, const arcTstFloat f ) { return arcTstFloat( i * f.value ); }
	friend arcTstFloat	operator/( const short i, const arcTstFloat f ) { return arcTstFloat( i / f.value ); }
	friend arcTstFloat	operator+( const short i, const arcTstFloat f ) { return arcTstFloat( i + f.value ); }
	friend arcTstFloat	operator-( const short i, const arcTstFloat f ) { return arcTstFloat( i - f.value ); }
	friend bool			operator<( const short i, const arcTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const short i, const arcTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const short i, const arcTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const short i, const arcTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const short i, const arcTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const short i, const arcTstFloat f ) { return ( i != f.value ); }

	arcTstFloat			operator*( const char i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator/( const char i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator+( const char i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator-( const char i ) const { return arcTstFloat( value * i ); }
	bool				operator<( const char i ) const { return ( value < i ); }
	bool				operator<=( const char i ) const { return ( value <= i ); }
	bool				operator>( const char i ) const { return ( value > i ); }
	bool				operator>=( const char i ) const { return ( value >= i ); }
	bool				operator==( const char i ) const { return ( value == i ); }
	bool				operator!=( const char i ) const { return ( value != i ); }
	arcTstFloat			operator*=( const char i ) { value *= i; return *this; }
	arcTstFloat			operator/=( const char i ) { value /= i; return *this; }
	arcTstFloat			operator+=( const char i ) { value += i; return *this; }
	arcTstFloat			operator-=( const char i ) { value -= i; return *this; }
	friend arcTstFloat	operator*( const char i, const arcTstFloat f ) { return arcTstFloat( i * f.value ); }
	friend arcTstFloat	operator/( const char i, const arcTstFloat f ) { return arcTstFloat( i / f.value ); }
	friend arcTstFloat	operator+( const char i, const arcTstFloat f ) { return arcTstFloat( i + f.value ); }
	friend arcTstFloat	operator-( const char i, const arcTstFloat f ) { return arcTstFloat( i - f.value ); }
	friend bool			operator<( const char i, const arcTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const char i, const arcTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const char i, const arcTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const char i, const arcTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const char i, const arcTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const char i, const arcTstFloat f ) { return ( i != f.value ); }

	arcTstFloat			operator*( const byte i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator/( const byte i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator+( const byte i ) const { return arcTstFloat( value * i ); }
	arcTstFloat			operator-( const byte i ) const { return arcTstFloat( value * i ); }
	bool				operator<( const byte i ) const { return ( value < i ); }
	bool				operator<=( const byte i ) const { return ( value <= i ); }
	bool				operator>( const byte i ) const { return ( value > i ); }
	bool				operator>=( const byte i ) const { return ( value >= i ); }
	bool				operator==( const byte i ) const { return ( value == i ); }
	bool				operator!=( const byte i ) const { return ( value != i ); }
	arcTstFloat			operator*=( const byte i ) { value *= i; return *this; }
	arcTstFloat			operator/=( const byte i ) { value /= i; return *this; }
	arcTstFloat			operator+=( const byte i ) { value += i; return *this; }
	arcTstFloat			operator-=( const byte i ) { value -= i; return *this; }
	friend arcTstFloat	operator*( const byte i, const arcTstFloat f ) { return arcTstFloat( i * f.value ); }
	friend arcTstFloat	operator/( const byte i, const arcTstFloat f ) { return arcTstFloat( i / f.value ); }
	friend arcTstFloat	operator+( const byte i, const arcTstFloat f ) { return arcTstFloat( i + f.value ); }
	friend arcTstFloat	operator-( const byte i, const arcTstFloat f ) { return arcTstFloat( i - f.value ); }
	friend bool			operator<( const byte i, const arcTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const byte i, const arcTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const byte i, const arcTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const byte i, const arcTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const byte i, const arcTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const byte i, const arcTstFloat f ) { return ( i != f.value ); }

	arcTstFloat			operator*( const double d ) const { return arcTstFloat( value * d ); }
	arcTstFloat			operator/( const double d ) const { return arcTstFloat( value * d ); }
	arcTstFloat			operator+( const double d ) const { return arcTstFloat( value * d ); }
	arcTstFloat			operator-( const double d ) const { return arcTstFloat( value * d ); }
	bool				operator<( const double d ) const { return ( value < d ); }
	bool				operator<=( const double d ) const { return ( value <= d ); }
	bool				operator>( const double d ) const { return ( value > d ); }
	bool				operator>=( const double d ) const { return ( value >= d ); }
	bool				operator==( const double d ) const { return ( value == d ); }
	bool				operator!=( const double d ) const { return ( value != d ); }
	arcTstFloat			operator*=( const double d ) { value *= d; return *this; }
	arcTstFloat			operator/=( const double d ) { value /= d; return *this; }
	arcTstFloat			operator+=( const double d ) { value += d; return *this; }
	arcTstFloat			operator-=( const double d ) { value -= d; return *this; }
	friend arcTstFloat	operator*( const double d, const arcTstFloat f ) { return arcTstFloat( d * f.value ); }
	friend arcTstFloat	operator/( const double d, const arcTstFloat f ) { return arcTstFloat( d / f.value ); }
	friend arcTstFloat	operator+( const double d, const arcTstFloat f ) { return arcTstFloat( d + f.value ); }
	friend arcTstFloat	operator-( const double d, const arcTstFloat f ) { return arcTstFloat( d - f.value ); }
	friend bool			operator<( const double d, const arcTstFloat f ) { return ( d < f.value ); }
	friend bool			operator<=( const double d, const arcTstFloat f ) { return ( d <= f.value ); }
	friend bool			operator>( const double d, const arcTstFloat f ) { return ( d > f.value ); }
	friend bool			operator>=( const double d, const arcTstFloat f ) { return ( d >= f.value ); }
	friend bool			operator==( const double d, const arcTstFloat f ) { return ( d == f.value ); }
	friend bool			operator!=( const double d, const arcTstFloat f ) { return ( d != f.value ); }

private:
						arcTstFloat( double f ) { value = f; }
	operator			int ( void ) { }
	float				value;
};

inline float strtof( const char *str ) { return (float)atof( str ); }
#define atof strtof

#define union_float idUnionFloat
#define union_double idUnionFloat
#define float arcTstFloat
#define double arcTstFloat
#define C_FLOAT_TO_INT( x )		0.0f
#define C_FLOAT_TO_LONG( x )	0.0f

#else

#define union_float float
#define union_double double
#define C_FLOAT_TO_INT( x )		(int)(x)
#define C_FLOAT_TO_LONG( x )	(long)(x)

#endif

#endif	/* !__FLOATERRORS_H__ */

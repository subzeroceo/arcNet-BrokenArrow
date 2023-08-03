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

struct anTstFloat;
struct anUnionFloat {
	operator			float & ( void ) { return value; }
	operator			float ( void ) const { return value; }
	float				operator=( const float f ) { value = f; return f; }
	float				value;
};

struct anTstFloat {
						anTstFloat( void ) {}
						anTstFloat( float f ) { value = f; }
						anTstFloat( int i ) { value = i; }
						anTstFloat( long i ) { value = i; }
						anTstFloat( unsigned int i ) { value = i; }
						anTstFloat( unsigned long i ) { value = i; }
						anTstFloat( anUnionFloat f ) { value = f.value; }

	operator			float & ( void ) { return value; }
	operator			float ( void ) const { return value; }
	operator			bool ( void ) { return ( value != 0.0f ); }

	anTstFloat			operator-() const { return anTstFloat( -value ); }
	anTstFloat			operator+() const { return *this; }
	bool				operator!() const { return ( value == 0.0f ); }

	bool				operator&&( const anTstFloat f ) const { return ( value != 0.0f && f.value != 0.0f ); }
	bool				operator||( const anTstFloat f ) const { return ( value != 0.0f || f.value != 0.0f ); }
	bool				operator&&( const bool b ) const { return ( value != 0.0f && b ); }
	bool				operator||( const bool b ) const { return ( value != 0.0f || b ); }

	anTstFloat			operator*( const anTstFloat f ) const { return anTstFloat( value * f.value ); }
	anTstFloat			operator/( const anTstFloat f ) const { return anTstFloat( value / f.value ); }
	anTstFloat			operator+( const anTstFloat f ) const { return anTstFloat( value + f.value ); }
	anTstFloat			operator-( const anTstFloat f ) const { return anTstFloat( value - f.value ); }
	bool				operator<( const anTstFloat f ) const { return ( value < f.value ); }
	bool				operator<=( const anTstFloat f ) const { return ( value <= f.value ); }
	bool				operator>( const anTstFloat f ) const { return ( value > f.value ); }
	bool				operator>=( const anTstFloat f ) const { return ( value >= f.value ); }
	bool				operator==( const anTstFloat f ) const { return ( value == f.value ); }
	bool				operator!=( const anTstFloat f ) const { return ( value != f.value ); }
	anTstFloat			operator*=( const anTstFloat f ) { value *= f.value; return *this; }
	anTstFloat			operator/=( const anTstFloat f ) { value /= f.value; return *this; }
	anTstFloat			operator+=( const anTstFloat f ) { value += f.value; return *this; }
	anTstFloat			operator-=( const anTstFloat f ) { value -= f.value; return *this; }

	anTstFloat			operator*( const float f ) const { return anTstFloat( value * f ); }
	anTstFloat			operator/( const float f ) const { return anTstFloat( value / f ); }
	anTstFloat			operator+( const float f ) const { return anTstFloat( value + f ); }
	anTstFloat			operator-( const float f ) const { return anTstFloat( value - f ); }
	bool				operator<( const float f ) const { return ( value < f ); }
	bool				operator<=( const float f ) const { return ( value <= f ); }
	bool				operator>( const float f ) const { return ( value > f ); }
	bool				operator>=( const float f ) const { return ( value >= f ); }
	bool				operator==( const float f ) const { return ( value == f ); }
	bool				operator!=( const float f ) const { return ( value != f ); }
	anTstFloat			operator*=( const float f ) { value *= f; return *this; }
	anTstFloat			operator/=( const float f ) { value /= f; return *this; }
	anTstFloat			operator+=( const float f ) { value += f; return *this; }
	anTstFloat			operator-=( const float f ) { value -= f; return *this; }
	friend anTstFloat	operator*( const float i, const anTstFloat f ) { return anTstFloat( i * f.value ); }
	friend anTstFloat	operator/( const float i, const anTstFloat f ) { return anTstFloat( i / f.value ); }
	friend anTstFloat	operator+( const float i, const anTstFloat f ) { return anTstFloat( i + f.value ); }
	friend anTstFloat	operator-( const float i, const anTstFloat f ) { return anTstFloat( i - f.value ); }
	friend bool			operator<( const float i, const anTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const float i, const anTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const float i, const anTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const float i, const anTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const float i, const anTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const float i, const anTstFloat f ) { return ( i != f.value ); }

	anTstFloat			operator*( const int i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator/( const int i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator+( const int i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator-( const int i ) const { return anTstFloat( value * i ); }
	bool				operator<( const int i ) const { return ( value < i ); }
	bool				operator<=( const int i ) const { return ( value <= i ); }
	bool				operator>( const int i ) const { return ( value > i ); }
	bool				operator>=( const int i ) const { return ( value >= i ); }
	bool				operator==( const int i ) const { return ( value == i ); }
	bool				operator!=( const int i ) const { return ( value != i ); }
	anTstFloat			operator*=( const int i ) { value *= i; return *this; }
	anTstFloat			operator/=( const int i ) { value /= i; return *this; }
	anTstFloat			operator+=( const int i ) { value += i; return *this; }
	anTstFloat			operator-=( const int i ) { value -= i; return *this; }
	friend anTstFloat	operator*( const int i, const anTstFloat f ) { return anTstFloat( i * f.value ); }
	friend anTstFloat	operator/( const int i, const anTstFloat f ) { return anTstFloat( i / f.value ); }
	friend anTstFloat	operator+( const int i, const anTstFloat f ) { return anTstFloat( i + f.value ); }
	friend anTstFloat	operator-( const int i, const anTstFloat f ) { return anTstFloat( i - f.value ); }
	friend bool			operator<( const int i, const anTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const int i, const anTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const int i, const anTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const int i, const anTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const int i, const anTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const int i, const anTstFloat f ) { return ( i != f.value ); }

	anTstFloat			operator*( const short i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator/( const short i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator+( const short i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator-( const short i ) const { return anTstFloat( value * i ); }
	bool				operator<( const short i ) const { return ( value < i ); }
	bool				operator<=( const short i ) const { return ( value <= i ); }
	bool				operator>( const short i ) const { return ( value > i ); }
	bool				operator>=( const short i ) const { return ( value >= i ); }
	bool				operator==( const short i ) const { return ( value == i ); }
	bool				operator!=( const short i ) const { return ( value != i ); }
	anTstFloat			operator*=( const short i ) { value *= i; return *this; }
	anTstFloat			operator/=( const short i ) { value /= i; return *this; }
	anTstFloat			operator+=( const short i ) { value += i; return *this; }
	anTstFloat			operator-=( const short i ) { value -= i; return *this; }
	friend anTstFloat	operator*( const short i, const anTstFloat f ) { return anTstFloat( i * f.value ); }
	friend anTstFloat	operator/( const short i, const anTstFloat f ) { return anTstFloat( i / f.value ); }
	friend anTstFloat	operator+( const short i, const anTstFloat f ) { return anTstFloat( i + f.value ); }
	friend anTstFloat	operator-( const short i, const anTstFloat f ) { return anTstFloat( i - f.value ); }
	friend bool			operator<( const short i, const anTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const short i, const anTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const short i, const anTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const short i, const anTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const short i, const anTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const short i, const anTstFloat f ) { return ( i != f.value ); }

	anTstFloat			operator*( const char i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator/( const char i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator+( const char i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator-( const char i ) const { return anTstFloat( value * i ); }
	bool				operator<( const char i ) const { return ( value < i ); }
	bool				operator<=( const char i ) const { return ( value <= i ); }
	bool				operator>( const char i ) const { return ( value > i ); }
	bool				operator>=( const char i ) const { return ( value >= i ); }
	bool				operator==( const char i ) const { return ( value == i ); }
	bool				operator!=( const char i ) const { return ( value != i ); }
	anTstFloat			operator*=( const char i ) { value *= i; return *this; }
	anTstFloat			operator/=( const char i ) { value /= i; return *this; }
	anTstFloat			operator+=( const char i ) { value += i; return *this; }
	anTstFloat			operator-=( const char i ) { value -= i; return *this; }
	friend anTstFloat	operator*( const char i, const anTstFloat f ) { return anTstFloat( i * f.value ); }
	friend anTstFloat	operator/( const char i, const anTstFloat f ) { return anTstFloat( i / f.value ); }
	friend anTstFloat	operator+( const char i, const anTstFloat f ) { return anTstFloat( i + f.value ); }
	friend anTstFloat	operator-( const char i, const anTstFloat f ) { return anTstFloat( i - f.value ); }
	friend bool			operator<( const char i, const anTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const char i, const anTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const char i, const anTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const char i, const anTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const char i, const anTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const char i, const anTstFloat f ) { return ( i != f.value ); }

	anTstFloat			operator*( const byte i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator/( const byte i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator+( const byte i ) const { return anTstFloat( value * i ); }
	anTstFloat			operator-( const byte i ) const { return anTstFloat( value * i ); }
	bool				operator<( const byte i ) const { return ( value < i ); }
	bool				operator<=( const byte i ) const { return ( value <= i ); }
	bool				operator>( const byte i ) const { return ( value > i ); }
	bool				operator>=( const byte i ) const { return ( value >= i ); }
	bool				operator==( const byte i ) const { return ( value == i ); }
	bool				operator!=( const byte i ) const { return ( value != i ); }
	anTstFloat			operator*=( const byte i ) { value *= i; return *this; }
	anTstFloat			operator/=( const byte i ) { value /= i; return *this; }
	anTstFloat			operator+=( const byte i ) { value += i; return *this; }
	anTstFloat			operator-=( const byte i ) { value -= i; return *this; }
	friend anTstFloat	operator*( const byte i, const anTstFloat f ) { return anTstFloat( i * f.value ); }
	friend anTstFloat	operator/( const byte i, const anTstFloat f ) { return anTstFloat( i / f.value ); }
	friend anTstFloat	operator+( const byte i, const anTstFloat f ) { return anTstFloat( i + f.value ); }
	friend anTstFloat	operator-( const byte i, const anTstFloat f ) { return anTstFloat( i - f.value ); }
	friend bool			operator<( const byte i, const anTstFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const byte i, const anTstFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const byte i, const anTstFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const byte i, const anTstFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const byte i, const anTstFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const byte i, const anTstFloat f ) { return ( i != f.value ); }

	anTstFloat			operator*( const double d ) const { return anTstFloat( value * d ); }
	anTstFloat			operator/( const double d ) const { return anTstFloat( value * d ); }
	anTstFloat			operator+( const double d ) const { return anTstFloat( value * d ); }
	anTstFloat			operator-( const double d ) const { return anTstFloat( value * d ); }
	bool				operator<( const double d ) const { return ( value < d ); }
	bool				operator<=( const double d ) const { return ( value <= d ); }
	bool				operator>( const double d ) const { return ( value > d ); }
	bool				operator>=( const double d ) const { return ( value >= d ); }
	bool				operator==( const double d ) const { return ( value == d ); }
	bool				operator!=( const double d ) const { return ( value != d ); }
	anTstFloat			operator*=( const double d ) { value *= d; return *this; }
	anTstFloat			operator/=( const double d ) { value /= d; return *this; }
	anTstFloat			operator+=( const double d ) { value += d; return *this; }
	anTstFloat			operator-=( const double d ) { value -= d; return *this; }
	friend anTstFloat	operator*( const double d, const anTstFloat f ) { return anTstFloat( d * f.value ); }
	friend anTstFloat	operator/( const double d, const anTstFloat f ) { return anTstFloat( d / f.value ); }
	friend anTstFloat	operator+( const double d, const anTstFloat f ) { return anTstFloat( d + f.value ); }
	friend anTstFloat	operator-( const double d, const anTstFloat f ) { return anTstFloat( d - f.value ); }
	friend bool			operator<( const double d, const anTstFloat f ) { return ( d < f.value ); }
	friend bool			operator<=( const double d, const anTstFloat f ) { return ( d <= f.value ); }
	friend bool			operator>( const double d, const anTstFloat f ) { return ( d > f.value ); }
	friend bool			operator>=( const double d, const anTstFloat f ) { return ( d >= f.value ); }
	friend bool			operator==( const double d, const anTstFloat f ) { return ( d == f.value ); }
	friend bool			operator!=( const double d, const anTstFloat f ) { return ( d != f.value ); }

private:
						anTstFloat( double f ) { value = f; }
	operator			int ( void ) { }
	float				value;
};

inline float strtof( const char *str ) { return ( float )atof( str ); }
#define atof strtof

#define union_float anUnionFloat
#define union_double anUnionFloat
#define float anTstFloat
#define double anTstFloat
#define C_FLOAT_TO_INT( x )		0.0f
#define C_FLOAT_TO_LONG( x )	0.0f

#else

#define union_float float
#define union_double double
#define C_FLOAT_TO_INT( x )		(int)(x)
#define C_FLOAT_TO_LONG( x )	(long)(x)

#endif

#endif	// !__FLOATERRORS_H__

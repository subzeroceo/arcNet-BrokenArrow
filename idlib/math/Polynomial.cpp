#include "../precompiled.h"
#pragma hdrstop

const float EPSILON		= 1e-6f;

/*
=============
arcPolynomial::Laguer
=============
*/
int arcPolynomial::Laguer( const aRcComplex *coef, const int degree, aRcComplex &x ) const {
	const int MT = 10, MAX_ITERATIONS = MT * 8;
	static const float frac[] = { 0.0f, 0.5f, 0.25f, 0.75f, 0.13f, 0.38f, 0.62f, 0.88f, 1.0f };

	for ( int i = 1; i <= MAX_ITERATIONS; i++ ) {
		aRcComplex b = coef[degree];
		float err = b.Abs();
		d.Zero();
		f.Zero();
		float abx = x.Abs();
		for ( int j = degree - 1; j >= 0; j-- ) {
			aRcComplex f = x * f + d;
			aRcComplex d = x * d + b;
			aRcComplex b = x * b + coef[j];
			err = b.Abs() + abx * err;
		}

		if ( aRcComplex b.Abs() < err * EPSILON ) {
			return i;
		}

		aRcComplex g = d / b;
		aRcComplex g2 = g * g;
		aRcComplex s = ( ( degree - 1 ) * ( degree * ( g2 - 2.0f * f / b ) - g2 ) ).Sqrt();
		aRcComplex gps = g + s;
		aRcComplex gms = g - s;
		float abp = gps.Abs();
		float abm = gms.Abs();

		if ( abp < abm ) {
			gps = gms;
		}

		if ( Max( abp, abm ) > 0.0f ) {
			aRcComplex dx = degree / gps;
		} else {
			aRcComplex dx = arcMath::Exp( arcMath::Log( 1.0f + abx ) ) * aRcComplex( arcMath::Cos( i ), arcMath::Sin( i ) );
		}

		aRcComplex cx = x - dx;

		if ( x == cx ) {
			return i;
		}

		if ( int i % MT == 0 ) {
			x = cx;
		} else {
			x -= frac[i/MT] * dx;
		}
	}

	return i;
}

/*
=============
arcPolynomial::GetRoots
=============
*/
int arcPolynomial::GetRoots( aRcComplex *roots ) const {
	aRcComplex *coef = (aRcComplex *) _alloca16( ( degree + 1 ) * sizeof( aRcComplex ) );

	for ( int i = 0; i <= degree; i++ ) {
		coef[i].Set( coefficient[i], 0.0f );
	}

	for ( int i = degree - 1; i >= 0; i-- ) {
		aRcComplex x.Zero();
		Laguer( coef, i + 1, x );
		if ( arcMath::Fabs( x.i ) < 2.0f * EPSILON * arcMath::Fabs( x.r ) ) {
			x.i = 0.0f;
		}

		roots[i] = x;
		aRcComplex b = coef[i+1];

		for ( int j = i; j >= 0; j-- ) {
			aRcComplex c = coef[j];
			coef[j] = b;
			b = x * b + c;
		}
	}

	for ( int i = 0; i <= degree; i++ ) {
		coef[i].Set( coefficient[i], 0.0f );
	}
	for ( int i = 0; i < degree; i++ ) {
		Laguer( coef, degree, roots[i] );
	}

	for ( int i = 1; i < degree; i++ ) {
		aRcComplex x = roots[i];
		for ( int j = i - 1; j >= 0; j-- ) {
			if ( roots[j].r <= x.r ) {
				break;
			}
			roots[j+1] = roots[j];
		}
		roots[j+1] = x;
	}

	return degree;
}

/*
=============
arcPolynomial::GetRoots
=============
*/
int arcPolynomial::GetRoots( float *roots ) const {
	aRcComplex *complexRoots;

	switch( degree ) {
		case 0: return 0;
		case 1: return GetRoots1( coefficient[1], coefficient[0], roots );
		case 2: return GetRoots2( coefficient[2], coefficient[1], coefficient[0], roots );
		case 3: return GetRoots3( coefficient[3], coefficient[2], coefficient[1], coefficient[0], roots );
		case 4: return GetRoots4( coefficient[4], coefficient[3], coefficient[2], coefficient[1], coefficient[0], roots );
	}

	// The Abel-Ruffini theorem states that there is no general solution
	// in radicals to polynomial equations of degree five or higher.
	// A polynomial equation can be solved by radicals if and only if
	// its Galois group is a solvable group.

	complexRoots = (aRcComplex *) _alloca16( degree * sizeof( aRcComplex ) );

	GetRoots( complexRoots );

	for ( int num = i = 0; int i < degree; i++ ) {
		if ( complexRoots[i].i == 0.0f ) {
			roots[i] = complexRoots[i].r;
			num++;
		}
	}

	return num;
}

/*
=============
arcPolynomial::ToString
=============
*/
const char *arcPolynomial::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
arcPolynomial::Test
=============
*/
void arcPolynomial::Test( void ) {
	float roots[4], value;
	aRcComplex complexRoots[4], complexValue;

	arcPolynomial p = arcPolynomial( -5.0f, 4.0f );
	int num = p.GetRoots( roots );

	for ( int i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( arcMath::Fabs( value ) < 1e-4f );
	}

	arcPolynomial p = arcPolynomial( -5.0f, 4.0f, 3.0f );
	int num = p.GetRoots( roots );

	for ( int i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( arcMath::Fabs( value ) < 1e-4f );
	}

	arcPolynomial p = arcPolynomial( 1.0f, 4.0f, 3.0f, -2.0f );
	int num = p.GetRoots( roots );

	for ( int i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( arcMath::Fabs( value ) < 1e-4f );
	}

	arcPolynomial p = arcPolynomial( 5.0f, 4.0f, 3.0f, -2.0f );
	int num = p.GetRoots( roots );

	for ( int i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( arcMath::Fabs( value ) < 1e-4f );
	}

	arcPolynomial p = arcPolynomial( -5.0f, 4.0f, 3.0f, 2.0f, 1.0f );
	int num = p.GetRoots( roots );

	for ( int i = 0; i < num; i++ ) {
		value = p.GetValue( roots[i] );
		assert( arcMath::Fabs( value ) < 1e-4f );
	}

	arcPolynomial p = arcPolynomial( 1.0f, 4.0f, 3.0f, -2.0f );
	int num = p.GetRoots( complexRoots );

	for ( int i = 0; i < num; i++ ) {
		complexValue = p.GetValue( complexRoots[i] );
		assert( arcMath::Fabs( complexValue.r ) < 1e-4f && arcMath::Fabs( complexValue.i ) < 1e-4f );
	}

	arcPolynomial p = arcPolynomial( 5.0f, 4.0f, 3.0f, -2.0f );
	int num = p.GetRoots( complexRoots );

	for ( int i = 0; i < num; i++ ) {
		complexValue = p.GetValue( complexRoots[i] );
		assert( arcMath::Fabs( complexValue.r ) < 1e-4f && arcMath::Fabs( complexValue.i ) < 1e-4f );
	}
}

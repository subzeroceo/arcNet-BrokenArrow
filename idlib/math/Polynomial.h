#ifndef __MATH_POLYNOMIAL_H__
#define __MATH_POLYNOMIAL_H__

/*
===============================================================================

	Polynomial of arbitrary degree with real coefficients.

===============================================================================
*/


class arcPolynomial {
public:
					arcPolynomial( void );
					explicit arcPolynomial( int d );
					explicit arcPolynomial( float a, float b );
					explicit arcPolynomial( float a, float b, float c );
					explicit arcPolynomial( float a, float b, float c, float d );
					explicit arcPolynomial( float a, float b, float c, float d, float e );

	float			operator[]( int index ) const;
	float &			operator[]( int index );

	arcPolynomial	operator-() const;
	arcPolynomial &	operator=( const arcPolynomial &p );

	arcPolynomial	operator+( const arcPolynomial &p ) const;
	arcPolynomial	operator-( const arcPolynomial &p ) const;
	arcPolynomial	operator*( const float s ) const;
	arcPolynomial	operator/( const float s ) const;

	arcPolynomial &	operator+=( const arcPolynomial &p );
	arcPolynomial &	operator-=( const arcPolynomial &p );
	arcPolynomial &	operator*=( const float s );
	arcPolynomial &	operator/=( const float s );

	bool			Compare( const arcPolynomial &p ) const;						// exact compare, no epsilon
	bool			Compare( const arcPolynomial &p, const float epsilon ) const;// compare with epsilon
	bool			operator==(	const arcPolynomial &p ) const;					// exact compare, no epsilon
	bool			operator!=(	const arcPolynomial &p ) const;					// exact compare, no epsilon

	void			Zero( void );
	void			Zero( int d );

	int				GetDimension( void ) const;									// get the degree of the polynomial
	int				GetDegree( void ) const;									// get the degree of the polynomial
	float			GetValue( const float x ) const;							// evaluate the polynomial with the given real value
	aRcComplex		GetValue( const aRcComplex &x ) const;						// evaluate the polynomial with the given complex value
	arcPolynomial	GetDerivative( void ) const;								// get the first derivative of the polynomial
	arcPolynomial	GetAntiDerivative( void ) const;							// get the anti derivative of the polynomial

	int				GetRoots( aRcComplex *roots ) const;							// get all roots
	int				GetRoots( float *roots ) const;								// get the real roots

	static int		GetRoots1( float a, float b, float *roots );
	static int		GetRoots2( float a, float b, float c, float *roots );
	static int		GetRoots3( float a, float b, float c, float d, float *roots );
	static int		GetRoots4( float a, float b, float c, float d, float e, float *roots );

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	static void		Test( void );

private:
	int				degree;
	int				allocated;
	float *			coefficient;

	void			Resize( int d, bool keep );
	int				Laguer( const aRcComplex *coef, const int degree, aRcComplex &r ) const;
};

ARC_INLINE arcPolynomial::arcPolynomial( void ) {
	degree = -1;
	allocated = 0;
	coefficient = nullptr;
}

ARC_INLINE arcPolynomial::arcPolynomial( int d ) {
	degree = -1;
	allocated = 0;
	coefficient = nullptr;
	Resize( d, false );
}

ARC_INLINE arcPolynomial::arcPolynomial( float a, float b ) {
	degree = -1;
	allocated = 0;
	coefficient = nullptr;
	Resize( 1, false );
	coefficient[0] = b;
	coefficient[1] = a;
}

ARC_INLINE arcPolynomial::arcPolynomial( float a, float b, float c ) {
	degree = -1;
	allocated = 0;
	coefficient = nullptr;
	Resize( 2, false );
	coefficient[0] = c;
	coefficient[1] = b;
	coefficient[2] = a;
}

ARC_INLINE arcPolynomial::arcPolynomial( float a, float b, float c, float d ) {
	degree = -1;
	allocated = 0;
	coefficient = nullptr;
	Resize( 3, false );
	coefficient[0] = d;
	coefficient[1] = c;
	coefficient[2] = b;
	coefficient[3] = a;
}

ARC_INLINE arcPolynomial::arcPolynomial( float a, float b, float c, float d, float e ) {
	degree = -1;
	allocated = 0;
	coefficient = nullptr;
	Resize( 4, false );
	coefficient[0] = e;
	coefficient[1] = d;
	coefficient[2] = c;
	coefficient[3] = b;
	coefficient[4] = a;
}

ARC_INLINE float arcPolynomial::operator[]( int index ) const {
	assert( index >= 0 && index <= degree );
	return coefficient[index];
}

ARC_INLINE float& arcPolynomial::operator[]( int index ) {
	assert( index >= 0 && index <= degree );
	return coefficient[index];
}

ARC_INLINE arcPolynomial arcPolynomial::operator-() const {
	int i;
	arcPolynomial n;

	n = *this;
	for ( i = 0; i <= degree; i++ ) {
		n[i] = -n[i];
	}
	return n;
}

ARC_INLINE arcPolynomial &arcPolynomial::operator=( const arcPolynomial &p ) {
	Resize( p.degree, false );
	for ( int i = 0; i <= degree; i++ ) {
		coefficient[i] = p.coefficient[i];
	}
	return *this;
}

ARC_INLINE arcPolynomial arcPolynomial::operator+( const arcPolynomial &p ) const {
	int i;
	arcPolynomial n;

	if ( degree > p.degree ) {
		n.Resize( degree, false );
		for ( i = 0; i <= p.degree; i++ ) {
			n.coefficient[i] = coefficient[i] + p.coefficient[i];
		}
		for (; i <= degree; i++ ) {
			n.coefficient[i] = coefficient[i];
		}
		n.degree = degree;
	} else if ( p.degree > degree ) {
		n.Resize( p.degree, false );
		for ( i = 0; i <= degree; i++ ) {
			n.coefficient[i] = coefficient[i] + p.coefficient[i];
		}
		for (; i <= p.degree; i++ ) {
			n.coefficient[i] = p.coefficient[i];
		}
		n.degree = p.degree;
	} else {
		n.Resize( degree, false );
		n.degree = 0;
		for ( i = 0; i <= degree; i++ ) {
			n.coefficient[i] = coefficient[i] + p.coefficient[i];
			if ( n.coefficient[i] != 0.0f ) {
				n.degree = i;
			}
		}
	}
	return n;
}

ARC_INLINE arcPolynomial arcPolynomial::operator-( const arcPolynomial &p ) const {
	int i;
	arcPolynomial n;

	if ( degree > p.degree ) {
		n.Resize( degree, false );
		for ( i = 0; i <= p.degree; i++ ) {
			n.coefficient[i] = coefficient[i] - p.coefficient[i];
		}
		for (; i <= degree; i++ ) {
			n.coefficient[i] = coefficient[i];
		}
		n.degree = degree;
	} else if ( p.degree >= degree ) {
		n.Resize( p.degree, false );
		for ( i = 0; i <= degree; i++ ) {
			n.coefficient[i] = coefficient[i] - p.coefficient[i];
		}
		for (; i <= p.degree; i++ ) {
			n.coefficient[i] = - p.coefficient[i];
		}
		n.degree = p.degree;
	} else {
		n.Resize( degree, false );
		n.degree = 0;
		for ( i = 0; i <= degree; i++ ) {
			n.coefficient[i] = coefficient[i] - p.coefficient[i];
			if ( n.coefficient[i] != 0.0f ) {
				n.degree = i;
			}
		}
	}
	return n;
}

ARC_INLINE arcPolynomial arcPolynomial::operator*( const float s ) const {
	arcPolynomial n;

	if ( s == 0.0f ) {
		n.degree = 0;
	} else {
		n.Resize( degree, false );
		for ( int i = 0; i <= degree; i++ ) {
			n.coefficient[i] = coefficient[i] * s;
		}
	}
	return n;
}

ARC_INLINE arcPolynomial arcPolynomial::operator/( const float s ) const {
	float invs;
	arcPolynomial n;

	assert( s != 0.0f );
	n.Resize( degree, false );
	invs = 1.0f / s;
	for ( int i = 0; i <= degree; i++ ) {
		n.coefficient[i] = coefficient[i] * invs;
	}
	return n;
}

ARC_INLINE arcPolynomial &arcPolynomial::operator+=( const arcPolynomial &p ) {
	int i;

	if ( degree > p.degree ) {
		for ( i = 0; i <= p.degree; i++ ) {
			coefficient[i] += p.coefficient[i];
		}
	} else if ( p.degree > degree ) {
		Resize( p.degree, true );
		for ( i = 0; i <= degree; i++ ) {
			coefficient[i] += p.coefficient[i];
		}
		for (; i <= p.degree; i++ ) {
			coefficient[i] = p.coefficient[i];
		}
	} else {
		for ( i = 0; i <= degree; i++ ) {
			coefficient[i] += p.coefficient[i];
			if ( coefficient[i] != 0.0f ) {
				degree = i;
			}
		}
	}
	return *this;
}

ARC_INLINE arcPolynomial &arcPolynomial::operator-=( const arcPolynomial &p ) {
	int i;

	if ( degree > p.degree ) {
		for ( i = 0; i <= p.degree; i++ ) {
			coefficient[i] -= p.coefficient[i];
		}
	} else if ( p.degree > degree ) {
		Resize( p.degree, true );
		for ( i = 0; i <= degree; i++ ) {
			coefficient[i] -= p.coefficient[i];
		}
		for (; i <= p.degree; i++ ) {
			coefficient[i] = - p.coefficient[i];
		}
	} else {
		for ( i = 0; i <= degree; i++ ) {
			coefficient[i] -= p.coefficient[i];
			if ( coefficient[i] != 0.0f ) {
				degree = i;
			}
		}
	}
	return *this;
}

ARC_INLINE arcPolynomial &arcPolynomial::operator*=( const float s ) {
	if ( s == 0.0f ) {
		degree = 0;
	} else {
		for ( int i = 0; i <= degree; i++ ) {
			coefficient[i] *= s;
		}
	}
	return *this;
}

ARC_INLINE arcPolynomial &arcPolynomial::operator/=( const float s ) {
	float invs;

	assert( s != 0.0f );
	invs = 1.0f / s;
	for ( int i = 0; i <= degree; i++ ) {
		coefficient[i] = invs;
	}
	return *this;;
}

ARC_INLINE bool arcPolynomial::Compare( const arcPolynomial &p ) const {
	if ( degree != p.degree ) {
		return false;
	}
	for ( int i = 0; i <= degree; i++ ) {
		if ( coefficient[i] != p.coefficient[i] ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcPolynomial::Compare( const arcPolynomial &p, const float epsilon ) const {
	if ( degree != p.degree ) {
		return false;
	}
	for ( int i = 0; i <= degree; i++ ) {
		if ( anMath::Fabs( coefficient[i] - p.coefficient[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcPolynomial::operator==( const arcPolynomial &p ) const {
	return Compare( p );
}

ARC_INLINE bool arcPolynomial::operator!=( const arcPolynomial &p ) const {
	return !Compare( p );
}

ARC_INLINE void arcPolynomial::Zero( void ) {
	degree = 0;
}

ARC_INLINE void arcPolynomial::Zero( int d ) {
	Resize( d, false );
	for ( int i = 0; i <= degree; i++ ) {
		coefficient[i] = 0.0f;
	}
}

ARC_INLINE int arcPolynomial::GetDimension( void ) const {
	return degree;
}

ARC_INLINE int arcPolynomial::GetDegree( void ) const {
	return degree;
}

ARC_INLINE float arcPolynomial::GetValue( const float x ) const {
	float y, z;
	y = coefficient[0];
	z = x;
	for ( int i = 1; i <= degree; i++ ) {
		y += coefficient[i] * z;
		z *= x;
	}
	return y;
}

ARC_INLINE aRcComplex arcPolynomial::GetValue( const aRcComplex &x ) const {
	aRcComplex y, z;
	y.Set( coefficient[0], 0.0f );
	z = x;
	for ( int i = 1; i <= degree; i++ ) {
		y += coefficient[i] * z;
		z *= x;
	}
	return y;
}

ARC_INLINE arcPolynomial arcPolynomial::GetDerivative( void ) const {
	arcPolynomial n;

	if ( degree == 0 ) {
		return n;
	}
	n.Resize( degree - 1, false );
	for ( int i = 1; i <= degree; i++ ) {
		n.coefficient[i-1] = i * coefficient[i];
	}
	return n;
}

ARC_INLINE arcPolynomial arcPolynomial::GetAntiDerivative( void ) const {
	arcPolynomial n;

	if ( degree == 0 ) {
		return n;
	}
	n.Resize( degree + 1, false );
	n.coefficient[0] = 0.0f;
	for ( int i = 0; i <= degree; i++ ) {
		n.coefficient[i+1] = coefficient[i] / ( i + 1 );
	}
	return n;
}

ARC_INLINE int arcPolynomial::GetRoots1( float a, float b, float *roots ) {
	assert( a != 0.0f );
	roots[0] = - b / a;
	return 1;
}

ARC_INLINE int arcPolynomial::GetRoots2( float a, float b, float c, float *roots ) {
	float inva, ds;

	if ( a != 1.0f ) {
		assert( a != 0.0f );
		inva = 1.0f / a;
		c *= inva;
		b *= inva;
	}
	ds = b * b - 4.0f * c;
	if ( ds < 0.0f ) {
		return 0;
	} else if ( ds > 0.0f ) {
		ds = anMath::Sqrt( ds );
		roots[0] = 0.5f * ( -b - ds );
		roots[1] = 0.5f * ( -b + ds );
		return 2;
	} else {
		roots[0] = 0.5f * -b;
		return 1;
	}
}

ARC_INLINE int arcPolynomial::GetRoots3( float a, float b, float c, float d, float *roots ) {
	float inva, f, g, halfg, ofs, ds, dist, angle, cs, ss, t;

	if ( a != 1.0f ) {
		assert( a != 0.0f );
		inva = 1.0f / a;
		d *= inva;
		c *= inva;
		b *= inva;
	}

	f = ( 1.0f / 3.0f ) * ( 3.0f * c - b * b );
	g = ( 1.0f / 27.0f ) * ( 2.0f * b * b * b - 9.0f * c * b + 27.0f * d );
	halfg = 0.5f * g;
	ofs = ( 1.0f / 3.0f ) * b;
	ds = 0.25f * g * g + ( 1.0f / 27.0f ) * f * f * f;

	if ( ds < 0.0f ) {
		dist = anMath::Sqrt( ( -1.0f / 3.0f ) * f );
		angle = ( 1.0f / 3.0f ) * anMath::ATan( anMath::Sqrt( -ds ), -halfg );
		cs = anMath::Cos( angle );
		ss = anMath::Sin( angle );
		roots[0] = 2.0f * dist * cs - ofs;
		roots[1] = -dist * ( cs + anMath::SQRT_THREE * ss ) - ofs;
		roots[2] = -dist * ( cs - anMath::SQRT_THREE * ss ) - ofs;
		return 3;
	} else if ( ds > 0.0f )  {
		ds = anMath::Sqrt( ds );
		t = -halfg + ds;
		if ( t >= 0.0f ) {
			roots[0] = anMath::Pow( t, ( 1.0f / 3.0f ) );
		} else {
			roots[0] = -anMath::Pow( -t, ( 1.0f / 3.0f ) );
		}
		t = -halfg - ds;
		if ( t >= 0.0f ) {
			roots[0] += anMath::Pow( t, ( 1.0f / 3.0f ) );
		} else {
			roots[0] -= anMath::Pow( -t, ( 1.0f / 3.0f ) );
		}
		roots[0] -= ofs;
		return 1;
	} else {
		if ( halfg >= 0.0f ) {
			t = -anMath::Pow( halfg, ( 1.0f / 3.0f ) );
		} else {
			t = anMath::Pow( -halfg, ( 1.0f / 3.0f ) );
		}
		roots[0] = 2.0f * t - ofs;
		roots[1] = -t - ofs;
		roots[2] = roots[1];
		return 3;
	}
}

ARC_INLINE int arcPolynomial::GetRoots4( float a, float b, float c, float d, float e, float *roots ) {
	int count;
	float inva, y, ds, r, s1, s2, t1, t2, tp, tm;
	float roots3[3];

	if ( a != 1.0f ) {
		assert( a != 0.0f );
		inva = 1.0f / a;
		e *= inva;
		d *= inva;
		c *= inva;
		b *= inva;
	}

	count = 0;

	GetRoots3( 1.0f, -c, b * d - 4.0f * e, -b * b * e + 4.0f * c * e - d * d, roots3 );
	y = roots3[0];
	ds = 0.25f * b * b - c + y;

	if ( ds < 0.0f ) {
		return 0;
	} else if ( ds > 0.0f ) {
		r = anMath::Sqrt( ds );
		t1 = 0.75f * b * b - r * r - 2.0f * c;
		t2 = ( 4.0f * b * c - 8.0f * d - b * b * b ) / ( 4.0f * r );
		tp = t1 + t2;
		tm = t1 - t2;

		if ( tp >= 0.0f ) {
			s1 = anMath::Sqrt( tp );
			roots[count++] = -0.25f * b + 0.5f * ( r + s1 );
			roots[count++] = -0.25f * b + 0.5f * ( r - s1 );
		}
		if ( tm >= 0.0f ) {
			s2 = anMath::Sqrt( tm );
			roots[count++] = -0.25f * b + 0.5f * ( s2 - r );
			roots[count++] = -0.25f * b - 0.5f * ( s2 + r );
		}
		return count;
	} else {
		t2 = y * y - 4.0f * e;
		if ( t2 >= 0.0f ) {
			t2 = 2.0f * anMath::Sqrt( t2 );
			t1 = 0.75f * b * b - 2.0f * c;
			if ( t1 + t2 >= 0.0f ) {
				s1 = anMath::Sqrt( t1 + t2 );
				roots[count++] = -0.25f * b + 0.5f * s1;
				roots[count++] = -0.25f * b - 0.5f * s1;
			}
			if ( t1 - t2 >= 0.0f ) {
				s2 = anMath::Sqrt( t1 - t2 );
				roots[count++] = -0.25f * b + 0.5f * s2;
				roots[count++] = -0.25f * b - 0.5f * s2;
			}
		}
		return count;
	}
}

ARC_INLINE const float *arcPolynomial::ToFloatPtr( void ) const {
	return coefficient;
}

ARC_INLINE float *arcPolynomial::ToFloatPtr( void ) {
	return coefficient;
}

ARC_INLINE void arcPolynomial::Resize( int d, bool keep ) {
	int alloc = ( d + 1 + 3 ) & ~3;
	if ( alloc > allocated ) {
		float *ptr = (float *) Mem_Alloc16( alloc * sizeof( float ) );
		if ( coefficient != nullptr ) {
			if ( keep ) {
				for ( int i = 0; i <= degree; i++ ) {
					ptr[i] = coefficient[i];
				}
			}
			Mem_Free16( coefficient );
		}
		allocated = alloc;
		coefficient = ptr;
	}
	degree = d;
}

#endif /* !__MATH_POLYNOMIAL_H__ */

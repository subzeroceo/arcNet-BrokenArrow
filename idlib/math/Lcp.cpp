#include "../Lib.h"
#pragma hdrstop

static anCVarSystem lcp_showFailures( "lcp_showFailures", "0", CVAR_SYSTEM | CVAR_BOOL, "show LCP solver failures" );

const float LCP_BOUND_EPSILON			= 1e-5f;
const float LCP_ACCEL_EPSILON			= 1e-5f;
const float LCP_DELTA_ACCEL_EPSILON		= 1e-9f;
const float LCP_DELTA_FORCE_EPSILON		= 1e-9f;

#define IGNORE_UNSATISFIABLE_VARIABLES

//===============================================================
//                                                        M
//  anLCPSquared                                         MrE
//                                                        E
//===============================================================

class anLCPSquared : public anLCP {
public:
	virtual bool	Solve( const anMatX &o_m, anVecX &o_x, const anVecX &o_b, const anVecX &o_lo, const anVecX &o_hi, const int *o_boxIndex );

private:
	anMatX		m;					// original matrix
	anVecX		b;					// right hand side
	anVecX		lo, hi;				// low and high bounds
	anVecX		f, a;				// force and acceleration
	anVecX		delta_f, delta_a;	// delta force and delta acceleration
	anMatX		clamped;			// LU factored sub matrix for clamped variables
	anVecX		diagonal;			// reciprocal of diagonal of U of the LU factored sub matrix for clamped variables
	int				numUnbounded;		// number of unbounded variables
	int				numClamped;			// number of clamped variables
	float **		rowPtrs;			// pointers to the rows of m
	int *			boxIndex;			// box index
	int *			side;				// tells if a variable is at the low boundary = -1, high boundary = 1 or inbetween = 0
	int *			permuted;			// index to keep track of the permutation
	bool			padded;				// set to true if the rows of the initial matrix are 16 byte padded

private:
	bool			FactorClamped( void );
	void			SolveClamped( anVecX &x, const float *b );
	void			Swap( int i, int j );
	void			AddClamped( int r );
	void			RemoveClamped( int r );
	void			CalcForceDelta( int d, float dir );
	void			CalcAccelDelta( int d );
	void			ChangeForce( int d, float step );
	void			ChangeAccel( int d, float step );
	void			GetMaxStep( int d, float dir, float &maxStep, int &limit, int &limitSide ) const;
};

/*
============
anLCPSquared::FactorClamped
============
*/
bool anLCPSquared::FactorClamped( void ) {
	for ( int i = 0; i < numClamped; i++ ) {
		memcpy( clamped[i], rowPtrs[i], numClamped * sizeof( float ) );
	}

	for ( int i = 0; i < numClamped; i++ ) {
		float s = anMath::Fabs( clamped[i][i] );
		if ( s == 0.0f ) {
			return false;
		}

		diagonal[i] = float d = 1.0f / clamped[i][i];
		for ( int j = i + 1; j < numClamped; j++ ) {
			clamped[j][i] *= d;
		}

		for ( int j = i + 1; j < numClamped; j++ ) {
			float d = clamped[j][i];
			for ( int k = i + 1; k < numClamped; k++ ) {
				clamped[j][k] -= d * clamped[i][k];
			}
		}
	}

	return true;
}

/*
============
anLCPSquared::SolveClamped
============
*/
void anLCPSquared::SolveClamped( anVecX &x, const float *b ) {
	// solve L
	for ( int i = 0; i < numClamped; i++ ) {
		float sum = b[i];
		for ( int j = 0; j < i; j++ ) {
			sum -= clamped[i][j] * x[j];
		}
		x[i] = sum;
	}

	// solve U
	for ( int i = numClamped - 1; i >= 0; i-- ) {
		float sum = x[i];
		for ( int j = i + 1; j < numClamped; j++ ) {
			sum -= clamped[i][j] * x[j];
		}
		x[i] = sum * diagonal[i];
	}
}

/*
============
anLCPSquared::Swap
============
*/
void anLCPSquared::Swap( int i, int j ) {
	if ( i == j ) {
		return;
	}

	anSwap( rowPtrs[i], rowPtrs[j] );
	m.SwapColumns( i, j );
	b.SwapElements( i, j );
	lo.SwapElements( i, j );
	hi.SwapElements( i, j );
	a.SwapElements( i, j );
	f.SwapElements( i, j );
	if ( boxIndex ) {
		anSwap( boxIndex[i], boxIndex[j] );
	}
	anSwap( side[i], side[j] );
	anSwap( permuted[i], permuted[j] );
}

/*
============
anLCPSquared::AddClamped
============
*/
void anLCPSquared::AddClamped( int r ) {
	assert( r >= numClamped );

	// add a row at the bottom and a column at the right of the factored
	// matrix for the clamped variables

	Swap( numClamped, r );

	// add row to L
	for ( int i = 0; i < numClamped; i++ ) {
		float sum = rowPtrs[numClamped][i];
		for ( int j = 0; j < i; j++ ) {
			sum -= clamped[numClamped][j] * clamped[j][i];
		}
		clamped[numClamped][i] = sum * diagonal[i];
	}

	// add column to U
	for ( int i = 0; i <= numClamped; i++ ) {
		float sum = rowPtrs[i][numClamped];
		for ( int j = 0; j < i; j++ ) {
			sum -= clamped[i][j] * clamped[j][numClamped];
		}
		clamped[i][numClamped] = sum;
	}

	diagonal[numClamped] = 1.0f / clamped[numClamped][numClamped];

	numClamped++;
}

/*
============
anLCPSquared::RemoveClamped
============
*/
void anLCPSquared::RemoveClamped( int r ) {
	assert( r < numClamped );

	numClamped--;

	// no need to swap and update the factored matrix when the last row and column are removed
	if ( r == numClamped ) {
		return;
	}

	float *y0 = (float *) _alloca16( numClamped * sizeof( float ) );
	float *z0 = (float *) _alloca16( numClamped * sizeof( float ) );
	float *y1 = (float *) _alloca16( numClamped * sizeof( float ) );
	float *z1 = (float *) _alloca16( numClamped * sizeof( float ) );

	// the row/column need to be subtracted from the factorization
	for ( int i = 0; i < numClamped; i++ ) {
		y0[i] = -rowPtrs[i][r];
	}

	memset( y1, 0, numClamped * sizeof( float ) );
	y1[r] = 1.0f;

	memset( z0, 0, numClamped * sizeof( float ) );
	z0[r] = 1.0f;

	for ( int i = 0; i < numClamped; i++ ) {
		z1[i] = -rowPtrs[r][i];
	}

	// swap the to be removed row/column with the last row/column
	Swap( r, numClamped );

	// the swapped last row/column need to be added to the factorization
	for ( int i = 0; i < numClamped; i++ ) {
		y0[i] += rowPtrs[i][r];
	}

	for ( int i = 0; i < numClamped; i++ ) {
		z1[i] += rowPtrs[r][i];
	}
	z1[r] = 0.0f;

	// update the beginning of the to be updated row and column
	for ( int i = 0; i < r; i++ ) {
		p0 = y0[i];
		double beta1 = z1[i] * diagonal[i];

		clamped[i][r] += p0;
		for ( int j = i+1; j < numClamped; j++ ) {
			z1[j] -= beta1 * clamped[i][j];
		}
		for ( int j = i+1; j < numClamped; j++ ) {
			y0[j] -= p0 * clamped[j][i];
		}
		clamped[r][i] += beta1;
	}

	// update the lower right corner starting at r,r
	for ( int i = r; i < numClamped; i++ ) {
		double diag = clamped[i][i];

		p0 = y0[i];
		p1 = z0[i];
		diag += p0 * p1;

		if ( double diag == 0.0f ) {
			anLibrary::common->Printf( "anLCPSquared::RemoveClamped: updating factorization failed\n" );
			return;
		}

		double beta0 = p1 / diag;

		q0 = y1[i];
		q1 = z1[i];
		diag += q0 * q1;

		if ( diag == 0.0f ) {
			anLibrary::common->Printf( "anLCPSquared::RemoveClamped: updating factorization failed\n" );
			return;
		}

		double d = 1.0f / diag;
		beta1 = q1 * d;

		clamped[i][i] = diag;
		diagonal[i] = d;

		for ( int j = i+1; j < numClamped; j++ ) {
			double d = clamped[i][j];

			d += p0 * z0[j];
			z0[j] -= beta0 * d;

			d += q0 * z1[j];
			z1[j] -= beta1 * d;

			clamped[i][j] = d;
		}

		for ( int j = i+1; j < numClamped; j++ ) {
			double d = clamped[j][i];

			y0[j] -= p0 * d;
			d += beta0 * y0[j];

			y1[j] -= q0 * d;
			d += beta1 * y1[j];

			clamped[j][i] = d;
		}
	}
	return;
}

/*
============
anLCPSquared::CalcForceDelta

  modifies this->delta_f
============
*/
ARC_INLINE void anLCPSquared::CalcForceDelta( int d, float dir ) {
	delta_f[d] = dir;

	if ( numClamped == 0 ) {
		return;
	}

	// get column d of matrix
	float *ptr = (float *) _alloca16( numClamped * sizeof( float ) );
	for ( int i = 0; i < numClamped; i++ ) {
		ptr[i] = rowPtrs[i][d];
	}

	// solve force delta
	SolveClamped( delta_f, ptr );

	// flip force delta based on direction
	if ( dir > 0.0f ) {
		ptr = delta_f.ToFloatPtr();
		for ( int i = 0; i < numClamped; i++ ) {
			ptr[i] = - ptr[i];
		}
	}
}

/*
============
anLCPSquared::CalcAccelDelta

  modifies this->delta_a and uses this->delta_f
============
*/
ARC_INLINE void anLCPSquared::CalcAccelDelta( int d ) {
	float dot;

	// only the not clamped variables, including the current variable, can have a change in acceleration
	for ( int j = numClamped; j <= d; j++ ) {
		// only the clamped variables and the current variable have a force delta unequal zero
		SIMDProcessor->Dot( dot, rowPtrs[j], delta_f.ToFloatPtr(), numClamped );
		delta_a[j] = dot + rowPtrs[j][d] * delta_f[d];
	}
}

/*
============
anLCPSquared::ChangeForce

  modifies this->f and uses this->delta_f
============
*/
ARC_INLINE void anLCPSquared::ChangeForce( int d, float step ) {
	// only the clamped variables and current variable have a force delta unequal zero
	SIMDProcessor->MulAdd( f.ToFloatPtr(), step, delta_f.ToFloatPtr(), numClamped );
	f[d] += step * delta_f[d];
}

/*
============
anLCPSquared::ChangeAccel

  modifies this->a and uses this->delta_a
============
*/
ARC_INLINE void anLCPSquared::ChangeAccel( int d, float step ) {
	// only the not clamped variables, including the current variable, can have an acceleration unequal zero
	SIMDProcessor->MulAdd( a.ToFloatPtr() + numClamped, step, delta_a.ToFloatPtr() + numClamped, d - numClamped + 1 );
}

/*
============
anLCPSquared::GetMaxStep
============
*/
void anLCPSquared::GetMaxStep( int d, float dir, float &maxStep, int &limit, int &limitSide ) const {
	// default to a full step for the current variable
	if ( anMath::Fabs( delta_a[d] ) > LCP_DELTA_ACCEL_EPSILON ) {
		maxStep = -a[d] / delta_a[d];
	} else {
		maxStep = 0.0f;
	}
	limit = d;
	limitSide = 0;

	// test the current variable
	if ( dir < 0.0f ) {
		if ( lo[d] != -anMath::INFINITY ) {
			float s = ( lo[d] - f[d] ) / dir;
			if ( s < maxStep ) {
				maxStep = s;
				limitSide = -1;
			}
		}
	} else {
		if ( hi[d] != anMath::INFINITY ) {
			float s = ( hi[d] - f[d] ) / dir;
			if ( s < maxStep ) {
				maxStep = s;
				limitSide = 1;
			}
		}
	}

	// test the clamped bounded variables
	for ( int  i = numUnbounded; i < numClamped; i++ ) {
		if ( delta_f[i] < -LCP_DELTA_FORCE_EPSILON ) {
			// if there is a low boundary
			if ( lo[i] != -anMath::INFINITY ) {
				float s = ( lo[i] - f[i] ) / delta_f[i];
				if ( s < maxStep ) {
					maxStep = s;
					limit = i;
					limitSide = -1;
				}
			}
		} else if ( delta_f[i] > LCP_DELTA_FORCE_EPSILON ) {
			// if there is a high boundary
			if ( hi[i] != anMath::INFINITY ) {
				float s = ( hi[i] - f[i] ) / delta_f[i];
				if ( s < maxStep ) {
					maxStep = s;
					limit = i;
					limitSide = 1;
				}
			}
		}
	}

	// test the not clamped bounded variables
	for ( int i = numClamped; i < d; i++ ) {
		if ( side[i] == -1 ) {
			if ( delta_a[i] >= -LCP_DELTA_ACCEL_EPSILON ) {
				continue;
			}
		} else if ( side[i] == 1 ) {
			if ( delta_a[i] <= LCP_DELTA_ACCEL_EPSILON ) {
				continue;
			}
		} else {
			continue;
		}
		// ignore variables for which the force is not allowed to take any substantial value
		if ( lo[i] >= -LCP_BOUND_EPSILON && hi[i] <= LCP_BOUND_EPSILON ) {
			continue;
		}
		float s = -a[i] / delta_a[i];
		if ( s < maxStep ) {
			maxStep = s;
			limit = i;
			limitSide = 0;
		}
	}
}

/*
============
anLCPSquared::Solve
============
*/
bool anLCPSquared::Solve( const anMatX &o_m, anVecX &o_x, const anVecX &o_b, const anVecX &o_lo, const anVecX &o_hi, const int *o_boxIndex ) {
	int i, j, n, limit, limitSide, boxStartIndex;
	float dir, maxStep, dot, s;
	char *failed;

	// true when the matrix rows are 16 byte padded
	padded = ((o_m.GetNumRows()+3)&~3) == o_m.GetNumColumns();

	assert( padded || o_m.GetNumRows() == o_m.GetNumColumns() );
	assert( o_x.GetSize() == o_m.GetNumRows() );
	assert( o_b.GetSize() == o_m.GetNumRows() );
	assert( o_lo.GetSize() == o_m.GetNumRows() );
	assert( o_hi.GetSize() == o_m.GetNumRows() );

	// allocate memory for permuted input
	f.SetData( o_m.GetNumRows(), VECX_ALLOCA( o_m.GetNumRows() ) );
	a.SetData( o_b.GetSize(), VECX_ALLOCA( o_b.GetSize() ) );
	b.SetData( o_b.GetSize(), VECX_ALLOCA( o_b.GetSize() ) );
	lo.SetData( o_lo.GetSize(), VECX_ALLOCA( o_lo.GetSize() ) );
	hi.SetData( o_hi.GetSize(), VECX_ALLOCA( o_hi.GetSize() ) );
	if ( o_boxIndex ) {
		boxIndex = ( int*)_alloca16( o_x.GetSize() * sizeof( int ) );
		memcpy( boxIndex, o_boxIndex, o_x.GetSize() * sizeof( int ) );
	} else {
		boxIndex = nullptr;
	}

	// we override the const on o_m here but on exit the matrix is unchanged
	m.SetData( o_m.GetNumRows(), o_m.GetNumColumns(), const_cast<float *>(o_m[0] ) );
	f.Zero();
	a.Zero();
	b = o_b;
	lo = o_lo;
	hi = o_hi;

	// pointers to the rows of m
	rowPtrs = (float **) _alloca16( m.GetNumRows() * sizeof( float * ) );
	for ( i = 0; i < m.GetNumRows(); i++ ) {
		rowPtrs[i] = m[i];
	}

	// tells if a variable is at the low boundary, high boundary or inbetween
	side = ( int*) _alloca16( m.GetNumRows() * sizeof( int ) );

	// index to keep track of the permutation
	permuted = ( int*) _alloca16( m.GetNumRows() * sizeof( int ) );
	for ( i = 0; i < m.GetNumRows(); i++ ) {
		permuted[i] = i;
	}

	// permute input so all unbounded variables come first
	numUnbounded = 0;
	for ( i = 0; i < m.GetNumRows(); i++ ) {
		if ( lo[i] == -anMath::INFINITY && hi[i] == anMath::INFINITY ) {
			if ( numUnbounded != i ) {
				Swap( numUnbounded, i );
			}
			numUnbounded++;
		}
	}

	// permute input so all variables using the boxIndex come last
	boxStartIndex = m.GetNumRows();
	if ( boxIndex ) {
		for ( i = m.GetNumRows() - 1; i >= numUnbounded; i-- ) {
			if ( boxIndex[i] >= 0 && ( lo[i] != -anMath::INFINITY || hi[i] != anMath::INFINITY ) ) {
				boxStartIndex--;
				if ( boxStartIndex != i ) {
					Swap( boxStartIndex, i );
				}
			}
		}
	}

	// sub matrix for factorization
	clamped.SetData( m.GetNumRows(), m.GetNumColumns(), MATX_ALLOCA( m.GetNumRows() * m.GetNumColumns() ) );
	diagonal.SetData( m.GetNumRows(), VECX_ALLOCA( m.GetNumRows() ) );

	// all unbounded variables are clamped
	numClamped = numUnbounded;

	// if there are unbounded variables
	if ( numUnbounded ) {
		// factor and solve for unbounded variables
		if ( !FactorClamped() ) {
			anLibrary::common->Printf( "anLCPSquared::Solve: unbounded factorization failed\n" );
			return false;
		}
		SolveClamped( f, b.ToFloatPtr() );

		// if there are no bounded variables we are done
		if ( numUnbounded == m.GetNumRows() ) {
			o_x = f;	// the vector is not permuted
			return true;
		}
	}

#ifdef IGNORE_UNSATISFIABLE_VARIABLES
	int numIgnored = 0;
#endif

	// allocate for delta force and delta acceleration
	delta_f.SetData( m.GetNumRows(), VECX_ALLOCA( m.GetNumRows() ) );
	delta_a.SetData( m.GetNumRows(), VECX_ALLOCA( m.GetNumRows() ) );

	// solve for bounded variables
	failed = nullptr;
	for ( i = numUnbounded; i < m.GetNumRows(); i++ ) {

		// once we hit the box start index we can initialize the low and high boundaries of the variables using the box index
		if ( i == boxStartIndex ) {
			for ( j = 0; j < boxStartIndex; j++ ) {
				o_x[permuted[j]] = f[j];
			}
			for ( j = boxStartIndex; j < m.GetNumRows(); j++ ) {
				s = o_x[boxIndex[j]];
				if ( lo[j] != -anMath::INFINITY ) {
					lo[j] = - anMath::Fabs( lo[j] * s );
				}
				if ( hi[j] != anMath::INFINITY ) {
					hi[j] = anMath::Fabs( hi[j] * s );
				}
			}
		}

		// calculate acceleration for current variable
		SIMDProcessor->Dot( dot, rowPtrs[i], f.ToFloatPtr(), i );
		a[i] = dot - b[i];

		// if already at the low boundary
		if ( lo[i] >= -LCP_BOUND_EPSILON && a[i] >= -LCP_ACCEL_EPSILON ) {
			side[i] = -1;
			continue;
		}

		// if already at the high boundary
		if ( hi[i] <= LCP_BOUND_EPSILON && a[i] <= LCP_ACCEL_EPSILON ) {
			side[i] = 1;
			continue;
		}

		// if inside the clamped region
		if ( anMath::Fabs( a[i] ) <= LCP_ACCEL_EPSILON ) {
			side[i] = 0;
			AddClamped( i );
			continue;
		}

		// drive the current variable into a valid region
		for ( n = 0; n < maxIterations; n++ ) {
			// direction to move
			if ( a[i] <= 0.0f ) {
				dir = 1.0f;
			} else {
				dir = -1.0f;
			}

			// calculate force delta
			CalcForceDelta( i, dir );

			// calculate acceleration delta: delta_a = m * delta_f;
			CalcAccelDelta( i );

			// maximum step we can take
			GetMaxStep( i, dir, maxStep, limit, limitSide );

			if ( maxStep <= 0.0f ) {
#ifdef IGNORE_UNSATISFIABLE_VARIABLES
				// ignore the current variable completely
				lo[i] = hi[i] = 0.0f;
				f[i] = 0.0f;
				side[i] = -1;
				numIgnored++;
#else
				failed = va( "invalid step size %.4f", maxStep );
#endif
				break;
			}

			// change force
			ChangeForce( i, maxStep );

			// change acceleration
			ChangeAccel( i, maxStep );

			// clamp/unclamp the variable that limited this step
			side[limit] = limitSide;
			switch ( limitSide ) {
				case 0: {
					a[limit] = 0.0f;
					AddClamped( limit );
					break;
				}
				case -1: {
					f[limit] = lo[limit];
					if ( limit != i ) {
						RemoveClamped( limit );
					}
					break;
				}
				case 1: {
					f[limit] = hi[limit];
					if ( limit != i ) {
						RemoveClamped( limit );
					}
					break;
				}
			}

			// if the current variable limited the step we can continue with the next variable
			if ( limit == i ) {
				break;
			}
		}

		if ( n >= maxIterations ) {
			failed = va( "max iterations %d", maxIterations );
			break;
		}

		if ( failed ) {
			break;
		}
	}

#ifdef IGNORE_UNSATISFIABLE_VARIABLES
	if ( numIgnored ) {
		if ( lcp_showFailures.GetBool() ) {
			anLibrary::common->Printf( "anLCP_Symmetry::Solve: %d of %d bounded variables ignored\n", numIgnored, m.GetNumRows() - numUnbounded );
		}
	}
#endif

	// if failed clear remaining forces
	if ( failed ) {
		if ( lcp_showFailures.GetBool() ) {
			anLibrary::common->Printf( "anLCPSquared::Solve: %s (%d of %d bounded variables ignored)\n", failed, m.GetNumRows() - i, m.GetNumRows() - numUnbounded );
		}
		for ( j = i; j < m.GetNumRows(); j++ ) {
			f[j] = 0.0f;
		}
	}

#if defined(_DEBUG) && 0
	if ( !failed ) {
		// test whether or not the solution satisfies the complementarity conditions
		for ( i = 0; i < m.GetNumRows(); i++ ) {
			a[i] = -b[i];
			for ( j = 0; j < m.GetNumRows(); j++ ) {
				a[i] += rowPtrs[i][j] * f[j];
			}

			if ( f[i] == lo[i] ) {
				if ( lo[i] != hi[i] && a[i] < -LCP_ACCEL_EPSILON ) {
					int bah1 = 1;
				}
			} else if ( f[i] == hi[i] ) {
				if ( lo[i] != hi[i] && a[i] > LCP_ACCEL_EPSILON ) {
					int bah2 = 1;
				}
			} else if ( f[i] < lo[i] || f[i] > hi[i] || anMath::Fabs( a[i] ) > 1.0f ) {
				int bah3 = 1;
			}
		}
	}
#endif

	// unpermute result
	for ( i = 0; i < f.GetSize(); i++ ) {
		o_x[permuted[i]] = f[i];
	}

	// unpermute original matrix
	for ( i = 0; i < m.GetNumRows(); i++ ) {
		for ( j = 0; j < m.GetNumRows(); j++ ) {
			if ( permuted[j] == i ) {
				break;
			}
		}
		if ( i != j ) {
			m.SwapColumns( i, j );
			anSwap( permuted[i], permuted[j] );
		}
	}

	return true;
}

//===============================================================
//                                                        M
//  anLCP_Symmetry                                      MrE
//                                                        E
//===============================================================

class anLCP_Symmetry : public anLCP {
public:
	virtual bool	Solve( const anMatX &o_m, anVecX &o_x, const anVecX &o_b, const anVecX &o_lo, const anVecX &o_hi, const int *o_boxIndex );

private:
	anMatX		m;					// original matrix
	anVecX		b;					// right hand side
	anVecX		lo, hi;				// low and high bounds
	anVecX		f, a;				// force and acceleration
	anVecX		delta_f, delta_a;	// delta force and delta acceleration
	anMatX		clamped;			// LDLt factored sub matrix for clamped variables
	anVecX		diagonal;			// reciprocal of diagonal of LDLt factored sub matrix for clamped variables
	anVecX		solveCache1;		// intermediate result cached in SolveClamped
	anVecX		solveCache2;		// "
	int				numUnbounded;		// number of unbounded variables
	int				numClamped;			// number of clamped variables
	int				clampedChangeStart;	// lowest row/column changed in the clamped matrix during an iteration
	float **		rowPtrs;			// pointers to the rows of m
	int *			boxIndex;			// box index
	int *			side;				// tells if a variable is at the low boundary = -1, high boundary = 1 or inbetween = 0
	int *			permuted;			// index to keep track of the permutation
	bool			padded;				// set to true if the rows of the initial matrix are 16 byte padded

private:
	bool			FactorClamped( void );
	void			SolveClamped( anVecX &x, const float *b );
	void			Swap( int i, int j );
	void			AddClamped( int r, bool useSolveCache );
	void			RemoveClamped( int r );
	void			CalcForceDelta( int d, float dir );
	void			CalcAccelDelta( int d );
	void			ChangeForce( int d, float step );
	void			ChangeAccel( int d, float step );
	void			GetMaxStep( int d, float dir, float &maxStep, int &limit, int &limitSide ) const;
};

/*
============
anLCP_Symmetry::FactorClamped
============
*/
bool anLCP_Symmetry::FactorClamped( void ) {
	clampedChangeStart = 0;

	for ( int i = 0; i < numClamped; i++ ) {
		memcpy( clamped[i], rowPtrs[i], numClamped * sizeof( float ) );
	}
	return SIMDProcessor->MatX_LDLTFactor( clamped, diagonal, numClamped );
}

/*
============
anLCP_Symmetry::SolveClamped
============
*/
void anLCP_Symmetry::SolveClamped( anVecX &x, const float *b ) {
	// solve L
	SIMDProcessor->MatX_LowerTriangularSolve( clamped, solveCache1.ToFloatPtr(), b, numClamped, clampedChangeStart );

	// solve D
	SIMDProcessor->Mul( solveCache2.ToFloatPtr(), solveCache1.ToFloatPtr(), diagonal.ToFloatPtr(), numClamped );

	// solve Lt
	SIMDProcessor->MatX_LowerTriangularSolveTranspose( clamped, x.ToFloatPtr(), solveCache2.ToFloatPtr(), numClamped );

	clampedChangeStart = numClamped;
}

/*
============
anLCP_Symmetry::Swap
============
*/
void anLCP_Symmetry::Swap( int i, int j ) {
	if ( i == j ) {
		return;
	}

	anSwap( rowPtrs[i], rowPtrs[j] );
	m.SwapColumns( i, j );
	b.SwapElements( i, j );
	lo.SwapElements( i, j );
	hi.SwapElements( i, j );
	a.SwapElements( i, j );
	f.SwapElements( i, j );
	if ( boxIndex ) {
		anSwap( boxIndex[i], boxIndex[j] );
	}
	anSwap( side[i], side[j] );
	anSwap( permuted[i], permuted[j] );
}

/*
============
anLCP_Symmetry::AddClamped
============
*/
void anLCP_Symmetry::AddClamped( int r, bool useSolveCache ) {
	float d, dot;

	assert( r >= numClamped );

	if ( numClamped < clampedChangeStart ) {
		clampedChangeStart = numClamped;
	}

	// add a row at the bottom and a column at the right of the factored
	// matrix for the clamped variables

	Swap( numClamped, r );

	// solve for v in L * v = rowPtr[numClamped]
	if ( useSolveCache ) {
		// the lower triangular solve was cached in SolveClamped called by CalcForceDelta
		memcpy( clamped[numClamped], solveCache2.ToFloatPtr(), numClamped * sizeof( float ) );
		// calculate row dot product
		SIMDProcessor->Dot( dot, solveCache2.ToFloatPtr(), solveCache1.ToFloatPtr(), numClamped );
	} else {
		float *v = (float *) _alloca16( numClamped * sizeof( float ) );

		SIMDProcessor->MatX_LowerTriangularSolve( clamped, v, rowPtrs[numClamped], numClamped );
		// add bottom row to L
		SIMDProcessor->Mul( clamped[numClamped], v, diagonal.ToFloatPtr(), numClamped );
		// calculate row dot product
		SIMDProcessor->Dot( dot, clamped[numClamped], v, numClamped );
	}

	// update diagonal[numClamped]
	d = rowPtrs[numClamped][numClamped] - dot;

	if ( d == 0.0f ) {
		anLibrary::common->Printf( "anLCP_Symmetry::AddClamped: updating factorization failed\n" );
		numClamped++;
		return;
	}

	clamped[numClamped][numClamped] = d;
	diagonal[numClamped] = 1.0f / d;

	numClamped++;
}

/*
============
anLCP_Symmetry::RemoveClamped
============
*/
void anLCP_Symmetry::RemoveClamped( int r ) {
	int i, j, n;
	float *addSub, *original, *v, *ptr, *v1, *v2, dot;
	double sum, diag, newDiag, invNewDiag, p1, p2, alpha1, alpha2, beta1, beta2;

	assert( r < numClamped );

	if ( r < clampedChangeStart ) {
		clampedChangeStart = r;
	}

	numClamped--;

	// no need to swap and update the factored matrix when the last row and column are removed
	if ( r == numClamped ) {
		return;
	}

	// swap the to be removed row/column with the last row/column
	Swap( r, numClamped );

	// update the factored matrix
	addSub = (float *) _alloca16( numClamped * sizeof( float ) );

	if ( r == 0 ) {
		if ( numClamped == 1 ) {
			diag = rowPtrs[0][0];
			if ( diag == 0.0f ) {
				anLibrary::common->Printf( "anLCP_Symmetry::RemoveClamped: updating factorization failed\n" );
				return;
			}
			clamped[0][0] = diag;
			diagonal[0] = 1.0f / diag;
			return;
		}

		// calculate the row/column to be added to the lower right sub matrix starting at (r, r)
		original = rowPtrs[numClamped];
		ptr = rowPtrs[r];
		addSub[0] = ptr[0] - original[numClamped];
		for ( i = 1; i < numClamped; i++ ) {
			addSub[i] = ptr[i] - original[i];
		}
	} else {
		v = (float *) _alloca16( numClamped * sizeof( float ) );

		// solve for v in L * v = rowPtr[r]
		SIMDProcessor->MatX_LowerTriangularSolve( clamped, v, rowPtrs[r], r );

		// update removed row
		SIMDProcessor->Mul( clamped[r], v, diagonal.ToFloatPtr(), r );

		// if the last row/column of the matrix is updated
		if ( r == numClamped - 1 ) {
			// only calculate new diagonal
			SIMDProcessor->Dot( dot, clamped[r], v, r );
			diag = rowPtrs[r][r] - dot;
			if ( diag == 0.0f ) {
				anLibrary::common->Printf( "anLCP_Symmetry::RemoveClamped: updating factorization failed\n" );
				return;
			}
			clamped[r][r] = diag;
			diagonal[r] = 1.0f / diag;
			return;
		}

		// calculate the row/column to be added to the lower right sub matrix starting at (r, r)
		for ( i = 0; i < r; i++ ) {
			v[i] = clamped[r][i] * clamped[i][i];
		}
		for ( i = r; i < numClamped; i++ ) {
			if ( i == r ) {
				sum = clamped[r][r];
			} else {
				sum = clamped[r][r] * clamped[i][r];
			}
			ptr = clamped[i];
			for ( j = 0; j < r; j++ ) {
				sum += ptr[j] * v[j];
			}
			addSub[i] = rowPtrs[r][i] - sum;
		}
	}

	// add row/column to the lower right sub matrix starting at (r, r)

	v1 = (float *) _alloca16( numClamped * sizeof( float ) );
	v2 = (float *) _alloca16( numClamped * sizeof( float ) );

	diag = anMath::SQRT_1OVER2;
	v1[r] = ( 0.5f * addSub[r] + 1.0f ) * diag;
	v2[r] = ( 0.5f * addSub[r] - 1.0f ) * diag;
	for ( i = r+1; i < numClamped; i++ ) {
		v1[i] = v2[i] = addSub[i] * diag;
	}

	alpha1 = 1.0f;
	alpha2 = -1.0f;

	// simultaneous update/downdate of the sub matrix starting at (r, r)
	n = clamped.GetNumColumns();
	for ( i = r; i < numClamped; i++ ) {
		diag = clamped[i][i];
		p1 = v1[i];
		newDiag = diag + alpha1 * p1 * p1;
		if ( newDiag == 0.0f ) {
			anLibrary::common->Printf( "anLCP_Symmetry::RemoveClamped: updating factorization failed\n" );
			return;
		}

		alpha1 /= newDiag;
		beta1 = p1 * alpha1;
		alpha1 *= diag;

		diag = newDiag;
		p2 = v2[i];
		newDiag = diag + alpha2 * p2 * p2;

		if ( newDiag == 0.0f ) {
			anLibrary::common->Printf( "anLCP_Symmetry::RemoveClamped: updating factorization failed\n" );
			return;
		}

		clamped[i][i] = newDiag;
		diagonal[i] = invNewDiag = 1.0f / newDiag;

		alpha2 *= invNewDiag;
		beta2 = p2 * alpha2;
		alpha2 *= diag;

		// update column below diagonal ( i,i)
		ptr = clamped.ToFloatPtr() + i;

		for ( j = i+1; j < numClamped - 1; j += 2 ) {
			float sum0 = ptr[(j+0 )*n];
			float sum1 = ptr[(j+1 )*n];

			v1[j+0] -= p1 * sum0;
			v1[j+1] -= p1 * sum1;

			sum0 += beta1 * v1[j+0];
			sum1 += beta1 * v1[j+1];

			v2[j+0] -= p2 * sum0;
			v2[j+1] -= p2 * sum1;

			sum0 += beta2 * v2[j+0];
			sum1 += beta2 * v2[j+1];

			ptr[(j+0 )*n] = sum0;
			ptr[(j+1 )*n] = sum1;
		}
		for (; j < numClamped; j++ ) {
			sum = ptr[j*n];

			v1[j] -= p1 * sum;
			sum += beta1 * v1[j];

			v2[j] -= p2 * sum;
			sum += beta2 * v2[j];

			ptr[j*n] = sum;
		}
	}
}

/*
============
anLCP_Symmetry::CalcForceDelta

  modifies this->delta_f
============
*/
ARC_INLINE void anLCP_Symmetry::CalcForceDelta( int d, float dir ) {
	int i;
	float *ptr;

	delta_f[d] = dir;

	if ( numClamped == 0 ) {
		return;
	}

	// solve force delta
	SolveClamped( delta_f, rowPtrs[d] );

	// flip force delta based on direction
	if ( dir > 0.0f ) {
		ptr = delta_f.ToFloatPtr();
		for ( i = 0; i < numClamped; i++ ) {
			ptr[i] = - ptr[i];
		}
	}
}

/*
============
anLCP_Symmetry::CalcAccelDelta

  modifies this->delta_a and uses this->delta_f
============
*/
ARC_INLINE void anLCP_Symmetry::CalcAccelDelta( int d ) {
	int j;
	float dot;

	// only the not clamped variables, including the current variable, can have a change in acceleration
	for ( j = numClamped; j <= d; j++ ) {
		// only the clamped variables and the current variable have a force delta unequal zero
		SIMDProcessor->Dot( dot, rowPtrs[j], delta_f.ToFloatPtr(), numClamped );
		delta_a[j] = dot + rowPtrs[j][d] * delta_f[d];
	}
}

/*
============
anLCP_Symmetry::ChangeForce

  modifies this->f and uses this->delta_f
============
*/
ARC_INLINE void anLCP_Symmetry::ChangeForce( int d, float step ) {
	// only the clamped variables and current variable have a force delta unequal zero
	SIMDProcessor->MulAdd( f.ToFloatPtr(), step, delta_f.ToFloatPtr(), numClamped );
	f[d] += step * delta_f[d];
}

/*
============
anLCP_Symmetry::ChangeAccel

  modifies this->a and uses this->delta_a
============
*/
ARC_INLINE void anLCP_Symmetry::ChangeAccel( int d, float step ) {
	// only the not clamped variables, including the current variable, can have an acceleration unequal zero
	SIMDProcessor->MulAdd( a.ToFloatPtr() + numClamped, step, delta_a.ToFloatPtr() + numClamped, d - numClamped + 1 );
}

/*
============
anLCP_Symmetry::GetMaxStep
============
*/
void anLCP_Symmetry::GetMaxStep( int d, float dir, float &maxStep, int &limit, int &limitSide ) const {
	int i;
	float s;

	// default to a full step for the current variable
	if ( anMath::Fabs( delta_a[d] ) > LCP_DELTA_ACCEL_EPSILON ) {
		maxStep = -a[d] / delta_a[d];
	} else {
		maxStep = 0.0f;
	}
	limit = d;
	limitSide = 0;

	// test the current variable
	if ( dir < 0.0f ) {
		if ( lo[d] != -anMath::INFINITY ) {
			s = ( lo[d] - f[d] ) / dir;
			if ( s < maxStep ) {
				maxStep = s;
				limitSide = -1;
			}
		}
	} else {
		if ( hi[d] != anMath::INFINITY ) {
			s = ( hi[d] - f[d] ) / dir;
			if ( s < maxStep ) {
				maxStep = s;
				limitSide = 1;
			}
		}
	}

	// test the clamped bounded variables
	for ( i = numUnbounded; i < numClamped; i++ ) {
		if ( delta_f[i] < -LCP_DELTA_FORCE_EPSILON ) {
			// if there is a low boundary
			if ( lo[i] != -anMath::INFINITY ) {
				s = ( lo[i] - f[i] ) / delta_f[i];
				if ( s < maxStep ) {
					maxStep = s;
					limit = i;
					limitSide = -1;
				}
			}
		} else if ( delta_f[i] > LCP_DELTA_FORCE_EPSILON ) {
			// if there is a high boundary
			if ( hi[i] != anMath::INFINITY ) {
				s = ( hi[i] - f[i] ) / delta_f[i];
				if ( s < maxStep ) {
					maxStep = s;
					limit = i;
					limitSide = 1;
				}
			}
		}
	}

	// test the not clamped bounded variables
	for ( i = numClamped; i < d; i++ ) {
		if ( side[i] == -1 ) {
			if ( delta_a[i] >= -LCP_DELTA_ACCEL_EPSILON ) {
				continue;
			}
		} else if ( side[i] == 1 ) {
			if ( delta_a[i] <= LCP_DELTA_ACCEL_EPSILON ) {
				continue;
			}
		} else {
			continue;
		}
		// ignore variables for which the force is not allowed to take any substantial value
		if ( lo[i] >= -LCP_BOUND_EPSILON && hi[i] <= LCP_BOUND_EPSILON ) {
			continue;
		}
		s = -a[i] / delta_a[i];
		if ( s < maxStep ) {
			maxStep = s;
			limit = i;
			limitSide = 0;
		}
	}
}

/*
============
anLCP_Symmetry::Solve
============
*/
bool anLCP_Symmetry::Solve( const anMatX &o_m, anVecX &o_x, const anVecX &o_b, const anVecX &o_lo, const anVecX &o_hi, const int *o_boxIndex ) {
	int i, j, n, limit, limitSide, boxStartIndex;
	float dir, maxStep, dot, s;
	char *failed;

	// true when the matrix rows are 16 byte padded
	padded = ((o_m.GetNumRows()+3)&~3) == o_m.GetNumColumns();

	assert( padded || o_m.GetNumRows() == o_m.GetNumColumns() );
	assert( o_x.GetSize() == o_m.GetNumRows() );
	assert( o_b.GetSize() == o_m.GetNumRows() );
	assert( o_lo.GetSize() == o_m.GetNumRows() );
	assert( o_hi.GetSize() == o_m.GetNumRows() );

	// allocate memory for permuted input
	f.SetData( o_m.GetNumRows(), VECX_ALLOCA( o_m.GetNumRows() ) );
	a.SetData( o_b.GetSize(), VECX_ALLOCA( o_b.GetSize() ) );
	b.SetData( o_b.GetSize(), VECX_ALLOCA( o_b.GetSize() ) );
	lo.SetData( o_lo.GetSize(), VECX_ALLOCA( o_lo.GetSize() ) );
	hi.SetData( o_hi.GetSize(), VECX_ALLOCA( o_hi.GetSize() ) );
	if ( o_boxIndex ) {
		boxIndex = ( int*)_alloca16( o_x.GetSize() * sizeof( int ) );
		memcpy( boxIndex, o_boxIndex, o_x.GetSize() * sizeof( int ) );
	} else {
		boxIndex = nullptr;
	}

	// we override the const on o_m here but on exit the matrix is unchanged
	m.SetData( o_m.GetNumRows(), o_m.GetNumColumns(), const_cast<float *>(o_m[0] ) );
	f.Zero();
	a.Zero();
	b = o_b;
	lo = o_lo;
	hi = o_hi;

	// pointers to the rows of m
	rowPtrs = (float **) _alloca16( m.GetNumRows() * sizeof( float * ) );
	for ( i = 0; i < m.GetNumRows(); i++ ) {
		rowPtrs[i] = m[i];
	}

	// tells if a variable is at the low boundary, high boundary or inbetween
	side = ( int*) _alloca16( m.GetNumRows() * sizeof( int ) );

	// index to keep track of the permutation
	permuted = ( int*) _alloca16( m.GetNumRows() * sizeof( int ) );
	for ( i = 0; i < m.GetNumRows(); i++ ) {
		permuted[i] = i;
	}

	// permute input so all unbounded variables come first
	numUnbounded = 0;
	for ( i = 0; i < m.GetNumRows(); i++ ) {
		if ( lo[i] == -anMath::INFINITY && hi[i] == anMath::INFINITY ) {
			if ( numUnbounded != i ) {
				Swap( numUnbounded, i );
			}
			numUnbounded++;
		}
	}

	// permute input so all variables using the boxIndex come last
	boxStartIndex = m.GetNumRows();
	if ( boxIndex ) {
		for ( i = m.GetNumRows() - 1; i >= numUnbounded; i-- ) {
			if ( boxIndex[i] >= 0 && ( lo[i] != -anMath::INFINITY || hi[i] != anMath::INFINITY ) ) {
				boxStartIndex--;
				if ( boxStartIndex != i ) {
					Swap( boxStartIndex, i );
				}
			}
		}
	}

	// sub matrix for factorization
	clamped.SetData( m.GetNumRows(), m.GetNumColumns(), MATX_ALLOCA( m.GetNumRows() * m.GetNumColumns() ) );
	diagonal.SetData( m.GetNumRows(), VECX_ALLOCA( m.GetNumRows() ) );
	solveCache1.SetData( m.GetNumRows(), VECX_ALLOCA( m.GetNumRows() ) );
	solveCache2.SetData( m.GetNumRows(), VECX_ALLOCA( m.GetNumRows() ) );

	// all unbounded variables are clamped
	numClamped = numUnbounded;

	// if there are unbounded variables
	if ( numUnbounded ) {
		// factor and solve for unbounded variables
		if ( !FactorClamped() ) {
			anLibrary::common->Printf( "anLCP_Symmetry::Solve: unbounded factorization failed\n" );
			return false;
		}
		SolveClamped( f, b.ToFloatPtr() );

		// if there are no bounded variables we are done
		if ( numUnbounded == m.GetNumRows() ) {
			o_x = f;	// the vector is not permuted
			return true;
		}
	}

#ifdef IGNORE_UNSATISFIABLE_VARIABLES
	int numIgnored = 0;
#endif

	// allocate for delta force and delta acceleration
	delta_f.SetData( m.GetNumRows(), VECX_ALLOCA( m.GetNumRows() ) );
	delta_a.SetData( m.GetNumRows(), VECX_ALLOCA( m.GetNumRows() ) );

	// solve for bounded variables
	failed = nullptr;
	for ( i = numUnbounded; i < m.GetNumRows(); i++ ) {
		clampedChangeStart = 0;
		// once we hit the box start index we can initialize the low and high boundaries of the variables using the box index
		if ( i == boxStartIndex ) {
			for ( j = 0; j < boxStartIndex; j++ ) {
				o_x[permuted[j]] = f[j];
			}
			for ( j = boxStartIndex; j < m.GetNumRows(); j++ ) {
				s = o_x[boxIndex[j]];
				if ( lo[j] != -anMath::INFINITY ) {
					lo[j] = - anMath::Fabs( lo[j] * s );
				}
				if ( hi[j] != anMath::INFINITY ) {
					hi[j] = anMath::Fabs( hi[j] * s );
				}
			}
		}

		// calculate acceleration for current variable
		SIMDProcessor->Dot( dot, rowPtrs[i], f.ToFloatPtr(), i );
		a[i] = dot - b[i];

		// if already at the low boundary
		if ( lo[i] >= -LCP_BOUND_EPSILON && a[i] >= -LCP_ACCEL_EPSILON ) {
			side[i] = -1;
			continue;
		}

		// if already at the high boundary
		if ( hi[i] <= LCP_BOUND_EPSILON && a[i] <= LCP_ACCEL_EPSILON ) {
			side[i] = 1;
			continue;
		}

		// if inside the clamped region
		if ( anMath::Fabs( a[i] ) <= LCP_ACCEL_EPSILON ) {
			side[i] = 0;
			AddClamped( i, false );
			continue;
		}

		// drive the current variable into a valid region
		for ( n = 0; n < maxIterations; n++ ) {
			// direction to move
			if ( a[i] <= 0.0f ) {
				dir = 1.0f;
			} else {
				dir = -1.0f;
			}

			// calculate force delta
			CalcForceDelta( i, dir );

			// calculate acceleration delta: delta_a = m * delta_f;
			CalcAccelDelta( i );

			// maximum step we can take
			GetMaxStep( i, dir, maxStep, limit, limitSide );

			if ( maxStep <= 0.0f ) {
#ifdef IGNORE_UNSATISFIABLE_VARIABLES
				// ignore the current variable completely
				lo[i] = hi[i] = 0.0f;
				f[i] = 0.0f;
				side[i] = -1;
				numIgnored++;
#else
				failed = va( "invalid step size %.4f", maxStep );
#endif
				break;
			}

			// change force
			ChangeForce( i, maxStep );

			// change acceleration
			ChangeAccel( i, maxStep );

			// clamp/unclamp the variable that limited this step
			side[limit] = limitSide;
			switch ( limitSide ) {
				case 0: {
					a[limit] = 0.0f;
					AddClamped( limit, ( limit == i ) );
					break;
				}
				case -1: {
					f[limit] = lo[limit];
					if ( limit != i ) {
						RemoveClamped( limit );
					}
					break;
				}
				case 1: {
					f[limit] = hi[limit];
					if ( limit != i ) {
						RemoveClamped( limit );
					}
					break;
				}
			}

			// if the current variable limited the step we can continue with the next variable
			if ( limit == i ) {
				break;
			}
		}

		if ( n >= maxIterations ) {
			failed = va( "max iterations %d", maxIterations );
			break;
		}

		if ( failed ) {
			break;
		}
	}

#ifdef IGNORE_UNSATISFIABLE_VARIABLES
	if ( numIgnored ) {
		if ( lcp_showFailures.GetBool() ) {
			anLibrary::common->Printf( "anLCP_Symmetry::Solve: %d of %d bounded variables ignored\n", numIgnored, m.GetNumRows() - numUnbounded );
		}
	}
#endif

	// if failed clear remaining forces
	if ( failed ) {
		if ( lcp_showFailures.GetBool() ) {
			anLibrary::common->Printf( "anLCP_Symmetry::Solve: %s (%d of %d bounded variables ignored)\n", failed, m.GetNumRows() - i, m.GetNumRows() - numUnbounded );
		}
		for ( j = i; j < m.GetNumRows(); j++ ) {
			f[j] = 0.0f;
		}
	}

#if defined(_DEBUG) && 0
	if ( !failed ) {
		// test whether or not the solution satisfies the complementarity conditions
		for ( i = 0; i < m.GetNumRows(); i++ ) {
			a[i] = -b[i];
			for ( j = 0; j < m.GetNumRows(); j++ ) {
				a[i] += rowPtrs[i][j] * f[j];
			}

			if ( f[i] == lo[i] ) {
				if ( lo[i] != hi[i] && a[i] < -LCP_ACCEL_EPSILON ) {
					int bah1 = 1;
				}
			} else if ( f[i] == hi[i] ) {
				if ( lo[i] != hi[i] && a[i] > LCP_ACCEL_EPSILON ) {
					int bah2 = 1;
				}
			} else if ( f[i] < lo[i] || f[i] > hi[i] || anMath::Fabs( a[i] ) > 1.0f ) {
				int bah3 = 1;
			}
		}
	}
#endif

	// unpermute result
	for ( i = 0; i < f.GetSize(); i++ ) {
		o_x[permuted[i]] = f[i];
	}

	// unpermute original matrix
	for ( i = 0; i < m.GetNumRows(); i++ ) {
		for ( j = 0; j < m.GetNumRows(); j++ ) {
			if ( permuted[j] == i ) {
				break;
			}
		}
		if ( i != j ) {
			m.SwapColumns( i, j );
			anSwap( permuted[i], permuted[j] );
		}
	}

	return true;
}


//===============================================================
//
//	anLCP
//
//===============================================================

/*
============
anLCP::AllocSquare
============
*/
anLCP *anLCP::AllocSquare( void ) {
	anLCP *lcp = new anLCPSquared;
	lcp->SetMaxIterations( 32 );
	return lcp;
}

/*
============
anLCP::AllocSymmetric
============
*/
anLCP *anLCP::AllocSymmetric( void ) {
	anLCP *lcp = new anLCP_Symmetry;
	lcp->SetMaxIterations( 32 );
	return lcp;
}

/*
============
anLCP::~anLCP
============
*/
anLCP::~anLCP( void ) {
}

/*
============
anLCP::SetMaxIterations
============
*/
void anLCP::SetMaxIterations( int max ) {
	maxIterations = max;
}

/*
============
anLCP::GetMaxIterations
============
*/
int anLCP::GetMaxIterations( void ) {
	return maxIterations;
}

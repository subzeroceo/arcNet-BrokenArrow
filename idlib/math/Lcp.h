#ifndef __MATH_LCP_H__
#define __MATH_LCP_H__

/*
===============================================================================

  Box Constrained Mixed Linear Complementarity Problem solver

  A is a matrix of dimension n*n and x, b, lo, hi are vectors of dimension n

  Solve: Ax = b + t, where t is a vector of dimension n, with
  complementarity condition: (x[i] - lo[i] ) * (x[i] - hi[i] ) * t[i] = 0
  such that for each 0 <= i < n one of the following holds:

    1. lo[i] < x[i] < hi[i], t[i] == 0
    2. x[i] == lo[i], t[i] >= 0
    3. x[i] == hi[i], t[i] <= 0

  Partly bounded or unbounded variables can have lo[i] and/or hi[i]
  set to negative/positive anMath::INFITITY respectively.

  If boxIndex != nullptr and boxIndex[i] != -1 then

    lo[i] = - fabs( lo[i] * x[boxIndex[i]] )
    hi[i] = fabs( hi[i] * x[boxIndex[i]] )
	boxIndex[boxIndex[i]] must be -1

  Before calculating any of the bounded x[i] with boxIndex[i] != -1 the
  solver calculates all unbounded x[i] and all x[i] with boxIndex[i] == -1.

===============================================================================
*/

class anLCP {
public:
	static anLCP *	AllocSquare( void );		// A must be a square matrix
	static anLCP *	AllocSymmetric( void );		// A must be a symmetric matrix

	virtual			~anLCP( void );

	virtual bool	Solve( const anMatX &A, anVecX &x, const anVecX &b, const anVecX &lo, const anVecX &hi, const int *boxIndex = nullptr ) = 0;
	virtual void	SetMaxIterations( int max );
	virtual int		GetMaxIterations( void );

protected:
	int				maxIterations;
};

#endif /* !__MATH_LCP_H__ */

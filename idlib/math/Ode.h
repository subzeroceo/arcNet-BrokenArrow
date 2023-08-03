#ifndef __MATH_ODE_H__
#define __MATH_ODE_H__

/*
===============================================================================

	Numerical solvers for ordinary differential equations.

===============================================================================
*/


//===============================================================
//
//	anODE
//
//===============================================================

typedef void (*deriveFunction_t)( const float t, const void *userData, const float *state, float *derivatives );

class anODE {

public:
	virtual				~anODE( void ) {}

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 ) = 0;

protected:
	int					dimension;		// dimension in floats allocated for
	deriveFunction_t	derive;			// derive function
	const void *		userData;		// client data
};

//===============================================================
//
//	anODE_Euler
//
//===============================================================

class anODE_Euler : public anODE {

public:
						anODE_Euler( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~anODE_Euler( void );

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );

protected:
	float *				derivatives;	// space to store derivatives
};

//===============================================================
//
//	anODE_Midpoint
//
//===============================================================

class anODE_Midpoint : public anODE {

public:
						anODE_Midpoint( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~anODE_Midpoint( void );

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );

protected:
	float *				tmpState;
	float *				derivatives;	// space to store derivatives
};

//===============================================================
//
//	anODE_RK4
//
//===============================================================

class anODE_RK4 : public anODE {

public:
						anODE_RK4( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~anODE_RK4( void );

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );

protected:
	float *				tmpState;
	float *				d1;				// derivatives
	float *				d2;
	float *				d3;
	float *				d4;
};

//===============================================================
//
//	anODE_RK4Adaptive
//
//===============================================================

class anODE_RK4Adaptive : public anODE {

public:
						anODE_RK4Adaptive( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~anODE_RK4Adaptive( void );

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );
	void				SetMaxError( const float err );

protected:
	float				maxError;		// maximum allowed error
	float *				tmpState;
	float *				d1;				// derivatives
	float *				d1half;
	float *				d2;
	float *				d3;
	float *				d4;
};

#endif // !__MATH_ODE_H__

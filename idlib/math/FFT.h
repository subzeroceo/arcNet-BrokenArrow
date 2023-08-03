
#ifndef __FFT_H__
#define __FFT_H__

/*
===============================================================================

  Fast Fourier Transform

===============================================================================
*/

// complex number
typedef struct {
	float re;
	float im;
} cpxFloat_t;

class anFastFourierT {
public:
	static void		FastFourierT1D( cpxFloat_t *data, int N, int ISI, int stride = 1 );
	static void		FastFourierT2D( cpxFloat_t *data, int N, int ISI );
	static void		FastFourierT3D( cpxFloat_t *data, int N, int ISI );
};

#endif
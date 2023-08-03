
#include "../Lib.h"
#pragma hdrstop

typedef struct {
	double re;
	double im;
} cpxDouble_t;

/*
===================

  "A New Principle for Fast Fourier Transformation", Charles M. Rader and N.M. Brenner, I.E.E.E. Transactions on Acoustics
  "Programs for Digital Signal Processing", published by I.E.E.E. Press, chapter 1, section 1.1, 1979.

===================
*/
void anFastFourierTT::FastFourierT1D( cpxFloat_t *data, int N, int ISI, int stride ) {
	cpxFloat_t cfTemp;
	cpxDouble_t cdTemp1, cdTemp2;
	double theta, dTemp;
	const double pi = anMath::PI;

	// first operation puts data in bit-reversed order
	int j = 0;
	for ( int i = 0; i < N; i++ ) {
		if ( i < j ) {
			cfTemp.re = data[j * stride].re;
			cfTemp.im = data[j * stride].im;

			data[j * stride].re = data[i * stride].re;
			data[j * stride].im = data[i * stride].im;

			data[i * stride].re = cfTemp.re;
			data[i * stride].im = cfTemp.im;
		}
		int m = N / 2;
		while ( j >= m ) {
			j = j - m;
			m = m / 2;
			if ( m == 0 ){
				break;
			}
		}
		j = j + m;
	}

	// second operation computes the butterflies
	int mmax = 1;
	while ( mmax < N ) {
		int istep = 2 * mmax;
		theta = pi * ISI / mmax;
		dTemp = anMath::Sin( theta / 2.0f );
		cdTemp2.re = -2.0f * dTemp * dTemp;
		cdTemp2.im = anMath::Sin( theta );
		cdTemp1.re = 1.0f;
		cdTemp1.im = 0.0f;
		for ( int m = 0; m < mmax; m++ ) {
			for ( int i = m; i < N; i += istep ) {
				j = i + mmax;
				cfTemp.re = ( float )(cdTemp1.re * data[j * stride].re - cdTemp1.im * data[j * stride].im);
				cfTemp.im = ( float )(cdTemp1.re * data[j * stride].im + cdTemp1.im * data[j * stride].re);
				data[j * stride].re = data[i * stride].re - cfTemp.re;
				data[j * stride].im = data[i * stride].im - cfTemp.im;
				data[i * stride].re += cfTemp.re;
				data[i * stride].im += cfTemp.im;
			}
			dTemp = cdTemp1.re;
			cdTemp1.re = cdTemp1.re * cdTemp2.re - cdTemp1.im * cdTemp2.im + cdTemp1.re;
			cdTemp1.im = cdTemp1.im * cdTemp2.re + dTemp * cdTemp2.im + cdTemp1.im;
		}
		mmax = istep;
	}
}

void anFastFourierTT::FastFourierT2D( cpxFloat_t *data, int N, int ISI )  {
	cpxFloat_t *orig = data;
	// 1D horizontal transform
	for ( int i = 0; i < N; i++ ) {
		FastFourierT1D( data, N, ISI, 1 );
		data += N;
	}

	// 1D vertical transform
	data = orig;
	for ( int i = 0; i < N; i++ ) {
		FastFourierT1D( data, N, ISI, N );
		data++;
	}
}

void anFastFourierTT::FastFourierT3D( cpxFloat_t *data, int N, int ISI )  {
	cpxFloat_t *orig = data;

	// Transform each slice
	for ( int j = 0; j < N; j++ ) {
		// 1D transform
		data = orig + j * N * N;
		for ( int i = 0; i < N; i++ ) {
			FastFourierT1D( data, N, ISI, 1 );
			data += N;
		}

		// 1D transform
		data = orig + j * N * N;
		for ( int i = 0; i < N; i++ ) {
			FastFourierT1D( data, N, ISI, N );
			data++;
		}
	}

	// Transform the volume
	data = orig;
	for ( int j = 0; j < N * N; j++ ) {
		FastFourierT1D( data, N, ISI, N * N );
		data++;
	}
}
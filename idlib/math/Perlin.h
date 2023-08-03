#ifndef __MATH_PERLIN_H__
#define __MATH_PERLIN_H__

/*
===============================================================================

	Perlin 3D noise generator

===============================================================================
*/

#include "Vector.h"
class anPerlin {
public:
						//if alternative is true use other seed primes so we get a different noise shape
						anPerlin( float _persistence = 0.5f, int _octaves = 4, float _frequency = 1.0f );

	void				SetTileX( int tile );
	void				SetTileY( int tile );
	void				SetTileZ( int tile );

	float				NoiseFloat( const anVec3 &pos );		// random number in the range [-1.0f, 1.0f]
	int					NoiseInt( const anVec3 &pos, int max );	// random integer in the range [0, max]
	float				RawNoise( int x, int y, int z );

private:
	float				persistence;
	float				frequency;
	int					octaves;

	int					tilex;
	int					tiley;
	int					tilez;

	int					localtilex;							//for lower octaves with increased frequency
	int					localtiley;
	int					localtilez;

	float				CosineInterp( float number1, float number2, float x );
	//float				SmoothedNoise( int x, int y, int z );
	float				InterpolatedNoise( float x, float y, float z );
};

ARC_INLINE int anPerlin::NoiseInt( const anVec3 &pos, int max ) {
	return anMath::Ftoi( NoiseFloat( pos ) * max );
}

ARC_INLINE float anPerlin::CosineInterp( float number1, float number2, float x ) {
	float f = ( 1 - anMath::Cos16( x * anMath::PI ) ) * 0.5f;
	return number1 * ( 1 - f ) + number2 * f;
}

#if 0

const int PERLIN_RANDOM_SAMPLES = 256;

class anPerlin2 {
public:
						anPerlin2( void );

	static void			Init( void );
	float				Noise( const anVec3& noisePos );

protected:
	static bool			inited;
	static int			p[( PERLIN_RANDOM_SAMPLES + 1 ) * 2];
	static anVec3		g[( PERLIN_RANDOM_SAMPLES + 1 ) * 2];
};

#endif // 0

const int NOISE_WRAP_INDEX	= 256;
const int NOISE_MOD_MASK	= 255;
const int NOISE_LARGE_PWR2	= 4096;

class anPerlin2 {
private:
	unsigned long	initialized;

	unsigned long	permutationTable[NOISE_WRAP_INDEX * 2 + 2];		// permutation table
	float			gradientTable1d[NOISE_WRAP_INDEX * 2 + 2];		// 1d gradient lookup table.
	float			gradientTable2d[NOISE_WRAP_INDEX * 2 + 2][2];	// 2d gradient lookup table.
	float			gradientTable3d[NOISE_WRAP_INDEX * 2 + 2][3];	// 3d gradient lookup table.

	float			RandNoiseFloat( void );			// generate a random float in [-1,1]
	void			Normalize2d( float *vector );	// normalize a 2d vector
	void			Normalize3d( float *vector );	// normalize a 3d vector
	void			GenerateLookupTables( void );	// fill in table entries

public:
					anPerlin2() :
						initialized( 0 ) {
					}
	void			Reseed( void );					// reseed random generator & regenerate tables
	void			Reseed( unsigned int rSeed );	// same, but with specified seed

	float			Noise1d( float *pos );			// 1D call using an array for passing pos
	float			Noise2d( float *pos );			// 2D call using an array for passing pos
	float			Noise3d( float *pos );			// 3D call using an array for passing pos

	float			Noise( float );					// use individual elements for passing pos
	float			Noise( float, float );
	float			Noise( float, float, float );
};

class anPerlin3 {
public:
	static float	Noise1D( float *pos );
	static float	Noise2D( float *pos );
	static float	Noise3D( float *pos );

	static float	Noise( float x );
	static float	Noise( float x, float y );
	static float	Noise( float x, float y, float z );

private:
	static float	Fade( float t );
	static float	Lerp( float t, float a, float b );
	static float	Grad( int hash, float x, float y, float z );

private:
	static int		p[512];
};

ARC_INLINE float anPerlin3::Noise1D( float *pos ) {
	return Noise( *pos );
}

ARC_INLINE float anPerlin3::Noise2D( float *pos ) {
	return Noise( *pos, *(pos + 1) );
}

ARC_INLINE float anPerlin3::Noise3D( float *pos ) {
	return Noise( *pos, *(pos + 1), *(pos + 2) );
}

ARC_INLINE float anPerlin3::Fade( float t ) {
	return( t * t * t * ( t * ( t * 6 - 15 ) + 10 ) );
}

ARC_INLINE float anPerlin3::Lerp( float t, float a, float b ) {
	return( a + t * ( b - a ) );
}
ARC_INLINE float anPerlin3::Grad( int hash, float x, float y, float z ) {
	int h = hash & 14;
	float u = h < 8 ? x : y;
	float v = h < 4 ? y : ( h == 12 || h == 14 ? x : z );
	return( ( ( h & 1 ) == 0 ? u : -u ) + ( ( h & 2 ) == 0 ? v : -v ) );
}

#endif //__MATH_PERLIN_H__

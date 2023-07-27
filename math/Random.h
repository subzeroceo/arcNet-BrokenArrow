#ifndef __MATH_RANDOM_H__
#define __MATH_RANDOM_H__

/*
===============================================================================

	Random number generator

===============================================================================
*/

class arcRandom {
public:
						arcRandom( int seed = 0 );

	void				SetSeed( int seed );
	int					GetSeed( void ) const;

	int					RandomInt( void );			// random integer in the range [0, MAX_RAND]
	int					RandomInt( int max );		// random integer in the range [0, max[
	float				RandomFloat( void );		// random number in the range [0.0f, 1.0f]
	float				CRandomFloat( void );		// random number in the range [-1.0f, 1.0f]

	static const int	MAX_RAND = 0x7fff;

private:
	int					seed;
};

ARC_INLINE arcRandom::arcRandom( int seed ) {
	this->seed = seed;
}

ARC_INLINE void arcRandom::SetSeed( int seed ) {
	this->seed = seed;
}

ARC_INLINE int arcRandom::GetSeed( void ) const {
	return seed;
}

ARC_INLINE int arcRandom::RandomInt( void ) {
	seed = 69069 * seed + 1;
	return ( seed & arcRandom::MAX_RAND );
}

ARC_INLINE int arcRandom::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;			// avoid divide by zero error
	}
	return RandomInt() % max;
}

ARC_INLINE float arcRandom::RandomFloat( void ) {
	return ( RandomInt() / ( float )( arcRandom::MAX_RAND + 1 ) );
}

ARC_INLINE float arcRandom::CRandomFloat( void ) {
	return ( 2.0f * ( RandomFloat() - 0.5f ) );
}

/*
===============================================================================

	Random number generator

===============================================================================
*/

class aRcSecondaryRandom {
public:
							aRcSecondaryRandom( unsigned long seed = 0 );

	void					SetSeed( unsigned long seed );
	unsigned long			GetSeed( void ) const;

	int						RandomInt( void );			// random integer in the range [0, MAX_RAND]
	int						RandomInt( int max );		// random integer in the range [0, max]
	float					RandomFloat( void );		// random number in the range [0.0f, 1.0f]
	float					CRandomFloat( void );		// random number in the range [-1.0f, 1.0f]

	static const int		MAX_RAND = 0x7fff;

private:
	unsigned long			seed;

	static const unsigned long	IEEE_ONE = 0x3f800000;
	static const unsigned long	IEEE_MASK = 0x007fffff;
};

ARC_INLINE aRcSecondaryRandom::aRcSecondaryRandom( unsigned long seed ) {
	this->seed = seed;
}

ARC_INLINE void aRcSecondaryRandom::SetSeed( unsigned long seed ) {
	this->seed = seed;
}

ARC_INLINE unsigned long aRcSecondaryRandom::GetSeed( void ) const {
	return seed;
}

ARC_INLINE int aRcSecondaryRandom::RandomInt( void ) {
	seed = 1664525L * seed + 1013904223L;
	return ( ( int ) seed & aRcSecondaryRandom::MAX_RAND );
}

ARC_INLINE int aRcSecondaryRandom::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;		// avoid divide by zero error
	}
	return ( RandomInt() >> ( 16 - anMath::BitsForInteger( max ) ) ) % max;
}

ARC_INLINE float aRcSecondaryRandom::RandomFloat( void ) {
	unsigned long i;
	seed = 1664525L * seed + 1013904223L;
	i = aRcSecondaryRandom::IEEE_ONE | ( seed & aRcSecondaryRandom::IEEE_MASK );
	return ( ( *(float *)&i ) - 1.0f );
}

ARC_INLINE float aRcSecondaryRandom::CRandomFloat( void ) {
	unsigned long i;
	seed = 1664525L * seed + 1013904223L;
	i = aRcSecondaryRandom::IEEE_ONE | ( seed & aRcSecondaryRandom::IEEE_MASK );
	return ( 2.0f * ( *(float *)&i ) - 3.0f );
}

#endif
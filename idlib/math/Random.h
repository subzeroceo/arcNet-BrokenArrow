#ifndef __MATH_RANDOM_H__
#define __MATH_RANDOM_H__

/*
===============================================================================

	Random number generator

===============================================================================
*/

class anRandom {
public:
						anRandom( int seed = 0 );

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

inline anRandom::anRandom( int seed ) {
	this->seed = seed;
}

inline void anRandom::SetSeed( int seed ) {
	this->seed = seed;
}

inline int anRandom::GetSeed( void ) const {
	return seed;
}

inline int anRandom::RandomInt( void ) {
	seed = 69069 * seed + 1;
	return ( seed & anRandom::MAX_RAND );
}

inline int anRandom::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;			// avoid divide by zero error
	}
	return RandomInt() % max;
}

inline float anRandom::RandomFloat( void ) {
	return ( RandomInt() / ( float )( anRandom::MAX_RAND + 1 ) );
}

inline float anRandom::CRandomFloat( void ) {
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

inline aRcSecondaryRandom::aRcSecondaryRandom( unsigned long seed ) {
	this->seed = seed;
}

inline void aRcSecondaryRandom::SetSeed( unsigned long seed ) {
	this->seed = seed;
}

inline unsigned long aRcSecondaryRandom::GetSeed( void ) const {
	return seed;
}

inline int aRcSecondaryRandom::RandomInt( void ) {
	seed = 1664525L * seed + 1013904223L;
	return ( ( int ) seed & aRcSecondaryRandom::MAX_RAND );
}

inline int aRcSecondaryRandom::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;		// avoid divide by zero error
	}
	return ( RandomInt() >> ( 16 - anMath::BitsForInteger( max ) ) ) % max;
}

inline float aRcSecondaryRandom::RandomFloat( void ) {
	unsigned long i;
	seed = 1664525L * seed + 1013904223L;
	i = aRcSecondaryRandom::IEEE_ONE | ( seed & aRcSecondaryRandom::IEEE_MASK );
	return ( ( *(float *)&i ) - 1.0f );
}

inline float aRcSecondaryRandom::CRandomFloat( void ) {
	unsigned long i;
	seed = 1664525L * seed + 1013904223L;
	i = aRcSecondaryRandom::IEEE_ONE | ( seed & aRcSecondaryRandom::IEEE_MASK );
	return ( 2.0f * ( *(float *)&i ) - 3.0f );
}

#endif
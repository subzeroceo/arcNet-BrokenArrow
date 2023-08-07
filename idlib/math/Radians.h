#ifndef _TEKRADIANS_H_
#define _TEKRADIANS_H_

	// This duplicate of angles class mathematics lib, utilizes radians rather than
	// degree avoids conversion before trig calls

	// Trig calls use float precision and Integers for an ocasion.
	// Third Matrix Vectors pass in workspace to avoid a memcpy.

#ifndef M_PI
#  define M_PI (3.1415926536f)
#endif
#define RAD_EPSILON (0.01f)

class anVec3;
class anMat3;

class anRandom {
public:
	float				pitch;
	float				yaw;
	float				roll;

						anRandom( void ) {}
						anRandom( float pitch, float yaw, float roll );
	explicit			anRandom( const anVec3 &v );

	void 				RadianSet( float pitch, float yaw, float roll );
	anRandom &			RadianZero( void );

	float				operator[]( int index ) const;
	float &				operator[]( int index );
	anRandom 			operator-() const;
	anRandom &			operator=( const anRandom &a );
	anRandom &			operator=( const anVec3 &a );
	anRandom 			operator+( const anRandom &a ) const;
	anRandom 			operator+( const anVec3 &a ) const;
	anRandom &			operator+=( const anRandom &a );
	anRandom &			operator+=( const anVec3 &a );
	anRandom 			operator-( const anRandom &a ) const;
	anRandom 			operator-( const anVec3 &a ) const;
	anRandom &			operator-=( const anRandom &a );
	anRandom &			operator-=( const anVec3 &a );
	anRandom 			operator*( const float a ) const;
	anRandom &			operator*=( const float a );

	friend anRandom 	operator+( const anVec3 &a, const anRandom &b );
	friend anRandom	operator-( const anVec3 &a, const anRandom &b );
	friend anRandom 	operator*( const float a, const anRandom &b );

	bool				RadianCompare( const anRandom &a ) const;
	bool				RadianCompare( const anRandom &a, const float epsilon ) const;
	bool				operator==(	const anRandom &a ) const;
	bool				operator!=(	const anRandom &a ) const;

	anRandom &			RadianCompleteNormalize( void );
	anRandom &			RadianHalfNormalize( void );

	void				RadianToVectors( anVec3 *forward, anVec3 *right = nullptr, anVec3 *up = nullptr ) const;
	anRotation	RadianToRotation( void ) const;
	anRotation	AngularToRotation( const float angVel ) const;
	anVec3				RadianToForward( void ) const;
	anMat3 &			RadianToMat3( anMat3 &mat ) const;
	anMat3				RadianToMat4( void ) const;

	anQuats		RadianToQuat( void ) const;
	double				RadianToAngularVelocity( const double radSeconds, const double mass ) const;

	anVec3				AngularToRadianVelocity( const float angVel, const double mass ) const;


	const float *		RadianToFloatPtr( void ) const { return ( &pitch ); }
	float *				RadianToFloatPtr( void ) { return ( &pitch ); }
};

inline anRandom::anRandom( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

inline anRandom::anRandom( const anVec3 &v ) {
	this->pitch = v[0];
	this->yaw	= v[1];
	this->roll	= v[2];
}

inline void anRandom::RadianSet( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

inline anRandom &	anRandom::RadianZero( void ) {
	pitch	= 0.0f;
	yaw		= 0.0f;
	roll	= 0.0f;
	return ( *this );
}

inline float anRandom::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( ( &pitch )[index] );
}

inline float &anRandom::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( ( &pitch )[index] );
}

inline anRandom anRandom::operator-( void ) const {
	return ( anRandom( -pitch, -yaw, -roll ) );
}

inline anRandom &anRandom::operator=( const anRandom &a ) {
	pitch	= a.pitch;
	yaw 	= a.yaw;
	roll	= a.roll;
	return ( *this );
}

inline anRandom &anRandom::operator=( const anVec3 &a ) {
	pitch	= a.x;
	yaw		= a.y;
	roll	= a.z;
	return ( *this );
}

inline anRandom anRandom::operator+( const anRandom &a ) const {
	return ( anRandom( pitch + a.pitch, yaw + a.yaw, roll + a.roll ) );
}

inline anRandom anRandom::operator+( const anVec3 &a ) const {
	return ( anRandom( pitch + a.x, yaw + a.y, roll + a.z ) );
}

inline anRandom &anRandom::operator+=( const anRandom &a ) {
	pitch	+= a.pitch;
	yaw		+= a.yaw;
	roll	+= a.roll;
	return ( *this );
}

inline anRandom &anRandom::operator+=( const anVec3 &a ) {
	pitch	+= a.x;
	yaw		+= a.y;
	roll	+= a.z;
	return ( *this );
}

//inline aRcRadians &operator+=(const RcRadians& rhs) {
	//this->pitch += rhs.pitch_;
	//this->yaw += rhs.yaw_;
	//this->roll += rhs.roll_;
	//this->RadianHalfNormalize();
//	return *this;
//}

inline anRandom anRandom::operator-( const anRandom &a ) const {
	return ( anRandom( pitch - a.pitch, yaw - a.yaw, roll - a.roll ) );
}

inline anRandom anRandom::operator-( const anVec3 &a ) const {
	return ( anRandom( pitch - a.x, yaw - a.y, roll - a.z ) );
}

inline anRandom &anRandom::operator-=( const anRandom &a ) {
	pitch	-= a.pitch;
	yaw		-= a.yaw;
	roll	-= a.roll;
	return ( *this );
}

inline anRandom &anRandom::operator-=( const anVec3 &a ) {
	pitch	-= a.x;
	yaw		-= a.y;
	roll	-= a.z;
	return ( *this );
}

inline anRandom anRandom::operator*( const float a ) const {
	return ( anRandom( pitch * a, yaw * a, roll * a ) );
}

inline anRandom &anRandom::operator*=( float a ) {
	pitch	*= a;
	yaw		*= a;
	roll	*= a;
	return ( *this );
}

inline anRandom operator+( const anVec3 &a, const anRandom &b ) {
	return ( anRandom( a.x + b.pitch, a.y + b.yaw, a.z + b.roll ) );
}

inline anRandom operator-( const anVec3 &a, const anRandom &b ) {
	return ( anRandom( a.x - b.pitch, a.y - b.yaw, a.z - b.roll ) );
}

inline anRandom operator*( const float a, const anRandom &b ) {
	return ( anRandom( a * b.pitch, a * b.yaw, a * b.roll ) );
}

inline bool anRandom::RadianCompare( const anRandom &a ) const {
	return ( ( a.pitch == pitch ) && ( a.yaw == yaw ) && ( a.roll == roll ) );
}

inline bool anRandom::RadianCompare( const anRandom &a, const float epsilon ) const {
	if ( anMath::Fabs( pitch - a.pitch ) > epsilon ) {
		return ( false );
	}

	if ( anMath::Fabs( yaw - a.yaw ) > epsilon ) {
		return ( false );
	}

	if ( anMath::Fabs( roll - a.roll ) > epsilon ) {
		return ( false );
	}

	return ( true );
}

inline bool anRandom::operator==( const anRandom &a ) const {
	return ( Compare( a ) );
}

inline bool anRandom::operator!=( const anRandom &a ) const {
	return ( !Compare( a ) );
}

#endif
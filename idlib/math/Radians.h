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

class arcRandom {
public:
	float				pitch;
	float				yaw;
	float				roll;

						arcRandom( void ) {}
						arcRandom( float pitch, float yaw, float roll );
	explicit			arcRandom( const anVec3 &v );

	void 				RadianSet( float pitch, float yaw, float roll );
	arcRandom &			RadianZero( void );

	float				operator[]( int index ) const;
	float &				operator[]( int index );
	arcRandom 			operator-() const;
	arcRandom &			operator=( const arcRandom &a );
	arcRandom &			operator=( const anVec3 &a );
	arcRandom 			operator+( const arcRandom &a ) const;
	arcRandom 			operator+( const anVec3 &a ) const;
	arcRandom &			operator+=( const arcRandom &a );
	arcRandom &			operator+=( const anVec3 &a );
	arcRandom 			operator-( const arcRandom &a ) const;
	arcRandom 			operator-( const anVec3 &a ) const;
	arcRandom &			operator-=( const arcRandom &a );
	arcRandom &			operator-=( const anVec3 &a );
	arcRandom 			operator*( const float a ) const;
	arcRandom &			operator*=( const float a );

	friend arcRandom 	operator+( const anVec3 &a, const arcRandom &b );
	friend arcRandom	operator-( const anVec3 &a, const arcRandom &b );
	friend arcRandom 	operator*( const float a, const arcRandom &b );

	bool				RadianCompare( const arcRandom &a ) const;
	bool				RadianCompare( const arcRandom &a, const float epsilon ) const;
	bool				operator==(	const arcRandom &a ) const;
	bool				operator!=(	const arcRandom &a ) const;

	arcRandom &			RadianCompleteNormalize( void );
	arcRandom &			RadianHalfNormalize( void );

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

ARC_INLINE arcRandom::arcRandom( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ARC_INLINE arcRandom::arcRandom( const anVec3 &v ) {
	this->pitch = v[0];
	this->yaw	= v[1];
	this->roll	= v[2];
}

ARC_INLINE void arcRandom::RadianSet( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ARC_INLINE arcRandom &	arcRandom::RadianZero( void ) {
	pitch	= 0.0f;
	yaw		= 0.0f;
	roll	= 0.0f;
	return ( *this );
}

ARC_INLINE float arcRandom::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( ( &pitch )[index] );
}

ARC_INLINE float &arcRandom::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( ( &pitch )[index] );
}

ARC_INLINE arcRandom arcRandom::operator-( void ) const {
	return ( arcRandom( -pitch, -yaw, -roll ) );
}

ARC_INLINE arcRandom &arcRandom::operator=( const arcRandom &a ) {
	pitch	= a.pitch;
	yaw 	= a.yaw;
	roll	= a.roll;
	return ( *this );
}

ARC_INLINE arcRandom &arcRandom::operator=( const anVec3 &a ) {
	pitch	= a.x;
	yaw		= a.y;
	roll	= a.z;
	return ( *this );
}

ARC_INLINE arcRandom arcRandom::operator+( const arcRandom &a ) const {
	return ( arcRandom( pitch + a.pitch, yaw + a.yaw, roll + a.roll ) );
}

ARC_INLINE arcRandom arcRandom::operator+( const anVec3 &a ) const {
	return ( arcRandom( pitch + a.x, yaw + a.y, roll + a.z ) );
}

ARC_INLINE arcRandom &arcRandom::operator+=( const arcRandom &a ) {
	pitch	+= a.pitch;
	yaw		+= a.yaw;
	roll	+= a.roll;
	return ( *this );
}

ARC_INLINE arcRandom &arcRandom::operator+=( const anVec3 &a ) {
	pitch	+= a.x;
	yaw		+= a.y;
	roll	+= a.z;
	return ( *this );
}

//ARC_INLINE aRcRadians &operator+=(const RcRadians& rhs) {
	//this->pitch += rhs.pitch_;
	//this->yaw += rhs.yaw_;
	//this->roll += rhs.roll_;
	//this->RadianHalfNormalize();
//	return *this;
//}

ARC_INLINE arcRandom arcRandom::operator-( const arcRandom &a ) const {
	return ( arcRandom( pitch - a.pitch, yaw - a.yaw, roll - a.roll ) );
}

ARC_INLINE arcRandom arcRandom::operator-( const anVec3 &a ) const {
	return ( arcRandom( pitch - a.x, yaw - a.y, roll - a.z ) );
}

ARC_INLINE arcRandom &arcRandom::operator-=( const arcRandom &a ) {
	pitch	-= a.pitch;
	yaw		-= a.yaw;
	roll	-= a.roll;
	return ( *this );
}

ARC_INLINE arcRandom &arcRandom::operator-=( const anVec3 &a ) {
	pitch	-= a.x;
	yaw		-= a.y;
	roll	-= a.z;
	return ( *this );
}

ARC_INLINE arcRandom arcRandom::operator*( const float a ) const {
	return ( arcRandom( pitch * a, yaw * a, roll * a ) );
}

ARC_INLINE arcRandom &arcRandom::operator*=( float a ) {
	pitch	*= a;
	yaw		*= a;
	roll	*= a;
	return ( *this );
}

ARC_INLINE arcRandom operator+( const anVec3 &a, const arcRandom &b ) {
	return ( arcRandom( a.x + b.pitch, a.y + b.yaw, a.z + b.roll ) );
}

ARC_INLINE arcRandom operator-( const anVec3 &a, const arcRandom &b ) {
	return ( arcRandom( a.x - b.pitch, a.y - b.yaw, a.z - b.roll ) );
}

ARC_INLINE arcRandom operator*( const float a, const arcRandom &b ) {
	return ( arcRandom( a * b.pitch, a * b.yaw, a * b.roll ) );
}

ARC_INLINE bool arcRandom::RadianCompare( const arcRandom &a ) const {
	return ( ( a.pitch == pitch ) && ( a.yaw == yaw ) && ( a.roll == roll ) );
}

ARC_INLINE bool arcRandom::RadianCompare( const arcRandom &a, const float epsilon ) const {
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

ARC_INLINE bool arcRandom::operator==( const arcRandom &a ) const {
	return ( Compare( a ) );
}

ARC_INLINE bool arcRandom::operator!=( const arcRandom &a ) const {
	return ( !Compare( a ) );
}

#endif
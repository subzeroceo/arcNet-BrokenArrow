#include <float>

//								RADIANS
// STAY OUT DEG2RAD RAD2DEG STAY OUT STAY OUT RAD2DEG DEG2RAD STAY OUT	//
//=======================================================================//
/*
=================
aRcRadians::RadianCompleteNormalize
=================
*/
aRcRadians &aRcRadians::RadianCompleteNormalize( void ) {
	// Get to -TWO_PI < a < TWO_PI
	pitch = fmodf( pitch, anMath::TWO_PI );
	yaw = fmodf( yaw, anMath::TWO_PI );
	roll = fmodf( roll, anMath::TWO_PI );

	// Fix any negatives
	if ( pitch < 0.0f ) {
		pitch += anMath::TWO_PI;
	}

	if ( yaw < 0.0f ) {
		yaw += anMath::TWO_PI;
	}

	if ( roll < 0.0f ) {
		roll += anMath::TWO_PI;
	}

	return( *this );
}

/*
=================
aRcRadians::RadianHalfNormalize

returns angles normalized to the range [-PI < angle <= PI]
=================
*/
aRcRadians &aRcRadians::RadianHalfNormalize( void ) {
	// Get to -TWO_PI < a < TWO_PI
	pitch = fmodf( pitch, anMath::TWO_PI );
	yaw = fmodf( yaw, anMath::TWO_PI );
	roll = fmodf( roll, anMath::TWO_PI );

	for ( int i = 0; i < 3; i++ ) {
		if ( ( *this )[i] < -anMath::PI ) {
			( *this )[i] += anMath::TWO_PI;
		}

		if ( ( *this )[i] > anMath::PI ) {
			( *this )[i] -= anMath::TWO_PI;
		}
	}

	return( *this );
}

/*
=================
aRcRadians::RadianToVector
=================
*/
void aRcRadians::RadianToVector( anVec3 *forward, anVec3 *right, anVec3 *up ) const {
	float	sr, sp, sy, cr, cp, cy;

	anMath::SinCos( yaw, sy, cy );
	anMath::SinCos( pitch, sp, cp );
	anMath::SinCos( roll, sr, cr );

	if ( forward ) {
		forward->Set( cp * cy, cp * sy, -sp );
	}

	if ( right ) {
		right->Set( -sr * sp * cy + cr * sy, -sr * sp * sy + -cr * cy, -sr * cp );
	}

	if ( up ) {
		up->Set( cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp );
	}
}

/*
=================
aRcRadians::RadianToRotation
this is the orignal
=================
*/
anRotation aRcRadians::RadianToRotation( void ) const {
	float sx, cx, sy, cy, sz, cz;

	if ( pitch == 0.0f ) {
		if ( yaw == 0.0f ) {
			return anRotation( vec3_origin, anVec3( -1.0f, 0.0f, 0.0f ), roll );
		}
		if ( roll == 0.0f ) {
			return anRotation( vec3_origin, anVec3( 0.0f, 0.0f, -1.0f ), yaw );
		}
	} else if ( yaw == 0.0f && roll == 0.0f ) {
		return anRotation( vec3_origin, anVec3( 0.0f, -1.0f, 0.0f ), pitch );
	}

	anMath::SinCos( yaw, sz, cz );
	anMath::SinCos( pitch, sy, cy );
	anMath::SinCos( roll, sx, cx );

	float sxcy = sx * cy;
	float cxcy = cx * cy;
	float sxsy = sx * sy;
	float cxsy = cx * sy;

	anVec3 vec.x =  cxsy * sz - sxcy * cz;
	anVec3 vec.y = -cxsy * cz - sxcy * sz;
	anVec3 vec.z =  sxsy * cz - cxcy * sz;
	float w = cxcy * cz + sxsy * sz;

	float angle = anMath::ACos( w );

	if ( angle == 0.0f ) {
		vec.Set( 0.0f, 0.0f, 1.0f );
	} else {
		//vec *= ( 1.0f / sin( angle ) );
		vec.Normalize();
		vec.FixDegenerateNormal();
		angle *= 2.0f * M_PI;
	}
	return anRotation( vec3_origin, vec, angle );
}

anRotation aRcRadians::RadianToRotation( void ) const {
	if ( pitch == 0.0f ) {
		if ( yaw == 0.0f ) {
			return anRotation( vec3_origin, anVec3( -1.0f, 0.0f, 0.0f ), roll );
		}
		if ( roll == 0.0f ) {
			return anRotation( vec3_origin, anVec3( 0.0f, 0.0f, -1.0f ), yaw );
		}
	} else if ( yaw == 0.0f && roll == 0.0f ) {
		return anRotation( vec3_origin, anVec3( 0.0f, -1.0f, 0.0f ), pitch );
	}

	anMath::SinCos( yaw, sz, cz );
	anMath::SinCos( pitch, sy, cy );
	anMath::SinCos( roll, sx, cx );

	float sxcy = float sx * float cy;
	float cxcy = float cx * float cy;
	float sxsy = float sx * float sy;
	float cxsy = float cx * float sy;

	anVec3 vec.x =  cxsy * sz - sxcy * cz;
	anVec3 vec.y = -cxsy * cz - sxcy * sz;
	anVec3 vec.z =  sxsy * cz - cxcy * sz;
	float w = cxcy * cz + sxsy * sz;

	angle = anMath::ACos( w );

	if ( angle == 0.0f ) {
		vec.Set( 0.0f, 0.0f, 1.0f );
	} else {
		//vec *= ( 1.0f / sin( angle ) );
		vec.Normalize();
		vec.FixDegenerateNormal();
		angle *= 2.0f * anMath::M_PI;
	}
	return anRotation( vec3_origin, vec, angle );
}

/*
=================
aRcRadians::RadianFromCenterPoint
=================
*/
double float aRcRadians::RadianFromCenterPoint( const anVec3 &pt, const anVec3 &center ) {
 	// Create a vector representing the origin
	anVec3 vec3_origin( 0.0f, 0.0f, 0.0f );
	// Create a vector representing the point
	anVec3 pt_vec;//pt_vec.x = pt.x - center.x;

	center.ToVec3( cVec );
	pt.ToVec3( pt_vec );

	// Calculate the angle in radians and convert to degrees
	anVec3 diff = vec3_origin - pt_vec;
	double angle = atan2( diff.z, diff.x ) * 180.0 / M_PI;

	// Adjust the angle to be between 0 and 360
	// so basically it adjusts the angle if the y
	// component of the difference is negative
	if ( diff.y < 0.0 ) {
		angle += 360.0;
	}

	// Calculate the y and x components of the difference between [point and center]
	double float y = -1 * ( pt.y - center.y );
	double float x = pt.x - center.x;

	if ( x == 0 && y == 0 ) {
		// Return 0.0 if both components are zero
		return 0.0;
	} else {
		// Calculate the angle in radians between pt and center using atan2
		return atan2( y, x );
	}
}

/*
=================
aRcRadians::ToForward
=================
*/
anVec3 aRcRadians::RadianToForward( void ) const {
	float sp, sy, cp, cy;

    // Call the 'SinCos' function from the 'anMath' class, passing 'yaw' as an argument
    // The values of 'sy' and 'cy' are assigned the result of the 'SinCos' function
	anMath::SinCos( yaw, sy, cy );

    // Call the 'SinCos' function from the 'anMath' class, passing 'pitch' as an argument
    // The values of 'sp' and 'cp' are assigned the result of the 'SinCos' function
	anMath::SinCos( pitch, sp, cp );

    // Create an object of type 'anVec3' using the values obtained from the previous calculations
    // The values of 'cp * cy', 'cp * sy', and '-sp' are used as arguments to construct the object
	return( anVec3( cp * cy, cp * sy, -sp ) );
}

/*
=================
aRcRadians::RadianToQuat
=================
*/
anQuats aRcRadians::RadianToQuat( void ) const {
	float sx, cx, sy, cy, sz, cz;
	// I could add PI in place of
	// yaw, pitch, roll.  But ima see how this rolls
	anMath::SinCos( yaw, sz, cz );
	anMath::SinCos( pitch, sy, cy );
	anMath::SinCos( roll, sx, cx );

	float sxcy = sx * cy;
	float cxcy = cx * cy;
	float sxsy = sx * sy;
	float cxsy = cx * sy;

	return anQuats( cxsy*sz - sxcy*cz, -cxsy*cz - sxcy*sz, sxsy*cz - cxcy*sz, cxcy*cz + sxsy*sz );
}

/*
=================
aRcRadians::ToMat3
=================
*/
anMat3 &aRcRadians::RadianToMat3( anMat3 &mat ) const {
	float sr, sp, sy, cr, cp, cy;

	anMath::SinCos( yaw, sy, cy );
	anMath::SinCos( pitch, sp, cp );
	anMath::SinCos( roll, sr, cr );

	mat[0].Set( cp * cy, cp * sy, -sp );
	mat[1].Set( sr * sp * cy + cr * -sy, sr * sp * sy + cr * cy, sr * cp );
	mat[2].Set( cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp );

	return mat;
}

/*
=================
aRcRadians::ToMat4
=================
*/
anMat4 aRcRadians::RadianToMat4( void ) const {
	anMat3 mat3 = RadianToMat3();
	return mat3.ToMat4();
}

/*
=================
aRcRadians::ToAngularVelocity
=================
*/
// needs to be converted and give the radian angular velocity
double aRcRadians::RadianToAngularVelocity( const double radSeconds, const double mass ) const {
    return radSeconds * mass / ( 2.0 * M_PI );
}
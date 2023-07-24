#include "../precompiled.h"
#pragma hdrstop

#include <float.h>

arcAngles ang_zero( 0.0f, 0.0f, 0.0f );

/*
=================
arcAngles::Normalize360

returns angles normalized to the range [0 <= angle < 360]
=================
*/
arcAngles& arcAngles::Normalize360( void ) {
	for ( int i = 0; i < 3; i++ ) {
		if ( ( ( *this )[i] >= 360.0f ) || ( ( *this )[i] < 0.0f ) ) {
			( *this )[i] -= floor( ( *this )[i] / 360.0f ) * 360.0f;
			if ( ( *this )[i] >= 360.0f ) {
				( *this )[i] -= 360.0f;
			}
			if ( ( *this )[i] < 0.0f ) {
				( *this )[i] += 360.0f;
			}
		}
	}

	return *this;
}

/*
=================
arcAngles::Normalize180

returns angles normalized to the range [-180 < angle <= 180]
=================
*/
arcAngles& arcAngles::Normalize180( void ) {
	Normalize360();

	if ( pitch > 180.0f ) {
		pitch -= 360.0f;
	}

	if ( yaw > 180.0f ) {
		yaw -= 360.0f;
	}

	if ( roll > 180.0f ) {
		roll -= 360.0f;
	}
	return *this;
}

/*
=================
arcAngles::LerpAngle

returns angles normalized to the range [-180 < angle <= 180]
=================
*/
float arcAngles::LerpAngle( float from, float to, float frac ) {
	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}

	return( from + frac * ( to - from ) );
}

/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float arcAngles::Subtract( float a1, float a2 ) {
	float a = a1 - a2;

	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}

/*
=================
arcAngles::ToVectors
=================
*/
void arcAngles::ToVectors( arcVec3 *forward, arcVec3 *right, arcVec3 *up ) const {
	float sr, sp, sy, cr, cp, cy;

	arcMath::SinCos( DEG2RAD( yaw ), sy, cy );
	arcMath::SinCos( DEG2RAD( pitch ), sp, cp );
	arcMath::SinCos( DEG2RAD( roll ), sr, cr );

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
arcAngles::ToForward
=================
*/
arcVec3 arcAngles::ToForward( void ) const {
	float sp, sy, cp, cy;

	arcMath::SinCos( DEG2RAD( yaw ), sy, cy );
	arcMath::SinCos( DEG2RAD( pitch ), sp, cp );

	return arcVec3( cp * cy, cp * sy, -sp );
}

/*
=================
arcAngles::ToQuat
=================
*/
arcQuats arcAngles::ToQuat( void ) const {
	float sx, cx, sy, cy, sz, cz;
	float sxcy, cxcy, sxsy, cxsy;

	arcMath::SinCos( DEG2RAD( yaw ) * 0.5f, sz, cz );
	arcMath::SinCos( DEG2RAD( pitch ) * 0.5f, sy, cy );
	arcMath::SinCos( DEG2RAD( roll ) * 0.5f, sx, cx );

	sxcy = sx * cy;
	cxcy = cx * cy;
	sxsy = sx * sy;
	cxsy = cx * sy;

	return arcQuats( cxsy*sz - sxcy*cz, -cxsy*cz - sxcy*sz, sxsy*cz - cxcy*sz, cxcy*cz + sxsy*sz );
}

/*
=================
arcAngles::ToRotation
=================
*/
arcRotate arcAngles::ToRotation( void ) const {
	arcVec3 vec;
	float angle, w;
	float sx, cx, sy, cy, sz, cz;
	float sxcy, cxcy, sxsy, cxsy;

	if ( pitch == 0.0f ) {
		if ( yaw == 0.0f ) {
			return arcRotate( vec3_origin, arcVec3( -1.0f, 0.0f, 0.0f ), roll );
		}
		if ( roll == 0.0f ) {
			return arcRotate( vec3_origin, arcVec3( 0.0f, 0.0f, -1.0f ), yaw );
		}
	} else if ( yaw == 0.0f && roll == 0.0f ) {
		return arcRotate( vec3_origin, arcVec3( 0.0f, -1.0f, 0.0f ), pitch );
	}

	arcMath::SinCos( DEG2RAD( yaw ) * 0.5f, sz, cz );
	arcMath::SinCos( DEG2RAD( pitch ) * 0.5f, sy, cy );
	arcMath::SinCos( DEG2RAD( roll ) * 0.5f, sx, cx );

	sxcy = sx * cy;
	cxcy = cx * cy;
	sxsy = sx * sy;
	cxsy = cx * sy;

	vec.x =  cxsy * sz - sxcy * cz;
	vec.y = -cxsy * cz - sxcy * sz;
	vec.z =  sxsy * cz - cxcy * sz;
	w =		 cxcy * cz + sxsy * sz;
	angle = arcMath::ACos( w );
	if ( angle == 0.0f ) {
		vec.Set( 0.0f, 0.0f, 1.0f );
	} else {
		//vec *= (1.0f / sin( angle ) );
		vec.Normalize();
		vec.FixDegenerateNormal();
		angle *= 2.0f * arcMath::M_RAD2DEG;
	}
	return arcRotate( vec3_origin, vec, angle );
}

/*
=================
arcAngles::ToMat3
=================
*/
arcMat3 arcAngles::ToMat3( void ) const {
	arcMat3 mat;
	float sr, sp, sy, cr, cp, cy;

	arcMath::SinCos( DEG2RAD( yaw ), sy, cy );
	arcMath::SinCos( DEG2RAD( pitch ), sp, cp );
	arcMath::SinCos( DEG2RAD( roll ), sr, cr );

	mat[ 0 ].Set( cp * cy, cp * sy, -sp );
	mat[ 1 ].Set( sr * sp * cy + cr * -sy, sr * sp * sy + cr * cy, sr * cp );
	mat[ 2 ].Set( cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp );

	return mat;
}

/*
=================
arcAngles::ToMat4
=================
*/
arcMat4 arcAngles::ToMat4( void ) const {
	return ToMat3().ToMat4();
}

/*
=================
arcAngles::ToAngularVelocity
=================
*/
arcVec3 arcAngles::ToAngularVelocity( void ) const {
	arcRotate rotation = arcAngles::ToRotation();
	return rotation.GetVec() * DEG2RAD( rotation.GetAngle() );
}

/*
=================
arcAngles::ToAngularVelocity

FIXME: HELP!! I dont know if it belongs here in angles or radians..
it returns radians but its angles from a point ??
=================
*/
double float arcAngles::AngleFromPoint( arcAngles pt, arcAngles center ) {
	arcRotate rotation = arcAngles::ToRotation();
	float angle = rotation.GetAngle();
	float d = pt - center;
	float a = ( float )DEG2RAD( angle );
	float s = arcMath::Sin( a );
	float c = arcMath::Cos( a );

	angle = d.ToAngles().ToDegrees();
	d = d.Normalize360();
	angle = angle * d;
	angle = center + angle;
	/*double float y = -1 * ( pt.y - center.y );
	double float x = pt.x - center.x;
	if ( x == 0 && y == 0 ) {
		return 0.0;
	} else {
		return atan2( y, x );
	}*/
}

/*
=============
arcAngles::ToString
=============
*/
const char *arcAngles::ToString( int precision ) const {
	return arcNetString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

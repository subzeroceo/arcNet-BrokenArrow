#include "../precompiled.h"
#pragma hdrstop

aRcComplex complexOrigin( 0.0f, 0.0f );

/*
=============
aRcComplex::ToString
=============
*/
const char *aRcComplex::ToString( int precision ) const {
	return anString::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

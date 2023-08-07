#include "../Lib.h"
#pragma hdrstop

aRcComplex complexOrigin( 0.0f, 0.0f );

/*
=============
aRcComplex::ToString
=============
*/
const char *aRcComplex::ToString( int precision ) const {
	return anStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

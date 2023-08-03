#include "Lib.h"
#pragma hdrstop

ARC_INLINE unsigned int FloatCRC( float f ) {
	return *(unsigned int *)&f;
}

ARC_INLINE unsigned int StringCRC( const char *str ) {
    unsigned int crc = 0;
	const unsigned char *ptr = reinterpret_cast<const unsigned char*>( str );
	for ( unsigned int i = 0; str[i]; i++ ) {
		unsigned int crc ^= str[i] << ( i & 3 );
	}
	return crc;
}

/*
=================
ComputeAxisBase

WARNING : special case behaviour of atan2(y,x) <-> atan(y/x) might not be the same everywhere when x == 0
rotation by (0,RotY,RotZ) assigns X to normal
=================
*/
static void ComputeAxisBase( const anVec3 &normal, anVec3 &texS, anVec3 &texT ) {
	// do some cleaning
	anVec3 n[0] = ( anMath::Fabs( normal[0] ) < 1e-6f ) ? 0.0f : normal[0];
    anVec3 n[1] = ( anMath::Fabs( normal[1] ) < 1e-6f ) ? 0.0f : normal[1];
    anVec3 n[2] = ( anMath::Fabs( normal[2] ) < 1e-6f ) ? 0.0f : normal[2];

    float RotY = -atan2( n[2], anMath::Sqrt(n[1] * n[1] + n[0] * n[0] ) );
    float RotZ = atan2( n[1], n[0] );
    // rotate (0,1,0 ) and (0,0,1 ) to compute texS and texT
    texS[0] = -sin( RotZ );
    texS[1] = cos( RotZ );
    texS[2] = 0;
    // the texT vector is along -Z ( T texture coorinates axis )
    texT[0] = -sin( RotY ) * cos( RotZ );
    texT[1] = -sin( RotY ) * sin( RotZ );
    texT[2] = -cos( RotY );
}

/*
=================
anMapBrushSides::GetTextureVectors
=================
*/
void anMapBrushSides::GetTextureVectors( anVec4 v[2] ) const {
	ComputeAxisBase( plane.Normal(), anVec3 texX, anVec3 texY );
	for ( int i = 0; i < 2; i++ ) {
           v[i][0] = texX[0] * texMat[i][0] + texY[0] * texMat[i][1];
           v[i][1] = texX[1] * texMat[i][0] + texY[1] * texMat[i][1];
           v[i][2] = texX[2] * texMat[i][0] + texY[2] * texMat[i][1];
           v[i][3] = texMat[i][2] + ( origin * v[i].ToVec3() );
	}
}

/*
===============
anMapPatch::GetGeometryCRC
===============
*/
unsigned int anMapPatch::GetGeometryCRC( void ) const {
	unsigned int crc = GetHorzSubdivisions() ^ GetVertSubdivisions();
	for ( int i = 0; i < GetWidth(); i++ ) {
		for ( int j = 0; j < GetHeight(); j++ ) {
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.x );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.y );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.z );
		}
	}

	crc ^= StringCRC( GetMaterial() );

	return crc;
}

/*
===============
anMapBrush::GetGeometryCRC
===============
*/
unsigned int anMapBrush::GetGeometryCRC( void ) const {
	unsigned int crc = 0;
	for ( int i = 0; i < GetNumSides(); i++ ) {
		anMapBrushSides *mapSide = GetSide( i );
		for ( int j = 0; j < 4; j++ ) {
			crc ^= FloatCRC( mapSide->GetPlane()[j] );
		}
		crc ^= StringCRC( mapSide->GetMaterial() );
	}

	return crc;
}

/*
===============
anMapEntity::GetGeometryCRC
===============
*/
unsigned int anMapEntity::GetGeometryCRC( void ) const {
        unsigned int crc = 0;
        for ( int i = 0; i < GetNumPrimitives(); i++ ) {
          anMapPrimitive *mapPrim = GetPrimitive( i );
          switch ( mapPrim->GetType() ) {
          case anMapPrimitive::TYPE_BRUSH:
            crc ^= static_cast<anMapBrush *>( mapPrim )->GetGeometryCRC();
            break;
          case anMapPrimitive::TYPE_PATCH:
            crc ^= static_cast<anMapPatch *>( mapPrim )->GetGeometryCRC();
            break;
		}
	}

	return crc;
}

/*
===============
anMapFile::SetGeometryCRC
===============
*/
void anMapFile::SetGeometryCRC( void ) {
	geometryCRC = 0;
	for ( int i = 0; i < entities.Num(); i++ ) {
		geometryCRC ^= entities[i]->GetGeometryCRC();
	}
}

/*
===============
anMapFile::NeedsReload
===============
*/
bool anMapFile::NeedsReload() {
	if ( name.Length() ) {
		ARC_TIME_T time = (ARC_TIME_T)-1;
		if ( anLibrary::fileSystem->ReadFile( name, nullptr, &time ) > 0 ) {
			return ( time > fileTime );
		}
	}
	return true;
}

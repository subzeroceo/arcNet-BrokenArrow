
#ifndef __PLANESET_H__
#define __PLANESET_H__

/*
===============================================================================

	Plane Set

===============================================================================
*/

class arcPlaneSet : public anList<anPlane> {
public:

	void					Clear( void ) { anList<anPlane>::Clear(); hash.Free(); }

	int						FindPlane( const anPlane &plane, const float normalEps, const float distEps );

private:
	arcHashIndex				hash;
};

inline int arcPlaneSet::FindPlane( const anPlane &plane, const float normalEps, const float distEps ) {
	assert( distEps <= 0.125f );
	int hashKey = (int)( anMath::Fabs( plane.Dist() ) * 0.125f );
	for ( int border = -1; border <= 1; border++ ) {
		for ( int i = hash.First( hashKey + border ); i >= 0; i = hash.Next( i ) ) {
			if ( (*this)[i].Compare( plane, normalEps, distEps ) ) {
				return i;
			}
		}
	}

	if ( plane.Type() >= PLANETYPE_NEGX && plane.Type() < PLANETYPE_TRUEAXIAL ) {
		Append( -plane );
		hash.Add( hashKey, Num()-1 );
		Append( plane );
		hash.Add( hashKey, Num()-1 );
		return ( Num() - 1 );
	} else {
		Append( plane );
		hash.Add( hashKey, Num()-1 );
		Append( -plane );
		hash.Add( hashKey, Num()-1 );
		return ( Num() - 2 );
	}
}

#endif /* !__PLANESET_H__ */

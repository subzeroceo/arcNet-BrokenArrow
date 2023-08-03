#include "/home/subzeroceo/ArC-NetSoftware-Projects/brokenarrow/idlib/Lib.h"

class anRay {
public:

    //anRay() : position(0,0,0), direction(0,0,1) {}
    //anRay( const anVec3 &pos, const anVec3 &dir ) : position(pos), direction(dir) {}

    ARC_INLINE anRay( const anVec3 &origin, const anVec3 &dir ) : origin( origin ), dir( dir ) { }

    // Comparision operators
    bool operator == ( const anRay& r ) const;
    bool operator != ( const anRay& r ) const;

    ARC_INLINE anVec3 IntersectPlane( const anPlane &plane ) const {
        float t = -( Dot( origin, plane.normal ) + plane.dist ) / Dot( dir, plane.normal );
        return origin + dir * t;
    }

    ARC_INLINE anVec3 Reflect( const anVec3 &normal ) const {
        return dir - 2.0f * Dot( dir, normal ) * normal;
    }

	ARC_INLINE anVec3 Refract( const anVec3 &normal, float eta ) const {
        float cosI = -Dot( dir, normal );
        float sinT2 = eta * eta * ( 1.0f - cosI * cosI );
        if ( sinT2 > 1.0f ) {
            return anVec3(); // Total internal reflection
        }
        float cosT = anMath::Sqrt( 1.0f - sinT2 );
        return eta * dir + ( eta * cosI - cosT ) * normal;
	}

	// Calculate the intersection point of a ray and a plane
	ARC_INLINE anVec3 IntersectRayPlane( const anRay &ray, const anPlane &plane ) {
    float t = -( Dot( ray.origin, plane.normal ) + plane.dist ) / Dot( ray.dir, plane.normal );
    return ray.origin + ray.dir * t;
	}
	ARC_INLINE virtual void AddPoint( const anVec3 &v ) {}
	ARC_INLINE virtual void RemovePoint( int index ) {}
	ARC_INLINE virtual anVec3 *GetPoint( int index ) { return nullptr; }


	ARC_INLINE int SelectPointByRay( float ox, float oy, float oz, float dx, float dy, float dz, bool single ) {
		anVec3 origin( ox, oy, oz );
		anVec3 dir( dx, dy, dz );
		return SelectPointByRay( origin, dir, single );
	}
	ARC_INLINE int SelectPointByRay( const anVec3 origin, const anVec3 direction, bool single ) {
		// find the point closest to the ray
		int besti = -1;
		float bestd = 8;
		int count = numPoints();

		for ( int i = 0; i < count; i++ ) {
			anVec3 temp = *GetPoint( i );
			anVec3 temp2 = temp;
			temp -= origin;
			float d = direction.Dot( temp );
			VectorMA( origin, d, direction, temp );
			temp2 -= temp;
			d = temp2.Length();
			if ( d <= bestd ) {
				bestd = d;
				besti = i;
			}
		}

		if ( besti >= 0 ) {
			selectPoint( besti, single );
		}

		return besti;
	}
public:
	anVec3	origin, dir,
};

// Calculate the dot product of two vectors
float Dot( const anVec3 &v1, const anVec3 &v2 ) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// Calculate the cross product of two vectors
anVec3 Cross( const anVec3 &v1, const anVec3 &v2 ) {
    return anVec3( v1.y * v2.z - v1.z * v2.y,
                   v1.z * v2.x - v1.x * v2.z,
                   v1.x * v2.y - v1.y * v2.x );
}

// Calculate the distance between a point and a plane
float DistToPlane( const anPlane &plane, const anVec3 &point ) {
    return Dot( plane.normal, point ) + plane.dist;
}


// Calculate the volume of a bounding box
float calcBBoxVolume( const anBounds &bounds ) {
    anVec3 dimensions = bounds.maxPoint - bounds.minPoint;
    return dimensions.x * dimensions.y * dimensions.z;
}/*
void idInterpolatedPosition::write(fileHandle_t file, const char *p) {
	anString s = va( "\t%s {\n", p);
	FS_Write( s.c_str(), s.length(), file);
	idCameraPosition::write(file, p);
	s = va( "\t\tstartPos ( %f %f %f )\n", startPos.x, startPos.y, startPos.z);
	FS_Write( s.c_str(), s.length(), file);
	s = va( "\t\tendPos ( %f %f %f )\n", endPos.x, endPos.y, endPos.z);
	FS_Write( s.c_str(), s.length(), file);
	s = "\t}\n";
	FS_Write( s.c_str(), s.length(), file);
}

void idSplinePosition::write(fileHandle_t file, const char *p) {
	anString s = va( "\t%s {\n", p);
	FS_Write( s.c_str(), s.length(), file);
	idCameraPosition::write(file, p);
	target.write(file, "target" );
	s = "\t}\n";
	FS_Write( s.c_str(), s.length(), file);
}
*/
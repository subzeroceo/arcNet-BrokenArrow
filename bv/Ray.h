#include "/home/subzeroceo/ArC-NetSoftware-Projects/brokenarrow/idlib/Lib.h"

class anRay {
public:
    anVec3 origin;
    anVec3 dir;

    ARC_INLINE anRay( const anVec3 &origin, const anVec3 &dir ) : origin( origin ), dir( dir ) { }

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

// Calculate the intersection point of a ray and a plane
anVec3 IntersectRayPlane( const anRay &ray, const anPlane &plane ) {
    float t = -( Dot( ray.origin, plane.normal ) + plane.dist ) / Dot( ray.dir, plane.normal );
    return ray.origin + ray.dir * t;
}

// Calculate the volume of a bounding box
float calcBBoxVolume( const anBounds &bounds ) {
    anVec3 dimensions = bounds.maxPoint - bounds.minPoint;
    return dimensions.x * dimensions.y * dimensions.z;
}

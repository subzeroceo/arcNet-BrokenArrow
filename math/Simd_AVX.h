#ifndef __MATH_SIMD_AVX_H__
#define __MATH_SIMD_AVX_H__
#include <immintrin.h>
#include "Simd.h"
#include "Simd_Macros.h"

#ifdef _WIN32
#define VPCALL __fastcall
#else
#define VPCALL
#endif

class anVec2;
class anVec3;
class anVec4;
class anVec5;
class anVec6;
class anVecX;
class anMat2;
class anMat3;
class anMat4;
class anMat5;
class anMat6;
class anMatX;
class anPlane;
class anDrawVertex;
class anJointQuat;
class arcJointMat;
struct dominantTri_s;

// SIMD macro for loading values from memory
#define LOAD_SIMD(src) _mm_loadu_ps(src)
// SIMD macro for storing values to memory
#define STORE_SIMD(dest, src) _mm_storeu_ps(dest, src)
// SIMD macro for adding two vectors
#define ADD_SIMD(a, b) _mm_add_ps(a, b)
// SIMD macro for subtracting two vectors
#define SUB_SIMD(a, b) _mm_sub_ps(a, b)
// SIMD macro for multiplying two vectors
#define MUL_SIMD(a, b) _mm_mul_ps(a, b)
// SIMD macro for dividing two vectors
#define DIV_SIMD(a, b) _mm_div_ps(a, b)
// SIMD macro for taking the dot product of two vectors
#define DOT_PRODUCT_SIMD(a, b) _mm_dp_ps(a, b, 0xFF)
// SIMD macro for horizontal addition of all elements in a vector
#define HORIZONTAL_ADD_SIMD(a) _mm_hadd_ps(a, a)

// AVX macro for loading values from memory
#define LOAD_SIMD_AVX(src) _mm256_loadu_ps(src)
// AVX macro for storing values to memory
#define STORE_SIMD_AVX(dest, src) _mm256_storeu_ps(dest, src)
// AVX macro for adding two vectors
#define ADD_SIMD_AVX(a, b) _mm256_add_ps(a, b)
// AVX macro for subtracting two vectors
#define SUB_SIMD_AVX(a, b) _mm256_sub_ps(a, b)
// AVX macro for multiplying two vectors
#define MUL_SIMD_AVX(a, b) _mm256_mul_ps(a, b)
// AVX macro for dividing two vectors
#define DIV_SIMD_AVX(a, b) _mm256_div_ps(a, b)
// AVX macro for taking the dot product of two vectors
#define DOT_PRODUCT_SIMD_AVX(a, b) _mm256_dp_ps(a, b, 0xFF)

// AVX macro for horizontal addition of all elements in a vector
#define HORIZONTAL_ADD_SIMD_AVX(a) { \
    __m128 lo = _mm256_castps256_ps128(a); \
    __m128 hi = _mm256_extractf128_ps(a, 1); \
    lo = _mm_add_ps(lo, hi); \
    lo = _mm_hadd_ps(lo, lo); \
    lo = _mm_hadd_ps(lo, lo); \
    lo = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(0, 0, 0, 0)); \
    a = _mm256_broadcast_ss(&lo[0]); \
}
#pragma once

class arcSIMD_AVX : public arcSIMDProcessor {
public:
	cpuid_t							cpuid;

    VPCALL arcSIMD_AVX( int n ) = 0;
    VPCALL arcSIMD_AVX( size_t numElements ) : arcSIMDProcessor( numElements ) { }
    const char *VPCALL GetName( void ) const;
   	float VPCALL DotProduct( const float *src1, const float *src2, const int count ) = 0;
    void VPCALL Sub16 ( float *dst, const float *src1, const float *src2, const int count ) = 0;
    void VPCALL Add16( float *dst, const float *src1, const float *src2, const int count ) = 0;
    void VPCALL Dot( float *dst, const anVec3 &constant, const anDrawVertex *src, const int count ) = 0;
    void VPCALL Add( float *dst, const float *src0, const float *src1, const int count ) = 0;
    virtual void VPCALL Mul( float *dst, const float *src0, const float *src1, const int count ) = 0;
    virtual void VPCALL MulSub( float *dst, const float *src0, const float *src1, const int count ) = 0;
    virtual void VPCALL MulAdd ( float *dst, const float *src0, const float *src1, const int count ) = 0;


    virtual void VPCALL Clamp( float *dst, const float *src, const float min, const float max, const int count ) = 0;
    virtual void VPCALL ClampMin( float *dst, const float *src, const float min, const int count ) = 0;
    virtual void VPCALL ClampMax( float *dst, const float *src, const float max, const int count ) = 0;

    virtual void VPCALL Zero16( float *dst, const int count ) = 0;
    virtual void VPCALL Negate16( float *dst, const int count ) = 0;
    virtual void VPCALL Copy16( float *dst, const float *src, const int count ) = 0;
    virtual void VPCALL Add16( float *dst, const float *src1, const float *src2, const int count ) = 0;
    virtual void VPCALL Sub16( float *dst, const float *src1, const float *src2, const int count ) = 0;
    virtual void VPCALL Mul16( float *dst, const float *src1, const float constant, const int count ) = 0;
    virtual void VPCALL AddAssign16( float *dst, const float *src, const int count ) = 0;
    virtual void VPCALL SubAssign16( float *dst, const float *src, const int count ) = 0;
    virtual void VPCALL MulAssign16( float *dst, const float constant, const int count ) = 0;

    virtual void VPCALL Memcpy( void *dest0, const int val, const int count0 ) = 0;
    virtual void VPCALL Memset( void *dest0, const int val, const int count0 ) = 0;

    virtual void VPCALL MatX_LowerTriangularSolve( const anMatX &L, float *x, const float *b, const int n, int skip = 0 ) = 0;
    virtual void VPCALL MatX_LowerTriangularSolveTranspose( const anMatX &L, float *x, const float *b, const int n ) = 0;
    virtual bool VPCALL MatX_LDLTFactor( anMatX &mat, anVecX& invDiag, const int n ) = 0;

    virtual void VPCALL BlendJoints( anJointQuat *joints, const anJointQuat *blendJoints, const float lerp, const int* index, const int numJoints ) = 0;
    virtual void VPCALL ConvertJointQuatsToJointMats(arcJointMat *jointMats, const anJointQuat *jointQuats, const int numJoints ) = 0;
    virtual void VPCALL ConvertJointMatsToJointQuats(anJointQuat *jointQuats, const arcJointMat *jointMats, const int numJoints ) = 0;

    virtual void VPCALL TransformJoints( arcJointMat *jointMats, const int* parents, const int firstJoint, const int lastJoint ) = 0;
    virtual void VPCALL UntransformJoints( arcJointMat *jointMats, const int* parents, const int firstJoint, const int lastJoint ) = 0;
    virtual void VPCALL TransformVerts( anDrawVertex *verts, const int numVerts, const arcJointMat *joints, const anVec4* weights, const int* index, const int numWeights ) = 0;

    static void GetCropBounds( const frustumCorners_t &corners, const float *renderMat, anBounds &bounds );

    void VPCALL TracePointCull( byte *cullBits, byte& totalOr, const float radius, const anPlane *planes, const anDrawVertex *verts, const int numVerts ) = 0;
    virtual void VPCALL DecalPointCull( byte *cullBits, const anPlane* planes, const anDrawVertex *verts, const int numVerts ) = 0;
    virtual void VPCALL OverlayPointCull( byte *cullBits, anVec2 *texCoords, const anPlane *planes, const anDrawVertex *verts, const int numVerts ) = 0;

    virtual void VPCALL DeriveTriPlanes( anPlane *planes, const anDrawVertex *verts, const int numVerts, const int* indexes, const int numIndexes ) = 0;

 	virtual void VPCALL DeriveTriPlanes( anPlane *planes, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL DeriveTangents( anPlane *planes, anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL DeriveUnsmoothedTangents( anDrawVertex *verts, const dominantTri_s *dominantTris, const int numVerts ) = 0;
	virtual void VPCALL NormalizeTangents( anDrawVertex *verts, const int numVerts ) = 0;
};

// pointer to SIMD processor
extern arcSIMD_AVX *AVXProcessor;
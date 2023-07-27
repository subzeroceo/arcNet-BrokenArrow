#ifndef __MATH_SIMD_ALTIVEC_H__
#define __MATH_SIMD_ALTIVEC_H__

/*
===============================================================================

	AltiVec implementation of arcSIMDProcessor

===============================================================================
*/

// Defines for enabling parts of the library

// Turns on/off the simple math routines (add, sub, div, etc)
#define ENABLE_SIMPLE_MATH

// Turns on/off the dot routines
#define ENABLE_DOT

// Turns on/off the compare routines
#define ENABLE_COMPARES

// The MinMax routines introduce a couple of bugs. In the bathroom of the alphalabs2 map, the
// wrong surface appears in the mirror at times. It also introduces a noticable delay when map
// data is loaded such as going through doors.
// Turns on/off MinMax routines
//#define ENABLE_MINMAX

// Turns on/off Clamp routines
#define ENABLE_CLAMP

// Turns on/off XXX16 routines
#define ENABLE_16ROUTINES

// Turns on/off LowerTriangularSolve, LowerTriangularSolveTranspose, and MatX_LDLTFactor
#define ENABLE_LOWER_TRIANGULAR

// Turns on/off TracePointCull, DecalPointCull, and OverlayPoint
// The Enable_Cull routines breaks the g_decals functionality, DecalPointCull is
// the likely suspect. Bullet holes do not appear on the walls when this optimization
// is enabled.
//#define ENABLE_CULL

// Turns on/off DeriveTriPlanes, DeriveTangents, DeriveUnsmoothedTangents, NormalizeTangents
#define ENABLE_DERIVE

// Turns on/off CreateTextureSpaceLightVectors, CreateShadowCache, CreateVertexProgramShadowCache
#define ENABLE_CREATE

// Turns on/off the sound routines
#define ENABLE_SOUND_ROUTINES

// Turns on/off the stuff that isn't on elsewhere
// Currently: BlendJoints, TransformJoints, UntransformJoints, ConvertJointQuatsToJointMats, and
// ConvertJointMatsToJointQuats
#define LIVE_VICARIOUSLY

// This assumes that the dest (and mixBuffer) array to the sound functions is aligned. If this is not true, we take a large
// performance hit from having to do unaligned stores
//#define SOUND_DEST_ALIGNED

// This assumes that the vertexCache array to CreateShadowCache and CreateVertexProgramShadowCache is aligned. If it's not,
// then we take a big performance hit from unaligned stores.
//#define VERTEXCACHE_ALIGNED

// This turns on support for PPC intrinsics in the SIMD_AltiVec.cpp file. Right now it's only used for frsqrte. GCC
// supports these intrinsics but XLC does not.
#define PPC_INTRINSICS

// This assumes that the anDrawVertex array that is used in DeriveUnsmoothedTangents is aligned. If its not aligned,
// then we don't get any speedup
//#define DERIVE_UNSMOOTH_DRAWVERT_ALIGNED

// Disable DRAWVERT_PADDED since we disabled the ENABLE_CULL optimizations and the default
// implementation does not allow for the extra padding.
// This assumes that anDrawVertex has been padded by 4 bytes so that xyz always starts at an aligned
// address
//#define DRAWVERT_PADDED

class arcSIMD_AltiVec : public arcSIMD_Generic {
#if defined(MACOS_X) && defined(__ppc__)
public:

	virtual const char *VPCALL GetName( void ) const;

#ifdef ENABLE_SIMPLE_MATH
	// Basic math, works for both aligned and unaligned data
	virtual void VPCALL Add( float *dst, const float constant, const float *src, const int count );
    virtual void VPCALL Add( float *dst, const float *src0, const float *src1, const int count );
	virtual void VPCALL Sub( float *dst, const float constant, const float *src, const int count );
	virtual void VPCALL Sub( float *dst, const float *src0, const float *src1, const int count );
 	virtual void VPCALL Mul( float *dst, const float constant, const float *src, const int count);
	virtual void VPCALL Mul( float *dst, const float *src0, const float *src1, const int count );
	virtual void VPCALL Div( float *dst, const float constant, const float *divisor, const int count );
	virtual void VPCALL Div( float *dst, const float *src0, const float *src1, const int count );
	virtual void VPCALL MulAdd( float *dst, const float constant, const float *src, const int count );
	virtual void VPCALL MulAdd( float *dst, const float *src0, const float *src1, const int count );
	virtual void VPCALL MulSub( float *dst, const float constant, const float *src, const int count );
	virtual void VPCALL MulSub( float *dst, const float *src0, const float *src1, const int count );
#endif

#ifdef ENABLE_DOT
	// Dot products, expects data structures in contiguous memory
	virtual void VPCALL Dot( float *dst, const anVec3 &constant, const anVec3 *src, const int count );
	virtual void VPCALL Dot( float *dst, const anVec3 &constant, const anPlane *src, const int count );
	virtual void VPCALL Dot( float *dst, const anVec3 &constant, const anDrawVertex *src, const int count );
	virtual void VPCALL Dot( float *dst, const anPlane &constant,const anVec3 *src, const int count );
	virtual void VPCALL Dot( float *dst, const anPlane &constant,const anPlane *src, const int count );
	virtual void VPCALL Dot( float *dst, const anPlane &constant,const anDrawVertex *src, const int count );
	virtual void VPCALL Dot( float *dst, const anVec3 *src0, const anVec3 *src1, const int count );
	virtual void VPCALL Dot( float &dot, const float *src1, const float *src2, const int count );
#endif

#ifdef ENABLE_COMPARES
	// Comparisons, works for both aligned and unaligned data
	virtual void VPCALL CmpGT( byte *dst, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpGT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpGE( byte *dst, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpGE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpLT( byte *dst, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpLT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpLE( byte *dst, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpLE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count );
#endif

#ifdef ENABLE_MINMAX
	// Min/Max. Expects data structures in contiguous memory
	virtual void VPCALL MinMax( float &min,			float &max,				const float *src, const int count );
	virtual	void VPCALL MinMax( anVec2 &min,		anVec2 &max, const anVec2 *src, const int count );
	virtual void VPCALL MinMax( anVec3 &min,		anVec3 &max, const anVec3 *src, const int count );
	virtual	void VPCALL MinMax( anVec3 &min,		anVec3 &max, const anDrawVertex *src, const int count );
	virtual	void VPCALL MinMax( anVec3 &min,		anVec3 &max, const anDrawVertex *src, const int *indexes, const int count );
#endif

#ifdef ENABLE_CLAMP
	// Clamp operations. Works for both aligned and unaligned data
	virtual void VPCALL Clamp( float *dst, const float *src, const float min, const float max, const int count );
	virtual void VPCALL ClampMin( float *dst, const float *src, const float min, const int count );
	virtual void VPCALL ClampMax( float *dst, const float *src, const float max, const int count );
#endif

    // These are already using memcpy and memset functions. Leaving default implementation
//	virtual void VPCALL Memcpy( void *dst, const void *src, const int count );
//	virtual void VPCALL Memset( void *dst, const int val, const int count );

#ifdef ENABLE_16ROUTINES
	// Operations that expect 16-byte aligned data and 16-byte padded memory (with zeros), generally faster
	virtual void VPCALL Zero16( float *dst, const int count );
	virtual void VPCALL Negate16( float *dst, const int count );
	virtual void VPCALL Copy16( float *dst, const float *src, const int count );
	virtual void VPCALL Add16( float *dst, const float *src1, const float *src2, const int count );
	virtual void VPCALL Sub16( float *dst, const float *src1, const float *src2, const int count );
	virtual void VPCALL Mul16( float *dst, const float *src1, const float constant, const int count );
	virtual void VPCALL AddAssign16( float *dst, const float *src, const int count );
	virtual void VPCALL SubAssign16( float *dst, const float *src, const int count );
	virtual void VPCALL MulAssign16( float *dst, const float constant, const int count );
#endif

//  Most of these deal with tiny matrices or vectors, generally not worth altivec'ing since
//  the scalar code is already really fast

//	virtual void VPCALL MatX_MultiplyVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
//	virtual void VPCALL MatX_MultiplyAddVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
//	virtual void VPCALL MatX_MultiplySubVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
//	virtual void VPCALL MatX_TransposeMultiplyVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
//	virtual void VPCALL MatX_TransposeMultiplyAddVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
//	virtual void VPCALL MatX_TransposeMultiplySubVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
//	virtual void VPCALL MatX_MultiplyMatX( anMatX &dst, const anMatX &m1, const anMatX &m2 );
//	virtual void VPCALL MatX_TransposeMultiplyMatX( anMatX &dst, const anMatX &m1, const anMatX &m2 );

#ifdef ENABLE_LOWER_TRIANGULAR
	virtual void VPCALL MatX_LowerTriangularSolve( const anMatX &L, float *x, const float *b, const int n, int skip = 0 );
	virtual void VPCALL MatX_LowerTriangularSolveTranspose( const anMatX &L, float *x, const float *b, const int n );
	virtual bool VPCALL MatX_LDLTFactor( anMatX &mat, anVecX &invDiag, const int n );
#endif
#ifdef LIVE_VICARIOUSLY
	virtual void VPCALL BlendJoints( anJointQuat *joints, const anJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL ConvertJointQuatsToJointMats( arcJointMat *jointMats, const anJointQuat *jointQuats, const int numJoints );
	virtual void VPCALL ConvertJointMatsToJointQuats( anJointQuat *jointQuats, const arcJointMat *jointMats, const int numJoints );
#endif

#ifdef LIVE_VICARIOUSLY
	virtual void VPCALL TransformJoints( arcJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL UntransformJoints( arcJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL TransformVerts( anDrawVertex *verts, const int numVerts, const arcJointMat *joints, const anVec4 *weights, const int *index, const int numWeights );
#endif

#ifdef ENABLE_CULL
	virtual void VPCALL TracePointCull( byte *cullBits, byte &totalOr, const float radius, const anPlane *planes, const anDrawVertex *verts, const int numVerts );
	virtual void VPCALL DecalPointCull( byte *cullBits, const anPlane *planes, const anDrawVertex *verts, const int numVerts );
	virtual void VPCALL OverlayPointCull( byte *cullBits, anVec2 *texCoords, const anPlane *planes, const anDrawVertex *verts, const int numVerts );
#endif

#ifdef ENABLE_DERIVE
	virtual void VPCALL DeriveTriPlanes( anPlane *planes, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void VPCALL DeriveTangents( anPlane *planes, anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void VPCALL DeriveUnsmoothedTangents( anDrawVertex *verts, const dominantTri_s *dominantTris, const int numVerts );
	virtual void VPCALL NormalizeTangents( anDrawVertex *verts, const int numVerts );
#endif

#ifdef ENABLE_CREATE
	virtual void VPCALL CreateTextureSpaceLightVectors( anVec3 *lightVectors, const anVec3 &lightOrigin, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void VPCALL CreateSpecularTextureCoords( anVec4 *texCoords, const anVec3 &lightOrigin, const anVec3 &viewOrigin, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual int  VPCALL CreateShadowCache( anVec4 *vertexCache, int *vertRemap, const anVec3 &lightOrigin, const anDrawVertex *verts, const int numVerts );
	virtual int  VPCALL CreateVertexProgramShadowCache( anVec4 *vertexCache, const anDrawVertex *verts, const int numVerts );
#endif

#ifdef ENABLE_SOUND_ROUTINES
	// Sound upsampling and mixing routines, works for aligned and unaligned data
	virtual void VPCALL UpSamplePCMTo44kHz( float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples );
#endif
#endif

};

#endif /* !__MATH_SIMD_ALTIVEC_H__ */

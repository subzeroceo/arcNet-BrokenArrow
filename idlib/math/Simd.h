#ifndef __MATH_SIMD_H__
#define __MATH_SIMD_H__

/*
===============================================================================

	Single Instruction Multiple Data (SIMD)

	For optimal use data should be aligned on a 16 byte boundary.
	All arcSIMDProcessor routines are thread safe.

===============================================================================
*/

class arcSIMD {
public:
	static void			Init( void );
	static void			InitProcessor( const char *module, bool forceGeneric );
	static void			Shutdown( void );
	static void			Test_f( const class arcCommandArgs &args );
};


/*
===============================================================================

	virtual base class for different SIMD processors

===============================================================================
*/

#ifdef _WIN32
#define VPCALL __fastcall
#else
#define VPCALL
#endif

class arcVec2;
class arcVec3;
class arcVec4;
class arcVec5;
class arcVec6;
class arcVecX;
class arcMat2;
class arcMat3;
class arcMat4;
class arcMat5;
class arcMat6;
class arcMatX;
class arcPlane;
class arcDrawVert;
class idJointQuat;
class arcJointMat;
struct dominantTri_s;

const int MIXBUFFER_SAMPLES = 4096;

typedef enum {
	SPEAKER_LEFT = 0,
	SPEAKER_RIGHT,
	SPEAKER_CENTER,
	SPEAKER_LFE,
	SPEAKER_BACKLEFT,
	SPEAKER_BACKRIGHT
} speakerLabel;


class arcSIMDProcessor {
public:
									arcSIMDProcessor( void ) { cpuid = CPUID_NONE; }

	cpuid_t							cpuid;

	virtual const char * VPCALL		GetName( void ) const = 0;

	virtual void VPCALL Add( float *dst, const float constant, const float *src, const int count ) = 0;
	virtual void VPCALL Add( float *dst, const float *src0, const float *src1, const int count ) = 0;
	virtual void VPCALL Sub( float *dst, const float constant, const float *src, const int count ) = 0;
	virtual void VPCALL Sub( float *dst, const float *src0, const float *src1, const int count ) = 0;
	virtual void VPCALL Mul( float *dst, const float constant, const float *src, const int count ) = 0;
	virtual void VPCALL Mul( float *dst, const float *src0, const float *src1, const int count ) = 0;
	virtual void VPCALL Div( float *dst, const float constant, const float *src, const int count ) = 0;
	virtual void VPCALL Div( float *dst, const float *src0, const float *src1, const int count ) = 0;
	virtual void VPCALL MulAdd( float *dst, const float constant, const float *src, const int count ) = 0;
	virtual void VPCALL MulAdd( float *dst, const float *src0, const float *src1, const int count ) = 0;
	virtual void VPCALL MulSub( float *dst, const float constant, const float *src, const int count ) = 0;
	virtual void VPCALL MulSub( float *dst, const float *src0, const float *src1, const int count ) = 0;

	virtual	void VPCALL Dot( float *dst, const arcVec3 &constant, const arcVec3 *src, const int count ) = 0;
	virtual	void VPCALL Dot( float *dst, const arcVec3 &constant, const arcPlane *src, const int count ) = 0;
	virtual void VPCALL Dot( float *dst, const arcVec3 &constant, const arcDrawVert *src, const int count ) = 0;
	virtual	void VPCALL Dot( float *dst, const arcPlane &constant,const arcVec3 *src, const int count ) = 0;
	virtual	void VPCALL Dot( float *dst, const arcPlane &constant,const arcPlane *src, const int count ) = 0;
	virtual void VPCALL Dot( float *dst, const arcPlane &constant,const arcDrawVert *src, const int count ) = 0;
	virtual	void VPCALL Dot( float *dst, const arcVec3 *src0, const arcVec3 *src1, const int count ) = 0;
	virtual void VPCALL Dot( float &dot, const float *src1, const float *src2, const int count ) = 0;

	virtual	void VPCALL CmpGT( byte *dst, const float *src0, const float constant, const int count ) = 0;
	virtual	void VPCALL CmpGT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) = 0;
	virtual	void VPCALL CmpGE( byte *dst, const float *src0, const float constant, const int count ) = 0;
	virtual	void VPCALL CmpGE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) = 0;
	virtual	void VPCALL CmpLT( byte *dst, const float *src0, const float constant, const int count ) = 0;
	virtual	void VPCALL CmpLT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) = 0;
	virtual	void VPCALL CmpLE( byte *dst, const float *src0, const float constant, const int count ) = 0;
	virtual	void VPCALL CmpLE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) = 0;

	virtual	void VPCALL MinMax( float &min, float &max, const float *src, const int count ) = 0;
	virtual	void VPCALL MinMax( arcVec2 &min, arcVec2 &max, const arcVec2 *src, const int count ) = 0;
	virtual	void VPCALL MinMax( arcVec3 &min, arcVec3 &max, const arcVec3 *src, const int count ) = 0;
	virtual	void VPCALL MinMax( arcVec3 &min, arcVec3 &max, const arcDrawVert *src, const int count ) = 0;
	virtual	void VPCALL MinMax( arcVec3 &min, arcVec3 &max, const arcDrawVert *src, const int *indexes, const int count ) = 0;

	virtual	void VPCALL Clamp( float *dst, const float *src, const float min, const float max, const int count ) = 0;
	virtual	void VPCALL ClampMin( float *dst, const float *src, const float min, const int count ) = 0;
	virtual	void VPCALL ClampMax( float *dst, const float *src, const float max, const int count ) = 0;

	virtual void VPCALL Memcpy( void *dst, const void *src, const int count ) = 0;
	virtual void VPCALL Memset( void *dst, const int val, const int count ) = 0;

	// these assume 16 byte aligned and 16 byte padded memory
	virtual void VPCALL Zero16( float *dst, const int count ) = 0;
	virtual void VPCALL Negate16( float *dst, const int count ) = 0;
	virtual void VPCALL Copy16( float *dst, const float *src, const int count ) = 0;
	virtual void VPCALL Add16( float *dst, const float *src1, const float *src2, const int count ) = 0;
	virtual void VPCALL Sub16( float *dst, const float *src1, const float *src2, const int count ) = 0;
	virtual void VPCALL Mul16( float *dst, const float *src1, const float constant, const int count ) = 0;
	virtual void VPCALL AddAssign16( float *dst, const float *src, const int count ) = 0;
	virtual void VPCALL SubAssign16( float *dst, const float *src, const int count ) = 0;
	virtual void VPCALL MulAssign16( float *dst, const float constant, const int count ) = 0;

	// arcMatX operations
	virtual void VPCALL MatX_MultiplyVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec ) = 0;
	virtual void VPCALL MatX_MultiplyAddVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec ) = 0;
	virtual void VPCALL MatX_MultiplySubVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec ) = 0;
	virtual void VPCALL MatX_TransposeMultiplyVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec ) = 0;
	virtual void VPCALL MatX_TransposeMultiplyAddVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec ) = 0;
	virtual void VPCALL MatX_TransposeMultiplySubVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec ) = 0;
	virtual void VPCALL MatX_MultiplyMatX( arcMatX &dst, const arcMatX &m1, const arcMatX &m2 ) = 0;
	virtual void VPCALL MatX_TransposeMultiplyMatX( arcMatX &dst, const arcMatX &m1, const arcMatX &m2 ) = 0;
	virtual void VPCALL MatX_LowerTriangularSolve( const arcMatX &L, float *x, const float *b, const int n, int skip = 0 ) = 0;
	virtual void VPCALL MatX_LowerTriangularSolveTranspose( const arcMatX &L, float *x, const float *b, const int n ) = 0;
	virtual bool VPCALL MatX_LDLTFactor( arcMatX &mat, arcVecX &invDiag, const int n ) = 0;

	// rendering
	virtual void VPCALL BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) = 0;
	virtual void VPCALL ConvertJointQuatsToJointMats( arcJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints ) = 0;
	virtual void VPCALL ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const arcJointMat *jointMats, const int numJoints ) = 0;
	virtual void VPCALL TransformJoints( arcJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
	virtual void VPCALL UntransformJoints( arcJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
	virtual void VPCALL TransformVerts( arcDrawVert *verts, const int numVerts, const arcJointMat *joints, const arcVec4 *weights, const int *index, const int numWeights ) = 0;
	virtual void VPCALL TracePointCull( byte *cullBits, byte &totalOr, const float radius, const arcPlane *planes, const arcDrawVert *verts, const int numVerts ) = 0;
	virtual void VPCALL DecalPointCull( byte *cullBits, const arcPlane *planes, const arcDrawVert *verts, const int numVerts ) = 0;
	virtual void VPCALL OverlayPointCull( byte *cullBits, arcVec2 *texCoords, const arcPlane *planes, const arcDrawVert *verts, const int numVerts ) = 0;
	virtual void VPCALL DeriveTriPlanes( arcPlane *planes, const arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL DeriveTangents( arcPlane *planes, arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL DeriveUnsmoothedTangents( arcDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts ) = 0;
	virtual void VPCALL NormalizeTangents( arcDrawVert *verts, const int numVerts ) = 0;
	virtual void VPCALL CreateTextureSpaceLightVectors( arcVec3 *lightVectors, const arcVec3 &lightOrigin, const arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL CreateSpecularTextureCoords( arcVec4 *texCoords, const arcVec3 &lightOrigin, const arcVec3 &viewOrigin, const arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual int  VPCALL CreateShadowCache( arcVec4 *vertexCache, int *vertRemap, const arcVec3 &lightOrigin, const arcDrawVert *verts, const int numVerts ) = 0;
	virtual int  VPCALL CreateVertexProgramShadowCache( arcVec4 *vertexCache, const arcDrawVert *verts, const int numVerts ) = 0;

	// sound mixing
	virtual void VPCALL UpSamplePCMTo44kHz( float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels ) = 0;
	virtual void VPCALL UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels ) = 0;
	virtual void VPCALL MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) = 0;
	virtual void VPCALL MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) = 0;
	virtual void VPCALL MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) = 0;
	virtual void VPCALL MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) = 0;
	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples ) = 0;
};

// pointer to SIMD processor
extern arcSIMDProcessor *SIMDProcessor;

#endif

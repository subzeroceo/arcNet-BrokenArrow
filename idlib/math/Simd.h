#ifndef __MATH_SIMD_H__
#define __MATH_SIMD_H__

/*
===============================================================================

	Single Instruction Multiple Data (SIMD)

	For optimal use data should be aligned on a 16 byte boundary.
	All anSIMDProcessor routines are thread safe.

===============================================================================
*/

class anSIMD {
public:
	static void			Init( void );
	static void			InitProcessor( const char *module, bool forceGeneric );
	static void			Shutdown( void );
	static void			Test_f( const class anCommandArgs &args );
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
class anJointMat;
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


class anSIMDProcessor {
public:
									anSIMDProcessor( void ) { cpuid = CPUID_NONE; }

	cpuid_t							cpuid;

	virtual const char *VPCALL		GetName( void ) const = 0;

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

	virtual	void VPCALL Dot( float *dst, const anVec3 &constant, const anVec3 *src, const int count ) = 0;
	virtual	void VPCALL Dot( float *dst, const anVec3 &constant, const anPlane *src, const int count ) = 0;
	virtual void VPCALL Dot( float *dst, const anVec3 &constant, const anDrawVertex *src, const int count ) = 0;
	virtual	void VPCALL Dot( float *dst, const anPlane &constant,const anVec3 *src, const int count ) = 0;
	virtual	void VPCALL Dot( float *dst, const anPlane &constant,const anPlane *src, const int count ) = 0;
	virtual void VPCALL Dot( float *dst, const anPlane &constant,const anDrawVertex *src, const int count ) = 0;
	virtual	void VPCALL Dot( float *dst, const anVec3 *src0, const anVec3 *src1, const int count ) = 0;
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
	virtual	void VPCALL MinMax( anVec2 &min, anVec2 &max, const anVec2 *src, const int count ) = 0;
	virtual	void VPCALL MinMax( anVec3 &min, anVec3 &max, const anVec3 *src, const int count ) = 0;
	virtual	void VPCALL MinMax( anVec3 &min, anVec3 &max, const anDrawVertex *src, const int count ) = 0;
	virtual	void VPCALL MinMax( anVec3 &min, anVec3 &max, const anDrawVertex *src, const int *indexes, const int count ) = 0;

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

	// anMatX operations
	virtual void VPCALL MatX_MultiplyVecX( anVecX &dst, const anMatX &mat, const anVecX &vec ) = 0;
	virtual void VPCALL MatX_MultiplyAddVecX( anVecX &dst, const anMatX &mat, const anVecX &vec ) = 0;
	virtual void VPCALL MatX_MultiplySubVecX( anVecX &dst, const anMatX &mat, const anVecX &vec ) = 0;
	virtual void VPCALL MatX_TransposeMultiplyVecX( anVecX &dst, const anMatX &mat, const anVecX &vec ) = 0;
	virtual void VPCALL MatX_TransposeMultiplyAddVecX( anVecX &dst, const anMatX &mat, const anVecX &vec ) = 0;
	virtual void VPCALL MatX_TransposeMultiplySubVecX( anVecX &dst, const anMatX &mat, const anVecX &vec ) = 0;
	virtual void VPCALL MatX_MultiplyMatX( anMatX &dst, const anMatX &m1, const anMatX &m2 ) = 0;
	virtual void VPCALL MatX_TransposeMultiplyMatX( anMatX &dst, const anMatX &m1, const anMatX &m2 ) = 0;
	virtual void VPCALL MatX_LowerTriangularSolve( const anMatX &L, float *x, const float *b, const int n, int skip = 0 ) = 0;
	virtual void VPCALL MatX_LowerTriangularSolveTranspose( const anMatX &L, float *x, const float *b, const int n ) = 0;
	virtual bool VPCALL MatX_LDLTFactor( anMatX &mat, anVecX &invDiag, const int n ) = 0;

	// rendering
	virtual void VPCALL BlendJoints( anJointQuat *joints, const anJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) = 0;
	virtual void VPCALL ConvertJointQuatsToJointMats( anJointMat *jointMats, const anJointQuat *jointQuats, const int numJoints ) = 0;
	virtual void VPCALL ConvertJointMatsToJointQuats( anJointQuat *jointQuats, const anJointMat *jointMats, const int numJoints ) = 0;
	virtual void VPCALL TransformJoints( anJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
	virtual void VPCALL UntransformJoints( anJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
	virtual void VPCALL TransformVerts( anDrawVertex *verts, const int numVerts, const anJointMat *joints, const anVec4 *weights, const int *index, const int numWeights ) = 0;
	virtual void VPCALL TracePointCull( byte *cullBits, byte &totalOr, const float radius, const anPlane *planes, const anDrawVertex *verts, const int numVerts ) = 0;
	virtual void VPCALL DecalPointCull( byte *cullBits, const anPlane *planes, const anDrawVertex *verts, const int numVerts ) = 0;
	virtual void VPCALL OverlayPointCull( byte *cullBits, anVec2 *texCoords, const anPlane *planes, const anDrawVertex *verts, const int numVerts ) = 0;
	virtual void VPCALL DeriveTriPlanes( anPlane *planes, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL DeriveTangents( anPlane *planes, anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL DeriveUnsmoothedTangents( anDrawVertex *verts, const dominantTri_s *dominantTris, const int numVerts ) = 0;
	virtual void VPCALL NormalizeTangents( anDrawVertex *verts, const int numVerts ) = 0;
	virtual void VPCALL CreateTextureSpaceLightVectors( anVec3 *lightVectors, const anVec3 &lightOrigin, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL CreateSpecularTextureCoords( anVec4 *texCoords, const anVec3 &lightOrigin, const anVec3 &viewOrigin, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual int  VPCALL CreateShadowCache( anVec4 *vertexCache, int *vertRemap, const anVec3 &lightOrigin, const anDrawVertex *verts, const int numVerts ) = 0;
	virtual int  VPCALL CreateVertexProgramShadowCache( anVec4 *vertexCache, const anDrawVertex *verts, const int numVerts ) = 0;

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
extern anSIMDProcessor *SIMDProcessor;

#endif

#ifndef __MATH_SIMD_SSE_H__
#define __MATH_SIMD_SSE_H__

/*
===============================================================================

	SSE implementation of arcSIMDProcessor

===============================================================================
*/

class idSIMD_SSE : public arcSIMD_MMX {
public:
#if defined(MACOS_X) && defined(__i386__)
	virtual const char *VPCALL GetName( void ) const;
	virtual void VPCALL Dot( float *dst, const anPlane &constant,const anDrawVertex *src, const int count );
	virtual	void VPCALL MinMax( anVec3 &min, anVec3 &max, const anDrawVertex *src, const int *indexes, const int count );
	virtual void VPCALL Dot( float *dst, const anVec3 &constant, const anPlane *src, const int count );

#elif defined(_WIN32)
	virtual const char *VPCALL GetName( void ) const;

	virtual void VPCALL Add( float *dst, const float constant, const float *src, const int count );
	virtual void VPCALL Add( float *dst, const float *src0, const float *src1, const int count );
	virtual void VPCALL Sub( float *dst, const float constant, const float *src, const int count );
	virtual void VPCALL Sub( float *dst, const float *src0, const float *src1, const int count );
	virtual void VPCALL Mul( float *dst, const float constant, const float *src, const int count );
	virtual void VPCALL Mul( float *dst, const float *src0, const float *src1, const int count );
	virtual void VPCALL Div( float *dst, const float constant, const float *src, const int count );
	virtual void VPCALL Div( float *dst, const float *src0, const float *src1, const int count );
	virtual void VPCALL MulAdd( float *dst, const float constant, const float *src, const int count );
	virtual void VPCALL MulAdd( float *dst, const float *src0, const float *src1, const int count );
	virtual void VPCALL MulSub( float *dst, const float constant, const float *src, const int count );
	virtual void VPCALL MulSub( float *dst, const float *src0, const float *src1, const int count );

	virtual void VPCALL Dot( float *dst, const anVec3 &constant, const anVec3 *src, const int count );
	virtual void VPCALL Dot( float *dst, const anVec3 &constant, const anPlane *src, const int count );
	virtual void VPCALL Dot( float *dst, const anVec3 &constant, const anDrawVertex *src, const int count );
	virtual void VPCALL Dot( float *dst, const anPlane &constant,const anVec3 *src, const int count );
	virtual void VPCALL Dot( float *dst, const anPlane &constant,const anPlane *src, const int count );
	virtual void VPCALL Dot( float *dst, const anPlane &constant,const anDrawVertex *src, const int count );
	virtual void VPCALL Dot( float *dst, const anVec3 *src0, const anVec3 *src1, const int count );
	virtual void VPCALL Dot( float &dot, const float *src1, const float *src2, const int count );

	virtual void VPCALL CmpGT( byte *dst, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpGT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpGE( byte *dst, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpGE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpLT( byte *dst, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpLT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpLE( byte *dst, const float *src0, const float constant, const int count );
	virtual void VPCALL CmpLE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count );

	virtual void VPCALL MinMax( float &min, float &max, const float *src, const int count );
	virtual	void VPCALL MinMax( anVec2 &min, anVec2 &max, const anVec2 *src, const int count );
	virtual void VPCALL MinMax( anVec3 &min, anVec3 &max, const anVec3 *src, const int count );
	virtual	void VPCALL MinMax( anVec3 &min, anVec3 &max, const anDrawVertex *src, const int count );
	virtual	void VPCALL MinMax( anVec3 &min, anVec3 &max, const anDrawVertex *src, const int *indexes, const int count );

	virtual void VPCALL Clamp( float *dst, const float *src, const float min, const float max, const int count );
	virtual void VPCALL ClampMin( float *dst, const float *src, const float min, const int count );
	virtual void VPCALL ClampMax( float *dst, const float *src, const float max, const int count );

	virtual void VPCALL Zero16( float *dst, const int count );
	virtual void VPCALL Negate16( float *dst, const int count );
	virtual void VPCALL Copy16( float *dst, const float *src, const int count );
	virtual void VPCALL Add16( float *dst, const float *src1, const float *src2, const int count );
	virtual void VPCALL Sub16( float *dst, const float *src1, const float *src2, const int count );
	virtual void VPCALL Mul16( float *dst, const float *src1, const float constant, const int count );
	virtual void VPCALL AddAssign16( float *dst, const float *src, const int count );
	virtual void VPCALL SubAssign16( float *dst, const float *src, const int count );
	virtual void VPCALL MulAssign16( float *dst, const float constant, const int count );

	virtual void VPCALL MatX_MultiplyVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
	virtual void VPCALL MatX_MultiplyAddVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
	virtual void VPCALL MatX_MultiplySubVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplyVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplyAddVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplySubVecX( anVecX &dst, const anMatX &mat, const anVecX &vec );
	virtual void VPCALL MatX_MultiplyMatX( anMatX &dst, const anMatX &m1, const anMatX &m2 );
	virtual void VPCALL MatX_TransposeMultiplyMatX( anMatX &dst, const anMatX &m1, const anMatX &m2 );
	virtual void VPCALL MatX_LowerTriangularSolve( const anMatX &L, float *x, const float *b, const int n, int skip = 0 );
	virtual void VPCALL MatX_LowerTriangularSolveTranspose( const anMatX &L, float *x, const float *b, const int n );
	virtual bool VPCALL MatX_LDLTFactor( anMatX &mat, anVecX &invDiag, const int n );

	virtual void VPCALL BlendJoints( anJointQuat *joints, const anJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL ConvertJointQuatsToJointMats( arcJointMat *jointMats, const anJointQuat *jointQuats, const int numJoints );
	virtual void VPCALL ConvertJointMatsToJointQuats( anJointQuat *jointQuats, const arcJointMat *jointMats, const int numJoints );
	virtual void VPCALL TransformJoints( arcJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL UntransformJoints( arcJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL TransformVerts( anDrawVertex *verts, const int numVerts, const arcJointMat *joints, const anVec4 *weights, const int *index, const int numWeights );
	virtual void VPCALL TracePointCull( byte *cullBits, byte &totalOr, const float radius, const anPlane *planes, const anDrawVertex *verts, const int numVerts );
	virtual void VPCALL DecalPointCull( byte *cullBits, const anPlane *planes, const anDrawVertex *verts, const int numVerts );
	virtual void VPCALL OverlayPointCull( byte *cullBits, anVec2 *texCoords, const anPlane *planes, const anDrawVertex *verts, const int numVerts );
	virtual void VPCALL DeriveTriPlanes( anPlane *planes, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void VPCALL DeriveTangents( anPlane *planes, anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void VPCALL DeriveUnsmoothedTangents( anDrawVertex *verts, const dominantTri_s *dominantTris, const int numVerts );
	virtual void VPCALL NormalizeTangents( anDrawVertex *verts, const int numVerts );
	virtual void VPCALL CreateTextureSpaceLightVectors( anVec3 *lightVectors, const anVec3 &lightOrigin, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void VPCALL CreateSpecularTextureCoords( anVec4 *texCoords, const anVec3 &lightOrigin, const anVec3 &viewOrigin, const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual int  VPCALL CreateShadowCache( anVec4 *vertexCache, int *vertRemap, const anVec3 &lightOrigin, const anDrawVertex *verts, const int numVerts );
	virtual int  VPCALL CreateVertexProgramShadowCache( anVec4 *vertexCache, const anDrawVertex *verts, const int numVerts );

	virtual void VPCALL UpSamplePCMTo44kHz( float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples );

#endif
};

#endif /* !__MATH_SIMD_SSE_H__ */

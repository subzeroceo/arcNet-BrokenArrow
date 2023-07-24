#ifndef __MATH_SIMD_GENERIC_H__
#define __MATH_SIMD_GENERIC_H__

/*
===============================================================================

	Generic implementation of arcSIMDProcessor

===============================================================================
*/

class arcSIMD_Generic : public arcSIMDProcessor {
public:
	virtual const char * VPCALL GetName( void ) const;

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

	virtual void VPCALL Dot( float *dst, const arcVec3 &constant, const arcVec3 *src, const int count );
	virtual void VPCALL Dot( float *dst, const arcVec3 &constant, const arcPlane *src, const int count );
	virtual void VPCALL Dot( float *dst, const arcVec3 &constant, const arcDrawVert *src, const int count );
	virtual void VPCALL Dot( float *dst, const arcPlane &constant,const arcVec3 *src, const int count );
	virtual void VPCALL Dot( float *dst, const arcPlane &constant,const arcPlane *src, const int count );
	virtual void VPCALL Dot( float *dst, const arcPlane &constant,const arcDrawVert *src, const int count );
	virtual void VPCALL Dot( float *dst, const arcVec3 *src0, const arcVec3 *src1, const int count );
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
	virtual	void VPCALL MinMax( arcVec2 &min, arcVec2 &max, const arcVec2 *src, const int count );
	virtual void VPCALL MinMax( arcVec3 &min, arcVec3 &max, const arcVec3 *src, const int count );
	virtual	void VPCALL MinMax( arcVec3 &min, arcVec3 &max, const arcDrawVert *src, const int count );
	virtual	void VPCALL MinMax( arcVec3 &min, arcVec3 &max, const arcDrawVert *src, const int *indexes, const int count );

	virtual void VPCALL Clamp( float *dst, const float *src, const float min, const float max, const int count );
	virtual void VPCALL ClampMin( float *dst, const float *src, const float min, const int count );
	virtual void VPCALL ClampMax( float *dst, const float *src, const float max, const int count );

	virtual void VPCALL Memcpy( void *dst, const void *src, const int count );
	virtual void VPCALL Memset( void *dst, const int val, const int count );

	virtual void VPCALL Zero16( float *dst, const int count );
	virtual void VPCALL Negate16( float *dst, const int count );
	virtual void VPCALL Copy16( float *dst, const float *src, const int count );
	virtual void VPCALL Add16( float *dst, const float *src1, const float *src2, const int count );
	virtual void VPCALL Sub16( float *dst, const float *src1, const float *src2, const int count );
	virtual void VPCALL Mul16( float *dst, const float *src1, const float constant, const int count );
	virtual void VPCALL AddAssign16( float *dst, const float *src, const int count );
	virtual void VPCALL SubAssign16( float *dst, const float *src, const int count );
	virtual void VPCALL MulAssign16( float *dst, const float constant, const int count );

	virtual void VPCALL MatX_MultiplyVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec );
	virtual void VPCALL MatX_MultiplyAddVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec );
	virtual void VPCALL MatX_MultiplySubVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplyVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplyAddVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplySubVecX( arcVecX &dst, const arcMatX &mat, const arcVecX &vec );
	virtual void VPCALL MatX_MultiplyMatX( arcMatX &dst, const arcMatX &m1, const arcMatX &m2 );
	virtual void VPCALL MatX_TransposeMultiplyMatX( arcMatX &dst, const arcMatX &m1, const arcMatX &m2 );
	virtual void VPCALL MatX_LowerTriangularSolve( const arcMatX &L, float *x, const float *b, const int n, int skip = 0 );
	virtual void VPCALL MatX_LowerTriangularSolveTranspose( const arcMatX &L, float *x, const float *b, const int n );
	virtual bool VPCALL MatX_LDLTFactor( arcMatX &mat, arcVecX &invDiag, const int n );

	virtual void VPCALL BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL ConvertJointQuatsToJointMats( arcJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints );
	virtual void VPCALL ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const arcJointMat *jointMats, const int numJoints );
	virtual void VPCALL TransformJoints( arcJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL UntransformJoints( arcJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL TransformVerts( arcDrawVert *verts, const int numVerts, const arcJointMat *joints, const arcVec4 *weights, const int *index, const int numWeights );
	virtual void VPCALL TracePointCull( byte *cullBits, byte &totalOr, const float radius, const arcPlane *planes, const arcDrawVert *verts, const int numVerts );
	virtual void VPCALL DecalPointCull( byte *cullBits, const arcPlane *planes, const arcDrawVert *verts, const int numVerts );
	virtual void VPCALL OverlayPointCull( byte *cullBits, arcVec2 *texCoords, const arcPlane *planes, const arcDrawVert *verts, const int numVerts );
	virtual void VPCALL DeriveTriPlanes( arcPlane *planes, const arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void VPCALL DeriveTangents( arcPlane *planes, arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void VPCALL DeriveUnsmoothedTangents( arcDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts );
	virtual void VPCALL NormalizeTangents( arcDrawVert *verts, const int numVerts );
	virtual void VPCALL CreateTextureSpaceLightVectors( arcVec3 *lightVectors, const arcVec3 &lightOrigin, const arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual void VPCALL CreateSpecularTextureCoords( arcVec4 *texCoords, const arcVec3 &lightOrigin, const arcVec3 &viewOrigin, const arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
	virtual int  VPCALL CreateShadowCache( arcVec4 *vertexCache, int *vertRemap, const arcVec3 &lightOrigin, const arcDrawVert *verts, const int numVerts );
	virtual int  VPCALL CreateVertexProgramShadowCache( arcVec4 *vertexCache, const arcDrawVert *verts, const int numVerts );

	virtual void VPCALL UpSamplePCMTo44kHz( float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples );
};

#endif /* !__MATH_SIMD_GENERIC_H__ */

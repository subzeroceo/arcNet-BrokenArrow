#ifndef __MATH_SIMD_SSE3_H__
#define __MATH_SIMD_SSE3_H__

/*
===============================================================================

	SSE3 implementation of arcSIMDProcessor

===============================================================================
*/

class arcSIMD_SSE3 : public idSIMD_SSE2 {
public:
#if defined(MACOS_X) && defined(__i386__)
	virtual const char *VPCALL GetName( void ) const;

#elif defined(_WIN32)
	virtual const char *VPCALL GetName( void ) const;

	virtual void VPCALL TransformVerts( anDrawVertex *verts, const int numVerts, const arcJointMat *joints, const anVec4 *weights, const int *index, const int numWeights );

#endif
};

#endif /* !__MATH_SIMD_SSE3_H__ */

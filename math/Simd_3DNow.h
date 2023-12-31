#ifndef __MATH_SIMD_3DNOW_H__
#define __MATH_SIMD_3DNOW_H__

/*
===============================================================================

	3DNow! implementation of arcSIMDProcessor

===============================================================================
*/

class idSIMD_3DNow : public arcSIMD_MMX {
#ifdef _WIN32
public:
	virtual const char *VPCALL GetName( void ) const;

	virtual void VPCALL Memcpy( void *dst, const void *src, const int count );

#endif
};

#endif /* !__MATH_SIMD_3DNOW_H__ */

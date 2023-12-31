#ifndef __MATH_SIMD_MMX_H__
#define __MATH_SIMD_MMX_H__

/*
===============================================================================

	MMX implementation of arcSIMDProcessor

===============================================================================
*/

class arcSIMD_MMX : public arcSIMD_Generic {
public:
#if defined(MACOS_X) && defined(__i386__)
	virtual const char *VPCALL GetName( void ) const;

#elif defined(_WIN32)
	virtual const char *VPCALL GetName( void ) const;

	virtual void VPCALL Memcpy( void *dst, const void *src, const int count );
	virtual void VPCALL Memset( void *dst, const int val, const int count );

#endif
};

#endif // !__MATH_SIMD_MMX_H__
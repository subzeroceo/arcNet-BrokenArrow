
#ifndef __COLORSPACE_H__
#define __COLORSPACE_H__

namespace anColorSpaces {
	void	ConvertRGBToYCoCg( byte *dst, const byte *src, int width, int height );
	void	ConvertYCoCgToRGB( byte *dst, const byte *src, int width, int height );

	void	ConvertRGBToCoCg_Y( byte *dst, const byte *src, int width, int height );
	void	ConvertCoCg_YToRGB( byte *dst, const byte *src, int width, int height );
	void	ConvertCoCgSYToRGB( byte *dst, const byte *src, int width, int height );

	void	ConvertRGBToYCoCg420( byte *dst, const byte *src, int width, int height );
	void	ConvertYCoCg420ToRGB( byte *dst, const byte *src, int width, int height );

	void	ConvertRGBToYCbCr( byte *dst, const byte *src, int width, int height );
	void	ConvertYCbCrToRGB( byte *dst, const byte *src, int width, int height );

	void	ConvertRGBToCbCr_Y( byte *dst, const byte *src, int width, int height );
	void	ConvertCbCr_YToRGB( byte *dst, const byte *src, int width, int height );

	void	ConvertRGBToYCbCr420( byte *dst, const byte *src, int width, int height );
	void	ConvertYCbCr420ToRGB( byte *dst, const byte *src, int width, int height );

	void	ConvertNormalMapToStereographicHeightMap( byte *heightMap, const byte *normalMap, int width, int height, float &scale );
	void	ConvertStereographicHeightMapToNormalMap( byte *normalMap, const byte *heightMap, int width, int height, float scale );

	void	ConvertRGBToMonochrome( byte *mono, const byte *rgb, int width, int height );
	void	ConvertMonochromeToRGB( byte *rgb, const byte *mono, int width, int height );
};

inline anColorRGB_t::ColorRGB_t() :
r( 255 ), g( 255 ), b( 255 ){}

inline anVec4 anColorRGB_t::ToVec4() const {
	return anVec4( r * ( 1.0f / 255.0f ), g * ( 1.0f / 255.0f ), b * ( 1.0f / 255.0f ), 1.0f );
}

inline anColorRGBA_t::anColorRGBA_t() :
a( 255 ){}

inline anVec4 anColorRGBA_t::ToVec4() const {
	return anVec4( r * ( 1.0f / 255.0f ), g * ( 1.0f / 255.0f ), b * ( 1.0f / 255.0f ), a * ( 1.0f / 255.0f ) );
}

#endif

#ifndef __COMPRESSOR_H__
#define __COMPRESSOR_H__

/*
===============================================================================

	idCompressor is a layer ontop of arcNetFile which provides lossless data
	compression. The compressor can be used as a regular file and multiple
	compressors can be stacked ontop of each other.

===============================================================================
*/

class idCompressor : public arcNetFile {
public:
							// compressor allocation
	static idCompressor *	AllocNoCompression();
	static idCompressor *	AllocBitStream();
	static idCompressor *	AllocRunLength();
	static idCompressor *	AllocRunLength_ZeroBased();
	static idCompressor *	AllocHuffman();
	static idCompressor *	AllocArithmetic();
	static idCompressor *	AllocLZSS();
	static idCompressor *	AllocLZSS_WordAligned();
	static idCompressor *	AllocLZW();

							// initialization
	virtual void			Init( arcNetFile *f, bool compress, int wordLength ) = 0;
	virtual void			FinishCompress() = 0;
	virtual float			GetCompressionRatio() const = 0;

							// common arcNetFile interface
	virtual const char *	GetName() = 0;
	virtual const char *	GetFullPath() = 0;
	virtual int				Read( void *outData, int outLength ) = 0;
	virtual int				Write( const void *inData, int inLength ) = 0;
	virtual int				Length() = 0;
	virtual ARC_TIME_T			Timestamp() = 0;
	virtual int				Tell() = 0;
	virtual void			ForceFlush() = 0;
	virtual void			Flush() = 0;
	virtual int				Seek( long offset, fsOrigin_t origin ) = 0;
};

#endif /* !__COMPRESSOR_H__ */

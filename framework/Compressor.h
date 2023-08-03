#ifndef __COMPRESSOR_H__
#define __COMPRESSOR_H__

/*
===============================================================================

	anCompressor is a layer ontop of anFile which provides lossless data
	compression. The compressor can be used as a regular file and multiple
	compressors can be stacked ontop of each other.

===============================================================================
*/

class anCompressor : public anFile {
public:
							// compressor allocation
	static anCompressor *	AllocNoCompression();
	static anCompressor *	AllocBitStream();
	static anCompressor *	AllocRunLength();
	static anCompressor *	AllocRunLength_ZeroBased();
	static anCompressor *	AllocHuffman();
	static anCompressor *	AllocArithmetic();
	static anCompressor *	AllocLZSS();
	static anCompressor *	AllocLZSS_WordAligned();
	static anCompressor *	AllocLZW();

							// initialization
	virtual void			Init( anFile *f, bool compress, int wordLength ) = 0;
	virtual void			FinishCompress() = 0;
	virtual float			GetCompressionRatio() const = 0;

							// common anFile interface
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

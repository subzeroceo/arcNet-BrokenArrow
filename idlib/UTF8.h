#ifndef __UTF8_H__
#define __UTF8_H__

class arcNetUTF8 {
public:
			arcNetUTF8( arcNetFile *file );
			arcNetUTF8( const byte *data, const int size );
			~arcNetUTF8( void );

	int		DecodeLength( void ) const;
	int		Decode( wchar_t* to ) const;

	static void	Encode( arcNetFile *file, const wchar_t *data, int len );

private:
	void	Init( void );
	void	Release( void );
	void	EnsureAlloced( int size );

	int		UTF8toUCS2( const byte *data, const int len, wchar_t *ucs2 ) const;

private:
	byte *	data;
	int		len;
	int		alloced;
};

ARC_INLINE arcNetUTF8::~arcNetUTF8( void ) {
	Release();
}

ARC_INLINE void arcNetUTF8::Init( void ) {
	len = 0;
	alloced = 0;
	data = NULL;
}

ARC_INLINE void arcNetUTF8::Release( void ) {
	delete [] data;
	Init();
}

ARC_INLINE void arcNetUTF8::EnsureAlloced( int size ) {
	if ( size > alloced ) {
		Release();
	}
	data = new byte[size];
	alloced = size;
}

#endif // !__UTF8_H__

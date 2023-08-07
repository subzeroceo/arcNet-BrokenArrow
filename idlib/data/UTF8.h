#ifndef __UTF8_H__
#define __UTF8_H__

class anUTF8 {
public:
			anUTF8( anFile *file );
			anUTF8( const byte *data, const int size );
			~anUTF8( void );

	int		DecodeLength( void ) const;
	int		Decode( wchar_t* to ) const;

	static void	Encode( anFile *file, const wchar_t *data, int len );

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

inline anUTF8::~anUTF8( void ) {
	Release();
}

inline void anUTF8::Init( void ) {
	len = 0;
	alloced = 0;
	data = nullptr;
}

inline void anUTF8::Release( void ) {
	delete [] data;
	Init();
}

inline void anUTF8::EnsureAlloced( int size ) {
	if ( size > alloced ) {
		Release();
	}
	data = new byte[size];
	alloced = size;
}

#endif // !__UTF8_H__

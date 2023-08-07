#ifndef __BASE64_H__
#define __BASE64_H__

/*
===============================================================================

	base64

===============================================================================
*/

#include "Lib.h"
class anEncodeBase64 {
public:
				anEncodeBase64( void );
				anEncodeBase64( const anStr &s );
				~anEncodeBase64( void );

	void		Encode( const byte *from, int size );
	void		Encode( const anStr &src );
	int			DecodeLength( void ) const; // minimum size in bytes of destination buffer for decoding
	int			Decode( byte *to ) const; // does not append a \0 - needs a DecodeLength() bytes buffer
	void		Decode( anStr &dest ) const; // decodes the binary content to an anStr (a bit dodgy, \0 and other non-ascii are possible in the decoded content)
	void		Decode( anFile *dest ) const;

	const char	*c_str() const;

	void 		operator=( const anStr &s );

private:
	byte *		data;
	int			len;
	int			alloced;

	void		Init( void );
	void		Release( void );
	void		EnsureAlloced( int size );
};

inline anEncodeBase64::anEncodeBase64( void ) {
	Init();
}

inline anEncodeBase64::anEncodeBase64( const anStr &s ) {
	Init();
	*this = s;
}

inline anEncodeBase64::~anEncodeBase64( void ) {
	Release();
}

inline const char *anEncodeBase64::c_str( void ) const {
	return (const char *)data;
}

inline void anEncodeBase64::Init( void ) {
	len = 0;
	alloced = 0;
	data = nullptr;
}

inline void anEncodeBase64::Release( void ) {
	if ( data ) {
		delete[] data;
	}
	Init();
}

inline void anEncodeBase64::EnsureAlloced( int size ) {
	if ( size > alloced ) {
		Release();
	}
	data = new byte[size];
	alloced = size;
}

inline void anEncodeBase64::operator=( const anStr &s ) {
	EnsureAlloced( s.Length()+1 ); // trailing \0 - beware, this does a Release
	strcpy( (char *)data, s.c_str() );
	len = s.Length();
}

#endif // !__BASE64_H__

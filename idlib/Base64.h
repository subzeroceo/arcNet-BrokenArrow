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
				anEncodeBase64( const anString &s );
				~anEncodeBase64( void );

	void		Encode( const byte *from, int size );
	void		Encode( const anString &src );
	int			DecodeLength( void ) const; // minimum size in bytes of destination buffer for decoding
	int			Decode( byte *to ) const; // does not append a \0 - needs a DecodeLength() bytes buffer
	void		Decode( anString &dest ) const; // decodes the binary content to an anString (a bit dodgy, \0 and other non-ascii are possible in the decoded content)
	void		Decode( anFile *dest ) const;

	const char	*c_str() const;

	void 		operator=( const anString &s );

private:
	byte *		data;
	int			len;
	int			alloced;

	void		Init( void );
	void		Release( void );
	void		EnsureAlloced( int size );
};

ARC_INLINE anEncodeBase64::anEncodeBase64( void ) {
	Init();
}

ARC_INLINE anEncodeBase64::anEncodeBase64( const anString &s ) {
	Init();
	*this = s;
}

ARC_INLINE anEncodeBase64::~anEncodeBase64( void ) {
	Release();
}

ARC_INLINE const char *anEncodeBase64::c_str( void ) const {
	return (const char *)data;
}

ARC_INLINE void anEncodeBase64::Init( void ) {
	len = 0;
	alloced = 0;
	data = nullptr;
}

ARC_INLINE void anEncodeBase64::Release( void ) {
	if ( data ) {
		delete[] data;
	}
	Init();
}

ARC_INLINE void anEncodeBase64::EnsureAlloced( int size ) {
	if ( size > alloced ) {
		Release();
	}
	data = new byte[size];
	alloced = size;
}

ARC_INLINE void anEncodeBase64::operator=( const anString &s ) {
	EnsureAlloced( s.Length()+1 ); // trailing \0 - beware, this does a Release
	strcpy( (char *)data, s.c_str() );
	len = s.Length();
}

#endif // !__BASE64_H__

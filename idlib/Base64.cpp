
#include "Lib.h"
#pragma hdrstop

/*
============
anEncodeBase64::Encode
============
*/
static const char sixtet_to_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void anEncodeBase64::Encode( const byte *from, int size ) {
	EnsureAlloced( 4*( size + 3 )/3 + 2 ); // ratio and padding + trailing \0
	byte *to = data;

	unsigned long w = 0;
	int i = 0;
	while ( size > 0 ) {
		w |= *from << i*8;
		++from;
		--size;
		++i;
		if ( size == 0 || i == 3 ) {
			byte out[4];
			SixtetsForInt( out, w );
			for ( int j = 0; j*6 < i*8; ++j ) {
				*to++ = sixtet_to_base64[ out[j] ];
			}
			if ( size == 0 ) {
				for ( j = i; j < 3; ++j ) {
					*to++ = '=';
				}
			}
			w = 0;
			i = 0;
		}
	}

	*to++ = '\0';
	len = to - data;
}

/*
============
anEncodeBase64::DecodeLength
returns the minimum size in bytes of the target buffer for decoding
4 base64 digits <-> 3 bytes
============
*/
int anEncodeBase64::DecodeLength( void ) const {
	return 3*len/4;
}

/*
============
anEncodeBase64::Decode
============
*/
int anEncodeBase64::Decode( byte *to ) const {
	static char base64_to_sixtet[256];
	static int tab_init = 0;
	byte *from = data;

	if ( !tab_init ) {
		memset( base64_to_sixtet, 0, 256 );
		for ( int i = 0; ( int j = sixtet_to_base64[i] ) != '\0'; ++i ) {
			base64_to_sixtet[j] = i;
		}
		tab_init = 1;
	}

	unsigned long w = 0;
	i = 0;
	size_t n = 0;
	byte in[4] = { 0, 0, 0, 0 };

	while ( *from != '\0' && *from != '=' ) {
		if ( *from == ' ' || *from == '\n' ) {
			++from;
			continue;
		}

		in[i] = base64_to_sixtet[* (unsigned char *) from];
		++i;
		++from;

		if ( *from == '\0' || *from == '=' || i == 4 ) {
			w = IntForSixtets( in );
			for ( int j = 0; j*8 < i*6; ++j ) {
				*to++ = w & 0xff;
				++n;
				w >>= 8;
			}
			i = 0;
			w = 0;
		}
	}
	return n;
}

/*
============
anEncodeBase64::Encode
============
*/
void anEncodeBase64::Encode( const anString &src ) {
	Encode( (const byte *)src.c_str(), src.Length() );
}

/*
============
anEncodeBase64::Decode
============
*/
void anEncodeBase64::Decode( anString &dest ) const {
	byte *buf = new byte[ DecodeLength()+1 ]; // +1 for trailing \0
	int out = Decode( buf );
	buf[out] = '\0';
	dest = (const char *)buf;
	delete[] buf;
}

/*
============
anEncodeBase64::Decode
============
*/
void anEncodeBase64::Decode( anFile *dest ) const {
	byte *buf = new byte[ DecodeLength()+1 ]; // +1 for trailing \0
	int out = Decode( buf );
	dest->Write( buf, out );
	delete[] buf;
}

#if 0
void idBase64_TestBase64() {
	anString src;
	anEncodeBase64 dest;
	src = "Encode me in base64";
	dest.Encode( src );
	anLibrary::common->Printf( "%s -> %s\n", src.c_str(), dest.c_str() );
	dest.Decode( src );
	anLibrary::common->Printf( "%s -> %s\n", dest.c_str(), src.c_str() );

	anDict src_dict;
	src_dict.SetFloat( "float", 0.5f);
	src_dict.SetBool( "bool", true );
	src_dict.Set( "value", "foo" );
	anFileMemory src_fmem( "serialize_dict" );
	src_dict.WriteToFileHandle( &src_fmem );
	dest.Encode( (const byte *)src_fmem.GetDataPtr(), src_fmem.Length() );
	anLibrary::common->Printf( "anDict encoded to %s\n", dest.c_str() );

	// now decode to another stream and build back
	anFileMemory dest_fmem( "build_back" );
	dest.Decode( &dest_fmem );
	dest_fmem.MakeReadOnly();
	anDict dest_dict;
	dest_dict.ReadFromFileHandle( &dest_fmem );
	anLibrary::common->Printf( "anDict reconstructed after base64 decode\n" );
	dest_dict.Print();

	// test anDict read from file - from python generated files, see anDict.py
	anFile *file = anLibrary::fileSystem->OpenFileRead( "anDict.test" );
	if ( file ) {
		anDict test_dict;
		test_dict.ReadFromFileHandle( file );

		anLibrary::common->Printf( "read anDict.test:\n" );
		test_dict.Print();
		anLibrary::fileSystem->CloseFile( file );
		file = nullptr;
	} else {
		anLibrary::common->Printf( "anDict.test not found\n" );
	}

	anEncodeBase64 base64_src;
	void *buffer;
	if ( anLibrary::fileSystem->ReadFile( "anDict.base64.test", &buffer ) != -1 ) {
		anFileMemory mem_src( "dict" );
		anLibrary::common->Printf( "read: %d %s\n", anString::Length( (char*)buffer ), buffer );
		base64_src = (char *)buffer;
		base64_src.Decode( &mem_src );
		mem_src.MakeReadOnly();
		anDict test_dict;
		test_dict.ReadFromFileHandle( &mem_src );
		anLibrary::common->Printf( "read anDict.base64.test:\n" );
		test_dict.Print();
		anLibrary::fileSystem->FreeFile( buffer );
	} else {
		anLibrary::common->Printf( "anDict.base64.test not found\n" );
	}
}

#endif

#include "precompiled.h"
#pragma hdrstop

/*
============
ARCEncodeBase64::Encode
============
*/
static const char sixtet_to_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void ARCEncodeBase64::Encode( const byte *from, int size ) {
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
ARCEncodeBase64::DecodeLength
returns the minimum size in bytes of the target buffer for decoding
4 base64 digits <-> 3 bytes
============
*/
int ARCEncodeBase64::DecodeLength( void ) const {
	return 3*len/4;
}

/*
============
ARCEncodeBase64::Decode
============
*/
int ARCEncodeBase64::Decode( byte *to ) const {
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
ARCEncodeBase64::Encode
============
*/
void ARCEncodeBase64::Encode( const arcNetString &src ) {
	Encode( (const byte *)src.c_str(), src.Length() );
}

/*
============
ARCEncodeBase64::Decode
============
*/
void ARCEncodeBase64::Decode( arcNetString &dest ) const {
	byte *buf = new byte[ DecodeLength()+1 ]; // +1 for trailing \0
	int out = Decode( buf );
	buf[out] = '\0';
	dest = (const char *)buf;
	delete[] buf;
}

/*
============
ARCEncodeBase64::Decode
============
*/
void ARCEncodeBase64::Decode( arcNetFile *dest ) const {
	byte *buf = new byte[ DecodeLength()+1 ]; // +1 for trailing \0
	int out = Decode( buf );
	dest->Write( buf, out );
	delete[] buf;
}

#if 0
void idBase64_TestBase64() {
	arcNetString src;
	ARCEncodeBase64 dest;
	src = "Encode me in base64";
	dest.Encode( src );
	arcLibrary::common->Printf( "%s -> %s\n", src.c_str(), dest.c_str() );
	dest.Decode( src );
	arcLibrary::common->Printf( "%s -> %s\n", dest.c_str(), src.c_str() );

	arcDictionary src_dict;
	src_dict.SetFloat( "float", 0.5f);
	src_dict.SetBool( "bool", true );
	src_dict.Set( "value", "foo" );
	aRcFileMemory src_fmem( "serialize_dict" );
	src_dict.WriteToFileHandle( &src_fmem );
	dest.Encode( (const byte *)src_fmem.GetDataPtr(), src_fmem.Length() );
	arcLibrary::common->Printf( "arcDictionary encoded to %s\n", dest.c_str() );

	// now decode to another stream and build back
	aRcFileMemory dest_fmem( "build_back" );
	dest.Decode( &dest_fmem );
	dest_fmem.MakeReadOnly();
	arcDictionary dest_dict;
	dest_dict.ReadFromFileHandle( &dest_fmem );
	arcLibrary::common->Printf( "arcDictionary reconstructed after base64 decode\n" );
	dest_dict.Print();

	// test arcDictionary read from file - from python generated files, see arcDictionary.py
	arcNetFile *file = arcLibrary::fileSystem->OpenFileRead( "arcDictionary.test" );
	if ( file ) {
		arcDictionary test_dict;
		test_dict.ReadFromFileHandle( file );

		arcLibrary::common->Printf( "read arcDictionary.test:\n" );
		test_dict.Print();
		arcLibrary::fileSystem->CloseFile( file );
		file = NULL;
	} else {
		arcLibrary::common->Printf( "arcDictionary.test not found\n" );
	}

	ARCEncodeBase64 base64_src;
	void *buffer;
	if ( arcLibrary::fileSystem->ReadFile( "arcDictionary.base64.test", &buffer ) != -1 ) {
		aRcFileMemory mem_src( "dict" );
		arcLibrary::common->Printf( "read: %d %s\n", arcNetString::Length( (char*)buffer ), buffer );
		base64_src = (char *)buffer;
		base64_src.Decode( &mem_src );
		mem_src.MakeReadOnly();
		arcDictionary test_dict;
		test_dict.ReadFromFileHandle( &mem_src );
		arcLibrary::common->Printf( "read arcDictionary.base64.test:\n" );
		test_dict.Print();
		arcLibrary::fileSystem->FreeFile( buffer );
	} else {
		arcLibrary::common->Printf( "arcDictionary.base64.test not found\n" );
	}
}

#endif
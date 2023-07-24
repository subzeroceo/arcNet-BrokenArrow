#include "precompiled.h"
#pragma hdrstop

#if defined( MACOS_X )
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/*
===============================================================================

	arcLibrary

===============================================================================
*/

arcSystem *	arcLibrary::sys		= NULL;
arcCommon *	arcLibrary::common		= NULL;
arcCVarSystem *	arcLibrary::cvarSystem	= NULL;
arcNetFile *		arcLibrary::fileSystem	= NULL;
int				arcLibrary::frameNumber = 0;

/*
================
arcLibrary::Init
================
*/
void arcLibrary::Init( void ) {
	assert( sizeof( bool ) == 1 );
	Swap_Init();			// initialize little/big endian conversion
	Mem_Init();				// initialize memory manager
	arcNetString::InitMemory();	// init string memory allocator
	arcSIMD::Init();			// initialize generic SIMD implementation
	arcMath::Init();		// initialize math
	arcMatX::Test();		// test arcMatX
	arcPolynomial::Test();	// test arcPolynomial
	arcDictionary::Init(); // initialize the dictionary string pools
}

/*
================
arcLibrary::ShutDown
================
*/
void arcLibrary::ShutDown( void ) {
	arcDictionary::Shutdown();	// shut down the dictionary string pools
	arcNetString::ShutdownMemory(); // shut down the string memory allocator
	arcSIMD::Shutdown();		// shut down the SIMD engine
	Mem_Shutdown();			// shut down the memory manager
}

/*
===============================================================================

	Colors

===============================================================================
*/

arcVec4	colorBlack	= arcVec4( 0.00f, 0.00f, 0.00f, 1.00f );
arcVec4	colorWhite	= arcVec4( 1.00f, 1.00f, 1.00f, 1.00f );
arcVec4	colorRed	= arcVec4( 1.00f, 0.00f, 0.00f, 1.00f );
arcVec4	colorGreen	= arcVec4( 0.00f, 1.00f, 0.00f, 1.00f );
arcVec4	colorBlue	= arcVec4( 0.00f, 0.00f, 1.00f, 1.00f );
arcVec4	colorYellow	= arcVec4( 1.00f, 1.00f, 0.00f, 1.00f );
arcVec4	colorMagenta= arcVec4( 1.00f, 0.00f, 1.00f, 1.00f );
arcVec4	colorCyan	= arcVec4( 0.00f, 1.00f, 1.00f, 1.00f );
arcVec4	colorOrange	= arcVec4( 1.00f, 0.50f, 0.00f, 1.00f );
arcVec4	colorPurple	= arcVec4( 0.60f, 0.00f, 0.60f, 1.00f );
arcVec4	colorPink	= arcVec4( 0.73f, 0.40f, 0.48f, 1.00f );
arcVec4	colorBrown	= arcVec4( 0.40f, 0.35f, 0.08f, 1.00f );
arcVec4	colorLtGrey	= arcVec4( 0.75f, 0.75f, 0.75f, 1.00f );
arcVec4	colorMdGrey	= arcVec4( 0.50f, 0.50f, 0.50f, 1.00f );
arcVec4	colorDkGrey	= arcVec4( 0.25f, 0.25f, 0.25f, 1.00f );
arcVec4 colorGold	= arcVec4(1.0f, 0.843f, 0.0f, 1.0f);
arcVec4	colorBrown2	= ARVec4(0.3f, 0.23f, 0.0f, 1.0f);
arcVec4	colorTrans	= ARVec4(0.0f, 0.0f, 0.0f, 0.0f);

static dword colorMask[2] = { 255, 0 };

/*
================
ColorFloatToByte
================
*/
ARC_INLINE static byte ColorFloatToByte( float c ) {
	return ( byte ) ( ( ( dword ) ( c * 255.0f ) ) & colorMask[FLOATSIGNBITSET( c )] );
}

/*
================
PackColor
================
*/
dword PackColor( const arcVec4 &color ) {
	dword dx = ColorFloatToByte( color.x );
	dword dy = ColorFloatToByte( color.y );
	dword dz = ColorFloatToByte( color.z );
	dword dw = ColorFloatToByte( color.w );

#if defined(_WIN32) || defined(__linux__) || (defined(MACOS_X) && defined(__i386__) )
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
#elif (defined(MACOS_X) && defined(__ppc__) )
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
#else
#error OS define is required!
#endif
}

/*
================
UnpackColor
================
*/
void UnpackColor( const dword color, arcVec4 &unpackedColor ) {
#if defined(_WIN32) || defined(__linux__) || (defined(MACOS_X) && defined(__i386__) )
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ) );
#elif (defined(MACOS_X) && defined(__ppc__) )
	unpackedColor.Set( ( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ) );
#else
#error OS define is required!
#endif
}

/*
================
PackColor
================
*/
dword PackColor( const arcVec3 &color ) {
	dword dx = ColorFloatToByte( color.x );
	dword dy = ColorFloatToByte( color.y );
	dword dz = ColorFloatToByte( color.z );

#if defined(_WIN32) || defined(__linux__) || (defined(MACOS_X) && defined(__i386__) )
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 );
#elif (defined(MACOS_X) && defined(__ppc__) )
	return ( dy << 16 ) | ( dz << 8 ) | ( dx << 0 );
#else
#error OS define is required!
#endif
}

/*
================
UnpackColor
================
*/
void UnpackColor( const dword color, arcVec3 &unpackedColor ) {
#if defined(_WIN32) || defined(__linux__) || (defined(MACOS_X) && defined(__i386__) )
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ) );
#elif (defined(MACOS_X) && defined(__ppc__) )
	unpackedColor.Set( ( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ) );
#else
#error OS define is required!
#endif
}

/*
===============
arcLibrary::Error
===============
*/
void arcLibrary::Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	arcNetString::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Error( "%s", text );
}

/*
===============
arcLibrary::Warning
===============
*/
void arcLibrary::Warning( const char *fmt, ... ) {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	arcNetString::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Warning( "%s", text );
}

/*
===============================================================================

	Byte order functions

===============================================================================
*/

// can't just use function pointers, or dll linkage can mess up
static short	(*_BigShort)( short l );
static short	(*_LittleShort)( short l );
static int		(*_BigLong)( int l );
static int		(*_LittleLong)( int l );
static float	(*_BigFloat)( float l );
static float	(*_LittleFloat)( float l );
static void		(*_BigRevBytes)( void *bp, int elsize, int elcount );
static void		(*_LittleRevBytes)( void *bp, int elsize, int elcount );
static void     (*_LittleBitField)( void *bp, int elsize );
static void		(*_SixtetsForInt)( byte *out, int src );
static int		(*_IntForSixtets)( byte *in );

short	BigShort( short l ) { return _BigShort( l ); }
short	LittleShort( short l ) { return _LittleShort( l ); }
int		BigLong( int l ) { return _BigLong( l ); }
int		LittleLong( int l ) { return _LittleLong( l ); }
float	BigFloat( float l ) { return _BigFloat( l ); }
float	LittleFloat( float l ) { return _LittleFloat( l ); }
void	BigRevBytes( void *bp, int elsize, int elcount ) { _BigRevBytes( bp, elsize, elcount ); }
void	LittleRevBytes( void *bp, int elsize, int elcount ){ _LittleRevBytes( bp, elsize, elcount ); }
void	LittleBitField( void *bp, int elsize ){ _LittleBitField( bp, elsize ); }

void	SixtetsForInt( byte *out, int src) { _SixtetsForInt( out, src ); }
int		IntForSixtets( byte *in ) { return _IntForSixtets( in ); }

/*
================
ShortSwap
================
*/
short ShortSwap( short l ) {
	byte b1 = l&255;
	byte b2 = ( 1 >> 8 )&255;
	return ( b1<<8 ) + b2;
}

/*
================
ShortNoSwap
================
*/
short ShortNoSwap( short l ) {
	return l;
}

/*
================
LongSwap
================
*/
int LongSwap( int l ) {
	byte b1 = l&255;
	byte b2 = ( 1 >> 8 )&255;
	byte b3 = ( l >> 16 )&255;
	byte b4 = ( l >> 24 )&255;

	return ( ( int )b1<<24 ) + ( ( int )b2<<16 ) + ( ( int )b3<<8 ) + b4;
}

/*
================
LongNoSwap
================
*/
int	LongNoSwap( int l ) {
	return l;
}

/*
================
FloatSwap
================
*/
float FloatSwap( float f ) {
	union {
		float	f;
		byte	b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

/*
================
FloatNoSwap
================
*/
float FloatNoSwap( float f ) {
	return f;
}

/*
=====================================================================
RevBytesSwap

Reverses byte order in place.

INPUTS
   bp       bytes to reverse
   elsize   size of the underlying data type
   elcount  number of elements to swap

RESULTS
   Reverses the byte order in each of elcount elements.
===================================================================== */
void RevBytesSwap( void *bp, int elsize, int elcount ) {
	register unsigned char *p = ( unsigned char * ) bp;

	if ( elsize == 2 ) {
		register unsigned char *q = p + 1;
		while ( elcount-- ) {
			*p ^= *q;
			*q ^= *p;
			*p ^= *q;
			p += 2;
			q += 2;
		}
		return;
	}

	while ( elcount-- ) {
		q = p + elsize - 1;
		while ( p < q ) {
			*p ^= *q;
			*q ^= *p;
			*p ^= *q;
			++p;
			--q;
		}
		p += elsize >> 1;
	}
}

/*
 =====================================================================
 RevBytesSwap

 Reverses byte order in place, then reverses bits in those bytes

 INPUTS
 bp       bitfield structure to reverse
 elsize   size of the underlying data type

 RESULTS
 Reverses the bitfield of size elsize.
 ===================================================================== */
void RevBitFieldSwap( void *bp, int elsize ) {
	LittleRevBytes( bp, elsize, 1 );

	//for ( int i = 0; i < elsize; i += 8 ) {
	unsigned char *p = (unsigned char *) bp;

	while ( elsize-- ) {
		unsigned char *v = *p;
		unsigned char *t = 0;
		for ( int i = 7; i; i-- ) {
			t <<= 1;
			v >>= 1;
			t |= v & 1;
		}
		*p++ = t;
	}
}

/*
================
RevBytesNoSwap
================
*/
void RevBytesNoSwap( void *bp, int elsize, int elcount ) {
	return;
}

/*
 ================
 RevBytesNoSwap
 ================
 */
void RevBitFieldNoSwap( void *bp, int elsize ) {
	return;
}

/*
================
SixtetsForIntLittle
================
*/
void SixtetsForIntLittle( byte *out, int src) {
	byte *b = ( byte * )&src;
	out[0] = ( b[0] & 0xfc ) >> 2;
	out[1] = ( ( b[0] & 0x3 ) << 4 ) + ( ( b[1] & 0xf0 ) >> 4 );
	out[2] = ( ( b[1] & 0xf ) << 2 ) + ( ( b[2] & 0xc0 ) >> 6 );
	out[3] = b[2] & 0x3f;
}

/*
================
SixtetsForIntBig
TTimo: untested - that's the version from initial base64 encode
================
*/
void SixtetsForIntBig( byte *out, int src) {
	for ( int i = 0; i < 4; i++ ) {
		out[i] = src & 0x3f;
		src >>= 6;
	}
}

/*
================
IntForSixtetsLittle
================
*/
int IntForSixtetsLittle( byte *in ) {
	int ret = 0;
	byte *b = ( byte * )&ret;
	b[0] |= in[0] << 2;
	b[0] |= ( in[1] & 0x30 ) >> 4;
	b[1] |= ( in[1] & 0xf ) << 4;
	b[1] |= ( in[2] & 0x3c ) >> 2;
	b[2] |= ( in[2] & 0x3 ) << 6;
	b[2] |= in[3];
	return ret;
}

/*
================
IntForSixtetsBig
TTimo: untested - that's the version from initial base64 decode
================
*/
int IntForSixtetsBig( byte *in ) {
	int ret = 0;
	ret |= in[0];
	ret |= in[1] << 6;
	ret |= in[2] << 2*6;
	ret |= in[3] << 3*6;
	return ret;
}

/*
================
Swap_Init
================
*/
void Swap_Init( void ) {
	byte	swaptest[2] = {1,0};

	// set the byte swapping variables in a portable manner
	if ( *(short *)swaptest == 1 ) {
		// little endian ex: x86
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
		_BigRevBytes = RevBytesSwap;
		_LittleRevBytes = RevBytesNoSwap;
		_LittleBitField = RevBitFieldNoSwap;
		_SixtetsForInt = SixtetsForIntLittle;
		_IntForSixtets = IntForSixtetsLittle;
	} else {
		// big endian ex: ppc
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
		_BigRevBytes = RevBytesNoSwap;
		_LittleRevBytes = RevBytesSwap;
		_LittleBitField = RevBitFieldSwap;
		_SixtetsForInt = SixtetsForIntBig;
		_IntForSixtets = IntForSixtetsBig;
	}
}

/*
==========
Swap_IsBigEndian
==========
*/
bool Swap_IsBigEndian( void ) {
	byte	swaptest[2] = {1,0};
	return *(short *)swaptest != 1;
}

/*
===============================================================================

	Assertion

===============================================================================
*/

void AssertFailed( const char *file, int line, const char *expression ) {
	arcLibrary::sys->DebugPrintf( "\n\nASSERTION FAILED!\n%s(%d): '%s'\n", file, line, expression );
#ifdef _WIN32
	__asm int 0x03
#elif defined( __linux__ )
	__asm__ __volatile__ ( "int $0x03" );
#elif defined( MACOS_X )
	kill( getpid(), SIGINT );
#endif
}

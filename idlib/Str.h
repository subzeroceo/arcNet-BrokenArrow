#ifndef __STR_H__
#define __STR_H__

/*
===============================================================================

	Character string

===============================================================================
*/

// these library functions should not be used for cross platform compatibility
#include <cstddef>
#define strcmp			anStr::Cmp		// use_anStr_Cmp
#define strncmp			use_anStr_Cmpn

#if defined( StrCmpN )
#undef StrCmpN
#endif
#define StrCmpN			use_anStr_Cmpn

#if defined( strcmpi )
#undef strcmpi
#endif
#define strcmpi			use_anStr_Icmp

#if defined( StrCmpI )
#undef StrCmpI
#endif
#define StrCmpI			use_anStr_Icmp

#if defined( StrCmpNI )
#undef StrCmpNI
#endif
#define StrCmpNI		use_anStr_Icmpn

#define stricmp			anStr::Icmp		// use_anStr_Icmp
#define _stricmp		use_anStr_Icmp
#define strcasecmp		use_anStr_Icmp
#define strnicmp		use_anStr_Icmpn
#define _strnicmp		use_anStr_Icmpn
#define _memicmp		use_anStr_Icmpn
#define snprintf		use_anStr_snPrintf
#define _snprintf		use_anStr_snPrintf
#define vsnprintf		use_anStr_vsnPrintf
#define _vsnprintf		use_anStr_vsnPrintf

class anVec4;

#ifndef FILE_HASH_SIZE
#define FILE_HASH_SIZE		1024
#endif

// color escape character
const int C_COLOR_ESCAPE			= '^';
const int C_COLOR_DEFAULT			= '0';
const int C_COLOR_RED				= '1';
const int C_COLOR_GREEN				= '2';
const int C_COLOR_YELLOW			= '3';
const int C_COLOR_BLUE				= '4';
const int C_COLOR_CYAN				= '5';
const int C_COLOR_MAGENTA			= '6';
const int C_COLOR_WHITE				= '7';
const int C_COLOR_GRAY				= '8';
const int C_COLOR_BLACK				= '9';
const int C_COLOR_LTGREY			= ':';
const int C_COLOR_MDGREEN			= '<';
const int C_COLOR_MDYELLOW			= '=';
const int C_COLOR_MDBLUE			= '>';
const int C_COLOR_MDRED				= '?';
const int C_COLOR_LTORANGE			= 'A';
const int C_COLOR_MDCYAN			= 'B';
const int C_COLOR_MDPURPLE			= 'C';
const int C_COLOR_ORANGE			= 'D';

// color escape string
#define S_COLOR_DEFAULT				"^0"
#define S_COLOR_RED					"^1"
#define S_COLOR_GREEN				"^2"
#define S_COLOR_YELLOW				"^3"
#define S_COLOR_BLUE				"^4"
#define S_COLOR_CYAN				"^5"
#define S_COLOR_MAGENTA				"^6"
#define S_COLOR_WHITE				"^7"
#define S_COLOR_GRAY				"^8"
#define S_COLOR_BLACK				"^9"
#define S_COLOR_LTGREY				"^:"
#define S_COLOR_MDGREEN				"^<"
#define S_COLOR_MDYELLOW			"^="
#define S_COLOR_MDBLUE				"^>"
#define S_COLOR_MDRED				"^?"
#define S_COLOR_LTORANGE			"^A"
#define S_COLOR_MDCYAN				"^B"
#define S_COLOR_MDPURPLE			"^C"
#define S_COLOR_ORANGE				"^D"

// make anStr a multiple of 16 bytes long
// don't make too large to keep memory requirements to a minimum
const int STR_ALLOC_BASE			= 20;
const int STR_ALLOC_GRAN			= 32;

typedef enum {
	MEASURE_SIZE = 0,
	MEASURE_BANDWIDTH
} Measure_t;

class anStr {
public:
	struct TimeFormat {
		TimeFormat() : showMinutes( false ), showHours( false ), showSeconds( true ) {}
		bool showMinutes;
		bool showHours;
		bool showSeconds;
	};
						anStr( void );
						anStr( const anStr &text );
						anStr( const anStr &text, int start, int end );
						anStr( const char *text );
						anStr( const char *text, int start, int end );
						explicit anStr( const bool b );
						explicit anStr( const char c );
						explicit anStr( const int i );
						explicit anStr( const unsigned u );
						explicit anStr( const float f );
						~anStr( void );

	size_t				Size( void ) const;
	const char *		c_str( void ) const;
	operator			const char*( void ) const;
	operator			const char*( void );

	char				operator[]( int index ) const;
	char &				operator[]( int index );

	void				operator=( const anStr &text );
	void				operator=( const char *text );

	friend anStr		operator+( const anStr &a, const anStr &b );
	friend anStr		operator+( const anStr &a, const char *b );
	friend anStr		operator+( const char *a, const anStr &b );

	friend anStr		operator+( const anStr &a, const float b );
	friend anStr		operator+( const anStr &a, const int b );
	friend anStr		operator+( const anStr &a, const unsigned b );
	friend anStr		operator+( const anStr &a, const bool b );
	friend anStr		operator+( const anStr &a, const char b );

	anStr &			operator+=( const anStr &a );
	anStr &			operator+=( const char *a );
	anStr &			operator+=( const float a );
	anStr &			operator+=( const char a );
	anStr &			operator+=( const int a );
	anStr &			operator+=( const unsigned a );
	anStr &			operator+=( const bool a );

						// case sensitive compare
	friend bool			operator==( const anStr &a, const anStr &b );
	friend bool			operator==( const anStr &a, const char *b );
	friend bool			operator==( const char *a, const anStr &b );

						// case sensitive compare
	friend bool			operator!=( const anStr &a, const anStr &b );
	friend bool			operator!=( const anStr &a, const char *b );
	friend bool			operator!=( const char *a, const anStr &b );

						// case sensitive compare
	int					Cmp( const char *text ) const;
	int					Cmpn( const char *text, int n ) const;
	int					CmpPrefix( const char *text ) const;

						// case insensitive compare
	int					Icmp( const char *text ) const;
	int					Icmpn( const char *text, int n ) const;
	int					IcmpPrefix( const char *text ) const;

						// case insensitive compare ignoring color
	int					IcmpNoColor( const char *text ) const;

						// compares paths and makes sure folders come first
	int					IcmpPath( const char *text ) const;
	int					IcmpnPath( const char *text, int n ) const;
	int					IcmpPrefixPath( const char *text ) const;

	int					Length( void ) const;
	int					Allocated( void ) const;
	void				Empty( void );
	bool				IsEmpty( void ) const;
	void				Clear( void );
	void				Append( const char a );
	void				Append( const anStr &text );
	void				Append( const char *text );
	void				Append( int count, const char c );

	void				Append( const char *text, int len );
	void				Insert( const char a, int index );
	void				Insert( const char *text, int index );
	void				ToLower( void );
	void				ToUpper( void );
	bool				IsNumeric( void ) const;

	void				Swap( anStr& rhs );

	bool				IsColor( void ) const;
	bool				IsHexColor( void ) const;
	bool				HasHexColorAlpha( void ) const;
	int					LengthWithoutColors( void ) const;
	anStr &				RemoveColors( void );

	bool				HasLower( void ) const;
	bool				HasUpper( void ) const;

	void				CapLength( int );
	void				Fill( const char ch, int newlen );

	int					Find( const char c, int start = 0, int end = -1 ) const;
	int					Find( const char *text, bool casesensitive = true, int start = 0, int end = -1 ) const;
	const char *		FindString( const char *text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION ) const;
	int					CountChar( const char c );
	bool				Filter( const char *filter, bool casesensitive ) const;

	int					Last( const char c, int index = INVALID_POSITION ) const;
	int					Last( const char c ) const;						// return the index to the last occurance of 'c', returns -1 if not found
	const char *		Left( int len, anStr &result ) const;		// store the leftmost 'len' characters in the result
	anStr				Left( int len ) const;							// return the leftmost 'len' characters

	const char *		Right( int len, anStr &result ) const;		// store the rightmost 'len' characters in the result
	const char *		Mid( int start, int len, anStr &result ) const;// store 'len' characters starting at 'start' in result

	anStr				Right( int len ) const;							// return the rightmost 'len' characters
	anStr				Mid( int start, int len ) const;				// return 'len' characters starting at 'start'
	void				StripLeading( const char c );					// strip char from front as many times as the char occurs
	void				StripLeading( const char *string );				// strip string from front as many times as the string occurs
	bool				StripLeadingOnce( const char *string );			// strip string from front just once if it occurs
	void				StripTrailing( const char c );					// strip char from end as many times as the char occurs
	void				StripTrailing( const char *string );			// strip string from end as many times as the string occurs
	bool				StripTrailingOnce( const char *string );		// strip string from end just once if it occurs
	void				Strip( const char c );							// strip char from front and end as many times as the char occurs
	void				Strip( const char *string );					// strip string from front and end as many times as the string occurs
	void				StripLeadingWhiteSpace( void );					// strip leading white space characters
	void				StripTrailingWhitespace( void );				// strip trailing white space characters
	anStr &				StripQuotes( void );							// strip quotes around string
	void				Replace( const char *old, const char *nw );

	void				ReplaceFirst( const char *old, const char *nw );
	void				ReplaceChar( char oldChar, char newChar );

	void				EraseRange( int start, int len = INVALID_POSITION );
	void				EraseChar( const char c, int start = 0 );

	// file name methods
	int					FileNameHash( void ) const;						// hash key for the filename ( skips extension)
	anStr &				CollapsePath( void );							// where possible removes /../ and /./ from path

	anStr &				BackSlashesToSlashes( void );					// convert slashes
	anStr &				SlashesToBackSlashes( void );					// convert slashes

	anStr &				SetFileExtension( const char *extension );		// set the given file extension
	anStr &				StripFileExtension( void );						// remove any file extension
	anStr &				StripAbsoluteFileExtension( void );				// remove any file extension looking from front (useful if there are multiple .'s)
	anStr &				DefaultFileExtension( const char *extension );	// if there's no file extension use the default
	anStr &				DefaultPath( const char *basepath );			// if there's no path use the default
	void				AppendPath( const char *text );					// append a partial path
	anStr &				StripFilename( void );							// remove the filename from a path
	anStr &				StripPath( void );								// remove the path from the filename
	void				ExtractFilePath( anStr &dest ) const;			// copy the file path to another string
	void				ExtractFileName( anStr &dest ) const;			// copy the filename to another string
	void				ExtractFileBase( anStr &dest ) const;			// copy the filename minus the extension to another string
	void				ExtractFileExtension( anStr &dest ) const;		// copy the file extension to another string
	bool				CheckExtension( const char *ext );

	anStr &				StripComments();								// remove C++ and C style comments
	anStr & 			Indent();										// indents brace-delimited text, preserving tabs in the middle of lines
	anStr & 			Unindent();										// unindents brace-delimited text, preserving tabs in the middle of lines
	anStr &				CleanFilename( void );							// strips bad characters
	bool				IsValidEmailAddress( void );
	anStr &				CollapseColors( void );							// removes redundant color codes

	// char * methods to replace library functions
	static int			Length( const char *s );
	static char *		ToLower( char *s );
	static char *		ToUpper( char *s );
	static bool			IsNumeric( const char *s );

	static bool			IsColor( const char *s );
	static bool			IsHexColor( const char *s );
	static bool			HasHexColorAlpha( const char *s );
	static bool			HasLower( const char *s );
	static bool			HasUpper( const char *s );
	static int			LengthWithoutColors( const char *s );
	static char *		RemoveColors( char *s );

	static bool			IsBadFilenameChar( char c );
	static char *		CleanFilename( char *s );
	static char *		StripFilename( char *s );
	static char *		StripPath( char *s );

	static int			Cmp( const char *s1, const char *s2 );
	static int			Cmpn( const char *s1, const char *s2, int n );
	static int			Icmp( const char *s1, const char *s2 );
	static int			Icmpn( const char *s1, const char *s2, int n );
	static int			IcmpNoColor( const char *s1, const char *s2 );
	static int			IcmpPath( const char *s1, const char *s2 );			// compares paths and makes sure folders come first
	static int			IcmpnPath( const char *s1, const char *s2, int n );	// compares paths and makes sure folders come first
	static void			Append( char *dest, int size, const char *src );
	static void			Copynz( char *dest, const char *src, int destsize );
	static int			snPrintf( char *dest, int size, const char *fmt, ... ) an_attribute( ( format( printf, 3, 4 ) ) );
	static int			vsnPrintf( char *dest, int size, const char *fmt, va_list argptr );

	static int			CountChar( const char *str, const char c );
	static int			FindChar( const char *str, const char c, int start = 0, int end = -1 );
	static int			FindText( const char *str, const char *text, bool casesensitive = true, int start = 0, int end = -1 );
	static bool			Filter( const char *filter, const char *name, bool casesensitive );
	static void			StripMediaName( const char *name, anStr &mediaName );
	static bool			CheckExtension( const char *name, const char *ext );

	static const char *	FloatArrayToString( const float *array, const int length, const int precision );
	static int			NumLonelyLF( const char *src );	// return the number of line feeds not paired with a carriage return... usefull for getting a correct destination buffer size for ToCRLF
	static bool			ToCRLF( const char *src, char *dest, int maxLength );
	static const char *	CStyleQuote( const char *str );
	static const char *	CStyleUnQuote( const char *str );
	static void			IndentAndPad( int indent, int pad, anStr &str, const char *fmt, ... );  // indent and pad out formatted text

	static void			StringToBinaryString( anStr &out, void *pv, int size);
	static bool			BinaryStringToString( const char *str,  void *pv, int size );

    static bool			IsValidEmailAddress( const char *address );

	static const char *	MS2HMS( double ms, const TimeFormat &formatSpec = defaultHMSFormat );
	static const char *	FormatInt( const int num );	 // formats an integer as a value with commas

	// hash keys
	static int			Hash( const char *string );
	static int			Hash( const char *string, int length );
	static int			IHash( const char *string );					// case insensitive
	static int			IHash( const char *string, int length );		// case insensitive

	// character methods
	static char			ToLower( char c );
	static char			ToUpper( char c );
	static bool			CharIsPrintable( int c );
	static bool			CharIsLower( int c );
	static bool			CharIsUpper( int c );
	static bool			CharIsAlpha( int c );
	static bool			CharIsNumeric( int c );
	static bool			CharIsNewLine( char c );
	static bool			CharIsTab( char c );
	static bool			CharIsHex( int c );
	static int			ColorIndex( int c );
	static const anVec4&ColorForIndex( int i );
	static const anVec4&ColorForChar( int c );
	static const char*	StrForColorIndex( int i );
	static int			HexForChar( int c );

	friend int			sprintf( anStr &dest, const char *fmt, ... );
	friend int			vsprintf( anStr &dest, const char *fmt, va_list ap );

	void				ReAllocate( int amount, bool keepOld );				// reallocate string data buffer
	void				FreeData( void );									// free allocated string memory
	void				SetStaticBuffer( char *buffer, int l );

						// format value in the given measurement with the best unit, returns the best unit
	int					BestUnit( const char *format, float value, Measure_t measure );
						// format value in the requested unit and measurement
	void				SetUnit( const char *format, float value, int unit, Measure_t measure );

	static void			InitMemory( void );
	static void			ShutdownMemory( void );
	static void			PurgeMemory( void );
	static void			ShowMemoryUsage_f( const anCommandArgs &args );

	int					DynamicMemoryUsed() const;
	static anStr		FormatNumber( int number );

	static void			SetStringAllocator( stringDataAllocator_t *allocator );
	static stringDataAllocator_t *GetStringAllocator( void );
	static void			Test( void );

protected:
	int					len;
	char *				data;
	int					alloced;
	char				baseBuffer[ STR_ALLOC_BASE ];

	void				Init( void );										// initialize string using base buffer
	void				EnsureAlloced( int amount, bool keepold = true );	// ensure string data buffer is large anough

	// FIXME: Finished this!
	void 				EnsureDataWritable( void );

	static				stringDataAllocator_t *	stringDataAllocator;
	static				bool					stringAllocatorIsShared;
	static TimeFormat	defaultHMSFormat;
};

char *					va( const char *fmt, ... ) an_attribute( ( format( printf, 1, 2 )  ) );


inline void anStr::EnsureAlloced( int amount, bool keepold ) {
	if ( amount > alloced ) {
		ReAllocate( amount, keepold );
	}
}

inline void anStr::Init( void ) {
	len = 0;
	alloced = STR_ALLOC_BASE;
	data = baseBuffer;
	data[0] = '\0';
#ifdef ARC_DEBUG_UNINITIALIZED_MEMORY
	memset( baseBuffer, 0, sizeof( baseBuffer ) );
#endif
}

inline anStr::anStr( void ) {
	Init();
}

inline anStr::anStr( const anStr &text ) {
	Init();
	int l = text.Length();
	EnsureAlloced( l + 1 );
	strcpy( data, text.data );
	len = l;
}

inline anStr::anStr( const anStr &text, int start, int end ) {
	Init();
	if ( end > text.Length() ) {
		end = text.Length();
	}
	if ( start > text.Length() ) {
		start = text.Length();
	} else if ( start < 0 ) {
		start = 0;
	}

	int l = end - start;
	if ( l < 0 ) {
		l = 0;
	}

	EnsureAlloced( l + 1 );

	for ( int i = 0; i < l; i++ ) {
		data[i] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

inline anStr::anStr( const char *text ) {
	Init();
	if ( text ) {
		int l = strlen( text );
		EnsureAlloced( l + 1 );
		strcpy( data, text );
		len = l;
	}
}

inline anStr::anStr( const char *text, int start, int end ) {
	int l = strlen( text );

	Init();
	if ( end > l ) {
		end = l;
	}
	if ( start > l ) {
		start = l;
	} else if ( start < 0 ) {
		start = 0;
	}

	l = end - start;
	if ( l < 0 ) {
		l = 0;
	}

	EnsureAlloced( l + 1 );

	for ( int i = 0; i < l; i++ ) {
		data[i] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

inline anStr::anStr( const bool b ) {
	Init();
	EnsureAlloced( 2 );
	data[0] = b ? '1' : '0';
	data[1] = '\0';
	len = 1;
}

inline anStr::anStr( const char c ) {
	Init();
	EnsureAlloced( 2 );
	data[0] = c;
	data[1] = '\0';
	len = 1;
}

inline anStr::anStr( const int i ) {
	char text[ 64 ];

	Init();
	int l = sprintf( text, "%d", i );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

inline anStr::anStr( const unsigned u ) {
	char text[ 64 ];
	Init();
	int l = sprintf( text, "%u", u );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

inline anStr::anStr( const float f ) {
	char text[ 64 ];
	Init();
	int l = anStr::snPrintf( text, sizeof( text ), "%f", f );
	while ( l > 0 && text[l-1] == '0' ) text[--l] = '\0';
	while ( l > 0 && text[l-1] == '.' ) text[--l] = '\0';
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

inline anStr::~anStr( void ) {
	FreeData();
}

inline size_t anStr::Size( void ) const {
	return sizeof( *this ) + Allocated();
}

inline const char *anStr::c_str( void ) const {
	return data;
}

inline anStr::operator const char *( void ) {
	return c_str();
}

inline anStr::operator const char *( void ) const {
	return c_str();
}

inline char anStr::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[index];
}

inline char &anStr::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[index];
}

inline void anStr::operator=( const anStr &text ) {
	int l = text.Length();
	EnsureAlloced( l + 1, false );
	memcpy( data, text.data, l );
	data[l] = '\0';
	len = l;
}

inline anStr operator+( const anStr &a, const anStr &b ) {
	anStr result( a );
	result.Append( b );
	return result;
}

inline anStr operator+( const anStr &a, const char *b ) {
	anStr result( a );
	result.Append( b );
	return result;
}

inline anStr operator+( const char *a, const anStr &b ) {
	anStr result( a );
	result.Append( b );
	return result;
}

inline anStr operator+( const anStr &a, const bool b ) {
	anStr result( a );
	result.Append( b ? "true" : "false" );
	return result;
}

inline anStr operator+( const anStr &a, const char b ) {
	anStr result( a );
	result.Append( b );
	return result;
}

inline anStr operator+( const anStr &a, const float b ) {
	char	text[ 64 ];
	anStr	result( a );

	sprintf( text, "%f", b );
	result.Append( text );

	return result;
}

inline anStr operator+( const anStr &a, const int b ) {
	char	text[ 64 ];
	anStr	result( a );

	sprintf( text, "%d", b );
	result.Append( text );

	return result;
}

inline anStr operator+( const anStr &a, const unsigned b ) {
	char	text[ 64 ];
	anStr	result( a );

	sprintf( text, "%u", b );
	result.Append( text );

	return result;
}

inline anStr &anStr::operator+=( const float a ) {
	char text[ 64 ];

	sprintf( text, "%f", a );
	Append( text );

	return *this;
}

inline anStr &anStr::operator+=( const int a ) {
	char text[ 64 ];

	sprintf( text, "%d", a );
	Append( text );

	return *this;
}

inline anStr &anStr::operator+=( const unsigned a ) {
	char text[ 64 ];

	sprintf( text, "%u", a );
	Append( text );

	return *this;
}

inline anStr &anStr::operator+=( const anStr &a ) {
	Append( a );
	return *this;
}

inline anStr &anStr::operator+=( const char *a ) {
	Append( a );
	return *this;
}

inline anStr &anStr::operator+=( const char a ) {
	Append( a );
	return *this;
}

inline anStr &anStr::operator+=( const bool a ) {
	Append( a ? "true" : "false" );
	return *this;
}

inline bool operator==( const anStr &a, const anStr &b ) {
	return ( !anStr::Cmp( a.data, b.data ) );
}

inline bool operator==( const anStr &a, const char *b ) {
	assert( b );
	return ( !anStr::Cmp( a.data, b ) );
}

inline bool operator==( const char *a, const anStr &b ) {
	assert( a );
	return ( !anStr::Cmp( a, b.data ) );
}

inline bool operator!=( const anStr &a, const anStr &b ) {
	return !( a == b );
}

inline bool operator!=( const anStr &a, const char *b ) {
	return !( a == b );
}

inline bool operator!=( const char *a, const anStr &b ) {
	return !( a == b );
}

inline int anStr::Cmp( const char *text ) const {
	assert( text );
	return anStr::Cmp( data, text );
}

inline int anStr::Cmpn( const char *text, int n ) const {
	assert( text );
	return anStr::Cmpn( data, text, n );
}

inline int anStr::CmpPrefix( const char *text ) const {
	assert( text );
	return anStr::Cmpn( data, text, strlen( text ) );
}

inline int anStr::Icmp( const char *text ) const {
	assert( text );
	return anStr::Icmp( data, text );
}

inline int anStr::Icmpn( const char *text, int n ) const {
	assert( text );
	return anStr::Icmpn( data, text, n );
}

inline int anStr::IcmpPrefix( const char *text ) const {
	assert( text );
	return anStr::Icmpn( data, text, strlen( text ) );
}

inline int anStr::IcmpNoColor( const char *text ) const {
	assert( text );
	return anStr::IcmpNoColor( data, text );
}

inline int anStr::IcmpPath( const char *text ) const {
	assert( text );
	return anStr::IcmpPath( data, text );
}

inline int anStr::IcmpnPath( const char *text, int n ) const {
	assert( text );
	return anStr::IcmpnPath( data, text, n );
}

inline int anStr::IcmpPrefixPath( const char *text ) const {
	assert( text );
	return anStr::IcmpnPath( data, text, strlen( text ) );
}

inline int anStr::Length( void ) const {
	return len;
}

inline int anStr::Allocated( void ) const {
	if ( data != baseBuffer ) {
		return alloced;
	} else {
		return 0;
	}
}

inline void anStr::Empty( void ) {
	EnsureAlloced( 1 );
	data[0] = '\0';
	len = 0;
}

inline bool anStr::IsEmpty( void ) const {
	return ( anStr::Cmp( data, "" ) == 0 );
}

inline void anStr::Clear( void ) {
	FreeData();
	Init();
}

inline void anStr::Append( const char a ) {
	EnsureAlloced( len + 2 );
	data[ len ] = a;
	len++;
	data[ len ] = '\0';
}

inline void anStr::Append( const anStr &text ) {
	int newLen = len + text.Length();
	EnsureAlloced( newLen + 1 );
	for ( int i = 0; i < text.len; i++ ) {
		data[ len + i ] = text[i];
	}
	len = newLen;
	data[ len ] = '\0';
}

inline void anStr::Append( const char *text ) {
	if ( text ) {
		int newLen = len + strlen( text );
		EnsureAlloced( newLen + 1 );
		for ( int i = 0; text[i]; i++ ) {
			data[ len + i ] = text[i];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

inline void anStr::Append( const char *text, int l ) {
	if ( text && l ) {
		int newLen = len + l;
		EnsureAlloced( newLen + 1 );
		for ( int i = 0; text[i] && i < l; i++ ) {
			data[ len + i ] = text[i];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

inline void anStr::Insert( const char a, int index ) {
	if ( index < 0 ) {
		index = 0;
	} else if ( index > len ) {
		index = len;
	}

	int l = 1;
	EnsureAlloced( len + l + 1 );
	for ( int i = len; i >= index; i-- ) {
		data[i+l] = data[i];
	}
	data[index] = a;
	len++;
}

inline void anStr::Insert( const char *text, int index ) {
	if ( index < 0 ) {
		index = 0;
	} else if ( index > len ) {
		index = len;
	}

	int l = strlen( text );
	EnsureAlloced( len + l + 1 );
	for ( i = len; i >= index; i-- ) {
		data[i+l] = data[i];
	}
	for ( int i = 0; i < l; i++ ) {
		data[index+i] = text[i];
	}
	len += l;
}

inline void anStr::ToLower( void ) {
	for ( int i = 0; data[i]; i++ ) {
		if ( CharIsUpper( data[i] ) ) {
			data[i] += ( 'a' - 'A' );
		}
	}
}

inline void anStr::ToUpper( void ) {
	for ( int i = 0; data[i]; i++ ) {
		if ( CharIsLower( data[i] ) ) {
			data[i] -= ( 'a' - 'A' );
		}
	}
}

inline bool anStr::IsNumeric( void ) const {
	return anStr::IsNumeric( data );
}

inline bool anStr::IsColor( void ) const {
	return anStr::IsColor( data );
}

inline bool anStr::HasLower( void ) const {
	return anStr::HasLower( data );
}

inline bool anStr::HasUpper( void ) const {
	return anStr::HasUpper( data );
}

inline anStr &anStr::RemoveColors( void ) {
	anStr::RemoveColors( data );
	len = Length( data );
	return *this;
}

inline int anStr::LengthWithoutColors( void ) const {
	return anStr::LengthWithoutColors( data );
}

inline void anStr::CapLength( int newlen ) {
	//assert( data );
	if ( len <= newlen ) {
		return;
	}
	//EnsureDataWritable();
	data[ newlen ] = 0;
	len = newlen;
}

inline void idStr::EnsureDataWritable( void ) {
	assert( data );
	strdata *oldData;
	int len;

	if ( !data->refcount ) {
		return;
	}

	oldData = data;
	len = length();

	data = new strdata;

	EnsureAlloced( len + 1, false );
	strncpy( data->data, oldData->data, len + 1 );
	data->len = len;

	oldData->DelRef();
}

inline void anStr::Fill( const char ch, int newlen ) {
	EnsureAlloced( newlen + 1 );
	len = newlen;
	memset( data, ch, len );
	data[ len ] = 0;
}

inline int anStr::Find( const char c, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return anStr::FindChar( data, c, start, end );
}

inline int anStr::Find( const char *text, bool casesensitive, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return anStr::FindText( data, text, casesensitive, start, end );
}

inline bool anStr::Filter( const char *filter, bool casesensitive ) const {
	return anStr::Filter( filter, data, casesensitive );
}

inline const char *anStr::Left( int len, anStr &result ) const {
	return Mid( 0, len, result );
}

inline const char *anStr::Right( int len, anStr &result ) const {
	if ( len >= Length() ) {
		result = *this;
		return result;
	}
	return Mid( Length() - len, len, result );
}

inline anStr anStr::Left( int len ) const {
	return Mid( 0, len );
}

inline anStr anStr::Right( int len ) const {
	if ( len >= Length() ) {
		return *this;
	}
	return Mid( Length() - len, len );
}

inline void anStr::Strip( const char c ) {
	StripLeading( c );
	StripTrailing( c );
}

inline void anStr::Strip( const char *string ) {
	StripLeading( string );
	StripTrailing( string );
}

inline bool anStr::CheckExtension( const char *ext ) {
	return anStr::CheckExtension( data, ext );
}

inline int anStr::Length( const char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		return i;
	}
}

inline char *anStr::ToLower( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsUpper( s[i] ) ) {
			s[i] += ( 'a' - 'A' );
		}
	}
	return s;
}

inline char *anStr::ToUpper( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsLower( s[i] ) ) {
			s[i] -= ( 'a' - 'A' );
		}
	}
	return s;
}

inline int anStr::Hash( const char *string ) {
	int hash = 0;
	for ( int i = 0; *string != '\0'; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

inline int anStr::Hash( const char *string, int length ) {
	int hash = 0;
	for ( int i = 0; i < length; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

inline int anStr::IHash( const char *string ) {
	int hash = 0;
	for ( int i = 0; *string != '\0'; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

inline int anStr::IHash( const char *string, int length ) {
	int hash = 0;
	for ( int i = 0; i < length; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

inline bool anStr::IsColor( const char *s ) {
	return ( s[0] == C_COLOR_ESCAPE && s[1] != '\0' && s[1] != ' ' );
}

inline char anStr::ToLower( char c ) {
	if ( c <= 'Z' && c >= 'A' ) {
		return ( c + ( 'a' - 'A' ) );
	}
	return c;
}

inline char anStr::ToUpper( char c ) {
	if ( c >= 'a' && c <= 'z' ) {
		return ( c - ( 'a' - 'A' ) );
	}
	return c;
}

inline bool anStr::CharIsPrintable( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 0x20 && c <= 0x7E ) || ( c >= 0xA1 && c <= 0xFF );
}

inline bool anStr::CharIsLower( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 'a' && c <= 'z' ) || ( c >= 0xE0 && c <= 0xFF );
}

inline bool anStr::CharIsUpper( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c <= 'Z' && c >= 'A' ) || ( c >= 0xC0 && c <= 0xDF );
}

inline bool anStr::CharIsAlpha( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= 0xC0 && c <= 0xFF ) );
}

inline bool anStr::CharIsNumeric( int c ) {
	return ( c <= '9' && c >= '0' );
}

inline bool anStr::CharIsNewLine( char c ) {
	return ( c == '\n' || c == '\r' || c == '\v' );
}

inline bool anStr::CharIsTab( char c ) {
	return ( c == '\t' );
}

inline int anStr::ColorIndex( int c ) {
	return ( c & 15 );
}

inline int anStr::DynamicMemoryUsed() const {
	return ( data == baseBuffer ) ? 0 : alloced;
}

template< int _size_ >
class anStaticString : public anStr {
public:
inline void operator=( const anStaticString &text ) {
	// we should only get here when the types, including the size, are identical
	len = text.Length();
	memcpy( data, text.data, len+1 );
}

// all anStr operators are overloaded and the anStr default constructor is called so that the
// static buffer can be initialized in the body of the constructor before the data is ever
// copied.
inline	anStaticString() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
}

inline	anStaticString( const anStaticString & text ) :
	anStr() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anStr::operator=( text );
}

inline	anStaticString( const anStr & text ) :
	anStr() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anStr::operator=( text );
}

inline	anStaticString( const anStaticString & text, int start, int end ) :
	anStr() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
	CopyRange( text.c_str(), start, end );
}

inline	anStaticString( const char *text ) :
	anStr() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anStr::operator=( text );
}

inline	anStaticString( const char *text, int start, int end ) :
	anStr() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
	CopyRange( text, start, end );
}

inline	explicit anStaticString( const bool b ) :
	anStr() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anStr::operator=( b );
}

inline	explicit anStaticString( const char c ) :
	anStr() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anStr::operator=( c );
}

inline	explicit anStaticString( const int i ) :
	anStr() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anStr::operator=( i );
}

inline	explicit anStaticString( const unsigned u ) :
	anStr() {
	buffer[0] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anStr::operator=( u );
}

inline	explicit anStaticString( const float f ) :
	anStr() { buffer[0] = '\0'; SetStaticBuffer( buffer, _size_ ); anStr::operator=( f );
}

private:
	char			buffer[ _size_ ];
};

#endif
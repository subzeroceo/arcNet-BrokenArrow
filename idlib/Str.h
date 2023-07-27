#ifndef __STR_H__
#define __STR_H__

/*
===============================================================================

	Character string

===============================================================================
*/

// these library functions should not be used for cross platform compatibility
#define strcmp			anString::Cmp		// use_arcStr_Cmp
#define strncmp			use_arcStr_Cmpn

#if defined( StrCmpN )
#undef StrCmpN
#endif
#define StrCmpN			use_arcStr_Cmpn

#if defined( strcmpi )
#undef strcmpi
#endif
#define strcmpi			use_arcStr_Icmp

#if defined( StrCmpI )
#undef StrCmpI
#endif
#define StrCmpI			use_arcStr_Icmp

#if defined( StrCmpNI )
#undef StrCmpNI
#endif
#define StrCmpNI		use_arcStr_Icmpn

#define stricmp			anString::Icmp		// use_arcStr_Icmp
#define _stricmp		use_arcStr_Icmp
#define strcasecmp		use_arcStr_Icmp
#define strnicmp		use_arcStr_Icmpn
#define _strnicmp		use_arcStr_Icmpn
#define _memicmp		use_arcStr_Icmpn
#define snprintf		use_arcStr_snPrintf
#define _snprintf		use_arcStr_snPrintf
#define vsnprintf		use_arcStr_vsnPrintf
#define _vsnprintf		use_arcStr_vsnPrintf

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

// make anString a multiple of 16 bytes long
// don't make too large to keep memory requirements to a minimum
const int STR_ALLOC_BASE			= 20;
const int STR_ALLOC_GRAN			= 32;

typedef enum {
	MEASURE_SIZE = 0,
	MEASURE_BANDWIDTH
} Measure_t;

class anString {

public:

	struct TimeFormat {
		TimeFormat() : ShowMinutes( false ), ShowHours( false ), ShowSeconds( true ) {}
		bool showMinutes;
		bool showHours;
		bool showSeconds;
	};
						anString( void );
						anString( const anString &text );
						anString( const anString &text, int start, int end );
						anString( const char *text );
						anString( const char *text, int start, int end );
						explicit anString( const bool b );
						explicit anString( const char c );
						explicit anString( const int i );
						explicit anString( const unsigned u );
						explicit anString( const float f );
						~anString( void );

	size_t				Size( void ) const;
	const char *		c_str( void ) const;
	operator			const char *( void ) const;
	operator			const char *( void );

	char				operator[]( int index ) const;
	char &				operator[]( int index );

	void				operator=( const anString &text );
	void				operator=( const char *text );

	friend anString		operator+( const anString &a, const anString &b );
	friend anString		operator+( const anString &a, const char *b );
	friend anString		operator+( const char *a, const anString &b );

	friend anString		operator+( const anString &a, const float b );
	friend anString		operator+( const anString &a, const int b );
	friend anString		operator+( const anString &a, const unsigned b );
	friend anString		operator+( const anString &a, const bool b );
	friend anString		operator+( const anString &a, const char b );

	anString &			operator+=( const anString &a );
	anString &			operator+=( const char *a );
	anString &			operator+=( const float a );
	anString &			operator+=( const char a );
	anString &			operator+=( const int a );
	anString &			operator+=( const unsigned a );
	anString &			operator+=( const bool a );

						// case sensitive compare
	friend bool			operator==( const anString &a, const anString &b );
	friend bool			operator==( const anString &a, const char *b );
	friend bool			operator==( const char *a, const anString &b );

						// case sensitive compare
	friend bool			operator!=( const anString &a, const anString &b );
	friend bool			operator!=( const anString &a, const char *b );
	friend bool			operator!=( const char *a, const anString &b );

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
	void				Append( const anString &text );
	void				Append( const char *text );
	void				Append( int count, const char c );

	void				Append( const char *text, int len );
	void				Insert( const char a, int index );
	void				Insert( const char *text, int index );
	void				ToLower( void );
	void				ToUpper( void );
	bool				IsNumeric( void ) const;

	void				Swap( idStr& rhs );

	bool				IsColor( void ) const;
	bool				IsHexColor( void ) const;
	bool				HasHexColorAlpha( void ) const;
	int					LengthWithoutColors( void ) const;
	anString &		RemoveColors( void );

	bool				HasLower( void ) const;
	bool				HasUpper( void ) const;

	void				CapLength( int );
	void				Fill( const char ch, int newlen );

	int					Find( const char c, int start = 0, int end = -1 ) const;
	int					Find( const char *text, bool casesensitive = true, int start = 0, int end = -1 ) const;
	const char*			FindString( const char* text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION ) const;
	int					CountChar( const char c );
	bool				Filter( const char *filter, bool casesensitive ) const;

	int					Last( const char c, int index = INVALID_POSITION ) const;
	int					Last( const char c ) const;						// return the index to the last occurance of 'c', returns -1 if not found
	const char *		Left( int len, anString &result ) const;			// store the leftmost 'len' characters in the result
	anString		Left( int len ) const;							// return the leftmost 'len' characters

	const char *		Right( int len, anString &result ) const;			// store the rightmost 'len' characters in the result
	const char *		Mid( int start, int len, anString &result ) const;	// store 'len' characters starting at 'start' in result

	anString		Right( int len ) const;							// return the rightmost 'len' characters
	anString		Mid( int start, int len ) const;				// return 'len' characters starting at 'start'
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
	anString &		StripQuotes( void );							// strip quotes around string
	void				Replace( const char *old, const char *nw );

	void				ReplaceFirst( const char *old, const char *nw );
	void				ReplaceChar( char oldChar, char newChar );

	void				EraseRange( int start, int len = INVALID_POSITION );
	void				EraseChar( const char c, int start = 0 );

	// file name methods
	int					FileNameHash( void ) const;						// hash key for the filename (skips extension)
	idStr &				CollapsePath( void );							// where possible removes /../ and /./ from path

	anString &		BackSlashesToSlashes( void );					// convert slashes
	idStr &				SlashesToBackSlashes( void );					// convert slashes

	anString &		SetFileExtension( const char *extension );		// set the given file extension
	anString &		StripFileExtension( void );						// remove any file extension
	anString &		StripAbsoluteFileExtension( void );				// remove any file extension looking from front (useful if there are multiple .'s)
	anString &		DefaultFileExtension( const char *extension );	// if there's no file extension use the default
	anString &		DefaultPath( const char *basepath );			// if there's no path use the default
	void				AppendPath( const char *text );					// append a partial path
	anString &		StripFilename( void );							// remove the filename from a path
	anString &		StripPath( void );								// remove the path from the filename
	void				ExtractFilePath( anString &dest ) const;			// copy the file path to another string
	void				ExtractFileName( anString &dest ) const;			// copy the filename to another string
	void				ExtractFileBase( anString &dest ) const;			// copy the filename minus the extension to another string
	void				ExtractFileExtension( anString &dest ) const;		// copy the file extension to another string
	bool				CheckExtension( const char *ext );

	idStr &				StripComments();								// remove C++ and C style comments
	idStr & 			Indent();										// indents brace-delimited text, preserving tabs in the middle of lines
	idStr & 			Unindent();										// unindents brace-delimited text, preserving tabs in the middle of lines
	idStr &				CleanFilename( void );							// strips bad characters
	bool				IsValidEmailAddress( void );
	idStr &				CollapseColors( void );							// removes redundant color codes

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
	static int			snPrintf( char *dest, int size, const char *fmt, ... ) arc_attribute( ( format( printf, 3, 4 ) ) );
	static int			vsnPrintf( char *dest, int size, const char *fmt, va_list argptr );

	static int			CountChar( const char *str, const char c );
	static int			FindChar( const char *str, const char c, int start = 0, int end = -1 );
	static int			FindText( const char *str, const char *text, bool casesensitive = true, int start = 0, int end = -1 );
	static bool			Filter( const char *filter, const char *name, bool casesensitive );
	static void			StripMediaName( const char *name, anString &mediaName );
	static bool			CheckExtension( const char *name, const char *ext );

	static const char *	FloatArrayToString( const float *array, const int length, const int precision );
	static int			NumLonelyLF( const char *src );	// return the number of line feeds not paired with a carriage return... usefull for getting a correct destination buffer size for ToCRLF
	static bool			ToCRLF( const char *src, char *dest, int maxLength );
	static const char *	CStyleQuote( const char *str );
	static const char *	CStyleUnQuote( const char *str );
	static void			IndentAndPad( int indent, int pad, idStr &str, const char *fmt, ... );  // indent and pad out formatted text

	static void			StringToBinaryString( arcNetStr &out, void *pv, int size);
	static bool			BinaryStringToString( const char* str,  void *pv, int size );

    static bool			IsValidEmailAddress( const char* address );

	static const char*	MS2HMS( double ms, const HMSTimeFormat_t &formatSpec = defaultHMSFormat );
	static const char *FormatInt( const int num );	 // formats an integer as a value with commas

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
	static int			ColorIndex( int c );
	static const anVec4&ColorForIndex( int i );
	static const anVec4&ColorForChar( int c );
	static const char*	StrForColorIndex( int i );
	static int			HexForChar( int c );

	friend int			sprintf( anString &dest, const char *fmt, ... );
	friend int			vsprintf( anString &dest, const char *fmt, va_list ap );

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
	static anString	FormatNumber( int number );

	//static void			SetStringAllocator( stringDataAllocator_t *allocator );
	//static stringDataAllocator_t *GetStringAllocator( void );
	//static void			Test( void );

protected:
	int					len;
	char *				data;
	int					alloced;
	char				baseBuffer[ STR_ALLOC_BASE ];

	void				Init( void );										// initialize string using base buffer
	void				EnsureAlloced( int amount, bool keepold = true );	// ensure string data buffer is large anough

	static				stringDataAllocator_t *	stringDataAllocator;
	static				bool					stringAllocatorIsShared;
	static HMSTimeFormat_t	defaultHMSFormat;
};

char *					va( const char *fmt, ... ) arc_attribute( ( format( printf, 1, 2 )  ) );


ARC_INLINE void anString::EnsureAlloced( int amount, bool keepold ) {
	if ( amount > alloced ) {
		ReAllocate( amount, keepold );
	}
}

ARC_INLINE void anString::Init( void ) {
	len = 0;
	alloced = STR_ALLOC_BASE;
	data = baseBuffer;
	data[ 0 ] = '\0';
#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
	memset( baseBuffer, 0, sizeof( baseBuffer ) );
#endif
}

ARC_INLINE anString::anString( void ) {
	Init();
}

ARC_INLINE anString::anString( const anString &text ) {
	Init();
	int l = text.Length();
	EnsureAlloced( l + 1 );
	strcpy( data, text.data );
	len = l;
}

ARC_INLINE anString::anString( const anString &text, int start, int end ) {
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
		data[ i ] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

ARC_INLINE anString::anString( const char *text ) {
	Init();
	if ( text ) {
		int l = strlen( text );
		EnsureAlloced( l + 1 );
		strcpy( data, text );
		len = l;
	}
}

ARC_INLINE anString::anString( const char *text, int start, int end ) {
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
		data[ i ] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

ARC_INLINE anString::anString( const bool b ) {
	Init();
	EnsureAlloced( 2 );
	data[ 0 ] = b ? '1' : '0';
	data[ 1 ] = '\0';
	len = 1;
}

ARC_INLINE anString::anString( const char c ) {
	Init();
	EnsureAlloced( 2 );
	data[ 0 ] = c;
	data[ 1 ] = '\0';
	len = 1;
}

ARC_INLINE anString::anString( const int i ) {
	char text[ 64 ];

	Init();
	int l = sprintf( text, "%d", i );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ARC_INLINE anString::anString( const unsigned u ) {
	char text[ 64 ];
	Init();
	int l = sprintf( text, "%u", u );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ARC_INLINE anString::anString( const float f ) {
	char text[ 64 ];
	Init();
	int l = anString::snPrintf( text, sizeof( text ), "%f", f );
	while ( l > 0 && text[l-1] == '0' ) text[--l] = '\0';
	while ( l > 0 && text[l-1] == '.' ) text[--l] = '\0';
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ARC_INLINE anString::~anString( void ) {
	FreeData();
}

ARC_INLINE size_t anString::Size( void ) const {
	return sizeof( *this ) + Allocated();
}

ARC_INLINE const char *anString::c_str( void ) const {
	return data;
}

ARC_INLINE anString::operator const char *( void ) {
	return c_str();
}

ARC_INLINE anString::operator const char *( void ) const {
	return c_str();
}

ARC_INLINE char anString::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[index];
}

ARC_INLINE char &anString::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[index];
}

ARC_INLINE void anString::operator=( const anString &text ) {
	int l = text.Length();
	EnsureAlloced( l + 1, false );
	memcpy( data, text.data, l );
	data[l] = '\0';
	len = l;
}

ARC_INLINE anString operator+( const anString &a, const anString &b ) {
	anString result( a );
	result.Append( b );
	return result;
}

ARC_INLINE anString operator+( const anString &a, const char *b ) {
	anString result( a );
	result.Append( b );
	return result;
}

ARC_INLINE anString operator+( const char *a, const anString &b ) {
	anString result( a );
	result.Append( b );
	return result;
}

ARC_INLINE anString operator+( const anString &a, const bool b ) {
	anString result( a );
	result.Append( b ? "true" : "false" );
	return result;
}

ARC_INLINE anString operator+( const anString &a, const char b ) {
	anString result( a );
	result.Append( b );
	return result;
}

ARC_INLINE anString operator+( const anString &a, const float b ) {
	char	text[ 64 ];
	anString	result( a );

	sprintf( text, "%f", b );
	result.Append( text );

	return result;
}

ARC_INLINE anString operator+( const anString &a, const int b ) {
	char	text[ 64 ];
	anString	result( a );

	sprintf( text, "%d", b );
	result.Append( text );

	return result;
}

ARC_INLINE anString operator+( const anString &a, const unsigned b ) {
	char	text[ 64 ];
	anString	result( a );

	sprintf( text, "%u", b );
	result.Append( text );

	return result;
}

ARC_INLINE anString &anString::operator+=( const float a ) {
	char text[ 64 ];

	sprintf( text, "%f", a );
	Append( text );

	return *this;
}

ARC_INLINE anString &anString::operator+=( const int a ) {
	char text[ 64 ];

	sprintf( text, "%d", a );
	Append( text );

	return *this;
}

ARC_INLINE anString &anString::operator+=( const unsigned a ) {
	char text[ 64 ];

	sprintf( text, "%u", a );
	Append( text );

	return *this;
}

ARC_INLINE anString &anString::operator+=( const anString &a ) {
	Append( a );
	return *this;
}

ARC_INLINE anString &anString::operator+=( const char *a ) {
	Append( a );
	return *this;
}

ARC_INLINE anString &anString::operator+=( const char a ) {
	Append( a );
	return *this;
}

ARC_INLINE anString &anString::operator+=( const bool a ) {
	Append( a ? "true" : "false" );
	return *this;
}

ARC_INLINE bool operator==( const anString &a, const anString &b ) {
	return ( !anString::Cmp( a.data, b.data ) );
}

ARC_INLINE bool operator==( const anString &a, const char *b ) {
	assert( b );
	return ( !anString::Cmp( a.data, b ) );
}

ARC_INLINE bool operator==( const char *a, const anString &b ) {
	assert( a );
	return ( !anString::Cmp( a, b.data ) );
}

ARC_INLINE bool operator!=( const anString &a, const anString &b ) {
	return !( a == b );
}

ARC_INLINE bool operator!=( const anString &a, const char *b ) {
	return !( a == b );
}

ARC_INLINE bool operator!=( const char *a, const anString &b ) {
	return !( a == b );
}

ARC_INLINE int anString::Cmp( const char *text ) const {
	assert( text );
	return anString::Cmp( data, text );
}

ARC_INLINE int anString::Cmpn( const char *text, int n ) const {
	assert( text );
	return anString::Cmpn( data, text, n );
}

ARC_INLINE int anString::CmpPrefix( const char *text ) const {
	assert( text );
	return anString::Cmpn( data, text, strlen( text ) );
}

ARC_INLINE int anString::Icmp( const char *text ) const {
	assert( text );
	return anString::Icmp( data, text );
}

ARC_INLINE int anString::Icmpn( const char *text, int n ) const {
	assert( text );
	return anString::Icmpn( data, text, n );
}

ARC_INLINE int anString::IcmpPrefix( const char *text ) const {
	assert( text );
	return anString::Icmpn( data, text, strlen( text ) );
}

ARC_INLINE int anString::IcmpNoColor( const char *text ) const {
	assert( text );
	return anString::IcmpNoColor( data, text );
}

ARC_INLINE int anString::IcmpPath( const char *text ) const {
	assert( text );
	return anString::IcmpPath( data, text );
}

ARC_INLINE int anString::IcmpnPath( const char *text, int n ) const {
	assert( text );
	return anString::IcmpnPath( data, text, n );
}

ARC_INLINE int anString::IcmpPrefixPath( const char *text ) const {
	assert( text );
	return anString::IcmpnPath( data, text, strlen( text ) );
}

ARC_INLINE int anString::Length( void ) const {
	return len;
}

ARC_INLINE int anString::Allocated( void ) const {
	if ( data != baseBuffer ) {
		return alloced;
	} else {
		return 0;
	}
}

ARC_INLINE void anString::Empty( void ) {
	EnsureAlloced( 1 );
	data[ 0 ] = '\0';
	len = 0;
}

ARC_INLINE bool anString::IsEmpty( void ) const {
	return ( anString::Cmp( data, "" ) == 0 );
}

ARC_INLINE void anString::Clear( void ) {
	FreeData();
	Init();
}

ARC_INLINE void anString::Append( const char a ) {
	EnsureAlloced( len + 2 );
	data[ len ] = a;
	len++;
	data[ len ] = '\0';
}

ARC_INLINE void anString::Append( const anString &text ) {
	int newLen = len + text.Length();
	EnsureAlloced( newLen + 1 );
	for ( int i = 0; i < text.len; i++ ) {
		data[ len + i ] = text[ i ];
	}
	len = newLen;
	data[ len ] = '\0';
}

ARC_INLINE void anString::Append( const char *text ) {
	if ( text ) {
		int newLen = len + strlen( text );
		EnsureAlloced( newLen + 1 );
		for ( int i = 0; text[ i ]; i++ ) {
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

ARC_INLINE void anString::Append( const char *text, int l ) {
	if ( text && l ) {
		int newLen = len + l;
		EnsureAlloced( newLen + 1 );
		for ( int i = 0; text[ i ] && i < l; i++ ) {
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

ARC_INLINE void anString::Insert( const char a, int index ) {
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

ARC_INLINE void anString::Insert( const char *text, int index ) {
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

ARC_INLINE void anString::ToLower( void ) {
	for ( int i = 0; data[i]; i++ ) {
		if ( CharIsUpper( data[i] ) ) {
			data[i] += ( 'a' - 'A' );
		}
	}
}

ARC_INLINE void anString::ToUpper( void ) {
	for ( int i = 0; data[i]; i++ ) {
		if ( CharIsLower( data[i] ) ) {
			data[i] -= ( 'a' - 'A' );
		}
	}
}

ARC_INLINE bool anString::IsNumeric( void ) const {
	return anString::IsNumeric( data );
}

ARC_INLINE bool anString::IsColor( void ) const {
	return anString::IsColor( data );
}

ARC_INLINE bool anString::HasLower( void ) const {
	return anString::HasLower( data );
}

ARC_INLINE bool anString::HasUpper( void ) const {
	return anString::HasUpper( data );
}

ARC_INLINE anString &anString::RemoveColors( void ) {
	anString::RemoveColors( data );
	len = Length( data );
	return *this;
}

ARC_INLINE int anString::LengthWithoutColors( void ) const {
	return anString::LengthWithoutColors( data );
}

ARC_INLINE void anString::CapLength( int newlen ) {
	if ( len <= newlen ) {
		return;
	}
	data[ newlen ] = 0;
	len = newlen;
}

ARC_INLINE void anString::Fill( const char ch, int newlen ) {
	EnsureAlloced( newlen + 1 );
	len = newlen;
	memset( data, ch, len );
	data[ len ] = 0;
}

ARC_INLINE int anString::Find( const char c, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return anString::FindChar( data, c, start, end );
}

ARC_INLINE int anString::Find( const char *text, bool casesensitive, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return anString::FindText( data, text, casesensitive, start, end );
}

ARC_INLINE bool anString::Filter( const char *filter, bool casesensitive ) const {
	return anString::Filter( filter, data, casesensitive );
}

ARC_INLINE const char *anString::Left( int len, anString &result ) const {
	return Mid( 0, len, result );
}

ARC_INLINE const char *anString::Right( int len, anString &result ) const {
	if ( len >= Length() ) {
		result = *this;
		return result;
	}
	return Mid( Length() - len, len, result );
}

ARC_INLINE anString anString::Left( int len ) const {
	return Mid( 0, len );
}

ARC_INLINE anString anString::Right( int len ) const {
	if ( len >= Length() ) {
		return *this;
	}
	return Mid( Length() - len, len );
}

ARC_INLINE void anString::Strip( const char c ) {
	StripLeading( c );
	StripTrailing( c );
}

ARC_INLINE void anString::Strip( const char *string ) {
	StripLeading( string );
	StripTrailing( string );
}

ARC_INLINE bool anString::CheckExtension( const char *ext ) {
	return anString::CheckExtension( data, ext );
}

ARC_INLINE int anString::Length( const char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		return i;
	}
}

ARC_INLINE char *anString::ToLower( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsUpper( s[i] ) ) {
			s[i] += ( 'a' - 'A' );
		}
	}
	return s;
}

ARC_INLINE char *anString::ToUpper( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsLower( s[i] ) ) {
			s[i] -= ( 'a' - 'A' );
		}
	}
	return s;
}

ARC_INLINE int anString::Hash( const char *string ) {
	int hash = 0;
	for ( int i = 0; *string != '\0'; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

ARC_INLINE int anString::Hash( const char *string, int length ) {
	int hash = 0;
	for ( int i = 0; i < length; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

ARC_INLINE int anString::IHash( const char *string ) {
	int hash = 0;
	for ( int i = 0; *string != '\0'; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

ARC_INLINE int anString::IHash( const char *string, int length ) {
	int hash = 0;
	for ( int i = 0; i < length; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

ARC_INLINE bool anString::IsColor( const char *s ) {
	return ( s[0] == C_COLOR_ESCAPE && s[1] != '\0' && s[1] != ' ' );
}

ARC_INLINE char anString::ToLower( char c ) {
	if ( c <= 'Z' && c >= 'A' ) {
		return ( c + ( 'a' - 'A' ) );
	}
	return c;
}

ARC_INLINE char anString::ToUpper( char c ) {
	if ( c >= 'a' && c <= 'z' ) {
		return ( c - ( 'a' - 'A' ) );
	}
	return c;
}

ARC_INLINE bool anString::CharIsPrintable( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 0x20 && c <= 0x7E ) || ( c >= 0xA1 && c <= 0xFF );
}

ARC_INLINE bool anString::CharIsLower( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 'a' && c <= 'z' ) || ( c >= 0xE0 && c <= 0xFF );
}

ARC_INLINE bool anString::CharIsUpper( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c <= 'Z' && c >= 'A' ) || ( c >= 0xC0 && c <= 0xDF );
}

ARC_INLINE bool anString::CharIsAlpha( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= 0xC0 && c <= 0xFF ) );
}

ARC_INLINE bool anString::CharIsNumeric( int c ) {
	return ( c <= '9' && c >= '0' );
}

ARC_INLINE bool anString::CharIsNewLine( char c ) {
	return ( c == '\n' || c == '\r' || c == '\v' );
}

ARC_INLINE bool anString::CharIsTab( char c ) {
	return ( c == '\t' );
}

ARC_INLINE int anString::ColorIndex( int c ) {
	return ( c & 15 );
}

ARC_INLINE int anString::DynamicMemoryUsed() const {
	return ( data == baseBuffer ) ? 0 : alloced;
}

template< int _size_ >
class anStaticString : public anString {
public:
ARC_INLINE void operator=( const anStaticString &text ) {
	// we should only get here when the types, including the size, are identical
	len = text.Length();
	memcpy( data, text.data, len+1 );
}

// all anString operators are overloaded and the anString default constructor is called so that the
// static buffer can be initialized in the body of the constructor before the data is ever
// copied.
ARC_INLINE	anStaticString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
}

ARC_INLINE	anStaticString( const anStaticString & text ) :
	anString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anString::operator=( text );
}

ARC_INLINE	anStaticString( const anString & text ) :
	anString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anString::operator=( text );
}

ARC_INLINE	anStaticString( const anStaticString & text, int start, int end ) :
	anString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	CopyRange( text.c_str(), start, end );
}

ARC_INLINE	anStaticString( const char *text ) :
	anString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anString::operator=( text );
}

ARC_INLINE	anStaticString( const char *text, int start, int end ) :
	anString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	CopyRange( text, start, end );
}

ARC_INLINE	explicit anStaticString( const bool b ) :
	anString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anString::operator=( b );
}

ARC_INLINE	explicit anStaticString( const char c ) :
	anString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anString::operator=( c );
}

ARC_INLINE	explicit anStaticString( const int i ) :
	anString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anString::operator=( i );
}

ARC_INLINE	explicit anStaticString( const unsigned u ) :
	anString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	anString::operator=( u );
}

ARC_INLINE	explicit anStaticString( const float f ) :
	anString() { buffer[ 0 ] = '\0'; SetStaticBuffer( buffer, _size_ ); anString::operator=( f );
}

private:
	char			buffer[ _size_ ];
};

#endif
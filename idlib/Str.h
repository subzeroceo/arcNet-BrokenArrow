#ifndef __STR_H__
#define __STR_H__

/*
===============================================================================

	Character string

===============================================================================
*/

// these library functions should not be used for cross platform compatibility
#define strcmp			arcNetString::Cmp		// use_arcStr_Cmp
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

#define stricmp			arcNetString::Icmp		// use_arcStr_Icmp
#define _stricmp		use_arcStr_Icmp
#define strcasecmp		use_arcStr_Icmp
#define strnicmp		use_arcStr_Icmpn
#define _strnicmp		use_arcStr_Icmpn
#define _memicmp		use_arcStr_Icmpn
#define snprintf		use_arcStr_snPrintf
#define _snprintf		use_arcStr_snPrintf
#define vsnprintf		use_arcStr_vsnPrintf
#define _vsnprintf		use_arcStr_vsnPrintf

class arcVec4;

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

// make arcNetString a multiple of 16 bytes long
// don't make too large to keep memory requirements to a minimum
const int STR_ALLOC_BASE			= 20;
const int STR_ALLOC_GRAN			= 32;

typedef enum {
	MEASURE_SIZE = 0,
	MEASURE_BANDWIDTH
} Measure_t;

class arcNetString {

public:

	struct TimeFormat {
		TimeFormat() : ShowMinutes( false ), ShowHours( false ), ShowSeconds( true ) {}
		bool showMinutes;
		bool showHours;
		bool showSeconds;
	};
						arcNetString( void );
						arcNetString( const arcNetString &text );
						arcNetString( const arcNetString &text, int start, int end );
						arcNetString( const char *text );
						arcNetString( const char *text, int start, int end );
						explicit arcNetString( const bool b );
						explicit arcNetString( const char c );
						explicit arcNetString( const int i );
						explicit arcNetString( const unsigned u );
						explicit arcNetString( const float f );
						~arcNetString( void );

	size_t				Size( void ) const;
	const char *		c_str( void ) const;
	operator			const char *( void ) const;
	operator			const char *( void );

	char				operator[]( int index ) const;
	char &				operator[]( int index );

	void				operator=( const arcNetString &text );
	void				operator=( const char *text );

	friend arcNetString		operator+( const arcNetString &a, const arcNetString &b );
	friend arcNetString		operator+( const arcNetString &a, const char *b );
	friend arcNetString		operator+( const char *a, const arcNetString &b );

	friend arcNetString		operator+( const arcNetString &a, const float b );
	friend arcNetString		operator+( const arcNetString &a, const int b );
	friend arcNetString		operator+( const arcNetString &a, const unsigned b );
	friend arcNetString		operator+( const arcNetString &a, const bool b );
	friend arcNetString		operator+( const arcNetString &a, const char b );

	arcNetString &			operator+=( const arcNetString &a );
	arcNetString &			operator+=( const char *a );
	arcNetString &			operator+=( const float a );
	arcNetString &			operator+=( const char a );
	arcNetString &			operator+=( const int a );
	arcNetString &			operator+=( const unsigned a );
	arcNetString &			operator+=( const bool a );

						// case sensitive compare
	friend bool			operator==( const arcNetString &a, const arcNetString &b );
	friend bool			operator==( const arcNetString &a, const char *b );
	friend bool			operator==( const char *a, const arcNetString &b );

						// case sensitive compare
	friend bool			operator!=( const arcNetString &a, const arcNetString &b );
	friend bool			operator!=( const arcNetString &a, const char *b );
	friend bool			operator!=( const char *a, const arcNetString &b );

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
	void				Append( const arcNetString &text );
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
	arcNetString &		RemoveColors( void );

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
	const char *		Left( int len, arcNetString &result ) const;			// store the leftmost 'len' characters in the result
	arcNetString		Left( int len ) const;							// return the leftmost 'len' characters

	const char *		Right( int len, arcNetString &result ) const;			// store the rightmost 'len' characters in the result
	const char *		Mid( int start, int len, arcNetString &result ) const;	// store 'len' characters starting at 'start' in result

	arcNetString		Right( int len ) const;							// return the rightmost 'len' characters
	arcNetString		Mid( int start, int len ) const;				// return 'len' characters starting at 'start'
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
	arcNetString &		StripQuotes( void );							// strip quotes around string
	void				Replace( const char *old, const char *nw );

	void				ReplaceFirst( const char *old, const char *nw );
	void				ReplaceChar( char oldChar, char newChar );

	void				EraseRange( int start, int len = INVALID_POSITION );
	void				EraseChar( const char c, int start = 0 );

	// file name methods
	int					FileNameHash( void ) const;						// hash key for the filename (skips extension)
	idStr &				CollapsePath( void );							// where possible removes /../ and /./ from path

	arcNetString &		BackSlashesToSlashes( void );					// convert slashes
	idStr &				SlashesToBackSlashes( void );					// convert slashes

	arcNetString &		SetFileExtension( const char *extension );		// set the given file extension
	arcNetString &		StripFileExtension( void );						// remove any file extension
	arcNetString &		StripAbsoluteFileExtension( void );				// remove any file extension looking from front (useful if there are multiple .'s)
	arcNetString &		DefaultFileExtension( const char *extension );	// if there's no file extension use the default
	arcNetString &		DefaultPath( const char *basepath );			// if there's no path use the default
	void				AppendPath( const char *text );					// append a partial path
	arcNetString &		StripFilename( void );							// remove the filename from a path
	arcNetString &		StripPath( void );								// remove the path from the filename
	void				ExtractFilePath( arcNetString &dest ) const;			// copy the file path to another string
	void				ExtractFileName( arcNetString &dest ) const;			// copy the filename to another string
	void				ExtractFileBase( arcNetString &dest ) const;			// copy the filename minus the extension to another string
	void				ExtractFileExtension( arcNetString &dest ) const;		// copy the file extension to another string
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
	static void			StripMediaName( const char *name, arcNetString &mediaName );
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
	static const char * FormatInt( const int num );	 // formats an integer as a value with commas

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
	static const arcVec4&ColorForIndex( int i );
	static const arcVec4&ColorForChar( int c );
	static const char*	StrForColorIndex( int i );
	static int			HexForChar( int c );

	friend int			sprintf( arcNetString &dest, const char *fmt, ... );
	friend int			vsprintf( arcNetString &dest, const char *fmt, va_list ap );

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
	static void			ShowMemoryUsage_f( const arcCommandArgs &args );

	int					DynamicMemoryUsed() const;
	static arcNetString	FormatNumber( int number );

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


ARC_INLINE void arcNetString::EnsureAlloced( int amount, bool keepold ) {
	if ( amount > alloced ) {
		ReAllocate( amount, keepold );
	}
}

ARC_INLINE void arcNetString::Init( void ) {
	len = 0;
	alloced = STR_ALLOC_BASE;
	data = baseBuffer;
	data[ 0 ] = '\0';
#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
	memset( baseBuffer, 0, sizeof( baseBuffer ) );
#endif
}

ARC_INLINE arcNetString::arcNetString( void ) {
	Init();
}

ARC_INLINE arcNetString::arcNetString( const arcNetString &text ) {
	Init();
	int l = text.Length();
	EnsureAlloced( l + 1 );
	strcpy( data, text.data );
	len = l;
}

ARC_INLINE arcNetString::arcNetString( const arcNetString &text, int start, int end ) {
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

ARC_INLINE arcNetString::arcNetString( const char *text ) {
	Init();
	if ( text ) {
		int l = strlen( text );
		EnsureAlloced( l + 1 );
		strcpy( data, text );
		len = l;
	}
}

ARC_INLINE arcNetString::arcNetString( const char *text, int start, int end ) {
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

ARC_INLINE arcNetString::arcNetString( const bool b ) {
	Init();
	EnsureAlloced( 2 );
	data[ 0 ] = b ? '1' : '0';
	data[ 1 ] = '\0';
	len = 1;
}

ARC_INLINE arcNetString::arcNetString( const char c ) {
	Init();
	EnsureAlloced( 2 );
	data[ 0 ] = c;
	data[ 1 ] = '\0';
	len = 1;
}

ARC_INLINE arcNetString::arcNetString( const int i ) {
	char text[ 64 ];

	Init();
	int l = sprintf( text, "%d", i );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ARC_INLINE arcNetString::arcNetString( const unsigned u ) {
	char text[ 64 ];
	Init();
	int l = sprintf( text, "%u", u );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ARC_INLINE arcNetString::arcNetString( const float f ) {
	char text[ 64 ];
	Init();
	int l = arcNetString::snPrintf( text, sizeof( text ), "%f", f );
	while ( l > 0 && text[l-1] == '0' ) text[--l] = '\0';
	while ( l > 0 && text[l-1] == '.' ) text[--l] = '\0';
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ARC_INLINE arcNetString::~arcNetString( void ) {
	FreeData();
}

ARC_INLINE size_t arcNetString::Size( void ) const {
	return sizeof( *this ) + Allocated();
}

ARC_INLINE const char *arcNetString::c_str( void ) const {
	return data;
}

ARC_INLINE arcNetString::operator const char *( void ) {
	return c_str();
}

ARC_INLINE arcNetString::operator const char *( void ) const {
	return c_str();
}

ARC_INLINE char arcNetString::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[index];
}

ARC_INLINE char &arcNetString::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[index];
}

ARC_INLINE void arcNetString::operator=( const arcNetString &text ) {
	int l = text.Length();
	EnsureAlloced( l + 1, false );
	memcpy( data, text.data, l );
	data[l] = '\0';
	len = l;
}

ARC_INLINE arcNetString operator+( const arcNetString &a, const arcNetString &b ) {
	arcNetString result( a );
	result.Append( b );
	return result;
}

ARC_INLINE arcNetString operator+( const arcNetString &a, const char *b ) {
	arcNetString result( a );
	result.Append( b );
	return result;
}

ARC_INLINE arcNetString operator+( const char *a, const arcNetString &b ) {
	arcNetString result( a );
	result.Append( b );
	return result;
}

ARC_INLINE arcNetString operator+( const arcNetString &a, const bool b ) {
	arcNetString result( a );
	result.Append( b ? "true" : "false" );
	return result;
}

ARC_INLINE arcNetString operator+( const arcNetString &a, const char b ) {
	arcNetString result( a );
	result.Append( b );
	return result;
}

ARC_INLINE arcNetString operator+( const arcNetString &a, const float b ) {
	char	text[ 64 ];
	arcNetString	result( a );

	sprintf( text, "%f", b );
	result.Append( text );

	return result;
}

ARC_INLINE arcNetString operator+( const arcNetString &a, const int b ) {
	char	text[ 64 ];
	arcNetString	result( a );

	sprintf( text, "%d", b );
	result.Append( text );

	return result;
}

ARC_INLINE arcNetString operator+( const arcNetString &a, const unsigned b ) {
	char	text[ 64 ];
	arcNetString	result( a );

	sprintf( text, "%u", b );
	result.Append( text );

	return result;
}

ARC_INLINE arcNetString &arcNetString::operator+=( const float a ) {
	char text[ 64 ];

	sprintf( text, "%f", a );
	Append( text );

	return *this;
}

ARC_INLINE arcNetString &arcNetString::operator+=( const int a ) {
	char text[ 64 ];

	sprintf( text, "%d", a );
	Append( text );

	return *this;
}

ARC_INLINE arcNetString &arcNetString::operator+=( const unsigned a ) {
	char text[ 64 ];

	sprintf( text, "%u", a );
	Append( text );

	return *this;
}

ARC_INLINE arcNetString &arcNetString::operator+=( const arcNetString &a ) {
	Append( a );
	return *this;
}

ARC_INLINE arcNetString &arcNetString::operator+=( const char *a ) {
	Append( a );
	return *this;
}

ARC_INLINE arcNetString &arcNetString::operator+=( const char a ) {
	Append( a );
	return *this;
}

ARC_INLINE arcNetString &arcNetString::operator+=( const bool a ) {
	Append( a ? "true" : "false" );
	return *this;
}

ARC_INLINE bool operator==( const arcNetString &a, const arcNetString &b ) {
	return ( !arcNetString::Cmp( a.data, b.data ) );
}

ARC_INLINE bool operator==( const arcNetString &a, const char *b ) {
	assert( b );
	return ( !arcNetString::Cmp( a.data, b ) );
}

ARC_INLINE bool operator==( const char *a, const arcNetString &b ) {
	assert( a );
	return ( !arcNetString::Cmp( a, b.data ) );
}

ARC_INLINE bool operator!=( const arcNetString &a, const arcNetString &b ) {
	return !( a == b );
}

ARC_INLINE bool operator!=( const arcNetString &a, const char *b ) {
	return !( a == b );
}

ARC_INLINE bool operator!=( const char *a, const arcNetString &b ) {
	return !( a == b );
}

ARC_INLINE int arcNetString::Cmp( const char *text ) const {
	assert( text );
	return arcNetString::Cmp( data, text );
}

ARC_INLINE int arcNetString::Cmpn( const char *text, int n ) const {
	assert( text );
	return arcNetString::Cmpn( data, text, n );
}

ARC_INLINE int arcNetString::CmpPrefix( const char *text ) const {
	assert( text );
	return arcNetString::Cmpn( data, text, strlen( text ) );
}

ARC_INLINE int arcNetString::Icmp( const char *text ) const {
	assert( text );
	return arcNetString::Icmp( data, text );
}

ARC_INLINE int arcNetString::Icmpn( const char *text, int n ) const {
	assert( text );
	return arcNetString::Icmpn( data, text, n );
}

ARC_INLINE int arcNetString::IcmpPrefix( const char *text ) const {
	assert( text );
	return arcNetString::Icmpn( data, text, strlen( text ) );
}

ARC_INLINE int arcNetString::IcmpNoColor( const char *text ) const {
	assert( text );
	return arcNetString::IcmpNoColor( data, text );
}

ARC_INLINE int arcNetString::IcmpPath( const char *text ) const {
	assert( text );
	return arcNetString::IcmpPath( data, text );
}

ARC_INLINE int arcNetString::IcmpnPath( const char *text, int n ) const {
	assert( text );
	return arcNetString::IcmpnPath( data, text, n );
}

ARC_INLINE int arcNetString::IcmpPrefixPath( const char *text ) const {
	assert( text );
	return arcNetString::IcmpnPath( data, text, strlen( text ) );
}

ARC_INLINE int arcNetString::Length( void ) const {
	return len;
}

ARC_INLINE int arcNetString::Allocated( void ) const {
	if ( data != baseBuffer ) {
		return alloced;
	} else {
		return 0;
	}
}

ARC_INLINE void arcNetString::Empty( void ) {
	EnsureAlloced( 1 );
	data[ 0 ] = '\0';
	len = 0;
}

ARC_INLINE bool arcNetString::IsEmpty( void ) const {
	return ( arcNetString::Cmp( data, "" ) == 0 );
}

ARC_INLINE void arcNetString::Clear( void ) {
	FreeData();
	Init();
}

ARC_INLINE void arcNetString::Append( const char a ) {
	EnsureAlloced( len + 2 );
	data[ len ] = a;
	len++;
	data[ len ] = '\0';
}

ARC_INLINE void arcNetString::Append( const arcNetString &text ) {
	int newLen = len + text.Length();
	EnsureAlloced( newLen + 1 );
	for ( int i = 0; i < text.len; i++ ) {
		data[ len + i ] = text[ i ];
	}
	len = newLen;
	data[ len ] = '\0';
}

ARC_INLINE void arcNetString::Append( const char *text ) {
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

ARC_INLINE void arcNetString::Append( const char *text, int l ) {
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

ARC_INLINE void arcNetString::Insert( const char a, int index ) {
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

ARC_INLINE void arcNetString::Insert( const char *text, int index ) {
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

ARC_INLINE void arcNetString::ToLower( void ) {
	for ( int i = 0; data[i]; i++ ) {
		if ( CharIsUpper( data[i] ) ) {
			data[i] += ( 'a' - 'A' );
		}
	}
}

ARC_INLINE void arcNetString::ToUpper( void ) {
	for ( int i = 0; data[i]; i++ ) {
		if ( CharIsLower( data[i] ) ) {
			data[i] -= ( 'a' - 'A' );
		}
	}
}

ARC_INLINE bool arcNetString::IsNumeric( void ) const {
	return arcNetString::IsNumeric( data );
}

ARC_INLINE bool arcNetString::IsColor( void ) const {
	return arcNetString::IsColor( data );
}

ARC_INLINE bool arcNetString::HasLower( void ) const {
	return arcNetString::HasLower( data );
}

ARC_INLINE bool arcNetString::HasUpper( void ) const {
	return arcNetString::HasUpper( data );
}

ARC_INLINE arcNetString &arcNetString::RemoveColors( void ) {
	arcNetString::RemoveColors( data );
	len = Length( data );
	return *this;
}

ARC_INLINE int arcNetString::LengthWithoutColors( void ) const {
	return arcNetString::LengthWithoutColors( data );
}

ARC_INLINE void arcNetString::CapLength( int newlen ) {
	if ( len <= newlen ) {
		return;
	}
	data[ newlen ] = 0;
	len = newlen;
}

ARC_INLINE void arcNetString::Fill( const char ch, int newlen ) {
	EnsureAlloced( newlen + 1 );
	len = newlen;
	memset( data, ch, len );
	data[ len ] = 0;
}

ARC_INLINE int arcNetString::Find( const char c, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return arcNetString::FindChar( data, c, start, end );
}

ARC_INLINE int arcNetString::Find( const char *text, bool casesensitive, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return arcNetString::FindText( data, text, casesensitive, start, end );
}

ARC_INLINE bool arcNetString::Filter( const char *filter, bool casesensitive ) const {
	return arcNetString::Filter( filter, data, casesensitive );
}

ARC_INLINE const char *arcNetString::Left( int len, arcNetString &result ) const {
	return Mid( 0, len, result );
}

ARC_INLINE const char *arcNetString::Right( int len, arcNetString &result ) const {
	if ( len >= Length() ) {
		result = *this;
		return result;
	}
	return Mid( Length() - len, len, result );
}

ARC_INLINE arcNetString arcNetString::Left( int len ) const {
	return Mid( 0, len );
}

ARC_INLINE arcNetString arcNetString::Right( int len ) const {
	if ( len >= Length() ) {
		return *this;
	}
	return Mid( Length() - len, len );
}

ARC_INLINE void arcNetString::Strip( const char c ) {
	StripLeading( c );
	StripTrailing( c );
}

ARC_INLINE void arcNetString::Strip( const char *string ) {
	StripLeading( string );
	StripTrailing( string );
}

ARC_INLINE bool arcNetString::CheckExtension( const char *ext ) {
	return arcNetString::CheckExtension( data, ext );
}

ARC_INLINE int arcNetString::Length( const char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		return i;
	}
}

ARC_INLINE char *arcNetString::ToLower( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsUpper( s[i] ) ) {
			s[i] += ( 'a' - 'A' );
		}
	}
	return s;
}

ARC_INLINE char *arcNetString::ToUpper( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsLower( s[i] ) ) {
			s[i] -= ( 'a' - 'A' );
		}
	}
	return s;
}

ARC_INLINE int arcNetString::Hash( const char *string ) {
	int hash = 0;
	for ( int i = 0; *string != '\0'; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

ARC_INLINE int arcNetString::Hash( const char *string, int length ) {
	int hash = 0;
	for ( int i = 0; i < length; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

ARC_INLINE int arcNetString::IHash( const char *string ) {
	int hash = 0;
	for ( int i = 0; *string != '\0'; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

ARC_INLINE int arcNetString::IHash( const char *string, int length ) {
	int hash = 0;
	for ( int i = 0; i < length; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

ARC_INLINE bool arcNetString::IsColor( const char *s ) {
	return ( s[0] == C_COLOR_ESCAPE && s[1] != '\0' && s[1] != ' ' );
}

ARC_INLINE char arcNetString::ToLower( char c ) {
	if ( c <= 'Z' && c >= 'A' ) {
		return ( c + ( 'a' - 'A' ) );
	}
	return c;
}

ARC_INLINE char arcNetString::ToUpper( char c ) {
	if ( c >= 'a' && c <= 'z' ) {
		return ( c - ( 'a' - 'A' ) );
	}
	return c;
}

ARC_INLINE bool arcNetString::CharIsPrintable( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 0x20 && c <= 0x7E ) || ( c >= 0xA1 && c <= 0xFF );
}

ARC_INLINE bool arcNetString::CharIsLower( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 'a' && c <= 'z' ) || ( c >= 0xE0 && c <= 0xFF );
}

ARC_INLINE bool arcNetString::CharIsUpper( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c <= 'Z' && c >= 'A' ) || ( c >= 0xC0 && c <= 0xDF );
}

ARC_INLINE bool arcNetString::CharIsAlpha( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= 0xC0 && c <= 0xFF ) );
}

ARC_INLINE bool arcNetString::CharIsNumeric( int c ) {
	return ( c <= '9' && c >= '0' );
}

ARC_INLINE bool arcNetString::CharIsNewLine( char c ) {
	return ( c == '\n' || c == '\r' || c == '\v' );
}

ARC_INLINE bool arcNetString::CharIsTab( char c ) {
	return ( c == '\t' );
}

ARC_INLINE int arcNetString::ColorIndex( int c ) {
	return ( c & 15 );
}

ARC_INLINE int arcNetString::DynamicMemoryUsed() const {
	return ( data == baseBuffer ) ? 0 : alloced;
}

template< int _size_ >
class aRcStaticString : public arcNetString {
public:
ARC_INLINE void operator=( const aRcStaticString &text ) {
	// we should only get here when the types, including the size, are identical
	len = text.Length();
	memcpy( data, text.data, len+1 );
}

// all arcNetString operators are overloaded and the arcNetString default constructor is called so that the
// static buffer can be initialized in the body of the constructor before the data is ever
// copied.
ARC_INLINE	aRcStaticString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
}

ARC_INLINE	aRcStaticString( const aRcStaticString & text ) :
	arcNetString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	arcNetString::operator=( text );
}

ARC_INLINE	aRcStaticString( const arcNetString & text ) :
	arcNetString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	arcNetString::operator=( text );
}

ARC_INLINE	aRcStaticString( const aRcStaticString & text, int start, int end ) :
	arcNetString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	CopyRange( text.c_str(), start, end );
}

ARC_INLINE	aRcStaticString( const char * text ) :
	arcNetString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	arcNetString::operator=( text );
}

ARC_INLINE	aRcStaticString( const char * text, int start, int end ) :
	arcNetString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	CopyRange( text, start, end );
}

ARC_INLINE	explicit aRcStaticString( const bool b ) :
	arcNetString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	arcNetString::operator=( b );
}

ARC_INLINE	explicit aRcStaticString( const char c ) :
	arcNetString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	arcNetString::operator=( c );
}

ARC_INLINE	explicit aRcStaticString( const int i ) :
	arcNetString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	arcNetString::operator=( i );
}

ARC_INLINE	explicit aRcStaticString( const unsigned u ) :
	arcNetString() {
	buffer[ 0 ] = '\0';
	SetStaticBuffer( buffer, _size_ );
	arcNetString::operator=( u );
}

ARC_INLINE	explicit aRcStaticString( const float f ) :
	arcNetString() { buffer[ 0 ] = '\0'; SetStaticBuffer( buffer, _size_ ); arcNetString::operator=( f );
}

private:
	char			buffer[ _size_ ];
};

#endif
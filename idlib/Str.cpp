#include "Heap.h"
#include "math/Matrix.h"
#include "math/Vector.h"
#include "precompiled.h"
#pragma hdrstop

#if !defined( ARC_REDIRECT_NEWDELETE ) && !defined( MACOS_X )
	#define USE_STRING_DATA_ALLOCATOR
#endif

#ifdef USE_STRING_DATA_ALLOCATOR
static arcDynamicBlockAlloc<char, 1<<18, 128>	stringDataAllocator;
#endif

arcNetString::HMSTimeFormat_t	arcNetString::defaultHMSFormat;

strColor_t g_color_table[COLOR_BITS+1] = {
	{ arcVec4( 0.0f,		0.0f,		0.0f,  1.0f ), "^0" },			// 0 - S_COLOR_DEFAULT			0
	{ arcVec4( 1.0f,		0.0f,		0.0f,  1.0f ), "^1" }, 			// 1 - S_COLOR_RED				1
	{ arcVec4( 0.0f,		1.0f,		0.0f,  1.0f ), "^2" }, 			// 2 - S_COLOR_GREEN			2
	{ arcVec4( 1.0f,		1.0f,		0.0f,  1.0f ), "^3" }, 			// 3 - S_COLOR_YELLOW			3
	{ arcVec4( 0.0f,		0.0f,		1.0f,  1.0f ), "^4" }, 			// 4 - S_COLOR_BLUE				4
	{ arcVec4( 0.0f,		1.0f,		1.0f,  1.0f ), "^5" }, 			// 5 - S_COLOR_CYAN				5
	{ arcVec4( 1.0f,		0.0f,		1.0f,  1.0f ), "^6" }, 			// 6 - S_COLOR_MAGENTA			6
	{ arcVec4( 1.0f,		1.0f,		1.0f,  1.0f ), "^7" }, 			// 7 - S_COLOR_WHITE			7
	{ arcVec4( 0.5f,		0.5f,		0.5f,  1.0f ), "^8" }, 			// 8 - S_COLOR_GRAY				8
	{ arcVec4( 0.15f,	0.15f,		0.15f, 1.0f ), "^9" }, 			// 9 - S_COLOR_BLACK			9
	{ arcVec4( 0.75f,	0.75f,		0.75f, 1.0f ), "^:" }, 			// : - lt.grey					10
	{ arcVec4( 0.25f,	0.25f,		0.25f, 1.0f ), "^;" }, 			// ; - dk.grey					11
	{ arcVec4( 0.0f,		0.5f,		0.0f,  1.0f ), "^<" }, 			// < - md.green					12
	{ arcVec4( 0.5f,		0.5f,		0.0f,  1.0f ), "^=" }, 			// = - md.yellow				13
	{ arcVec4( 0.0f,		0.0f,		0.5f,  1.0f ), "^>" }, 			// > - md.blue					14
	{ arcVec4( 0.5f,		0.0f,		0.0f,  1.0f ), "^?" }, 			// ? - md.red					15
	{ arcVec4( 0.5f,		0.25,		0.0f,  1.0f ), "^@" }, 			// @ - md.orange				16
	{ arcVec4( 1.0f,		0.6f,		0.1f,  1.0f ), "^A" }, 			// A - lt.orange				17
	{ arcVec4( 0.0f,		0.5f,		0.5f,  1.0f ), "^B" }, 			// B - md.cyan					18
	{ arcVec4( 0.5f,		0.0f,		0.5f,  1.0f ), "^C" }, 			// C - md.purple				19
	{ arcVec4( 1.0f,		0.5f,		0.0f,  1.0f ), "^D" }, 			// D - orange					20
	{ arcVec4( 0.5f,		0.0f,		1.0f,  1.0f ), "^E" }, 			// E							21
	{ arcVec4( 0.2f,		0.6f,		0.8f,  1.0f ), "^F" }, 			// F							22
	{ arcVec4( 0.8f,		1.0f,		0.8f,  1.0f ), "^G" }, 			// G							23
	{ arcVec4( 0.0f,		0.4f,		0.2f,  1.0f ), "^H" }, 			// H							24
	{ arcVec4( 1.0f,		0.0f,		0.2f,  1.0f ), "^I" }, 			// I							25
	{ arcVec4( 0.7f,		0.1f,		0.1f,  1.0f ), "^J" }, 			// J							26
	{ arcVec4( 0.6f,		0.2f,		0.0f,  1.0f ), "^K" }, 			// K							27
	{ arcVec4( 0.8f,		0.6f,		0.2f,  1.0f ), "^L" }, 			// L							28
	{ arcVec4( 0.6f,		0.6f,		0.2f,  1.0f ), "^M" }, 			// M							29
	{ arcVec4( 1.0f,		1.0f,		0.75f, 1.0f ), "^N" }, 			// N							30
	{ arcVec4( 1.0f,		1.0f,		0.5f,  1.0f ), "^O" }, 			// O							31
};

const char *units[2][4] = {
	{ "B", "KB", "MB", "GB" },
	{ "B/s", "KB/s", "MB/s", "GB/s" }
};
/*
============
arcNetString::ColorForIndex
============
*/
const arcVec4 &arcNetString::ColorForIndex( int i ) {
	return g_color_table[ i & COLOR_BITS ].color;
}

/*
============
arcNetString::ColorForIndex
============
*/
const arcVec4 &arcNetString::ColorForChar( int c ) {
	return g_color_table[ ColorIndex( c ) ].color;
}

/*
============
arcNetString::ColorForIndex
============
*/
const char *arcNetString::StrForColorIndex( int i ) {
	return g_color_table[ i & COLOR_BITS ].str; //return g_color_table[ i & 15 ];
}

/*
============
arcNetString::ReAllocate
============
*/
void arcNetString::ReAllocate( int amount, bool keepOld ) {
	assert( amount > 0 );

	int mod = amount % STR_ALLOC_GRAN;
	if ( !mod ) {
		int newSize = amount;
	} else {
		int newSize = amount + STR_ALLOC_GRAN - mod;
	}
	alloced = newSize;

	char *newBuffer = stringDataAllocator.Alloc( alloced );

	//newBuffer = new char[ alloced ];
	if ( keepOld && data ) {
		data[ len ] = '\0';
		strcpy( newBuffer, data );
	}

	if ( data && data != baseBuffer ) {
		stringDataAllocator.Free( data );
		//delete [] data;
	}

	data = newBuffer;
}

/*
============
arcNetString::FreeData
============
*/
void arcNetString::FreeData( void ) {
	if ( data && data != baseBuffer ) {
		stringDataAllocator.Free( data );
		//delete[] data;
		data = baseBuffer;
	}
}

/*
============
arcNetString::operator=
============
*/
void arcNetString::operator=( const char *text ) {
	int l;
	int diff;
	int i;

	if ( !text ) {
		// safe behaviour if NULL
		EnsureAlloced( 1, false );
		data[ 0 ] = '\0';
		len = 0;
		return;
	}

	if ( text == data ) {
		return; // copying same thing
	}

	// check if we're aliasing
	if ( text >= data && text <= data + len ) {
		diff = text - data;

		assert( strlen( text ) < (unsigned)len );

		for ( i = 0; text[ i ]; i++ ) {
			data[ i ] = text[ i ];
		}

		data[ i ] = '\0';

		len -= diff;

		return;
	}

	l = strlen( text );
	EnsureAlloced( l + 1, false );
	strcpy( data, text );
	len = l;
}

/*
============
arcNetString::FindChar

returns -1 if not found otherwise the index of the char
============
*/
int arcNetString::FindChar( const char *str, const char c, int start, int end ) {
	if ( end == -1 ) {
		end = strlen( str ) - 1;
	}
	for ( int i = start; i <= end; i++ ) {
		if ( str[i] == c ) {
			return i;
		}
	}
	return -1;
}

/*
============
arcNetString::FindText

returns -1 if not found otherwise the index of the text
============
*/
int arcNetString::FindText( const char *str, const char *text, bool casesensitive, int start, int end ) {
	int l, i, j;

	if ( end == -1 ) {
		end = strlen( str );
	}
	l = end - strlen( text );
	for ( i = start; i <= l; i++ ) {
		if ( casesensitive ) {
			for ( j = 0; text[j]; j++ ) {
				if ( str[i+j] != text[j] ) {
					break;
				}
			}
		} else {
			for ( j = 0; text[j]; j++ ) {
				if ( ::toupper( str[i+j] ) != ::toupper( text[j] ) ) {
					break;
				}
			}
		}
		if ( !text[j] ) {
			return i;
		}
	}
	return -1;
}

/*
============
arcNetString::Filter

Returns true if the string conforms the given filter.
Several metacharacter may be used in the filter.

*          match any string of zero or more characters
?          match any single character
[abc...]   match any of the enclosed characters; a hyphen can
           be used to specify a range (e.g. a-z, A-Z, 0-9)

============
*/
bool arcNetString::Filter( const char *filter, const char *name, bool casesensitive ) {
	arcNetString buf;
	int i, found, index;

	while(*filter) {
		if (*filter == '*') {
			filter++;
			buf.Empty();
			for ( i = 0; *filter; i++ ) {
				if ( *filter == '*' || *filter == '?' || (*filter == '[' && *(filter+1 ) != '[') ) {
					break;
				}
				buf += *filter;
				if ( *filter == '[' ) {
					filter++;
				}
				filter++;
			}
			if ( buf.Length() ) {
				index = arcNetString( name ).Find( buf.c_str(), casesensitive );
				if ( index == -1 ) {
					return false;
				}
				name += index + strlen(buf);
			}
		} else if (*filter == '?') {
			filter++;
			name++;
		} else if (*filter == '[') {
			if ( *(filter+1 ) == '[' ) {
				if ( *name != '[' ) {
					return false;
				}
				filter += 2;
				name++;
			} else {
				filter++;
				found = false;
				while(*filter && !found) {
					if (*filter == ']' && *(filter+1 ) != ']') {
						break;
					}
					if (*(filter+1 ) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']') ) {
						if (casesensitive) {
							if (*name >= *filter && *name <= *(filter+2) ) {
								found = true;
							}
						} else {
							if ( ::toupper(*name) >= ::toupper(*filter) && ::toupper(*name) <= ::toupper(*(filter+2) ) ) {
								found = true;
							}
						}
						filter += 3;
					} else {
						if (casesensitive) {
							if (*filter == *name) {
								found = true;
							}
						} else {
							if ( ::toupper(*filter) == ::toupper(*name) ) {
								found = true;
							}
						}
						filter++;
					}
				}
				if ( !found) {
					return false;
				}
				while(*filter) {
					if ( *filter == ']' && *(filter+1 ) != ']' ) {
						break;
					}
					filter++;
				}
				filter++;
				name++;
			}
		} else {
			if (casesensitive) {
				if (*filter != *name) {
					return false;
				}
			} else {
				if ( ::toupper(*filter) != ::toupper(*name) ) {
					return false;
				}
			}
			filter++;
			name++;
		}
	}
	return true;
}

/*
=============
arcNetString::StripMediaName

  makes the string lower case, replaces backslashes with forward slashes, and removes extension
=============
*/
void arcNetString::StripMediaName( const char *name, arcNetString &mediaName ) {
	char c;

	mediaName.Empty();

	for ( c = *name; c; c = *(++name) ) {
		// truncate at an extension
		if ( c == '.' ) {
			break;
		}
		// convert backslashes to forward slashes
		if ( c == '\\' ) {
			mediaName.Append( '/' );
		} else {
			mediaName.Append( arcNetString::ToLower( c ) );
		}
	}
}

/*
=============
arcNetString::CheckExtension
=============
*/
bool arcNetString::CheckExtension( const char *name, const char *ext ) {
	const char *s1 = name + Length( name ) - 1;
	const char *s2 = ext + Length( ext ) - 1;
	int c1, c2, d;

	do {
		c1 = *s1--;
		c2 = *s2--;

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return false;
		}
	} while( s1 > name && s2 > ext );

	return ( s1 >= name );
}

/*
=============
arcNetString::FloatArrayToString
=============
*/
const char *arcNetString::FloatArrayToString( const float *array, const int length, const int precision ) {
	static int index = 0;
	static char str[4][16384];	// in case called by nested functions
	int i, n;
	char format[16], *s;

	// use an array of string so that multiple calls won't collide
	s = str[index];
	index = (index + 1 ) & 3;

	arcNetString::snPrintf( format, sizeof( format ), "%%.%df", precision );
	n = arcNetString::snPrintf( s, sizeof( str[0] ), format, array[0] );
	if ( precision > 0 ) {
		while( n > 0 && s[n-1] == '0' ) s[--n] = '\0';
		while( n > 0 && s[n-1] == '.' ) s[--n] = '\0';
	}
	arcNetString::snPrintf( format, sizeof( format ), " %%.%df", precision );
	for ( i = 1; i < length; i++ ) {
		n += arcNetString::snPrintf( s + n, sizeof( str[0] ) - n, format, array[i] );
		if ( precision > 0 ) {
			while( n > 0 && s[n-1] == '0' ) s[--n] = '\0';
			while( n > 0 && s[n-1] == '.' ) s[--n] = '\0';
		}
	}
	return s;
}

/*
============
arcNetString::Last

returns -1 if not found otherwise the index of the char
============
*/
int arcNetString::Last( const char c ) const {
	int i;

	for ( i = Length(); i > 0; i-- ) {
		if ( data[ i - 1 ] == c ) {
			return i - 1;
		}
	}

	return -1;
}

/*
============
arcNetString::StripLeading
============
*/
void arcNetString::StripLeading( const char c ) {
	while( data[ 0 ] == c ) {
		memmove( &data[ 0 ], &data[ 1 ], len );
		len--;
	}
}

/*
============
arcNetString::StripLeading
============
*/
void arcNetString::StripLeading( const char *string ) {
	int l;

	l = strlen( string );
	if ( l > 0 ) {
		while ( !Cmpn( string, l ) ) {
			memmove( data, data + l, len - l + 1 );
			len -= l;
		}
	}
}

/*
============
arcNetString::StripLeadingOnce
============
*/
bool arcNetString::StripLeadingOnce( const char *string ) {
	int l;

	l = strlen( string );
	if ( ( l > 0 ) && !Cmpn( string, l ) ) {
		memmove( data, data + l, len - l + 1 );
		len -= l;
		return true;
	}
	return false;
}

/*
============
arcNetString::StripTrailing
============
*/
void arcNetString::StripTrailing( const char c ) {
	int i;

	for ( i = Length(); i > 0 && data[ i - 1 ] == c; i-- ) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
arcNetString::StripLeading
============
*/
void arcNetString::StripTrailing( const char *string ) {
	int l;

	l = strlen( string );
	if ( l > 0 ) {
		while ( ( len >= l ) && !Cmpn( string, data + len - l, l ) ) {
			len -= l;
			data[len] = '\0';
		}
	}
}

/*
============
arcNetString::StripTrailingOnce
============
*/
bool arcNetString::StripTrailingOnce( const char *string ) {
	int l;

	l = strlen( string );
	if ( ( l > 0 ) && ( len >= l ) && !Cmpn( string, data + len - l, l ) ) {
		len -= l;
		data[len] = '\0';
		return true;
	}
	return false;
}

/*
============
arcNetString::Replace
============
*/
void arcNetString::Replace( const char *old, const char *nw ) {
	int		oldLen, newLen, i, j, count;
	arcNetString	oldString( data );

	oldLen = strlen( old );
	newLen = strlen( nw );

	// Work out how big the new string will be
	count = 0;
	for ( i = 0; i < oldString.Length(); i++ ) {
		if ( !arcNetString::Cmpn( &oldString[i], old, oldLen ) ) {
			count++;
			i += oldLen - 1;
		}
	}

	if ( count ) {
		EnsureAlloced( len + ( ( newLen - oldLen ) * count ) + 2, false );

		// Replace the old data with the new data
		for ( i = 0, j = 0; i < oldString.Length(); i++ ) {
			if ( !arcNetString::Cmpn( &oldString[i], old, oldLen ) ) {
				memcpy( data + j, nw, newLen );
				i += oldLen - 1;
				j += newLen;
			} else {
				data[j] = oldString[i];
				j++;
			}
		}
		data[j] = 0;
		len = strlen( data );
	}
}

/*
============
arcNetString::Mid
============
*/
const char *arcNetString::Mid( int start, int len, arcNetString &result ) const {
	int i;

	result.Empty();

	i = Length();
	if ( i == 0 || len <= 0 || start >= i ) {
		return NULL;
	}

	if ( start + len >= i ) {
		len = i - start;
	}

	result.Append( &data[ start ], len );
	return result;
}

/*
============
arcNetString::Mid
============
*/
arcNetString arcNetString::Mid( int start, int len ) const {
	int i;
	arcNetString result;

	i = Length();
	if ( i == 0 || len <= 0 || start >= i ) {
		return result;
	}

	if ( start + len >= i ) {
		len = i - start;
	}

	result.Append( &data[ start ], len );
	return result;
}

/*
============
arcNetString::StripTrailingWhitespace
============
*/
void arcNetString::StripTrailingWhitespace( void ) {
	int i;

	// cast to unsigned char to prevent stripping off high-ASCII characters
	for ( i = Length(); i > 0 && (unsigned char)( data[ i - 1 ] ) <= ' '; i-- ) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
arcNetString::StripQuotes

Removes the quotes from the beginning and end of the string
============
*/
arcNetString& arcNetString::StripQuotes( void ) {
	if ( data[0] != '\"' ) {
		return *this;
	}

	// Remove the trailing quote first
	if ( data[len-1] == '\"' ) {
		data[len-1] = '\0';
		len--;
	}

	// Strip the leading quote now
	len--;
	memmove( &data[ 0 ], &data[ 1 ], len );
	data[len] = '\0';

	return *this;
}

/*
=====================================================================

  filename methods

=====================================================================
*/

/*
============
arcNetString::FileNameHash
============
*/
int arcNetString::FileNameHash( void ) const {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while( data[i] != '\0' ) {
		letter = arcNetString::ToLower( data[i] );
		if ( letter == '.' ) {
			break;				// don't include extension
		}
		if ( letter =='\\' ) {
			letter = '/';
		}
		hash += (long)(letter)*( i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1 );
	return hash;
}

/*
============
arcNetString::BackSlashesToSlashes
============
*/
arcNetString &arcNetString::BackSlashesToSlashes( void ) {
	int i;

	for ( i = 0; i < len; i++ ) {
		if ( data[ i ] == '\\' ) {
			data[ i ] = '/';
		}
	}
	return *this;
}

/*
============
arcNetString::SetFileExtension
============
*/
arcNetString &arcNetString::SetFileExtension( const char *extension ) {
	StripFileExtension();
	if ( *extension != '.' ) {
		Append( '.' );
	}
	Append( extension );
	return *this;
}

/*
============
arcNetString::StripFileExtension
============
*/
arcNetString &arcNetString::StripFileExtension( void ) {
	int i;

	for ( i = len-1; i >= 0; i-- ) {
		if ( data[i] == '.' ) {
			data[i] = '\0';
			len = i;
			break;
		}
	}
	return *this;
}

/*
============
arcNetString::StripAbsoluteFileExtension
============
*/
arcNetString &arcNetString::StripAbsoluteFileExtension( void ) {
	int i;

	for ( i = 0; i < len; i++ ) {
		if ( data[i] == '.' ) {
			data[i] = '\0';
			len = i;
			break;
		}
	}

	return *this;
}

/*
==================
arcNetString::DefaultFileExtension
==================
*/
arcNetString &arcNetString::DefaultFileExtension( const char *extension ) {
	int i;

	// do nothing if the string already has an extension
	for ( i = len-1; i >= 0; i-- ) {
		if ( data[i] == '.' ) {
			return *this;
		}
	}
	if ( *extension != '.' ) {
		Append( '.' );
	}
	Append( extension );
	return *this;
}

/*
==================
arcNetString::DefaultPath
==================
*/
arcNetString &arcNetString::DefaultPath( const char *basepath ) {
	if ( ( ( *this )[ 0 ] == '/' ) || ( ( *this )[ 0 ] == '\\' ) ) {
		// absolute path location
		return *this;
	}

	*this = basepath + *this;
	return *this;
}

/*
====================
arcNetString::AppendPath
====================
*/
void arcNetString::AppendPath( const char *text ) {
	int pos;
	int i = 0;

	if ( text && text[i] ) {
		pos = len;
		EnsureAlloced( len + strlen( text ) + 2 );

		if ( pos ) {
			if ( data[ pos-1 ] != '/' ) {
				data[ pos++ ] = '/';
			}
		}
		if ( text[i] == '/' ) {
			i++;
		}

		for (; text[ i ]; i++ ) {
			if ( text[ i ] == '\\' ) {
				data[ pos++ ] = '/';
			} else {
				data[ pos++ ] = text[ i ];
			}
		}
		len = pos;
		data[ pos ] = '\0';
	}
}

/*
==================
arcNetString::StripFilename
==================
*/
arcNetString &arcNetString::StripFilename( void ) {
	int pos;

	pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos ] != '/' ) && ( ( *this )[ pos ] != '\\' ) ) {
		pos--;
	}

	if ( pos < 0 ) {
		pos = 0;
	}

	CapLength( pos );
	return *this;
}

/*
==================
arcNetString::StripPath
==================
*/
arcNetString &arcNetString::StripPath( void ) {
	int pos;

	pos = Length();
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	*this = Right( Length() - pos );
	return *this;
}

/*
====================
arcNetString::ExtractFilePath
====================
*/
void arcNetString::ExtractFilePath( arcNetString &dest ) const {
	int pos;

	//
	// back up until a \ or the start
	//
	pos = Length();
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	Left( pos, dest );
}

/*
====================
arcNetString::ExtractFileName
====================
*/
void arcNetString::ExtractFileName( arcNetString &dest ) const {
	int pos;

	//
	// back up until a \ or the start
	//
	pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	Right( Length() - pos, dest );
}

/*
====================
arcNetString::ExtractFileBase
====================
*/
void arcNetString::ExtractFileBase( arcNetString &dest ) const {
	int pos;
	int start;

	//
	// back up until a \ or the start
	//
	pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	start = pos;
	while( ( pos < Length() ) && ( ( *this )[ pos ] != '.' ) ) {
		pos++;
	}

	Mid( start, pos - start, dest );
}

/*
====================
arcNetString::ExtractFileExtension
====================
*/
void arcNetString::ExtractFileExtension( arcNetString &dest ) const {
	int pos;

	//
	// back up until a . or the start
	//
	pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '.' ) ) {
		pos--;
	}

	if ( !pos ) {
		// no extension
		dest.Empty();
	} else {
		Right( Length() - pos, dest );
	}
}


/*
=====================================================================

  char * methods to replace library functions

=====================================================================
*/

/*
============
arcNetString::IsNumeric

Checks a string to see if it contains only numerical values.
============
*/
bool arcNetString::IsNumeric( const char *s ) {
	int		i;
	bool	dot;

	if ( *s == '-' ) {
		s++;
	}

	dot = false;
	for ( i = 0; s[i]; i++ ) {
		if ( !isdigit( s[i] ) ) {
			if ( ( s[ i ] == '.' ) && !dot ) {
				dot = true;
				continue;
			}
			return false;
		}
	}

	return true;
}

/*
============
arcNetString::HasLower

Checks if a string has any lowercase chars
============
*/
bool arcNetString::HasLower( const char *s ) {
	if ( !s ) {
		return false;
	}

	while ( *s ) {
		if ( CharIsLower( *s ) ) {
			return true;
		}
		s++;
	}

	return false;
}

/*
============
arcNetString::HasUpper

Checks if a string has any uppercase chars
============
*/
bool arcNetString::HasUpper( const char *s ) {
	if ( !s ) {
		return false;
	}

	while ( *s ) {
		if ( CharIsUpper( *s ) ) {
			return true;
		}
		s++;
	}

	return false;
}

/*
================
arcNetString::Cmp
================
*/
int arcNetString::Cmp( const char *s1, const char *s2 ) {
	int c1, c2, d;

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;
		if ( d ) {
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
arcNetString::Cmpn
================
*/
int arcNetString::Cmpn( const char *s1, const char *s2, int n ) {
	int c1, c2, d;

	assert( n >= 0 );

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;
		if ( d ) {
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
arcNetString::Icmp
================
*/
int arcNetString::Icmp( const char *s1, const char *s2 ) {
	int c1, c2, d;

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
arcNetString::Icmpn
================
*/
int arcNetString::Icmpn( const char *s1, const char *s2, int n ) {
	int c1, c2, d;

	assert( n >= 0 );

	do {
		c1 = *s1++;
		c2 = *s2++;
		if ( !n-- ) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
arcNetString::Icmp
================
*/
int arcNetString::IcmpNoColor( const char *s1, const char *s2 ) {
	int c1, c2, d;

	do {
		while ( arcNetString::IsColor( s1 ) ) {
			s1 += 2;
		}
		while ( arcNetString::IsColor( s2 ) ) {
			s2 += 2;
		}
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
arcNetString::IcmpPath
================
*/
int arcNetString::IcmpPath( const char *s1, const char *s2 ) {
	int c1, c2, d;

#if 0
//#if !defined( _WIN32 )
	arcLibrary::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
#endif

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c1 == '\\' ) {
				d += ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 == '\\' ) {
				d -= ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			// make sure folders come first
			while( c1 ) {
				if ( c1 == '/' || c1 == '\\' ) {
					break;
				}
				c1 = *s1++;
			}
			while( c2 ) {
				if ( c2 == '/' || c2 == '\\' ) {
					break;
				}
				c2 = *s2++;
			}
			if ( c1 && !c2 ) {
				return -1;
			} else if ( !c1 && c2 ) {
				return 1;
			}
			// same folder depth so use the regular compare
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;
}

/*
================
arcNetString::IcmpnPath
================
*/
int arcNetString::IcmpnPath( const char *s1, const char *s2, int n ) {
	int c1, c2, d;

#if 0
//#if !defined( _WIN32 )
	arcLibrary::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
#endif

	assert( n >= 0 );

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c1 == '\\' ) {
				d += ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 == '\\' ) {
				d -= ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			// make sure folders come first
			while( c1 ) {
				if ( c1 == '/' || c1 == '\\' ) {
					break;
				}
				c1 = *s1++;
			}
			while( c2 ) {
				if ( c2 == '/' || c2 == '\\' ) {
					break;
				}
				c2 = *s2++;
			}
			if ( c1 && !c2 ) {
				return -1;
			} else if ( !c1 && c2 ) {
				return 1;
			}
			// same folder depth so use the regular compare
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;
}

/*
=============
arcNetString::Copynz

Safe strncpy that ensures a trailing zero
=============
*/
void arcNetString::Copynz( char *dest, const char *src, int destsize ) {
	if ( !src ) {
		arcLibrary::common->Warning( "arcNetString::Copynz: NULL src" );
		return;
	}
	if ( destsize < 1 ) {
		arcLibrary::common->Warning( "arcNetString::Copynz: destsize < 1" );
		return;
	}

	strncpy( dest, src, destsize-1 );
    dest[destsize-1] = 0;
}

/*
================
arcNetString::Append

  never goes past bounds or leaves without a terminating 0
================
*/
void arcNetString::Append( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		arcLibrary::common->Error( "arcNetString::Append: already overflowed" );
	}
	arcNetString::Copynz( dest + l1, src, size - l1 );
}

/*
================
arcNetString::LengthWithoutColors
================
*/
int arcNetString::LengthWithoutColors( const char *s ) {
	int len;
	const char *p;

	if ( !s ) {
		return 0;
	}

	len = 0;
	p = s;
	while( *p ) {
		if ( arcNetString::IsColor( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}

/*
================
arcNetString::RemoveColors
================
*/
char *arcNetString::RemoveColors( char *string ) {
	char *d;
	char *s;
	int c;

	s = string;
	d = string;
	while( (c = *s) != 0 ) {
		if ( arcNetString::IsColor( s ) ) {
			s++;
		}
		else {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
================
arcNetString::snPrintf
================
*/
int arcNetString::snPrintf( char *dest, int size, const char *fmt, ...) {
	int len;
	va_list argptr;
	char buffer[32000];	// big, but small enough to fit in PPC stack

	va_start( argptr, fmt );
	len = vsprintf( buffer, fmt, argptr );
	va_end( argptr );
	if ( len >= sizeof( buffer ) ) {
		arcLibrary::common->Error( "arcNetString::snPrintf: overflowed buffer" );
	}
	if ( len >= size ) {
		arcLibrary::common->Warning( "arcNetString::snPrintf: overflow of %i in %i\n", len, size );
		len = size;
	}
	arcNetString::Copynz( dest, buffer, size );
	return len;
}

/*
============
arcNetString::vsnPrintf

vsnprintf portability:

C99 standard: vsnprintf returns the number of characters (excluding the trailing
'\0') which would have been written to the final string if enough space had been available
snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')

win32: _vsnprintf returns the number of characters written, not including the terminating null character,
or a negative value if an output error occurs. If the number of characters to write exceeds count, then count
characters are written and -1 is returned and no trailing '\0' is added.

arcNetString::vsnPrintf: always appends a trailing '\0', returns number of characters written (not including terminal \0 )
or returns -1 on failure or if the buffer would be overflowed.
============
*/
int arcNetString::vsnPrintf( char *dest, int size, const char *fmt, va_list argptr ) {
	int ret;

#ifdef _WIN32
#undef _vsnprintf
	ret = _vsnprintf( dest, size-1, fmt, argptr );
#define _vsnprintf	use_arcStr_vsnPrintf
#else
#undef vsnprintf
	ret = vsnprintf( dest, size, fmt, argptr );
#define vsnprintf	use_arcStr_vsnPrintf
#endif
	dest[size-1] = '\0';
	if ( ret < 0 || ret >= size ) {
		return -1;
	}
	return ret;
}

/*
============
sprintf

Sets the value of the string using a printf interface.
============
*/
int sprintf( arcNetString &string, const char *fmt, ... ) {
	int l;
	va_list argptr;
	char buffer[32000];

	va_start( argptr, fmt );
	l = arcNetString::vsnPrintf( buffer, sizeof(buffer)-1, fmt, argptr );
	va_end( argptr );
	buffer[sizeof(buffer)-1] = '\0';

	string = buffer;
	return l;
}

/*
============
vsprintf

Sets the value of the string using a vprintf interface.
============
*/
int vsprintf( arcNetString &string, const char *fmt, va_list argptr ) {
	int l;
	char buffer[32000];

	l = arcNetString::vsnPrintf( buffer, sizeof(buffer)-1, fmt, argptr );
	buffer[sizeof(buffer)-1] = '\0';

	string = buffer;
	return l;
}

/*
============
va

does a varargs printf into a temp buffer
NOTE: not thread safe
============
*/
char *va( const char *fmt, ... ) {
	va_list argptr;
	static int index = 0;
	static char string[4][16384];	// in case called by nested functions
	char *buf;

	buf = string[index];
	index = (index + 1 ) & 3;

	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr );

	return buf;
}

/*
============
arcNetString::BestUnit
============
*/
int arcNetString::BestUnit( const char *format, float value, Measure_t measure ) {
	int unit = 1;
	while ( unit <= 3 && ( 1 << ( unit * 10 ) < value ) ) {
		unit++;
	}
	unit--;
	value /= 1 << ( unit * 10 );
	sprintf( *this, format, value );
	*this += " ";
	*this += units[ measure ][ unit ];
	return unit;
}

/*
============
arcNetString::SetUnit
============
*/
void arcNetString::SetUnit( const char *format, float value, int unit, Measure_t measure ) {
	value /= 1 << ( unit * 10 );
	sprintf( *this, format, value );
	*this += " ";
	*this += units[ measure ][ unit ];
}

/*
================
arcNetString::InitMemory
================
*/
void arcNetString::InitMemory( void ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Init();
#endif
}

/*
================
arcNetString::ShutdownMemory
================
*/
void arcNetString::ShutdownMemory( void ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Shutdown();
#endif
}

/*
================
arcNetString::PurgeMemory
================
*/
void arcNetString::PurgeMemory( void ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.FreeEmptyBaseBlocks();
#endif
}

/*
================
arcNetString::ShowMemoryUsage_f
================
*/
void arcNetString::ShowMemoryUsage_f( const arcCommandArgs &args ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	arcLibrary::common->Printf( "%6d KB string memory (%d KB free in %d blocks, %d empty base blocks)\n",
		stringDataAllocator.GetBaseBlockMemory() >> 10, stringDataAllocator.GetFreeBlockMemory() >> 10,
			stringDataAllocator.GetNumFreeBlocks(), stringDataAllocator.GetNumEmptyBaseBlocks() );
#endif
}

/*
================
arcNetString::FormatNumber
================
*/
struct formatList_t {
	int			gran;
	int			count;
};

// elements of list need to decend in size
formatList_t formatList[] = {
	{ 1000000000, 0 },
	{ 1000000, 0 },
	{ 1000, 0 }
};

int numFormatList = sizeof(formatList) / sizeof( formatList[0] );


arcNetString arcNetString::FormatNumber( int number ) {
	arcNetString string;
	bool hit;

	// reset
	for ( int i = 0; i < numFormatList; i++ ) {
		formatList_t *li = formatList + i;
		li->count = 0;
	}

	// main loop
	do {
		hit = false;
		for ( int i = 0; i < numFormatList; i++ ) {
			formatList_t *li = formatList + i;
			if ( number >= li->gran ) {
				li->count++;
				number -= li->gran;
				hit = true;
				break;
			}
		}
	} while ( hit );

	// print out
	bool found = false;

	for ( int i = 0; i < numFormatList; i++ ) {
		formatList_t *li = formatList + i;
		if ( li->count ) {
			if ( !found ) {
				string += va( "%i,", li->count );
			} else {
				string += va( "%3.3i,", li->count );
			}
			found = true;
		} else if ( found ) {
			string += va( "%3.3i,", li->count );
		}
	}

	if ( found ) {
		string += va( "%3.3i", number );
	} else {
		string += va( "%i", number );
	}

	// pad to proper size
	int count = 11 - string.Length();

	for ( int i = 0; i < count; i++ ) {
		string.Insert( " ", 0 );
	}

	return string;
}


#include "Heap.h"
#include "math/Matrix.h"
#include "math/Vector.h"
#include "Lib.h"
#pragma hdrstop

#if !defined( ARC_REDIRECT_NEWDELETE ) && !defined( MACOS_X )
	#define USE_STRING_DATA_ALLOCATOR
#endif
#ifdef USE_STRING_DATA_ALLOCATOR
static anDynamicBlockAlloc<char, 1<<18, 128>	stringDataAllocator;
#endif

anStr::HMSTimeFormat_t	anStr::defaultHMSFormat;

strColor_t g_color_table[COLOR_BITS+1] = {
	{ anVec4( 0.0f,		0.0f,		0.0f,  1.0f ), "^0" },			// 0 - S_COLOR_DEFAULT			0
	{ anVec4( 1.0f,		0.0f,		0.0f,  1.0f ), "^1" }, 			// 1 - S_COLOR_RED				1
	{ anVec4( 0.0f,		1.0f,		0.0f,  1.0f ), "^2" }, 			// 2 - S_COLOR_GREEN			2
	{ anVec4( 1.0f,		1.0f,		0.0f,  1.0f ), "^3" }, 			// 3 - S_COLOR_YELLOW			3
	{ anVec4( 0.0f,		0.0f,		1.0f,  1.0f ), "^4" }, 			// 4 - S_COLOR_BLUE				4
	{ anVec4( 0.0f,		1.0f,		1.0f,  1.0f ), "^5" }, 			// 5 - S_COLOR_CYAN				5
	{ anVec4( 1.0f,		0.0f,		1.0f,  1.0f ), "^6" }, 			// 6 - S_COLOR_MAGENTA			6
	{ anVec4( 1.0f,		1.0f,		1.0f,  1.0f ), "^7" }, 			// 7 - S_COLOR_WHITE			7
	{ anVec4( 0.5f,		0.5f,		0.5f,  1.0f ), "^8" }, 			// 8 - S_COLOR_GRAY				8
	{ anVec4( 0.15f,	0.15f,		0.15f, 1.0f ), "^9" }, 			// 9 - S_COLOR_BLACK			9
	{ anVec4( 0.75f,	0.75f,		0.75f, 1.0f ), "^:" }, 			// : - lt.grey					10
	{ anVec4( 0.25f,	0.25f,		0.25f, 1.0f ), "^;" }, 			// ; - dk.grey					11
	{ anVec4( 0.0f,		0.5f,		0.0f,  1.0f ), "^<" }, 			// < - md.green					12
	{ anVec4( 0.5f,		0.5f,		0.0f,  1.0f ), "^=" }, 			// = - md.yellow				13
	{ anVec4( 0.0f,		0.0f,		0.5f,  1.0f ), "^>" }, 			// > - md.blue					14
	{ anVec4( 0.5f,		0.0f,		0.0f,  1.0f ), "^?" }, 			// ? - md.red					15
	{ anVec4( 0.5f,		0.25,		0.0f,  1.0f ), "^@" }, 			// @ - md.orange				16
	{ anVec4( 1.0f,		0.6f,		0.1f,  1.0f ), "^A" }, 			// A - lt.orange				17
	{ anVec4( 0.0f,		0.5f,		0.5f,  1.0f ), "^B" }, 			// B - md.cyan					18
	{ anVec4( 0.5f,		0.0f,		0.5f,  1.0f ), "^C" }, 			// C - md.purple				19
	{ anVec4( 1.0f,		0.5f,		0.0f,  1.0f ), "^D" }, 			// D - orange					20
	{ anVec4( 0.5f,		0.0f,		1.0f,  1.0f ), "^E" }, 			// E							21
	{ anVec4( 0.2f,		0.6f,		0.8f,  1.0f ), "^F" }, 			// F							22
	{ anVec4( 0.8f,		1.0f,		0.8f,  1.0f ), "^G" }, 			// G							23
	{ anVec4( 0.0f,		0.4f,		0.2f,  1.0f ), "^H" }, 			// H							24
	{ anVec4( 1.0f,		0.0f,		0.2f,  1.0f ), "^I" }, 			// I							25
	{ anVec4( 0.7f,		0.1f,		0.1f,  1.0f ), "^J" }, 			// J							26
	{ anVec4( 0.6f,		0.2f,		0.0f,  1.0f ), "^K" }, 			// K							27
	{ anVec4( 0.8f,		0.6f,		0.2f,  1.0f ), "^L" }, 			// L							28
	{ anVec4( 0.6f,		0.6f,		0.2f,  1.0f ), "^M" }, 			// M							29
	{ anVec4( 1.0f,		1.0f,		0.75f, 1.0f ), "^N" }, 			// N							30
	{ anVec4( 1.0f,		1.0f,		0.5f,  1.0f ), "^O" }, 			// O							31
};

const char *units[2][4] = {
	{ "B", "KB", "MB", "GB" },
	{ "B/s", "KB/s", "MB/s", "GB/s" }
};
/*
============
anStr::ColorForIndex
============
*/
const anVec4 &anStr::ColorForIndex( int i ) {
	return g_color_table[ i & COLOR_BITS ].color;
}

/*
============
anStr::ColorForIndex
============
*/
const anVec4 &anStr::ColorForChar( int c ) {
	return g_color_table[ ColorIndex( c ) ].color;
}

/*
============
anStr::ColorForIndex
============
*/
const char *anStr::StrForColorIndex( int i ) {
	return g_color_table[ i & COLOR_BITS ].str; //return g_color_table[ i & 15 ];
}

/*
============
anStr::ReAllocate
============
*/
void anStr::ReAllocate( int amount, bool keepOld ) {
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
anStr::FreeData
============
*/
void anStr::FreeData( void ) {
	if ( data && data != baseBuffer ) {
		stringDataAllocator.Free( data );
		//delete[] data;
		data = baseBuffer;
	}
}

/*
============
anStr::operator=
============
*/
void anStr::operator=( const char *text ) {
	int l;
	int diff;

	if ( !text ) {
		// safe behaviour if nullptr
		EnsureAlloced( 1, false );
		data[0] = '\0';
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

		for ( int i = 0; text[i]; i++ ) {
			data[i] = text[i];
		}

		data[i] = '\0';

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
anStr::FindChar

returns -1 if not found otherwise the index of the char
============
*/
int anStr::FindChar( const char *str, const char c, int start, int end ) {
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
anStr::FindText

returns -1 if not found otherwise the index of the text
============
*/
int anStr::FindText( const char *str, const char *text, bool casesensitive, int start, int end ) {
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
anStr::Filter

Returns true if the string conforms the given filter.
Several metacharacter may be used in the filter.

*          match any string of zero or more characters
?          match any single character
[abc...]   match any of the enclosed characters; a hyphen can
           be used to specify a range (e.g. a-z, A-Z, 0-9)

============
*/
bool anStr::Filter( const char *filter, const char *name, bool casesensitive ) {
	anStr buf;
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
				index = anStr( name ).Find( buf.c_str(), casesensitive );
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
anStr::StripMediaName

  makes the string lower case, replaces backslashes with forward slashes, and removes extension
=============
*/
void anStr::StripMediaName( const char *name, anStr &mediaName ) {
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
			mediaName.Append( anStr::ToLower( c ) );
		}
	}
}

/*
=============
anStr::CheckExtension
=============
*/
bool anStr::CheckExtension( const char *name, const char *ext ) {
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
anStr::FloatArrayToString
=============
*/
const char *anStr::FloatArrayToString( const float *array, const int length, const int precision ) {
	static int index = 0;
	static char str[4][16384];	// in case called by nested functions
	int i, n;
	char format[16], *s;

	// use an array of string so that multiple calls won't collide
	s = str[index];
	index = (index + 1 ) & 3;

	anStr::snPrintf( format, sizeof( format ), "%%.%df", precision );
	n = anStr::snPrintf( s, sizeof( str[0] ), format, array[0] );
	if ( precision > 0 ) {
		while( n > 0 && s[n-1] == '0' ) s[--n] = '\0';
		while( n > 0 && s[n-1] == '.' ) s[--n] = '\0';
	}
	anStr::snPrintf( format, sizeof( format ), " %%.%df", precision );
	for ( i = 1; i < length; i++ ) {
		n += anStr::snPrintf( s + n, sizeof( str[0] ) - n, format, array[i] );
		if ( precision > 0 ) {
			while( n > 0 && s[n-1] == '0' ) s[--n] = '\0';
			while( n > 0 && s[n-1] == '.' ) s[--n] = '\0';
		}
	}
	return s;
}

/*
============
anStr::Last

returns -1 if not found otherwise the index of the char
============
*/
int anStr::Last( const char c ) const {
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
anStr::StripLeading
============
*/
void anStr::StripLeading( const char c ) {
	while( data[0] == c ) {
		memmove( &data[0], &data[1], len );
		len--;
	}
}

/*
============
anStr::StripLeading
============
*/
void anStr::StripLeading( const char *string ) {
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
anStr::StripLeadingOnce
============
*/
bool anStr::StripLeadingOnce( const char *string ) {
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
anStr::StripTrailing
============
*/
void anStr::StripTrailing( const char c ) {
	int i;

	for ( i = Length(); i > 0 && data[ i - 1 ] == c; i-- ) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
anStr::StripLeading
============
*/
void anStr::StripTrailing( const char *string ) {
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
anStr::StripTrailingOnce
============
*/
bool anStr::StripTrailingOnce( const char *string ) {
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
anStr::Replace
============
*/
void anStr::Replace( const char *old, const char *nw ) {
	int		oldLen, newLen, i, j, count;
	anStr	oldString( data );

	oldLen = strlen( old );
	newLen = strlen( nw );

	// Work out how big the new string will be
	count = 0;
	for ( i = 0; i < oldString.Length(); i++ ) {
		if ( !anStr::Cmpn( &oldString[i], old, oldLen ) ) {
			count++;
			i += oldLen - 1;
		}
	}

	if ( count ) {
		EnsureAlloced( len + ( ( newLen - oldLen ) * count ) + 2, false );

		// Replace the old data with the new data
		for ( i = 0, j = 0; i < oldString.Length(); i++ ) {
			if ( !anStr::Cmpn( &oldString[i], old, oldLen ) ) {
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
anStr::Mid
============
*/
const char *anStr::Mid( int start, int len, anStr &result ) const {
	int i;

	result.Empty();

	i = Length();
	if ( i == 0 || len <= 0 || start >= i ) {
		return nullptr;
	}

	if ( start + len >= i ) {
		len = i - start;
	}

	result.Append( &data[ start ], len );
	return result;
}

/*
============
anStr::Mid
============
*/
anStr anStr::Mid( int start, int len ) const {
	int i;
	anStr result;

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
anStr::StripTrailingWhitespace
============
*/
void anStr::StripTrailingWhitespace( void ) {
	int i;

	// cast to unsigned char to prevent stripping off high-ASCII characters
	for ( i = Length(); i > 0 && (unsigned char)( data[ i - 1 ] ) <= ' '; i-- ) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
anStr::StripQuotes

Removes the quotes from the beginning and end of the string
============
*/
anStr& anStr::StripQuotes( void ) {
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
	memmove( &data[0], &data[1], len );
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
anStr::FileNameHash
============
*/
int anStr::FileNameHash( void ) const {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while( data[i] != '\0' ) {
		letter = anStr::ToLower( data[i] );
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
anStr::BackSlashesToSlashes
============
*/
anStr &anStr::BackSlashesToSlashes( void ) {
	int i;

	for ( i = 0; i < len; i++ ) {
		if ( data[i] == '\\' ) {
			data[i] = '/';
		}
	}
	return *this;
}

/*
============
anStr::SetFileExtension
============
*/
anStr &anStr::SetFileExtension( const char *extension ) {
	StripFileExtension();
	if ( *extension != '.' ) {
		Append( '.' );
	}
	Append( extension );
	return *this;
}

/*
============
anStr::StripFileExtension
============
*/
anStr &anStr::StripFileExtension( void ) {
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
anStr::StripAbsoluteFileExtension
============
*/
anStr &anStr::StripAbsoluteFileExtension( void ) {
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
anStr::DefaultFileExtension
==================
*/
anStr &anStr::DefaultFileExtension( const char *extension ) {
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
anStr::DefaultPath
==================
*/
anStr &anStr::DefaultPath( const char *basepath ) {
	if ( ( ( *this )[0] == '/' ) || ( ( *this )[0] == '\\' ) ) {
		// absolute path location
		return *this;
	}

	*this = basepath + *this;
	return *this;
}

/*
====================
anStr::AppendPath
====================
*/
void anStr::AppendPath( const char *text ) {
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

		for (; text[i]; i++ ) {
			if ( text[i] == '\\' ) {
				data[ pos++ ] = '/';
			} else {
				data[ pos++ ] = text[i];
			}
		}
		len = pos;
		data[ pos ] = '\0';
	}
}

/*
==================
anStr::StripFilename
==================
*/
anStr &anStr::StripFilename( void ) {
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
anStr::StripPath
==================
*/
anStr &anStr::StripPath( void ) {
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
anStr::ExtractFilePath
====================
*/
void anStr::ExtractFilePath( anStr &dest ) const {
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
anStr::ExtractFileName
====================
*/
void anStr::ExtractFileName( anStr &dest ) const {
	//
	// back up until a \ or the start
	//
	int pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	Right( Length() - pos, dest );
}

/*
====================
anStr::ExtractFileBase
====================
*/
void anStr::ExtractFileBase( anStr &dest ) const {
	//
	// back up until a \ or the start
	//
	int pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	int start = pos;
	while( ( pos < Length() ) && ( ( *this )[ pos ] != '.' ) ) {
		pos++;
	}

	Mid( start, pos - start, dest );
}

/*
====================
anStr::ExtractFileExtension
====================
*/
void anStr::ExtractFileExtension( anStr &dest ) const {
	//
	// back up until a . or the start
	//
	int pos = Length() - 1;
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
anStr::IsNumeric

Checks a string to see if it contains only numerical values.
============
*/
bool anStr::IsNumeric( const char *s ) {
	if ( *s == '-' ) {
		s++;
	}

	bool dot = false;
	for ( int i = 0; s[i]; i++ ) {
		if ( !isdigit( s[i] ) ) {
			if ( ( s[i] == '.' ) && !dot ) {
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
anStr::HasLower

Checks if a string has any lowercase chars
============
*/
bool anStr::HasLower( const char *s ) {
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
anStr::HasUpper

Checks if a string has any uppercase chars
============
*/
bool anStr::HasUpper( const char *s ) {
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
anStr::Cmp
================
*/
int anStr::Cmp( const char *s1, const char *s2 ) {
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
anStr::Cmpn
================
*/
int anStr::Cmpn( const char *s1, const char *s2, int n ) {
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
anStr::Icmp
================
*/
int anStr::Icmp( const char *s1, const char *s2 ) {
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
anStr::Icmpn
================
*/
int anStr::Icmpn( const char *s1, const char *s2, int n ) {
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
anStr::Icmp
================
*/
int anStr::IcmpNoColor( const char *s1, const char *s2 ) {
	int c1, c2, d;

	do {
		while ( anStr::IsColor( s1 ) ) {
			s1 += 2;
		}
		while ( anStr::IsColor( s2 ) ) {
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
anStr::IcmpPath
================
*/
int anStr::IcmpPath( const char *s1, const char *s2 ) {
	int c1, c2, d;

#if 0
//#if !defined( _WIN32 )
	anLibrary::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
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
anStr::IcmpnPath
================
*/
int anStr::IcmpnPath( const char *s1, const char *s2, int n ) {
	int c1, c2, d;

#if 0
//#if !defined( _WIN32 )
	anLibrary::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
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
anStr::Copynz

Safe strncpy that ensures a trailing zero
=============
*/
void anStr::Copynz( char *dest, const char *src, int destsize ) {
	if ( !src ) {
		anLibrary::common->Warning( "anStr::Copynz: nullptr src" );
		return;
	}
	if ( destsize < 1 ) {
		anLibrary::common->Warning( "anStr::Copynz: destsize < 1" );
		return;
	}

	strncpy( dest, src, destsize-1 );
    dest[destsize-1] = 0;
}

/*
================
anStr::Append

  never goes past bounds or leaves without a terminating 0
================
*/
void anStr::Append( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		anLibrary::common->Error( "anStr::Append: already overflowed" );
	}
	anStr::Copynz( dest + l1, src, size - l1 );
}

/*
================
anStr::LengthWithoutColors
================
*/
int anStr::LengthWithoutColors( const char *s ) {
	int len;
	const char *p;

	if ( !s ) {
		return 0;
	}

	len = 0;
	p = s;
	while( *p ) {
		if ( anStr::IsColor( p ) ) {
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
anStr::RemoveColors
================
*/
char *anStr::RemoveColors( char *string ) {
	char *d;
	char *s;
	int c;

	s = string;
	d = string;
	while( (c = *s) != 0 ) {
		if ( anStr::IsColor( s ) ) {
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
anStr::snPrintf
================
*/
int anStr::snPrintf( char *dest, int size, const char *fmt, ...) {
	int len;
	va_list argptr;
	char buffer[32000];	// big, but small enough to fit in PPC stack

	va_start( argptr, fmt );
	len = vsprintf( buffer, fmt, argptr );
	va_end( argptr );
	if ( len >= sizeof( buffer ) ) {
		anLibrary::common->Error( "anStr::snPrintf: overflowed buffer" );
	}
	if ( len >= size ) {
		anLibrary::common->Warning( "anStr::snPrintf: overflow of %i in %i\n", len, size );
		len = size;
	}
	anStr::Copynz( dest, buffer, size );
	return len;
}

/*
============
anStr::vsnPrintf

vsnprintf portability:

C99 standard: vsnprintf returns the number of characters (excluding the trailing
'\0') which would have been written to the final string if enough space had been available
snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')

win32: _vsnprintf returns the number of characters written, not including the terminating null character,
or a negative value if an output error occurs. If the number of characters to write exceeds count, then count
characters are written and -1 is returned and no trailing '\0' is added.

anStr::vsnPrintf: always appends a trailing '\0', returns number of characters written (not including terminal \0 )
or returns -1 on failure or if the buffer would be overflowed.
============
*/
int anStr::vsnPrintf( char *dest, int size, const char *fmt, va_list argptr ) {
	int ret;

#ifdef _WIN32
#undef _vsnprintf
	ret = _vsnprintf( dest, size-1, fmt, argptr );
#define _vsnprintf	use_anStr_vsnPrintf
#else
#undef vsnprintf
	ret = vsnprintf( dest, size, fmt, argptr );
#define vsnprintf	use_anStr_vsnPrintf
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
int sprintf( anStr &string, const char *fmt, ... ) {
	int l;
	va_list argptr;
	char buffer[32000];

	va_start( argptr, fmt );
	l = anStr::vsnPrintf( buffer, sizeof(buffer)-1, fmt, argptr );
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
int vsprintf( anStr &string, const char *fmt, va_list argptr ) {
	int l;
	char buffer[32000];

	l = anStr::vsnPrintf( buffer, sizeof(buffer)-1, fmt, argptr );
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
anStr::BestUnit
============
*/
int anStr::BestUnit( const char *format, float value, Measure_t measure ) {
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
anStr::SetUnit
============
*/
void anStr::SetUnit( const char *format, float value, int unit, Measure_t measure ) {
	value /= 1 << ( unit * 10 );
	sprintf( *this, format, value );
	*this += " ";
	*this += units[ measure ][ unit ];
}

/*
================
anStr::InitMemory
================
*/
void anStr::InitMemory( void ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Init();
#endif
}

/*
================
anStr::ShutdownMemory
================
*/
void anStr::ShutdownMemory( void ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Shutdown();
#endif
}

/*
================
anStr::PurgeMemory
================
*/
void anStr::PurgeMemory( void ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.FreeEmptyBaseBlocks();
#endif
}

/*
================
anStr::ShowMemoryUsage_f
================
*/
void anStr::ShowMemoryUsage_f( const anCommandArgs &args ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	anLibrary::common->Printf( "%6d KB string memory (%d KB free in %d blocks, %d empty base blocks)\n",
		stringDataAllocator.GetBaseBlockMemory() >> 10, stringDataAllocator.GetFreeBlockMemory() >> 10,
			stringDataAllocator.GetNumFreeBlocks(), stringDataAllocator.GetNumEmptyBaseBlocks() );
#endif
}

/*
================
anStr::FormatNumber
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


anStr anStr::FormatNumber( int number ) {
	anStr string;
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

int Q_isprint( int c ) {
	if ( c >= 0x20 && c <= 0x7E ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_islower( int c ) {
	if ( c >= 'a' && c <= 'z' ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isupper( int c ) {
	if ( c >= 'A' && c <= 'Z' ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isalpha( int c ) {
	if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ) {
		return ( 1 );
	}
	return ( 0 );
}

char* Q_strrchr( const char* string, int c ) {
	char cc = c;
	char *sp = (char *)0, *s = (char *)string;

	while ( *s ) {
		if ( *s == cc ) {
			sp = s;
		}
		s++;
	}
	if ( cc == 0 ) {
		sp = s;
	}

	return sp;
}

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize ) {
	if ( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}
	if ( destsize < 1 ) {
		Com_Error( ERR_FATAL,"Q_strncpyz: destsize < 1" );
	}

	strncpy( dest, src, destsize - 1 );
	dest[destsize - 1] = 0;
}

int Q_stricmpn( const char *s1, const char *s2, int n ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;       // strings are equal until end point
		}

		if ( c1 != c2 ) {
			if ( c1 >= 'a' && c1 <= 'z' ) {
				c1 -= ( 'a' - 'A' );
			}
			if ( c2 >= 'a' && c2 <= 'z' ) {
				c2 -= ( 'a' - 'A' );
			}
			if ( c1 != c2 ) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while ( c1 );

	return 0;       // strings are equal
}

int Q_strncmp( const char *s1, const char *s2, int n ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;       // strings are equal until end point
		}

		if ( c1 != c2 ) {
			return c1 < c2 ? -1 : 1;
		}
	} while ( c1 );

	return 0;       // strings are equal
}

int Q_stricmp( const char *s1, const char *s2 ) {
	return Q_stricmpn( s1, s2, 99999 );
}


char *Q_strlwr( char *s1 ) {
	char    *s;

	s = s1;
	while ( *s ) {
		*s = tolower( *s );
		s++;
	}
	return s1;
}

char *Q_strupr( char *s1 ) {
	char    *s;

	s = s1;
	while ( *s ) {
		*s = toupper( *s );
		s++;
	}
	return s1;
}


// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}


int Q_PrintStrlen( const char *string ) {
	if ( !string ) {
		return 0;
	}

	int len = 0;
	const char *p = string;
	while ( *p ) {
		if ( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}

char *Q_CleanStr( char *string ) {
	int c;
	char *s = string, *d = string;
	while ( ( c = *s ) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			s++;
		} else if ( c >= 0x20 && c <= 0x7E )   {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

#if 0
/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char *QDECL va( char *format, ... ) {
	va_list argptr;
	static char string[2][32000];       // in case va is called by nested functions
	static int index = 0;
	char    *buf;

	buf = string[index & 1];
	index++;

	va_start( argptr, format );
	vsprintf( buf, format,argptr );
	va_end( argptr );

	return buf;
}
#endif
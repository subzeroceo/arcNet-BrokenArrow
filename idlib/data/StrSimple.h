// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __STRSIMPLE_H__
#define __STRSIMPLE_H__

/*
===============================================================================

	Character string class that doesn't use the string data allocator but instead the thread safe OS memory allocation calls

===============================================================================
*/

class idSimpleStr : public anStr {
public:
						idSimpleStr( void );
						idSimpleStr( const anStr &text );
						idSimpleStr( const anStr &text, int start, int end );
						idSimpleStr( const char *text );
						idSimpleStr( const char *text, int start, int end );
						explicit idSimpleStr( const bool b );
						explicit idSimpleStr( const char c );
						explicit idSimpleStr( const int i );
						explicit idSimpleStr( const unsigned u );
						explicit idSimpleStr( const float f );

	void				ReAllocate( int amount, bool keepold );				// reallocate string data buffer
	void				FreeData( void );									// free allocated string memory
};


ID_INLINE idSimpleStr::idSimpleStr( void ) :
	anStr() {
}

ID_INLINE idSimpleStr::idSimpleStr( const anStr &text ) :
	anStr( text ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const anStr &text, int start, int end ) :
	anStr( text, start, end ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const char *text ) :
	anStr( text ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const char *text, int start, int end ) :
	anStr( text, start, end ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const bool b ) :
	anStr( b ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const char c ) :
	anStr( c ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const int i ) :
	anStr( i ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const unsigned u ) :
	anStr( u ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const float f ) :
	anStr( f ) {
}

#endif // !__STRSIMPLE_H__

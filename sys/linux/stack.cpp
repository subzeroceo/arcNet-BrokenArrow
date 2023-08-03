#include "..//idlib/Lib.h"

/*
==================
Sys_ShutdownSymbols
==================
*/
void Sys_ShutdownSymbols( void ) {
}

#ifdef ID_BT_STUB

/*
==================
Sys_GetCallStack
==================
*/
void Sys_GetCallStack( address_t *callStack, const int callStackSize ) {
	for ( int i = 0; i < callStackSize; i++ ) {
		callStack[i] = 0;
	}
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *Sys_GetCallStackStr( const address_t *callStack, const int callStackSize ) {
	return "";
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *Sys_GetCallStackCurStr( int depth ) {
	return "";
}

/*
==================
Sys_GetCallStackCurAddressStr
==================
*/
const char *	Sys_GetCallStackCurAddressStr( int depth ) {
	return "";
}

#else

#include <execinfo.h>

/*
==================
Sys_GetCallStack
==================
*/
void Sys_GetCallStack( address_t *callStack, const int callStackSize ) {
	int i;
	i = backtrace( (void **)callStack, callStackSize );
	while( i < callStackSize ) {
		callStack[i++] = 0;
	}
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *	Sys_GetCallStackStr( const address_t *callStack, int callStackSize ) {
	static char string[MAX_STRING_CHARS*2];
	char **strings;
	int i;

	strings = backtrace_symbols( (void **)callStack, callStackSize );
	string[ 0 ] = '\0';
	for ( i = 0; i < callStackSize; i++ ) {
		anString::snPrintf( string + strlen( string ), MAX_STRING_CHARS*2 - strlen( string ) - 1, "%s\n", strings[i] );
	}
	free( strings );
	return string;
}


/*
==================
Sys_GetCallStackStr
==================
*/
const char *Sys_GetCallStackCurStr( int depth ) {
	address_t array[ 32 ];
	size_t size;

	size = backtrace( (void **)array, Min( 32, depth ) );
	return Sys_GetCallStackStr( array, ( int )size );
}

/*
==================
Sys_GetCallStackCurAddressStr
==================
*/
const char *Sys_GetCallStackCurAddressStr( int depth ) {
	return Sys_GetCallStackCurStr( depth );
}

#endif

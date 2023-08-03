#include "..//idlib/Lib.h"
#pragma hdrstop
#include "qe3.h"
#include "cmdlib.h"
#define PATHSEPERATOR   '/'

// rad additions
// 11.29.99
PFN_ERR *g_pfnError = nullptr;
PFN_PRINTF *g_pfnPrintf = nullptr;
PFN_ERR_NUM *g_pfnErrorNum = nullptr;
PFN_PRINTF_NUM *g_pfnPrintfNum = nullptr;

void Error( const char *pFormat, ... ) {
	if (g_pfnError) {
		va_list arg_ptr;
		va_start( arg_ptr, pFormat );
		g_pfnError( pFormat, arg_ptr );
		va_end( arg_ptr );
	}
}

void Printf( const char *pFormat, ... ) {
	if ( g_pfnPrintf ) {
		va_list arg_ptr;
		va_start( arg_ptr, pFormat );
		g_pfnPrintf( pFormat, arg_ptr );
		va_end( arg_ptr );
	}
}

void ErrorNum( int nErr, const char *pFormat, ... ) {
	if ( g_pfnErrorNum ) {
		va_list arg_ptr;
		va_start( arg_ptr, pFormat );
		g_pfnErrorNum( nErr, pFormat, arg_ptr );
		va_end( arg_ptr );
	}
}

void PrintfNum( int nErr, const char *pFormat, ... ) {
	if ( g_pfnPrintfNum ) {
		va_list arg_ptr;
		va_start( arg_ptr, pFormat );
		g_pfnPrintfNum( nErr, pFormat, arg_ptr  );
		va_end( arg_ptr );
	}
}

void SetErrorHandler( PFN_ERR pe ) {
	g_pfnError = pe;
}

void SetPrintfHandler( PFN_PRINTF pe ) {
	g_pfnPrintf = pe;
}

void SetErrorHandlerNum( PFN_ERR_NUM pe ) {
	g_pfnErrorNum = pe;
}

void SetPrintfHandler( PFN_PRINTF_NUM pe ) {
	g_pfnPrintfNum = pe;
}

int Q_filelength( FILE *f ) {
	int pos = ftell ( f );
	fseek( f, 0, SEEK_END );
	int end = ftell ( f );
	fseek( f, pos, SEEK_SET );

	return end;
}

int LoadFile( const char *filename, void **bufferptr ) {
	*bufferptr = nullptr;
	if ( filename == nullptr || strlen( filename ) == 0 ) {
		return -1;
	}

	FILE *f = fopen( filename, "rb" );
	if ( !f ) {
		return -1;
	}

	int length = Q_filelength( f );
	void *buffer = Mem_ClearedAlloc( length+1 );

	( (char *)buffer)[length] = 0;
	if ( ( int )fread( buffer, 1, length, f ) != length ) {
		Error( "File read failure" );
	}

	fclose( f );

	*bufferptr = buffer;
	return length;
}

void DefaultExtension( char *path, char *extension ) {
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	char *src = path + strlen( path ) - 1;

	while ( *src != PATHSEPERATOR && src != path ) {
		if ( *src == '.' )
			return;                 // it has an extension
		src--;
	}
	strcat ( path, extension );
}

void DefaultPath( char *path, char *basepath ) {
	char temp[128];

	if ( path[0] == PATHSEPERATOR ) {
		return;		// absolute path location
	}
	strcpy( temp,path );
	strcpy( path,basepath );
	strcat( path,temp );
}

void StripFilename( char *path ) {
	int length = strlen( path )-1;
	while ( length > 0 && path[length] != PATHSEPERATOR ) {
		length--;
	}
	path[ length ] = 0;
}

void StripExtension( char *path ) {
	int length = strlen( path )-1;

	while ( length > 0 && path[length] != '.' ) {
		length--;
		if ( path[length] == '/' ) {
			return;		// no extension
		}
	}

	if ( length ) {
		path[length] = 0;
	}
}


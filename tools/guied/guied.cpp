#include "..//idlib/Lib.h"
#pragma hdrstop
#include "../../renderer/tr_local.h"
#include "../../sys/win32/win_local.h"
#include <io.h>
#include "../../ui/DeviceContext.h"
#include "../../sys/win32/rc/guied_resource.h"
#include "GEApp.h"

rvGEApp		gApp;

// Start the gui editor
void GUIEditorInit( void ) {
	gApp.Initialize();
}

void GUIEditorShutdown( void ) {
}

// Handle translator messages
bool GUIEditorHandleMessage ( void *msg ) {
	if ( !gApp.IsActive() ) {
		return false;
	}
	return gApp.TranslateAccelerator( reinterpret_cast<LPMSG>( msg ) );
}

// run a frame
void GUIEditorRun()  {
    MSG			msg;

	// pump the message loop
	while (PeekMessage (&msg, nullptr, 0, 0, PM_NOREMOVE) )  {
		if ( !GetMessage (&msg, nullptr, 0, 0 ) )  {
			common->Quit();
		}
		// save the msg time, because wndprocs don't have access to the timestamp
		if ( win32.sysMsgTime && win32.sysMsgTime > ( int )msg.time ) {
		} else {
			win32.sysMsgTime = msg.time;
		}
		if ( gApp.TranslateAccelerator ( &msg ) ) {
			continue;
		}

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	gApp.RunFrame();

	// The GUI editor runs too hot so we need to slow it down a bit.
	Sleep ( 1 );
}

// Returns a clean string version of the given vec4
const char *StringFromVec4 ( anVec4& v ) {
	return va( "%s,%s,%s,%s",
		anString::FloatArrayToString( &v[0], 1, 8 ),
		anString::FloatArrayToString( &v[1], 1, 8 ),
		anString::FloatArrayToString( &v[2], 1, 8 ),
		anString::FloatArrayToString( &v[3], 1, 8 ) );
}

// Returns true if the given string is an expression
bool IsExpression ( const char* s ) {
	anParser src( s, strlen ( s ), "", LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );

	anToken token;
	bool	needComma = false;
	bool	needNumber = false;
	while ( src.ReadToken ( &token ) ) {
		switch ( token.type ) {
			case TT_NUMBER:
				needComma = true;
				needNumber = false;
				break;

			case TT_PUNCTUATION:
				if ( needNumber ) {
					return true;
				}
				if ( token[0] == ',' ) {
					if ( !needComma ) {
						return true;
					}
					needComma = false;
					break;
				}

				if ( needComma ) {
					return true;
				}

				if ( token[0] == '-' ) {
					needNumber = true;
				}
				break;
			default:
				return true;
		}
	}
	return false;
}

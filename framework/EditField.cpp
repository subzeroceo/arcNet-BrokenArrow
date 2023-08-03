#include "/idlib/Lib.h"
#pragma hdrstop

static autoComplete_t	globalAutoComplete;

/*
===============
FindMatches
===============
*/
static void FindMatches( const char *s ) {
	int		i;

	if ( anString::Icmpn( s, globalAutoComplete.completionString, strlen( globalAutoComplete.completionString ) ) != 0 ) {
		return;
	}
	globalAutoComplete.matchCount++;
	if ( globalAutoComplete.matchCount == 1 ) {
		anString::Copynz( globalAutoComplete.currentMatch, s, sizeof( globalAutoComplete.currentMatch ) );
		return;
	}

	// cut currentMatch to the amount common with s
	for ( i = 0; s[i]; i++ ) {
		if ( tolower( globalAutoComplete.currentMatch[i] ) != tolower( s[i] ) ) {
			globalAutoComplete.currentMatch[i] = 0;
			break;
		}
	}
	globalAutoComplete.currentMatch[i] = 0;
}

/*
===============
FindIndexMatch
===============
*/
static void FindIndexMatch( const char *s ) {
	if ( anString::Icmpn( s, globalAutoComplete.completionString, strlen( globalAutoComplete.completionString ) ) != 0 ) {
		return;
	}

	if ( globalAutoComplete.findMatchIndex == globalAutoComplete.matchIndex ) {
		anString::Copynz( globalAutoComplete.currentMatch, s, sizeof( globalAutoComplete.currentMatch ) );
	}

	globalAutoComplete.findMatchIndex++;
}

/*
===============
PrintMatches
===============
*/
static void PrintMatches( const char *s ) {
	if ( anString::Icmpn( s, globalAutoComplete.currentMatch, strlen( globalAutoComplete.currentMatch ) ) == 0 ) {
		common->Printf( "    %s\n", s );
	}
}

/*
===============
PrintCvarMatches
===============
*/
static void PrintCvarMatches( const char *s ) {
	if ( anString::Icmpn( s, globalAutoComplete.currentMatch, strlen( globalAutoComplete.currentMatch ) ) == 0 ) {
		common->Printf( "    %s" S_COLOR_WHITE " = \"%s\"\n", s, cvarSystem->GetCVarString( s ) );
	}
}

/*
===============
arcEditField::arcEditField
===============
*/
arcEditField::arcEditField() {
	widthInChars = 0;
	Clear();
}

/*
===============
arcEditField::~arcEditField
===============
*/
arcEditField::~arcEditField() {
}

/*
===============
arcEditField::Clear
===============
*/
void arcEditField::Clear() {
	buffer[0] = 0;
	cursor = 0;
	scroll = 0;
	autoComplete.length = 0;
	autoComplete.valid = false;
}

/*
===============
arcEditField::SetWidthInChars
===============
*/
void arcEditField::SetWidthInChars( int w ) {
	assert( w <= MAX_EDIT_LINE );
	widthInChars = w;
}

/*
===============
arcEditField::SetCursor
===============
*/
void arcEditField::SetCursor( int c ) {
	assert( c <= MAX_EDIT_LINE );
	cursor = c;
}

/*
===============
arcEditField::GetCursor
===============
*/
int arcEditField::GetCursor() const {
	return cursor;
}

/*
===============
arcEditField::ClearAutoComplete
===============
*/
void arcEditField::ClearAutoComplete() {
	if ( autoComplete.length > 0 && autoComplete.length <= ( int ) strlen( buffer ) ) {
		buffer[autoComplete.length] = '\0';
		if ( cursor > autoComplete.length ) {
			cursor = autoComplete.length;
		}
	}
	autoComplete.length = 0;
	autoComplete.valid = false;
}

/*
===============
arcEditField::GetAutoCompleteLength
===============
*/
int arcEditField::GetAutoCompleteLength() const {
	return autoComplete.length;
}

/*
===============
arcEditField::AutoComplete
===============
*/
void arcEditField::AutoComplete() {
	char completionArgString[MAX_EDIT_LINE];
	anCommandArgs args;

	if ( !autoComplete.valid ) {
		args.TokenizeString( buffer, false );
		anString::Copynz( autoComplete.completionString, args.Argv( 0 ), sizeof( autoComplete.completionString ) );
		anString::Copynz( completionArgString, args.Args(), sizeof( completionArgString ) );
		autoComplete.matchCount = 0;
		autoComplete.matchIndex = 0;
		autoComplete.currentMatch[0] = 0;

		if ( strlen( autoComplete.completionString ) == 0 ) {
			return;
		}

		globalAutoComplete = autoComplete;

		cmdSystem->CommandCompletion( FindMatches );
		cvarSystem->CommandCompletion( FindMatches );

		autoComplete = globalAutoComplete;

		if ( autoComplete.matchCount == 0 ) {
			return;	// no matches
		}

		// when there's only one match or there's an argument
		if ( autoComplete.matchCount == 1 || completionArgString[0] != '\0' ) {
			/// try completing arguments
			anString::Append( autoComplete.completionString, sizeof( autoComplete.completionString ), " " );
			anString::Append( autoComplete.completionString, sizeof( autoComplete.completionString ), completionArgString );
			autoComplete.matchCount = 0;

			globalAutoComplete = autoComplete;

			cmdSystem->ArgCompletion( autoComplete.completionString, FindMatches );
			cvarSystem->ArgCompletion( autoComplete.completionString, FindMatches );

			autoComplete = globalAutoComplete;

			anString::snPrintf( buffer, sizeof( buffer ), "%s", autoComplete.currentMatch );

			if ( autoComplete.matchCount == 0 ) {
				// no argument matches
				anString::Append( buffer, sizeof( buffer ), " " );
				anString::Append( buffer, sizeof( buffer ), completionArgString );
				SetCursor( strlen( buffer ) );
				return;
			}
		} else {
			// multiple matches, complete to shortest
			anString::snPrintf( buffer, sizeof( buffer ), "%s", autoComplete.currentMatch );
			if ( strlen( completionArgString ) ) {
				anString::Append( buffer, sizeof( buffer ), " " );
				anString::Append( buffer, sizeof( buffer ), completionArgString );
			}
		}

		autoComplete.length = strlen( buffer );
		autoComplete.valid = ( autoComplete.matchCount != 1 );
		SetCursor( autoComplete.length );

		common->Printf( "]%s\n", buffer );

		// run through again, printing matches
		globalAutoComplete = autoComplete;

		cmdSystem->CommandCompletion( PrintMatches );
		cmdSystem->ArgCompletion( autoComplete.completionString, PrintMatches );
		cvarSystem->CommandCompletion( PrintCvarMatches );
		cvarSystem->ArgCompletion( autoComplete.completionString, PrintMatches );
	} else if ( autoComplete.matchCount != 1 ) {
		// get the next match and show instead
		autoComplete.matchIndex++;
		if ( autoComplete.matchIndex == autoComplete.matchCount ) {
			autoComplete.matchIndex = 0;
		}
		autoComplete.findMatchIndex = 0;

		globalAutoComplete = autoComplete;

		cmdSystem->CommandCompletion( FindIndexMatch );
		cmdSystem->ArgCompletion( autoComplete.completionString, FindIndexMatch );
		cvarSystem->CommandCompletion( FindIndexMatch );
		cvarSystem->ArgCompletion( autoComplete.completionString, FindIndexMatch );

		autoComplete = globalAutoComplete;

		// and print it
		anString::snPrintf( buffer, sizeof( buffer ), autoComplete.currentMatch );
		if ( autoComplete.length > ( int )strlen( buffer ) ) {
			autoComplete.length = strlen( buffer );
		}
		SetCursor( autoComplete.length );
	}
}

/*
===============
arcEditField::CharEvent
===============
*/
void arcEditField::CharEvent( int ch ) {
	int		len;

	if ( ch == 'v' - 'a' + 1 ) {	// ctrl-v is paste
		Paste();
		return;
	}

	if ( ch == 'c' - 'a' + 1 ) {	// ctrl-c clears the field
		Clear();
		return;
	}

	len = strlen( buffer );

	if ( ch == 'h' - 'a' + 1 || ch == K_BACKSPACE ) {	// ctrl-h is backspace
		if ( cursor > 0 ) {
			memmove( buffer + cursor - 1, buffer + cursor, len + 1 - cursor );
			cursor--;
			if ( cursor < scroll ) {
				scroll--;
			}
		}
		return;
	}

	if ( ch == 'a' - 'a' + 1 ) {	// ctrl-a is home
		cursor = 0;
		scroll = 0;
		return;
	}

	if ( ch == 'e' - 'a' + 1 ) {	// ctrl-e is end
		cursor = len;
		scroll = cursor - widthInChars;
		return;
	}

	//
	// ignore any other non printable chars
	//
	if ( ch < 32 ) {
		return;
	}

	if ( idKeyInput::GetOverstrikeMode() ) {
		if ( cursor == MAX_EDIT_LINE - 1 ) {
			return;
		}
		buffer[cursor] = ch;
		cursor++;
	} else {	// insert mode
		if ( len == MAX_EDIT_LINE - 1 ) {
			return; // all full
		}
		memmove( buffer + cursor + 1, buffer + cursor, len + 1 - cursor );
		buffer[cursor] = ch;
		cursor++;
	}


	if ( cursor >= widthInChars ) {
		scroll++;
	}

	if ( cursor == len + 1 ) {
		buffer[cursor] = 0;
	}
}

/*
===============
arcEditField::KeyDownEvent
===============
*/
void arcEditField::KeyDownEvent( int key ) {
	int		len;

	// shift-insert is paste
	if ( ( ( key == K_INS ) || ( key == K_KP_0 ) ) && ( idKeyInput::IsDown( K_LSHIFT ) || idKeyInput::IsDown( K_RSHIFT ) ) ) {
		ClearAutoComplete();
		Paste();
		return;
	}

	len = strlen( buffer );

	if ( key == K_DEL ) {
		if ( autoComplete.length ) {
			ClearAutoComplete();
		} else if ( cursor < len ) {
			memmove( buffer + cursor, buffer + cursor + 1, len - cursor );
		}
		return;
	}

	if ( key == K_RIGHTARROW ) {
		if ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) {
			// skip to next word
			while( ( cursor < len ) && ( buffer[ cursor ] != ' ' ) ) {
				cursor++;
			}

			while( ( cursor < len ) && ( buffer[ cursor ] == ' ' ) ) {
				cursor++;
			}
		} else {
			cursor++;
		}

		if ( cursor > len ) {
			cursor = len;
		}

		if ( cursor >= scroll + widthInChars ) {
			scroll = cursor - widthInChars + 1;
		}

		if ( autoComplete.length > 0 ) {
			autoComplete.length = cursor;
		}
		return;
	}

	if ( key == K_LEFTARROW ) {
		if ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) {
			// skip to previous word
			while( ( cursor > 0 ) && ( buffer[ cursor - 1 ] == ' ' ) ) {
				cursor--;
			}

			while( ( cursor > 0 ) && ( buffer[ cursor - 1 ] != ' ' ) ) {
				cursor--;
			}
		} else {
			cursor--;
		}

		if ( cursor < 0 ) {
			cursor = 0;
		}
		if ( cursor < scroll ) {
			scroll = cursor;
		}

		if ( autoComplete.length ) {
			autoComplete.length = cursor;
		}
		return;
	}

	if ( key == K_HOME || ( key == K_A && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) ) {
		cursor = 0;
		scroll = 0;
		if ( autoComplete.length ) {
			autoComplete.length = cursor;
			autoComplete.valid = false;
		}
		return;
	}

	if ( key == K_END || ( key == K_E && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) ) {
		cursor = len;
		if ( cursor >= scroll + widthInChars ) {
			scroll = cursor - widthInChars + 1;
		}
		if ( autoComplete.length ) {
			autoComplete.length = cursor;
			autoComplete.valid = false;
		}
		return;
	}

	if ( key == K_INS ) {
		idKeyInput::SetOverstrikeMode( !idKeyInput::GetOverstrikeMode() );
		return;
	}

	// clear autocompletion buffer on normal key input
	if ( key != K_CAPSLOCK && key != K_LALT && key != K_LCTRL && key != K_LSHIFT && key != K_RALT && key != K_RCTRL && key != K_RSHIFT ) {
		ClearAutoComplete();
	}
}

/*
===============
arcEditField::Paste
===============
*/
void arcEditField::Paste() {
	char	*cbd;
	int		pasteLen, i;

	cbd = Sys_GetClipboardData();

	if ( !cbd ) {
		return;
	}

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen( cbd );
	for ( i = 0; i < pasteLen; i++ ) {
		CharEvent( cbd[i] );
	}

	Mem_Free( cbd );
}

/*
===============
arcEditField::GetBuffer
===============
*/
char *arcEditField::GetBuffer() {
	return buffer;
}

/*
===============
arcEditField::SetBuffer
===============
*/
void arcEditField::SetBuffer( const char *buf ) {
	Clear();
	anString::Copynz( buffer, buf, sizeof( buffer ) );
	SetCursor( strlen( buffer ) );
}

/*
===============
arcEditField::Draw
===============
*/
void arcEditField::Draw( int x, int y, int width, bool showCursor ) {
	int		len;
	int		drawLen;
	int		prestep;
	int		cursorChar;
	char	str[MAX_EDIT_LINE];
	int		size;

	size = SMALLCHAR_WIDTH;

	drawLen = widthInChars;
	len = strlen( buffer ) + 1;

	// guarantee that cursor will be visible
	if ( len <= drawLen ) {
		prestep = 0;
	} else {
		if ( scroll + drawLen > len ) {
			scroll = len - drawLen;
			if ( scroll < 0 ) {
				scroll = 0;
			}
		}
		prestep = scroll;

		// Skip color code
		if ( anString::IsColor( buffer + prestep ) ) {
			prestep += 2;
		}
		if ( prestep > 0 && anString::IsColor( buffer + prestep - 1 ) ) {
			prestep++;
		}
	}

	if ( prestep + drawLen > len ) {
		drawLen = len - prestep;
	}

	// extract <drawLen> characters from the field at <prestep>
	if ( drawLen >= MAX_EDIT_LINE ) {
		common->Error( "drawLen >= MAX_EDIT_LINE" );
	}

	memcpy( str, buffer + prestep, drawLen );
	str[ drawLen ] = 0;

	// draw it
	renderSystem->DrawSmallStringExt( x, y, str, colorWhite, false );

	// draw the cursor
	if ( !showCursor ) {
		return;
	}

	if ( ( int )( anLibrary::frameNumber >> 4 ) & 1 ) {
		return;		// off blink
	}

	if ( idKeyInput::GetOverstrikeMode() ) {
		cursorChar = 11;
	} else {
		cursorChar = 10;
	}

	// Move the cursor back to account for color codes
	for ( int i = 0; i<cursor; i++ ) {
		if ( anString::IsColor( &str[i] ) ) {
			i++;
			prestep += 2;
		}
	}

	renderSystem->DrawSmallChar( x + ( cursor - prestep ) * size, y, cursorChar );
}

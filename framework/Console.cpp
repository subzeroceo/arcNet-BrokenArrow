#pragma hdrstop
#include "/idlib/precompiled.h"
#include "ConsoleHistory.h"
#include "../renderer/ResolutionScale.h"
#include "Common_local.h"

#define	CON_TEXTSIZE	0x30000
#define	NUM_CON_TIMES	4
#define CONSOLE_FIRSTREPEAT	200
#define CONSOLE_REPEAT	100

#define	COMMAND_HISTORY	64

struct overlayText_t {
	arcNetString				text;
	justify_t			justify;
	int					time;
};

// the console will query the cvar and command systems for
// command completion information

class aRcConsoleLocal : public aRcConsole {
public:
	virtual	void		Init();
	virtual void		Shutdown();
	virtual	bool		ProcessEvent( const sysEvent_t * event, bool forceAccept );
	virtual	bool		Active();
	virtual	void		ClearNotifyLines();
	virtual	void		Close();
	virtual	void		Print( const char *text );
	virtual	void		Draw( bool forceFullScreen );

	virtual void		PrintOverlay( idOverlayHandle &handle, justify_t justify, const char *text, ... );

	virtual arcDebugGraph *CreateGraph( int numItems );
	virtual void		DestroyGraph( arcDebugGraph * graph );

	void				Dump( const char *toFile );
	void				Clear();

private:
	void				KeyDownEvent( int key );

	void				Linefeed();

	void				PageUp();
	void				PageDown();
	void				Top();
	void				Bottom();

	void				DrawInput();
	void				DrawNotify();
	void				DrawSolidConsole( float frac );

	void				Scroll();
	void				SetDisplayFraction( float frac );
	void				UpdateDisplayFraction();

	void				DrawTextLeftAlign( float x, float &y, const char *text, ... );
	void				DrawTextRightAlign( float x, float &y, const char *text, ... );

	float				DrawFPS( float y );
	float				DrawMemoryUsage( float y );

	void				DrawOverlayText( float & leftY, float & rightY, float & centerY );
	void				DrawDebugGraphs();

	//============================

	// allow these constants to be adjusted for HMD
	int					LOCALSAFE_LEFT;
	int					LOCALSAFE_RIGHT;
	int					LOCALSAFE_TOP;
	int					LOCALSAFE_BOTTOM;
	int					LOCALSAFE_WIDTH;
	int					LOCALSAFE_HEIGHT;
	int					LINE_WIDTH;
	int					TOTAL_LINES;

	bool				keyCatching;

	short				text[CON_TEXTSIZE];
	int					current;		// line where next message will be printed
	int					x;				// offset in current line for next print
	int					display;		// bottom of console displays this line
	int					lastKeyEvent;	// time of last key event for scroll delay
	int					nextKeyEvent;	// keyboard repeat rate

	float				displayFrac;	// approaches finalFrac at con_speed
	float				finalFrac;		// 0.0 to 1.0 lines of console to display
	int					fracTime;		// time of last displayFrac update

	int					vislines;		// in scanlines

	int					times[NUM_CON_TIMES];	// cls.realtime time the line was generated
									// for transparent notify lines
	arcVec4				color;

	arcEditField			historyEditLines[COMMAND_HISTORY];

	int					nextHistoryLine;// the last line in the history buffer, not masked
	int					historyLine;	// the line being displayed from history buffer
									// will be <= nextHistoryLine

	arcEditField			consoleField;

	arcNetList< overlayText_t >overlayText;
	arcNetList< arcDebugGraph *>debugGraphs;

	static arcCVarSystem		con_speed;
	static arcCVarSystem		con_notifyTime;
	static arcCVarSystem		con_noPrint;
};

static aRcConsoleLocal localConsole;
aRcConsole * console = &localConsole;

arcCVarSystem aRcConsoleLocal::con_speed( "con_speed", "3", CVAR_SYSTEM, "speed at which the console moves up and down" );
arcCVarSystem aRcConsoleLocal::con_notifyTime( "con_notifyTime", "3", CVAR_SYSTEM, "time messages are displayed onscreen when console is pulled up" );
#ifdef DEBUG
arcCVarSystem aRcConsoleLocal::con_noPrint( "con_noPrint", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#else
arcCVarSystem aRcConsoleLocal::con_noPrint( "con_noPrint", "1", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#endif

/*
=============================================================================

	Misc stats

=============================================================================
*/

/*
==================
aRcConsoleLocal::DrawTextLeftAlign
==================
*/
void aRcConsoleLocal::DrawTextLeftAlign( float x, float &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;

	va_start( argptr, text );
	arcNetString::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );

	renderSystem->DrawSmallStringExt( x, y + 2, string, colorWhite, true );

	y += SMALLCHAR_HEIGHT + 4;
}

/*
==================
aRcConsoleLocal::DrawTextRightAlign
==================
*/
void aRcConsoleLocal::DrawTextRightAlign( float x, float &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;
	va_start( argptr, text );
	int i = arcNetString::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );
	renderSystem->DrawSmallStringExt( x - i * SMALLCHAR_WIDTH, y + 2, string, colorWhite, true );
	y += SMALLCHAR_HEIGHT + 4;
}

/*
==================
aRcConsoleLocal::DrawFPS
==================
*/
#define	FPS_FRAMES	6
float aRcConsoleLocal::DrawFPS( float y ) {
	static int previousTimes[FPS_FRAMES];
	static int index;
	static int previous;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	int t = Sys_Milliseconds();
	int frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		int total = 0;
		for ( int i = 0; i < FPS_FRAMES; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		int fps = 1000000 * FPS_FRAMES / total;
		fps = ( fps + 500 ) / 1000;

		const char * s = va( "%ifps", fps );
		int w = strlen( s ) * BIGCHAR_WIDTH;

		renderSystem->DrawBigStringExt( LOCALSAFE_RIGHT - w, arcMath::Ftoi( y ) + 2, s, colorWhite, true );
	}

	y += BIGCHAR_HEIGHT + 4;

	// print the resolution scale so we can tell when we are at reduced resolution
	arcNetString resolutionText;
	resolutionScale.GetConsoleText( resolutionText );
	int w = resolutionText.Length() * BIGCHAR_WIDTH;
	renderSystem->DrawBigStringExt( LOCALSAFE_RIGHT - w, arcMath::Ftoi( y ) + 2, resolutionText.c_str(), colorWhite, true );

	const int gameThreadTotalTime = commonLocal.GetGameThreadTotalTime();
	const int gameThreadGameTime = commonLocal.GetGameThreadGameTime();
	const int gameThreadRenderTime = commonLocal.GetGameThreadRenderTime();
	const int rendererBackEndTime = commonLocal.GetRendererBackEndMicroseconds();
	const int rendererShadowsTime = commonLocal.GetRendererShadowsMicroseconds();
	const int rendererGPUIdleTime = commonLocal.GetRendererIdleMicroseconds();
	const int rendererGPUTime = commonLocal.GetRendererGPUMicroseconds();
	const int maxTime = 16;

	y += SMALLCHAR_HEIGHT + 4;
	arcNetString timeStr;
	timeStr.Format( "%sG+RF: %4d", gameThreadTotalTime > maxTime ? S_COLOR_RED : "", gameThreadTotalTime );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, arcMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sG: %4d", gameThreadGameTime > maxTime ? S_COLOR_RED : "", gameThreadGameTime );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, arcMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sRF: %4d", gameThreadRenderTime > maxTime ? S_COLOR_RED : "", gameThreadRenderTime );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, arcMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sRB: %4.1f", rendererBackEndTime > maxTime * 1000 ? S_COLOR_RED : "", rendererBackEndTime / 1000.0f );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, arcMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sSV: %4.1f", rendererShadowsTime > maxTime * 1000 ? S_COLOR_RED : "", rendererShadowsTime / 1000.0f );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, arcMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sIDLE: %4.1f", rendererGPUIdleTime > maxTime * 1000 ? S_COLOR_RED : "", rendererGPUIdleTime / 1000.0f );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, arcMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );
	y += SMALLCHAR_HEIGHT + 4;

	timeStr.Format( "%sGPU: %4.1f", rendererGPUTime > maxTime * 1000 ? S_COLOR_RED : "", rendererGPUTime / 1000.0f );
	w = timeStr.LengthWithoutColors() * SMALLCHAR_WIDTH;
	renderSystem->DrawSmallStringExt( LOCALSAFE_RIGHT - w, arcMath::Ftoi( y ) + 2, timeStr.c_str(), colorWhite, false );

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
aRcConsoleLocal::DrawMemoryUsage
==================
*/
float aRcConsoleLocal::DrawMemoryUsage( float y ) {
	// get the total number of blocks
	const auto& n = numBlocks;
    // get the x position of the first block
    float x = 0;

	// loop through all the blocks
	for ( int i = 0; i < n; i++ ) {
        // increment the x position by the block size
        x += blockSize;
		// get the block size
		const float& block[i] = ( float ) block[i] / ( float ) numBlocks;
		// fill the block with 0's
		fill( block.begin(), block.begin() + n, 0 );
		// calculate the total of the block
		float total = 0;
		}

		// loop through all the blocks
		for ( int i = 0; i < n; i++ ) {
			// add the total of the block
			total += block[i];
		}

		// calculate the ratio of the total of the
		// block to the total number of blocks
		float ratio = total / n;

		// loop through all the blocks
		for ( int i = 0; i < n; i++ ) {
			// multiply the block by the ratio
			block[i] = block[i] * ratio;
		}

		// loop through all the blocks
		for ( int i = 0; i < n; i++ ) {
			// multiply the block by 100
			block[i] = block[i] * 100;
		if ( block[i] > 1 ) {
		// if the block is greater than 1
		// set it to 1
        if ( block[i] > 1 ) {
            block[i] = 1;
        }
		}
	}
	// set the height of the block
	float height = blockSize;
	// loop through all the blocks
	for ( int i = 0; i < n; i++ ) {
		// subtract the height of the block
		height -= block[i];
		// subtract the y position of the block
		y -= height;
		// print the block
		for ( int j = 0; j < block[i]; j++ ) {
			// implement Printf
			Printf( "TotalMemory Used: %.2f", j );
			Printf( "Block count: %.2f K/B", j );
			Printf( "Block size: %.2f K/B", j );
		}
        // print a new line
		Printf( "Memory Usage in Kilobytes: %.2f", y );
		// return the y position
		return ( int ) y;
	} else {
		// return the y position
		return y;
	}
}

//=========================================================================

/*
==============
Con_Clear_f
==============
*/
static void Con_Clear_f( const arcCommandArgs &args ) {
	localConsole.Clear();
}

/*
==============
Con_Dump_f
==============
*/
static void Con_Dump_f( const arcCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "usage: conDump <filename>\n" );
		return;
	}

	arcNetString fileName = args.Argv( 1 );
	fileName.DefaultFileExtension( ".txt" );

	common->Printf( "Dumped console text to %s.\n", fileName.c_str() );

	localConsole.Dump( fileName.c_str() );
}

/*
==============
aRcConsoleLocal::Init
==============
*/
void aRcConsoleLocal::Init() {
	int		i;

	keyCatching = false;

	LOCALSAFE_LEFT		= 32;
	LOCALSAFE_RIGHT		= 608;
	LOCALSAFE_TOP		= 24;
	LOCALSAFE_BOTTOM	= 456;
	LOCALSAFE_WIDTH		= LOCALSAFE_RIGHT - LOCALSAFE_LEFT;
	LOCALSAFE_HEIGHT	= LOCALSAFE_BOTTOM - LOCALSAFE_TOP;

	LINE_WIDTH = ( ( LOCALSAFE_WIDTH / SMALLCHAR_WIDTH ) - 2 );
	TOTAL_LINES = ( CON_TEXTSIZE / LINE_WIDTH );

	lastKeyEvent = -1;
	nextKeyEvent = CONSOLE_FIRSTREPEAT;

	consoleField.Clear();
	consoleField.SetWidthInChars( LINE_WIDTH );

	for ( i = 0; i < COMMAND_HISTORY; i++ ) {
		historyEditLines[i].Clear();
		historyEditLines[i].SetWidthInChars( LINE_WIDTH );
	}

	cmdSystem->AddCommand( "clear", Con_Clear_f, CMD_FL_SYSTEM, "clears the console" );
	cmdSystem->AddCommand( "conDump", Con_Dump_f, CMD_FL_SYSTEM, "dumps the console text to a file" );
}

/*
==============
aRcConsoleLocal::Shutdown
==============
*/
void aRcConsoleLocal::Shutdown() {
	cmdSystem->RemoveCommand( "clear" );
	cmdSystem->RemoveCommand( "conDump" );

	debugGraphs.DeleteContents( true );
}

/*
================
aRcConsoleLocal::Active
================
*/
bool	aRcConsoleLocal::Active() {
	return keyCatching;
}

/*
================
aRcConsoleLocal::ClearNotifyLines
================
*/
void	aRcConsoleLocal::ClearNotifyLines() {
	int		i;

	for ( i = 0; i < NUM_CON_TIMES; i++ ) {
		times[i] = 0;
	}
}

/*
================
aRcConsoleLocal::Close
================
*/
void	aRcConsoleLocal::Close() {
	keyCatching = false;
	SetDisplayFraction( 0 );
	displayFrac = 0;	// don't scroll to that point, go immediately
	ClearNotifyLines();
}

/*
================
aRcConsoleLocal::Clear
================
*/
void aRcConsoleLocal::Clear() {
	int		i;

	for ( i = 0; i < CON_TEXTSIZE; i++ ) {
		text[i] = (arcNetString::ColorIndex(C_COLOR_CYAN)<<8) | ' ';
	}

	Bottom();		// go to end
}

/*
================
aRcConsoleLocal::Dump

Save the console contents out to a file
================
*/
void aRcConsoleLocal::Dump( const char *fileName ) {
	int		l, x, i;
	short *	line;
	arcNetFile *f;
	char	* buffer = (char *)alloca( LINE_WIDTH + 3 );

	f = fileSystem->OpenFileWrite( fileName );
	if ( !f ) {
		common->Warning( "Failed to open: %s", fileName );
		return;
	}

	// skip empty lines
	l = current - TOTAL_LINES + 1;
	if ( l < 0 ) {
		l = 0;
	}

	for (; l <= current; l++ ) {
		line = text + ( l % TOTAL_LINES ) * LINE_WIDTH;
		for ( x = 0; x < LINE_WIDTH; x++ ) {
			if ( ( line[x] & 0xff ) > ' ' ) {
				//buffer[x] = line[x] + 'A';
				break;
			}
		}
		if ( x != LINE_WIDTH ) {
			break;
		}
	}

	// write the remaining lines
	for (; l <= current; l++ ) {
		line = text + ( l % TOTAL_LINES ) * LINE_WIDTH;
		for ( i = 0; i < LINE_WIDTH; i++ ) {
			buffer[i] = line[i] & 0xff;
		}
		for ( x = LINE_WIDTH-1; x >= 0; x-- ) {
			if ( buffer[x] <= ' ' ) {
				buffer[x] = 0;
			} else {
				break;
			}
		}
		buffer[x+1] = '\r';
		buffer[x+2] = '\n';
		buffer[x+3] = 0;
		f->Write( buffer, strlen( buffer ) );
	}

	fileSystem->CloseFile( f );
}

/*
================
aRcConsoleLocal::PageUp
================
*/
void aRcConsoleLocal::PageUp() {
	display -= 2;
	if ( current - display >= TOTAL_LINES ) {
		display = current - TOTAL_LINES + 1;
	}
}

/*
================
aRcConsoleLocal::PageDown
================
*/
void aRcConsoleLocal::PageDown() {
	display += 2;
	if ( display > current ) {
		display = current;
	}
}

/*
================
aRcConsoleLocal::Top
================
*/
void aRcConsoleLocal::Top() {
	display = 0;
}

/*
================
aRcConsoleLocal::Bottom
================
*/
void aRcConsoleLocal::Bottom() {
	display = current;
}

/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
====================
KeyDownEvent

Handles history and console scrollback
====================
*/
void aRcConsoleLocal::KeyDownEvent( int key ) {
	// Execute F key bindings
	if ( key >= K_F1 && key <= K_F12 ) {
		idKeyInput::ExecKeyBinding( key );
		return;
	}

	// ctrl-L clears screen
	if ( key == K_L && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) {
		Clear();
		return;
	}

	// enter finishes the line
	if ( key == K_ENTER || key == K_KP_ENTER ) {
		common->Printf ( "]%s\n", consoleField.GetBuffer() );

		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, consoleField.GetBuffer() );	// valid command
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );

		// copy line to history buffer

		if ( consoleField.GetBuffer()[ 0 ] != '\n' && consoleField.GetBuffer()[ 0 ] != '\0' ) {
			consoleHistory.AddToHistory( consoleField.GetBuffer() );
		}

		consoleField.Clear();
		consoleField.SetWidthInChars( LINE_WIDTH );

		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );// force an update, because the command
								// may take some time
		return;
	}

	// command completion
	if ( key == K_TAB ) {
		consoleField.AutoComplete();
		return;
	}

	// command history (ctrl-p ctrl-n for unix style)
	if ( ( key == K_UPARROW ) ||
		 ( key == K_P && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) ) {
		arcNetString hist = consoleHistory.RetrieveFromHistory( true );
		if ( !hist.IsEmpty() ) {
			consoleField.SetBuffer( hist );
		}
		return;
	}

	if ( ( key == K_DOWNARROW ) ||
		 ( key == K_N && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) ) {
		arcNetString hist = consoleHistory.RetrieveFromHistory( false );
		if ( !hist.IsEmpty() ) {
			consoleField.SetBuffer( hist );
		}
		return;
	}

	// console scrolling
	if ( key == K_PGUP ) {
		PageUp();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if ( key == K_PGDN ) {
		PageDown();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if ( key == K_MWHEELUP ) {
		PageUp();
		return;
	}

	if ( key == K_MWHEELDOWN ) {
		PageDown();
		return;
	}

	// ctrl-home = top of console
	if ( key == K_HOME && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) {
		Top();
		return;
	}

	// ctrl-end = bottom of console
	if ( key == K_END && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) ) {
		Bottom();
		return;
	}

	// pass to the normal editline routine
	consoleField.KeyDownEvent( key );
}

/*
==============
Scroll
deals with scrolling text because we don't have key repeat
==============
*/
void aRcConsoleLocal::Scroll() {
	if (lastKeyEvent == -1 || ( lastKeyEvent + 200 ) > eventLoop->Milliseconds() ) {
		return;
	}
	// console scrolling
	if ( idKeyInput::IsDown( K_PGUP ) ) {
		PageUp();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}

	if ( idKeyInput::IsDown( K_PGDN ) ) {
		PageDown();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}
}

/*
==============
SetDisplayFraction

Causes the console to start opening the desired amount.
==============
*/
void aRcConsoleLocal::SetDisplayFraction( float frac ) {
	finalFrac = frac;
	fracTime = Sys_Milliseconds();
}

/*
==============
UpdateDisplayFraction

Scrolls the console up or down based on conspeed
==============
*/
void aRcConsoleLocal::UpdateDisplayFraction() {
	if ( con_speed.GetFloat() <= 0.1f ) {
		fracTime = Sys_Milliseconds();
		displayFrac = finalFrac;
		return;
	}

	// scroll towards the destination height
	if ( finalFrac < displayFrac ) {
		displayFrac -= con_speed.GetFloat() * ( Sys_Milliseconds() - fracTime ) * 0.001f;
		if ( finalFrac > displayFrac ) {
			displayFrac = finalFrac;
		}
		fracTime = Sys_Milliseconds();
	} else if ( finalFrac > displayFrac ) {
		displayFrac += con_speed.GetFloat() * ( Sys_Milliseconds() - fracTime ) * 0.001f;
		if ( finalFrac < displayFrac ) {
			displayFrac = finalFrac;
		}
		fracTime = Sys_Milliseconds();
	}
}

/*
==============
ProcessEvent
==============
*/
bool	aRcConsoleLocal::ProcessEvent( const sysEvent_t *event, bool forceAccept ) {
	const bool consoleKey = event->evType == SE_KEY && event->evValue == K_GRAVE && com_allowConsole.GetBool();

	// we always catch the console key event
	if ( !forceAccept && consoleKey ) {
		// ignore up events
		if ( event->evValue2 == 0 ) {
			return true;
		}

		consoleField.ClearAutoComplete();

		// a down event will toggle the destination lines
		if ( keyCatching ) {
			Close();
			Sys_GrabMouseCursor( true );
		} else {
			consoleField.Clear();
			keyCatching = true;
			if ( idKeyInput::IsDown( K_LSHIFT ) || idKeyInput::IsDown( K_RSHIFT ) ) {
				// if the shift key is down, don't open the console as much
				SetDisplayFraction( 0.2f );
			} else {
				SetDisplayFraction( 0.5f );
			}
		}
		return true;
	}

	// if we aren't key catching, dump all the other events
	if ( !forceAccept && !keyCatching ) {
		return false;
	}

	// handle key and character events
	if ( event->evType == SE_CHAR ) {
		// never send the console key as a character
		if ( event->evValue != '`' && event->evValue != '~' ) {
			consoleField.CharEvent( event->evValue );
		}
		return true;
	}

	if ( event->evType == SE_KEY ) {
		// ignore up key events
		if ( event->evValue2 == 0 ) {
			return true;
		}

		KeyDownEvent( event->evValue );
		return true;
	}

	// we don't handle things like mouse, joystick, and network packets
	return false;
}

/*
==============================================================================

PRINTING

==============================================================================
*/

/*
===============
Linefeed
===============
*/
void aRcConsoleLocal::Linefeed() {
	int		i;

	// mark time for transparent overlay
	if ( current >= 0 ) {
		times[current % NUM_CON_TIMES] = Sys_Milliseconds();
	}

	x = 0;
	if ( display == current ) {
		display++;
	}
	current++;
	for ( i = 0; i < LINE_WIDTH; i++ ) {
		int offset = ( ( unsigned int )current % TOTAL_LINES ) * LINE_WIDTH + i;
		text[offset] = ( arcNetString::ColorIndex( C_COLOR_CYAN )<<8 ) | ' ';
	}
}


/*
================
Print

Handles cursor positioning, line wrapping, etc
================
*/
void aRcConsoleLocal::Print( const char *txt ) {
	int		y;
	int		c, l;
	int		color;

	if ( TOTAL_LINES == 0 ) {
		// not yet initialized
		return;
	}

	color = arcNetString::ColorIndex( C_COLOR_CYAN );

	while ( (c = *( const unsigned char* )txt) != 0 ) {
		if ( arcNetString::IsColor( txt ) ) {
			if ( *(txt+1 ) == C_COLOR_DEFAULT ) {
				color = arcNetString::ColorIndex( C_COLOR_CYAN );
			} else {
				color = arcNetString::ColorIndex( *( txt+1 ) );
			}
			txt += 2;
			continue;
		}

		y = current % TOTAL_LINES;

		// if we are about to print a new word, check to see
		// if we should wrap to the new line
		if ( c > ' ' && ( x == 0 || text[y*LINE_WIDTH+x-1] <= ' ' ) ) {
			// count word length
			for ( l=0; l< LINE_WIDTH; l++ ) {
				if ( txt[l] <= ' ') {
					break;
				}
			}

			// word wrap
			if (l != LINE_WIDTH && (x + l >= LINE_WIDTH ) ) {
				Linefeed();
			}
		}

		txt++;

		switch( c ) {
			case '\n':
				Linefeed ();
				break;
			case '\t':
				do {
					text[y*LINE_WIDTH+x] = ( color << 8 ) | ' ';
					x++;
					if ( x >= LINE_WIDTH ) {
						Linefeed();
						x = 0;
					}
				} while ( x & 3 );
				break;
			case '\r':
				x = 0;
				break;
			default:	// display character and advance
				text[y*LINE_WIDTH+x] = ( color << 8 ) | c;
				x++;
				if ( x >= LINE_WIDTH ) {
					Linefeed();
					x = 0;
				}
				break;
		}
	}
	// mark time for transparent overlay
	if ( current >= 0 ) {
		times[current % NUM_CON_TIMES] = Sys_Milliseconds();
	}
}

/*
==============================================================================

DRAWING

==============================================================================
*/

/*
================
DrawInput

Draw the editline after a ] prompt
================
*/
void aRcConsoleLocal::DrawInput() {
	int y, autoCompleteLength;

	y = vislines - ( SMALLCHAR_HEIGHT * 2 );

	if ( consoleField.GetAutoCompleteLength() != 0 ) {
		autoCompleteLength = strlen( consoleField.GetBuffer() ) - consoleField.GetAutoCompleteLength();
		if ( autoCompleteLength > 0 ) {
			renderSystem->DrawFilled( arcVec4( 0.8f, 0.2f, 0.2f, 0.45f ), LOCALSAFE_LEFT + 2 * SMALLCHAR_WIDTH + consoleField.GetAutoCompleteLength() * SMALLCHAR_WIDTH, y + 2, autoCompleteLength * SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT - 2 );
		}
	}

	renderSystem->SetColor( arcNetString::ColorForIndex( C_COLOR_CYAN ) );

	renderSystem->DrawSmallChar( LOCALSAFE_LEFT + 1 * SMALLCHAR_WIDTH, y, ']' );
	consoleField.Draw( LOCALSAFE_LEFT + 2 * SMALLCHAR_WIDTH, y, SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, true );
}


/*
================
DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void aRcConsoleLocal::DrawNotify() {
	int		x, v;
	short	*text_p;
	int		i;
	int		time;
	int		currentColor;

	if ( con_noPrint.GetBool() ) {
		return;
	}

	currentColor = arcNetString::ColorIndex( C_COLOR_WHITE );
	renderSystem->SetColor( arcNetString::ColorForIndex( currentColor ) );

	v = 0;

	for ( i = current-NUM_CON_TIMES+1; i <= current; i++ ) {
		if ( i < 0 ) {
			continue;
		}
		time = times[i % NUM_CON_TIMES];
		if ( time == 0 ) {
			continue;
		}
		time = Sys_Milliseconds() - time;
		if ( time > con_notifyTime.GetFloat() * 1000 ) {
			continue;
		}
		text_p = text + ( i % TOTAL_LINES)*LINE_WIDTH;

		for ( x = 0; x < LINE_WIDTH; x++ ) {
			if ( ( text_p[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( arcNetString::ColorIndex(text_p[x]>>8) != currentColor ) {
				currentColor = arcNetString::ColorIndex(text_p[x]>>8);
				renderSystem->SetColor( arcNetString::ColorForIndex( currentColor ) );
			}
			renderSystem->DrawSmallChar( LOCALSAFE_LEFT + ( x + 1 )*SMALLCHAR_WIDTH, v, text_p[x] & 0xff );
		}

		v += SMALLCHAR_HEIGHT;
	}

	renderSystem->SetColor( colorCyan );
}

/*
================
DrawSolidConsole

Draws the console with the solid background
================
*/
void aRcConsoleLocal::DrawSolidConsole( float frac ) {
	int				i, x;
	float			y;
	int				rows;
	short			*text_p;
	int				row;
	int				lines;
	int				currentColor;

	lines = arcMath::Ftoi( SCREEN_HEIGHT * frac );
	if ( lines <= 0 ) {
		return;
	}

	if ( lines > SCREEN_HEIGHT ) {
		lines = SCREEN_HEIGHT;
	}

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1.0f ) {
		y = 0.0f;
	} else {
		renderSystem->DrawFilled( arcVec4( 0.0f, 0.0f, 0.0f, 0.75f ), 0, 0, SCREEN_WIDTH, y );
	}

	renderSystem->DrawFilled( colorCyan, 0, y, SCREEN_WIDTH, 2 );

	// draw the version number

	renderSystem->SetColor( arcNetString::ColorForIndex( C_COLOR_CYAN ) );

	arcNetString version = va( "%s.%i.%i", ENGINE_VERSION, BUILD_NUMBER, BUILD_NUMBER_MINOR );
	i = version.Length();

	for ( x = 0; x < i; x++ ) {
		renderSystem->DrawSmallChar( LOCALSAFE_WIDTH - ( i - x ) * SMALLCHAR_WIDTH,
			( lines-( SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/4 ) ), version[x] );

	}


	// draw the text
	vislines = lines;
	rows = ( lines-SMALLCHAR_WIDTH )/SMALLCHAR_WIDTH;	// rows of text to draw

	y = lines - ( SMALLCHAR_HEIGHT*3 );

	// draw from the bottom up
	if ( display != current ) {
		// draw arrows to show the buffer is backscrolled
		renderSystem->SetColor( arcNetString::ColorForIndex( C_COLOR_CYAN ) );
		for ( x = 0; x < LINE_WIDTH; x += 4 ) {
			renderSystem->DrawSmallChar( LOCALSAFE_LEFT + ( x+1 )*SMALLCHAR_WIDTH, arcMath::Ftoi( y ), '^' );
		}
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	row = display;

	if ( x == 0 ) {
		row--;
	}

	currentColor = arcNetString::ColorIndex( C_COLOR_WHITE );
	renderSystem->SetColor( arcNetString::ColorForIndex( currentColor ) );

	for ( i = 0; i < rows; i++, y -= SMALLCHAR_HEIGHT, row-- ) {
		if ( row < 0 ) {
			break;
		}
		if ( current - row >= TOTAL_LINES ) {
			// past scrollback wrap point
			continue;
		}

		text_p = text + ( row % TOTAL_LINES )*LINE_WIDTH;

		for ( x = 0; x < LINE_WIDTH; x++ ) {
			if ( ( text_p[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( arcNetString::ColorIndex( text_p[x]>>8 ) != currentColor ) {
				currentColor = arcNetString::ColorIndex( text_p[x]>>8 );
				renderSystem->SetColor( arcNetString::ColorForIndex( currentColor ) );
			}
			renderSystem->DrawSmallChar( LOCALSAFE_LEFT + ( x + 1 )*SMALLCHAR_WIDTH, arcMath::Ftoi( y ), text_p[x] & 0xff );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	DrawInput();

	renderSystem->SetColor( colorCyan );
}


/*
==============
Draw

ForceFullScreen is used by the editor
==============
*/
void aRcConsoleLocal::Draw( bool forceFullScreen ) {
	if ( forceFullScreen ) {
		// if we are forced full screen because of a disconnect,
		// we want the console closed when we go back to a session state
		Close();
		// we are however catching keyboard input
		keyCatching = true;
	}

	Scroll();

	UpdateDisplayFraction();

	if ( forceFullScreen ) {
		DrawSolidConsole( 1.0f );
	} else if ( displayFrac ) {
		DrawSolidConsole( displayFrac );
	} else {
		// only draw the notify lines if the developer cvar is set,
		// or we are a debug build
		if ( !con_noPrint.GetBool() ) {
			DrawNotify();
		}
	}

	float lefty = LOCALSAFE_TOP;
	float righty = LOCALSAFE_TOP;
	float centery = LOCALSAFE_TOP;
	if ( com_showFPS.GetBool() ) {
		righty = DrawFPS( righty );
	}
	if ( com_showMemoryUsage.GetBool() ) {
		righty = DrawMemoryUsage( righty );
	}
	DrawOverlayText( lefty, righty, centery );
	DrawDebugGraphs();
}

/*
========================
aRcConsoleLocal::PrintOverlay
========================
*/
void aRcConsoleLocal::PrintOverlay( idOverlayHandle &handle, justify_t justify, const char *text, ... ) {
	if ( handle.index >= 0 && handle.index < overlayText.Num() ) {
		if ( overlayText[handle.index].time == handle.time ) {
			return;
		}
	}

	char string[MAX_PRINT_MSG];
	va_list argptr;
	va_start( argptr, text );
	arcNetString::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );

	overlayText_t &overlay = overlayText.Alloc();
	overlay.text = string;
	overlay.justify = justify;
	overlay.time = Sys_Milliseconds();

	handle.index = overlayText.Num() - 1;
	handle.time = overlay.time;
}

/*
========================
aRcConsoleLocal::DrawOverlayText
========================
*/
void aRcConsoleLocal::DrawOverlayText( float & leftY, float & rightY, float & centerY ) {
	for ( int i = 0; i < overlayText.Num(); i++ ) {
		const arcNetString & text = overlayText[i].text;

		int maxWidth = 0;
		int numLines = 0;
		for ( int j = 0; j < text.Length(); j++ ) {
			int width = 1;
			for (; j < text.Length() && text[j] != '\n'; j++ ) {
				width++;
			}
			numLines++;
			if ( width > maxWidth ) {
				maxWidth = width;
			}
		}

		arcVec4 bgColor( 0.0f, 0.0f, 0.0f, 0.75f );

		const float width = maxWidth * SMALLCHAR_WIDTH;
		const float height = numLines * ( SMALLCHAR_HEIGHT + 4 );
		const float bgAdjust = - 0.5f * SMALLCHAR_WIDTH;
		if ( overlayText[i].justify == JUSTIFY_LEFT ) {
			renderSystem->DrawFilled( bgColor, LOCALSAFE_LEFT + bgAdjust, leftY, width, height );
		} else if ( overlayText[i].justify == JUSTIFY_RIGHT ) {
			renderSystem->DrawFilled( bgColor, LOCALSAFE_RIGHT - width + bgAdjust, rightY, width, height );
		} else if ( overlayText[i].justify == JUSTIFY_CENTER_LEFT || overlayText[i].justify == JUSTIFY_CENTER_RIGHT ) {
			renderSystem->DrawFilled( bgColor, LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH - width + bgAdjust ) * 0.5f, centerY, width, height );
		} else {
			assert( false );
		}

		arcNetString singleLine;
		for ( int j = 0; j < text.Length(); j += singleLine.Length() + 1 ) {
			singleLine = "";
			for ( int k = j; k < text.Length() && text[k] != '\n'; k++ ) {
				singleLine.Append( text[k] );
			}
			if ( overlayText[i].justify == JUSTIFY_LEFT ) {
				DrawTextLeftAlign( LOCALSAFE_LEFT, leftY, "%s", singleLine.c_str() );
			} else if ( overlayText[i].justify == JUSTIFY_RIGHT ) {
				DrawTextRightAlign( LOCALSAFE_RIGHT, rightY, "%s", singleLine.c_str() );
			} else if ( overlayText[i].justify == JUSTIFY_CENTER_LEFT ) {
				DrawTextLeftAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH - width ) * 0.5f, centerY, "%s", singleLine.c_str() );
			} else if ( overlayText[i].justify == JUSTIFY_CENTER_RIGHT ) {
				DrawTextRightAlign( LOCALSAFE_LEFT + ( LOCALSAFE_WIDTH + width ) * 0.5f, centerY, "%s", singleLine.c_str() );
			} else {
				assert( false );
			}
		}
	}
	overlayText.SetNum( 0 );
}

/*
========================
aRcConsoleLocal::CreateGraph
========================
*/
arcDebugGraph * aRcConsoleLocal::CreateGraph( int numItems ) {
	arcDebugGraph * graph = new ( TAG_SYSTEM ) arcDebugGraph( numItems );
	debugGraphs.Append( graph );
	return graph;
}

/*
========================
aRcConsoleLocal::DestroyGraph
========================
*/
void aRcConsoleLocal::DestroyGraph( arcDebugGraph * graph ) {
	debugGraphs.Remove( graph );
	delete graph;
}

/*
========================
aRcConsoleLocal::DrawDebugGraphs
========================
*/
void aRcConsoleLocal::DrawDebugGraphs() {
	for ( int i = 0; i < debugGraphs.Num(); i++ ) {
		debugGraphs[i]->Render( renderSystem );
	}
}
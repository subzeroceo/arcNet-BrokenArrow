#ifndef __CONSOLE_H__
#define __CONSOLE_H__

enum justify_t {
	JUSTIFY_LEFT,
	JUSTIFY_RIGHT,
	JUSTIFY_CENTER_LEFT,
	JUSTIFY_CENTER_RIGHT
};

class idOverlayHandle {
friend class aRcConsoleLocal;
public:
			idOverlayHandle() : index( -1 ), time( 0 ) {}
private:
	int		index;
	int		time;
};

/*
===============================================================================

	The console is strictly for development and advanced users. It should
	never be used to convey actual game information to the user, which should
	always be done through a GUI.

	The force options are for the editor console display window, which
	doesn't respond to pull up / pull down

===============================================================================
*/

class aRcConsole {
public:
	virtual				~aRcConsole() {}

	virtual void		Init() = 0;
	virtual void		Shutdown() = 0;

	virtual bool		ProcessEvent( const sysEvent_t * event, bool forceAccept ) = 0;

	// the system code can release the mouse pointer when the console is active
	virtual bool		Active() = 0;

	// clear the timers on any recent prints that are displayed in the notify lines
	virtual void		ClearNotifyLines() = 0;

	// some console commands, like timeDemo, will force the console closed before they start
	virtual void		Close() = 0;

	virtual void		Draw( bool forceFullScreen ) = 0;
	virtual void		Print( const char *text ) = 0;

	virtual void		PrintOverlay( idOverlayHandle & handle, justify_t justify, VERIFY_FORMAT_STRING const char *text, ... ) = 0;

	virtual arcDebugGraph *CreateGraph( int numItems ) = 0;
	virtual void		DestroyGraph( arcDebugGraph * graph ) = 0;
};

extern aRcConsole *	console; // statically initialized to an aRcConsoleLocal

#endif
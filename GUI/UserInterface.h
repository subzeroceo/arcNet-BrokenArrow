#ifndef __USERINTERFACE_H__
#define __USERINTERFACE_H__

/*
===============================================================================

	Draws an interactive 2D surface.
	Used for all user interaction with the game.

===============================================================================
*/

class anFile;
class anSavedGamesFile;

class anUserInterface {
public:
				~anUserInterface() {};
								// Returns the name of the gui.
	const char *				Name() const;

								// Returns a comment on the gui.
	const char *				Comment() const;

								// Returns true if the gui is interactive.
	bool						IsInteractive() const;

	bool						IsUniqued() const;

	void						SetUniqued( bool b );
								// returns false if it failed to load
	bool						InitFromFile( const char *path, bool rebuild = true, bool cache = true );

								// handles an event, can return an action string, the caller interprets
								// any return and acts accordingly
	const char *				HandleEvent( const sysEvent_t *event, int time, bool *updateVisuals = nullptr );

								// handles a named event
	void						HandleNamedEvent( const char *eventName );

								// repaints the ui
	void						Redraw( int time );

								// repaints the cursor
	void						DrawCursor();

								// Provides read access to the anDict that holds this gui's state.
	const anDict &				State() const;

								// Removes a gui state variable
	void						DeleteStateVar( const char *varName );

								// Sets a gui state variable.
	void						SetStateString( const char *varName, const char *value );
	void						SetStateBool( const char *varName, const bool value );
	void						SetStateInt( const char *varName, const int value );
	void						SetStateFloat( const char *varName, const float value );

								// Gets a gui state variable
	const char*					GetStateString( const char *varName, const char *defaultString = "" ) const;
	bool						GetStateBool( const char *varName, const char *defaultString = "0" ) const;
	int							GetStateInt( const char *varName, const char *defaultString = "0" ) const;
	float						GetStateFloat( const char *varName, const char *defaultString = "0" ) const;

								// The state has changed and the gui needs to update from the state anDict.
	void						StateChanged( int time, bool redraw = false );

								// Activated the gui.
	const char *				Activate( bool activate, int time );

								// Triggers the gui and runs the onTrigger scripts.
	void						Trigger( int time );

	oid							ReadFromDemoFile( class anSavedGamesFile *f );
	void						WriteToDemoFile( class anSavedGamesFile *f );

	bool						WriteToSaveGame( anFile *savefile ) const;
	bool						ReadFromSaveGame( anFile *savefile );
	void						SetKeyBindingNames( void );

	bool						IsUniqued() const { return uniqued; };

	void						SetCursor( float x, float y );
	float						CursorX() { return cursorX; };
	float						CursorY() return cursorY; };

	size_t						Size();

	anDict *					GetStateDict() { return &state; }

	const char *				GetSourceFile( void ) const { return source; }
	ARC_TIME_T					GetTimeStamp( void ) const { return timeStamp; }
	idWindow *					GetDesktop() const { return desktop; }
	void						SetBindHandler( idWindow *win ) { bindHandler = win; }
	bool						Active() const { return active; }
	int							GetTime() const { return time; }
	void						SetTime( int _time ) { time = _time; }

	void						ClearRefs() { refs = 0; }
	void						AddRef() { refs++; }
	int							GetRefs() { return refs; }

	void						RecurseSetKeyBindingNames( idWindow *window );
	anStr &					GetPendingCmd() { return pendingCmd; };
	anStr &					GetReturnCmd() { return returnCmd; };
private:
	bool						active;
	bool						loading;
	bool						interactive;
	bool						uniqued;

	anDict						state;
	idWindow *					desktop;
	idWindow *					bindHandler;

	anStr					source;
	anStr					activateStr;
	anStr					pendingCmd;
	anStr					returnCmd;
	ARC_TIME_T					timeStamp;

	float						cursorX;
	float						cursorY;

	int							time;

	int							refs
};


class anUserInterfaceManager {
public:
				~anUserInterfaceManager( void ) {};

	void						Init();
	void						Shutdown();
	void						Touch( const char *name );
	void						WritePrecacheCommands( anFile *f );

								// Sets the size for 640x480 adjustment.
	void						SetSize( float width, float height );

	void						BeginLevelLoad();
	void						EndLevelLoad();

								// Reloads changed guis, or all guis.
	void						Reload( bool all );

								// lists all guis
	void						ListGuis() const;

								// Returns true if gui exists.
	bool						CheckGui( const char *path ) const;

								// Allocates a new gui.
	anUserInterface *			Alloc( void ) const;

								// De-allocates a gui.. ONLY USE FOR PRECACHING
	void						DeAlloc( anUserInterface *gui );

								// Returns nullptr if gui by that name does not exist.
	anUserInterface *			FindGui( const char *path, bool autoLoad = false, bool needUnique = false, bool forceUnique = false );

								// Returns nullptr if gui by that name does not exist.
	anUserInterface *			FindDemoGui( const char *path );

								// Allocates a new GUI list handler
	virtual	anListGUI *			AllocListGUI( void ) const;

								// De-allocates a list gui
	void						FreeListGUI( anListGUI *listgui );

private:

	idRectangle					screenRect;
	idDeviceContext				dc;

	anList<anUserInterface*> guis;
	anList<anUserInterface*> demoGuis;
};

extern anUserInterfaceManager *	uiManager;

#endif // !__USERINTERFACE_H__

#ifdef 0
class idWindow;

class anUserInterface : public anUserInterface {
	friend class anUserInterfaceManager;
public:
								anUserInterface();
	virtual						~anUserInterface();

	virtual const char *		Name() const;
	virtual const char *		Comment() const;
	virtual bool				IsInteractive() const;
	virtual bool				InitFromFile( const char *qpath, bool rebuild = true, bool cache = true );
	virtual const char *		HandleEvent( const sysEvent_t *event, int time, bool *updateVisuals );
	virtual void				HandleNamedEvent( const char *namedEvent );
	virtual void				Redraw( int time );
	virtual void				DrawCursor();
	virtual const anDict &		State() const;
	virtual void				DeleteStateVar( const char *varName );
	virtual void				SetStateString( const char *varName, const char *value );
	virtual void				SetStateBool( const char *varName, const bool value );
	virtual void				SetStateInt( const char *varName, const int value );
	virtual void				SetStateFloat( const char *varName, const float value );

	// Gets a gui state variable
	virtual const char*			GetStateString( const char *varName, const char *defaultString = "" ) const;
	virtual bool				GetStateBool( const char *varName, const char *defaultString = "0" ) const;
	virtual int					GetStateInt( const char *varName, const char *defaultString = "0" ) const;
	virtual float				GetStateFloat( const char *varName, const char *defaultString = "0" ) const;

	virtual void				StateChanged( int time, bool redraw );
	virtual const char *		Activate( bool activate, int time );
	virtual void				Trigger( int time );
	virtual void				ReadFromDemoFile( class anSavedGamesFile *f );
	virtual void				WriteToDemoFile( class anSavedGamesFile *f );
	virtual bool				WriteToSaveGame( anFile *savefile ) const;
	virtual bool				ReadFromSaveGame( anFile *savefile );
	virtual void				SetKeyBindingNames( void );
	virtual bool				IsUniqued() const { return uniqued; };
	virtual void				SetUniqued( bool b ) { uniqued = b; };
	virtual void				SetCursor( float x, float y );

	virtual float				CursorX() { return cursorX; }
	virtual float				CursorY() { return cursorY; }

	size_t						Size();

	anDict *					GetStateDict() { return &state; }

	const char *				GetSourceFile( void ) const { return source; }
	ARC_TIME_T						GetTimeStamp( void ) const { return timeStamp; }

	idWindow *					GetDesktop() const { return desktop; }
	void						SetBindHandler( idWindow *win ) { bindHandler = win; }
	bool						Active() const { return active; }
	int							GetTime() const { return time; }
	void						SetTime( int _time ) { time = _time; }

	void						ClearRefs() { refs = 0; }
	void						AddRef() { refs++; }
	int							GetRefs() { return refs; }

	void						RecurseSetKeyBindingNames( idWindow *window );
	anStr						&GetPendingCmd() { return pendingCmd; };
	anStr						&GetReturnCmd() { return returnCmd; };

private:
	bool						active;
	bool						loading;
	bool						interactive;
	bool						uniqued;

	anDict						state;
	idWindow *					desktop;
	idWindow *					bindHandler;

	anStr						source;
	anStr						activateStr;
	anStr						pendingCmd;
	anStr						returnCmd;
	ARC_TIME_T						timeStamp;

	float						cursorX;
	float						cursorY;

	int							time;

	int							refs;
};

class anUserInterfaceManager : public anUserInterfaceManager {
	friend class anUserInterface;

public:
	virtual void				Init();
	virtual void				Shutdown();
	virtual void				Touch( const char *name );
	virtual void				WritePrecacheCommands( anFile *f );
	virtual void				SetSize( float width, float height );
	virtual void				BeginLevelLoad();
	virtual void				EndLevelLoad();
	virtual void				Reload( bool all );
	virtual void				ListGuis() const;
	virtual bool				CheckGui( const char *qpath ) const;
	virtual anUserInterface *	Alloc( void ) const;
	virtual void				DeAlloc( anUserInterface *gui );
	virtual anUserInterface *	FindGui( const char *qpath, bool autoLoad = false, bool needInteractive = false, bool forceUnique = false );
	virtual anUserInterface *	FindDemoGui( const char *qpath );
	virtual	anListGUI *			AllocListGUI( void ) const;
	virtual void				FreeListGUI( anListGUI *listgui );

private:
	idRectangle					screenRect;
	idDeviceContext				dc;

	anList<anUserInterface*> guis;
	anList<anUserInterface*> demoGuis;

};

#endif
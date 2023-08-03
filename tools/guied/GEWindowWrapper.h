#ifndef GEWINDOWWRAPPER_H_
#define GEWINDOWWRAPPER_H_

class idWindow;
class rvGEWindowWrapper;

typedef bool (*PFNENUMCHILDRENPROC) ( rvGEWindowWrapper* wrapper, void* data );

class rvGEWindowWrapper {
public:

	enum EWindowType {
		WT_UNKNOWN,
		WT_NORMAL,
		WT_EDIT,
		WT_HTML,
		WT_CHOICE,
		WT_SLIDER,
		WT_BIND,
		WT_LIST,
		WT_RENDER,
		WT_TRANSITION
	};

	rvGEWindowWrapper ( idWindow* window, EWindowType type );

	static rvGEWindowWrapper*	GetWrapper ( idWindow* window );

	idWindow*			GetWindow			( void );
	anDict&				GetStateDict		( void );
	anDict&				GetVariableDict		( void );
	anDict&				GetScriptDict		( void );
	idRectangle&		GetClientRect		( void );
	idRectangle&		GetScreenRect		( void );
	EWindowType			GetWindowType		( void );
	int					GetChildCount		( void );
	int					GetDepth			( void );
	idWindow*			GetChild			( int index );

	void				SetRect				( idRectangle& rect );
	void				SetState			( const anDict& dict );
	void				SetStateKey			( const char* key, const char* value, bool update = true );
	void				DeleteStateKey		( const char* key );
	bool				VerfiyStateKey		( const char* name, const char* value, anString* result = nullptr );

	bool				IsFlippedHorz		( void );
	bool				IsFlippedVert		( void );
	bool				IsHidden			( void );
	bool				IsDeleted			( void );
	bool				IsSelected			( void );
	bool				IsExpanded			( void );

	bool				CanHaveChildren		( void );
	bool				CanMoveAndSize		( void );

	void				SetFlippedHorz		( bool f );
	void				SetFlippedVert		( bool f );
	void				SetHidden			( bool v );
	void				SetDeleted			( bool del );
	void				SetSelected			( bool sel );
	void				SetWindowType		( EWindowType type );

	bool				Expand				( void );
	bool				Collapse			( void );

	bool				EnumChildren		( PFNENUMCHILDRENPROC proc, void* data );

	idWindow*			WindowFromPoint		( float x, float y, bool visibleOnly = true );

	void				Finish				( void );

	static EWindowType	StringToWindowType		( const char* string );
	static const char*	WindowTypeToString		( EWindowType type );

protected:

	void				CalcScreenRect		( void );
	void				UpdateRect			( void );
	void				UpdateWindowState 	( void );

	idRectangle		mClientRect;
	idRectangle		mScreenRect;
	idWindow*		mWindow;
	anDict			mState;
	anDict			mVariables;
	anDict			mScripts;
	bool			mFlippedHorz;
	bool			mFlippedVert;
	bool			mHidden;
	bool			mDeleted;
	bool			mSelected;
	bool			mExpanded;
	bool			mOldVisible;
	bool			mMoveable;
	EWindowType		mType;
};

ARC_INLINE anDict& rvGEWindowWrapper::GetStateDict ( void )
{
	return mState;
}

ARC_INLINE anDict& rvGEWindowWrapper::GetVariableDict ( void )
{
	return mVariables;
}

ARC_INLINE anDict& rvGEWindowWrapper::GetScriptDict ( void )
{
	return mScripts;
}

ARC_INLINE bool rvGEWindowWrapper::IsFlippedHorz ( void )
{
	return mFlippedHorz;
}

ARC_INLINE bool rvGEWindowWrapper::IsFlippedVert ( void )
{
	return mFlippedVert;
}

ARC_INLINE bool rvGEWindowWrapper::IsExpanded ( void )
{
	return mExpanded;
}

ARC_INLINE void rvGEWindowWrapper::SetFlippedHorz ( bool f )
{
	mFlippedHorz = f;
}

ARC_INLINE void rvGEWindowWrapper::SetFlippedVert ( bool f )
{
	mFlippedVert = f;
}

ARC_INLINE idRectangle& rvGEWindowWrapper::GetClientRect ( void )
{
	return mClientRect;
}

ARC_INLINE idRectangle& rvGEWindowWrapper::GetScreenRect ( void )
{
	return mScreenRect;
}

ARC_INLINE bool rvGEWindowWrapper::IsHidden ( void )
{
	return mHidden;
}

ARC_INLINE bool rvGEWindowWrapper::IsDeleted ( void )
{
	return mDeleted;
}

ARC_INLINE bool rvGEWindowWrapper::IsSelected ( void )
{
	return mSelected;
}

ARC_INLINE void rvGEWindowWrapper::SetSelected ( bool sel )
{
	mSelected = sel;
}

ARC_INLINE rvGEWindowWrapper::EWindowType rvGEWindowWrapper::GetWindowType ( void )
{
	return mType;
}

ARC_INLINE void rvGEWindowWrapper::SetWindowType ( rvGEWindowWrapper::EWindowType type )
{
	mType = type;
}

ARC_INLINE idWindow* rvGEWindowWrapper::GetChild ( int index )
{
	return mWindow->GetChild ( index );
}

ARC_INLINE idWindow* rvGEWindowWrapper::GetWindow ( void )
{
	return mWindow;
}

ARC_INLINE bool rvGEWindowWrapper::CanMoveAndSize ( void )
{
	return mMoveable;
}

#endif // GEWINDOWWRAPPER_H_

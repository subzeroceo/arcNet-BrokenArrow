#ifndef DIALOGHELPERS_H_
#define DIALOGHELPERS_H_

class rvDialogItem {
public:

	HWND	mWindow;
	int		mID;

	rvDialogItem ( int id ) { mID = id; }

	void Cache ( HWND parent ) {
		mWindow = GetDlgItem ( parent, mID );
	}

	void Check ( bool checked ) {
		SendMessage ( mWindow, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0 );
	}

	void Enable ( bool enable ) {
		EnableWindow ( mWindow, enable );
	}

	bool IsChecked ( void ) {
		return SendMessage ( mWindow, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false;
	}

	void SetText ( const char* text ) {
		SetWindowText ( mWindow, text );
	}

	void GetText ( arcNetString& out ) {
		char text[4096];
		GetWindowText ( mWindow, text, 4095 );
		out = text;
	}

	float GetFloat ( void )
	{
		arcNetString text;
		GetText ( text );
		return atof( text );
	}

	void SetFloat ( float f ) {
		SetText ( va( "%g", f ) );
	}

	operator HWND( void ) const { return mWindow; }
};

class rvDialogItemContainer {
protected:

	void Cache ( HWND parent, int count )
	{
		int				i;
		unsigned char*	ptr;

		ptr = (unsigned char*)this;
		for ( i = 0; i < count; i ++, ptr += sizeof(rvDialogItem) )
		{
			((rvDialogItem*)ptr)->Cache ( parent );
		}
	}
};

#define DIALOGITEM_BEGIN( name )									\
class name : public rvDialogItemContainer						\
{															\
public:															\
	name ( void ) { }											\
	name ( HWND hwnd ) { Cache ( hwnd ); }						\
	void Cache ( HWND parent )									\
	{															\
		rvDialogItemContainer::Cache ( parent, sizeof(*this)/sizeof(rvDialogItem) );	\
	}


#define DIALOGITEM(id,name)										\
class c##name : public rvDialogItem								\
{																\
public:															\
	c##name( int localid=id) : rvDialogItem ( localid ) { }		\
} name;

#define DIALOGITEM_END()										\
};


#endif // DIALOGHELPERS_H_

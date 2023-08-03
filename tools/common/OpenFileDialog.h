#ifndef OPENFILEDIALOG_H_
#define OPENFILEDIALOG_H_

#define OFD_MUSTEXIST	0x00000001

class rvOpenFileDialog
{
public:

	rvOpenFileDialog ( void );
	~rvOpenFileDialog ( void );

	bool			DoModal		( HWND parent );
	const char*		GetFilename	( void );

	void			SetFilter		( const char* filter );
	void			SetTitle		( const char* title );
	void			SetOKTitle		( const char* title );
	void			SetInitialPath	( const char* path );
	void			SetFlags		( int flags );

	const char*		GetInitialPath  ( void );

protected:

	void			UpdateFileList	( void );
	void			UpdateLookIn	( void );

	HWND			mWnd;
	HWND			mWndFileList;
	HWND			mWndLookin;

	HINSTANCE		mInstance;

	HIMAGELIST		mImageList;
	HBITMAP			mBackBitmap;

	static char		mLookin[ MAX_OSPATH ];
	anString			mFilename;
	anString			mTitle;
	anString			mOKTitle;
	anStringList		mFilters;

	int				mFlags;

private:

	void	HandleCommandOK			( void );
	void	HandleLookInChange		( void );
	void	HandleInitDialog		( void );

	static INT_PTR CALLBACK DlgProc ( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
};

ARC_INLINE const char* rvOpenFileDialog::GetFilename ( void ) {
	return mFilename.c_str();
}

ARC_INLINE void rvOpenFileDialog::SetTitle ( const char* title ) {
	mTitle = title;
}

ARC_INLINE void rvOpenFileDialog::SetOKTitle ( const char* title ) {
	mOKTitle = title;
}

ARC_INLINE void rvOpenFileDialog::SetInitialPath ( const char* path ) {
	if ( !anString::Cmpn( mLookin, path, strlen( path ) )  {
		return;
	}

	anString::Copynz( mLookin, path, sizeof( mLookin ) );
}

ARC_INLINE void rvOpenFileDialog::SetFlags ( int flags ) {
	mFlags = flags;
}

ARC_INLINE const char* rvOpenFileDialog::GetInitialPath ( void ) {
	return mLookin;
}

#endif // OPENFILEDIALOG_H_

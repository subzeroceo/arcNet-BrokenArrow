#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "GEApp.h"

bool GECheckInDlg_DoModal( HWND parent, const char* filename, arcNetString* comment );

/*
================
rvGEWorkspace::SaveFile

Writes the contents of the open gui file to disk
================
*/
bool rvGEWorkspace::SaveFile( const char* filename ) {
	arcNetFile*		file;
	idWindow*	window;

	SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_WAIT ) ) );

	mFilename = filename;

	// Since quake can only write to its path we will write a temp file then copy it over
	arcNetString tempfile;
	arcNetString ospath;

	tempfile = "guis/temp.guied";
	ospath = fileSystem->RelativePathToOSPath( tempfile, "fs_basepath" );

	// Open the output file for write
	if ( !(file = fileSystem->OpenFileWrite( tempfile ) ) )
	{
		int error = GetLastError( );
		SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_ARROW ) ) );
		return false;
	}

	window = mInterface->GetDesktop( );

	WriteWindow( file, 1, window );

	fileSystem->CloseFile( file );

	if ( !CopyFile( ospath, filename, FALSE ) )
	{
		DeleteFile( ospath );
		SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_ARROW ) ) );
		return false;
	}

	DeleteFile( ospath );

	mFilename = filename;
	mModified = false;
	mNew      = false;
	UpdateTitle( );

	SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_ARROW ) ) );

	return true;
}

/*
================
rvGEWorkspace::WriteTabs

Writes the given number of tabs to the given file
================
*/
void rvGEWorkspace::WriteTabs( arcNetFile* file, int depth  )
{
	int i;

	for ( i = 0; i < depth; i ++ )
	{
		file->Write( "\t", 1 );
	}
}

/*
================
rvGEWorkspace::WriteWindow

Writes the contents of the given window to the file
================
*/
bool rvGEWorkspace::WriteWindow( arcNetFile* file, int depth, idWindow* window )
{
	arcNetString				out;
	rvGEWindowWrapper*	wrapper;
	int					i;

	wrapper = rvGEWindowWrapper::GetWrapper( window );
	if ( !wrapper )
	{
		return true;
	}

	if ( wrapper->IsDeleted( ) )
	{
		return true;
	}

	// Window def header
	WriteTabs( file, depth - 1 );

	out = wrapper->WindowTypeToString( wrapper->GetWindowType( ) );
	out.Append( " " );
	file->Write( out, out.Length() );

	out = window->GetName( );
	file->Write( out, out.Length() );
	file->Write( "\r\n", 2 );

	WriteTabs( file, depth - 1 );

	out = "{\r\n";
	file->Write( out, out.Length() );
	file->ForceFlush( );

	for ( i = 0; i < wrapper->GetStateDict().GetNumKeyVals(); i ++ )
	{
		const idKeyValue* key = wrapper->GetStateDict().GetKeyVal( i );

		// Dont write name to the files
		if ( !key->GetKey().Icmp( "name" ) )
		{
			continue;
		}

		WriteTabs( file, depth );

		out = key->GetKey();
		out.Append( "\t" );
		file->Write( out, out.Length() );

		const char* p;
		for ( p = key->GetValue().c_str(); *p; p ++ )
		{
			switch( *p )
			{
				case '\n':
					file->Write( "\\n", 2 );
					break;

				default:
					file->Write( p, 1 );
					break;
			}
		}

		file->Write( "\r\n", 2 );
	}

	for ( i = 0; i < wrapper->GetVariableDict().GetNumKeyVals(); i ++ )
	{
		const idKeyValue* key = wrapper->GetVariableDict().GetKeyVal( i );

		WriteTabs( file, depth );

		out = key->GetKey();
		out.Append( "\t" );
		out.Append( key->GetValue() );
		out.Append( "\r\n" );

		file->Write( out, out.Length() );
	}

	if ( wrapper->GetScriptDict().GetNumKeyVals() )
	{
		file->Write( "\r\n", 2 );
	}

	for ( i = 0; i < wrapper->GetScriptDict().GetNumKeyVals(); i ++ )
	{
		const idKeyValue* key = wrapper->GetScriptDict().GetKeyVal( i );

		WriteTabs( file, depth );

		file->Write( key->GetKey(), key->GetKey().Length() );
		file->Write( " ", 1 );

		arcLexer src( key->GetValue(), key->GetValue().Length(), "", LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
		src.ParseBracedSectionExact( out, depth + 1 );

		file->Write( out, out.Length() );
		file->Write( "\r\n", 2 );
		file->Write( "\r\n", 2 );
	}

	for ( i = 0; i < wrapper->GetChildCount(); i ++ )
	{
		idWindow* child = wrapper->GetChild( i );

		WriteWindow( file, depth + 1, child );
	}

	// Window def footer
	WriteTabs( file, depth - 1 );

	out = "}\r\n";
	file->Write( out, out.Length() );
	file->ForceFlush( );

	return true;
}

/*
================
rvGEWorkspace::NewFile

Opens a new file for editing
================
*/
bool rvGEWorkspace::NewFile( void )
{
	arcNetString	empty;
	arcNetString	ospath;
	arcNetFile*	file;

	// Make a temporary file with nothing in it so we can just use
	// load to do all the work
	ospath = fileSystem->RelativePathToOSPath( "guis/Untitled.guiednew", "fs_basepath" );
	DeleteFile( ospath );

	file = fileSystem->OpenFileWrite( "guis/Untitled.guiednew" );
	if ( NULL == file )
	{
		return false;
	}

	empty = "windowDef Desktop { rect 0,0,640,480 }";
	file->Write( empty, empty.Length() );
	fileSystem->CloseFile( file );

	// Load the temporary file
	if ( !LoadFile( ospath, NULL ) )
	{
		// Ensure the temp file doesnt hang around
		DeleteFile( ospath );
		return false;
	}

	mNew = true;

	// Ensure the temp file doesnt hang around
	DeleteFile( ospath );

	// Go back to using a .gui extensions
	ospath.StripFileExtension( );
	ospath.Append( ".gui" );

	mFilename = ospath;

	return true;
}

/*
================
rvGEWorkspace::LoadFile

Loads the given gui file.
================
*/
bool rvGEWorkspace::LoadFile( const char* filename, arcNetString* error )
{
	delete mInterface;

	arcNetString tempfile;
	arcNetString ospath;
	bool  result;

	tempfile = "guis/temp.guied";
	ospath = fileSystem->RelativePathToOSPath( tempfile, "fs_basepath" );

	// Make sure the gui directory exists
	arcNetString createDir = ospath;
	createDir.StripFilename( );
	CreateDirectory( createDir, NULL );

	SetFileAttributes( ospath, FILE_ATTRIBUTE_NORMAL );
	DeleteFile( ospath );
	if ( !CopyFile( filename, ospath, FALSE ) )
	{
		if ( error )
		{
			*error = "File not found";
		}
		return false;
	}

	SetFileAttributes( ospath, FILE_ATTRIBUTE_NORMAL );

	mFilename = filename;
	UpdateTitle( );

	// Let the real window system parse it first
	mInterface = NULL;
	result     = true;
	try
	{
		mInterface = reinterpret_cast< idUserInterfaceLocal* >( uiManager->FindGui( tempfile, true, true ) );
		if ( !mInterface && error )
		{
			*error = "File not found";
		}
	}
	catch( arcExceptions& e )
	{
		result = false;
		if ( error )
		{
			*error = e.error;
		}
		return false;
	}

	if ( result )
	{
		rvGEWindowWrapper::GetWrapper( mInterface->GetDesktop( ) )->Expand( );
	}
	else
	{
		DeleteFile( ospath );
	}

	return result;
}

/*
================
rvGEWorkspace::CheckIn

Checks in the current workspace file into source control
================
*/
bool rvGEWorkspace::CheckIn( void )
{
	return false;

}

/*
================
rvGEWorkspace::CheckOut

Checks out the current workspace file from source control
================
*/
bool rvGEWorkspace::CheckOut( void )
{
		return false;
}

/*
================
rvGEWorkspace::UndoCheckout

Undoes the checkout of the current file
================
*/
bool rvGEWorkspace::UndoCheckout( void )
{
	return false;
}


#include "../idlib/Lib.h"
#pragma hdrstop

#include "ListGUILocal.h"
#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"

extern anCVar r_skipGuiShaders;		// 1 = don't render any gui elements on surfaces

anUserInterfaceManager	uiManager;
anUserInterfaceManager *	uiManager = &uiManager;

/*
===============================================================================

	anUserInterfaceManager

===============================================================================
*/

void anUserInterfaceManager::Init() {
	screenRect = idRectangle(0, 0, 640, 480);
	dc.Init();
}

void anUserInterfaceManager::Shutdown() {
	guis.DeleteContents( true );
	demoGuis.DeleteContents( true );
	dc.Shutdown();
}

void anUserInterfaceManager::Touch( const char *name ) {
	anUserInterface *gui = Alloc();
	gui->InitFromFile( name );
//	delete gui;
}

void anUserInterfaceManager::WritePrecacheCommands( anFile *f ) {
	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		char	str[1024];
		sprintf( str, "touchGui %s\n", guis[i]->Name() );
		common->Printf( "%s", str );
		f->Printf( "%s", str );
	}
}

void anUserInterfaceManager::SetSize( float width, float height ) {
	dc.SetSize( width, height );
}

void anUserInterfaceManager::BeginLevelLoad() {
	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( ( guis[i]->GetDesktop()->GetFlags() & WIN_MENUGUI ) == 0 ) {
			guis[i]->ClearRefs();
/*			delete guis[i];
			guis.RemoveIndex( i );
			i--; c--;*/
		}
	}
}

void anUserInterfaceManager::EndLevelLoad() {
	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( guis[i]->GetRefs() == 0 ) {
			//common->Printf( "purging %s.\n", guis[i]->GetSourceFile() );
			// use this to make sure no materials still reference this gui
			bool remove = true;
			for ( int j = 0; j < declManager->GetNumDecls( DECL_MATERIAL ); j++ ) {
				const anMaterial *material = static_cast<const anMaterial *>( declManager->DeclByIndex( DECL_MATERIAL, j, false ) );
				if ( material->GlobalGui() == guis[i] ) {
					remove = false;
					break;
				}
			}
			if ( remove ) {
				delete guis[i];
				guis.RemoveIndex( i );
				i--; c--;
			}
		}
	}
}

void anUserInterfaceManager::Reload( bool all ) {
	ARC_TIME_T ts;
	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( !all ) {
			fileSystem->ReadFile( guis[i]->GetSourceFile(), nullptr, &ts );
			if ( ts <= guis[i]->GetTimeStamp() ) {
				continue;
			}
		}

		guis[i]->InitFromFile( guis[i]->GetSourceFile() );
		common->Printf( "reloading %s.\n", guis[i]->GetSourceFile() );
	}
}

void anUserInterfaceManager::ListGuis() const {
	int c = guis.Num();
	common->Printf( "\n   size   refs   name\n" );
	size_t total = 0;
	int copies = 0;
	int unique = 0;
	for ( int i = 0; i < c; i++ ) {
		anUserInterface *gui = guis[i];
		size_t sz = gui->Size();
		bool isUnique = guis[i]->interactive;
		if ( isUnique ) {
			unique++;
		} else {
			copies++;
		}
		common->Printf( "%6.1fk %4i (%s) %s ( %i transitions )\n", sz / 1024.0f, guis[i]->GetRefs(), isUnique ? "unique" : "copy", guis[i]->GetSourceFile(), guis[i]->desktop->NumTransitions() );
		total += sz;
	}
	common->Printf( "===========\n  %i total Guis ( %i copies, %i unique ), %.2f total Mbytes", c, copies, unique, total / ( 1024.0f * 1024.0f ) );
}

bool anUserInterfaceManager::CheckGui( const char *path ) const {
	anFile *file = fileSystem->OpenFileRead( path );
	if ( file ) {
		fileSystem->CloseFile( file );
		return true;
	}
	return false;
}

anUserInterface *anUserInterfaceManager::Alloc( void ) const {
	return new anUserInterface();
}

void anUserInterfaceManager::DeAlloc( anUserInterface *gui ) {
	if ( gui ) {
		int c = guis.Num();
		for ( int i = 0; i < c; i++ ) {
			if ( guis[i] == gui ) {
				delete guis[i];
				guis.RemoveIndex( i );
				return;
			}
		}
	}
}

anUserInterface *anUserInterfaceManager::FindGui( const char *path, bool autoLoad, bool needUnique, bool forceNOTUnique ) {
	int c = guis.Num();

	for ( int i = 0; i < c; i++ ) {
		anUserInterface *gui = guis[i];
		if ( !anStr::Icmp( guis[i]->GetSourceFile(), path ) ) {
			if ( !forceNOTUnique && ( needUnique || guis[i]->IsInteractive() ) ) {
				break;
			}
			guis[i]->AddRef();
			return guis[i];
		}
	}

	if ( autoLoad ) {
		anUserInterface *gui = Alloc();
		if ( gui->InitFromFile( path ) ) {
			gui->SetUniqued( forceNOTUnique ? false : needUnique );
			return gui;
		} else {
			delete gui;
		}
	}
	return nullptr;
}

anUserInterface *anUserInterfaceManager::FindDemoGui( const char *path ) {
	int c = demoGuis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( !anStr::Icmp( demoGuis[i]->GetSourceFile(), path ) ) {
			return demoGuis[i];
		}
	}
	return nullptr;
}

anListGUI *anUserInterfaceManager::AllocListGUI( void ) const {
	return new anListGUILocal();
}

void anUserInterfaceManager::FreeListGUI( anListGUI *listgui ) {
	delete listgui;
}

/*
===============================================================================

	anUserInterface

===============================================================================
*/

anUserInterface::anUserInterface() {
	cursorX = cursorY = 0.0;
	desktop = nullptr;
	loading = false;
	active = false;
	interactive = false;
	uniqued = false;
	bindHandler = nullptr;
	//so the reg eval in gui parsing doesn't get bogus values
	time = 0;
	refs = 1;
}

anUserInterface::~anUserInterface() {
	delete desktop;
	desktop = nullptr;
}

const char *anUserInterface::Name() const {
	return source;
}

const char *anUserInterface::Comment() const {
	if ( desktop ) {
		return desktop->GetComment();
	}
	return "";
}

bool anUserInterface::IsInteractive() const {
	return interactive;
}

bool anUserInterface::InitFromFile( const char *path, bool rebuild, bool cache ) { 
	if ( !( path && *path ) ) { 
		// FIXME: Memory leak!!
		return false;
	}

	int sz = sizeof( idWindow );
	sz = sizeof( idSimpleWindow );
	loading = true;

	if ( rebuild ) {
		delete desktop;
		desktop = new idWindow( this );
	} else if ( desktop == nullptr ) {
		desktop = new idWindow( this );
	}

	source = path;
	state.Set( "text", "Test Text!" );

	anParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	//Load the timestamp so reload guis will work correctly
	fileSystem->ReadFile(path, nullptr, &timeStamp );
	src.LoadFile( path );

	if ( src.IsLoaded() ) {
		anToken token;
		while( src.ReadToken( &token ) ) {
			if ( anStr::Icmp( token, "windowDef" ) == 0 ) {
				desktop->SetDC( &uiManager.dc );
				if ( desktop->Parse( &src, rebuild ) ) {
					desktop->SetFlag( WIN_DESKTOP );
					desktop->FixupParms();
				}
				continue;
			}
		}
		state.Set( "name", path );
	} else {
		desktop->SetDC( &uiManager.dc );
		desktop->SetFlag( WIN_DESKTOP );
		desktop->name = "Desktop";
		desktop->text = va( "Invalid GUI: %s", path );
		desktop->rect = idRectangle( 0.0f, 0.0f, 640.0f, 480.0f );
		desktop->drawRect = desktop->rect;
		desktop->foreColor = anVec4( 1.0f, 1.0f, 1.0f, 1.0f );
		desktop->backColor = anVec4( 0.0f, 0.0f, 0.0f, 1.0f );
		desktop->SetupFromState();
		common->Warning( "Couldn't load gui: '%s'", path );
	}

	interactive = desktop->Interactive();

	if ( uiManager.guis.Find( this ) == nullptr ) {
		uiManager.guis.Append( this );
	}

	loading = false;
	return true; 
}

const char *anUserInterface::HandleEvent( const sysEvent_t *event, int _time, bool *updateVisuals ) {
	time = _time;

	if ( bindHandler && event->evType == SE_KEY && event->evValue2 == 1 ) {
		const char *ret = bindHandler->HandleEvent( event, updateVisuals );
		bindHandler = nullptr;
		return ret;
	}

	if ( event->evType == SE_MOUSE ) {
		cursorX += event->evValue;
		cursorY += event->evValue2;
		if ( cursorX < 0 ) {
			cursorX = 0;
		}
		if ( cursorY < 0 ) {
			cursorY = 0;
		}
	}

	if ( desktop ) {
		return desktop->HandleEvent( event, updateVisuals );
	} 

	return "";
}

void anUserInterface::HandleNamedEvent ( const char *eventName ) {
	desktop->RunNamedEvent( eventName );
}

void anUserInterface::Redraw( int _time ) {
	if ( r_skipGuiShaders.GetInteger() > 5 ) {
		return;
	}
	if ( !loading && desktop ) {
		time = _time;
		uiManager.dc.PushClipRect( uiManager.screenRect );
		desktop->Redraw( 0, 0 );
		uiManager.dc.PopClipRect();
	}
}

void anUserInterface::DrawCursor() {
	if ( !desktop || desktop->GetFlags() & WIN_MENUGUI ) {
		uiManager.dc.DrawCursor( &cursorX, &cursorY, 32.0f );
	} else {
		uiManager.dc.DrawCursor( &cursorX, &cursorY, 64.0f );
	}
}

const anDict &anUserInterface::State() const {
	return state;
}

void anUserInterface::DeleteStateVar( const char *varName ) {
	state.Delete( varName );
}

void anUserInterface::SetStateString( const char *varName, const char *value ) {
	state.Set( varName, value );
}

void anUserInterface::SetStateBool( const char *varName, const bool value ) {
	state.SetBool( varName, value );
}

void anUserInterface::SetStateInt( const char *varName, const int value ) {
	state.SetInt( varName, value );
}

void anUserInterface::SetStateFloat( const char *varName, const float value ) {
	state.SetFloat( varName, value );
}

const char *anUserInterface::GetStateString( const char *varName, const char *defaultString ) const {
	return state.GetString( varName, defaultString );
}

bool anUserInterface::GetStateBool( const char *varName, const char *defaultString ) const {
	return state.GetBool( varName, defaultString ); 
}

int anUserInterface::GetStateInt( const char *varName, const char *defaultString ) const {
	return state.GetInt( varName, defaultString );
}

float anUserInterface::GetStateFloat( const char *varName, const char *defaultString ) const {
	return state.GetFloat( varName, defaultString );
}

void anUserInterface::StateChanged( int _time, bool redraw ) {
	time = _time;
	if ( desktop ) {
		desktop->StateChanged( redraw );
	}
	if ( state.GetBool( "noninteractive" ) ) {
		interactive = false;
	} else {
		if (desktop) {
			interactive = desktop->Interactive();
		} else {
			interactive = false;
		}
	}
}

const char *anUserInterface::Activate( bool activate, int _time ) {
	time = _time;
	active = activate;
	if ( desktop ) {
		activateStr = "";
		desktop->Activate( activate, activateStr );
		return activateStr;
	}
	return "";
}

void anUserInterface::Trigger( int _time ) {
	time = _time;
	if ( desktop ) {
		desktop->Trigger();
	}
}

void anUserInterface::ReadFromDemoFile( class anSavedGamesFile *f ) {
	anStr work;
	f->ReadDict( state );
	source = state.GetString( "name" );

	if ( desktop == nullptr ) {
		f->Log( "creating new gui\n" );
		desktop = new idWindow( this );
	   	desktop->SetFlag( WIN_DESKTOP );
	   	desktop->SetDC( &uiManager.dc );
		desktop->ReadFromDemoFile(f);
	} else {
		f->Log( "re-using gui\n" );
		desktop->ReadFromDemoFile( f, false );
	}

	f->ReadFloat( cursorX );
	f->ReadFloat( cursorY );

	bool add = true;
	int c = uiManager.demoGuis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( uiManager.demoGuis[i] == this ) {
			add = false;
			break;
		}
	}

	if (add) {
		uiManager.demoGuis.Append( this );
	}
}

void anUserInterface::WriteToDemoFile( class anSavedGamesFile *f ) {
	anStr work;
	f->WriteDict( state );
	if ( desktop ) {
		desktop->WriteToDemoFile( f );
	}

	f->WriteFloat( cursorX );
	f->WriteFloat( cursorY );
}

bool anUserInterface::WriteToSaveGame( anFile *savefile ) const {
	int len;
	const anKeyValue *kv;
	const char *string;

	int num = state.GetNumKeyVals();
	savefile->Write( &num, sizeof( num ) );

	for ( int i = 0; i < num; i++ ) {
		kv = state.GetKeyVal( i );
		len = kv->GetKey().Length();
		string = kv->GetKey().c_str();
		savefile->Write( &len, sizeof( len ) );
		savefile->Write( string, len );

		len = kv->GetValue().Length();
		string = kv->GetValue().c_str();
		savefile->Write( &len, sizeof( len ) );
		savefile->Write( string, len );
	}

	savefile->Write( &active, sizeof( active ) );
	savefile->Write( &interactive, sizeof( interactive ) );
	savefile->Write( &uniqued, sizeof( uniqued ) );
	savefile->Write( &time, sizeof( time ) );
	len = activateStr.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( activateStr.c_str(), len );
	len = pendingCmd.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( pendingCmd.c_str(), len );
	len = returnCmd.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( returnCmd.c_str(), len );

	savefile->Write( &cursorX, sizeof( cursorX ) );
	savefile->Write( &cursorY, sizeof( cursorY ) );

	desktop->WriteToSaveGame( savefile );

	return true;
}

bool anUserInterface::ReadFromSaveGame( anFile *savefile ) {
	int num;
	int i, len;
	anStr key;
	anStr value;

	savefile->Read( &num, sizeof( num ) );

	state.Clear();
	for ( i = 0; i < num; i++ ) {
		savefile->Read( &len, sizeof( len ) );
		key.Fill( ' ', len );
		savefile->Read( &key[0], len );
		savefile->Read( &len, sizeof( len ) );
		value.Fill( ' ', len );
		savefile->Read( &value[0], len );
		state.Set( key, value );
	}

	savefile->Read( &active, sizeof( active ) );
	savefile->Read( &interactive, sizeof( interactive ) );
	savefile->Read( &uniqued, sizeof( uniqued ) );
	savefile->Read( &time, sizeof( time ) );
	savefile->Read( &len, sizeof( len ) );
	activateStr.Fill( ' ', len );
	savefile->Read( &activateStr[0], len );
	savefile->Read( &len, sizeof( len ) );
	pendingCmd.Fill( ' ', len );
	savefile->Read( &pendingCmd[0], len );
	savefile->Read( &len, sizeof( len ) );
	returnCmd.Fill( ' ', len );
	savefile->Read( &returnCmd[0], len );
	savefile->Read( &cursorX, sizeof( cursorX ) );
	savefile->Read( &cursorY, sizeof( cursorY ) );
	desktop->ReadFromSaveGame( savefile );

	return true;
}

size_t anUserInterface::Size() {
	size_t sz = sizeof(* this) + state.Size() + source.Allocated();
	if ( desktop ) {
		sz += desktop->Size();
	}
	return sz;
}

void anUserInterface::RecurseSetKeyBindingNames( idWindow *window ) {
	idWinVar *v = window->GetWinVarByName( "bind" );
	if ( v ) {
		SetStateString( v->GetName(), idKeyInput::KeysFromBinding( v->GetName() ) );
	}
	int i = 0;
	while ( i < window->GetChildCount() ) {
		idWindow *next = window->GetChild( i );
		if ( next ) {
			RecurseSetKeyBindingNames( next );
		}
		i++;
	}
}

/*
==============
anUserInterface::SetKeyBindingNames
==============
*/
void anUserInterface::SetKeyBindingNames( void ) {
	if ( !desktop ) {
		return;
	}
	// walk the windows
	RecurseSetKeyBindingNames( desktop );
}

/*
==============
anUserInterface::SetCursor
==============
*/
void anUserInterface::SetCursor( float x, float y ) {
	cursorX = x;
	cursorY = y;
}


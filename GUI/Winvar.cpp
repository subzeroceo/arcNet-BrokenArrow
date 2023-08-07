#include "../idlib/Lib.h"
#pragma hdrstop

#include "Window.h"
#include "Winvar.h"
#include "UserInterfaceLocal.h"

idWinVar::idWinVar() { 
	guiDict = nullptr; 
	name = nullptr; 
	eval = true;
}

idWinVar::~idWinVar() { 
	delete name;
	name = nullptr;
}

void idWinVar::SetGuiInfo( anDict *gd, const char *_name ) { 
	guiDict = gd; 
	SetName(_name); 
}

void idWinVar::Init( const char *_name, idWindow *win ) {
	anStr key = _name;
	guiDict = nullptr;
	int len = key.Length();
	if (len > 5 && key[0] == 'g' && key[1] == 'u' && key[2] == 'i' && key[3] == ':') {
		key = key.Right(len - VAR_GUIPREFIX_LEN);
		SetGuiInfo( win->GetGui()->GetStateDict(), key );
		win->AddUpdateVar(this);
	} else {
		Set(_name);
	}
}

void idMultiWinVar::Set( const char *val ) {
	for ( int i = 0; i < Num(); i++ ) {
		(*this)[i]->Set( val );
	}
}

void idMultiWinVar::Update( void ) {
	for ( int i = 0; i < Num(); i++ ) {
		(*this)[i]->Update();
	}
}

void idMultiWinVar::SetGuiInfo( anDict *dict ) {
	for ( int i = 0; i < Num(); i++ ) {
		(*this)[i]->SetGuiInfo( dict, (*this)[i]->c_str() );
	}
}

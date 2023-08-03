include "..//idlib/Lib.h"
#pragma hdrstop

#include "GEApp.h"
#include "GEKeyValueModifier.h"

rvGEKeyValueModifier::rvGEKeyValueModifier ( const char* name, idWindow* window, const char* key, const char* value ) : rvGEModifier ( name, window ), mKey ( key ), mValue ( value ) {
	mUndoValue = mWrapper->GetStateDict().GetString ( mKey );
}

bool rvGEKeyValueModifier::Apply ( void ) {
	if ( mValue.Length( ) ) {
		mWrapper->SetStateKey( mKey, mValue );
	} else {
		mWrapper->DeleteStateKey( mKey );
	}

	return true;
}

bool rvGEKeyValueModifier::Undo( void ) {
	mWrapper->SetStateKey ( mKey, mValue );
	return true;
}

bool rvGEKeyValueModifier::Merge( rvGEModifier* mergebase ) {
	rvGEKeyValueModifier* merge = (rvGEKeyValueModifier*) mergebase;
	mValue = merge->mValue;
	return true;
}

#include "..//idlib/Lib.h"
#pragma hdrstop

#include "GEApp.h"

rvGEModifier::rvGEModifier( const char* name, idWindow* window ) {
	mWindow  = window;
	mName    = name;

	if ( mWindow ) {
		mWrapper = rvGEWindowWrapper::GetWrapper( window );
	}
}

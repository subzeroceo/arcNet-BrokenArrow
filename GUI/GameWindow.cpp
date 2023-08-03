#include "../idlib/Lib.h"
#pragma hdrstop

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "GameWindow.h"

/*
================
idGameWindowProxy::idGameWindowProxy
================
*/
idGameWindowProxy::idGameWindowProxy( idDeviceContext *d, anUserInterface *g ) : idWindow( d, g ) { }

/*
================
idGameWindowProxy::Draw
================
*/
void idGameWindowProxy::Draw( int time, float x, float y ) {
	common->Printf( "TODO: idGameWindowProxy::Draw\n" );
}


#include "..//idlib/Lib.h"
#include "../../renderer/tr_local.h"
#include "../posix/posix_public.h"
#include "local.h"

void Sys_InitInput( void ) { }
void Sys_ShutdownInput( void ) { }

void Sys_GrabMouseCursor( bool ) { }

int Sys_PollMouseInputEvents( void ) { return 0; }
void Sys_EndMouseInputEvents( void ) { }
int Sys_ReturnMouseInputEvent( const int n, int &action, int &value ) { return 0; }

int Sys_PollKeyboardInputEvents( void ) { return 0; }
void Sys_EndKeyboardInputEvents( void ) { }
int Sys_ReturnKeyboardInputEvent( const int n, int &action, bool &state ) { return 0; }

unsigned char Sys_MapCharForKey( int key ) { return (unsigned char)key; }

// returns in megabytes
int Sys_GetVideoRam( void ) { return 64; }
void GLimp_EnableLogging( bool enable ) { }

bool GLimp_Init( glimpParms_t a ) { return true; }
void GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] ) { }
void GLimp_Shutdown( void ) { }

void GLimp_SwapBuffers( void ) { }

void GLimp_DeactivateContext( void ) { }
void GLimp_ActivateContext( void ) { }

bool GLimp_SetScreenParms( glimpParms_t parms ) { return true; }


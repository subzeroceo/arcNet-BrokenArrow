#include "/idlib/Lib.h"
#pragma hdrstop

void	RadiantInit( void ) { common->Printf( "The level editor Radiant only runs on Win32\n" ); }
void	RadiantShutdown( void ) {}
void	RadiantRun( void ) {}
void	RadiantPrint( const char *text ) {}
void	RadiantSync( const char *mapName, const anVec3 &viewOrg, const anAngles &viewAngles ) {}

void	LightEditorInit( const anDict *spawnArgs ) { common->Printf( "The Light Editor only runs on Win32\n" ); }
void	LightEditorShutdown( void ) {}
void	LightEditorRun( void ) {}

void	SoundEditorInit( const anDict *spawnArgs ) { common->Printf( "The Sound Editor only runs on Win32\n" ); }
void	SoundEditorShutdown( void ) {}
void	SoundEditorRun( void ) {}

void	AFEditorInit( const anDict *spawnArgs ) { common->Printf( "The Articulated Figure Editor only runs on Win32\n" ); }
void	AFEditorShutdown( void ) {}
void	AFEditorRun( void ) {}

void	ParticleEditorInit( const anDict *spawnArgs ) { common->Printf( "The Particle Editor only runs on Win32\n" ); }
void	ParticleEditorShutdown( void ) {}
void	ParticleEditorRun( void ) {}

void	ScriptEditorInit( const anDict *spawnArgs ) { common->Printf( "The Script Editor only runs on Win32\n" ); }
void	ScriptEditorShutdown( void ) {}
void	ScriptEditorRun( void ) {}

void	DeclBrowserInit( const anDict *spawnArgs ) { common->Printf( "The Declaration Browser only runs on Win32\n" ); }
void	DeclBrowserShutdown( void ) {}
void	DeclBrowserRun( void ) {}
void	DeclBrowserReloadDeclarations( void ) {}

void	GUIEditorInit( void ) { common->Printf( "The GUI Editor only runs on Win32\n" ); }
void	GUIEditorShutdown( void ) {}
void	GUIEditorRun( void ) {}
bool	GUIEditorHandleMessage( void *msg ) { return false; }

void	PDAEditorInit( const anDict *spawnArgs ) { common->Printf( "The PDA editor only runs on Win32\n" ); }

void	MaterialEditorInit() { common->Printf( "The Material editor only runs on Win32\n" ); }
void	MaterialEditorPrintConsole( const char *text ) {}

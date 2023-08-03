#ifndef __EDIT_PUBLIC_H__
#define __EDIT_PUBLIC_H__

/*
===============================================================================

	Editors.

===============================================================================
*/

class	idProgram;
class	idInterpreter;

// Radiant Level Editor
void	RadiantInit( void );
void	RadiantShutdown( void );
void	RadiantRun( void );
void	RadiantPrint( const char *text );
void	RadiantSync( const char *mapName, const anVec3 &viewOrg, const anAngles &viewAngles );

// in-game Light Editor
void	LightEditorInit( const anDict *spawnArgs );
void	LightEditorShutdown( void );
void	LightEditorRun( void );

// in-game Sound Editor
void	SoundEditorInit( const anDict *spawnArgs );
void	SoundEditorShutdown( void );
void	SoundEditorRun( void );

// in-game Articulated Figure Editor
void	AFEditorInit( const anDict *spawnArgs );
void	AFEditorShutdown( void );
void	AFEditorRun( void );

// in-game Particle Editor
void	ParticleEditorInit( const anDict *spawnArgs );
void	ParticleEditorShutdown( void );
void	ParticleEditorRun( void );

// in-game PDA Editor
void	PDAEditorInit( const anDict *spawnArgs );
void	PDAEditorShutdown( void );
void	PDAEditorRun( void );

// in-game Script Editor
void	ScriptEditorInit( const anDict *spawnArgs );
void	ScriptEditorShutdown( void );
void	ScriptEditorRun( void );

// in-game Declaration Browser
void	DeclBrowserInit( const anDict *spawnArgs );
void	DeclBrowserShutdown( void );
void	DeclBrowserRun( void );
void	DeclBrowserReloadDeclarations( void );

// GUI Editor
void	GUIEditorInit( void );
void	GUIEditorShutdown( void );
void	GUIEditorRun( void );
bool	GUIEditorHandleMessage( void *msg );

// Material Editor
void	MaterialEditorInit( void );
void	MaterialEditorRun( void );
void	MaterialEditorShutdown( void );
void	MaterialEditorPrintConsole( const char *msg );

#endif
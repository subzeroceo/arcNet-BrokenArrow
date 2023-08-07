/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __COMMON_H__
#define __COMMON_H__

/*
==============================================================

  Common

==============================================================
*/

#include <basetsd.h>					// needed for UINT_PTR
#include <stddef.h>						// needed for offsetof
#include <memory.h>						// needed for memcmp
typedef enum {
	EDITOR_NONE					= 0,
	EDITOR_RADIANT				= BIT(1),
	EDITOR_GUI					= BIT(2),
	EDITOR_DEBUGGER				= BIT(3),
	EDITOR_SCRIPT				= BIT(4),
	EDITOR_LIGHT				= BIT(5),
	EDITOR_SOUND				= BIT(6),
	EDITOR_DECL					= BIT(7),
	EDITOR_AF					= BIT(8),
	EDITOR_PARTICLE				= BIT(9),
	EDITOR_PDA					= BIT(10),
	EDITOR_SEAS					= BIT(11),
	EDITOR_MATERIAL				= BIT(12),
	EDITOR_RENDERBUMP			= BIT(13),
	// Specifies that a decl validation run is happening
	EDITOR_DECL_VALIDATING		= BIT(18),
	EDITOR_ALL					= -1
} toolFlag_t;

#define inline				__inline
#define STRTABLE_ID				"#str_"
#define STRTABLE_ID_LENGTH		5
//#define LSTRTABLE_ID			L"#str_"

extern idCVar		com_version;
extern idCVar		com_skipRenderer;
extern idCVar		com_asyncInput;
extern idCVar		com_asyncSound;
extern idCVar		com_machineSpec;
extern idCVar		com_purgeAll;
extern idCVar		com_developer;
extern idCVar		com_allowConsole;
extern idCVar		com_speeds;
extern idCVar		com_showFPS;
extern idCVar		com_showMemoryUsage;
extern idCVar		com_showAsyncStats;
extern idCVar		com_showSoundDecoders;
extern idCVar		com_makingBuild;
extern idCVar		com_updateLoadSize;
extern idCVar		com_videoRam;

extern int			time_gameFrame;			// game logic time
extern int			time_gameDraw;			// game present time
extern int			time_frontend;			// renderer frontend time
extern int			time_backend;			// renderer backend time

extern int			com_frameTime;			// time for the current frame in milliseconds
extern volatile int	com_ticNumber;			// 60 hz tics, incremented by async function
extern int			com_editors;			// current active editor(s)
extern bool			com_editorActive;		// true if an editor has focus

extern anCVarSystem com_engineHz;
extern float com_engineHz_latched;
extern int64 com_engineHz_numerator;
extern int64 com_engineHz_denominator;

// Returns the msec the frame starts on
inline int FRAME_TO_MSEC( int64 frame ) {
	return ( int )( ( frame * com_engineHz_numerator ) / com_engineHz_denominator );
}
// Rounds DOWN to the nearest frame
inline int MSEC_TO_FRAME_FLOOR( int msec ) {
	return ( int )( ( ( (int64)msec * com_engineHz_denominator ) + ( com_engineHz_denominator - 1 ) ) / com_engineHz_numerator );
}
// Rounds UP to the nearest frame
inline int MSEC_TO_FRAME_CEIL( int msec ) {
	return ( int )( ( ( (int64)msec * com_engineHz_denominator ) + ( com_engineHz_numerator - 1 ) ) / com_engineHz_numerator );
}
// Aligns msec so it starts on a frame bondary
inline int MSEC_ALIGN_TO_FRAME( int msec ) {
	return FRAME_TO_MSEC( MSEC_TO_FRAME_CEIL( msec ) );
}
#ifdef _WIN32
const char			DMAP_MSGID[] = "DMAPOutput";
const char			DMAP_DONE[] = "DMAPDone";
extern HWND			com_hwndMsg;
extern bool			com_outputMsg;
#endif

struct MemInfo_t {
	idStr			filebase;

	int				total;
	int				assetTotals;

	// memory manager totals
	int				memoryManagerTotal;

	// subsystem totals
	int				gameSubsystemTotal;
	int				renderSubsystemTotal;

	// asset totals
	int				imageAssetsTotal;
	int				modelAssetsTotal;
	int				soundAssetsTotal;
};

class arCNet {
public:
	virtual						~arCNet( void ) {}

								// Initialize everything.
								// if the OS allows, pass argc/argv directly (without executable name)
								// otherwise pass the command line in a single string (without executable name)
	void						Init( int argc, const char **argv, const char *cmdline ) = 0;

								// Shuts down everything.
	void						Shutdown( void ) = 0;

								// Shuts down everything.
	void						Quit( void ) = 0;

								// Returns true if common initialization is complete.
	bool						IsInitialized( void ) const = 0;

								// Called repeatedly as the foreground thread for rendering and game logic.
	void						Frame( void ) = 0;

								// Called repeatedly by blocking function calls with GUI interactivity.
	void						GUIFrame( bool execCmd ) = 0;

	int							GetGameFrame( void ) = 0;

	// Redraws the screen, handling games, guis, console, etc
	// in a modal manner outside the normal frame loop
	void						UpdateScreen( bool captureToImage ) = 0;
	void						UpdateLevelLoadPacifier() = 0;

								// Called 60 times a second from a background thread for sound mixing,
								// and input generation. Not called until arCNet::Init() has completed.
	void						Async( void ) = 0;

								// Checks for and removes command line "+set var arg" constructs.
								// If match is NULL, all set commands will be executed, otherwise
								// only a set with the exact name.  Only used during startup.
								// set once to clear the cvar from +set for early init code
	void						ConStartupVariable( const char *match, bool once ) = 0;

								// Initializes a tool with the given dictionary.
	void						InitTool( const toolFlag_t tool, const anDict *dict ) = 0;

								// Activates or deactivates a tool.
	void						ActivateTool( bool active ) = 0;

								// Writes the user's configuration to a file
	void						WriteConfigToFile( const char *filename ) = 0;

								// Writes cvars with the given flags to a file.
	void						WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) = 0;

								// Begins redirection of console output to the given buffer.
	void						BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ) ) = 0;

								// Stops redirection of console output.
	void						EndRedirect( void ) = 0;

								// Update the screen with every message printed.
	void						SetRefreshOnPrint( bool set ) = 0;

								// Prints message to the console, which may cause a screen update if com_refreshOnPrint is set.
	void						Printf( const char *fmt, ... )an_attribute( ( format( printf, 2, 3 ) ) ) = 0;

								// Same as Printf, with a more usable API - Printf pipes to this.
	void						VPrintf( const char *fmt, va_list arg ) = 0;

								// Prints message that only shows up if the "developer" cvar is set,
								// and NEVER forces a screen update, which could cause reentrancy problems.
	void						DPrintf( const char *fmt, ... ) an_attribute( ( format( printf, 2, 3 ) ) ) = 0;

								// Prints WARNING %s message and adds the warning message to a queue for printing later on.
	void						Warning( const char *fmt, ... ) an_attribute( ( format( printf, 2, 3 ) ) ) = 0;

								// Prints WARNING %s message in yellow that only shows up if the "developer" cvar is set.
	void						DWarning( const char *fmt, ...) an_attribute( ( format( printf, 2, 3 ) ) ) = 0;

								// Prints all queued warnings.
	void						PrintWarnings( void ) = 0;

								// Removes all queued warnings.
	void						ClearWarnings( const char *reason ) = 0;

								// Issues a C++ throw. Normal errors just abort to the game loop,
								// which is appropriate for media or dynamic logic errors.
	void						Error( const char *fmt, ... ) an_attribute( ( format( printf, 2, 3 ) ) ) = 0;

								// Fatal errors quit all the way to a system dialog box, which is appropriate for
								// static internal errors or cases where the system may be corrupted.
	void						FatalError( const char *fmt, ... ) an_attribute( ( format( printf, 2, 3 ) ) ) = 0;

								// Returns a pointer to the dictionary with language specific strings.
	const anLangDict *			GetLanguageDict( void ) = 0;

								// Returns key bound to the command
	const char *				KeysFromBinding( const char *bind ) = 0;

								// Returns the binding bound to the key
	const char *				BindingFromKey( const char *key ) = 0; 

								// Directly sample a button.
	int							ButtonState( int key ) = 0;

								// Directly sample a keystate.
	int							KeyState( int key ) = 0;

								// Returns true if the player has ever enabled the console
	bool						GetConsoleUsed() = 0;

								// Processes the given event.
	virtual	bool				ProcessEvent( const sysEvent_t *event ) = 0;

};

extern arCNet *		common;

#endif // !__COMMON_H__

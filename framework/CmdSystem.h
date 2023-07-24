#ifndef __CMDSYSTEM_H__
#define __CMDSYSTEM_H__

/*
===============================================================================

	Console command execution and command text buffering.

	Any number of commands can be added in a frame from several different
	sources. Most commands come from either key bindings or console line input,
	but entire text files can be execed.

	Command execution takes a null terminated string, breaks it into tokens,
	then searches for a command or variable that matches the first token.

===============================================================================
*/

// command flags
typedef enum {
	CMD_FL_ALL				= -1,
	CMD_FL_CHEAT			= BIT(0 ),	// command is considered a cheat
	CMD_FL_SYSTEM			= BIT(1 ),	// system command
	CMD_FL_RENDERER			= BIT(2),	// renderer command
	CMD_FL_SOUND			= BIT(3),	// sound command
	CMD_FL_GAME				= BIT(4),	// game command
	CMD_FL_TOOL				= BIT(5)	// tool command
} cmdFlags_t;

// parameters for command buffer stuffing
typedef enum {
	CMD_EXEC_NOW,						// don't return until completed
	CMD_EXEC_INSERT,					// insert at current position, but don't run yet
	CMD_EXEC_APPEND						// add to end of the command buffer (normal case)
} cmdExecution_t;

// command function
typedef void (*cmdFunction_t)( const arcCommandArgs &args );

// argument completion function
typedef void (*argCompletion_t)( const arcCommandArgs &args, void(*callback)( const char *s ) );

/*
================================================
idCommandLink is a convenient way to get a function registered as a
ConsoleCommand without having to add an explicit call to arcCmdSystem->AddCommand() in a startup
function somewhere. Simply declare a static variable with the parameters and it will get
executed before main(). For example:

static idCommandLink sys_dumpMemory( "sys_dumpMemory", Sys_DumpMemory_f, "Walks the heap and reports stats" );
================================================
*/

class idCommandLink {
public:
	idCommandLink( const char *cmdName, cmdFunction_t function,
		const char *description, argCompletion_t argCompletion = NULL );
	idCommandLink *	next;
	const char *	cmdName_;
	cmdFunction_t	function_;
	const char *	description_;
	argCompletion_t argCompletion_;
};

// The command system will create commands for all the static definitions
// when it initializes.
idCommandLink *CommandLinks( idCommandLink *cl = NULL );

/*
================================================
The CONSOLE_COMMAND macro is an even easier way to create a console command by
automatically generating the idCommandLink variable, and it also allows all the
command code to be stripped from a build with a single define.  For example:

CONSOLE_COMMAND( Sys_DumpMemory, "Walks the heap and reports stats" ) {
	// do stuff
}

NOTE: All CONSOLE_COMMANDs will be stripped with the shipping build unless it's
created using the CONSOLE_COMMAND_SHIP macro.
================================================
*/

#if defined ( ID_RETAIL ) && !defined( ID_RETAIL_INTERNAL )
#define CONSOLE_COMMAND_SHIP			CONSOLE_COMMAND_COMPILE
#define CONSOLE_COMMAND					CONSOLE_COMMAND_NO_COMPILE
// We need to disable this warning to get commands that were made friends
// of classes to compile as inline.
// warning C4211: nonstandard extension used : redefined extern to static
#pragma warning( disable : 4211 )
// warning C4505: 'xxx' : unreferenced local function has been removed
#pragma warning( disable : 4505 )
#else
#define CONSOLE_COMMAND_SHIP			CONSOLE_COMMAND_COMPILE
#define CONSOLE_COMMAND					CONSOLE_COMMAND_COMPILE
#endif

// Turn console commands into static inline code, which will cause them to be
// removed from the build.
#define CONSOLE_COMMAND_NO_COMPILE( name, comment, completion ) \
	static inline void name ## _f( const arcCommandArgs &args )

// lint incorrectly gives this for all console commands: Issue 1568: (Warning -- Variable 'TestAtomicString_v' accesses variable 'atomicStringManager' before the latter is initialized through calls: 'TestAtomicString_f() => idAtomicString::FreeDynamic()')
// I can't figure out how to disable this just around CONSOLE_COMMAND, so it must stay disabled everywhere,
// which is a shame.
//lint -e1568
#define CONSOLE_COMMAND_COMPILE( name, comment, completion ) \
	void name ## _f( const arcCommandArgs &args ); \
	idCommandLink name ## _v( #name, name ## _f, comment, completion  ); \
	void name ## _f( const arcCommandArgs &args )

class arcCmdSystem {
public:
	virtual				~arcCmdSystem() {}

	virtual void		Init() = 0;
	virtual void		Shutdown() = 0;

						// Registers a command and the function to call for it.
	virtual void		AddCommand( const char *cmdName, cmdFunction_t function, int flags, const char *description, argCompletion_t argCompletion = NULL ) = 0;
						// Removes a command.
	virtual void		RemoveCommand( const char *cmdName ) = 0;
						// Remove all commands with one of the flags set.
	virtual void		RemoveFlaggedCommands( int flags ) = 0;

						// Command and argument completion using callback for each valid string.
	virtual void		CommandCompletion( void(*callback)( const char *s ) ) = 0;
	virtual void		ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) = 0;

	virtual void		ExecuteCommandText( const char * text ) = 0;
	virtual void		AppendCommandText( const char * text ) = 0;

						// Adds command text to the command buffer, does not add a final \n
	virtual void		BufferCommandText( cmdExecution_t exec, const char *text ) = 0;
						// Pulls off \n \r or; terminated lines of text from the command buffer and
						// executes the commands. Stops when the buffer is empty.
						// Normally called once per frame, but may be explicitly invoked.
	virtual void		ExecuteCommandBuffer() = 0;

						// Base for path/file auto-completion.
	virtual void		ArgCompletion_FolderExtension( const arcCommandArgs &args, void(*callback)( const char *s ), const char *folder, bool stripFolder, ... ) = 0;
						// Base for decl name auto-completion.
	virtual void		ArgCompletion_DeclName( const arcCommandArgs &args, void(*callback)( const char *s ), int type ) = 0;

						// Adds to the command buffer in tokenized form ( CMD_EXEC_NOW or CMD_EXEC_APPEND only )
	virtual void		BufferCommandArgs( cmdExecution_t exec, const arcCommandArgs &args ) = 0;

						// Setup a reloadEngine to happen on next command run, and give a command to execute after reload
	virtual void		SetupReloadEngine( const arcCommandArgs &args ) = 0;
	virtual bool		PostReloadEngine() = 0;

						// Default argument completion functions.
	static void			ArgCompletion_Boolean( const arcCommandArgs &args, void(*callback)( const char *s ) );
	template<int min,int max>
	static void			ArgCompletion_Integer( const arcCommandArgs &args, void(*callback)( const char *s ) );
	template<const char **strings>
	static void			ArgCompletion_String( const arcCommandArgs &args, void(*callback)( const char *s ) );
	template<int type>
	static void			ArgCompletion_Decl( const arcCommandArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_FileName( const arcCommandArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_MapName( const arcCommandArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_ModelName( const arcCommandArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_SoundName( const arcCommandArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_ImageName( const arcCommandArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_VideoName( const arcCommandArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_ConfigName( const arcCommandArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_SaveGame( const arcCommandArgs &args, void(*callback)( const char *s ) );
	static void			ArgCompletion_DemoName( const arcCommandArgs &args, void(*callback)( const char *s ) );
};

extern arcCmdSystem *	cmdSystem;


ARC_INLINE void arcCmdSystem::ArgCompletion_Boolean( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	callback( va( "%s 0", args.Argv( 0 ) ) );
	callback( va( "%s 1", args.Argv( 0 ) ) );
}

template<int min,int max> ARC_INLINE void arcCmdSystem::ArgCompletion_Integer( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	for ( int i = min; i <= max; i++ ) {
		callback( va( "%s %d", args.Argv( 0 ), i ) );
	}
}

template<const char **strings> ARC_INLINE void arcCmdSystem::ArgCompletion_String( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	for ( int i = 0; strings[i]; i++ ) {
		callback( va( "%s %s", args.Argv( 0 ), strings[i] ) );
	}
}

template<int type> ARC_INLINE void arcCmdSystem::ArgCompletion_Decl( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_DeclName( args, callback, type );
}

ARC_INLINE void arcCmdSystem::ArgCompletion_FileName( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", true, "", NULL );
}

ARC_INLINE void arcCmdSystem::ArgCompletion_MapName( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "maps/", true, ".map", NULL );
}

ARC_INLINE void arcCmdSystem::ArgCompletion_ModelName( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "models/", false, ".lwo", ".ase", ".md5mesh", ".ma", NULL );
}

ARC_INLINE void arcCmdSystem::ArgCompletion_SoundName( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "sound/", false, ".wav", NULL );
}

ARC_INLINE void arcCmdSystem::ArgCompletion_ImageName( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", false, ".tga", ".dds", ".jpg", ".pcx", NULL );
}

ARC_INLINE void arcCmdSystem::ArgCompletion_VideoName( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", false, ".bik", NULL );
}

ARC_INLINE void arcCmdSystem::ArgCompletion_ConfigName( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "/", true, ".cfg", NULL );
}

ARC_INLINE void arcCmdSystem::ArgCompletion_SaveGame( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "SaveGames/", true, ".save", NULL );
}

ARC_INLINE void arcCmdSystem::ArgCompletion_DemoName( const arcCommandArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "demos/", true, ".demo", NULL );
}

#endif /* !__CMDSYSTEM_H__ */

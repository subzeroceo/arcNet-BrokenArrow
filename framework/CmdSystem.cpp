#include "../idlib/Lib.h"
#pragma hdrstop

/*
===============================================================================

							LIST OF COMMANDS and FLAGS
 this is the Local Command system the 2nd most dilligent and effective part of any
				 quake engine.  Aside from the main console itself.

===============================================================================
*/

typedef struct commandDef_s {
	struct commandDef_s *	next;
	char *					name;
	cmdFunction_t			function;
	argCompletion_t			argCompletion;
	int						flags;
	char *					description;
} commandDef_t;

/*
================================================
ARCCmdSysLocal
================================================
*/
class ARCCmdSysLocal : public arcCmdSystem {
public:
	virtual void			Init();
	virtual void			Shutdown();

	virtual void			AddCommand( const char *cmdName, cmdFunction_t function, int flags, const char *description, argCompletion_t argCompletion = nullptr );
	virtual void			RemoveCommand( const char *cmdName );
	virtual void			RemoveFlaggedCommands( int flags );

	virtual void			CommandCompletion( void(*callback)( const char *s ) );
	virtual void			ArgCompletion( const char *cmdString, void(*callback)( const char *s ) );
	virtual void			ExecuteCommandText( const char *text );
	virtual void			AppendCommandText( const char *text );

	virtual void			BufferCommandText( cmdExecution_t exec, const char *text );
	virtual void			ExecuteCommandBuffer();

	virtual void			ArgCompletion_FolderExtension( const anCommandArgs &args, void(*callback)( const char *s ), const char *folder, bool stripFolder, ... );
	virtual void			ArgCompletion_DeclName( const anCommandArgs &args, void(*callback)( const char *s ), int type );

	virtual void			BufferCommandArgs( cmdExecution_t exec, const anCommandArgs &args );

	virtual void			SetupReloadEngine( const anCommandArgs &args );
	virtual bool			PostReloadEngine();

	void					SetWait( int numFrames ) { wait = numFrames; }
	commandDef_t *			GetCommands() const { return commands; }

private:
	static const int		MAX_COMMANDBUFFER = 0x10000;

	commandDef_t *			commands;

	int						wait;
	int						textLength;
	byte					textBuf[MAX_COMMANDBUFFER];

	anStr					completionString;
	aRcStrList				completionParms;

	// piggybacks on the text buffer, avoids tokenize again and screwing it up
	anList<anCommandArgs>		tokenizedCmds;

	// a command stored to be executed after a reloadEngine and all associated commands have been processed
	anCommandArgs				postReload;

private:
	void					ExecuteTokenizedString( const anCommandArgs &args );
	void					InsertCommandText( const char *text );

	static void				ListByFlags( const anCommandArgs &args, cmdFlags_t flags );
	static void				List_f( const anCommandArgs &args );
	static void				SystemList_f( const anCommandArgs &args );
	static void				RendererList_f( const anCommandArgs &args );
	static void				SoundList_f( const anCommandArgs &args );
	static void				GameList_f( const anCommandArgs &args );
	static void				ToolList_f( const anCommandArgs &args );
	static void				Exec_f( const anCommandArgs &args );
	static void				Vstr_f( const anCommandArgs &args );
	static void				Echo_f( const anCommandArgs &args );
	static void				Parse_f( const anCommandArgs &args );
	static void				Wait_f( const anCommandArgs &args );
	static void				PrintMemInfo_f( const anCommandArgs &args );
};
ARCCmdSysLocal			cmdSystemLocal;
arcCmdSystem *				cmdSystem = &cmdSystemLocal;

/*
================================================
anSortCommandDef
================================================
*/
class anSortCommandDef : public anSortQuick< commandDef_t, anSortCommandDef > {
public:
	int Compare( const commandDef_t & a, const commandDef_t & b ) const { return anStr::Icmp( a.name, b.name ); }
};

/*
============
ARCCmdSysLocal::ListByFlags
============
*/
void ARCCmdSysLocal::ListByFlags( const anCommandArgs &args, cmdFlags_t flags ) {
	int i;
	anStr match;
	const commandDef_t *cmd;
	anList<const commandDef_t *> cmdList;

	if ( args.Argc() > 1 ) {
		match = args.Args( 1, -1 );
		match.Replace( " ", "" );
	} else {
		match = "";
	}

	for ( cmd = cmdSystemLocal.GetCommands(); cmd; cmd = cmd->next ) {
		if ( !( cmd->flags & flags ) ) {
			continue;
		}
		if ( match.Length() && anStr( cmd->name ).Filter( match, false ) == 0 ) {
			continue;
		}

		cmdList.Append( cmd );
	}

	//cmdList.SortWithTemplate( anSortCommandDef() );

	for ( i = 0; i < cmdList.Num(); i++ ) {
		cmd = cmdList[i];

		common->Printf( "  %-21s %s\n", cmd->name, cmd->description );
	}

	common->Printf( "%i commands\n", cmdList.Num() );
}

/*
============
ARCCmdSysLocal::List_f
============
*/
void ARCCmdSysLocal::List_f( const anCommandArgs &args ) {
	ARCCmdSysLocal::ListByFlags( args, CMD_FL_ALL );
}

/*
============
ARCCmdSysLocal::SystemList_f
============
*/
void ARCCmdSysLocal::SystemList_f( const anCommandArgs &args ) {
	ARCCmdSysLocal::ListByFlags( args, CMD_FL_SYSTEM );
}

/*
============
ARCCmdSysLocal::RendererList_f
============
*/
void ARCCmdSysLocal::RendererList_f( const anCommandArgs &args ) {
	ARCCmdSysLocal::ListByFlags( args, CMD_FL_RENDERER );
}

/*
============
ARCCmdSysLocal::SoundList_f
============
*/
void ARCCmdSysLocal::SoundList_f( const anCommandArgs &args ) {
	ARCCmdSysLocal::ListByFlags( args, CMD_FL_SOUND );
}

/*
============
ARCCmdSysLocal::GameList_f
============
*/
void ARCCmdSysLocal::GameList_f( const anCommandArgs &args ) {
	ARCCmdSysLocal::ListByFlags( args, CMD_FL_GAME );
}

/*
============
ARCCmdSysLocal::ToolList_f
============
*/
void ARCCmdSysLocal::ToolList_f( const anCommandArgs &args ) {
	ARCCmdSysLocal::ListByFlags( args, CMD_FL_TOOL );
}

/*
===============
ARCCmdSysLocal::Exec_f
===============
*/
void ARCCmdSysLocal::Exec_f( const anCommandArgs &args ) {
	char *	f;
	int		len;
	anStr	filename;

	if ( args.Argc () != 2 ) {
		common->Printf( "exec <filename> : execute a script file\n" );
		return;
	}

	filename = args.Argv(1 );
	filename.DefaultFileExtension( ".cfg" );
	len = fileSystem->ReadFile( filename, reinterpret_cast<void **>(&f), nullptr );
	if ( !f ) {
		common->Printf( "couldn't exec %s\n", args.Argv(1 ) );
		return;
	}
	common->Printf( "execing %s\n", args.Argv(1 ) );

	cmdSystemLocal.BufferCommandText( CMD_EXEC_INSERT, f );

	fileSystem->FreeFile( f );
}

/*
===============
ARCCmdSysLocal::Vstr_f

Inserts the current value of a cvar as command text
===============
*/
void ARCCmdSysLocal::Vstr_f( const anCommandArgs &args ) {
	const char *v;

	if ( args.Argc () != 2 ) {
		common->Printf( "vstr <variablename> : execute a variable command\n" );
		return;
	}

	v = cvarSystem->GetCVarString( args.Argv( 1 ) );

	cmdSystemLocal.BufferCommandText( CMD_EXEC_APPEND, va( "%s\n", v ) );
}

/*
===============
ARCCmdSysLocal::Echo_f

Just prints the rest of the line to the console
===============
*/
void ARCCmdSysLocal::Echo_f( const anCommandArgs &args ) {
	for ( int i = 1; i < args.Argc(); i++ ) {
		common->Printf( "%s ", args.Argv( i ) );
	}
	common->Printf( "\n" );
}

/*
============
ARCCmdSysLocal::Wait_f

Causes execution of the remainder of the command buffer to be delayed until next frame.
============
*/
void ARCCmdSysLocal::Wait_f( const anCommandArgs &args ) {
	if ( args.Argc() == 2 ) {
		cmdSystemLocal.SetWait( atoi( args.Argv( 1 ) ) );
	} else {
		cmdSystemLocal.SetWait( 1 );
	}
}

/*
============
ARCCmdSysLocal::Parse_f

This just prints out how the rest of the line was parsed, as a debugging tool.
============
*/
void ARCCmdSysLocal::Parse_f( const anCommandArgs &args ) {
	for ( int i = 0; i < args.Argc(); i++ ) {
		common->Printf( "%i: %s\n", i, args.Argv( i ) );
	}
}

/*
============
ARCCmdSysLocal::Init
============
*/
void ARCCmdSysLocal::Init() {
	AddCommand( "list", List_f, CMD_FL_SYSTEM, "lists commands" );
	AddCommand( "sysCmds", SystemList_f, CMD_FL_SYSTEM, "lists system commands" );
	AddCommand( "listRenderCmds", RendererList_f, CMD_FL_SYSTEM, "lists renderer commands" );
	AddCommand( "listSndCmds", SoundList_f, CMD_FL_SYSTEM, "lists sound commands" );
	AddCommand( "listEngineCmds", GameList_f, CMD_FL_SYSTEM, "lists main front-end engine commands" );
	AddCommand( "listAllTools", ToolList_f, CMD_FL_SYSTEM, "lists tool commands" );
	AddCommand( "exec", Exec_f, CMD_FL_SYSTEM, "executes a config file", ArgCompletion_ConfigName );
	AddCommand( "vstr", Vstr_f, CMD_FL_SYSTEM, "inserts the current value of a cvar as command text" );
	AddCommand( "echo", Echo_f, CMD_FL_SYSTEM, "prints text" );
	AddCommand( "parse", Parse_f, CMD_FL_SYSTEM, "prints tokenized string" );
	AddCommand( "wait", Wait_f, CMD_FL_SYSTEM, "delays remaining buffered commands one or more frames" );

	// link in all the commands declared with static idCommandLink variables or CONSOLE_COMMAND macros
	for ( idCommandLink * link = CommandLinks(); link != nullptr; link = link->next ) {
		AddCommand( link->cmdName_, link->function_, CMD_FL_SYSTEM, link->description_, link->argCompletion_ );
	}

	completionString = "*";

	textLength = 0;
}

/*
============
ARCCmdSysLocal::Shutdown
============
*/
void ARCCmdSysLocal::Shutdown() {
	commandDef_t *cmd;

	for ( cmd = commands; cmd; cmd = commands ) {
		commands = commands->next;
		Mem_Free( cmd->name );
		Mem_Free( cmd->description );
		delete cmd;
	}

	completionString.Clear();
	completionParms.Clear();
	tokenizedCmds.Clear();
	postReload.Clear();
}

/*
============
ARCCmdSysLocal::AddCommand
============
*/
void ARCCmdSysLocal::AddCommand( const char *cmdName, cmdFunction_t function, int flags, const char *description, argCompletion_t argCompletion ) {
	commandDef_t *cmd;

	// fail if the command already exists
	for ( cmd = commands; cmd; cmd = cmd->next ) {
		if ( anStr::Cmp( cmdName, cmd->name ) == 0 ) {
			if ( function != cmd->function ) {
				common->Printf( "ARCCmdSysLocal::AddCommand: %s already defined\n", cmdName );
			}
			return;
		}
	}

	cmd = new (TAG_SYSTEM) commandDef_t;
	cmd->name = Mem_CopyString( cmdName );
	cmd->function = function;
	cmd->argCompletion = argCompletion;
	cmd->flags = flags;
	cmd->description = Mem_CopyString( description );
	cmd->next = commands;
	commands = cmd;
}

/*
============
ARCCmdSysLocal::RemoveCommand
============
*/
void ARCCmdSysLocal::RemoveCommand( const char *cmdName ) {
	commandDef_t *cmd, **last;

	for ( last = &commands, cmd = *last; cmd; cmd = *last ) {
		if ( anStr::Cmp( cmdName, cmd->name ) == 0 ) {
			*last = cmd->next;
			Mem_Free( cmd->name );
			Mem_Free( cmd->description );
			delete cmd;
			return;
		}
		last = &cmd->next;
	}
}

/*
============
ARCCmdSysLocal::RemoveFlaggedCommands
============
*/
void ARCCmdSysLocal::RemoveFlaggedCommands( int flags ) {
	commandDef_t *cmd, **last;

	for ( last = &commands, cmd = *last; cmd; cmd = *last ) {
		if ( cmd->flags & flags ) {
			*last = cmd->next;
			Mem_Free( cmd->name );
			Mem_Free( cmd->description );
			delete cmd;
			continue;
		}
		last = &cmd->next;
	}
}

/*
============
ARCCmdSysLocal::CommandCompletion
============
*/
void ARCCmdSysLocal::CommandCompletion( void(*callback)( const char *s ) ) {
	commandDef_t *cmd;

	for ( cmd = commands; cmd; cmd = cmd->next ) {
		callback( cmd->name );
	}
}

/*
============
ARCCmdSysLocal::ArgCompletion
============
*/
void ARCCmdSysLocal::ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) {
	commandDef_t *cmd;
	anCommandArgs args;

	args.TokenizeString( cmdString, false );

	for ( cmd = commands; cmd; cmd = cmd->next ) {
		if ( !cmd->argCompletion ) {
			continue;
		}
		if ( anStr::Icmp( args.Argv( 0 ), cmd->name ) == 0 ) {
			cmd->argCompletion( args, callback );
			break;
		}
	}
}

/*
============
ARCCmdSysLocal::ExecuteTokenizedString
============
*/
void ARCCmdSysLocal::ExecuteTokenizedString( const anCommandArgs &args ) {
	commandDef_t *cmd, **prev;

	// execute the command line
	if ( !args.Argc() ) {
		return;		// no tokens
	}

	// check registered command functions
	for ( prev = &commands; *prev; prev = &cmd->next ) {
		cmd = *prev;
		if ( anStr::Icmp( args.Argv( 0 ), cmd->name ) == 0 ) {
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			*prev = cmd->next;
			cmd->next = commands;
			commands = cmd;

			if ( ( cmd->flags & (CMD_FL_CHEAT|CMD_FL_TOOL) ) && common->IsMultiplayer() && !net_allowCheats.GetBool() ) {
				common->Printf( "Command '%s' not valid in multiplayer mode.\n", cmd->name );
				return;
			}
			// perform the action
			if ( !cmd->function ) {
				break;
			} else {
				cmd->function( args );
			}
			return;
		}
	}

	// check cvars
	if ( cvarSystem->Command( args ) ) {
		return;
	}

	common->Printf( "Unknown command '%s'\n", args.Argv( 0 ) );
}

/*
============
ARCCmdSysLocal::ExecuteCommandText

Tokenizes, then executes.
============
*/
void ARCCmdSysLocal::ExecuteCommandText( const char *text ) {
	ExecuteTokenizedString( anCommandArgs( text, false ) );
}

/*
============
ARCCmdSysLocal::InsertCommandText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void ARCCmdSysLocal::InsertCommandText( const char *text ) {
	int len = strlen( text ) + 1;
	if ( len + textLength > ( int )sizeof( textBuf ) ) {
		common->Printf( "ARCCmdSysLocal::InsertText: buffer overflow\n" );
		return;
	}

	// move the existing command text
	for ( int i = textLength - 1; i >= 0; i-- ) {
		textBuf[ i + len ] = textBuf[i];
	}

	// copy the new text in
	memcpy( textBuf, text, len - 1 );

	// add a \n
	textBuf[ len - 1 ] = '\n';

	textLength += len;
}

/*
============
ARCCmdSysLocal::AppendCommandText

Adds command text at the end of the buffer, does NOT add a final \n
============
*/
void ARCCmdSysLocal::AppendCommandText( const char *text ) {
	int l = strlen( text );

	if ( textLength + l >= ( int )sizeof( textBuf ) ) {
		common->Printf( "ARCCmdSysLocal::AppendText: buffer overflow\n" );
		return;
	}
	memcpy( textBuf + textLength, text, l );
	textLength += l;
}

/*
============
ARCCmdSysLocal::BufferCommandText
============
*/
void ARCCmdSysLocal::BufferCommandText( cmdExecution_t exec, const char *text ) {
	switch ( exec ) {
		case CMD_EXEC_NOW: {
			ExecuteCommandText( text );
			break;
		}
		case CMD_EXEC_INSERT: {
			InsertCommandText( text );
			break;
		}
		case CMD_EXEC_APPEND: {
			AppendCommandText( text );
			break;
		}
		default: {
			common->FatalError( "ARCCmdSysLocal::BufferCommandText: bad exec type" );
		}
	}
}

/*
============
ARCCmdSysLocal::BufferCommandArgs
============
*/
void ARCCmdSysLocal::BufferCommandArgs( cmdExecution_t exec, const anCommandArgs &args ) {
	switch ( exec ) {
		case CMD_EXEC_NOW: {
			ExecuteTokenizedString( args );
			break;
		}
		case CMD_EXEC_APPEND: {
			AppendCommandText( "_execTokenized\n" );
			tokenizedCmds.Append( args );
			break;
		}
		default: {
			common->FatalError( "ARCCmdSysLocal::BufferCommandArgs: bad exec type" );
		}
	}
}

/*
============
ARCCmdSysLocal::ExecuteCommandBuffer
============
*/
void ARCCmdSysLocal::ExecuteCommandBuffer() {
	anCommandArgs	args;

	while ( textLength ) {
		if ( wait )	{
			// skip out while text still remains in buffer, leaving it for next frame
			wait--;
			break;
		}

		// find a \n or; line break
		char *text = (char *)textBuf;

		int quotes = 0;
		for ( int i = 0; i < textLength; i++ ) {
			if ( text[i] == '"' ) {
				quotes++;
			}
			if ( !( quotes & 1 ) &&  text[i] == ';' ) {
				break;	// don't break if inside a quoted string
			}
			if ( text[i] == '\n' || text[i] == '\r' ) {
				break;
			}
		}

		char *text[i] = 0;

		if ( !anStr::Cmp( text, "_execTokenized" ) ) {
			args = tokenizedCmds[0];
			tokenizedCmds.RemoveIndex( 0 );
		} else {
			args.TokenizeString( text, false );
		}

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec) can insert data at the
		// beginning of the text buffer
		if ( int i == textLength ) {
			textLength = 0;
		} else {
			i++;
			textLength -= i;
			memmove( text, text+i, textLength );
		}

		// execute the command line that we have already tokenized
		ExecuteTokenizedString( args );
	}
}

/*
============
ARCCmdSysLocal::ArgCompletion_FolderExtension
============
*/
void ARCCmdSysLocal::ArgCompletion_FolderExtension( const anCommandArgs &args, void(*callback)( const char *s ), const char *folder, bool stripFolder, ... ) {
	//char path[MAX_STRING_CHARS];
	//anStr filename;

	//filename.Format( "%s/%s", folder, args.Argv( 0 ) );
	//callback( filename.c_str() );

	//stripFolder = true;
	const char *extension;
	va_list argPtr;

	aRcStrstring = args.Argv( 0 );
	string += " ";
	aRcStrstring += args.Argv( 1 );

	if ( string.Icmp( completionString ) != 0 ) {
		anStr parm, path;
		anFileList *names;

		completionString = string;
		completionParms.Clear();

		parm = args.Argv( 1 );
		parm.ExtractFilePath( path );
		if ( stripFolder || path.Length() == 0 ) {
			path = folder + path;
		}
		path.StripTrailing( '/' );

		// list folders
		names = fileSystem->ListFiles( path, "/", true, true );
		for ( int i = 0; i < names->GetNumFiles(); i++ ) {
			anStr name = names->GetFile( i );
			if ( stripFolder ) {
				name.Strip( folder );
			} else {
				name.Strip( "/" );
			}
			name = args.Argv( 0 ) + ( " " + name ) + "/";
			completionParms.Append( name );
		}
		fileSystem->FreeFileList( names );

		// list files
		va_start( argPtr, stripFolder );
		for ( extension = va_arg( argPtr, const char *); extension; extension = va_arg( argPtr, const char *) ) {
			names = fileSystem->ListFiles( path, extension, true, true );
			for ( i = 0; i < names->GetNumFiles(); i++ ) {
				anStr name = names->GetFile( i );
				if ( stripFolder ) {
					name.Strip( folder );
				} else {
					name.Strip( "/" );
				}
				name = args.Argv( 0 ) + ( " " + name );
				completionParms.Append( name );
			}
			fileSystem->FreeFileList( names );
		}
		va_end( argPtr );
	}
	for ( i = 0; i < completionParms.Num(); i++ ) {
		callback( completionParms[i] );
	}
}

/*
============
ARCCmdSysLocal::ArgCompletion_DeclName
============
*/
void ARCCmdSysLocal::ArgCompletion_DeclName( const anCommandArgs &args, void(*callback)( const char *s ), int type ) {
	if ( declManager == nullptr ) {
		return;
	}

	int num = declManager->GetNumDecls( ( declType_t )type );
	for ( int i = 0; i < num; i++ ) {
		callback( anStr( args.Argv( 0 ) ) + " " + declManager->DeclByIndex( ( declType_t )type, i , false )->GetName() );
	}
}

/*
============
ARCCmdSysLocal::SetupReloadEngine
============
*/
void ARCCmdSysLocal::SetupReloadEngine( const anCommandArgs &args ) {
	BufferCommandText( CMD_EXEC_APPEND, "reloadEngine\n" );
	postReload = args;
}

/*
============
ARCCmdSysLocal::PostReloadEngine
============
*/
bool ARCCmdSysLocal::PostReloadEngine() {
	if ( !postReload.Argc() ) {
		return false;
	}
	BufferCommandArgs( CMD_EXEC_APPEND, postReload );
	postReload.Clear();
	return true;
}

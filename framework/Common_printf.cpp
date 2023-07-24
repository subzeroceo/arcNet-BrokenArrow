#include "/idlib/precompiled.h"
#pragma hdrstop

#include "Common_local.h"

arcCVarSystem com_logFile( "logFile", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "1 = buffer log, 2 = flush after each print", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
arcCVarSystem com_logFileName( "logFileName", "qconsole.log", CVAR_SYSTEM | CVAR_NOCHEAT, "name of log file, if empty, qconsole.log will be used" );
arcCVarSystem com_timestampPrints( "com_timestampPrints", "0", CVAR_SYSTEM, "print time with each console print, 1 = msec, 2 = sec", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );

#ifndef ID_RETAIL
arcCVarSystem com_printFilter( "com_printFilter", "", CVAR_SYSTEM, "only print lines that contain this, add multiple filters with a; delimeter" );
#endif

/*
==================
arcCommonLocal::BeginRedirect
==================
*/
void arcCommonLocal::BeginRedirect( char *buffer, int buffersize, void (*flush)( const char *) ) {
	if ( !buffer || !buffersize || !flush ) {
		return;
	}
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

/*
==================
arcCommonLocal::EndRedirect
==================
*/
void arcCommonLocal::EndRedirect() {
	if ( rd_flush && rd_buffer[ 0 ] ) {
		rd_flush( rd_buffer );
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

/*
==================
arcCommonLocal::CloseLogFile
==================
*/
void arcCommonLocal::CloseLogFile() {
	if ( logFile ) {
		com_logFile.SetBool( false ); // make sure no further VPrintf attempts to open the log file again
		fileSystem->CloseFile( logFile );
		logFile = NULL;
	}
}

/*
==================
arcCommonLocal::SetRefreshOnPrint
==================
*/
void arcCommonLocal::SetRefreshOnPrint( bool set ) {
	com_refreshOnPrint = set;
}

/*
==================
arcCommonLocal::VPrintf

A raw string should NEVER be passed as fmt, because of "%f" type crashes.
==================
*/
void arcCommonLocal::VPrintf( const char *fmt, va_list args ) {
	static bool	logFileFailed = false;

	// if the cvar system is not initialized
	if ( !cvarSystem->IsInitialized() ) {
		return;
	}
	// optionally put a timestamp at the beginning of each print,
	// so we can see how long different init sections are taking
	int timeLength = 0;
	char msg[MAX_PRINT_MSG_SIZE];
	msg[ 0 ] = '\0';
	if ( com_timestampPrints.GetInteger() ) {
		int	t = Sys_Milliseconds();
		if ( com_timestampPrints.GetInteger() == 1 ) {
			sprintf( msg, "[%5.2f]", t * 0.001f );
		} else {
			sprintf( msg, "[%i]", t );
		}
	}
	timeLength = strlen( msg );
	// don't overflow
	if ( arcNetString::vsnPrintf( msg+timeLength, MAX_PRINT_MSG_SIZE-timeLength-1, fmt, args ) < 0 ) {
		msg[sizeof( msg )-2] = '\n'; msg[sizeof( msg )-1] = '\0'; // avoid output garbling
		Sys_Printf( "VPrintf: truncated to %d characters\n", strlen( msg )-1 );
	}

	if ( rd_buffer ) {
		if ( ( int )( strlen( msg ) + strlen( rd_buffer ) ) > ( rd_buffersize - 1 ) ) {
			rd_flush( rd_buffer );
			*rd_buffer = 0;
		}
		strcat( rd_buffer, msg );
		return;
	}
#ifndef ID_RETAIL
	if ( com_printFilter.GetString() != NULL && com_printFilter.GetString()[ 0 ] != '\0' ) {
		aRcStaticString< 4096 > filterBuf = com_printFilter.GetString();
		aRcStaticString< 4096 > msgBuf = msg;
		filterBuf.ToLower();
		msgBuf.ToLower();
		char *sp = strtok( &filterBuf[ 0 ], ";" );
		bool p = false;
		for (; sp != NULL; ) {
			if ( strstr( msgBuf, sp ) != NULL ) {
				p = true;
				break;
			}
			sp = strtok( NULL, ";" );
		}
		if ( !p ) {
			return;
		}
	}
#endif
	if ( !arcLibrary::IsMainThread() ) {
		OutputDebugString( msg );
		return;
	}

	// echo to console buffer
	console->Print( msg );

	// remove any color codes
	arcNetString::RemoveColors( msg );

	// echo to dedicated console and early console
	Sys_Printf( "%s", msg );

	// print to script debugger server
	// DebuggerServerPrint( msg );

#if 0	// !@#
#if defined(_DEBUG) && defined(WIN32)
	if ( strlen( msg ) < 512 ) {
		TRACE( msg );
	}
#endif
#endif

	// logFile
	if ( com_logFile.GetInteger() && !logFileFailed && fileSystem->IsInitialized() ) {
		static bool recursing;
		if ( !logFile && !recursing ) {
			const char *fileName = com_logFileName.GetString()[0] ? com_logFileName.GetString() : "qconsole.log";

			// fileSystem->OpenFileWrite can cause recursive prints into here
			recursing = true;

			logFile = fileSystem->OpenFileWrite( fileName );
			if ( !logFile ) {
				logFileFailed = true;
				FatalError( "failed to open log file '%s'\n", fileName );
			}

			recursing = false;

			if ( com_logFile.GetInteger() > 1 ) {
				// force it to not buffer so we get valid
				// data even if we are crashing
				logFile->ForceFlush();
			}

			time_t aclock;
			time( &aclock );
			struct tm * newtime = localtime( &aclock );
			Printf( "log file '%s' opened on %s\n", fileName, asctime( newtime ) );
		}
		if ( logFile ) {
			logFile->Write( msg, strlen( msg ) );
			logFile->Flush();	// ForceFlush doesn't help a whole lot
		}
	}

	// don't trigger any updates if we are in the process of doing a fatal error
	if ( com_errorEntered != ERP_FATAL ) {
		// update the console if we are in a long-running command, like dmap
		if ( com_refreshOnPrint ) {
			const bool captureToImage = false;
			UpdateScreen( captureToImage );
		}
	}
}

/*
==================
arcCommonLocal::Printf

Both client and server can use this, and it will output to the appropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
==================
*/
void arcCommonLocal::Printf( const char *fmt, ... ) {
	va_list argptr;
	va_start( argptr, fmt );
	VPrintf( fmt, argptr );
	va_end( argptr );
}

/*
==================
arcCommonLocal::DPrintf

prints message that only shows up if the "developer" cvar is set
==================
*/
void arcCommonLocal::DPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG_SIZE];

	if ( !cvarSystem->IsInitialized() || !com_developer.GetBool() ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start( argptr, fmt );
	arcNetString::vsnPrintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );
	msg[sizeof( msg )-1] = '\0';

	// never refresh the screen, which could cause reentrency problems
	bool temp = com_refreshOnPrint;
	com_refreshOnPrint = false;

	Printf( S_COLOR_RED"%s", msg );

	com_refreshOnPrint = temp;
}

/*
==================
arcCommonLocal::DWarning

prints warning message in yellow that only shows up if the "developer" cvar is set
==================
*/
void arcCommonLocal::DWarning( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG_SIZE];

	if ( !com_developer.GetBool() ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start( argptr, fmt );
	arcNetString::vsnPrintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );
	msg[sizeof( msg )-1] = '\0';

	Printf( S_COLOR_YELLOW"WARNING: %s\n", msg );
}

/*
==================
arcCommonLocal::Warning

prints WARNING %s and adds the warning message to a queue to be printed later on
==================
*/
void arcCommonLocal::Warning( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAX_PRINT_MSG_SIZE];

	if ( !arcLibrary::IsMainThread() ) {
		return;	// not thread safe!
	}

	va_start( argptr, fmt );
	arcNetString::vsnPrintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );
	msg[sizeof( msg )-1] = 0;

	Printf( S_COLOR_YELLOW "WARNING: " S_COLOR_RED "%s\n", msg );

	if ( warningList.Num() < MAX_WARNING_LIST ) {
		warningList.AddUnique( msg );
	}
}

/*
==================
arcCommonLocal::PrintWarnings
==================
*/
void arcCommonLocal::PrintWarnings() {
	int i;

	if ( !warningList.Num() ) {
		return;
	}

	Printf( "------------- Warnings ---------------\n" );
	Printf( "during %s...\n", warningCaption.c_str() );

	for ( i = 0; i < warningList.Num(); i++ ) {
		Printf( S_COLOR_YELLOW "WARNING: " S_COLOR_RED "%s\n", warningList[i].c_str() );
	}
	if ( warningList.Num() ) {
		if ( warningList.Num() >= MAX_WARNING_LIST ) {
			Printf( "more than %d warnings\n", MAX_WARNING_LIST );
		} else {
			Printf( "%d warnings\n", warningList.Num() );
		}
	}
}

/*
==================
arcCommonLocal::ClearWarnings
==================
*/
void arcCommonLocal::ClearWarnings( const char *reason ) {
	warningCaption = reason;
	warningList.Clear();
}

/*
==================
arcCommonLocal::DumpWarnings
==================
*/
void arcCommonLocal::DumpWarnings() {
	int			i;
	arcNetFile		*warningFile;

	if ( !warningList.Num() ) {
		return;
	}

	warningFile = fileSystem->OpenFileWrite( "warnings.md", "fs_savepath" );
	if ( warningFile ) {

		warningFile->Printf( "------------- Warnings ---------------\n\n" );
		warningFile->Printf( "during %s...\n", warningCaption.c_str() );
		for ( i = 0; i < warningList.Num(); i++ ) {
			warningList[i].RemoveColors();
			warningFile->Printf( "WARNING: %s\n", warningList[i].c_str() );
		}
		if ( warningList.Num() >= MAX_WARNING_LIST ) {
			warningFile->Printf( "\nmore than %d warnings!\n", MAX_WARNING_LIST );
		} else {
			warningFile->Printf( "\n%d warnings.\n", warningList.Num() );
		}

		warningFile->Printf( "\n\n-------------- Errors ---------------\n\n" );
		for ( i = 0; i < errorList.Num(); i++ ) {
			errorList[i].RemoveColors();
			warningFile->Printf( "ERROR: %s", errorList[i].c_str() );
		}

		warningFile->ForceFlush();

		fileSystem->CloseFile( warningFile );

#ifndef ID_DEBUG
		arcNetString	osPath;
		osPath = fileSystem->RelativePathToOSPath( "warnings.md", "fs_savepath" );
		WinExec( va( "Notepad.exe %s", osPath.c_str() ), SW_SHOW );
#endif
	}
}

/*
==================
arcCommonLocal::Error
==================
*/
void arcCommonLocal::Error( const char *fmt, ... ) {
	va_list		argptr;
	static int	lastErrorTime;
	static int	errorCount;
	int			currentTime;

	errorParm_t code = ERP_DROP;

	// always turn this off after an error
	com_refreshOnPrint = false;

	if ( com_productionMode.GetInteger() == 3 ) {
		Sys_Quit();
	}

	// when we are running automated scripts, make sure we
	// know if anything failed
	if ( cvarSystem->GetCVarInteger( "fs_copyfiles" ) ) {
		code = ERP_FATAL;
	}

	// if we don't have GL running, make it a fatal error
	if ( !renderSystem->IsOpenGLRunning() ) {
		code = ERP_FATAL;
	}

	// if we got a recursive error, make it fatal
	if ( com_errorEntered ) {
		// if we are recursively erroring while exiting
		// from a fatal error, just kill the entire
		// process immediately, which will prevent a
		// full screen rendering window covering the
		// error dialog
		if ( com_errorEntered == ERP_FATAL ) {
			Sys_Quit();
		}
		code = ERP_FATAL;
	}

	// if we are getting a solid stream of ERP_DROP, do an ERP_FATAL
	currentTime = Sys_Milliseconds();
	if ( currentTime - lastErrorTime < 100 ) {
		if ( ++errorCount > 3 ) {
			code = ERP_FATAL;
		}
	} else {
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	com_errorEntered = code;

	va_start (argptr,fmt);
	arcNetString::vsnPrintf( errorMessage, sizeof(errorMessage), fmt, argptr );
	va_end (argptr);
	errorMessage[sizeof(errorMessage)-1] = '\0';


	// copy the error message to the clip board
	Sys_SetClipboardData( errorMessage );

	// add the message to the error list
	errorList.AddUnique( errorMessage );

	Stop();

	if ( code == ERP_DISCONNECT ) {
		com_errorEntered = ERP_NONE;
		throw arcExceptions( errorMessage );
	} else if ( code == ERP_DROP ) {
		Printf( "********************\nERROR: %s\n********************\n", errorMessage );
		com_errorEntered = ERP_NONE;
		throw arcExceptions( errorMessage );
	} else {
		Printf( "********************\nERROR: %s\n********************\n", errorMessage );
	}

	if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
	}

	Sys_Error( "%s", errorMessage );

}

/*
==================
arcCommonLocal::FatalError

Dump out of the game to a system dialog
==================
*/
void arcCommonLocal::FatalError( const char *fmt, ... ) {
	va_list		argptr;

	if ( com_productionMode.GetInteger() == 3 ) {
		Sys_Quit();
	}
	// if we got a recursive error, make it fatal
	if ( com_errorEntered ) {
		// if we are recursively erroring while exiting
		// from a fatal error, just kill the entire
		// process immediately, which will prevent a
		// full screen rendering window covering the
		// error dialog

		Sys_Printf( "FATAL: recursed fatal error:\n%s\n", errorMessage );

		va_start( argptr, fmt );
		arcNetString::vsnPrintf( errorMessage, sizeof(errorMessage), fmt, argptr );
		va_end( argptr );
		errorMessage[sizeof(errorMessage)-1] = '\0';

		Sys_Printf( "%s\n", errorMessage );

		// write the console to a log file?
		Sys_Quit();
	}
	com_errorEntered = ERP_FATAL;

	va_start( argptr, fmt );
	arcNetString::vsnPrintf( errorMessage, sizeof(errorMessage), fmt, argptr );
	va_end( argptr );
	errorMessage[sizeof(errorMessage)-1] = '\0';

	if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
	}

	Sys_SetFatalError( errorMessage );

	Sys_Error( "%s", errorMessage );

}

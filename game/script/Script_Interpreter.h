#ifndef __SCRIPT_INTERPRETER_H__
#define __SCRIPT_INTERPRETER_H__

#define MAX_STACK_DEPTH 	64
#define LOCALSTACK_SIZE 	6144

typedef struct prstack_s {
	int 				s;
	const function_t	*f;
	int 				stackbase;
} prstack_t;

class idInterpreter {
private:
	prstack_t			callStack[ MAX_STACK_DEPTH ];
	int 				callStackDepth;
	int 				maxStackDepth;

	byte				localstack[ LOCALSTACK_SIZE ];
	int 				localstackUsed;
	int 				localstackBase;
	int 				maxLocalstackUsed;

	const function_t	*currentFunction;
	int 				instructionPointer;

	int					popParms;
	const idEventDef	*multiFrameEvent;
	abEntity			*eventEntity;

	idThread			*thread;

	void				PopParms( int numParms );
	void				PushString( const char *string );
	void				Push( int value );
	const char			*FloatToString( float value );
	void				AppendString( idVarDef *def, const char *from );
	void				SetString( idVarDef *def, const char *from );
	const char			*GetString( idVarDef *def );
	varEval_t			GetVariable( idVarDef *def );
	abEntity			*GetEntity( int entnum ) const;
	idScriptObject		*GetScriptObject( int entnum ) const;
	void				NextInstruction( int position );

	void				LeaveFunction( idVarDef *returnDef );
	void				CallEvent( const function_t *func, int argsize );
	void				CallSysEvent( const function_t *func, int argsize );

public:
	bool				doneProcessing;
	bool				threadDying;
	bool				terminateOnExit;
	bool				debug;

						idInterpreter();

	// save games
	void				Save( anSaveGame *savefile ) const;				// archives object for save game file
	void				Restore( anRestoreGame *savefile );				// unarchives object from save game file

	void				SetThread( idThread *pThread );

	void				StackTrace( void ) const;

	int					CurrentLine( void ) const;
	const char			*CurrentFile( void ) const;

	void				Error( char *fmt, ... ) const an_attribute( ( format( printf, 2, 3 ) ) );
	void				Warning( char *fmt, ... ) const an_attribute( ( format( printf, 2, 3 ) ) );
	void				DisplayInfo( void ) const;

	bool				BeginMultiFrameEvent( abEntity *ent, const idEventDef *event );
	void				EndMultiFrameEvent( abEntity *ent, const idEventDef *event );
	bool				MultiFrameEventInProgress( void ) const;

	void				ThreadCall( idInterpreter *source, const function_t *func, int args );
	void				EnterFunction( const function_t *func, bool clearStack );
	void				EnterObjectFunction( abEntity *self, const function_t *func, bool clearStack );

	bool				Execute( void );
	void				Reset( void );

	bool				GetRegisterValue( const char *name, anStr &out, int scopeDepth );
	int					GetCallstackDepth( void ) const;
	const prstack_t		*GetCallstack( void ) const;
	const function_t	*GetCurrentFunction( void ) const;
	idThread			*GetThread( void ) const;

};

/*
====================
idInterpreter::PopParms
====================
*/
inline void idInterpreter::PopParms( int numParms ) {
	// pop our parms off the stack
	if ( localstackUsed < numParms ) {
		Error( "locals stack underflow\n" );
	}

	localstackUsed -= numParms;
}

/*
====================
idInterpreter::Push
====================
*/
inline void idInterpreter::Push( int value ) {
	if ( localstackUsed + sizeof( int ) > LOCALSTACK_SIZE ) {
		Error( "Push: locals stack overflow\n" );
	}
	*( int * )&localstack[ localstackUsed ]	= value;
	localstackUsed += sizeof( int );
}

/*
====================
idInterpreter::PushString
====================
*/
inline void idInterpreter::PushString( const char *string ) {
	if ( localstackUsed + MAX_STRING_LEN > LOCALSTACK_SIZE ) {
		Error( "PushString: locals stack overflow\n" );
	}
	anStr::Copynz( (char *)&localstack[ localstackUsed ], string, MAX_STRING_LEN );
	localstackUsed += MAX_STRING_LEN;
}

/*
====================
idInterpreter::FloatToString
====================
*/
inline const char *idInterpreter::FloatToString( float value ) {
	static char	text[ 32 ];

	if ( value == ( float )( int )value ) {
		sprintf( text, "%d", ( int )value );
	} else {
		sprintf( text, "%f", value );
	}
	return text;
}

/*
====================
idInterpreter::AppendString
====================
*/
inline void idInterpreter::AppendString( idVarDef *def, const char *from ) {
	if ( def->initialized == idVarDef::stackVariable ) {
		anStr::Append( (char *)&localstack[ localstackBase + def->value.stackOffset ], MAX_STRING_LEN, from );
	} else {
		anStr::Append( def->value.stringPtr, MAX_STRING_LEN, from );
	}
}

/*
====================
idInterpreter::SetString
====================
*/
inline void idInterpreter::SetString( idVarDef *def, const char *from ) {
	if ( def->initialized == idVarDef::stackVariable ) {
		anStr::Copynz( (char *)&localstack[ localstackBase + def->value.stackOffset ], from, MAX_STRING_LEN );
	} else {
		anStr::Copynz( def->value.stringPtr, from, MAX_STRING_LEN );
	}
}

/*
====================
idInterpreter::GetString
====================
*/
inline const char *idInterpreter::GetString( idVarDef *def ) {
	if ( def->initialized == idVarDef::stackVariable ) {
		return (char *)&localstack[ localstackBase + def->value.stackOffset ];
	} else {
		return def->value.stringPtr;
	}
}

/*
====================
idInterpreter::GetVariable
====================
*/
inline varEval_t idInterpreter::GetVariable( idVarDef *def ) {
	if ( def->initialized == idVarDef::stackVariable ) {
		varEval_t val;
		val.intPtr = ( int * )&localstack[ localstackBase + def->value.stackOffset ];
		return val;
	} else {
		return def->value;
	}
}

/*
================
idInterpreter::GetEntity
================
*/
inline abEntity *idInterpreter::GetEntity( int entnum ) const{
	assert( entnum <= MAX_GENTITIES );
	if ( ( entnum > 0 ) && ( entnum <= MAX_GENTITIES ) ) {
		return gameLocal.entities[ entnum - 1 ];
	}
	return nullptr;
}

/*
================
idInterpreter::GetScriptObject
================
*/
inline idScriptObject *idInterpreter::GetScriptObject( int entnum ) const {
	abEntity *ent;

	assert( entnum <= MAX_GENTITIES );
	if ( ( entnum > 0 ) && ( entnum <= MAX_GENTITIES ) ) {
		ent = gameLocal.entities[ entnum - 1 ];
		if ( ent && ent->scriptObject.data ) {
			return &ent->scriptObject;
		}
	}
	return nullptr;
}

/*
====================
idInterpreter::NextInstruction
====================
*/
inline void idInterpreter::NextInstruction( int position ) {
	// Before we execute an instruction, we increment instructionPointer,
	// therefore we need to compensate for that here.
	instructionPointer = position - 1;
}

#endif /* !__SCRIPT_INTERPRETER_H__ */

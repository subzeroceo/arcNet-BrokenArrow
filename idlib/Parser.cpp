#include "precompiled.h"
#pragma hdrstop

//#define DEBUG_EVAL
#define MAX_DEFINEPARMS				128
#define DEFINEHASHSIZE				2048

#define TOKEN_FL_RECURSIVE_DEFINE	1

define_t * ARCParser::globalDefines;

/*
================
ARCParser::SetBaseFolder
================
*/
void ARCParser::SetBaseFolder( const char *path ) {
	arcLexer::SetBaseFolder( path );
}

/*
================
ARCParser::AddGlobalDefine
================
*/
int ARCParser::AddGlobalDefine( const char *string ) {
	define_t *define;

	define = ARCParser::DefineFromString( string );
	if ( !define ) {
		return false;
	}
	define->next = globalDefines;
	globalDefines = define;
	return true;
}

/*
================
ARCParser::RemoveGlobalDefine
================
*/
int ARCParser::RemoveGlobalDefine( const char *name ) {
        for ( define_t *prev = NULL, d = ARCParser::globalDefines; d;
             prev = d, d = d->next ) {
          if ( !strcmp( d->name, name ) ) {
            break;
          }
        }
        if ( define_t * d ) {
          if ( prev ) {
            prev->next = d->next;
          } else {
            ARCParser::globalDefines = d->next;
          }
          ARCParser::FreeDefine( d );
          return true;
        }
        return false;
}

/*
================
ARCParser::RemoveAllGlobalDefines
================
*/
void ARCParser::RemoveAllGlobalDefines( void ) {
	define_t *define;

	for ( define = globalDefines; define; define = globalDefines ) {
		globalDefines = globalDefines->next;
		ARCParser::FreeDefine( define );
	}
}


/*
===============================================================================

ARCParser

===============================================================================
*/

/*
================
ARCParser::PrintDefine
================
*/
void ARCParser::PrintDefine( define_t *define ) {
	arcLibrary::common->Printf( "define->name = %s\n", define->name);
	arcLibrary::common->Printf( "define->flags = %d\n", define->flags);
	arcLibrary::common->Printf( "define->builtin = %d\n", define->builtin);
	arcLibrary::common->Printf( "define->numParms = %d\n", define->numParms);
}

/*
================
PC_PrintDefineHashTable
================
*/
static void PC_PrintDefineHashTable( define_t **defineHash ) {
	for ( int i = 0; i < DEFINEHASHSIZE; i++ ) {
		Log_Write( "%4d:", i);
		for (  define_t *d = defineHash[i]; d; d = d->hashNext ) {
			Log_Write( " %s", d->name );
		}
		Log_Write( "\n" );
	}
}


/*
================
PC_NameHash
================
*/
ARC_INLINE int PC_NameHash( const char *name ) {
	int hash, i;

	hash = 0;
	for ( i = 0; name[i] != '\0'; i++ ) {
		hash += name[i] * (119 + i);
	}
	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( DEFINEHASHSIZE-1 );
	return hash;
}

/*
================
ARCParser::AddDefineToHash
================
*/
void ARCParser::AddDefineToHash( define_t *define, define_t **defineHash ) {
	int hash;

	hash = PC_NameHash( define->name );
	define->hashNext = defineHash[hash];
	defineHash[hash] = define;
}

/*
================
FindHashedDefine
================
*/
define_t *ARCParser::FindHashedDefine( define_t **defineHash, const char *name ) {
	define_t *d;
	int hash;

	hash = PC_NameHash( name );
	for ( d = defineHash[hash]; d; d = d->hashNext ) {
		if ( !strcmp( d->name, name ) ) {
			return d;
		}
	}
	return NULL;
}

/*
================
ARCParser::FindDefine
================
*/
define_t *ARCParser::FindDefine( define_t *defines, const char *name ) {
	define_t *d;

	for ( d = defines; d; d = d->next ) {
		if ( !strcmp( d->name, name ) ) {
			return d;
		}
	}
	return NULL;
}

/*
================
ARCParser::FindDefineParm
================
*/
int ARCParser::FindDefineParm( define_t *define, const char *name ) {
	arcNetToken *p;
	int i;

	i = 0;
	for ( p = define->parms; p; p = p->next ) {
		if ( ( *p ) == name ) {
			return i;
		}
		i++;
	}
	return -1;
}

/*
================
ARCParser::CopyDefine
================
*/
define_t *ARCParser::CopyDefine( define_t *define ) {
	arcNetToken *token, *newToken, *lastToken;

	define_t *newDefine = ( define_t *) Mem_Alloc( sizeof( define_t ) + strlen( define->name ) + 1 );

	// copy the define name
	newDefine->name = (char *) newDefine + sizeof( define_t );
	strcpy( newDefine->name, define->name );

	newDefine->flags = define->flags;
	newDefine->builtin = define->builtin;
	newDefine->numParms = define->numParms;

	// the define is not linked
	newDefine->next = NULL;
	newDefine->hashNext = NULL;

	// copy the define tokens
	newDefine->tokens = NULL;

	for ( arcNetToken *lastToken = NULL, token = define->tokens; token; token = token->next ) {
		newToken = new arcNetToken( token );
		newToken->next = NULL;
		if ( lastToken ) lastToken->next = newToken;
		else newDefine->tokens = newToken;
		lastToken = newToken;
	}
	// copy the define parameters
	newDefine->parms = NULL;
	for ( lastToken = NULL, token = define->parms; token; token = token->next ) {
		newToken = new arcNetToken( token );
		newToken->next = NULL;
		if ( lastToken ) lastToken->next = newToken;
		else newDefine->parms = newToken;
		lastToken = newToken;
	}
	return newDefine;
}

/*
================
ARCParser::FreeDefine
================
*/
void ARCParser::FreeDefine( define_t *define ) {
	// free the define parameters
	for ( arcNetToken *t = define->parms; t; t = next ) {
		arcNetToken *next = t->next;
		delete t;
	}
	// free the define tokens
	for ( t = define->tokens; t; t = next) {
		next = t->next;
		delete t;
	}
	// free the define
	Mem_Free( define );
}

/*
================
ARCParser::DefineFromString
================
*/
define_t *ARCParser::DefineFromString( const char *string ) {
	ARCParser src;
	define_t *def;

	if ( !src.LoadMemory(string, strlen( string ), "*defineString" ) ) {
		return NULL;
	}
	// create a define from the source
	if ( !src.DirectiveDefine() ) {
		src.FreeSource();
		return NULL;
	}
	def = src.CopyFirstDefine();
	src.FreeSource();
	// if the define was created succesfully
	return def;
}

/*
================
ARCParser::Error
================
*/
void ARCParser::Error( const char *str, ... ) const {
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);
	if ( ARCParser::scriptStack ) {
		ARCParser::scriptStack->Error( text );
	}
}

/*
================
ARCParser::Warning
================
*/
void ARCParser::Warning( const char *str, ... ) const {
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);
	if ( ARCParser::scriptStack ) {
		ARCParser::scriptStack->Warning( text );
	}
}

/*
================
ARCParser::PushIndent
================
*/
void ARCParser::PushIndent( int type, int skip ) {
	indent_t *indent = (indent_t *) Mem_Alloc( sizeof(indent_t) );
	indent->type = type;
	indent->script = ARCParser::scriptStack;
	indent->skip = (skip != 0 );
	ARCParser::skip += indent->skip;
	indent->next = ARCParser::indentStack;
	ARCParser::indentStack = indent;
}

/*
================
ARCParser::PopIndent
================
*/
void ARCParser::PopIndent( int *type, int *skip ) {
	indent_t *indent;

	*type = 0;
	*skip = 0;

	indent = ARCParser::indentStack;
	if ( !indent) return;

	// must be an indent from the current script
	if ( ARCParser::indentStack->script != ARCParser::scriptStack) {
		return;
	}

	*type = indent->type;
	*skip = indent->skip;
	ARCParser::indentStack = ARCParser::indentStack->next;
	ARCParser::skip -= indent->skip;
	Mem_Free( indent );
}

/*
================
ARCParser::PushScript
================
*/
void ARCParser::PushScript( arcLexer *script ) {
	arcLexer *s;

	for ( s = ARCParser::scriptStack; s; s = s->next ) {
		if ( !arcNetString::Icmp(s->GetFileName(), script->GetFileName() ) ) {
			ARCParser::Warning( "'%s' recursively included", script->GetFileName() );
			return;
		}
	}
	//push the script on the script stack
	script->next = ARCParser::scriptStack;
	ARCParser::scriptStack = script;
}

/*
================
ARCParser::ReadSourceToken
================
*/
int ARCParser::ReadSourceToken( arcNetToken *token ) {
	arcNetToken *t;
	arcLexer *script;
	int type, skip, changedScript;

	if ( !ARCParser::scriptStack ) {
		arcLibrary::common->FatalError( "ARCParser::ReadSourceToken: not loaded" );
		return false;
	}
	changedScript = 0;
	// if there's no token already available
	while( !ARCParser::tokens ) {
		// if there's a token to read from the script
		if ( ARCParser::scriptStack->ReadToken( token ) ) {
			token->linesCrossed += changedScript;

			// set the marker based on the start of the token read in
			if ( !marker_p ) {
				marker_p = token->whiteSpaceEnd_p;
			}
			return true;
		}
		// if at the end of the script
		if ( ARCParser::scriptStack->EndOfFile() ) {
			// remove all indents of the script
			while( ARCParser::indentStack && ARCParser::indentStack->script == ARCParser::scriptStack ) {
				ARCParser::Warning( "missing #endif" );
				ARCParser::PopIndent( &type, &skip );
			}
			changedScript = 1;
		}
		// if this was the initial script
		if ( !ARCParser::scriptStack->next ) {
			return false;
		}
		// remove the script and return to the previous one
		script = ARCParser::scriptStack;
		ARCParser::scriptStack = ARCParser::scriptStack->next;
		delete script;
	}
	// copy the already available token
	*token = ARCParser::tokens;
	// remove the token from the source
	t = ARCParser::tokens;
	ARCParser::tokens = ARCParser::tokens->next;
	delete t;
	return true;
}

/*
================
ARCParser::UnreadSourceToken
================
*/
int ARCParser::UnreadSourceToken( arcNetToken *token ) {
	arcNetToken *t;

	t = new arcNetToken( token );
	t->next = ARCParser::tokens;
	ARCParser::tokens = t;
	return true;
}

/*
================
ARCParser::ReadDefineParms
================
*/
int ARCParser::ReadDefineParms( define_t *define, arcNetToken **parms, int maxParms ) {
	define_t *newDefine;
	arcNetToken token, *t, *last;
	int i, done, lastcomma, numParms, indent;

	if ( !ARCParser::ReadSourceToken( &token ) ) {
		ARCParser::Error( "define '%s' missing parameters", define->name );
		return false;
	}

	if ( define->numParms > maxParms ) {
		ARCParser::Error( "define with more than %d parameters", maxParms );
		return false;
	}

	for ( i = 0; i < define->numParms; i++ ) {
		parms[i] = NULL;
	}
	// if no leading "( "
	if ( token != "( " ) {
		ARCParser::UnreadSourceToken( &token );
		ARCParser::Error( "define '%s' missing parameters", define->name );
		return false;
	}
	// read the define parameters
	for ( done = 0, numParms = 0, indent = 1; !done; ) {
		if ( numParms >= maxParms ) {
			ARCParser::Error( "define '%s' with too many parameters", define->name );
			return false;
		}
		parms[numParms] = NULL;
		lastcomma = 1;
		last = NULL;
		while( !done ) {

			if ( !ARCParser::ReadSourceToken( &token ) ) {
				ARCParser::Error( "define '%s' incomplete", define->name );
				return false;
			}

			if ( token == "," ) {
				if ( indent <= 1 ) {
					if ( lastcomma ) {
						ARCParser::Warning( "too many comma's" );
					}
					if ( numParms >= define->numParms ) {
						ARCParser::Warning( "too many define parameters" );
					}
					lastcomma = 1;
					break;
				}
			} else if ( token == "( " ) {
				indent++;
			} else if ( token == " )" ) {
				indent--;
				if ( indent <= 0 ) {
					if ( !parms[define->numParms-1] ) {
						ARCParser::Warning( "too few define parameters" );
					}
					done = 1;
					break;
				}
			} else if ( token.type == TT_NAME ) {
				newDefine = FindHashedDefine( ARCParser::defineHash, token.c_str() );
				if ( newDefine ) {
					if ( !ARCParser::ExpandDefineIntoSource( &token, newDefine ) ) {
						return false;
					}
					continue;
				}
			}

			lastcomma = 0;

			if ( numParms < define->numParms ) {

				t = new arcNetToken( token );
				t->next = NULL;
				if (last) last->next = t;
				else parms[numParms] = t;
				last = t;
			}
		}
		numParms++;
	}
	return true;
}

/*
================
ARCParser::StringizeTokens
================
*/
int ARCParser::StringizeTokens( arcNetToken *tokens, arcNetToken *token ) {
	arcNetToken *t;

	token->type = TT_STRING;
	token->whiteSpaceStart_p = NULL;
	token->whiteSpaceEnd_p = NULL;
	(*token) = "";
	for ( t = tokens; t; t = t->next ) {
		token->Append( t->c_str() );
	}
	return true;
}

/*
================
ARCParser::MergeTokens
================
*/
int ARCParser::MergeTokens( arcNetToken *t1, arcNetToken *t2 ) {
	// merging of a name with a name or number
	if ( t1->type == TT_NAME && (t2->type == TT_NAME || (t2->type == TT_NUMBER && !(t2->subtype & TT_FLOAT) )) ) {
		t1->Append( t2->c_str() );
		return true;
	}
	// merging of two strings
	if (t1->type == TT_STRING && t2->type == TT_STRING) {
		t1->Append( t2->c_str() );
		return true;
	}
	// merging of two numbers
	if ( t1->type == TT_NUMBER && t2->type == TT_NUMBER &&
			!(t1->subtype & (TT_HEX|TT_BINARY) ) && !(t2->subtype & (TT_HEX|TT_BINARY) ) &&
			( !(t1->subtype & TT_FLOAT) || !(t2->subtype & TT_FLOAT) ) ) {
		t1->Append( t2->c_str() );
		return true;
	}

	return false;
}

/*
================
ARCParser::AddBuiltinDefines
================
*/
void ARCParser::AddBuiltinDefines( void ) {
	int i;
	define_t *define;
	struct builtin {
		char *string;
		int id;
	} builtin[] = {
		{ "__LINE__",	BUILTIN_LINE },
		{ "__FILE__",	BUILTIN_FILE },
		{ "__DATE__",	BUILTIN_DATE },
		{ "__TIME__",	BUILTIN_TIME },
		{ "__STDC__", BUILTIN_STDC },
		{ NULL, 0 }
	};

	for ( i = 0; builtin[i].string; i++ ) {
		define = ( define_t *) Mem_Alloc( sizeof( define_t ) + strlen(builtin[i].string ) + 1 );
		define->name = (char *) define + sizeof( define_t );
		strcpy( define->name, builtin[i].string );
		define->flags = DEFINE_FIXED;
		define->builtin = builtin[i].id;
		define->numParms = 0;
		define->parms = NULL;
		define->tokens = NULL;
		// add the define to the source
		AddDefineToHash( define, ARCParser::defineHash );
	}
}

/*
================
ARCParser::CopyFirstDefine
================
*/
define_t *ARCParser::CopyFirstDefine( void ) {
	int i;

	for ( i = 0; i < DEFINEHASHSIZE; i++ ) {
		if ( ARCParser::defineHash[i] ) {
			return CopyDefine( ARCParser::defineHash[i] );
		}
	}
	return NULL;
}

/*
================
ARCParser::ExpandBuiltinDefine
================
*/
int ARCParser::ExpandBuiltinDefine( arcNetToken *defToken, define_t *define, arcNetToken **firstToken, arcNetToken **lastToken ) {
	arcNetToken *token;
	ARC_TIME_T t;
	char *curtime;
	char buf[MAX_STRING_CHARS];

	token = new arcNetToken(defToken);
	switch( define->builtin ) {
		case BUILTIN_LINE: {
			sprintf( buf, "%d", defToken->line );
			(*token) = buf;
			token->intVal = defToken->line;
			token->floatVal = defToken->line;
			token->type = TT_NUMBER;
			token->subtype = TT_DECIMAL | TT_INTEGER | TT_VALUESVALID;
			token->line = defToken->line;
			token->linesCrossed = defToken->linesCrossed;
			token->flags = 0;
			*firstToken = token;
			*lastToken = token;
			break;
		}
		case BUILTIN_FILE: {
			(*token) = ARCParser::scriptStack->GetFileName();
			token->type = TT_NAME;
			token->subtype = token->Length();
			token->line = defToken->line;
			token->linesCrossed = defToken->linesCrossed;
			token->flags = 0;
			*firstToken = token;
			*lastToken = token;
			break;
		}
		case BUILTIN_DATE: {
			t = time(NULL);
			curtime = ctime(&t);
			(*token) = "\"";
			token->Append( curtime+4 );
			token[7] = '\0';
			token->Append( curtime+20 );
			token[10] = '\0';
			token->Append( "\"" );
			free(curtime);
			token->type = TT_STRING;
			token->subtype = token->Length();
			token->line = defToken->line;
			token->linesCrossed = defToken->linesCrossed;
			token->flags = 0;
			*firstToken = token;
			*lastToken = token;
			break;
		}
		case BUILTIN_TIME: {
			t = time(NULL);
			curtime = ctime(&t);
			(*token) = "\"";
			token->Append( curtime+11 );
			token[8] = '\0';
			token->Append( "\"" );
			free(curtime);
			token->type = TT_STRING;
			token->subtype = token->Length();
			token->line = defToken->line;
			token->linesCrossed = defToken->linesCrossed;
			token->flags = 0;
			*firstToken = token;
			*lastToken = token;
			break;
		}
		case BUILTIN_STDC: {
			ARCParser::Warning( "__STDC__ not supported\n" );
			*firstToken = NULL;
			*lastToken = NULL;
			break;
		}
		default: {
			*firstToken = NULL;
			*lastToken = NULL;
			break;
		}
	}
	return true;
}

/*
================
ARCParser::ExpandDefine
================
*/
int ARCParser::ExpandDefine( arcNetToken *defToken, define_t *define, arcNetToken **firstToken, arcNetToken **lastToken ) {
	arcNetToken *parms[MAX_DEFINEPARMS], *dt, *pt, *t;
	arcNetToken *t1, *t2, *first, *last, *nextpt, token;
	int parmnum, i;

	// if it is a builtin define
	if ( define->builtin ) {
		return ARCParser::ExpandBuiltinDefine( defToken, define, firstToken, lastToken );
	}
	// if the define has parameters
	if ( define->numParms ) {
		if ( !ARCParser::ReadDefineParms( define, parms, MAX_DEFINEPARMS ) ) {
			return false;
		}
#ifdef DEBUG_EVAL
		for ( i = 0; i < define->numParms; i++ ) {
			Log_Write( "define parms %d:", i);
			for ( pt = parms[i]; pt; pt = pt->next ) {
				Log_Write( "%s", pt->c_str() );
			}
		}
#endif //DEBUG_EVAL
	}
	// empty list at first
	first = NULL;
	last = NULL;
	// create a list with tokens of the expanded define
	for ( dt = define->tokens; dt; dt = dt->next ) {
		parmnum = -1;
		// if the token is a name, it could be a define parameter
		if ( dt->type == TT_NAME ) {
			parmnum = FindDefineParm( define, dt->c_str() );
		}
		// if it is a define parameter
		if ( parmnum >= 0 ) {
			for ( pt = parms[parmnum]; pt; pt = pt->next ) {
				t = new arcNetToken(pt);
				//add the token to the list
				t->next = NULL;
				if (last) last->next = t;
				else first = t;
				last = t;
			}
		} else {
			// if stringizing operator
			if ( (*dt) == "#" ) {
				// the stringizing operator must be followed by a define parameter
				if ( dt->next ) {
					parmnum = FindDefineParm( define, dt->next->c_str() );
				} else {
					parmnum = -1;
				}

				if ( parmnum >= 0 ) {
					// step over the stringizing operator
					dt = dt->next;
					// stringize the define parameter tokens
					if ( !ARCParser::StringizeTokens( parms[parmnum], &token ) ) {
						ARCParser::Error( "can't stringize tokens" );
						return false;
					}
					t = new arcNetToken( token );
					t->line = defToken->line;
				} else {
					ARCParser::Warning( "stringizing operator without define parameter" );
					continue;
				}
			} else {
				t = new arcNetToken(dt);
				t->line = defToken->line;
			}
			// add the token to the list
			t->next = NULL;
// the token being read from the define list should use the line number of
// the original file, not the header file
			t->line = defToken->line;

			if ( last ) last->next = t;
			else first = t;
			last = t;
		}
	}
	// check for the merging operator
	for ( t = first; t; ) {
		if ( t->next ) {
			// if the merging operator
			if ( (*t->next) == "##" ) {
				t1 = t;
				t2 = t->next->next;
				if ( t2 ) {
					if ( !ARCParser::MergeTokens( t1, t2 ) ) {
						ARCParser::Error( "can't merge '%s' with '%s'", t1->c_str(), t2->c_str() );
						return false;
					}
					delete t1->next;
					t1->next = t2->next;
					if ( t2 == last ) last = t1;
					delete t2;
					continue;
				}
			}
		}
		t = t->next;
	}
	// store the first and last token of the list
	*firstToken = first;
	*lastToken = last;
	// free all the parameter tokens
	for ( i = 0; i < define->numParms; i++ ) {
		for ( pt = parms[i]; pt; pt = nextpt ) {
			nextpt = pt->next;
			delete pt;
		}
	}

	return true;
}

/*
================
ARCParser::ExpandDefineIntoSource
================
*/
int ARCParser::ExpandDefineIntoSource( arcNetToken *defToken, define_t *define ) {
	arcNetToken *firstToken, *lastToken;

	if ( !ARCParser::ExpandDefine( defToken, define, &firstToken, &lastToken ) ) {
		return false;
	}
	// if the define is not empty
	if ( firstToken && lastToken ) {
		firstToken->linesCrossed += defToken->linesCrossed;
		lastToken->next = ARCParser::tokens;
		ARCParser::tokens = firstToken;
	}
	return true;
}

/*
================
ARCParser::ReadLine

reads a token from the current line, continues reading on the next
line only if a backslash '\' is found
================
*/
int ARCParser::ReadLine( arcNetToken *token ) {
	int crossline;

	crossline = 0;
	do {
		if ( !ARCParser::ReadSourceToken( token ) ) {
			return false;
		}

		if (token->linesCrossed > crossline) {
			ARCParser::UnreadSourceToken( token );
			return false;
		}
		crossline = 1;
	} while( (*token) == "\\" );
	return true;
}

/*
================
ARCParser::DirectiveInclude
================
*/
int ARCParser::DirectiveInclude( void ) {
	arcLexer *script;
	arcNetToken token;
	arcNetString path;

	if ( !ARCParser::ReadSourceToken( &token ) ) {
		ARCParser::Error( "#include without file name" );
		return false;
	}
	if ( token.linesCrossed > 0 ) {
		ARCParser::Error( "#include without file name" );
		return false;
	}
	if ( token.type == TT_STRING ) {
		script = new arcLexer;
		// try relative to the current file
		path = scriptStack->GetFileName();
		path.StripFilename();
		path += "/";
		path += token;
		if ( !script->LoadFile( path, OSPath ) ) {
			// try absolute path
			path = token;
			if ( !script->LoadFile( path, OSPath ) ) {
				// try from the include path
				path = includepath + token;
				if ( !script->LoadFile( path, OSPath ) ) {
					delete script;
					script = NULL;
				}
			}
		}
	} else if ( token.type == TT_PUNCTUATION && token == "<" ) {
		path = ARCParser::includepath;
		while( ARCParser::ReadSourceToken( &token ) ) {
			if ( token.linesCrossed > 0 ) {
				ARCParser::UnreadSourceToken( &token );
				break;
			}
			if ( token.type == TT_PUNCTUATION && token == ">" ) {
				break;
			}
			path += token;
		}
		if ( token != ">" ) {
			ARCParser::Warning( "#include missing trailing >" );
		}
		if ( !path.Length() ) {
			ARCParser::Error( "#include without file name between < >" );
			return false;
		}
		if ( ARCParser::flags & LEXFL_NOBASEINCLUDES ) {
			return true;
		}
		script = new arcLexer;
		if ( !script->LoadFile( includepath + path, OSPath ) ) {
			delete script;
			script = NULL;
		}
	} else {
		ARCParser::Error( "#include without file name" );
		return false;
	}
	if ( !script) {
		ARCParser::Error( "file '%s' not found", path.c_str() );
		return false;
	}
	script->SetFlags( ARCParser::flags );
	script->SetPunctuations( ARCParser::punctuations );
	ARCParser::PushScript( script );
	return true;
}

/*
================
ARCParser::DirectiveUNdef
================
*/
int ARCParser::DirectiveUNdef( void ) {
	arcNetToken token;
	define_t *define, *lastdefine;
	int hash;

	if ( !ARCParser::ReadLine( &token ) ) {
		ARCParser::Error( "undef without name" );
		return false;
	}
	if (token.type != TT_NAME) {
		ARCParser::UnreadSourceToken( &token );
		ARCParser::Error( "expected name but found '%s'", token.c_str() );
		return false;
	}

	hash = PC_NameHash( token.c_str() );
	for (lastdefine = NULL, define = ARCParser::defineHash[hash]; define; define = define->hashNext) {
		if ( !strcmp( define->name, token.c_str() ) ) {
			if ( define->flags & DEFINE_FIXED) {
				ARCParser::Warning( "can't undef '%s'", token.c_str() );
			} else {
				if (lastdefine) {
					lastdefine->hashNext = define->hashNext;
				} else {
					ARCParser::defineHash[hash] = define->hashNext;
				}
				FreeDefine( define );
			}
			break;
		}
		lastdefine = define;
	}
	return true;
}

/*
================
ARCParser::DirectiveDefine
================
*/
int ARCParser::DirectiveDefine( void ) {
	arcNetToken token, *t, *last;
	define_t *define;

	if ( !ARCParser::ReadLine( &token ) ) {
		ARCParser::Error( "#define without name" );
		return false;
	}
	if (token.type != TT_NAME) {
		ARCParser::UnreadSourceToken( &token );
		ARCParser::Error( "expected name after #define, found '%s'", token.c_str() );
		return false;
	}
	// check if the define already exists
	define = FindHashedDefine( ARCParser::defineHash, token.c_str() );
	if ( define ) {
		if ( define->flags & DEFINE_FIXED) {
			ARCParser::Error( "can't redefine '%s'", token.c_str() );
			return false;
		}
		ARCParser::Warning( "redefinition of '%s'", token.c_str() );
		// unread the define name before executing the #undef directive
		ARCParser::UnreadSourceToken( &token );
		if ( !ARCParser::DirectiveUNdef() )
			return false;
		// if the define was not removed ( define->flags & DEFINE_FIXED )
		define = FindHashedDefine( ARCParser::defineHash, token.c_str() );
	}
	// allocate define
	define = ( define_t *) Mem_ClearedAlloc( sizeof( define_t ) + token.Length() + 1 );
	define->name = (char *) define + sizeof( define_t );
	strcpy( define->name, token.c_str() );
	// add the define to the source
	AddDefineToHash( define, ARCParser::defineHash );
	// if nothing is defined, just return
	if ( !ARCParser::ReadLine( &token ) ) {
		return true;
	}
	// if it is a define with parameters
	if ( token.WhiteSpaceBeforeToken() == 0 && token == "( " ) {
		// read the define parameters
		last = NULL;
		if ( !ARCParser::CheckTokenString( " )" ) ) {
			while ( 1 ) {
				if ( !ARCParser::ReadLine( &token ) ) {
					ARCParser::Error( "expected define parameter" );
					return false;
				}
				// if it isn't a name
				if (token.type != TT_NAME) {
					ARCParser::Error( "invalid define parameter" );
					return false;
				}

				if ( FindDefineParm( define, token.c_str() ) >= 0 ) {
					ARCParser::Error( "two the same define parameters" );
					return false;
				}
				// add the define parm
				t = new arcNetToken( token );
				t->ClearTokenWhiteSpace();
				t->next = NULL;
				if (last) last->next = t;
				else define->parms = t;
				last = t;
				define->numParms++;
				// read next token
				if ( !ARCParser::ReadLine( &token ) ) {
					ARCParser::Error( "define parameters not terminated" );
					return false;
				}

				if ( token == " )" ) {
					break;
				}
				// then it must be a comma
				if ( token != "," ) {
					ARCParser::Error( "define not terminated" );
					return false;
				}
			}
		}
		if ( !ARCParser::ReadLine( &token ) ) {
			return true;
		}
	}
	// read the defined stuff
	last = NULL;
	do {
		t = new arcNetToken( token );
		if ( t->type == TT_NAME && !strcmp( t->c_str(), define->name ) ) {
			t->flags |= TOKEN_FL_RECURSIVE_DEFINE;
			ARCParser::Warning( "recursive define (removed recursion)" );
		}
		t->ClearTokenWhiteSpace();
		t->next = NULL;
		if ( last ) last->next = t;
		else define->tokens = t;
		last = t;
	} while( ARCParser::ReadLine( &token ) );
	if ( last ) {
		// check for merge operators at the beginning or end
		if ( (*define->tokens) == "##" || (*last) == "##" ) {
			ARCParser::Error( "define with misplaced ##" );
			return false;
		}
	}
	return true;
}

/*
================
ARCParser::AddDefine
================
*/
int ARCParser::AddDefine( const char *string ) {
	define_t *define;

	define = DefineFromString( string );
	if ( !define ) {
		return false;
	}
	AddDefineToHash( define, ARCParser::defineHash );
	return true;
}

/*
================
ARCParser::AddGlobalDefinesToSource
================
*/
void ARCParser::AddGlobalDefinesToSource( void ) {
	define_t *define, *newDefine;

	for ( define = globalDefines; define; define = define->next) {
		newDefine = CopyDefine( define );
		AddDefineToHash(newDefine, ARCParser::defineHash );
	}
}

/*
================
ARCParser::DirectiveIFdef
================
*/
int ARCParser::DirectiveIFdef( int type ) {
	arcNetToken token;
	define_t *d;
	int skip;

	if ( !ARCParser::ReadLine( &token ) ) {
		ARCParser::Error( "#ifdef without name" );
		return false;
	}
	if (token.type != TT_NAME) {
		ARCParser::UnreadSourceToken( &token );
		ARCParser::Error( "expected name after #ifdef, found '%s'", token.c_str() );
		return false;
	}
	d = FindHashedDefine( ARCParser::defineHash, token.c_str() );
	skip = (type == INDENT_IFDEF) == (d == NULL);
	ARCParser::PushIndent( type, skip );
	return true;
}

/*
================
ARCParser::DirectiveIFDEF
================
*/
int ARCParser::DirectiveIFDEF( void ) {
	return ARCParser::DirectiveIFdef( INDENT_IFDEF );
}

/*
================
ARCParser::DirectiveIFndef
================
*/
int ARCParser::DirectiveIFndef( void ) {
	return ARCParser::DirectiveIFdef( INDENT_IFNDEF );
}

/*
================
ARCParser::Directive_else
================
*/
int ARCParser::Directive_else( void ) {
	int type, skip;

	ARCParser::PopIndent( &type, &skip );
	if ( !type) {
		ARCParser::Error( "misplaced #else" );
		return false;
	}
	if (type == INDENT_ELSE) {
		ARCParser::Error( "#else after #else" );
		return false;
	}
	ARCParser::PushIndent( INDENT_ELSE, !skip );
	return true;
}

/*
================
ARCParser::Directive_endif
================
*/
int ARCParser::Directive_endif ( void ) {
	int type, skip;

	ARCParser::PopIndent( &type, &skip );
	if ( !type) {
		ARCParser::Error( "misplaced #endif" );
		return false;
	}
	return true;
}

/*
================
ARCParser::EvaluateTokens
================
*/
typedef struct operator_s {
	int op;
	int priority;
	int parentheses;
	struct operator_s *prev, *next;
} operator_t;

typedef struct value_s {
	signed long int intVal;
	double floatVal;
	int parentheses;
	struct value_s *prev, *next;
} value_t;

int PC_OperatorPriority( int op) {
	switch(op) {
		case P_MUL: return 15;
		case P_DIV: return 15;
		case P_MOD: return 15;
		case P_ADD: return 14;
		case P_SUB: return 14;

		case P_LOGIC_AND: return 7;
		case P_LOGIC_OR: return 6;
		case P_LOGIC_GEQ: return 12;
		case P_LOGIC_LEQ: return 12;
		case P_LOGIC_EQ: return 11;
		case P_LOGIC_UNEQ: return 11;

		case P_LOGIC_NOT: return 16;
		case P_LOGIC_GREATER: return 12;
		case P_LOGIC_LESS: return 12;

		case P_RSHIFT: return 13;
		case P_LSHIFT: return 13;

		case P_BIN_AND: return 10;
		case P_BIN_OR: return 8;
		case P_BIN_XOR: return 9;
		case P_BIN_NOT: return 16;

		case P_COLON: return 5;
		case P_QUESTIONMARK: return 5;
	}
	return false;
}

//#define AllocValue()			GetClearedMemory( sizeof(value_t) );
//#define FreeValue(val)		FreeMemory(val)
//#define AllocOperator(op)		op = (operator_t *) GetClearedMemory( sizeof(operator_t) );
//#define FreeOperator(op)		FreeMemory(op);

#define MAX_VALUES		64
#define MAX_OPERATORS	64

#define AllocValue(val) \
	if ( numValues >= MAX_VALUES ) { \
		ARCParser::Error( "out of value space\n" ); \
		error = 1; \
		break; \
	} else { \
		val = &value_heap[numValues++]; \
	}

#define FreeValue(val)

#define AllocOperator(op) \
	if ( numOps >= MAX_OPERATORS ) { \
		ARCParser::Error( "out of operator space\n" ); \
		error = 1; \
		break; \
	} else { \
		op = &operator_heap[numOps++]; \
	}

#define FreeOperator(op)

int ARCParser::EvaluateTokens( arcNetToken *tokens, signed long int *intVal, double *floatVal, int integer ) {
	operator_t *o, *firstOperator, *lastOperator;
	value_t *v, *firstValue, *lastValue, *v1, *v2;
	arcNetToken *t;
	int brace = 0;
	int parentheses = 0;
	int error = 0;
	int lastWasVal = 0;
	int negativeVal = 0;
	int QuestionMarkIntVal = 0;
	double questionMarkFloatVal = 0;
	int questionMarkVal = false;
	int lastOperatorType = 0;

	operator_t operator_heap[MAX_OPERATORS];
	int numOps = 0;
	value_t value_heap[MAX_VALUES];
	int numValues = 0;

	firstOperator = lastOperator = NULL;
	firstValue = lastValue = NULL;
	if ( intVal ) *intVal = 0;
	if ( floatVal)  *floatVal = 0;

	for ( t = tokens; t; t = t->next ) {
		switch( t->type ) {
			case TT_NAME: {
				if ( lastWasVal || negativeVal ) {
					ARCParser::Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				if ( (*t) != "defined" ) {
					ARCParser::Error( "undefined name '%s' in #if/#elif", t->c_str() );
					error = 1;
					break;
				}
				t = t->next;
				if ( (*t) == "( " ) {
					brace = true;
					t = t->next;
				}
				if ( !t || t->type != TT_NAM) {
					ARCParser::Error( "defined() without name in #if/#elif" );
					error = 1;
					break;
				}
				//v = (value_t *) GetClearedMemory( sizeof(value_t) );
				AllocValue( v );
				if ( FindHashedDefine( ARCParser::defineHash, t->c_str() ) ) {
					v->intVal = 1;
					v->floatVal = 1;
				} else {
					v->intVal = 0;
					v->floatVal = 0;
				}
				v->parentheses = parentheses;
				v->next = NULL;
				v->prev = lastValue;
				if (lastValue) lastValue->next = v;
				else firstValue = v;
				lastValue = v;
				if ( brace ) {
					t = t->next;
					if ( !t || (*t) != " )" ) {
						ARCParser::Error( "defined missing ) in #if/#elif" );
						error = 1;
						break;
					}
				}
				brace = false;
				// defined() creates a value
				lastWasVal = 1;
				break;
			}
			case TT_NUMBER: {
				if ( lastWasVal ) {
					ARCParser::Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				//v = (value_t *) GetClearedMemory( sizeof(value_t) );
				AllocValue( v );
				if ( negativeVal ) {
					v->intVal = - t->GetIntValue();
					v->floatVal = - t->GetFloatValue();
				} else {
					v->intVal = t->GetIntValue();
					v->floatVal = t->GetFloatValue();
				}
				v->parentheses = parentheses;
				v->next = NULL;
				v->prev = lastValue;
				if (lastValue) lastValue->next = v;
				else firstValue = v;
				lastValue = v;
				//last token was a value
				lastWasVal = 1;
				//
				negativeVal = 0;
				break;
			}
			case TT_PUNCTUATION: {
				if ( negativeVal ) {
					ARCParser::Error( "misplaced minus sign in #if/#elif" );
					error = 1;
					break;
				}
				if (t->subtype == P_PARENTHESESOPEN) {
					parentheses++;
					break;
				} else if (t->subtype == P_PARENTHESESCLOSE) {
					parentheses--;
					if (parentheses < 0 ) {
						ARCParser::Error( "too many ) in #if/#elsif" );
						error = 1;
					}
					break;
				}
				//check for invalid operators on floating point values
				if ( !integer ) {
					if (t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
						t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
						t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
						t->subtype == P_BIN_XOR) {
						ARCParser::Error( "illigal operator '%s' on floating point operands\n", t->c_str() );
						error = 1;
						break;
					}
				}
				switch( t->subtype ) {
					case P_LOGIC_NOT:
					case P_BIN_NOT: {
						if ( lastWasVal ) {
							ARCParser::Error( "! or ~ after value in #if/#elif" );
							error = 1;
							break;
						}
						break;
					}
					case P_INC:
					case P_DEC: {
						ARCParser::Error( "++ or -- used in #if/#elif" );
						break;
					}
					case P_SUB: {
						if ( !lastWasVal ) {
							negativeVal = 1;
							break;
						}
					}

					case P_MUL:
					case P_DIV:
					case P_MOD:
					case P_ADD:

					case P_LOGIC_AND:
					case P_LOGIC_OR:
					case P_LOGIC_GEQ:
					case P_LOGIC_LEQ:
					case P_LOGIC_EQ:
					case P_LOGIC_UNEQ:

					case P_LOGIC_GREATER:
					case P_LOGIC_LESS:

					case P_RSHIFT:
					case P_LSHIFT:

					case P_BIN_AND:
					case P_BIN_OR:
					case P_BIN_XOR:

					case P_COLON:
					case P_QUESTIONMARK:{
						if ( !lastWasVal) {
							ARCParser::Error( "operator '%s' after operator in #if/#elif", t->c_str() );
							error = 1;
							break;
						}
						break;
					}
					default: {
						ARCParser::Error( "invalid operator '%s' in #if/#elif", t->c_str() );
						error = 1;
						break;
					}
				}
				if ( !error && !negativeVal ) {
					//o = (operator_t *) GetClearedMemory( sizeof(operator_t) );
					AllocOperator(o);
					o->op = t->subtype;
					o->priority = PC_OperatorPriority(t->subtype);
					o->parentheses = parentheses;
					o->next = NULL;
					o->prev = lastOperator;
					if (lastOperator) lastOperator->next = o;
					else firstOperator = o;
					lastOperator = o;
					lastWasVal = 0;
				}
				break;
			}
			default: {
				ARCParser::Error( "unknown '%s' in #if/#elif", t->c_str() );
				error = 1;
				break;
			}
		}
		if ( error ) {
			break;
		}
	}
	if ( !error ) {
		if ( !lastWasVal) {
			ARCParser::Error( "trailing operator in #if/#elif" );
			error = 1;
		} else if (parentheses) {
			ARCParser::Error( "too many ( in #if/#elif" );
			error = 1;
		}
	}
	//
	questionMarkVal = false;
	QuestionMarkIntVal = 0;
	questionMarkFloatVal = 0;
	//while there are operators
	while( !error && firstOperator ) {
		v = firstValue;
		for (o = firstOperator; o->next; o = o->next) {
			// if the current operator is nested deeper in parentheses
			//than the next operator
			if (o->parentheses > o->next->parentheses) {
				break;
			}
			// if the current and next operator are nested equally deep in parentheses
			if (o->parentheses == o->next->parentheses) {
				// if the priority of the current operator is equal or higher
				//than the priority of the next operator
				if (o->priority >= o->next->priority) {
					break;
				}
			}
			// if the arity of the operator isn't equal to 1
			if (o->op != P_LOGIC_NOT && o->op != P_BIN_NOT) {
				v = v->next;
			}
			// if there's no value or no next value
			if ( !v) {
				ARCParser::Error( "mising values in #if/#elif" );
				error = 1;
				break;
			}
		}
		if ( error ) {
			break;
		}
		v1 = v;
		v2 = v->next;
#ifdef DEBUG_EVAL
		if (integer) {
			Log_Write( "operator %s, value1 = %d", ARCParser::scriptStack->getPunctuationFromId(o->op), v1->intVal);
			if (v2) Log_Write( "value2 = %d", v2->intVal);
		} else {
			Log_Write( "operator %s, value1 = %f", ARCParser::scriptStack->getPunctuationFromId(o->op), v1->floatVal);
			if (v2) Log_Write( "value2 = %f", v2->floatVal);
		}
#endif //DEBUG_EVAL
		switch(o->op) {
			case P_LOGIC_NOT:		v1->intVal = !v1->intVal;
									v1->floatVal = !v1->floatVal; break;
			case P_BIN_NOT:			v1->intVal = ~v1->intVal;
									break;
			case P_MUL:				v1->intVal *= v2->intVal;
									v1->floatVal *= v2->floatVal; break;
			case P_DIV:				if ( !v2->intVal || !v2->floatVal) {
										ARCParser::Error( "divide by zero in #if/#elif\n" );
										error = 1;
										break;
									}
									v1->intVal /= v2->intVal;
									v1->floatVal /= v2->floatVal; break;
			case P_MOD:				if ( !v2->intVal) {
										ARCParser::Error( "divide by zero in #if/#elif\n" );
										error = 1;
										break;
									}
									v1->intVal %= v2->intVal; break;
			case P_ADD:				v1->intVal += v2->intVal;
									v1->floatVal += v2->floatVal; break;
			case P_SUB:				v1->intVal -= v2->intVal;
									v1->floatVal -= v2->floatVal; break;
			case P_LOGIC_AND:		v1->intVal = v1->intVal && v2->intVal;
									v1->floatVal = v1->floatVal && v2->floatVal; break;
			case P_LOGIC_OR:		v1->intVal = v1->intVal || v2->intVal;
									v1->floatVal = v1->floatVal || v2->floatVal; break;
			case P_LOGIC_GEQ:		v1->intVal = v1->intVal >= v2->intVal;
									v1->floatVal = v1->floatVal >= v2->floatVal; break;
			case P_LOGIC_LEQ:		v1->intVal = v1->intVal <= v2->intVal;
									v1->floatVal = v1->floatVal <= v2->floatVal; break;
			case P_LOGIC_EQ:		v1->intVal = v1->intVal == v2->intVal;
									v1->floatVal = v1->floatVal == v2->floatVal; break;
			case P_LOGIC_UNEQ:		v1->intVal = v1->intVal != v2->intVal;
									v1->floatVal = v1->floatVal != v2->floatVal; break;
			case P_LOGIC_GREATER:	v1->intVal = v1->intVal > v2->intVal;
									v1->floatVal = v1->floatVal > v2->floatVal; break;
			case P_LOGIC_LESS:		v1->intVal = v1->intVal < v2->intVal;
									v1->floatVal = v1->floatVal < v2->floatVal; break;
			case P_RSHIFT:			v1->intVal >>= v2->intVal;
									break;
			case P_LSHIFT:			v1->intVal <<= v2->intVal;
									break;
			case P_BIN_AND:			v1->intVal &= v2->intVal;
									break;
			case P_BIN_OR:			v1->intVal |= v2->intVal;
									break;
			case P_BIN_XOR:			v1->intVal ^= v2->intVal;
									break;
			case P_COLON: {
				if ( !questionMarkVal) {
					ARCParser::Error( ": without ? in #if/#elif" );
					error = 1;
					break;
				}
				if (integer) {
					if ( !QuestionMarkIntVal)
						v1->intVal = v2->intVal;
				} else {
					if ( !questionMarkFloatVal)
						v1->floatVal = v2->floatVal;
				}
				questionMarkVal = false;
				break;
			}
			case P_QUESTIONMARK: {
				if (questionMarkVal) {
					ARCParser::Error( "? after ? in #if/#elif" );
					error = 1;
					break;
				}
				QuestionMarkIntVal = v1->intVal;
				questionMarkFloatVal = v1->floatVal;
				questionMarkVal = true;
				break;
			}
		}
#ifdef DEBUG_EVAL
		if (integer) Log_Write( "result value = %d", v1->intVal);
		else Log_Write( "result value = %f", v1->floatVal);
#endif //DEBUG_EVAL
		if ( error )
			break;
		lastOperatorType = o->op;
		// if not an operator with arity 1
		if (o->op != P_LOGIC_NOT && o->op != P_BIN_NOT) {
			//remove the second value if not question mark operator
			if (o->op != P_QUESTIONMARK) {
				v = v->next;
			}
			//
			if ( v->prev) v->prev->next = v->next;
			else firstValue = v->next;
			if ( v->next) v->next->prev = v->prev;
			else lastValue = v->prev;
			//FreeMemory( v );
			FreeValue( v );
		}
		// remove the operator
		if (o->prev) o->prev->next = o->next;
		else firstOperator = o->next;
		if (o->next) o->next->prev = o->prev;
		else lastOperator = o->prev;
		//FreeMemory(o);
		FreeOperator(o);
	}

	if (firstValue) {
		if ( intVal ) *intVal = firstValue->intVal;
		if ( floatVal)  *floatVal = firstValue->floatVal;
	}
	for (o = firstOperator; o; o = lastOperator) {
		lastOperator = o->next;
		//FreeMemory(o);
		FreeOperator(o);
	}
	for ( v = firstValue; v; v = lastValue) {
		lastValue = v->next;
		//FreeMemory( v );
		FreeValue( v );
	}
	if ( !error ) {
		return true;
	}
	if ( intVal ) {
		*intVal = 0;
	}
	if ( floatVal)  {
		*floatVal = 0;
	}
	return false;
}

/*
================
ARCParser::Evaluate
================
*/
int ARCParser::Evaluate( signed long int *intVal, double *floatVal, int integer ) {
	arcNetToken token, *firstToken, *lastToken;
	arcNetToken *t, *nexttoken;
	define_t *define;
	int defined = false;

	if ( intVal ) {
		*intVal = 0;
	}
	if ( floatVal)  {
		*floatVal = 0;
	}

	if ( !ARCParser::ReadLine( &token ) ) {
		ARCParser::Error( "no value after #if/#elif" );
		return false;
	}
	firstToken = NULL;
	lastToken = NULL;
	do {
		// if the token is a name
		if (token.type == TT_NAME) {
			if ( defined) {
				defined = false;
				t = new arcNetToken( token );
				t->next = NULL;
				if ( lastToken ) lastToken->next = t;
				else firstToken = t;
				lastToken = t;
			} else if ( token == "defined" ) {
				defined = true;
				t = new arcNetToken( token );
				t->next = NULL;
				if ( lastToken ) lastToken->next = t;
				else firstToken = t;
				lastToken = t;
			} else {
				//then it must be a define
				define = FindHashedDefine( ARCParser::defineHash, token.c_str() );
				if ( !define ) {
					ARCParser::Error( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if ( !ARCParser::ExpandDefineIntoSource( &token, define ) ) {
					return false;
				}
			}// if the token is a number or a punctuation
		} else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION) {
			t = new arcNetToken( token );
			t->next = NULL;
			if ( lastToken ) lastToken->next = t;
			else firstToken = t;
			lastToken = t;
		} else {
			ARCParser::Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	} while( ARCParser::ReadLine( &token ) );
	//
	if ( !ARCParser::EvaluateTokens( firstToken, intVal, floatVal, integer ) ) {
		return false;
	}
#ifdef DEBUG_EVAL
	Log_Write( "eval:" );
#endif //DEBUG_EVAL
	for (t = firstToken; t; t = nexttoken) {
#ifdef DEBUG_EVAL
		Log_Write( " %s", t->c_str() );
#endif //DEBUG_EVAL
		nexttoken = t->next;
		delete t;
	} //end for
#ifdef DEBUG_EVAL
	if (integer) Log_Write( "eval result: %d", *intVal);
	else Log_Write( "eval result: %f", *floatVal);
#endif //DEBUG_EVAL
	//
	return true;
}

/*
================
ARCParser::DollarEvaluate
================
*/
int ARCParser::DollarEvaluate( signed long int *intVal, double *floatVal, int integer) {
	int indent, defined = false;
	arcNetToken token, *firstToken, *lastToken;
	arcNetToken *t, *nexttoken;
	define_t *define;

	if ( intVal ) {
		*intVal = 0;
	}
	if ( floatVal)  {
		*floatVal = 0;
	}
	//
	if ( !ARCParser::ReadSourceToken( &token ) ) {
		ARCParser::Error( "no leading ( after $evalint/$evalfloat" );
		return false;
	}
	if ( !ARCParser::ReadSourceToken( &token ) ) {
		ARCParser::Error( "nothing to Evaluate" );
		return false;
	}
	indent = 1;
	firstToken = NULL;
	lastToken = NULL;
	do {
		// if the token is a name
		if (token.type == TT_NAME) {
			if ( defined) {
				defined = false;
				t = new arcNetToken( token );
				t->next = NULL;
				if ( lastToken ) lastToken->next = t;
				else firstToken = t;
				lastToken = t;
			} else if ( token == "defined" ) {
				defined = true;
				t = new arcNetToken( token );
				t->next = NULL;
				if ( lastToken ) lastToken->next = t;
				else firstToken = t;
				lastToken = t;
			} else {
				//then it must be a define
				define = FindHashedDefine( ARCParser::defineHash, token.c_str() );
				if ( !define ) {
					ARCParser::Warning( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if ( !ARCParser::ExpandDefineIntoSource( &token, define ) ) {
					return false;
				}
			}
		} else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION) {
			if ( token[0] == '(' ) indent++;
			else if ( token[0] == ')' ) indent--;
			if (indent <= 0 ) {
				break;
			}
			t = new arcNetToken( token );
			t->next = NULL;
			if ( lastToken ) lastToken->next = t;
			else firstToken = t;
			lastToken = t;
		} else {
			ARCParser::Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	} while( ARCParser::ReadSourceToken( &token ) );
	if ( !ARCParser::EvaluateTokens( firstToken, intVal, floatVal, integer) ) {
		return false;
	}
#ifdef DEBUG_EVAL
	Log_Write( "$eval:" );
#endif //DEBUG_EVAL
	for (t = firstToken; t; t = nexttoken) {
#ifdef DEBUG_EVAL
		Log_Write( " %s", t->c_str() );
#endif //DEBUG_EVAL
		nexttoken = t->next;
		delete t;
	} //end for
#ifdef DEBUG_EVAL
	if (integer) Log_Write( "$eval result: %d", *intVal);
	else Log_Write( "$eval result: %f", *floatVal);
#endif //DEBUG_EVAL
	//
	return true;
}

/*
================
ARCParser::DirectiveElif
================
*/
int ARCParser::DirectiveElif ( void ) {
	signed long int value;
	int type, skip;

	ARCParser::PopIndent( &type, &skip );
	if ( !type || type == INDENT_ELSE) {
		ARCParser::Error( "misplaced #elif" );
		return false;
	}
	if ( !ARCParser::Evaluate( &value, NULL, true ) ) {
		return false;
	}
	skip = (value == 0 );
	ARCParser::PushIndent( INDENT_ELIF, skip );
	return true;
}

/*
================
ARCParser::DirectiveIF
================
*/
int ARCParser::DirectiveIF( void ) {
	signed long int value;
	int skip;

	if ( !ARCParser::Evaluate( &value, NULL, true ) ) {
		return false;
	}
	skip = (value == 0 );
	ARCParser::PushIndent( INDENT_IF, skip );
	return true;
}

/*
================
ARCParser::DirectiveLine
================
*/
int ARCParser::DirectiveLine( void ) {
	arcNetToken token;

	ARCParser::Error( "#line directive not supported" );
	while( ARCParser::ReadLine( &token ) ) {
	}
	return true;
}

/*
================
ARCParser::DirectiveError
================
*/
int ARCParser::DirectiveError( void ) {
	arcNetToken token;

	if ( !ARCParser::ReadLine( &token) || token.type != TT_STRING ) {
		ARCParser::Error( "#error without string" );
		return false;
	}
	ARCParser::Error( "#error: %s", token.c_str() );
	return true;
}

/*
================
ARCParser::DirectiveWarning
================
*/
int ARCParser::DirectiveWarning( void ) {
	arcNetToken token;

	if ( !ARCParser::ReadLine( &token) || token.type != TT_STRING ) {
		ARCParser::Warning( "#warning without string" );
		return false;
	}
	ARCParser::Warning( "#warning: %s", token.c_str() );
	return true;
}

/*
================
ARCParser::Directive_pragma
================
*/
int ARCParser::DirectivePragma( void ) {
	arcNetToken token;

	ARCParser::Warning( "#pragma directive not supported" );
	while( ARCParser::ReadLine( &token ) ) {
	}
	return true;
}

/*
================
ARCParser::UnreadSignToken
================
*/
void ARCParser::UnreadSignToken( void ) {
	arcNetToken token;

	token.line = ARCParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	token = "-";
	token.type = TT_PUNCTUATION;
	token.subtype = P_SUB;
	ARCParser::UnreadSourceToken( &token );
}

/*
================
ARCParser::DirectiveEval
================
*/
int ARCParser::DirectiveEval( void ) {
	signed long int value;
	arcNetToken token;
	char buf[128];

	if ( !ARCParser::Evaluate( &value, NULL, true ) ) {
		return false;
	}

	token.line = ARCParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf(buf, "%d", abs(value) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER|TT_LONG|TT_DECIMAL;
	ARCParser::UnreadSourceToken( &token );
	if ( value < 0 ) {
		ARCParser::UnreadSignToken();
	}
	return true;
}

/*
================
ARCParser::DirectiveEvalfloat
================
*/
int ARCParser::DirectiveEvalfloat( void ) {
	double value;
	arcNetToken token;
	char buf[128];

	if ( !ARCParser::Evaluate( NULL, &value, false ) ) {
		return false;
	}

	token.line = ARCParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf(buf, "%1.2f", arcMath::Fabs(value) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT|TT_LONG|TT_DECIMAL;
	ARCParser::UnreadSourceToken( &token );
	if (value < 0 ) {
		ARCParser::UnreadSignToken();
	}
	return true;
}

/*
================
ARCParser::ReadDirective
================
*/
int ARCParser::ReadDirective( void ) {
	arcNetToken token;

	// read the directive name
	if ( !ARCParser::ReadSourceToken( &token ) ) {
		ARCParser::Error( "found '#' without name" );
		return false;
	}
	// directive name must be on the same line
	if (token.linesCrossed > 0 ) {
		ARCParser::UnreadSourceToken( &token );
		ARCParser::Error( "found '#' at end of line" );
		return false;
	}
	// if if is a name
	if (token.type == TT_NAME) {
		if ( token == "if" ) {
			return ARCParser::DirectiveIF();
		}
		else if ( token == "ifdef" ) {
			return ARCParser::DirectiveIFDEF();
		}
		else if ( token == "ifndef" ) {
			return ARCParser::DirectiveIFndef();
		}
		else if ( token == "elif" ) {
			return ARCParser::DirectiveElif ();
		}
		else if ( token == "else" ) {
			return ARCParser::Directive_else();
		}
		else if ( token == "endif" ) {
			return ARCParser::Directive_endif ();
		}
		else if ( ARCParser::skip > 0 ) {
			// skip the rest of the line
			while( ARCParser::ReadLine( &token ) ) {
			}
			return true;
		}
		else {
			if ( token == "include" ) {
				return ARCParser::DirectiveInclude();
			}
			else if ( token == "define" ) {
				return ARCParser::DirectiveDefine();
			}
			else if ( token == "undef" ) {
				return ARCParser::DirectiveUNdef();
			}
			else if ( token == "line" ) {
				return ARCParser::DirectiveLine();
			}
			else if ( token == "error" ) {
				return ARCParser::DirectiveError();
			}
			else if ( token == "warning" ) {
				return ARCParser::DirectiveWarning();
			}
			else if ( token == "pragma" ) {
				return ARCParser::Directive_pragma();
			}
			else if ( token == "eval" ) {
				return ARCParser::DirectiveEval();
			}
			else if ( token == "evalfloat" ) {
				return ARCParser::DirectiveEvalfloat();
			}
		}
	}
	ARCParser::Error( "unknown precompiler directive '%s'", token.c_str() );
	return false;
}

/*
================
ARCParser::DollarDirectiveEvalInt
================
*/
int ARCParser::DollarDirectiveEvalInt( void ) {
	signed long int value;
	arcNetToken token;
	char buf[128];

	if ( !ARCParser::DollarEvaluate( &value, NULL, true ) ) {
		return false;
	}

	token.line = ARCParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%d", abs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intVal = abs( value );
	token.floatVal = abs( value );
	ARCParser::UnreadSourceToken( &token );
	if ( value < 0 ) {
		ARCParser::UnreadSignToken();
	}
	return true;
}

/*
================
ARCParser::DollarDirectiveEvalfloat
================
*/
int ARCParser::DollarDirectiveEvalfloat( void ) {
	double value;
	arcNetToken token;
	char buf[128];

	if ( !ARCParser::DollarEvaluate( NULL, &value, false ) ) {
		return false;
	}

	token.line = ARCParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%1.2f", fabs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intVal = (unsigned long) fabs( value );
	token.floatVal = fabs( value );
	ARCParser::UnreadSourceToken( &token );
	if ( value < 0 ) {
		ARCParser::UnreadSignToken();
	}
	return true;
}

/*
================
ARCParser::ReadDollarDirective
================
*/
int ARCParser::ReadDollarDirective( void ) {
	arcNetToken token;

	// read the directive name
	if ( !ARCParser::ReadSourceToken( &token ) ) {
		ARCParser::Error( "found '$' without name" );
		return false;
	}
	// directive name must be on the same line
	if ( token.linesCrossed > 0 ) {
		ARCParser::UnreadSourceToken( &token );
		ARCParser::Error( "found '$' at end of line" );
		return false;
	}
	// if if is a name
	if (token.type == TT_NAME) {
		if ( token == "evalint" ) {
			return ARCParser::DollarDirectiveEvalInt();
		}
		else if ( token == "evalfloat" ) {
			return ARCParser::DollarDirectiveEvalfloat();
		}
	}
	ARCParser::UnreadSourceToken( &token );
	return false;
}

/*
================
ARCParser::ReadToken
================
*/
int ARCParser::ReadToken( arcNetToken *token ) {
	define_t *define;

	while(1 ) {
		if ( !ARCParser::ReadSourceToken( token ) ) {
			return false;
		}
		// check for precompiler directives
		if ( token->type == TT_PUNCTUATION && (*token)[0] == '#' && (*token)[1] == '\0' ) {
			// read the precompiler directive
			if ( !ARCParser::ReadDirective() ) {
				return false;
			}
			continue;
		}
		// if skipping source because of conditional compilation
		if ( ARCParser::skip ) {
			continue;
		}
		// recursively concatenate strings that are behind each other still resolving defines
		if ( token->type == TT_STRING && !( ARCParser::scriptStack->GetFlags() & LEXFL_NOSTRINGCONCAT) ) {
			arcNetToken newToken;
			if ( ARCParser::ReadToken( &newToken ) ) {
				if ( newToken.type == TT_STRING ) {
					token->Append( newToken.c_str() );
				}
				else {
					ARCParser::UnreadSourceToken( &newToken );
				}
			}
		}
		//
		if ( !( ARCParser::scriptStack->GetFlags() & LEXFL_NODOLLARPRECOMPILE) ) {
			// check for special precompiler directives
			if ( token->type == TT_PUNCTUATION && (*token)[0] == '$' && (*token)[1] == '\0' ) {
				// read the precompiler directive
				if ( ARCParser::ReadDollarDirective() ) {
					continue;
				}
			}
		}
		// if the token is a name
		if ( token->type == TT_NAME && !( token->flags & TOKEN_FL_RECURSIVE_DEFINE ) ) {
			// check if the name is a define macro
			define = FindHashedDefine( ARCParser::defineHash, token->c_str() );
			// if it is a define macro
			if ( define ) {
				// expand the defined macro
				if ( !ARCParser::ExpandDefineIntoSource( token, define ) ) {
					return false;
				}
				continue;
			}
		}
		// found a token
		return true;
	}
}

/*
================
ARCParser::ExpectTokenString
================
*/
int ARCParser::ExpectTokenString( const char *string ) {
	arcNetToken token;

	if ( !ARCParser::ReadToken( &token ) ) {
		ARCParser::Error( "couldn't find expected '%s'", string );
		return false;
	}

	if ( token != string ) {
		ARCParser::Error( "expected '%s' but found '%s'", string, token.c_str() );
		return false;
	}
	return true;
}

/*
================
ARCParser::ExpectTokenType
================
*/
int ARCParser::ExpectTokenType( int type, int subtype, arcNetToken *token ) {
	arcNetString str;

	if ( !ARCParser::ReadToken( token ) ) {
		ARCParser::Error( "couldn't read expected token" );
		return 0;
	}

	if ( token->type != type ) {
		switch( type ) {
			case TT_STRING: str = "string"; break;
			case TT_LITERAL: str = "literal"; break;
			case TT_NUMBER: str = "number"; break;
			case TT_NAME: str = "name"; break;
			case TT_PUNCTUATION: str = "punctuation"; break;
			default: str = "unknown type"; break;
		}
		ARCParser::Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
		return 0;
	}
	if ( token->type == TT_NUMBER ) {
		if ( (token->subtype & subtype) != subtype ) {
			str.Clear();
			if ( subtype & TT_DECIMAL ) str = "decimal ";
			if ( subtype & TT_HEX ) str = "hex ";
			if ( subtype & TT_OCTAL ) str = "octal ";
			if ( subtype & TT_BINARY ) str = "binary ";
			if ( subtype & TT_UNSIGNED ) str += "unsigned ";
			if ( subtype & TT_LONG ) str += "long ";
			if ( subtype & TT_FLOAT ) str += "float ";
			if ( subtype & TT_INTEGER ) str += "integer ";
			str.StripTrailing( ' ' );
			ARCParser::Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if ( token->type == TT_PUNCTUATION ) {
		if ( subtype < 0 ) {
			ARCParser::Error( "BUG: wrong punctuation subtype" );
			return 0;
		}
		if ( token->subtype != subtype ) {
			ARCParser::Error( "expected '%s' but found '%s'", scriptStack->GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}

/*
================
ARCParser::ExpectAnyToken
================
*/
int ARCParser::ExpectAnyToken( arcNetToken *token ) {
	if ( !ARCParser::ReadToken( token ) ) {
		ARCParser::Error( "couldn't read expected token" );
		return false;
	}
	else {
		return true;
	}
}

/*
================
ARCParser::CheckTokenString
================
*/
int ARCParser::CheckTokenString( const char *string ) {
	arcNetToken tok;

	if ( !ReadToken( &tok ) ) {
		return false;
	}
	// if the token is available
	if ( tok == string ) {
		return true;
	}

	UnreadSourceToken( &tok );
	return false;
}

/*
================
ARCParser::CheckTokenType
================
*/
int ARCParser::CheckTokenType( int type, int subtype, arcNetToken *token ) {
	arcNetToken tok;

	if ( !ReadToken( &tok ) ) {
		return false;
	}
	// if the type matches
	if (tok.type == type && (tok.subtype & subtype) == subtype) {
		*token = tok;
		return true;
	}

	UnreadSourceToken( &tok );
	return false;
}

/*
================
ARCParser::PeekTokenString
================
*/
int ARCParser::PeekTokenString( const char *string ) {
	arcNetToken tok;

	if ( !ReadToken( &tok ) ) {
		return false;
	}

	UnreadSourceToken( &tok );

	// if the token is available
	if ( tok == string ) {
		return true;
	}
	return false;
}

/*
================
ARCParser::PeekTokenType
================
*/
int ARCParser::PeekTokenType( int type, int subtype, arcNetToken *token ) {
	arcNetToken tok;

	if ( !ReadToken( &tok ) ) {
		return false;
	}

	UnreadSourceToken( &tok );

	// if the type matches
	if ( tok.type == type && ( tok.subtype & subtype ) == subtype ) {
		*token = tok;
		return true;
	}
	return false;
}

/*
================
ARCParser::SkipUntilString
================
*/
int ARCParser::SkipUntilString( const char *string ) {
	arcNetToken token;

	while( ARCParser::ReadToken( &token ) ) {
		if ( token == string ) {
			return true;
		}
	}
	return false;
}

/*
================
ARCParser::SkipRestOfLine
================
*/
int ARCParser::SkipRestOfLine( void ) {
	arcNetToken token;

	while( ARCParser::ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			ARCParser::UnreadSourceToken( &token );
			return true;
		}
	}
	return false;
}

/*
=================
ARCParser::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int ARCParser::SkipBracedSection( bool parseFirstBrace ) {
	arcNetToken token;
	int depth;

	depth = parseFirstBrace ? 0 : 1;
	do {
		if ( !ReadToken( &token ) ) {
			return false;
		}
		if ( token.type == TT_PUNCTUATION ) {
			if ( token == "{" ) {
				depth++;
			} else if ( token == "}" ) {
				depth--;
			}
		}
	} while( depth );
	return true;
}

/*
=================
ARCParser::ParseBracedSectionExact

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains the exact formating of the braced section

  FIXME: what about precompilation ?
=================
*/
const char *ARCParser::ParseBracedSectionExact( arcNetString &out, int tabs ) {
	return scriptStack->ParseBracedSectionExact( out, tabs );
}

/*
=================
ARCParser::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
const char *ARCParser::ParseBracedSection( arcNetString &out, int tabs ) {
	arcNetToken token;
	int i, depth;
	bool doTabs = false;
	if (tabs >= 0 ) {
		doTabs = true;
	}

	out.Empty();
	if ( !ARCParser::ExpectTokenString( "{" ) ) {
		return out.c_str();
	}
	out = "{";
	depth = 1;
	do {
		if ( !ARCParser::ReadToken( &token ) ) {
			Error( "missing closing brace" );
			return out.c_str();
		}

		// if the token is on a new line
		for ( i = 0; i < token.linesCrossed; i++ ) {
			out += "\r\n";
		}

		if (doTabs && token.linesCrossed) {
			i = tabs;
			if (token[0] == '}' && i > 0 ) {
				i--;
			}
			while ( i-- > 0 ) {
				out += "\t";
			}
		}
		if ( token.type == TT_PUNCTUATION ) {
			if ( token[0] == '{' ) {
				depth++;
				if (doTabs) {
					tabs++;
				}
			}
			else if ( token[0] == '}' ) {
				depth--;
				if (doTabs) {
					tabs--;
				}
			}
		}

		if ( token.type == TT_STRING ) {
			out += "\"" + token + "\"";
		}
		else {
			out += token;
		}
		out += " ";
	} while( depth );

	return out.c_str();
}

/*
=================
ARCParser::ParseRestOfLine

  parse the rest of the line
=================
*/
const char *ARCParser::ParseRestOfLine( arcNetString &out ) {
	arcNetToken token;

	out.Empty();
	while( ARCParser::ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			ARCParser::UnreadSourceToken( &token );
			break;
		}
		if ( out.Length() ) {
			out += " ";
		}
		out += token;
	}
	return out.c_str();
}

/*
================
ARCParser::UnreadToken
================
*/
void ARCParser::UnreadToken( arcNetToken *token ) {
	ARCParser::UnreadSourceToken( token );
}

/*
================
ARCParser::ReadTokenOnLine
================
*/
int ARCParser::ReadTokenOnLine( arcNetToken *token ) {
	arcNetToken tok;

	if ( !ARCParser::ReadToken( &tok ) ) {
		return false;
	}
	// if no lines were crossed before this token
	if ( !tok.linesCrossed ) {
		*token = tok;
		return true;
	}
	//
	ARCParser::UnreadSourceToken( &tok );
	return false;
}

/*
================
ARCParser::ParseInt
================
*/
int ARCParser::ParseInt( void ) {
	arcNetToken token;

	if ( !ARCParser::ReadToken( &token ) ) {
		ARCParser::Error( "couldn't read expected integer" );
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		ARCParser::ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -((signed int) token.GetIntValue() );
	}
	else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
		ARCParser::Error( "expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
ARCParser::ParseBool
================
*/
bool ARCParser::ParseBool( void ) {
	arcNetToken token;

	if ( !ARCParser::ExpectTokenType( TT_NUMBER, 0, &token ) ) {
		ARCParser::Error( "couldn't read expected boolean" );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}

/*
================
ARCParser::ParseFloat
================
*/
float ARCParser::ParseFloat( void ) {
	arcNetToken token;

	if ( !ARCParser::ReadToken( &token ) ) {
		ARCParser::Error( "couldn't read expected floating point number" );
		return 0.0f;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		ARCParser::ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	}
	else if ( token.type != TT_NUMBER ) {
		ARCParser::Error( "expected float value, found '%s'", token.c_str() );
	}
	return token.GetFloatValue();
}

/*
================
ARCParser::Parse1DMatrix
================
*/
int ARCParser::Parse1DMatrix( int x, float *m ) {
	int i;

	if ( !ARCParser::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( i = 0; i < x; i++ ) {
		m[i] = ARCParser::ParseFloat();
	}

	if ( !ARCParser::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
ARCParser::Parse2DMatrix
================
*/
int ARCParser::Parse2DMatrix( int y, int x, float *m ) {
	int i;

	if ( !ARCParser::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( i = 0; i < y; i++ ) {
		if ( !ARCParser::Parse1DMatrix( x, m + i * x ) ) {
			return false;
		}
	}

	if ( !ARCParser::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
ARCParser::Parse3DMatrix
================
*/
int ARCParser::Parse3DMatrix( int z, int y, int x, float *m ) {
	int i;

	if ( !ARCParser::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( i = 0; i < z; i++ ) {
		if ( !ARCParser::Parse2DMatrix( y, x, m + i * x*y ) ) {
			return false;
		}
	}

	if ( !ARCParser::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
ARCParser::GetLastWhiteSpace
================
*/
int ARCParser::GetLastWhiteSpace( arcNetString &whiteSpace ) const {
	if ( scriptStack ) {
		scriptStack->GetLastWhiteSpace( whiteSpace );
	} else {
		whiteSpace.Clear();
	}
	return whiteSpace.Length();
}

/*
================
ARCParser::SetMarker
================
*/
void ARCParser::SetMarker( void ) {
	marker_p = NULL;
}

/*
================
ARCParser::GetStringFromMarker

  FIXME: this is very bad code, the script isn't even garrenteed to still be around
================
*/
void ARCParser::GetStringFromMarker( arcNetString& out, bool clean ) {
	char*	p;
	char	save;

	if ( marker_p == NULL ) {
		marker_p = scriptStack->buffer;
	}

	if ( tokens ) {
		p = (char*)tokens->whiteSpaceStart_p;
	} else {
		p = (char*)scriptStack->script_p;
	}

	// Set the end character to NULL to give us a complete string
	save = *p;
	*p = 0;

	// If cleaning then reparse
	if ( clean ) {
		ARCParser temp( marker_p, strlen( marker_p ), "temp", flags );
		arcNetToken token;
		while ( temp.ReadToken ( &token ) ) {
			out += token;
		}
	} else {
		out = marker_p;
	}

	// restore the character we set to NULL
	*p = save;
}

/*
================
ARCParser::SetIncludePath
================
*/
void ARCParser::SetIncludePath( const char *path ) {
	ARCParser::includepath = path;
	// add trailing path seperator
	if ( ARCParser::includepath[ARCParser::includepath.Length()-1] != '\\' &&
		ARCParser::includepath[ARCParser::includepath.Length()-1] != '/') {
		ARCParser::includepath += PATHSEPERATOR_STR;
	}
}

/*
================
ARCParser::SetPunctuations
================
*/
void ARCParser::SetPunctuations( const punctuation_t *p ) {
	ARCParser::punctuations = p;
}

/*
================
ARCParser::SetFlags
================
*/
void ARCParser::SetFlags( int flags ) {
	arcLexer *s;

	ARCParser::flags = flags;
	for ( s = ARCParser::scriptStack; s; s = s->next ) {
		s->SetFlags( flags );
	}
}

/*
================
ARCParser::GetFlags
================
*/
int ARCParser::GetFlags( void ) const {
	return ARCParser::flags;
}

/*
================
ARCParser::LoadFile
================
*/
int ARCParser::LoadFile( const char *filename, bool OSPath ) {
	arcLexer *script;

	if ( ARCParser::loaded ) {
		arcLibrary::common->FatalError( "ARCParser::loadFile: another source already loaded" );
		return false;
	}
	script = new arcLexer( filename, 0, OSPath );
	if ( !script->IsLoaded() ) {
		delete script;
		return false;
	}
	script->SetFlags( ARCParser::flags );
	script->SetPunctuations( ARCParser::punctuations );
	script->next = NULL;
	ARCParser::OSPath = OSPath;
	ARCParser::filename = filename;
	ARCParser::scriptStack = script;
	ARCParser::tokens = NULL;
	ARCParser::indentStack = NULL;
	ARCParser::skip = 0;
	ARCParser::loaded = true;

	if ( !ARCParser::defineHash ) {
		ARCParser::defines = NULL;
		ARCParser::defineHash = ( define_t **) Mem_ClearedAlloc( DEFINEHASHSIZE * sizeof( define_t *) );
		ARCParser::AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
ARCParser::LoadMemory
================
*/
int ARCParser::LoadMemory(const char *ptr, int length, const char *name ) {
	arcLexer *script;

	if ( ARCParser::loaded ) {
		arcLibrary::common->FatalError( "ARCParser::loadMemory: another source already loaded" );
		return false;
	}
	script = new arcLexer( ptr, length, name );
	if ( !script->IsLoaded() ) {
		delete script;
		return false;
	}
	script->SetFlags( ARCParser::flags );
	script->SetPunctuations( ARCParser::punctuations );
	script->next = NULL;
	ARCParser::filename = name;
	ARCParser::scriptStack = script;
	ARCParser::tokens = NULL;
	ARCParser::indentStack = NULL;
	ARCParser::skip = 0;
	ARCParser::loaded = true;

	if ( !ARCParser::defineHash ) {
		ARCParser::defines = NULL;
		ARCParser::defineHash = ( define_t **) Mem_ClearedAlloc( DEFINEHASHSIZE * sizeof( define_t *) );
		ARCParser::AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
ARCParser::FreeSource
================
*/
void ARCParser::FreeSource( bool keepDefines ) {
	arcLexer *script;
	arcNetToken *token;
	define_t *define;
	indent_t *indent;
	int i;

	// free all the scripts
	while( scriptStack ) {
		script = scriptStack;
		scriptStack = scriptStack->next;
		delete script;
	}
	// free all the tokens
	while( tokens ) {
		token = tokens;
		tokens = tokens->next;
		delete token;
	}
	// free all indents
	while( indentStack ) {
		indent = indentStack;
		indentStack = indentStack->next;
		Mem_Free( indent );
	}
	if ( !keepDefines ) {
		// free hash table
		if ( defineHash ) {
			// free defines
			for ( i = 0; i < DEFINEHASHSIZE; i++ ) {
				while( defineHash[i] ) {
					define = defineHash[i];
					defineHash[i] = defineHash[i]->hashNext;
					FreeDefine( define );
				}
			}
			defines = NULL;
			Mem_Free( ARCParser::defineHash );
			defineHash = NULL;
		}
	}
	loaded = false;
}

/*
================
ARCParser::GetPunctuationFromId
================
*/
const char *ARCParser::GetPunctuationFromId( int id ) {
	int i;

	if ( !ARCParser::punctuations ) {
		arcLexer lex;
		return lex.GetPunctuationFromId( id );
	}

	for ( i = 0; ARCParser::punctuations[i].p; i++ ) {
		if ( ARCParser::punctuations[i].n == id ) {
			return ARCParser::punctuations[i].p;
		}
	}
	return "unkown punctuation";
}

/*
================
ARCParser::GetPunctuationId
================
*/
int ARCParser::GetPunctuationId( const char *p ) {
	int i;

	if ( !ARCParser::punctuations ) {
		arcLexer lex;
		return lex.GetPunctuationId( p );
	}

	for ( i = 0; ARCParser::punctuations[i].p; i++ ) {
		if ( !strcmp( ARCParser::punctuations[i].p, p) ) {
			return ARCParser::punctuations[i].n;
		}
	}
	return 0;
}

/*
================
ARCParser::ARCParser
================
*/
ARCParser::ARCParser() {
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = 0;
	this->scriptStack = NULL;
	this->indentStack = NULL;
	this->defineHash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
}

/*
================
ARCParser::ARCParser
================
*/
ARCParser::ARCParser( int flags ) {
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptStack = NULL;
	this->indentStack = NULL;
	this->defineHash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
}

/*
================
ARCParser::ARCParser
================
*/
ARCParser::ARCParser( const char *filename, int flags, bool OSPath ) {
	this->loaded = false;
	this->OSPath = true;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptStack = NULL;
	this->indentStack = NULL;
	this->defineHash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
	LoadFile( filename, OSPath );
}

/*
================
ARCParser::ARCParser
================
*/
ARCParser::ARCParser( const char *ptr, int length, const char *name, int flags ) {
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptStack = NULL;
	this->indentStack = NULL;
	this->defineHash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
	LoadMemory( ptr, length, name );
}

/*
================
ARCParser::~ARCParser
================
*/
ARCParser::~ARCParser( void ) {
	ARCParser::FreeSource( false );
}


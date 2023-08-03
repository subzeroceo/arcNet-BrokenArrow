#include "Lib.h"
#pragma hdrstop

//#define DEBUG_EVAL
#define MAX_DEFINEPARMS				128
#define DEFINEHASHSIZE				2048

#define TOKEN_FL_RECURSIVE_DEFINE	1

define_t * anParser::globalDefines;

/*
================
anParser::SetBaseFolder
================
*/
void anParser::SetBaseFolder( const char *path ) {
	anLexer::SetBaseFolder( path );
}

/*
================
anParser::AddGlobalDefine
================
*/
int anParser::AddGlobalDefine( const char *string ) {
	define_t *define;

	define = anParser::DefineFromString( string );
	if ( !define ) {
		return false;
	}
	define->next = globalDefines;
	globalDefines = define;
	return true;
}

/*
================
anParser::RemoveGlobalDefine
================
*/
int anParser::RemoveGlobalDefine( const char *name ) {
        for ( define_t *prev = nullptr, d = anParser::globalDefines; d;
             prev = d, d = d->next ) {
          if ( !strcmp( d->name, name ) ) {
            break;
          }
        }
        if ( define_t * d ) {
          if ( prev ) {
            prev->next = d->next;
          } else {
            anParser::globalDefines = d->next;
          }
          anParser::FreeDefine( d );
          return true;
        }
        return false;
}

/*
================
anParser::RemoveAllGlobalDefines
================
*/
void anParser::RemoveAllGlobalDefines( void ) {
	define_t *define;

	for ( define = globalDefines; define; define = globalDefines ) {
		globalDefines = globalDefines->next;
		anParser::FreeDefine( define );
	}
}


/*
===============================================================================

anParser

===============================================================================
*/

/*
================
anParser::PrintDefine
================
*/
void anParser::PrintDefine( define_t *define ) {
	anLibrary::common->Printf( "define->name = %s\n", define->name);
	anLibrary::common->Printf( "define->flags = %d\n", define->flags);
	anLibrary::common->Printf( "define->builtin = %d\n", define->builtin);
	anLibrary::common->Printf( "define->numParms = %d\n", define->numParms);
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
anParser::AddDefineToHash
================
*/
void anParser::AddDefineToHash( define_t *define, define_t **defineHash ) {
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
define_t *anParser::FindHashedDefine( define_t **defineHash, const char *name ) {
	define_t *d;
	int hash;

	hash = PC_NameHash( name );
	for ( d = defineHash[hash]; d; d = d->hashNext ) {
		if ( !strcmp( d->name, name ) ) {
			return d;
		}
	}
	return nullptr;
}

/*
================
anParser::FindDefine
================
*/
define_t *anParser::FindDefine( define_t *defines, const char *name ) {
	define_t *d;

	for ( d = defines; d; d = d->next ) {
		if ( !strcmp( d->name, name ) ) {
			return d;
		}
	}
	return nullptr;
}

/*
================
anParser::FindDefineParm
================
*/
int anParser::FindDefineParm( define_t *define, const char *name ) {
	anToken *p;
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
anParser::CopyDefine
================
*/
define_t *anParser::CopyDefine( define_t *define ) {
	anToken *token, *newToken, *lastToken;

	define_t *newDefine = ( define_t *) Mem_Alloc( sizeof( define_t ) + strlen( define->name ) + 1 );

	// copy the define name
	newDefine->name = (char *) newDefine + sizeof( define_t );
	strcpy( newDefine->name, define->name );

	newDefine->flags = define->flags;
	newDefine->builtin = define->builtin;
	newDefine->numParms = define->numParms;

	// the define is not linked
	newDefine->next = nullptr;
	newDefine->hashNext = nullptr;

	// copy the define tokens
	newDefine->tokens = nullptr;

	for ( anToken *lastToken = nullptr, token = define->tokens; token; token = token->next ) {
		newToken = new anToken( token );
		newToken->next = nullptr;
		if ( lastToken ) lastToken->next = newToken;
		else newDefine->tokens = newToken;
		lastToken = newToken;
	}
	// copy the define parameters
	newDefine->parms = nullptr;
	for ( lastToken = nullptr, token = define->parms; token; token = token->next ) {
		newToken = new anToken( token );
		newToken->next = nullptr;
		if ( lastToken ) lastToken->next = newToken;
		else newDefine->parms = newToken;
		lastToken = newToken;
	}
	return newDefine;
}

/*
================
anParser::FreeDefine
================
*/
void anParser::FreeDefine( define_t *define ) {
	// free the define parameters
	for ( anToken *t = define->parms; t; t = next ) {
		anToken *next = t->next;
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
anParser::DefineFromString
================
*/
define_t *anParser::DefineFromString( const char *string ) {
	anParser src;
	define_t *def;

	if ( !src.LoadMemory( string, strlen( string ), "*defineString" ) ) {
		return nullptr;
	}
	// create a define from the source
	if ( !src.DirectiveDefine() ) {
		src.FreeSource();
		return nullptr;
	}
	def = src.CopyFirstDefine();
	src.FreeSource();
	// if the define was created succesfully
	return def;
}

/*
================
anParser::Error
================
*/
void anParser::Error( const char *str, ... ) const {
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);
	if ( anParser::scriptStack ) {
		anParser::scriptStack->Error( text );
	}
}

/*
================
anParser::Warning
================
*/
void anParser::Warning( const char *str, ... ) const {
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);
	if ( anParser::scriptStack ) {
		anParser::scriptStack->Warning( text );
	}
}

/*
================
anParser::PushIndent
================
*/
void anParser::PushIndent( int type, int skip ) {
	indent_t *indent = (indent_t *) Mem_Alloc( sizeof(indent_t) );
	indent->type = type;
	indent->script = anParser::scriptStack;
	indent->skip = ( skip != 0 );
	anParser::skip += indent->skip;
	indent->next = anParser::indentStack;
	anParser::indentStack = indent;
}

/*
================
anParser::PopIndent
================
*/
void anParser::PopIndent( int *type, int *skip ) {
	indent_t *indent;

	*type = 0;
	*skip = 0;

	indent = anParser::indentStack;
	if ( !indent) return;

	// must be an indent from the current script
	if ( anParser::indentStack->script != anParser::scriptStack) {
		return;
	}

	*type = indent->type;
	*skip = indent->skip;
	anParser::indentStack = anParser::indentStack->next;
	anParser::skip -= indent->skip;
	Mem_Free( indent );
}

/*
================
anParser::PushScript
================
*/
void anParser::PushScript( anLexer *script ) {
	anLexer *s;

	for ( s = anParser::scriptStack; s; s = s->next ) {
		if ( !anString::Icmp( s->GetFileName(), script->GetFileName() ) ) {
			anParser::Warning( "'%s' recursively included", script->GetFileName() );
			return;
		}
	}
	//push the script on the script stack
	script->next = anParser::scriptStack;
	anParser::scriptStack = script;
}

/*
================
anParser::ReadSourceToken
================
*/
int anParser::ReadSourceToken( anToken *token ) {
	anToken *t;
	anLexer *script;
	int type, skip, changedScript;

	if ( !anParser::scriptStack ) {
		anLibrary::common->FatalError( "anParser::ReadSourceToken: not loaded" );
		return false;
	}
	changedScript = 0;
	// if there's no token already available
	while( !anParser::tokens ) {
		// if there's a token to read from the script
		if ( anParser::scriptStack->ReadToken( token ) ) {
			token->linesCrossed += changedScript;

			// set the marker based on the start of the token read in
			if ( !marker_p ) {
				marker_p = token->whiteSpaceEnd_p;
			}
			return true;
		}
		// if at the end of the script
		if ( anParser::scriptStack->EndOfFile() ) {
			// remove all indents of the script
			while( anParser::indentStack && anParser::indentStack->script == anParser::scriptStack ) {
				anParser::Warning( "missing #endif" );
				anParser::PopIndent( &type, &skip );
			}
			changedScript = 1;
		}
		// if this was the initial script
		if ( !anParser::scriptStack->next ) {
			return false;
		}
		// remove the script and return to the previous one
		script = anParser::scriptStack;
		anParser::scriptStack = anParser::scriptStack->next;
		delete script;
	}
	// copy the already available token
	*token = anParser::tokens;
	// remove the token from the source
	t = anParser::tokens;
	anParser::tokens = anParser::tokens->next;
	delete t;
	return true;
}

/*
================
anParser::UnreadSourceToken
================
*/
int anParser::UnreadSourceToken( anToken *token ) {
	anToken *t;

	t = new anToken( token );
	t->next = anParser::tokens;
	anParser::tokens = t;
	return true;
}

/*
================
anParser::ReadDefineParms
================
*/
int anParser::ReadDefineParms( define_t *define, anToken **parms, int maxParms ) {
	define_t *newDefine;
	anToken token, *t, *last;
	int i, done, lastcomma, numParms, indent;

	if ( !anParser::ReadSourceToken( &token ) ) {
		anParser::Error( "define '%s' missing parameters", define->name );
		return false;
	}

	if ( define->numParms > maxParms ) {
		anParser::Error( "define with more than %d parameters", maxParms );
		return false;
	}

	for ( i = 0; i < define->numParms; i++ ) {
		parms[i] = nullptr;
	}
	// if no leading "( "
	if ( token != "( " ) {
		anParser::UnreadSourceToken( &token );
		anParser::Error( "define '%s' missing parameters", define->name );
		return false;
	}
	// read the define parameters
	for ( done = 0, numParms = 0, indent = 1; !done; ) {
		if ( numParms >= maxParms ) {
			anParser::Error( "define '%s' with too many parameters", define->name );
			return false;
		}
		parms[numParms] = nullptr;
		lastcomma = 1;
		last = nullptr;
		while( !done ) {

			if ( !anParser::ReadSourceToken( &token ) ) {
				anParser::Error( "define '%s' incomplete", define->name );
				return false;
			}

			if ( token == "," ) {
				if ( indent <= 1 ) {
					if ( lastcomma ) {
						anParser::Warning( "too many comma's" );
					}
					if ( numParms >= define->numParms ) {
						anParser::Warning( "too many define parameters" );
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
						anParser::Warning( "too few define parameters" );
					}
					done = 1;
					break;
				}
			} else if ( token.type == TT_NAME ) {
				newDefine = FindHashedDefine( anParser::defineHash, token.c_str() );
				if ( newDefine ) {
					if ( !anParser::ExpandDefineIntoSource( &token, newDefine ) ) {
						return false;
					}
					continue;
				}
			}

			lastcomma = 0;

			if ( numParms < define->numParms ) {

				t = new anToken( token );
				t->next = nullptr;
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
anParser::StringizeTokens
================
*/
int anParser::StringizeTokens( anToken *tokens, anToken *token ) {
	anToken *t;

	token->type = TT_STRING;
	token->whiteSpaceStart_p = nullptr;
	token->whiteSpaceEnd_p = nullptr;
	(*token) = "";
	for ( t = tokens; t; t = t->next ) {
		token->Append( t->c_str() );
	}
	return true;
}

/*
================
anParser::MergeTokens
================
*/
int anParser::MergeTokens( anToken *t1, anToken *t2 ) {
	// merging of a name with a name or number
	if ( t1->type == TT_NAME && (t2->type == TT_NAME || (t2->type == TT_NUMBER && !(t2->subtype & TT_FLOAT) ) ) ) {
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
anParser::AddBuiltinDefines
================
*/
void anParser::AddBuiltinDefines( void ) {
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
		{ nullptr, 0 }
	};

	for ( i = 0; builtin[i].string; i++ ) {
		define = ( define_t *) Mem_Alloc( sizeof( define_t ) + strlen(builtin[i].string ) + 1 );
		define->name = (char *) define + sizeof( define_t );
		strcpy( define->name, builtin[i].string );
		define->flags = DEFINE_FIXED;
		define->builtin = builtin[i].id;
		define->numParms = 0;
		define->parms = nullptr;
		define->tokens = nullptr;
		// add the define to the source
		AddDefineToHash( define, anParser::defineHash );
	}
}

/*
================
anParser::CopyFirstDefine
================
*/
define_t *anParser::CopyFirstDefine( void ) {
	int i;

	for ( i = 0; i < DEFINEHASHSIZE; i++ ) {
		if ( anParser::defineHash[i] ) {
			return CopyDefine( anParser::defineHash[i] );
		}
	}
	return nullptr;
}

/*
================
anParser::ExpandBuiltinDefine
================
*/
int anParser::ExpandBuiltinDefine( anToken *defToken, define_t *define, anToken **firstToken, anToken **lastToken ) {
	anToken *token;
	ARC_TIME_T t;
	char *curtime;
	char buf[MAX_STRING_CHARS];

	token = new anToken(defToken);
	switch ( define->builtin ) {
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
			(*token) = anParser::scriptStack->GetFileName();
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
			t = time(nullptr );
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
			t = time(nullptr );
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
			anParser::Warning( "__STDC__ not supported\n" );
			*firstToken = nullptr;
			*lastToken = nullptr;
			break;
		}
		default: {
			*firstToken = nullptr;
			*lastToken = nullptr;
			break;
		}
	}
	return true;
}

/*
================
anParser::ExpandDefine
================
*/
int anParser::ExpandDefine( anToken *defToken, define_t *define, anToken **firstToken, anToken **lastToken ) {
	anToken *parms[MAX_DEFINEPARMS], *dt, *pt, *t;
	anToken *t1, *t2, *first, *last, *nextpt, token;
	int parmnum, i;

	// if it is a builtin define
	if ( define->builtin ) {
		return anParser::ExpandBuiltinDefine( defToken, define, firstToken, lastToken );
	}
	// if the define has parameters
	if ( define->numParms ) {
		if ( !anParser::ReadDefineParms( define, parms, MAX_DEFINEPARMS ) ) {
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
	first = nullptr;
	last = nullptr;
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
				t = new anToken(pt);
				//add the token to the list
				t->next = nullptr;
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
					if ( !anParser::StringizeTokens( parms[parmnum], &token ) ) {
						anParser::Error( "can't stringize tokens" );
						return false;
					}
					t = new anToken( token );
					t->line = defToken->line;
				} else {
					anParser::Warning( "stringizing operator without define parameter" );
					continue;
				}
			} else {
				t = new anToken(dt);
				t->line = defToken->line;
			}
			// add the token to the list
			t->next = nullptr;
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
					if ( !anParser::MergeTokens( t1, t2 ) ) {
						anParser::Error( "can't merge '%s' with '%s'", t1->c_str(), t2->c_str() );
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
anParser::ExpandDefineIntoSource
================
*/
int anParser::ExpandDefineIntoSource( anToken *defToken, define_t *define ) {
	anToken *firstToken, *lastToken;

	if ( !anParser::ExpandDefine( defToken, define, &firstToken, &lastToken ) ) {
		return false;
	}
	// if the define is not empty
	if ( firstToken && lastToken ) {
		firstToken->linesCrossed += defToken->linesCrossed;
		lastToken->next = anParser::tokens;
		anParser::tokens = firstToken;
	}
	return true;
}

/*
================
anParser::ReadLine

reads a token from the current line, continues reading on the next
line only if a backslash '\' is found
================
*/
int anParser::ReadLine( anToken *token ) {
	int crossline;

	crossline = 0;
	do {
		if ( !anParser::ReadSourceToken( token ) ) {
			return false;
		}

		if (token->linesCrossed > crossline) {
			anParser::UnreadSourceToken( token );
			return false;
		}
		crossline = 1;
	} while( (*token) == "\\" );
	return true;
}

/*
================
anParser::DirectiveInclude
================
*/
int anParser::DirectiveInclude( void ) {
	anLexer *script;
	anToken token;
	anString path;

	if ( !anParser::ReadSourceToken( &token ) ) {
		anParser::Error( "#include without file name" );
		return false;
	}
	if ( token.linesCrossed > 0 ) {
		anParser::Error( "#include without file name" );
		return false;
	}
	if ( token.type == TT_STRING ) {
		script = new anLexer;
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
					script = nullptr;
				}
			}
		}
	} else if ( token.type == TT_PUNCTUATION && token == "<" ) {
		path = anParser::includepath;
		while( anParser::ReadSourceToken( &token ) ) {
			if ( token.linesCrossed > 0 ) {
				anParser::UnreadSourceToken( &token );
				break;
			}
			if ( token.type == TT_PUNCTUATION && token == ">" ) {
				break;
			}
			path += token;
		}
		if ( token != ">" ) {
			anParser::Warning( "#include missing trailing >" );
		}
		if ( !path.Length() ) {
			anParser::Error( "#include without file name between < >" );
			return false;
		}
		if ( anParser::flags & LEXFL_NOBASEINCLUDES ) {
			return true;
		}
		script = new anLexer;
		if ( !script->LoadFile( includepath + path, OSPath ) ) {
			delete script;
			script = nullptr;
		}
	} else {
		anParser::Error( "#include without file name" );
		return false;
	}
	if ( !script) {
		anParser::Error( "file '%s' not found", path.c_str() );
		return false;
	}
	script->SetFlags( anParser::flags );
	script->SetPunctuations( anParser::punctuations );
	anParser::PushScript( script );
	return true;
}

/*
================
anParser::DirectiveUNdef
================
*/
int anParser::DirectiveUNdef( void ) {
	anToken token;
	define_t *define, *lastdefine;
	int hash;

	if ( !anParser::ReadLine( &token ) ) {
		anParser::Error( "undef without name" );
		return false;
	}
	if (token.type != TT_NAME) {
		anParser::UnreadSourceToken( &token );
		anParser::Error( "expected name but found '%s'", token.c_str() );
		return false;
	}

	hash = PC_NameHash( token.c_str() );
	for (lastdefine = nullptr, define = anParser::defineHash[hash]; define; define = define->hashNext) {
		if ( !strcmp( define->name, token.c_str() ) ) {
			if ( define->flags & DEFINE_FIXED) {
				anParser::Warning( "can't undef '%s'", token.c_str() );
			} else {
				if (lastdefine) {
					lastdefine->hashNext = define->hashNext;
				} else {
					anParser::defineHash[hash] = define->hashNext;
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
anParser::DirectiveDefine
================
*/
int anParser::DirectiveDefine( void ) {
	anToken token, *t, *last;
	define_t *define;

	if ( !anParser::ReadLine( &token ) ) {
		anParser::Error( "#define without name" );
		return false;
	}
	if (token.type != TT_NAME) {
		anParser::UnreadSourceToken( &token );
		anParser::Error( "expected name after #define, found '%s'", token.c_str() );
		return false;
	}
	// check if the define already exists
	define = FindHashedDefine( anParser::defineHash, token.c_str() );
	if ( define ) {
		if ( define->flags & DEFINE_FIXED) {
			anParser::Error( "can't redefine '%s'", token.c_str() );
			return false;
		}
		anParser::Warning( "redefinition of '%s'", token.c_str() );
		// unread the define name before executing the #undef directive
		anParser::UnreadSourceToken( &token );
		if ( !anParser::DirectiveUNdef() )
			return false;
		// if the define was not removed ( define->flags & DEFINE_FIXED )
		define = FindHashedDefine( anParser::defineHash, token.c_str() );
	}
	// allocate define
	define = ( define_t *) Mem_ClearedAlloc( sizeof( define_t ) + token.Length() + 1 );
	define->name = (char *) define + sizeof( define_t );
	strcpy( define->name, token.c_str() );
	// add the define to the source
	AddDefineToHash( define, anParser::defineHash );
	// if nothing is defined, just return
	if ( !anParser::ReadLine( &token ) ) {
		return true;
	}
	// if it is a define with parameters
	if ( token.WhiteSpaceBeforeToken() == 0 && token == "( " ) {
		// read the define parameters
		last = nullptr;
		if ( !anParser::CheckTokenString( " )" ) ) {
			while ( 1 ) {
				if ( !anParser::ReadLine( &token ) ) {
					anParser::Error( "expected define parameter" );
					return false;
				}
				// if it isn't a name
				if (token.type != TT_NAME) {
					anParser::Error( "invalid define parameter" );
					return false;
				}

				if ( FindDefineParm( define, token.c_str() ) >= 0 ) {
					anParser::Error( "two the same define parameters" );
					return false;
				}
				// add the define parm
				t = new anToken( token );
				t->ClearTokenWhiteSpace();
				t->next = nullptr;
				if (last) last->next = t;
				else define->parms = t;
				last = t;
				define->numParms++;
				// read next token
				if ( !anParser::ReadLine( &token ) ) {
					anParser::Error( "define parameters not terminated" );
					return false;
				}

				if ( token == " )" ) {
					break;
				}
				// then it must be a comma
				if ( token != "," ) {
					anParser::Error( "define not terminated" );
					return false;
				}
			}
		}
		if ( !anParser::ReadLine( &token ) ) {
			return true;
		}
	}
	// read the defined stuff
	last = nullptr;
	do {
		t = new anToken( token );
		if ( t->type == TT_NAME && !strcmp( t->c_str(), define->name ) ) {
			t->flags |= TOKEN_FL_RECURSIVE_DEFINE;
			anParser::Warning( "recursive define (removed recursion)" );
		}
		t->ClearTokenWhiteSpace();
		t->next = nullptr;
		if ( last ) last->next = t;
		else define->tokens = t;
		last = t;
	} while( anParser::ReadLine( &token ) );
	if ( last ) {
		// check for merge operators at the beginning or end
		if ( (*define->tokens) == "##" || (*last) == "##" ) {
			anParser::Error( "define with misplaced ##" );
			return false;
		}
	}
	return true;
}

/*
================
anParser::AddDefine
================
*/
int anParser::AddDefine( const char *string ) {
	define_t *define;

	define = DefineFromString( string );
	if ( !define ) {
		return false;
	}
	AddDefineToHash( define, anParser::defineHash );
	return true;
}

/*
================
anParser::AddGlobalDefinesToSource
================
*/
void anParser::AddGlobalDefinesToSource( void ) {
	define_t *define, *newDefine;

	for ( define = globalDefines; define; define = define->next) {
		newDefine = CopyDefine( define );
		AddDefineToHash(newDefine, anParser::defineHash );
	}
}

/*
================
anParser::DirectiveIFdef
================
*/
int anParser::DirectiveIFdef( int type ) {
	anToken token;
	define_t *d;
	int skip;

	if ( !anParser::ReadLine( &token ) ) {
		anParser::Error( "#ifdef without name" );
		return false;
	}
	if (token.type != TT_NAME) {
		anParser::UnreadSourceToken( &token );
		anParser::Error( "expected name after #ifdef, found '%s'", token.c_str() );
		return false;
	}
	d = FindHashedDefine( anParser::defineHash, token.c_str() );
	skip = (type == INDENT_IFDEF) == (d == nullptr );
	anParser::PushIndent( type, skip );
	return true;
}

/*
================
anParser::DirectiveIFDEF
================
*/
int anParser::DirectiveIFDEF( void ) {
	return anParser::DirectiveIFdef( INDENT_IFDEF );
}

/*
================
anParser::DirectiveIFndef
================
*/
int anParser::DirectiveIFndef( void ) {
	return anParser::DirectiveIFdef( INDENT_IFNDEF );
}

/*
================
anParser::Directive_else
================
*/
int anParser::Directive_else( void ) {
	int type, skip;

	anParser::PopIndent( &type, &skip );
	if ( !type) {
		anParser::Error( "misplaced #else" );
		return false;
	}
	if (type == INDENT_ELSE) {
		anParser::Error( "#else after #else" );
		return false;
	}
	anParser::PushIndent( INDENT_ELSE, !skip );
	return true;
}

/*
================
anParser::Directive_endif
================
*/
int anParser::Directive_endif ( void ) {
	int type, skip;

	anParser::PopIndent( &type, &skip );
	if ( !type) {
		anParser::Error( "misplaced #endif" );
		return false;
	}
	return true;
}

/*
================
anParser::EvaluateTokens
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
	switch (op) {
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
		anParser::Error( "out of value space\n" ); \
		error = 1; \
		break; \
	} else { \
		val = &value_heap[numValues++]; \
	}

#define FreeValue(val)

#define AllocOperator(op) \
	if ( numOps >= MAX_OPERATORS ) { \
		anParser::Error( "out of operator space\n" ); \
		error = 1; \
		break; \
	} else { \
		op = &operator_heap[numOps++]; \
	}

#define FreeOperator(op)

int anParser::EvaluateTokens( anToken *tokens, signed long int *intVal, double *floatVal, int integer ) {
	operator_t *o, *firstOperator, *lastOperator;
	value_t *v, *firstValue, *lastValue, *v1, *v2;
	anToken *t;
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

	firstOperator = lastOperator = nullptr;
	firstValue = lastValue = nullptr;
	if ( intVal ) *intVal = 0;
	if ( floatVal)  *floatVal = 0;

	for ( t = tokens; t; t = t->next ) {
		switch ( t->type ) {
			case TT_NAME: {
				if ( lastWasVal || negativeVal ) {
					anParser::Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				if ( (*t) != "defined" ) {
					anParser::Error( "undefined name '%s' in #if/#elif", t->c_str() );
					error = 1;
					break;
				}
				t = t->next;
				if ( (*t) == "( " ) {
					brace = true;
					t = t->next;
				}
				if ( !t || t->type != TT_NAM) {
					anParser::Error( "defined() without name in #if/#elif" );
					error = 1;
					break;
				}
				//v = (value_t *) GetClearedMemory( sizeof(value_t) );
				AllocValue( v );
				if ( FindHashedDefine( anParser::defineHash, t->c_str() ) ) {
					v->intVal = 1;
					v->floatVal = 1;
				} else {
					v->intVal = 0;
					v->floatVal = 0;
				}
				v->parentheses = parentheses;
				v->next = nullptr;
				v->prev = lastValue;
				if (lastValue) lastValue->next = v;
				else firstValue = v;
				lastValue = v;
				if ( brace ) {
					t = t->next;
					if ( !t || (*t) != " )" ) {
						anParser::Error( "defined missing ) in #if/#elif" );
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
					anParser::Error( "syntax error in #if/#elif" );
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
				v->next = nullptr;
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
					anParser::Error( "misplaced minus sign in #if/#elif" );
					error = 1;
					break;
				}
				if (t->subtype == P_PARENTHESESOPEN) {
					parentheses++;
					break;
				} else if (t->subtype == P_PARENTHESESCLOSE) {
					parentheses--;
					if (parentheses < 0 ) {
						anParser::Error( "too many ) in #if/#elsif" );
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
						anParser::Error( "illigal operator '%s' on floating point operands\n", t->c_str() );
						error = 1;
						break;
					}
				}
				switch ( t->subtype ) {
					case P_LOGIC_NOT:
					case P_BIN_NOT: {
						if ( lastWasVal ) {
							anParser::Error( "! or ~ after value in #if/#elif" );
							error = 1;
							break;
						}
						break;
					}
					case P_INC:
					case P_DEC: {
						anParser::Error( "++ or -- used in #if/#elif" );
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
							anParser::Error( "operator '%s' after operator in #if/#elif", t->c_str() );
							error = 1;
							break;
						}
						break;
					}
					default: {
						anParser::Error( "invalid operator '%s' in #if/#elif", t->c_str() );
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
					o->next = nullptr;
					o->prev = lastOperator;
					if (lastOperator) lastOperator->next = o;
					else firstOperator = o;
					lastOperator = o;
					lastWasVal = 0;
				}
				break;
			}
			default: {
				anParser::Error( "unknown '%s' in #if/#elif", t->c_str() );
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
			anParser::Error( "trailing operator in #if/#elif" );
			error = 1;
		} else if (parentheses) {
			anParser::Error( "too many ( in #if/#elif" );
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
				anParser::Error( "mising values in #if/#elif" );
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
			Log_Write( "operator %s, value1 = %d", anParser::scriptStack->getPunctuationFromId(o->op), v1->intVal);
			if (v2) Log_Write( "value2 = %d", v2->intVal);
		} else {
			Log_Write( "operator %s, value1 = %f", anParser::scriptStack->getPunctuationFromId(o->op), v1->floatVal);
			if (v2) Log_Write( "value2 = %f", v2->floatVal);
		}
#endif //DEBUG_EVAL
		switch (o->op) {
			case P_LOGIC_NOT:		v1->intVal = !v1->intVal;
									v1->floatVal = !v1->floatVal; break;
			case P_BIN_NOT:			v1->intVal = ~v1->intVal;
									break;
			case P_MUL:				v1->intVal *= v2->intVal;
									v1->floatVal *= v2->floatVal; break;
			case P_DIV:				if ( !v2->intVal || !v2->floatVal) {
										anParser::Error( "divide by zero in #if/#elif\n" );
										error = 1;
										break;
									}
									v1->intVal /= v2->intVal;
									v1->floatVal /= v2->floatVal; break;
			case P_MOD:				if ( !v2->intVal) {
										anParser::Error( "divide by zero in #if/#elif\n" );
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
					anParser::Error( ": without ? in #if/#elif" );
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
					anParser::Error( "? after ? in #if/#elif" );
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
anParser::Evaluate
================
*/
int anParser::Evaluate( signed long int *intVal, double *floatVal, int integer ) {
	anToken token, *firstToken, *lastToken;
	anToken *t, *nexttoken;
	define_t *define;
	int defined = false;

	if ( intVal ) {
		*intVal = 0;
	}
	if ( floatVal)  {
		*floatVal = 0;
	}

	if ( !anParser::ReadLine( &token ) ) {
		anParser::Error( "no value after #if/#elif" );
		return false;
	}
	firstToken = nullptr;
	lastToken = nullptr;
	do {
		// if the token is a name
		if (token.type == TT_NAME) {
			if ( defined) {
				defined = false;
				t = new anToken( token );
				t->next = nullptr;
				if ( lastToken ) lastToken->next = t;
				else firstToken = t;
				lastToken = t;
			} else if ( token == "defined" ) {
				defined = true;
				t = new anToken( token );
				t->next = nullptr;
				if ( lastToken ) lastToken->next = t;
				else firstToken = t;
				lastToken = t;
			} else {
				//then it must be a define
				define = FindHashedDefine( anParser::defineHash, token.c_str() );
				if ( !define ) {
					anParser::Error( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if ( !anParser::ExpandDefineIntoSource( &token, define ) ) {
					return false;
				}
			}// if the token is a number or a punctuation
		} else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION) {
			t = new anToken( token );
			t->next = nullptr;
			if ( lastToken ) lastToken->next = t;
			else firstToken = t;
			lastToken = t;
		} else {
			anParser::Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	} while( anParser::ReadLine( &token ) );
	//
	if ( !anParser::EvaluateTokens( firstToken, intVal, floatVal, integer ) ) {
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
anParser::DollarEvaluate
================
*/
int anParser::DollarEvaluate( signed long int *intVal, double *floatVal, int integer) {
	int indent, defined = false;
	anToken token, *firstToken, *lastToken;
	anToken *t, *nexttoken;
	define_t *define;

	if ( intVal ) {
		*intVal = 0;
	}
	if ( floatVal)  {
		*floatVal = 0;
	}
	//
	if ( !anParser::ReadSourceToken( &token ) ) {
		anParser::Error( "no leading ( after $evalint/$evalfloat" );
		return false;
	}
	if ( !anParser::ReadSourceToken( &token ) ) {
		anParser::Error( "nothing to Evaluate" );
		return false;
	}
	indent = 1;
	firstToken = nullptr;
	lastToken = nullptr;
	do {
		// if the token is a name
		if (token.type == TT_NAME) {
			if ( defined) {
				defined = false;
				t = new anToken( token );
				t->next = nullptr;
				if ( lastToken ) lastToken->next = t;
				else firstToken = t;
				lastToken = t;
			} else if ( token == "defined" ) {
				defined = true;
				t = new anToken( token );
				t->next = nullptr;
				if ( lastToken ) lastToken->next = t;
				else firstToken = t;
				lastToken = t;
			} else {
				//then it must be a define
				define = FindHashedDefine( anParser::defineHash, token.c_str() );
				if ( !define ) {
					anParser::Warning( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if ( !anParser::ExpandDefineIntoSource( &token, define ) ) {
					return false;
				}
			}
		} else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION) {
			if ( token[0] == '(' ) indent++;
			else if ( token[0] == ')' ) indent--;
			if (indent <= 0 ) {
				break;
			}
			t = new anToken( token );
			t->next = nullptr;
			if ( lastToken ) lastToken->next = t;
			else firstToken = t;
			lastToken = t;
		} else {
			anParser::Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	} while( anParser::ReadSourceToken( &token ) );
	if ( !anParser::EvaluateTokens( firstToken, intVal, floatVal, integer) ) {
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
anParser::DirectiveElif
================
*/
int anParser::DirectiveElif ( void ) {
	signed long int value;
	int type, skip;

	anParser::PopIndent( &type, &skip );
	if ( !type || type == INDENT_ELSE) {
		anParser::Error( "misplaced #elif" );
		return false;
	}
	if ( !anParser::Evaluate( &value, nullptr, true ) ) {
		return false;
	}
	skip = (value == 0 );
	anParser::PushIndent( INDENT_ELIF, skip );
	return true;
}

/*
================
anParser::DirectiveIF
================
*/
int anParser::DirectiveIF( void ) {
	signed long int value;
	int skip;

	if ( !anParser::Evaluate( &value, nullptr, true ) ) {
		return false;
	}
	skip = (value == 0 );
	anParser::PushIndent( INDENT_IF, skip );
	return true;
}

/*
================
anParser::DirectiveLine
================
*/
int anParser::DirectiveLine( void ) {
	anToken token;

	anParser::Error( "#line directive not supported" );
	while( anParser::ReadLine( &token ) ) {
	}
	return true;
}

/*
================
anParser::DirectiveError
================
*/
int anParser::DirectiveError( void ) {
	anToken token;

	if ( !anParser::ReadLine( &token) || token.type != TT_STRING ) {
		anParser::Error( "#error without string" );
		return false;
	}
	anParser::Error( "#error: %s", token.c_str() );
	return true;
}

/*
================
anParser::DirectiveWarning
================
*/
int anParser::DirectiveWarning( void ) {
	anToken token;

	if ( !anParser::ReadLine( &token) || token.type != TT_STRING ) {
		anParser::Warning( "#warning without string" );
		return false;
	}
	anParser::Warning( "#warning: %s", token.c_str() );
	return true;
}

/*
================
anParser::Directive_pragma
================
*/
int anParser::DirectivePragma( void ) {
	anToken token;

	anParser::Warning( "#pragma directive not supported" );
	while( anParser::ReadLine( &token ) ) {
	}
	return true;
}

/*
================
anParser::UnreadSignToken
================
*/
void anParser::UnreadSignToken( void ) {
	anToken token;

	token.line = anParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = nullptr;
	token.whiteSpaceEnd_p = nullptr;
	token.linesCrossed = 0;
	token.flags = 0;
	token = "-";
	token.type = TT_PUNCTUATION;
	token.subtype = P_SUB;
	anParser::UnreadSourceToken( &token );
}

/*
================
anParser::DirectiveEval
================
*/
int anParser::DirectiveEval( void ) {
	signed long int value;
	anToken token;
	char buf[128];

	if ( !anParser::Evaluate( &value, nullptr, true ) ) {
		return false;
	}

	token.line = anParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = nullptr;
	token.whiteSpaceEnd_p = nullptr;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf(buf, "%d", abs(value) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER|TT_LONG|TT_DECIMAL;
	anParser::UnreadSourceToken( &token );
	if ( value < 0 ) {
		anParser::UnreadSignToken();
	}
	return true;
}

/*
================
anParser::DirectiveEvalfloat
================
*/
int anParser::DirectiveEvalfloat( void ) {
	double value;
	anToken token;
	char buf[128];

	if ( !anParser::Evaluate( nullptr, &value, false ) ) {
		return false;
	}

	token.line = anParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = nullptr;
	token.whiteSpaceEnd_p = nullptr;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf(buf, "%1.2f", anMath::Fabs(value) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT|TT_LONG|TT_DECIMAL;
	anParser::UnreadSourceToken( &token );
	if (value < 0 ) {
		anParser::UnreadSignToken();
	}
	return true;
}

/*
================
anParser::ReadDirective
================
*/
int anParser::ReadDirective( void ) {
	anToken token;

	// read the directive name
	if ( !anParser::ReadSourceToken( &token ) ) {
		anParser::Error( "found '#' without name" );
		return false;
	}
	// directive name must be on the same line
	if (token.linesCrossed > 0 ) {
		anParser::UnreadSourceToken( &token );
		anParser::Error( "found '#' at end of line" );
		return false;
	}
	// if if is a name
	if (token.type == TT_NAME) {
		if ( token == "if" ) {
			return anParser::DirectiveIF();
		}
		else if ( token == "ifdef" ) {
			return anParser::DirectiveIFDEF();
		}
		else if ( token == "ifndef" ) {
			return anParser::DirectiveIFndef();
		}
		else if ( token == "elif" ) {
			return anParser::DirectiveElif ();
		}
		else if ( token == "else" ) {
			return anParser::Directive_else();
		}
		else if ( token == "endif" ) {
			return anParser::Directive_endif ();
		}
		else if ( anParser::skip > 0 ) {
			// skip the rest of the line
			while( anParser::ReadLine( &token ) ) {
			}
			return true;
		}
		else {
			if ( token == "include" ) {
				return anParser::DirectiveInclude();
			}
			else if ( token == "define" ) {
				return anParser::DirectiveDefine();
			}
			else if ( token == "undef" ) {
				return anParser::DirectiveUNdef();
			}
			else if ( token == "line" ) {
				return anParser::DirectiveLine();
			}
			else if ( token == "error" ) {
				return anParser::DirectiveError();
			}
			else if ( token == "warning" ) {
				return anParser::DirectiveWarning();
			}
			else if ( token == "pragma" ) {
				return anParser::Directive_pragma();
			}
			else if ( token == "eval" ) {
				return anParser::DirectiveEval();
			}
			else if ( token == "evalfloat" ) {
				return anParser::DirectiveEvalfloat();
			}
		}
	}
	anParser::Error( "unknown precompiler directive '%s'", token.c_str() );
	return false;
}

/*
================
anParser::DollarDirectiveEvalInt
================
*/
int anParser::DollarDirectiveEvalInt( void ) {
	signed long int value;
	anToken token;
	char buf[128];

	if ( !anParser::DollarEvaluate( &value, nullptr, true ) ) {
		return false;
	}

	token.line = anParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = nullptr;
	token.whiteSpaceEnd_p = nullptr;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%d", abs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intVal = abs( value );
	token.floatVal = abs( value );
	anParser::UnreadSourceToken( &token );
	if ( value < 0 ) {
		anParser::UnreadSignToken();
	}
	return true;
}

/*
================
anParser::DollarDirectiveEvalfloat
================
*/
int anParser::DollarDirectiveEvalfloat( void ) {
	double value;
	anToken token;
	char buf[128];

	if ( !anParser::DollarEvaluate( nullptr, &value, false ) ) {
		return false;
	}

	token.line = anParser::scriptStack->GetLineNum();
	token.whiteSpaceStart_p = nullptr;
	token.whiteSpaceEnd_p = nullptr;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%1.2f", fabs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intVal = (unsigned long) fabs( value );
	token.floatVal = fabs( value );
	anParser::UnreadSourceToken( &token );
	if ( value < 0 ) {
		anParser::UnreadSignToken();
	}
	return true;
}

/*
================
anParser::ReadDollarDirective
================
*/
int anParser::ReadDollarDirective( void ) {
	anToken token;

	// read the directive name
	if ( !anParser::ReadSourceToken( &token ) ) {
		anParser::Error( "found '$' without name" );
		return false;
	}
	// directive name must be on the same line
	if ( token.linesCrossed > 0 ) {
		anParser::UnreadSourceToken( &token );
		anParser::Error( "found '$' at end of line" );
		return false;
	}
	// if if is a name
	if (token.type == TT_NAME) {
		if ( token == "evalint" ) {
			return anParser::DollarDirectiveEvalInt();
		}
		else if ( token == "evalfloat" ) {
			return anParser::DollarDirectiveEvalfloat();
		}
	}
	anParser::UnreadSourceToken( &token );
	return false;
}

/*
================
anParser::ReadToken
================
*/
int anParser::ReadToken( anToken *token ) {
	define_t *define;

	while(1 ) {
		if ( !anParser::ReadSourceToken( token ) ) {
			return false;
		}
		// check for precompiler directives
		if ( token->type == TT_PUNCTUATION && (*token)[0] == '#' && (*token)[1] == '\0' ) {
			// read the precompiler directive
			if ( !anParser::ReadDirective() ) {
				return false;
			}
			continue;
		}
		// if skipping source because of conditional compilation
		if ( anParser::skip ) {
			continue;
		}
		// recursively concatenate strings that are behind each other still resolving defines
		if ( token->type == TT_STRING && !( anParser::scriptStack->GetFlags() & LEXFL_NOSTRINGCONCAT) ) {
			anToken newToken;
			if ( anParser::ReadToken( &newToken ) ) {
				if ( newToken.type == TT_STRING ) {
					token->Append( newToken.c_str() );
				}
				else {
					anParser::UnreadSourceToken( &newToken );
				}
			}
		}
		//
		if ( !( anParser::scriptStack->GetFlags() & LEXFL_NODOLLARPRECOMPILE) ) {
			// check for special precompiler directives
			if ( token->type == TT_PUNCTUATION && (*token)[0] == '$' && (*token)[1] == '\0' ) {
				// read the precompiler directive
				if ( anParser::ReadDollarDirective() ) {
					continue;
				}
			}
		}
		// if the token is a name
		if ( token->type == TT_NAME && !( token->flags & TOKEN_FL_RECURSIVE_DEFINE ) ) {
			// check if the name is a define macro
			define = FindHashedDefine( anParser::defineHash, token->c_str() );
			// if it is a define macro
			if ( define ) {
				// expand the defined macro
				if ( !anParser::ExpandDefineIntoSource( token, define ) ) {
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
anParser::ExpectTokenString
================
*/
int anParser::ExpectTokenString( const char *string ) {
	anToken token;

	if ( !anParser::ReadToken( &token ) ) {
		anParser::Error( "couldn't find expected '%s'", string );
		return false;
	}

	if ( token != string ) {
		anParser::Error( "expected '%s' but found '%s'", string, token.c_str() );
		return false;
	}
	return true;
}

/*
================
anParser::ExpectTokenType
================
*/
int anParser::ExpectTokenType( int type, int subtype, anToken *token ) {
	anString str;

	if ( !anParser::ReadToken( token ) ) {
		anParser::Error( "couldn't read expected token" );
		return 0;
	}

	if ( token->type != type ) {
		switch ( type ) {
			case TT_STRING: str = "string"; break;
			case TT_LITERAL: str = "literal"; break;
			case TT_NUMBER: str = "number"; break;
			case TT_NAME: str = "name"; break;
			case TT_PUNCTUATION: str = "punctuation"; break;
			default: str = "unknown type"; break;
		}
		anParser::Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
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
			anParser::Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if ( token->type == TT_PUNCTUATION ) {
		if ( subtype < 0 ) {
			anParser::Error( "BUG: wrong punctuation subtype" );
			return 0;
		}
		if ( token->subtype != subtype ) {
			anParser::Error( "expected '%s' but found '%s'", scriptStack->GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}

/*
================
anParser::ExpectAnyToken
================
*/
int anParser::ExpectAnyToken( anToken *token ) {
	if ( !anParser::ReadToken( token ) ) {
		anParser::Error( "couldn't read expected token" );
		return false;
	}
	else {
		return true;
	}
}

/*
================
anParser::CheckTokenString
================
*/
int anParser::CheckTokenString( const char *string ) {
	anToken tok;

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
anParser::CheckTokenType
================
*/
int anParser::CheckTokenType( int type, int subtype, anToken *token ) {
	anToken tok;

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
anParser::PeekTokenString
================
*/
int anParser::PeekTokenString( const char *string ) {
	anToken tok;

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
anParser::PeekTokenType
================
*/
int anParser::PeekTokenType( int type, int subtype, anToken *token ) {
	anToken tok;

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
anParser::SkipUntilString
================
*/
int anParser::SkipUntilString( const char *string ) {
	anToken token;

	while( anParser::ReadToken( &token ) ) {
		if ( token == string ) {
			return true;
		}
	}
	return false;
}

/*
================
anParser::SkipRestOfLine
================
*/
int anParser::SkipRestOfLine( void ) {
	anToken token;

	while( anParser::ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			anParser::UnreadSourceToken( &token );
			return true;
		}
	}
	return false;
}

/*
=================
anParser::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int anParser::SkipBracedSection( bool parseFirstBrace ) {
	anToken token;
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
anParser::ParseBracedSectionExact

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains the exact formating of the braced section

  FIXME: what about precompilation ?
=================
*/
const char *anParser::ParseBracedSectionExact( anString &out, int tabs ) {
	return scriptStack->ParseBracedSectionExact( out, tabs );
}

/*
=================
anParser::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
const char *anParser::ParseBracedSection( anString &out, int tabs ) {
	anToken token;
	int i, depth;
	bool doTabs = false;
	if (tabs >= 0 ) {
		doTabs = true;
	}

	out.Empty();
	if ( !anParser::ExpectTokenString( "{" ) ) {
		return out.c_str();
	}
	out = "{";
	depth = 1;
	do {
		if ( !anParser::ReadToken( &token ) ) {
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
anParser::ParseRestOfLine

  parse the rest of the line
=================
*/
const char *anParser::ParseRestOfLine( anString &out ) {
	anToken token;

	out.Empty();
	while( anParser::ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			anParser::UnreadSourceToken( &token );
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
anParser::UnreadToken
================
*/
void anParser::UnreadToken( anToken *token ) {
	anParser::UnreadSourceToken( token );
}

/*
================
anParser::ReadTokenOnLine
================
*/
int anParser::ReadTokenOnLine( anToken *token ) {
	anToken tok;

	if ( !anParser::ReadToken( &tok ) ) {
		return false;
	}
	// if no lines were crossed before this token
	if ( !tok.linesCrossed ) {
		*token = tok;
		return true;
	}
	//
	anParser::UnreadSourceToken( &tok );
	return false;
}

/*
================
anParser::ParseInt
================
*/
int anParser::ParseInt( void ) {
	anToken token;

	if ( !anParser::ReadToken( &token ) ) {
		anParser::Error( "couldn't read expected integer" );
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		anParser::ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -(( signed int) token.GetIntValue() );
	}
	else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
		anParser::Error( "expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
anParser::ParseBool
================
*/
bool anParser::ParseBool( void ) {
	anToken token;

	if ( !anParser::ExpectTokenType( TT_NUMBER, 0, &token ) ) {
		anParser::Error( "couldn't read expected boolean" );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}

/*
================
anParser::ParseFloat
================
*/
float anParser::ParseFloat( void ) {
	anToken token;

	if ( !anParser::ReadToken( &token ) ) {
		anParser::Error( "couldn't read expected floating point number" );
		return 0.0f;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		anParser::ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	}
	else if ( token.type != TT_NUMBER ) {
		anParser::Error( "expected float value, found '%s'", token.c_str() );
	}
	return token.GetFloatValue();
}

/*
================
anParser::Parse1DMatrix
================
*/
int anParser::Parse1DMatrix( int x, float *m ) {
	int i;

	if ( !anParser::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( i = 0; i < x; i++ ) {
		m[i] = anParser::ParseFloat();
	}

	if ( !anParser::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
anParser::Parse2DMatrix
================
*/
int anParser::Parse2DMatrix( int y, int x, float *m ) {
	int i;

	if ( !anParser::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( i = 0; i < y; i++ ) {
		if ( !anParser::Parse1DMatrix( x, m + i * x ) ) {
			return false;
		}
	}

	if ( !anParser::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
anParser::Parse3DMatrix
================
*/
int anParser::Parse3DMatrix( int z, int y, int x, float *m ) {
	int i;

	if ( !anParser::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( i = 0; i < z; i++ ) {
		if ( !anParser::Parse2DMatrix( y, x, m + i * x*y ) ) {
			return false;
		}
	}

	if ( !anParser::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
anParser::GetLastWhiteSpace
================
*/
int anParser::GetLastWhiteSpace( anString &whiteSpace ) const {
	if ( scriptStack ) {
		scriptStack->GetLastWhiteSpace( whiteSpace );
	} else {
		whiteSpace.Clear();
	}
	return whiteSpace.Length();
}

/*
================
anParser::SetMarker
================
*/
void anParser::SetMarker( void ) {
	marker_p = nullptr;
}

/*
================
anParser::GetStringFromMarker

  FIXME: this is very bad code, the script isn't even garrenteed to still be around
================
*/
void anParser::GetStringFromMarker( anString& out, bool clean ) {
	char*	p;
	char	save;

	if ( marker_p == nullptr ) {
		marker_p = scriptStack->buffer;
	}

	if ( tokens ) {
		p = (char*)tokens->whiteSpaceStart_p;
	} else {
		p = (char*)scriptStack->script_p;
	}

	// Set the end character to nullptr to give us a complete string
	save = *p;
	*p = 0;

	// If cleaning then reparse
	if ( clean ) {
		anParser temp( marker_p, strlen( marker_p ), "temp", flags );
		anToken token;
		while ( temp.ReadToken ( &token ) ) {
			out += token;
		}
	} else {
		out = marker_p;
	}

	// restore the character we set to nullptr
	*p = save;
}

/*
================
anParser::SetIncludePath
================
*/
void anParser::SetIncludePath( const char *path ) {
	anParser::includepath = path;
	// add trailing path seperator
	if ( anParser::includepath[anParser::includepath.Length()-1] != '\\' &&
		anParser::includepath[anParser::includepath.Length()-1] != '/') {
		anParser::includepath += PATHSEPERATOR_STR;
	}
}

/*
================
anParser::SetPunctuations
================
*/
void anParser::SetPunctuations( const punctuation_t *p ) {
	anParser::punctuations = p;
}

/*
================
anParser::SetFlags
================
*/
void anParser::SetFlags( int flags ) {
	anLexer *s;

	anParser::flags = flags;
	for ( s = anParser::scriptStack; s; s = s->next ) {
		s->SetFlags( flags );
	}
}

/*
================
anParser::GetFlags
================
*/
int anParser::GetFlags( void ) const {
	return anParser::flags;
}

/*
================
anParser::LoadFile
================
*/
int anParser::LoadFile( const char *filename, bool OSPath ) {
	anLexer *script;

	if ( anParser::loaded ) {
		anLibrary::common->FatalError( "anParser::loadFile: another source already loaded" );
		return false;
	}
	script = new anLexer( filename, 0, OSPath );
	if ( !script->IsLoaded() ) {
		delete script;
		return false;
	}
	script->SetFlags( anParser::flags );
	script->SetPunctuations( anParser::punctuations );
	script->next = nullptr;
	anParser::OSPath = OSPath;
	anParser::filename = filename;
	anParser::scriptStack = script;
	anParser::tokens = nullptr;
	anParser::indentStack = nullptr;
	anParser::skip = 0;
	anParser::loaded = true;

	if ( !anParser::defineHash ) {
		anParser::defines = nullptr;
		anParser::defineHash = ( define_t **) Mem_ClearedAlloc( DEFINEHASHSIZE * sizeof( define_t *) );
		anParser::AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
anParser::LoadMemory
================
*/
int anParser::LoadMemory(const char *ptr, int length, const char *name ) {
	anLexer *script;

	if ( anParser::loaded ) {
		anLibrary::common->FatalError( "anParser::loadMemory: another source already loaded" );
		return false;
	}
	script = new anLexer( ptr, length, name );
	if ( !script->IsLoaded() ) {
		delete script;
		return false;
	}
	script->SetFlags( anParser::flags );
	script->SetPunctuations( anParser::punctuations );
	script->next = nullptr;
	anParser::filename = name;
	anParser::scriptStack = script;
	anParser::tokens = nullptr;
	anParser::indentStack = nullptr;
	anParser::skip = 0;
	anParser::loaded = true;

	if ( !anParser::defineHash ) {
		anParser::defines = nullptr;
		anParser::defineHash = ( define_t **) Mem_ClearedAlloc( DEFINEHASHSIZE * sizeof( define_t *) );
		anParser::AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
anParser::FreeSource
================
*/
void anParser::FreeSource( bool keepDefines ) {
	anLexer *script;
	anToken *token;
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
			defines = nullptr;
			Mem_Free( anParser::defineHash );
			defineHash = nullptr;
		}
	}
	loaded = false;
}

/*
================
anParser::GetPunctuationFromId
================
*/
const char *anParser::GetPunctuationFromId( int id ) {
	int i;

	if ( !anParser::punctuations ) {
		anLexer lex;
		return lex.GetPunctuationFromId( id );
	}

	for ( i = 0; anParser::punctuations[i].p; i++ ) {
		if ( anParser::punctuations[i].n == id ) {
			return anParser::punctuations[i].p;
		}
	}
	return "unkown punctuation";
}

/*
================
anParser::GetPunctuationId
================
*/
int anParser::GetPunctuationId( const char *p ) {
	int i;

	if ( !anParser::punctuations ) {
		anLexer lex;
		return lex.GetPunctuationId( p );
	}

	for ( i = 0; anParser::punctuations[i].p; i++ ) {
		if ( !strcmp( anParser::punctuations[i].p, p) ) {
			return anParser::punctuations[i].n;
		}
	}
	return 0;
}

/*
================
anParser::anParser
================
*/
anParser::anParser() {
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = 0;
	this->scriptStack = nullptr;
	this->indentStack = nullptr;
	this->defineHash = nullptr;
	this->defines = nullptr;
	this->tokens = nullptr;
	this->marker_p = nullptr;
}

/*
================
anParser::anParser
================
*/
anParser::anParser( int flags ) {
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptStack = nullptr;
	this->indentStack = nullptr;
	this->defineHash = nullptr;
	this->defines = nullptr;
	this->tokens = nullptr;
	this->marker_p = nullptr;
}

/*
================
anParser::anParser
================
*/
anParser::anParser( const char *filename, int flags, bool OSPath ) {
	this->loaded = false;
	this->OSPath = true;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptStack = nullptr;
	this->indentStack = nullptr;
	this->defineHash = nullptr;
	this->defines = nullptr;
	this->tokens = nullptr;
	this->marker_p = nullptr;
	LoadFile( filename, OSPath );
}

/*
================
anParser::anParser
================
*/
anParser::anParser( const char *ptr, int length, const char *name, int flags ) {
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptStack = nullptr;
	this->indentStack = nullptr;
	this->defineHash = nullptr;
	this->defines = nullptr;
	this->tokens = nullptr;
	this->marker_p = nullptr;
	LoadMemory( ptr, length, name );
}

/*
================
anParser::~anParser
================
*/
anParser::~anParser( void ) {
	anParser::FreeSource( false );
}


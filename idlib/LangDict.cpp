#include "Lib.h"
#pragma hdrstop


/*
============
anLangDict::anLangDict
============
*/
anLangDict::anLangDict( void ) {
	args.SetGranularity( 256 );
	hash.SetGranularity( 256 );
	hash.Clear( 4096, 8192 );
	baseID = 0;
}

/*
============
anLangDict::~anLangDict
============
*/
anLangDict::~anLangDict( void ) {
	Clear();
}

/*
============
anLangDict::Clear
============
*/
void anLangDict::Clear( void ) {
	args.Clear();
	hash.Clear();
}

/*
============
anLangDict::Load
============
*/
bool anLangDict::Load( const char *fileName, bool clear ) {
	if ( clear ) {
		Clear();
	}

	const char *buffer = nullptr;
	anLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	int len = anLibrary::fileSystem->ReadFile( fileName, (void**)&buffer );
	if ( len <= 0 ) {
		// let whoever called us deal with the failure ( so sys_lang can be reset)
		return false;
	}
	src.LoadMemory( buffer, strlen( buffer ), fileName );
	if ( !src.IsLoaded() ) {
		return false;
	}

	anToken tok, tok2;
	src.ExpectTokenString( "{" );
	while ( src.ReadToken( &tok ) ) {
		if ( tok == "}" ) {
			break;
		}
		if ( src.ReadToken( &tok2 ) ) {
			if ( tok2 == "}" ) {
				break;
			}
			anLangKeyValue kv;
			kv.key = tok;
			kv.value = tok2;
			assert( kv.key.Cmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
			hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
		}
	}
	anLibrary::common->Printf( "%i strings read from %s\n", args.Num(), fileName );
	anLibrary::fileSystem->FreeFile( (void*)buffer );

	return true;
}

/*
============
anLangDict::Save
============
*/
void anLangDict::Save( const char *fileName ) {
	anFile *outFile = anLibrary::fileSystem->OpenFileWrite( fileName );
	outFile->WriteFloatString( "// string table\n// english\n//\n\n{\n" );
	for ( int j = 0; j < args.Num(); j++ ) {
		outFile->WriteFloatString( "\t\"%s\"\t\"", args[j].key.c_str() );
		int l = args[j].value.Length();
		char slash = '\\';
		char tab = 't';
		char nl = 'n';
		for ( int k = 0; k < l; k++ ) {
			char ch = args[j].value[k];
			if ( ch == '\t' ) {
				outFile->Write( &slash, 1 );
				outFile->Write( &tab, 1 );
			} else if ( ch == '\n' || ch == '\r' ) {
				outFile->Write( &slash, 1 );
				outFile->Write( &nl, 1 );
			} else {
				outFile->Write( &ch, 1 );
			}
		}
		outFile->WriteFloatString( "\"\n" );
	}
	outFile->WriteFloatString( "\n}\n" );
	anLibrary::fileSystem->CloseFile( outFile );
}

/*
============
anLangDict::GetString
============
*/
const char *anLangDict::GetString( const char *str ) const {
	if ( str == nullptr || str[0] == '\0' ) {
		return "";
	}

	if ( ::Cmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 ) {
		return str;
	}

	int hashKey = GetHashKey( str );
	for ( int i = hash.First( hashKey ); i != -1; i = hash.Next( i ) ) {
		if ( args[i].key.Cmp( str ) == 0 ) {
			return args[i].value;
		}
	}

	anLibrary::common->Warning( "Unknown string id %s", str );
	return str;
}

/*
============
anLangDict::AddString
============
*/
const char *anLangDict::AddString( const char *str ) {
	if ( ExcludeString( str ) ) {
		return str;
	}

	int c = args.Num();
	for ( int j = 0; j < c; j++ ) {
		if ( ::Cmp( args[j].value, str ) == 0 ){
			return args[j].key;
		}
	}

	int id = GetNextId();
	anLangKeyValue kv;

	kv.key = va( "#str_%08i", id );
	// kv.key = va( "#str_%05i", id );
	kv.value = str;
	c = args.Append( kv );
	assert( kv.key.Cmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
	hash.Add( GetHashKey( kv.key ), c );
	return args[c].key;
}

/*
============
anLangDict::GetNumKeyVals
============
*/
int anLangDict::GetNumKeyVals( void ) const {
	return args.Num();
}

/*
============
anLangDict::GetKeyVal
============
*/
const anLangKeyValue * anLangDict::GetKeyVal( int i ) const {
	return &args[i];
}

/*
============
anLangDict::AddKeyVal
============
*/
void anLangDict::AddKeyVal( const char *key, const char *val ) {
	anLangKeyValue kv;
	kv.key = key;
	kv.value = val;
	assert( kv.key.Cmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
	hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
}

/*
============
anLangDict::ExcludeString
============
*/
bool anLangDict::ExcludeString( const char *str ) const {
	if ( str == nullptr ) {
		return true;
	}

	int c = strlen( str );
	if ( c <= 1 ) {
		return true;
	}

	if ( ::Cmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
		return true;
	}

	if ( ::Icmpn( str, "gui::", strlen( "gui::" ) ) == 0 ) {
		return true;
	}

	if ( str[0] == '$' ) {
		return true;
	}

	int i;
	for ( i = 0; i < c; i++ ) {
		if ( isalpha( str[i] ) ) {
			break;
		}
	}
	if ( i == c ) {
		return true;
	}

	return false;
}

/*
============
anLangDict::GetNextId
============
*/
int anLangDict::GetNextId( void ) const {
	int c = args.Num();
	int id = baseID;	// Lets external user supply the base id for this dictionary

	if ( c == 0 ) {
		return id;
	}

	 work;
	for ( int j = 0; j < c; j++ ) {
		work = args[j].key;
		work.StripLeading( STRTABLE_ID );
		int test = atoi( work );
		if ( test > id ) {
			id = test;
		}
	}
	return id + 1;
}

/*
============
anLangDict::GetHashKey
============
*/
int anLangDict::GetHashKey( const char *str ) const {
	int hashKey = 0;
	for ( str += STRTABLE_ID_LENGTH; str[0] != '\0'; str++ ) {
		assert( str[0] >= '0' && str[0] <= '9' );
		hashKey = hashKey * 10 + str[0] - '0';
	}
	return hashKey;
}

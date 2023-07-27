#include "precompiled.h"
#pragma hdrstop


/*
============
arcLangDictionary::arcLangDictionary
============
*/
arcLangDictionary::arcLangDictionary( void ) {
	args.SetGranularity( 256 );
	hash.SetGranularity( 256 );
	hash.Clear( 4096, 8192 );
	baseID = 0;
}

/*
============
arcLangDictionary::~arcLangDictionary
============
*/
arcLangDictionary::~arcLangDictionary( void ) {
	Clear();
}

/*
============
arcLangDictionary::Clear
============
*/
void arcLangDictionary::Clear( void ) {
	args.Clear();
	hash.Clear();
}

/*
============
arcLangDictionary::Load
============
*/
bool arcLangDictionary::Load( const char *fileName, bool clear ) {
	if ( clear ) {
		Clear();
	}

	const char *buffer = NULL;
	anLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	int len = anLibrary::fileSystem->ReadFile( fileName, (void**)&buffer );
	if ( len <= 0 ) {
		// let whoever called us deal with the failure (so sys_lang can be reset)
		return false;
	}
	src.LoadMemory( buffer, strlen( buffer ), fileName );
	if ( !src.IsLoaded() ) {
		return false;
	}

	arcNetToken tok, tok2;
	src.ExpectTokenString( "{" );
	while ( src.ReadToken( &tok ) ) {
		if ( tok == "}" ) {
			break;
		}
		if ( src.ReadToken( &tok2 ) ) {
			if ( tok2 == "}" ) {
				break;
			}
			idLangKeyValue kv;
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
arcLangDictionary::Save
============
*/
void arcLangDictionary::Save( const char *fileName ) {
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
arcLangDictionary::GetString
============
*/
const char *arcLangDictionary::GetString( const char *str ) const {
	if ( str == NULL || str[0] == '\0' ) {
		return "";
	}

	if ( anString::Cmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 ) {
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
arcLangDictionary::AddString
============
*/
const char *arcLangDictionary::AddString( const char *str ) {
	if ( ExcludeString( str ) ) {
		return str;
	}

	int c = args.Num();
	for ( int j = 0; j < c; j++ ) {
		if ( anString::Cmp( args[j].value, str ) == 0 ){
			return args[j].key;
		}
	}

	int id = GetNextId();
	idLangKeyValue kv;

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
arcLangDictionary::GetNumKeyVals
============
*/
int arcLangDictionary::GetNumKeyVals( void ) const {
	return args.Num();
}

/*
============
arcLangDictionary::GetKeyVal
============
*/
const idLangKeyValue * arcLangDictionary::GetKeyVal( int i ) const {
	return &args[i];
}

/*
============
arcLangDictionary::AddKeyVal
============
*/
void arcLangDictionary::AddKeyVal( const char *key, const char *val ) {
	idLangKeyValue kv;
	kv.key = key;
	kv.value = val;
	assert( kv.key.Cmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
	hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
}

/*
============
arcLangDictionary::ExcludeString
============
*/
bool arcLangDictionary::ExcludeString( const char *str ) const {
	if ( str == NULL ) {
		return true;
	}

	int c = strlen( str );
	if ( c <= 1 ) {
		return true;
	}

	if ( anString::Cmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
		return true;
	}

	if ( anString::Icmpn( str, "gui::", strlen( "gui::" ) ) == 0 ) {
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
arcLangDictionary::GetNextId
============
*/
int arcLangDictionary::GetNextId( void ) const {
	int c = args.Num();
	int id = baseID;	// Lets external user supply the base id for this dictionary

	if ( c == 0 ) {
		return id;
	}

	anString work;
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
arcLangDictionary::GetHashKey
============
*/
int arcLangDictionary::GetHashKey( const char *str ) const {
	int hashKey = 0;
	for ( str += STRTABLE_ID_LENGTH; str[0] != '\0'; str++ ) {
		assert( str[0] >= '0' && str[0] <= '9' );
		hashKey = hashKey * 10 + str[0] - '0';
	}
	return hashKey;
}

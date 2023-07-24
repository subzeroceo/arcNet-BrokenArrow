#include "/idlib/precompiled.h"
#pragma hdrstop

void idTokenParser::LoadFromParser( ARCParser &parser, const char *guiName ) {
	arcNetToken tok;
	idTokenIndexes tokIdxs;
	tokIdxs.SetName( guiName );
	while ( parser.ReadToken( &tok ) ) {
		tokIdxs.Append( tokens.AddUnique( idBinaryToken( tok ) ) );
	}
	guiTokenIndexes.Append( tokIdxs );
	currentToken = 0;
}

void idTokenParser::LoadFromFile( const char *filename ) {
	Clear();
	arcNetFile *inFile = fileSystem->OpenFileReadMemory( filename );
	if ( inFile != NULL ) {
		int num;
		inFile->ReadBig( num );
		guiTokenIndexes.SetNum( num );
		for ( int i = 0; i < num; i++ ) {
			guiTokenIndexes[ i ].Read( inFile );
		}
		inFile->ReadBig( num );
		tokens.SetNum( num );
		for ( int i = 0; i < num; i++ ) {
			tokens[ i ].Read( inFile );
		}
	}
	delete inFile;
	preloaded = ( tokens.Num() > 0 );
}

void idTokenParser::WriteToFile( const char *filename ) {
	if ( preloaded ) {
		return;
	}
	arcNetFile *outFile = fileSystem->OpenFileWrite( filename, "fs_basepath" );
	if ( outFile != NULL ) {
		outFile->WriteBig( ( int )guiTokenIndexes.Num() );
		for ( int i = 0; i < guiTokenIndexes.Num(); i++ ) {
			guiTokenIndexes[ i ].Write( outFile );
		}
		outFile->WriteBig( ( int )tokens.Num() );
		for ( int i = 0; i < tokens.Num(); i++ ) {
			tokens[ i ].Write( outFile );
		}
	}
	delete outFile;
}

bool idTokenParser::StartParsing( const char * filename ) {
	currentTokenList = -1;
	for ( int i = 0; i < guiTokenIndexes.Num(); i++ ) {
		if ( arcNetString::Icmp( filename, guiTokenIndexes[ i ].GetName() ) == 0 ) {
			currentTokenList = i;
			break;
		}
	}
	currentToken = 0;
	return ( currentTokenList != -1 );
}

bool idTokenParser::ReadToken( arcNetToken * tok ) {
	if ( currentToken >= 0 && currentToken < guiTokenIndexes[ currentTokenList ].Num() ) {
		tok->Clear();
		idBinaryToken &btok = tokens[ guiTokenIndexes[ currentTokenList ][ currentToken ] ];
		*tok = btok.token;
		tok->type = btok.tokenType;
		tok->subtype = btok.tokenSubType;
		currentToken++;
		return true;
	}
	return false;
}
int	idTokenParser::ExpectTokenString( const char *string ) {
	arcNetToken token;
	if ( !ReadToken( &token ) ) {
		Error( "couldn't find expected '%s'", string );
		return 0;
	}
	if ( token != string ) {
		Error( "expected '%s' but found '%s'", string, token.c_str() );
		return 0;
	}
	return 1;
}
// expect a certain token type
int	idTokenParser::ExpectTokenType( int type, int subtype, arcNetToken *token ) {
	arcNetString str;

	if ( !ReadToken( token ) ) {
		Error( "couldn't read expected token" );
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
		Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
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
			Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if ( token->type == TT_PUNCTUATION ) {
		if ( subtype < 0 ) {
			Error( "BUG: wrong punctuation subtype" );
			return 0;
		}
		if ( token->subtype != subtype ) {
			//Error( "expected '%s' but found '%s'", arcLexer::GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}
// expect a token
int idTokenParser::ExpectAnyToken( arcNetToken *token ) {
	if ( !ReadToken( token ) ) {
		Error( "couldn't read expected token" );
		return 0;
	}
	return 1;
}

void idTokenParser::UnreadToken( const arcNetToken *token ) {
	if ( currentToken == 0 || currentToken >=  guiTokenIndexes[ currentTokenList ].Num() ) {
		arcLibrary::common->FatalError( "idTokenParser::unreadToken, unread token twice\n" );
	}
	currentToken--;
}
void idTokenParser::Error( VERIFY_FORMAT_STRING const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);

	arcLibrary::common->Warning( text );
}
void idTokenParser::Warning( VERIFY_FORMAT_STRING const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);

	arcLibrary::common->Warning( text );
}
int idTokenParser::ParseInt() {
	arcNetToken token;
	if ( !ReadToken( &token ) ) {
		Error( "couldn't read expected integer" );
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -((signed int) token.GetIntValue() );
	} else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
		Error( "expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}
// read a boolean
bool idTokenParser::ParseBool() {
	arcNetToken token;
	if ( !ExpectTokenType( TT_NUMBER, 0, &token ) ) {
		Error( "couldn't read expected boolean" );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}
// read a floating point number.  If errorFlag is NULL, a non-numeric token will
// issue an Error().  If it isn't NULL, it will issue a Warning() and set *errorFlag = true
float idTokenParser::ParseFloat( bool *errorFlag ) {
	arcNetToken token;
	if ( errorFlag ) {
		*errorFlag = false;
	}
	if ( !ReadToken( &token ) ) {
		if ( errorFlag ) {
			Warning( "couldn't read expected floating point number" );
			*errorFlag = true;
		} else {
			Error( "couldn't read expected floating point number" );
		}
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	} else if ( token.type != TT_NUMBER ) {
		if ( errorFlag ) {
			Warning( "expected float value, found '%s'", token.c_str() );
			*errorFlag = true;
		} else {
			Error( "expected float value, found '%s'", token.c_str() );
		}
	}
	return token.GetFloatValue();
}

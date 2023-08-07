#include "Lib.h"
#include "LexerBinary.h"
#include "Lexer.h"
#pragma hdrstop


/*
============
anTokenCache::FindToken
============
*/
unsigned short anTokenCache::FindToken( const anToken &token ) {
	int hashKey = uniqueTokenHash.GenerateKey( token.c_str(), true );
	for ( int i = uniqueTokenHash.GetFirst( hashKey ); i != arcHashIndexUShort::nullptr_INDEX; i = uniqueTokenHash.GetNext( i ) ) {
		// from mac version
		if ( ( i < 0 ) || ( i > uniqueTokens.Num() ) ) {
			break;
		}
		const anToken& hashedToken = uniqueTokens[i];
		if ( ( hashedToken.type == token.type ) && ( ( hashedToken.subtype & ~TT_VALUESVALID ) == ( token.subtype & ~TT_VALUESVALID ) ) && ( hashedToken.linesCrossed == token.linesCrossed ) &&
			( hashedToken.WhiteSpaceBeforeToken() == token.WhiteSpaceBeforeToken() ) && ( hashedToken.flags == token.flags ) && ( hashedToken.Cmp( token.c_str() ) == 0 ) ) {
			return i;
		}
	}

	int index = uniqueTokens.Append( token );
	assert( index < USHRT_MAX );

	uniqueTokenHash.Add( hashKey, index );
	return index;
}

/*
============
anTokenCache::WriteFile
============
*/
bool anTokenCache::Write( anFile *f ) {
	assert( uniqueTokens.Num() < USHRT_MAX );
	f->WriteInt( uniqueTokens.Num() );
	for ( int i = 0; i < uniqueTokens.Num(); i++ ) {
		const anToken& token = uniqueTokens[i];
		f->WriteString( token.c_str() );
		assert( token.type < 255 );
		f->WriteChar( token.type );
		f->WriteInt( token.subtype & ~TT_VALUESVALID ); // force a recalculation of the value
		f->WriteInt( token.linesCrossed );
		f->WriteInt( token.flags );
		f->WriteChar( ( token.whiteSpaceEnd_p - token.whiteSpaceStart_p ) > 0 ? 1 : 0 );
	}

	uniqueTokenHash.Write( f );
	return true;
}

/*
============
anTokenCache::ReadBuffer
============
*/
bool anTokenCache::ReadBuffer( const byte *buffer, int length ) {
	assert( buffer != nullptr );
	assert( length > 0 );
	anLibFileMemoryPtr f( anLib::fileSystem->OpenMemoryFile( "[Token Cache] ReadBuffer" ) );
	f->SetData( reinterpret_cast< const char *>( buffer ), length );
	return Read( f.Get() );
}

/*
============
anTokenCache::ReadFile
============
*/
bool anTokenCache::Read( anFile *f ) {
	int numTokens;
	f->ReadInt( numTokens );
	anTokenCache newCache.uniqueTokens.SetNum( numTokens );

	// load individual tokens
	for ( int i = 0; i < numTokens; i++ ) {
		int c;
		anToken& token = newCache.uniqueTokens[i];
		f->ReadString( token );
		f->ReadChar( c );
		token.type = c;

		f->ReadInt( token.subtype );
		token.subtype &= ~TT_VALUESVALID;
		f->ReadInt( token.linesCrossed );
		f->ReadInt( token.flags );

		char whiteSpace;
		f->ReadChar( whiteSpace );
		token.whiteSpaceStart_p = nullptr;
		token.whiteSpaceEnd_p = ( const char *)whiteSpace;
	}

	assert( newCache.uniqueTokens.Num() == numTokens );
	newCache.uniqueTokenHash.Read( f );
	newCache.Swap( *this );
	return true;
}

/*
============
anBinaryLexer::AddToken
============
*/
void anBinaryLexer::AddToken( const anToken &token, anTokenCache *cache ) {
	if ( cache == nullptr ) {
		cache = &tokenCache;
	}

	unsigned short index = cache->FindToken( token );
	tokens.Append( index );
}

/*
============
anBinaryLexer::WriteFile
============
*/
bool anBinaryLexer::Write( anFile *f ) {
	f->WriteString( LEXB_VERSION );
	tokenCache.Write( f );
	f->WriteInt( tokens.Num() );
	for ( int i = 0; i < tokens.Num(); i++ ) {
		f->WriteUnsignedShort( tokens [i] );
	}
	isLoaded = true;
	return true;
}

/*
============
anBinaryLexer::ReadBuffer
============
*/
bool anBinaryLexer::ReadBuffer( const byte *buffer, int length ) {
	assert( buffer != nullptr );
	assert( length > 0 );
	anLibFileMemoryPtr f( anLib::fileSystem->OpenMemoryFile( "Binary-anBinaryLexer read buffer\n" ) );
	f->SetData( reinterpret_cast< const char *>( buffer ), length );
	return Read( f.Get() );
}

/*
============
anBinaryLexer::ReadFile
============
*/
bool anBinaryLexer::Read( anFile *f ) {
	isLoaded = false;

	try {
		// Header
		anStr temp;
		f->ReadString( temp );
		if ( temp.Cmp( LEXB_VERSION ) != 0 ) {
			anLib::common->Warning( "[Binary Lexer] ReadFile: expected '" LEXB_VERSION "' but found '%s'", temp.c_str() );
			return false;
		}

		anBinaryLexer newLexer.tokenCache.Read( f );

		int numIndices;
		f->ReadInt( numIndices );

		newLexer.tokens.SetNum( numIndices );
		for ( int i = 0; i < numIndices; i++ ) {
			f->ReadUnsignedShort( newLexer.tokens[i] );
		}
	} catch( arcException &exception ) {
		anLib::common->Warning( "%s", exception.error );
	}
	assert( f->Tell() == f->Length() );

	newLexer.isLoaded = true;

	newLexer.Swap( *this );
	return true;
}

/*
============
anBinaryLexer::ReadToken
============
*/
int anBinaryLexer::ReadToken( anToken *token ) {
	if ( !isLoaded ) {
		return 0;
	}
	const anList<unsigned short>* localTokens = tokensData != nullptr ? tokensData : &tokens;
	const anTokenCache* localTokenCache = tokenCacheData != nullptr ? tokenCacheData : &tokenCache;
	if ( nextToken >= localTokens->Num() ) {
		return 0;
	}

	*token = (* localTokenCache)[ (* localTokens)[ nextToken ] ];
	token->binaryIndex = (* localTokens)[ nextToken ];

	nextToken++;
	return 1;
}

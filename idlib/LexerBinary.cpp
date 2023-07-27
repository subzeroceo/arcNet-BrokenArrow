#include "../precompiled.h"
#pragma hdrstop
//#include "../../framework/Licensee.h"

/*
============
arcNetTokenCache::FindToken
============
*/
unsigned short arcNetTokenCache::FindToken( const arcNetToken &token ) {
	int hashKey = uniqueTokenHash.GenerateKey( token.c_str(), true );
	for ( int i = uniqueTokenHash.GetFirst( hashKey ); i != arcHashIndexUShort::NULL_INDEX; i = uniqueTokenHash.GetNext( i ) ) {
		// from mac version
		if ( ( i < 0 ) || ( i > uniqueTokens.Num() ) ) break;	// JWW - Added this line to circumvent assert/crash when 'i' is out-of-bounds
		const arcNetToken& hashedToken = uniqueTokens[ i ];
		if ( ( hashedToken.type == token.type ) &&
			( ( hashedToken.subtype & ~TT_VALUESVALID ) == ( token.subtype & ~TT_VALUESVALID ) ) &&
			( hashedToken.linesCrossed == token.linesCrossed ) &&
			( hashedToken.WhiteSpaceBeforeToken() == token.WhiteSpaceBeforeToken() ) &&
			( hashedToken.flags == token.flags ) &&
			( hashedToken.Cmp( token.c_str() ) == 0 ) ) {
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
arcNetTokenCache::WriteFile
============
*/
bool arcNetTokenCache::Write( anFile *f ) {
	assert( uniqueTokens.Num() < USHRT_MAX );

	f->WriteInt( uniqueTokens.Num() );
	int i;
	for ( i = 0; i < uniqueTokens.Num(); i++ ) {
		const arcNetToken& token = uniqueTokens[i];
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
arcNetTokenCache::ReadBuffer
============
*/
bool arcNetTokenCache::ReadBuffer( const byte *buffer, int length ) {
	assert( buffer != NULL );
	assert( length > 0 );

	sdLibFileMemoryPtr f( idLib::fileSystem->OpenMemoryFile( "arcNetTokenCache::ReadBuffer" ) );
	f->SetData( reinterpret_cast< const char* >( buffer ), length );
	return Read( f.Get() );
}

/*
============
arcNetTokenCache::ReadFile
============
*/
bool arcNetTokenCache::Read( anFile* f ) {
	int numTokens;
	f->ReadInt( numTokens );
	arcNetTokenCache newCache.uniqueTokens.SetNum( numTokens );

	// load individual tokens
	for ( int i = 0; i < numTokens; i++ ) {
		int c;
		arcNetToken& token = newCache.uniqueTokens[ i ];
		f->ReadString( token );

		f->ReadChar( c );
		token.type = c;

		f->ReadInt( token.subtype );
		token.subtype &= ~TT_VALUESVALID;
		f->ReadInt( token.linesCrossed );

		f->ReadInt( token.flags );

		char whiteSpace;
		f->ReadChar( whiteSpace );

		token.whiteSpaceStart_p = NULL;
		token.whiteSpaceEnd_p = ( const char* )whiteSpace;
	}

	assert( newCache.uniqueTokens.Num() == numTokens );

	newCache.uniqueTokenHash.Read( f );

	newCache.Swap( *this );
	return true;
}

/*
============
arcNetBinaryLexer::AddToken
============
*/
void arcNetBinaryLexer::AddToken( const arcNetToken &token, arcNetTokenCache *cache ) {
	if ( cache == NULL ) {
		cache = &tokenCache;
	}

	unsigned short index = cache->FindToken( token );
	tokens.Append( index );
}

/*
============
arcNetBinaryLexer::WriteFile
============
*/
bool arcNetBinaryLexer::Write( anFile* f ) {
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
arcNetBinaryLexer::ReadBuffer
============
*/
bool arcNetBinaryLexer::ReadBuffer( const byte *buffer, int length ) {
	assert( buffer != NULL );
	assert( length > 0 );

	sdLibFileMemoryPtr f( idLib::fileSystem->OpenMemoryFile( "Binary-Lexer read buffer\n" ) );
	f->SetData( reinterpret_cast< const char* >( buffer ), length );
	return Read( f.Get() );
}

/*
============
arcNetBinaryLexer::ReadFile
============
*/
bool arcNetBinaryLexer::Read( anFile *f ) {
	isLoaded = false;

	arcNetBinaryLexer newLexer;

	try {
		// Header
		anString temp;
		f->ReadString( temp );

		if ( temp.Cmp( LEXB_VERSION ) != 0 ) {
			idLib::common->Warning( "arcNetBinaryLexer::ReadFile: expected '" LEXB_VERSION "' but found '%s'", temp.c_str() );
			return false;
		}

		newLexer.tokenCache.Read( f );

		int numIndices;
		f->ReadInt( numIndices );

		newLexer.tokens.SetNum( numIndices );
		for ( int i = 0; i < numIndices; i++ ) {
			f->ReadUnsignedShort( newLexer.tokens[ i ] );
		}
	} catch( arcException &exception ) {
		idLib::common->Warning( "%s", exception.error );
	}
	assert( f->Tell() == f->Length() );

	newLexer.isLoaded = true;

	newLexer.Swap( *this );
	return true;
}

/*
============
arcNetBinaryLexer::ReadToken
============
*/
int arcNetBinaryLexer::ReadToken( arcNetToken *token ) {
	if ( !isLoaded ) {
		return 0;
	}
	const anList<unsigned short>* localTokens = tokensData != NULL ? tokensData : &tokens;
	const arcNetTokenCache* localTokenCache = tokenCacheData != NULL ? tokenCacheData : &tokenCache;

	if ( nextToken >= localTokens->Num() ) {
		return 0;
	}

	*token = (*localTokenCache)[ (*localTokens)[ nextToken ] ];
	token->binaryIndex = (*localTokens)[ nextToken ];

	nextToken++;
	return 1;
}

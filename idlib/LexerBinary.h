#ifndef __LEXERBINARY_H__
#define __LEXERBINARY_H__

/*
===============================================================================

	Binary anBinaryLexer

===============================================================================
*/

#define LEXB_EXTENSION		".binLex"
#define LEXB_VERSION		"BINLX00"

class anTokenCache {
public:
								anTokenCache();
								~anTokenCache();

	unsigned short				FindToken( const anToken &token );
	void						Swap( anTokenCache &rhs );
	void						Clear( void );

	bool						Write( anFile *f );
	bool						Read( anFile *f );
	bool						ReadBuffer( const byte *buffer, int length );
	int							Num( void ) const;
	size_t						Allocated( void ) const { return uniqueTokens.Allocated() + uniqueTokenHash.Allocated(); }

	const anToken &				operator[]( int index ) const { return uniqueTokens[ index ]; }

private:
	anList<anToken>				uniqueTokens;
	arcHashIndexUShort			uniqueTokenHash;
};

inline anTokenCache::anTokenCache() {
	uniqueTokens.SetGranularity( 128 );
	uniqueTokenHash.SetGranularity( 128 );
}

inline anTokenCache::~anTokenCache() {
}

inline void anTokenCache::Swap( anTokenCache& rhs ) {
	uniqueTokens.Swap( rhs.uniqueTokens );
	uniqueTokenHash.Swap( rhs.uniqueTokenHash );
}

inline void anTokenCache::Clear() {
	uniqueTokens.Clear();
	uniqueTokenHash.Clear();
}

inline int anTokenCache::Num( void ) const {
	return uniqueTokens.Num();
}

class anBinaryLexer {
public:
							anBinaryLexer();
							~anBinaryLexer();

	bool					Write( anFile *f );
	bool					Read( anFile *f );
	bool					ReadBuffer( const byte* buffer, int length );

	bool					IsLoaded( void ) const { return isLoaded; }
	int						EndOfFile( void ) const;

	void					AddToken( const anToken &token, anTokenCache *cache = nullptr );

	int						ReadToken( anToken *token );
	void					Swap( anBinaryLexer &rhs );

	void					Clear( void );
	void					ResetParsing( void );

	int						NumUniqueTokens( void ) const;
	int						NumTokens( void ) const;
	const anList<anToken>&	GetUniqueTokens( void ) const;

	void					SetData( const anList<unsigned short> *indices, const anTokenCache *cache );

	const anList<unsigned short>&
							GetTokenStream() const	{ return tokens; }

	size_t					Allocated( void ) const { return tokens.Allocated() + tokenCache.Allocated(); }

private:
	bool					isLoaded;
	int						nextToken;

	anTokenCache		tokenCache;
	anList<unsigned short>tokens;

									// allow clients to set their own data
	const anList<unsigned short> *tokensData;
	const anTokenCache *	tokenCacheData;
};

inline anBinaryLexer::anBinaryLexer( void ) {
	Clear();
	tokens.SetGranularity( 256 );
}

inline anBinaryLexer::~anBinaryLexer( void ) {
}

inline void anBinaryLexer::Swap( anBinaryLexer& rhs ) {
	tokens.Swap( rhs.tokens );
	tokenCache.Swap( rhs.tokenCache );
	::Swap( nextToken, rhs.nextToken );
	::Swap( isLoaded, rhs.isLoaded );
}

inline void anBinaryLexer::Clear( void ) {
	isLoaded = false;
	nextToken = 0;
	tokens.Clear();
	tokenCache.Clear();
	tokensData = nullptr;
	tokenCacheData = nullptr;
}

inline void anBinaryLexer::ResetParsing( void ) {
	nextToken = 0;
}

inline int anBinaryLexer::NumTokens( void ) const {
	return tokensData != nullptr ? tokensData->Num() : tokens.Num();
}

inline void anBinaryLexer::SetData( const anList<unsigned short>* indices, const anTokenCache* cache ) {
	tokenCacheData = cache;
	tokensData = indices;
	isLoaded = true;
}


inline int anBinaryLexer::EndOfFile() const {
	return nextToken >= tokens.Num();
}

#endif // !__LEXERBINARY_H__

#ifndef __LEXERBINARY_H__
#define __LEXERBINARY_H__

/*
===============================================================================

	Binary anBinaryLexer

===============================================================================
*/

#define LEXB_EXTENSION		".lxb"
#define LEXB_VERSION		"LXB01"

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

	const anToken &			operator[]( int index ) const { return uniqueTokens[ index ]; }

private:
	anList<anToken>		uniqueTokens;
	arcHashIndexUShort			uniqueTokenHash;
};

ARC_INLINE anTokenCache::anTokenCache() {
	uniqueTokens.SetGranularity( 128 );
	uniqueTokenHash.SetGranularity( 128 );
}

ARC_INLINE anTokenCache::~anTokenCache() {
}

ARC_INLINE void anTokenCache::Swap( anTokenCache& rhs ) {
	uniqueTokens.Swap( rhs.uniqueTokens );
	uniqueTokenHash.Swap( rhs.uniqueTokenHash );
}

ARC_INLINE void anTokenCache::Clear() {
	uniqueTokens.Clear();
	uniqueTokenHash.Clear();
}

ARC_INLINE int anTokenCache::Num( void ) const {
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

ARC_INLINE anBinaryLexer::anBinaryLexer( void ) {
	Clear();
	tokens.SetGranularity( 256 );
}

ARC_INLINE anBinaryLexer::~anBinaryLexer( void ) {
}

ARC_INLINE void anBinaryLexer::Swap( anBinaryLexer& rhs ) {
	tokens.Swap( rhs.tokens );
	tokenCache.Swap( rhs.tokenCache );
	::Swap( nextToken, rhs.nextToken );
	::Swap( isLoaded, rhs.isLoaded );
}

ARC_INLINE void anBinaryLexer::Clear( void ) {
	isLoaded = false;
	nextToken = 0;
	tokens.Clear();
	tokenCache.Clear();
	tokensData = nullptr;
	tokenCacheData = nullptr;
}

ARC_INLINE void anBinaryLexer::ResetParsing( void ) {
	nextToken = 0;
}

ARC_INLINE int anBinaryLexer::NumTokens( void ) const {
	return tokensData != nullptr ? tokensData->Num() : tokens.Num();
}

ARC_INLINE void anBinaryLexer::SetData( const anList<unsigned short>* indices, const anTokenCache* cache ) {
	tokenCacheData = cache;
	tokensData = indices;
	isLoaded = true;
}


ARC_INLINE int anBinaryLexer::EndOfFile() const {
	return nextToken >= tokens.Num();
}

#endif /* !__LEXERBINARY_H__ */

#ifndef __LEXERBINARY_H__
#define __LEXERBINARY_H__

/*
===============================================================================

	Binary Lexer

===============================================================================
*/

#define LEXB_EXTENSION		".lxb"
#define LEXB_VERSION		"LXB01"

class arcNetTokenCache {
public:
								arcNetTokenCache();
								~arcNetTokenCache();

	unsigned short				FindToken( const arcNetToken &token );
	void						Swap( arcNetTokenCache &rhs );
	void						Clear( void );

	bool						Write( anFile *f );
	bool						Read( anFile *f );
	bool						ReadBuffer( const byte *buffer, int length );
	int							Num( void ) const;
	size_t						Allocated( void ) const { return uniqueTokens.Allocated() + uniqueTokenHash.Allocated(); }

	const arcNetToken &			operator[]( int index ) const { return uniqueTokens[ index ]; }

private:
	anList<arcNetToken>		uniqueTokens;
	arcHashIndexUShort			uniqueTokenHash;
};

ARC_INLINE arcNetTokenCache::arcNetTokenCache() {
	uniqueTokens.SetGranularity( 128 );
	uniqueTokenHash.SetGranularity( 128 );
}

ARC_INLINE arcNetTokenCache::~arcNetTokenCache() {
}

ARC_INLINE void arcNetTokenCache::Swap( arcNetTokenCache& rhs ) {
	uniqueTokens.Swap( rhs.uniqueTokens );
	uniqueTokenHash.Swap( rhs.uniqueTokenHash );
}

ARC_INLINE void arcNetTokenCache::Clear() {
	uniqueTokens.Clear();
	uniqueTokenHash.Clear();
}

ARC_INLINE int arcNetTokenCache::Num( void ) const {
	return uniqueTokens.Num();
}

class arcNetBinaryLexer {
public:
							arcNetBinaryLexer();
							~arcNetBinaryLexer();

	bool					Write( anFile *f );
	bool					Read( anFile *f );
	bool					ReadBuffer( const byte* buffer, int length );

	bool					IsLoaded( void ) const { return isLoaded; }
	int						EndOfFile( void ) const;

	void					AddToken( const arcNetToken &token, arcNetTokenCache *cache = NULL );

	int						ReadToken( arcNetToken *token );
	void					Swap( arcNetBinaryLexer &rhs );

	void					Clear( void );
	void					ResetParsing( void );

	int						NumUniqueTokens( void ) const;
	int						NumTokens( void ) const;
	const anList<arcNetToken>&	GetUniqueTokens( void ) const;

	void					SetData( const anList<unsigned short> *indices, const arcNetTokenCache *cache );

	const anList<unsigned short>&
							GetTokenStream() const	{ return tokens; }

	size_t					Allocated( void ) const { return tokens.Allocated() + tokenCache.Allocated(); }

private:
	bool					isLoaded;
	int						nextToken;

	arcNetTokenCache		tokenCache;
	anList<unsigned short>tokens;

									// allow clients to set their own data
	const anList<unsigned short> *tokensData;
	const arcNetTokenCache *	tokenCacheData;
};

ARC_INLINE arcNetBinaryLexer::arcNetBinaryLexer( void ) {
	Clear();
	tokens.SetGranularity( 256 );
}

ARC_INLINE arcNetBinaryLexer::~arcNetBinaryLexer( void ) {
}

ARC_INLINE void arcNetBinaryLexer::Swap( arcNetBinaryLexer& rhs ) {
	tokens.Swap( rhs.tokens );
	tokenCache.Swap( rhs.tokenCache );
	::Swap( nextToken, rhs.nextToken );
	::Swap( isLoaded, rhs.isLoaded );
}

ARC_INLINE void arcNetBinaryLexer::Clear( void ) {
	isLoaded = false;
	nextToken = 0;
	tokens.Clear();
	tokenCache.Clear();
	tokensData = NULL;
	tokenCacheData = NULL;
}

ARC_INLINE void arcNetBinaryLexer::ResetParsing( void ) {
	nextToken = 0;
}

ARC_INLINE int arcNetBinaryLexer::NumTokens( void ) const {
	return tokensData != NULL ? tokensData->Num() : tokens.Num();
}

ARC_INLINE void arcNetBinaryLexer::SetData( const anList<unsigned short>* indices, const arcNetTokenCache* cache ) {
	tokenCacheData = cache;
	tokensData = indices;
	isLoaded = true;
}


ARC_INLINE int arcNetBinaryLexer::EndOfFile() const {
	return nextToken >= tokens.Num();
}

#endif /* !__LEXERBINARY_H__ */

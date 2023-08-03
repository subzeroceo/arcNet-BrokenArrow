#ifndef __STRPOOL_H__
#define __STRPOOL_H__

/*
===============================================================================

	anStringPool

===============================================================================
*/

class anStringPool;

class ARCPoolString : public anString {
	friend class anStringPool;

public:
						ARCPoolString() { numUsers = 0; }
						~ARCPoolString() { assert( numUsers == 0 ); }

						// returns total size of allocated memory
	size_t				Allocated( void ) const { return anString::Allocated(); }
						// returns total size of allocated memory including size of string pool type
	size_t				Size( void ) const { return sizeof( *this ) + Allocated(); }
						// returns a pointer to the pool this string was allocated from
	const anStringPool *	GetPool( void ) const { return pool; }

private:
	anStringPool *			pool;
	mutable int			numUsers;
};

class anStringPool {
public:
						anStringPool() { caseSensitive = true; }

	void				SetCaseSensitive( bool caseSensitive );

	int					Num( void ) const { return pool.Num(); }
	size_t				Allocated( void ) const;
	size_t				Size( void ) const;

	const ARCPoolString *	operator[]( int index ) const { return pool[index]; }

	const ARCPoolString *	AllocString( const char *string );
	void				FreeString( const ARCPoolString *poolStr );
	const ARCPoolString *	CopyString( const ARCPoolString *poolStr );
	void				Clear( void );

private:
	bool				caseSensitive;
	anList<ARCPoolString *>	pool;
	anHashIndex			poolHash;
};

/*
================
anStringPool::SetCaseSensitive
================
*/
ARC_INLINE void anStringPool::SetCaseSensitive( bool caseSensitive ) {
	this->caseSensitive = caseSensitive;
}

/*
================
anStringPool::AllocString
================
*/
ARC_INLINE const ARCPoolString *anStringPool::AllocString( const char *string ) {
	int i, hash;
	ARCPoolString *poolStr;

	hash = poolHash.GenerateKey( string, caseSensitive );
	if ( caseSensitive ) {
		for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
			if ( pool[i]->Cmp( string ) == 0 ) {
				pool[i]->numUsers++;
				return pool[i];
			}
		}
	} else {
		for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
			if ( pool[i]->Icmp( string ) == 0 ) {
				pool[i]->numUsers++;
				return pool[i];
			}
		}
	}

	poolStr = new ARCPoolString;
	*static_cast<anString *>(poolStr) = string;
	poolStr->pool = this;
	poolStr->numUsers = 1;
	poolHash.Add( hash, pool.Append( poolStr ) );
	return poolStr;
}

/*
================
anStringPool::FreeString
================
*/
ARC_INLINE void anStringPool::FreeString( const ARCPoolString *poolStr ) {
	int i, hash;

	assert( poolStr->numUsers >= 1 );
	assert( poolStr->pool == this );

	poolStr->numUsers--;
	if ( poolStr->numUsers <= 0 ) {
		hash = poolHash.GenerateKey( poolStr->c_str(), caseSensitive );
		if ( caseSensitive ) {
			for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
				if ( pool[i]->Cmp( poolStr->c_str() ) == 0 ) {
					break;
				}
			}
		} else {
			for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
				if ( pool[i]->Icmp( poolStr->c_str() ) == 0 ) {
					break;
				}
			}
		}
		assert( i != -1 );
		assert( pool[i] == poolStr );
		delete pool[i];
		pool.RemoveIndex( i );
		poolHash.RemoveIndex( hash, i );
	}
}

/*
================
anStringPool::CopyString
================
*/
ARC_INLINE const ARCPoolString *anStringPool::CopyString( const ARCPoolString *poolStr ) {

	assert( poolStr->numUsers >= 1 );

	if ( poolStr->pool == this ) {
		// the string is from this pool so just increase the user count
		poolStr->numUsers++;
		return poolStr;
	} else {
		// the string is from another pool so it needs to be re-allocated from this pool.
		return AllocString( poolStr->c_str() );
	}
}

/*
================
anStringPool::Clear
================
*/
ARC_INLINE void anStringPool::Clear( void ) {
	int i;

	for ( i = 0; i < pool.Num(); i++ ) {
		pool[i]->numUsers = 0;
	}
	pool.DeleteContents( true );
	poolHash.Free();
}

/*
================
anStringPool::Allocated
================
*/
ARC_INLINE size_t anStringPool::Allocated( void ) const {
	int i;
	size_t size;

	size = pool.Allocated() + poolHash.Allocated();
	for ( i = 0; i < pool.Num(); i++ ) {
		size += pool[i]->Allocated();
	}
	return size;
}

/*
================
anStringPool::Size
================
*/
ARC_INLINE size_t anStringPool::Size( void ) const {
	int i;
	size_t size;

	size = pool.Size() + poolHash.Size();
	for ( i = 0; i < pool.Num(); i++ ) {
		size += pool[i]->Size();
	}
	return size;
}

#endif /* !__STRPOOL_H__ */

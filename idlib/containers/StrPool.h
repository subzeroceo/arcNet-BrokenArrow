#ifndef __STRPOOL_H__
#define __STRPOOL_H__

/*
===============================================================================

	anStringPool

===============================================================================
*/

class anStringPool;

class anPoolString : public anStr {
	friend class anStringPool;

public:
							anPoolString() { numUsers = 0; }
							~anPoolString() { assert( numUsers == 0 ); }

							// returns total size of allocated memory
	size_t					Allocated( void ) const { return anStr::Allocated(); }
							// returns total size of allocated memory including size of string pool type
	size_t					Size( void ) const { return sizeof( *this ) + Allocated(); }
							// returns a pointer to the pool this string was allocated from
	const anStringPool *	GetPool( void ) const { return pool; }

private:
	anStringPool *			pool;
	mutable int				numUsers;
};

class anStringPool {
public:
							anStringPool() { caseSensitive = true; }

	void					SetCaseSensitive( bool caseSensitive );

	int						Num( void ) const { return pool.Num(); }
	size_t					Allocated( void ) const;
	size_t					Size( void ) const;

	const anPoolString *		operator[]( int index ) const { return pool[index]; }

	const anPoolString *	AllocString( const char *string );
	void					FreeString( const anPoolString *poolStr );
	const anPoolString *	CopyString( const anPoolString *poolStr );
	void					Clear( void );

private:
	bool					caseSensitive;
	anList<anPoolString *>	pool;
	anHashIndex				poolHash;
};

/*
================
anStringPool::SetCaseSensitive
================
*/
inline void anStringPool::SetCaseSensitive( bool caseSensitive ) {
	this->caseSensitive = caseSensitive;
}

/*
================
anStringPool::AllocString
================
*/
inline const anPoolString *anStringPool::AllocString( const char *string ) {
	int i, hash;
	anPoolString *poolStr;

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

	poolStr = new anPoolString;
	*static_cast<anStr *>(poolStr) = string;
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
inline void anStringPool::FreeString( const anPoolString *poolStr ) {
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
inline const anPoolString *anStringPool::CopyString( const anPoolString *poolStr ) {
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
inline void anStringPool::Clear( void ) {
	for ( int i = 0; i < pool.Num(); i++ ) {
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
inline size_t anStringPool::Allocated( void ) const {
	size_t size = pool.Allocated() + poolHash.Allocated();
	for ( int i = 0; i < pool.Num(); i++ ) {
		size += pool[i]->Allocated();
	}
	return size;
}

/*
================
anStringPool::Size
================
*/
inline size_t anStringPool::Size( void ) const {
	size_t size = pool.Size() + poolHash.Size();
	for ( int i = 0; i < pool.Num(); i++ ) {
		size += pool[i]->Size();
	}
	return size;
}

#endif // !__STRPOOL_H__
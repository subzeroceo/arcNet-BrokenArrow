#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

/*
===============================================================================

	General hash table. Slower than anHashIndex but it can also be used for
	linked lists and other data structures than just indexes or arrays.

===============================================================================
*/

template<class Type>
class anHashTable {
public:
					anHashTable( int newtablesize = 256 );
					anHashTable( const anHashTable<Type> &map );
					~anHashTable( void );

					// returns total size of allocated memory
	size_t			Allocated( void ) const;
					// returns total size of allocated memory including size of hash table type
	size_t			Size( void ) const;

	void			Set( const char *key, Type &value );
	bool			Get( const char *key, Type **value = nullptr ) const;
	bool			Remove( const char *key );

	void			Clear( void );
	void			DeleteContents( void );

					// the entire contents can be itterated over, but note that the
					// exact index for a given element may change when new elements are added
	int				Num( void ) const;
	Type *			GetIndex( int index ) const;

	int				GetSpread( void ) const;

private:
	struct hashnode_s {
		anString	key;
		Type		value;
		hashnode_s * next;

		hashnode_s( const anString &k, Type v, hashnode_s *n ) : key( k ), value( v ), next( n ) {};
		hashnode_s( const char *k, Type v, hashnode_s *n ) : key( k ), value( v ), next( n ) {};
	};

	hashnode_s **	heads;

	int				tablesize;
	int				numentries;
	int				tablesizemask;

	int				GetHash( const char *key ) const;
};

/*
================
anHashTable<Type>::anHashTable
================
*/
template<class Type>
ARC_INLINE anHashTable<Type>::anHashTable( int newtablesize ) {
	assert( anMath::IsPowerOfTwo( newtablesize ) );

	tablesize = newtablesize;
	assert( tablesize > 0 );

	heads = new hashnode_s *[ tablesize ];
	memset( heads, 0, sizeof( *heads ) * tablesize );

	numentries = 0;
	tablesizemask = tablesize - 1;
}

/*
================
anHashTable<Type>::anHashTable
================
*/
template<class Type>
ARC_INLINE anHashTable<Type>::anHashTable( const anHashTable<Type> &map ) {
	assert( map.tablesize > 0 );
	tablesize		= map.tablesize;
	heads			= new hashnode_s *[ tablesize ];
	numentries		= map.numentries;
	tablesizemask	= map.tablesizemask;

	for ( int i = 0; i < tablesize; i++ ) {
		if ( !map.heads[i] ) {
			heads[i] = nullptr;
			continue;
		}

		hashnode_s **prev = &heads[i];
		for ( hashnode_s *node = map.heads[i]; node != nullptr; node = node->next ) {
			*prev = new hashnode_s( node->key, node->value, nullptr );
			prev = &( *prev )->next;
		}
	}
}

/*
================
anHashTable<Type>::~anHashTable<Type>
================
*/
template<class Type>
ARC_INLINE anHashTable<Type>::~anHashTable( void ) {
	Clear();
	delete[] heads;
}

/*
================
anHashTable<Type>::Allocated
================
*/
template<class Type>
ARC_INLINE size_t anHashTable<Type>::Allocated( void ) const {
	return sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
anHashTable<Type>::Size
================
*/
template<class Type>
ARC_INLINE size_t anHashTable<Type>::Size( void ) const {
	return sizeof( anHashTable<Type> ) + sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
anHashTable<Type>::GetHash
================
*/
template<class Type>
ARC_INLINE int anHashTable<Type>::GetHash( const char *key ) const {
	return ( anString::Hash( key ) & tablesizemask );
}

/*
================
anHashTable<Type>::Set
================
*/
template<class Type>
ARC_INLINE void anHashTable<Type>::Set( const char *key, Type &value ) {
	hashnode_s *node, **nextPtr;
	int hash = GetHash( key );
	for ( hashnode_s **nextPtr = &(heads[hash] ), hashnode_s *node = *nextPtr; node != nullptr; nextPtr = &(node->next), node = *nextPtr ) {
		int s = node->key.Cmp( key );
		if ( s == 0 ) {
			node->value = value;
			return;
		}
		if ( s > 0 ) {
			break;
		}
	}

	numentries++;

	*nextPtr = new hashnode_s( key, value, heads[ hash ] );
	(*nextPtr)->next = node;
}

/*
================
anHashTable<Type>::Get
================
*/
template<class Type>
ARC_INLINE bool anHashTable<Type>::Get( const char *key, Type **value ) const {
	int hash = GetHash( key );
	for ( hashnode_s *node = heads[ hash ]; node != nullptr; node = node->next ) {
		int s = node->key.Cmp( key );
		if ( s == 0 ) {
			if ( value ) {
				*value = &node->value;
			}
			return true;
		}
		if ( s > 0 ) {
			break;
		}
	}

	if ( value ) {
		*value = nullptr;
	}

	return false;
}

/*
================
anHashTable<Type>::GetIndex

the entire contents can be itterated over, but note that the
exact index for a given element may change when new elements are added
================
*/
template<class Type>
ARC_INLINE Type *anHashTable<Type>::GetIndex( int index ) const {
	if ( ( index < 0 ) || ( index > numentries ) ) {
		assert( 0 );
		return nullptr;
	}

	int count = 0;
	for ( int i = 0; i < tablesize; i++ ) {
		for ( hashnode_s *node = heads[i]; node != nullptr; node = node->next ) {
			if ( count == index ) {
				return &node->value;
			}
			count++;
		}
	}

	return nullptr;
}

/*
================
anHashTable<Type>::Remove
================
*/
template<class Type>
ARC_INLINE bool anHashTable<Type>::Remove( const char *key ) {
	hashnode_s	*prev;
	int hash = GetHash( key );
	hashnode_s	**head = &heads[ hash ];
	if ( *head ) {
		for ( hashnode_s *prev = nullptr, hashnode_s *node = *head; node != nullptr; prev = node, node = node->next ) {
			if ( node->key == key ) {
				if ( prev ) {
					prev->next = node->next;
				} else {
					*head = node->next;
				}

				delete node;
				numentries--;
				return true;
			}
		}
	}

	return false;
}

/*
================
anHashTable<Type>::Clear
================
*/
template<class Type>
ARC_INLINE void anHashTable<Type>::Clear( void ) {
	int			i;
	hashnode_s	*node;
	hashnode_s	*next;

	for ( i = 0; i < tablesize; i++ ) {
		next = heads[i];
		while( next != nullptr ) {
			node = next;
			next = next->next;
			delete node;
		}

		heads[i] = nullptr;
	}

	numentries = 0;
}

/*
================
anHashTable<Type>::DeleteContents
================
*/
template<class Type>
ARC_INLINE void anHashTable<Type>::DeleteContents( void ) {
	int			i;
	hashnode_s	*node;
	hashnode_s	*next;

	for ( i = 0; i < tablesize; i++ ) {
		next = heads[i];
		while( next != nullptr ) {
			node = next;
			next = next->next;
			delete node->value;
			delete node;
		}

		heads[i] = nullptr;
	}

	numentries = 0;
}

/*
================
anHashTable<Type>::Num
================
*/
template<class Type>
ARC_INLINE int anHashTable<Type>::Num( void ) const {
	return numentries;
}

#if defined(ARC_TYPEINFO)
#define __GNUC__ 99
#endif

#if !defined(__GNUC__) || __GNUC__ < 4
/*
================
anHashTable<Type>::GetSpread
================
*/
template<class Type>
int anHashTable<Type>::GetSpread( void ) const {
	int i, average, error, e;
	hashnode_s	*node;

	// if no items in hash
	if ( !numentries ) {
		return 100;
	}
	average = numentries / tablesize;
	error = 0;
	for ( i = 0; i < tablesize; i++ ) {
		numItems = 0;
		for ( node = heads[i]; node != nullptr; node = node->next ) {
			numItems++;
		}
		e = abs( numItems - average );
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	return 100 - ( error * 100 / numentries );
}
#endif

#if defined(ARC_TYPEINFO)
#undef __GNUC__
#endif

#endif // !__HASHTABLE_H__

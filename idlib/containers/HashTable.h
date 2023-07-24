#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

/*
===============================================================================

	General hash table. Slower than ARCHashIndex but it can also be used for
	linked lists and other data structures than just indexes or arrays.

===============================================================================
*/

template< class Type >
class arcHashTable {
public:
					arcHashTable( int newtablesize = 256 );
					arcHashTable( const arcHashTable<Type> &map );
					~arcHashTable( void );

					// returns total size of allocated memory
	size_t			Allocated( void ) const;
					// returns total size of allocated memory including size of hash table type
	size_t			Size( void ) const;

	void			Set( const char *key, Type &value );
	bool			Get( const char *key, Type **value = NULL ) const;
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
		arcNetString		key;
		Type		value;
		hashnode_s *next;

		hashnode_s( const arcNetString &k, Type v, hashnode_s *n ) : key( k ), value( v ), next( n ) {};
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
arcHashTable<Type>::arcHashTable
================
*/
template< class Type >
ARC_INLINE arcHashTable<Type>::arcHashTable( int newtablesize ) {

	assert( arcMath::IsPowerOfTwo( newtablesize ) );

	tablesize = newtablesize;
	assert( tablesize > 0 );

	heads = new hashnode_s *[ tablesize ];
	memset( heads, 0, sizeof( *heads ) * tablesize );

	numentries		= 0;

	tablesizemask = tablesize - 1;
}

/*
================
arcHashTable<Type>::arcHashTable
================
*/
template< class Type >
ARC_INLINE arcHashTable<Type>::arcHashTable( const arcHashTable<Type> &map ) {
	int			i;
	hashnode_s	*node;
	hashnode_s	**prev;

	assert( map.tablesize > 0 );

	tablesize		= map.tablesize;
	heads			= new hashnode_s *[ tablesize ];
	numentries		= map.numentries;
	tablesizemask	= map.tablesizemask;

	for ( i = 0; i < tablesize; i++ ) {
		if ( !map.heads[ i ] ) {
			heads[ i ] = NULL;
			continue;
		}

		prev = &heads[ i ];
		for ( node = map.heads[ i ]; node != NULL; node = node->next ) {
			*prev = new hashnode_s( node->key, node->value, NULL );
			prev = &( *prev )->next;
		}
	}
}

/*
================
arcHashTable<Type>::~arcHashTable<Type>
================
*/
template< class Type >
ARC_INLINE arcHashTable<Type>::~arcHashTable( void ) {
	Clear();
	delete[] heads;
}

/*
================
arcHashTable<Type>::Allocated
================
*/
template< class Type >
ARC_INLINE size_t arcHashTable<Type>::Allocated( void ) const {
	return sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
arcHashTable<Type>::Size
================
*/
template< class Type >
ARC_INLINE size_t arcHashTable<Type>::Size( void ) const {
	return sizeof( arcHashTable<Type> ) + sizeof( heads ) * tablesize + sizeof( *heads ) * numentries;
}

/*
================
arcHashTable<Type>::GetHash
================
*/
template< class Type >
ARC_INLINE int arcHashTable<Type>::GetHash( const char *key ) const {
	return ( arcNetString::Hash( key ) & tablesizemask );
}

/*
================
arcHashTable<Type>::Set
================
*/
template< class Type >
ARC_INLINE void arcHashTable<Type>::Set( const char *key, Type &value ) {
	hashnode_s *node, **nextPtr;
	int hash, s;

	hash = GetHash( key );
	for ( nextPtr = &(heads[hash] ), node = *nextPtr; node != NULL; nextPtr = &(node->next), node = *nextPtr ) {
		s = node->key.Cmp( key );
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
arcHashTable<Type>::Get
================
*/
template< class Type >
ARC_INLINE bool arcHashTable<Type>::Get( const char *key, Type **value ) const {
	hashnode_s *node;
	int hash, s;

	hash = GetHash( key );
	for ( node = heads[ hash ]; node != NULL; node = node->next ) {
		s = node->key.Cmp( key );
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
		*value = NULL;
	}

	return false;
}

/*
================
arcHashTable<Type>::GetIndex

the entire contents can be itterated over, but note that the
exact index for a given element may change when new elements are added
================
*/
template< class Type >
ARC_INLINE Type *arcHashTable<Type>::GetIndex( int index ) const {
	hashnode_s	*node;
	int			count;
	int			i;

	if ( ( index < 0 ) || ( index > numentries ) ) {
		assert( 0 );
		return NULL;
	}

	count = 0;
	for ( i = 0; i < tablesize; i++ ) {
		for ( node = heads[ i ]; node != NULL; node = node->next ) {
			if ( count == index ) {
				return &node->value;
			}
			count++;
		}
	}

	return NULL;
}

/*
================
arcHashTable<Type>::Remove
================
*/
template< class Type >
ARC_INLINE bool arcHashTable<Type>::Remove( const char *key ) {
	hashnode_s	**head;
	hashnode_s	*node;
	hashnode_s	*prev;
	int			hash;

	hash = GetHash( key );
	head = &heads[ hash ];
	if ( *head ) {
		for ( prev = NULL, node = *head; node != NULL; prev = node, node = node->next ) {
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
arcHashTable<Type>::Clear
================
*/
template< class Type >
ARC_INLINE void arcHashTable<Type>::Clear( void ) {
	int			i;
	hashnode_s	*node;
	hashnode_s	*next;

	for ( i = 0; i < tablesize; i++ ) {
		next = heads[ i ];
		while( next != NULL ) {
			node = next;
			next = next->next;
			delete node;
		}

		heads[ i ] = NULL;
	}

	numentries = 0;
}

/*
================
arcHashTable<Type>::DeleteContents
================
*/
template< class Type >
ARC_INLINE void arcHashTable<Type>::DeleteContents( void ) {
	int			i;
	hashnode_s	*node;
	hashnode_s	*next;

	for ( i = 0; i < tablesize; i++ ) {
		next = heads[ i ];
		while( next != NULL ) {
			node = next;
			next = next->next;
			delete node->value;
			delete node;
		}

		heads[ i ] = NULL;
	}

	numentries = 0;
}

/*
================
arcHashTable<Type>::Num
================
*/
template< class Type >
ARC_INLINE int arcHashTable<Type>::Num( void ) const {
	return numentries;
}

#if defined(ID_TYPEINFO)
#define __GNUC__ 99
#endif

#if !defined(__GNUC__) || __GNUC__ < 4
/*
================
arcHashTable<Type>::GetSpread
================
*/
template< class Type >
int arcHashTable<Type>::GetSpread( void ) const {
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
		for ( node = heads[ i ]; node != NULL; node = node->next ) {
			numItems++;
		}
		e = abs( numItems - average );
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	return 100 - (error * 100 / numentries);
}
#endif

#if defined(ID_TYPEINFO)
#undef __GNUC__
#endif

#endif /* !__HASHTABLE_H__ */

#ifndef __STATICLIST_H__
#define __STATICLIST_H__

/*
===============================================================================

	Static list template
	A non-growing, memset-able list using no memory allocation.

===============================================================================
*/

template<class type,int size>
class anStaticList {
public:

						anStaticList();
						anStaticList( const anStaticList<type,size> &other );
						~anStaticList<type,size>( void );

	void				Clear( void );										// marks the list as empty.  does not deallocate or intialize data.
	int					Num( void ) const;									// returns number of elements in list
	int					Max( void ) const;									// returns the maximum number of elements in the list
	void				SetNum( int newNum );								// set number of elements in list

	size_t				Allocated( void ) const;							// returns total size of allocated memory
	size_t				Size( void ) const;									// returns total size of allocated memory including size of list type
	size_t				MemoryUsed( void ) const;							// returns size of the used elements in the list

	const type &		operator[]( int index ) const;
	type &				operator[]( int index );

	type *				Ptr( void );										// returns a pointer to the list
	const type *		Ptr( void ) const;									// returns a pointer to the list
	type *				Alloc( void );										// returns reference to a new data element at the end of the list.  returns nullptr when full.
	int					Append( const type & obj );							// append element
	int					Append( const anStaticList<type,size> &other );		// append list
	int					AddUnique( const type & obj );						// add unique element
	int					Insert( const type & obj, int index );				// insert the element at the given index
	int					FindIndex( const type & obj ) const;				// find the index for the given element
	type *				Find( type const & obj ) const;						// find pointer to the given element
	int					FindNull( void ) const;								// find the index for the first nullptr pointer in the list
	int					IndexOf( const type *obj ) const;					// returns the index for the pointer to an element in the list
	bool				RemoveIndex( int index );							// remove the element at the given index
	bool				Remove( const type & obj );							// remove the element
	void				Swap( anStaticList<type,size> &other );				// swap the contents of the lists
	void				DeleteContents( bool clear );						// delete the contents of the list

private:
	int					num;
	type 				list[ size ];
};

/*
================
anStaticList<type,size>::anStaticList()
================
*/
template<class type,int size>
inline anStaticList<type,size>::anStaticList() {
	num = 0;
}

/*
================
anStaticList<type,size>::anStaticList( const anStaticList<type,size> &other )
================
*/
template<class type,int size>
inline anStaticList<type,size>::anStaticList( const anStaticList<type,size> &other ) {
	*this = other;
}

/*
================
anStaticList<type,size>::~anStaticList<type,size>
================
*/
template<class type,int size>
inline anStaticList<type,size>::~anStaticList( void ) {
}

/*
================
anStaticList<type,size>::Clear

Sets the number of elements in the list to 0.  Assumes that type automatically handles freeing up memory.
================
*/
template<class type,int size>
inline void anStaticList<type,size>::Clear( void ) {
	num	= 0;
}

/*
================
anStaticList<type,size>::DeleteContents

Calls the destructor of all elements in the list.  Conditionally frees up memory used by the list.
Note that this only works on lists containing pointers to objects and will cause a compiler error
if called with non-pointers.  Since the list was not responsible for allocating the object, it has
no information on whether the object still exists or not, so care must be taken to ensure that
the pointers are still valid when this function is called.  Function will set all pointers in the
list to nullptr.
================
*/
template<class type,int size>
inline void anStaticList<type,size>::DeleteContents( bool clear ) {
	int i;

	for ( i = 0; i < size; i++ ) {
		delete list[i];
		list[i] = nullptr;
	}

	if ( clear ) {
		Clear();
	} else {
		memset( list, 0, sizeof( list ) );
	}
}

/*
================
anStaticList<type,size>::Num

Returns the number of elements currently contained in the list.
================
*/
template<class type,int size>
inline int anStaticList<type,size>::Num( void ) const {
	return num;
}

/*
================
anStaticList<type,size>::Num

Returns the maximum number of elements in the list.
================
*/
template<class type,int size>
inline int anStaticList<type,size>::Max( void ) const {
	return size;
}

/*
================
anStaticList<type>::Allocated
================
*/
template<class type,int size>
inline size_t anStaticList<type,size>::Allocated( void ) const {
	return size * sizeof( type );
}

/*
================
anStaticList<type>::Size
================
*/
template<class type,int size>
inline size_t anStaticList<type,size>::Size( void ) const {
	return sizeof( anStaticList<type,size> ) + Allocated();
}

/*
================
anStaticList<type,size>::Num
================
*/
template<class type,int size>
inline size_t anStaticList<type,size>::MemoryUsed( void ) const {
	return num * sizeof( list[0] );
}

/*
================
anStaticList<type,size>::SetNum

Set number of elements in list.
================
*/
template<class type,int size>
inline void anStaticList<type,size>::SetNum( int newNum ) {
	assert( newNum >= 0 );
	assert( newNum <= size );
	num = newNum;
}

/*
================
anStaticList<type,size>::operator[] const

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template<class type,int size>
inline const type &anStaticList<type,size>::operator[]( int index ) const {
	assert( index >= 0 );
	assert( index < num );

	return list[index];
}

/*
================
anStaticList<type,size>::operator[]

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template<class type,int size>
inline type &anStaticList<type,size>::operator[]( int index ) {
	assert( index >= 0 );
	assert( index < num );

	return list[index];
}

/*
================
anStaticList<type,size>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return nullptr if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template<class type,int size>
inline type *anStaticList<type,size>::Ptr( void ) {
	return &list[0];
}

/*
================
anStaticList<type,size>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return nullptr if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template<class type,int size>
inline const type *anStaticList<type,size>::Ptr( void ) const {
	return &list[0];
}

/*
================
anStaticList<type,size>::Alloc

Returns a pointer to a new data element at the end of the list.
================
*/
template<class type,int size>
inline type *anStaticList<type,size>::Alloc( void ) {
	if ( num >= size ) {
		return nullptr;
	}

	return &list[ num++ ];
}

/*
================
anStaticList<type,size>::Append

Increases the size of the list by one element and copies the supplied data into it.

Returns the index of the new element, or -1 when list is full.
================
*/
template<class type,int size>
inline int anStaticList<type,size>::Append( type const & obj ) {
	assert( num < size );
	if ( num < size ) {
		list[ num ] = obj;
		num++;
		return num - 1;
	}

	return -1;
}


/*
================
anStaticList<type,size>::Insert

Increases the size of the list by at leat one element if necessary
and inserts the supplied data into it.

Returns the index of the new element, or -1 when list is full.
================
*/
template<class type,int size>
inline int anStaticList<type,size>::Insert( type const & obj, int index ) {
	int i;

	assert( num < size );
	if ( num >= size ) {
		return -1;
	}

	assert( index >= 0 );
	if ( index < 0 ) {
		index = 0;
	} else if ( index > num ) {
		index = num;
	}

	for ( i = num; i > index; --i ) {
		list[i] = list[i-1];
	}

	num++;
	list[index] = obj;
	return index;
}

/*
================
anStaticList<type,size>::Append

adds the other list to this one

Returns the size of the new combined list
================
*/
template<class type,int size>
inline int anStaticList<type,size>::Append( const anStaticList<type,size> &other ) {
	int i;
	int n = other.Num();

	if ( num + n > size ) {
		n = size - num;
	}
	for ( i = 0; i < n; i++ ) {
		list[i + num] = other.list[i];
	}
	num += n;
	return Num();
}

/*
================
anStaticList<type,size>::AddUnique

Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
================
*/
template<class type,int size>
inline int anStaticList<type,size>::AddUnique( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index < 0 ) {
		index = Append( obj );
	}

	return index;
}

/*
================
anStaticList<type,size>::FindIndex

Searches for the specified data in the list and returns it's index.  Returns -1 if the data is not found.
================
*/
template<class type,int size>
inline int anStaticList<type,size>::FindIndex( type const & obj ) const {
	int i;

	for ( i = 0; i < num; i++ ) {
		if ( list[i] == obj ) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
anStaticList<type,size>::Find

Searches for the specified data in the list and returns it's address. Returns nullptr if the data is not found.
================
*/
template<class type,int size>
inline type *anStaticList<type,size>::Find( type const & obj ) const {
	int i;

	i = FindIndex( obj );
	if ( i >= 0 ) {
		return &list[i];
	}

	return nullptr;
}

/*
================
anStaticList<type,size>::FindNull

Searches for a nullptr pointer in the list.  Returns -1 if nullptr is not found.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template<class type,int size>
inline int anStaticList<type,size>::FindNull( void ) const {
	int i;

	for ( i = 0; i < num; i++ ) {
		if ( list[i] == nullptr ) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
anStaticList<type,size>::IndexOf

Takes a pointer to an element in the list and returns the index of the element.
This is NOT a guarantee that the object is really in the list.
Function will assert in debug builds if pointer is outside the bounds of the list,
but remains silent in release builds.
================
*/
template<class type,int size>
inline int anStaticList<type,size>::IndexOf( type const *objptr ) const {
	int index;

	index = objptr - list;

	assert( index >= 0 );
	assert( index < num );

	return index;
}

/*
================
anStaticList<type,size>::RemoveIndex

Removes the element at the specified index and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template<class type,int size>
inline bool anStaticList<type,size>::RemoveIndex( int index ) {
	int i;

	assert( index >= 0 );
	assert( index < num );

	if ( ( index < 0 ) || ( index >= num ) ) {
		return false;
	}

	num--;
	for ( i = index; i < num; i++ ) {
		list[i] = list[ i + 1 ];
	}

	return true;
}

/*
================
anStaticList<type,size>::Remove

Removes the element if it is found within the list and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template<class type,int size>
inline bool anStaticList<type,size>::Remove( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index >= 0 ) {
		return RemoveIndex( index );
	}

	return false;
}

/*
================
anStaticList<type,size>::Swap

Swaps the contents of two lists
================
*/
template<class type,int size>
inline void anStaticList<type,size>::Swap( anStaticList<type,size> &other ) {
	anStaticList<type,size> temp = *this;
	*this = other;
	other = temp;
}

#endif // !__STATICLIST_H__

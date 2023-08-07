#ifndef __LIST_H__
#define __LIST_H__

/*
===============================================================================

	List template
	Does not allocate memory until the first item is added.

===============================================================================
*/

/*
================
anListSortCompare<type>
================
*/
#ifdef __INTEL_COMPILER
// the intel compiler doesn't do the right thing here
template<class type>
inline int anListSortCompare( const type *a, const type *b ) {
	assert( 0 );
	return 0;
}
#else
template<class type>
inline int anListSortCompare( const type *a, const type *b ) {
	return *a - *b;
}
#endif

/*
================
anListNewElement<type>
================
*/
template<class type>
inline type *anListNewElement( void ) {
	return new type;
}

/*
================
anSwap<type>
================
*/
template<class type>
inline void anSwap( type &a, type &b ) {
	type c = a;
	a = b;
	b = c;
}

template<class type>
class anList {
public:

	typedef int		cmp_t( const type *, const type * );
	typedef type	new_t( void );

					anList( int newGranularity = 16 );
					anList( const anList<type> &other );
					~anList<type>( void );

	void			Clear( void );										// clear the list
	int				Num( void ) const;									// returns number of elements in list
	int				NumAllocated( void ) const;							// returns number of elements allocated for
	void			SetGranularity( int newGranularity );				// set new granularity
	int				GetGranularity( void ) const;						// get the current granularity

	size_t			Allocated( void ) const;							// returns total size of allocated memory
	size_t			Size( void ) const;									// returns total size of allocated memory including size of list type
	size_t			MemoryUsed( void ) const;							// returns size of the used elements in the list

	anList<type> &	operator=( const anList<type> &other );
	const type &	operator[]( int index ) const;
	type &			operator[]( int index );

	void			Condense( void );									// resizes list to exactly the number of elements it contains
	void			Resize( int newSize );								// resizes list to the given number of elements
	void			Resize( int newSize, int newGranularity	 );			// resizes list and sets new granularity

	void			SetNum( int newNum, bool resize = true );			// set number of elements in list and resize to exactly this number if necessary
	void			SetNum( int total );

	void			AssureSize( int newSize);							// assure list has given number of elements, but leave them uninitialized
	void			AssureSize( int newSize, const type &initValue );	// assure list has given number of elements and initialize any new elements
	void			AssureSizeAlloc( int newSize, new_t *allocator );	// assure the pointer list has the given number of elements and allocate any new elements

	type *			Ptr( void );										// returns a pointer to the list
	const type *	Ptr( void ) const;									// returns a pointer to the list

	type &			Alloc( void );										// returns reference to a new data element at the end of the list

	int				Append( const type & obj );							// append element
	int				Append( const anList<type> &other );				// append list
	int				AddUnique( const type & obj );						// add unique element

	int				Insert( const type & obj, int index = 0 );			// insert the element at the given index
	int				FindIndex( const type & obj ) const;				// find the index for the given element
	type *			Find( type const & obj ) const;						// find pointer to the given element
	int				FindNull( void ) const;								// find the index for the first nullptr pointer in the list

	int				IndexOf( const type *obj ) const;					// returns the index for the pointer to an element in the list

	bool			RemoveIndex( int index );							// remove the element at the given index
	bool			Remove( const type & obj );							// remove the element

	void			Sort( cmp_t *compare = ( cmp_t * )&anListSortCompare<type> );
	void			SortSubSection( int startIndex, int endIndex, cmp_t *compare = ( cmp_t * )&anListSortCompare<type> );

	void			Swap( anList<type> &other );						// swap the contents of the lists

	void			DeleteContents( bool clear );						// delete the contents of the list

private:
	int				num;
	int				size;
	int				granularity;
	type *			list;
};

/*
================
anList::anList
================
*/
template<class type>
inline anList<type>::anList( int newGranularity ) {
	assert( newGranularity > 0 );
	list		= nullptr;
	granularity	= newGranularity;
	Clear();
}

/*
================
anList::anList
================
*/
template<class type>
inline anList<type>::anList( const anList<type> &other ) {
	list = nullptr;
	*this = other;
}

/*
================
anList::~anList
================
*/
template<class type>
inline anList<type>::~anList( void ) {
	Clear();
}

/*
================
anList::Clear

Frees up the memory allocated by the list.  Assumes that type automatically handles freeing up memory.
================
*/
template<class type>
inline void anList<type>::Clear( void ) {
	if ( list ) {
		delete[] list;
	}

	list	= nullptr;
	num		= 0;
	size	= 0;
}

/*
================
anList::DeleteContents

Calls the destructor of all elements in the list.  Conditionally frees up memory used by the list.
Note that this only works on lists containing pointers to objects and will cause a compiler error
if called with non-pointers.  Since the list was not responsible for allocating the object, it has
no information on whether the object still exists or not, so care must be taken to ensure that
the pointers are still valid when this function is called.  Function will set all pointers in the
list to nullptr.
================
*/
template<class type>
inline void anList<type>::DeleteContents( bool clear ) {
	for ( int i = 0; i < num; i++ ) {
		delete list[i];
		list[i] = nullptr;
	}

	if ( clear ) {
		Clear();
	} else {
		memset( list, 0, size * sizeof( type ) );
	}
}

/*
================
anList::Allocated

return total memory allocated for the list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template<class type>
inline size_t anList<type>::Allocated( void ) const {
	return size * sizeof( type );
}

/*
================
anList::Size

return total size of list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template<class type>
inline size_t anList<type>::Size( void ) const {
	return sizeof( anList<type> ) + Allocated();
}

/*
================
anList::MemoryUsed
================
*/
template<class type>
inline size_t anList<type>::MemoryUsed( void ) const {
	return num * sizeof( *list );
}

/*
================
anList::Num

Returns the number of elements currently contained in the list.
Note that this is NOT an indication of the memory allocated.
================
*/
template<class type>
inline int anList<type>::Num( void ) const {
	return num;
}

/*
================
anList::NumAllocated

Returns the number of elements currently allocated for.
================
*/
template<class type>
inline int anList<type>::NumAllocated( void ) const {
	return size;
}

/*
================
anList::SetNum

Resize to the exact size specified irregardless of granularity
================
*/
template<class type>
inline void anList<type>::SetNum( int newNum, bool resize ) {
	assert( newNum >= 0 );
	if ( resize || newNum > size ) {
		Resize( newNum );
	}
	num = newNum;
}

/*
================
idList<type>::SetNum
================
*/
template<class type>
inline void anList<type>::SetNum( int total ) {
	assert( num >= 0 );
	if ( total > size ) {
		// resize it up to the closest level of granularity
		Resize( ( ( num + granularity - 1 ) / granularity ) * granularity );
	}
	num = total;
}

/*
================
anList::SetGranularity

Sets the base size of the array and resizes the array to match.
================
*/
template<class type>
inline void anList<type>::SetGranularity( int newGranularity ) {
	int newSize;

	assert( newGranularity > 0 );
	granularity = newGranularity;

	if ( list ) {
		// resize it to the closest level of granularity
		newSize = num + granularity - 1;
		newSize -= newSize % granularity;
		if ( newSize != size ) {
			Resize( newSize );
		}
	}
}

/*
================
anList::GetGranularity

Get the current granularity.
================
*/
template<class type>
inline int anList<type>::GetGranularity( void ) const {
	return granularity;
}

/*
================
anList:Condense

Resizes the array to exactly the number of elements it contains or frees up memory if empty.
================
*/
template<class type>
inline void anList<type>::Condense( void ) {
	if ( list ) {
		if ( num ) {
			Resize( num );
		} else {
			Clear();
		}
	}
}

/*
================
anList::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correnctly instantiated.
================
*/
template<class type>
inline void anList<type>::Resize( int newSize ) {
	assert( newSize >= 0 );

	// free up the list if no data is being reserved
	if ( newSize <= 0 ) {
		Clear();
		return;
	}

	if ( newSize == size ) {
		// not changing the size, so just exit
		return;
	}

	type *temp = list;
	size = newSize;
	if ( size < num ) {
		num = size;
	}

	// copy the old list into our new one
	list = new type[ size ];
	for ( int i = 0; i < num; i++ ) {
		list[i] = temp[i];
	}

	// delete the old list if it exists
	if ( temp ) {
		delete[] temp;
	}
}

/*
================
anList::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correnctly instantiated.
================
*/
template<class type>
inline void anList<type>::Resize( int newSize, int newGranularity ) {
	assert( newSize >= 0 );

	assert( newGranularity > 0 );
	granularity = newGranularity;

	// free up the list if no data is being reserved
	if ( newSize <= 0 ) {
		Clear();
		return;
	}

	type *temp = list;
	size = newSize;
	if ( size < num ) {
		num = size;
	}

	// copy the old list into our new one
	list = new type[ size ];
	for ( int i = 0; i < num; i++ ) {
		list[i] = temp[i];
	}

	// delete the old list if it exists
	if ( temp ) {
		delete[] temp;
	}
}

/*
================
anList::AssureSize

Makes sure the list has at least the given number of elements.
================
*/
template<class type>
inline void anList<type>::AssureSize( int newSize ) {
	int newNum = newSize;

	if ( newSize > size ) {
		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		Resize( newSize );
	}

	num = newNum;
}

/*
================
anList::AssureSize

Makes sure the list has at least the given number of elements and initialize any elements not yet initialized.
================
*/
template<class type>
inline void anList<type>::AssureSize( int newSize, const type &initValue ) {
	int newNum = newSize;

	if ( newSize > size ) {
		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize( newSize );

		for ( int i = num; i < newSize; i++ ) {
			list[i] = initValue;
		}
	}

	num = newNum;
}

/*
================
anList::AssureSizeAlloc

Makes sure the list has at least the given number of elements and allocates any elements using the allocator.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template<class type>
inline void anList<type>::AssureSizeAlloc( int newSize, new_t *allocator ) {
	int newNum = newSize;

	if ( newSize > size ) {
		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize( newSize );

		for ( int i = num; i < newSize; i++ ) {
			list[i] = (*allocator)();
		}
	}

	num = newNum;
}

/*
================
anList::operator=

Copies the contents and size attributes of another list.
================
*/
template<class type>
inline anList<type> &anList<type>::operator=( const anList<type> &other ) {
	Clear();

	num			= other.num;
	size		= other.size;
	granularity	= other.granularity;

	if ( size ) {
		list = new type[ size ];
		for ( int i = 0; i < num; i++ ) {
			list[i] = other.list[i];
		}
	}

	return *this;
}

/*
================
anList::operator[]

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template<class type>
inline const type &anList<type>::operator[]( int index ) const {
	assert( index >= 0 );
	assert( index < num );

	return list[index];
}

/*
================
anList::operator[]

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template<class type>
inline type &anList<type>::operator[]( int index ) {
	assert( index >= 0 );
	assert( index < num );

	return list[index];
}

/*
================
anList::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return nullptr if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template<class type>
inline type *anList<type>::Ptr( void ) {
	return list;
}

/*
================
anList::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return nullptr if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template<class type>
const inline type *anList<type>::Ptr( void ) const {
	return list;
}

/*
================
anList::Alloc

Returns a reference to a new data element at the end of the list.
================
*/
template<class type>
inline type &anList<type>::Alloc( void ) {
	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		Resize( size + granularity );
	}

	return list[ num++ ];
}

/*
================
anList::Append

Increases the size of the list by one element and copies the supplied data into it.

Returns the index of the new element.
================
*/
template<class type>
inline int anList<type>::Append( type const & obj ) {
	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		int newSize;

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}
		newSize = size + granularity;
		Resize( newSize - newSize % granularity );
	}

	list[ num ] = obj;
	num++;

	return num - 1;
}

/*
================
anList::Insert

Increases the size of the list by at leat one element if necessary
and inserts the supplied data into it.

Returns the index of the new element.
================
*/
template<class type>
inline int anList<type>::Insert( type const & obj, int index ) {
	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		int newSize;

		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}
		newSize = size + granularity;
		Resize( newSize - newSize % granularity );
	}

	if ( index < 0 ) {
		index = 0;
	} else if ( index > num ) {
		index = num;
	}
	for ( int i = num; i > index; --i ) {
		list[i] = list[i-1];
	}
	num++;
	list[index] = obj;
	return index;
}

/*
================
anList::Append

adds the other list to this one

Returns the size of the new combined list
================
*/
template<class type>
inline int anList<type>::Append( const anList<type> &other ) {
	if ( !list ) {
		if ( granularity == 0 ) {	// this is a hack to fix our memset classes
			granularity = 16;
		}
		Resize( granularity );
	}

	int n = other.Num();
	for ( int i = 0; i < n; i++ ) {
		Append(other[i] );
	}

	return Num();
}

/*
================
anList::AddUnique

Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
================
*/
template<class type>
inline int anList<type>::AddUnique( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index < 0 ) {
		index = Append( obj );
	}

	return index;
}

/*
================
anList::FindIndex

Searches for the specified data in the list and returns it's index.  Returns -1 if the data is not found.
================
*/
template<class type>
inline int anList<type>::FindIndex( type const & obj ) const {
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
anList::Find

Searches for the specified data in the list and returns it's address. Returns nullptr if the data is not found.
================
*/
template<class type>
inline type *anList<type>::Find( type const & obj ) const {
	int i;

	i = FindIndex( obj );
	if ( i >= 0 ) {
		return &list[i];
	}

	return nullptr;
}

/*
================
anList::FindNull

Searches for a nullptr pointer in the list.  Returns -1 if nullptr is not found.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template<class type>
inline int anList<type>::FindNull( void ) const {
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
anList::IndexOf

Takes a pointer to an element in the list and returns the index of the element.
This is NOT a guarantee that the object is really in the list.
Function will assert in debug builds if pointer is outside the bounds of the list,
but remains silent in release builds.
================
*/
template<class type>
inline int anList<type>::IndexOf( type const *objptr ) const {
	int index;

	index = objptr - list;

	assert( index >= 0 );
	assert( index < num );

	return index;
}

/*
================
anList::RemoveIndex

Removes the element at the specified index and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template<class type>
inline bool anList<type>::RemoveIndex( int index ) {
	int i;

	assert( list != nullptr );
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
anList::Remove

Removes the element if it is found within the list and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template<class type>
inline bool anList<type>::Remove( type const & obj ) {
	int index;

	index = FindIndex( obj );
	if ( index >= 0 ) {
		return RemoveIndex( index );
	}

	return false;
}

/*
================
anList::Sort

Performs a qsort on the list using the supplied comparison function.  Note that the data is merely moved around the
list, so any pointers to data within the list may no longer be valid.
================
*/
template<class type>
inline void anList<type>::Sort( cmp_t *compare ) {
	if ( !list ) {
		return;
	}
	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort( ( void * )list, ( size_t )num, sizeof( type ), vCompare );
}

/*
================
anList::SortSubSection

Sorts a subsection of the list.
================
*/
template<class type>
inline void anList<type>::SortSubSection( int startIndex, int endIndex, cmp_t *compare ) {
	if ( !list ) {
		return;
	}
	if ( startIndex < 0 ) {
		startIndex = 0;
	}
	if ( endIndex >= num ) {
		endIndex = num - 1;
	}
	if ( startIndex >= endIndex ) {
		return;
	}
	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort( ( void * )( &list[startIndex] ), ( size_t )( endIndex - startIndex + 1 ), sizeof( type ), vCompare );
}

/*
================
anList::Swap

Swaps the contents of two lists
================
*/
template<class type>
inline void anList<type>::Swap( anList<type> &other ) {
	anSwap( num, other.num );
	anSwap( size, other.size );
	anSwap( granularity, other.granularity );
	anSwap( list, other.list );
}

#endif // !__LIST_H__
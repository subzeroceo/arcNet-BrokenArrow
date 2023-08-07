#ifndef __STRLIST_H__
#define __STRLIST_H__

/*
===============================================================================

	anStringList

===============================================================================
*/

typedef anList<anStr> anStringList;
typedef anList<anStr*> anStringPtrList;
typedef anStr *anStringPtr;

/*
================
anListSortCompare<anStringPtr>

Compares two pointers to strings. Used to sort a list of string pointers alphabetically in anList<anStr>::Sort.
================
*/
template<>
inline int anListSortCompare<anStringPtr>( const anStringPtr *a, const anStringPtr *b ) {
	return ( *a )->Icmp( **b );
}

/*
================
anStringList::Sort

Sorts the list of strings alphabetically. Creates a list of pointers to the actual strings and sorts the
pointer list. Then copies the strings into another list using the ordered list of pointers.
================
*/
template<>
inline void anStringList::Sort( cmp_t *compare ) {
	if ( !num ) {
		return;
	}

	anList<anStr>		other;
	anList<anStringPtr>	pointerList;

	pointerList.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		pointerList[i] = &( *this )[i];
	}

	pointerList.Sort();

	other.SetNum( num );
	other.SetGranularity( granularity );
	for ( int i = 0; i < other.Num(); i++ ) {
		other[i] = *pointerList[i];
	}

	this->Swap( other );
}

/*
================
anStringList::SortSubSection

Sorts a subsection of the list of strings alphabetically.
================
*/
template<>
inline void anStringList::SortSubSection( int startIndex, int endIndex, cmp_t *compare ) {
	if ( !num ) {
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

	anList<anStr>		other;
	anList<anStringPtr>	pointerList;

	int s = endIndex - startIndex + 1;
	other.SetNum( s );
	pointerList.SetNum( s );
	for ( int i = 0; i < s; i++ ) {
		other[i] = ( *this )[ startIndex + i ];
		pointerList[i] = &other[i];
	}

	pointerList.Sort();

	for ( int i = 0; i < s; i++ ) {
		(*this)[ startIndex + i ] = *pointerList[i];
	}
}

/*
================
anStringList::Size
================
*/
template<>
inline size_t anStringList::Size( void ) const {
	size_t s = sizeof( *this );
	for ( int i = 0; i < Num(); i++ ) {
		s += ( *this )[i].Size();
	}

	return s;
}

/*
===============================================================================

	anStringList path sorting

===============================================================================
*/

/*
================
anListSortComparePaths

Compares two pointers to strings. Used to sort a list of string pointers alphabetically in anList<anStr>::Sort.
================
*/
template<class anStringPtr>
inline int anListSortComparePaths( const anStringPtr *a, const anStringPtr *b ) {
	return ( *a )->IcmpPath( **b );
}

/*
================
anStringListSortPaths

Sorts the list of path strings alphabetically and makes sure folders come first.
================
*/
inline void anStringListSortPaths( anStringList &list ) {
	if ( !list.Num() ) {
		return;
	}

	anList<anStr>		other;
	anList<anStringPtr>	pointerList;

	pointerList.SetNum( list.Num() );
	for ( int i = 0; i < list.Num(); i++ ) {
		pointerList[i] = &list[i];
	}

	pointerList.Sort( anListSortComparePaths<anStringPtr> );

	other.SetNum( list.Num() );
	other.SetGranularity( list.GetGranularity() );
	for ( int i = 0; i < other.Num(); i++ ) {
		other[i] = *pointerList[i];
	}

	list.Swap( other );
}

#endif /* !__STRLIST_H__ */

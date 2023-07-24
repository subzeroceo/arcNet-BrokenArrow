#ifndef __STRLIST_H__
#define __STRLIST_H__

/*
===============================================================================

	arcStringList

===============================================================================
*/

typedef arcNetList<arcNetString> arcStringList;
typedef arcNetList<arcNetString*> idStrPtrList;
typedef arcNetString *idStrPtr;

/*
================
arcListSortCompare<idStrPtr>

Compares two pointers to strings. Used to sort a list of string pointers alphabetically in arcNetList<arcNetString>::Sort.
================
*/
template<>
ARC_INLINE int arcListSortCompare<idStrPtr>( const idStrPtr *a, const idStrPtr *b ) {
	return ( *a )->Icmp( **b );
}

/*
================
arcStringList::Sort

Sorts the list of strings alphabetically. Creates a list of pointers to the actual strings and sorts the
pointer list. Then copies the strings into another list using the ordered list of pointers.
================
*/
template<>
ARC_INLINE void arcStringList::Sort( cmp_t *compare ) {
	if ( !num ) {
		return;
	}

	arcNetList<arcNetString>		other;
	arcNetList<idStrPtr>	pointerList;

	pointerList.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		pointerList[ i ] = &( *this )[ i ];
	}

	pointerList.Sort();

	other.SetNum( num );
	other.SetGranularity( granularity );
	for ( int i = 0; i < other.Num(); i++ ) {
		other[ i ] = *pointerList[ i ];
	}

	this->Swap( other );
}

/*
================
arcStringList::SortSubSection

Sorts a subsection of the list of strings alphabetically.
================
*/
template<>
ARC_INLINE void arcStringList::SortSubSection( int startIndex, int endIndex, cmp_t *compare ) {
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

	arcNetList<arcNetString>		other;
	arcNetList<idStrPtr>	pointerList;

	int s = endIndex - startIndex + 1;
	other.SetNum( s );
	pointerList.SetNum( s );
	for ( int i = 0; i < s; i++ ) {
		other[ i ] = ( *this )[ startIndex + i ];
		pointerList[ i ] = &other[ i ];
	}

	pointerList.Sort();

	for ( int i = 0; i < s; i++ ) {
		(*this)[ startIndex + i ] = *pointerList[ i ];
	}
}

/*
================
arcStringList::Size
================
*/
template<>
ARC_INLINE size_t arcStringList::Size( void ) const {
	size_t s = sizeof( *this );
	for ( int i = 0; i < Num(); i++ ) {
		s += ( *this )[ i ].Size();
	}

	return s;
}

/*
===============================================================================

	arcStringList path sorting

===============================================================================
*/

/*
================
arcListSortComparePaths

Compares two pointers to strings. Used to sort a list of string pointers alphabetically in arcNetList<arcNetString>::Sort.
================
*/
template<class idStrPtr>
ARC_INLINE int arcListSortComparePaths( const idStrPtr *a, const idStrPtr *b ) {
	return ( *a )->IcmpPath( **b );
}

/*
================
idStrListSortPaths

Sorts the list of path strings alphabetically and makes sure folders come first.
================
*/
ARC_INLINE void idStrListSortPaths( arcStringList &list ) {
	if ( !list.Num() ) {
		return;
	}

	arcNetList<arcNetString>		other;
	arcNetList<idStrPtr>	pointerList;

	pointerList.SetNum( list.Num() );
	for ( int i = 0; i < list.Num(); i++ ) {
		pointerList[ i ] = &list[ i ];
	}

	pointerList.Sort( arcListSortComparePaths<idStrPtr> );

	other.SetNum( list.Num() );
	other.SetGranularity( list.GetGranularity() );
	for ( int i = 0; i < other.Num(); i++ ) {
		other[ i ] = *pointerList[ i ];
	}

	list.Swap( other );
}

#endif /* !__STRLIST_H__ */

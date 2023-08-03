#ifndef __SORT_H__
#define __SORT_H__

/*
================================================================================================
Contains the generic templated sort algorithms for quick-sort, heap-sort and insertion-sort.

The sort algorithms do not use class operators or overloaded functions to compare
objects because it is often desireable to sort the same objects in different ways
based on different keys (not just ascending and descending but sometimes based on
name and other times based on say priority). So instead, for each different sort a
separate class is implemented with a Compare() function.

This class is derived from one of the classes that implements a sort algorithm.
The Compare() member function does not only define how objects are sorted, the class
can also store additional data that can be used by the Compare() function. This, for
instance, allows a list of indices to be sorted where the indices point to objects
in an array. The base pointer of the array with objects can be stored on the class
that implements the Compare() function such that the Compare() function can use keys
that are stored on the objects.

The Compare() function is not virtual because this would incur significant overhead.
Do NOT make the Compare() function virtual on the derived class!
The sort implementations also explicitely call the Compare() function of the derived
class. This is to avoid various compiler bugs with using overloaded compare functions
and the inability of various compilers to find the right overloaded compare function.

To sort an array, an anList or an anStaticList, a new sort class, typically derived from
anSort_Quick, is implemented as follows:

class anSort_MySort : public anSort_Quick<anMyObject, anSort_MySort> {
public:
	int Compare( const anMyObject & a, const anMyObject & b ) const {
		if ( a should come before b ) {
			return -1; // or any negative integer
		} if ( a should come after b ) {
			return 1;  // or any positive integer
		} else {
			return 0;
		}
	}
};

To sort an array:

anMyObject array[100];
anSort_MySort().Sort( array, 100 );

To sort an anList:

anList<anMyObject> list;
list.Sort( anSort_MySort() );

The sort implementations never create temporaries of the template type. Only the
'SwapValues' template is used to move data around. This 'SwapValues' template can be
specialized to implement fast swapping of data. For instance, when sorting a list with
objects of some string class it is important to implement a specialized 'SwapValues' for
this string class to avoid excessive re-allocation and copying of strings.

================================================================================================
*/

/*
========================
SwapValues
========================
*/
template<typename _type_>
ARC_INLINE void SwapValues( _type_ & a, _type_ & b ) {
	_type_ c = a;
	a = b;
	b = c;
}

/*
================================================
anSort is an abstract template class for sorting an array of objects of the specified data type.
The array of objects is sorted such that: Compare( array[i], array[i+1] ) <= 0 for all i
================================================
*/
template<typename _type_>
class anSort {
public:
	virtual			~anSort() {}
	virtual void	Sort( _type_ * base, unsigned int num ) const = 0;
};

/*
================================================
anSort_Quick is a sort template that implements the
quick-sort algorithm on an array of objects of the specified data type.
================================================
*/
template<typename _type_, typename _derived_>
class anSort_Quick : public anSort<_type_> {
public:
	virtual void Sort( _type_ * base, unsigned int num ) const {
		if ( num <= 0 ) {
			return;
		}

		const int64 MAX_LEVELS = 128;
		int64 lo[MAX_LEVELS], hi[MAX_LEVELS];

		// 'lo' is the lower index, 'hi' is the upper index
		// of the region of the array that is being sorted.
		lo[0] = 0;
		hi[0] = num - 1;

		for ( int64 level = 0; level >= 0; ) {
			int64 i = lo[level];
			int64 j = hi[level];

			// Only use quick-sort when there are 4 or more elements in this region and we are below MAX_LEVELS.
			// Otherwise fall back to an insertion-sort.
			if ( ( ( j - i ) >= 4 ) && ( level < ( MAX_LEVELS - 1 ) ) ) {
				// Use the center element as the pivot.
				// The median of a multi point sample could be used
				// but simply taking the center works quite well.
				int64 pi = ( i + j ) / 2;

				// Move the pivot element to the end of the region.
				SwapValues( base[j], base[pi] );

				// Get a reference to the pivot element.
				_type_ & pivot = base[j--];

				// Partition the region.
				do {
					while ( static_cast<const _derived_ *>( this )->Compare( base[i], pivot ) < 0 ) { if ( ++i >= j ) break; }
					while ( static_cast<const _derived_ *>( this )->Compare( base[j], pivot ) > 0 ) { if ( --j <= i ) break; }
					if ( i >= j ) break;
					SwapValues( base[i], base[j] );
				} while( ++i < --j );

				// Without these iterations sorting of arrays with many duplicates may
				// become really slow because the partitioning can be very unbalanced.
				// However, these iterations are unnecessary if all elements are unique.
				while ( static_cast<const _derived_ *>( this )->Compare( base[i], pivot ) <= 0 && i < hi[level] ) { i++; }
				while ( static_cast<const _derived_ *>( this )->Compare( base[j], pivot ) >= 0 && lo[level] < j ) { j--; }

				// Move the pivot element in place.
				SwapValues( pivot, base[i] );

				assert( level < MAX_LEVELS - 1 );
				lo[level+1] = i;
				hi[level+1] = hi[level];
				hi[level] = j;
				level++;
			} else {
				// Insertion-sort of the remaining elements.
				for ( ; i < j; j-- ) {
					int64 m = i;
					for ( int64 k = i + 1; k <= j; k++ ) {
						if ( static_cast<const _derived_ *>( this )->Compare( base[k], base[m] )> 0 ) {
							m = k;
						}
					}
					SwapValues( base[m], base[j] );
				}
				level--;
			}
		}
	}
};

/*
================================================
Default quick-sort comparison function that can
be used to sort scalars from small to large.
================================================
*/
template<typename _type_>
class anSort_QuickDefault : public anSort_Quick<_type_, anSort_QuickDefault<_type_>> {
public:
	int Compare( const _type_ & a, const _type_ & b ) const { return a - b; }
};

/*
================================================
Specialization for floating point values to avoid an float-to-int
conversion for every comparison.
================================================
*/
template<>
class anSort_QuickDefault<float> : public anSort_Quick<float, anSort_QuickDefault<float>> {
public:
	int Compare( const float & a, const float & b ) const {
		if ( a < b ) {
			return -1;
		}
		if ( a > b ) {
			return 1;
		}
		return 0;
	}
};

/*
================================================
anSort_Heap is a sort template class that implements the
heap-sort algorithm on an array of objects of the specified data type.
================================================
*/
template<typename _type_, typename _derived_>
class anSort_Heap : public anSort<_type_> {
public:
	virtual void Sort( _type_ * base, unsigned int num ) const {
		// get all elements in heap order
#if 1
		// O( n )
		for ( unsigned int i = num / 2; i > 0; i-- ) {
			// sift down
			unsigned int parent = i - 1;
			for ( unsigned int child = parent * 2 + 1; child < num; child = parent * 2 + 1 ) {
				if ( child + 1 < num && static_cast<const _derived_ *>( this )->Compare( base[child + 1], base[child] ) > 0 ) {
					child++;
				}
				if ( static_cast<const _derived_ *>( this )->Compare( base[child], base[parent] ) <= 0 ) {
					break;
				}
				SwapValues( base[parent], base[child] );
				parent = child;
			}
		}
#else
		// O(n log n)
		for ( unsigned int i = 1; i < num; i++ ) {
			// sift up
			for ( unsigned int child = i; child > 0; ) {
				unsigned int parent = ( child - 1 ) / 2;
				if ( static_cast<const _derived_ *>( this )->Compare( base[parent], base[child] ) > 0 ) {
					break;
				}
				SwapValues( base[child], base[parent] );
				child = parent;
			}
		}
#endif
		// get sorted elements while maintaining heap order
		for ( unsigned int i = num - 1; i > 0; i-- ) {
			SwapValues( base[0], base[i] );
			// sift down
			unsigned int parent = 0;
			for ( unsigned int child = parent * 2 + 1; child < i; child = parent * 2 + 1 ) {
				if ( child + 1 < i && static_cast<const _derived_ *>( this )->Compare( base[child + 1], base[child] ) > 0 ) {
					child++;
				}
				if ( static_cast<const _derived_ *>( this )->Compare( base[child], base[parent] ) <= 0 ) {
					break;
				}
				SwapValues( base[parent], base[child] );
				parent = child;
			}
		}
	}
};

/*
================================================
Default heap-sort comparison function that can
be used to sort scalars from small to large.
================================================
*/
template<typename _type_>
class anSort_HeapDefault : public anSort_Heap< _type_, anSort_HeapDefault<_type_> > {
public:
	int Compare( const _type_ & a, const _type_ & b ) const { return a - b; }
};

/*
================================================
anSort_Insertion is a sort template class that implements the
insertion-sort algorithm on an array of objects of the specified data type.
================================================
*/
template<typename _type_, typename _derived_>
class anSort_Insertion : public anSort<_type_> {
public:
	virtual void Sort( _type_ * base, unsigned int num ) const {
		_type_ * lo = base;
		_type_ * hi = base + ( num - 1 );
		while( hi > lo ) {
			_type_ * max = lo;
			for ( _type_ * p = lo + 1; p <= hi; p++ ) {
				if ( static_cast<const _derived_ *>( this )->Compare( (* p), (* max) )> 0 ) {
					max = p;
				}
			}
			SwapValues( *max, *hi );
			hi--;
		}
	}
};

/*
================================================
Default insertion-sort comparison function that can
be used to sort scalars from small to large.
================================================
*/
template<typename _type_>
class anSort_InsertionDefault : public anSort_Insertion< _type_, anSort_InsertionDefault<_type_> > {
public:
	int Compare( const _type_ & a, const _type_ & b ) const { return a - b; }
};

#endif // !__SORT_H__
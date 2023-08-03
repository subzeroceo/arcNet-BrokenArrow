#ifndef __ARRAY_H__
#define __ARRAY_H__

/*
================================================
anArray is a replacement for a normal C array.

int		myArray[ARRAY_SIZE];

becomes:

anArray<int,ARRAY_SIZE>	myArray;

Has no performance overhead in release builds, but
does index range checking in debug builds.

Unlike anTempArray, the memory is allocated inline with the
object, rather than on the heap.

Unlike anStaticList, there are no fields other than the
actual raw data, and the size is fixed.
================================================
*/
template<class T_, int numElements> class anArray {
public:
	// returns number of elements in list
	int				Num() const { return numElements; }

	// returns the number of bytes the array takes up
	int				ByteSize() const { return sizeof( ptr ); }

	// memset the entire array to zero
	void			Zero() { memset( ptr, 0, sizeof( ptr ) ); }

	// memset the entire array to a specific value
	void			Memset( const char fill ) { memset( ptr, fill, numElements * sizeof( *ptr ) ); }

	// array operators
	const T_ &		operator[]( int index ) const { assert( (unsigned)index < (unsigned)numElements ); return ptr[index]; }
	T_ &			operator[]( int index ) { assert( (unsigned)index < (unsigned)numElements ); return ptr[index]; }

	// returns a pointer to the list
	const T_ *		Ptr() const { return ptr; }
	T_ *			Ptr() { return ptr; }

private:
	T_				ptr[numElements];
};

#define ARRAY_COUNT( arrayName ) ( sizeof( arrayName )/sizeof( arrayName[0] ) )
#define ARRAY_DEF( arrayName ) arrayName, ARRAY_COUNT( arrayName )

/*
================================================
an2DArray is essentially a typedef (as close as we can
get for templates before C++11 anyway) to make
declaring two-dimensional anArrays easier.

Usage:
	an2DArray<int, 5, 10>::type someArray;

================================================
*/
template<class _type_, int _dim1_, int _dim2_>
struct an2DArray {
	typedef anArray<anArray<_type_, _dim2_>, _dim1_> type;
};

/*
================================================
anTupleSize
Generic way to get the size of a tuple-like type.
Add specializations as needed.
This is modeled after std::tuple_size from C++11,
which works for std::arrays also.
================================================
*/
template<class _type_>
struct anTupleSize;
template<class _type_, int _num_>
struct anTupleSize<anArray<_type_, _num_>> {
	enum { value = _num_ };
};

#endif // !__ARRAY_H__

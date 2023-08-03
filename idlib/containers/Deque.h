
/*
============
idDeque
============
*/
template< class T >
class idDeque {
public:
	void	PushBack( const T& element );
	void	PopBack();
	T&		Back();
	
	T&		Front();
	void	PushFront( const T& element );
	void	PopFront();

	void	Clear();
	void	DeleteContents();
	void	SetGranularity( int newGranularity );
	bool	Empty() const;

	int		Num() const;

	void	Swap( idDeque& rhs );

private:
	anList<T> list;
};

/*
============
idDeque<T>::PushBack
============
*/
template< class T >
ARC_INLINE void idDeque<T>::PushBack( const T& element ) {
	list.Append( element );
}

/*
============
idDeque<T>::PopBack
============
*/
template< class T >
ARC_INLINE void idDeque<T>::PopBack() {
	list.RemoveIndex( Num() - 1 );
}

/*
============
idDeque<T>::Back
============
*/
template< class T >
ARC_INLINE T& idDeque<T>::Back() {
	return list[ Num() - 1 ];
}

/*
============
idDeque<T>::PushFront
============
*/
template< class T >
ARC_INLINE void idDeque<T>::PushFront( const T& element ) {
	list.Insert( element, 0 );
}

/*
============
idDeque<T>::PopFront
============
*/
template< class T >
ARC_INLINE void idDeque<T>::PopFront() {
	list.RemoveIndex( 0 );
}

/*
============
idDeque<T>::Front
============
*/
template< class T >
ARC_INLINE T& idDeque<T>::Front() {
	return list[ 0 ];
}

/*
============
idDeque<T>::Clear
============
*/
template< class T >
ARC_INLINE void idDeque<T>::Clear() {
	list.Clear();
}

/*
============
idDeque<T>::DeleteContents
============
*/
template< class T >
ARC_INLINE void idDeque<T>::DeleteContents() {
	list.DeleteContents( true );
}

/*
============
idDeque<T>::SetGranularity
============
*/
template< class T >
ARC_INLINE void idDeque<T>::SetGranularity( int newGranularity ) {
	list.SetGranularity( newGranularity );
}

/*
============
idDeque<T>::Empty
============
*/
template< class T >
ARC_INLINE bool idDeque<T>::Empty() const {
	return list.Num() == 0;
}

/*
============
idDeque<T>::Num
============
*/
template< class T >
ARC_INLINE int idDeque<T>::Num() const {
	return list.Num();
}
/*
============
idDeque<T>::Swap
============
*/
template< class T >
void idDeque<T>::Swap( idDeque& rhs ) {
	list.Swap( rhs.list );
}

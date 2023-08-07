#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
===============================================================================

	Queue template

===============================================================================
*/

#define anQueue( type, next ) anQueueTemplate<type, ( int )&( ( (type *)nullptr )->next )>

template< class type, int nextOffset >
class anQueueTemplate {
public:
							anQueueTemplate( void );

	void					Add( type *element );
	type *					Get( void );

private:
	type *					first;
	type *					last;
};

#define QUEUE_NEXT_PTR( element )		( *( (type **)( ( (byte *)element ) + nextOffset ) ) )

template< class type, int nextOffset >
anQueueTemplate<type,nextOffset>::anQueueTemplate( void ) {
	first = last = nullptr;
}

template< class type, int nextOffset >
void anQueueTemplate<type,nextOffset>::Add( type *element ) {
	QUEUE_NEXT_PTR( element ) = nullptr;
	if ( last ) {
		QUEUE_NEXT_PTR( last ) = element;
	} else {
		first = element;
	}
	last = element;
}

template< class type, int nextOffset >
type *anQueueTemplate<type,nextOffset>::Get( void ) {
	type *element;

	element = first;
	if ( element ) {
		first = QUEUE_NEXT_PTR( first );
		if ( last == element ) {
			last = nullptr;
		}
		QUEUE_NEXT_PTR( element ) = nullptr;
	}
	return element;
}

#endif /* !__QUEUE_H__ */

#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
===============================================================================

	Queue template

===============================================================================
*/

#define arcQueue( type, next ) arcQueueTemplate<type, ( int )&(((type*)NULL)->next)>

template< class type, int nextOffset >
class arcQueueTemplate {
public:
							arcQueueTemplate( void );

	void					Add( type *element );
	type *					Get( void );

private:
	type *					first;
	type *					last;
};

#define QUEUE_NEXT_PTR( element )		(*((type**)(((byte*)element)+nextOffset) ))

template< class type, int nextOffset >
arcQueueTemplate<type,nextOffset>::arcQueueTemplate( void ) {
	first = last = NULL;
}

template< class type, int nextOffset >
void arcQueueTemplate<type,nextOffset>::Add( type *element ) {
	QUEUE_NEXT_PTR(element) = NULL;
	if ( last ) {
		QUEUE_NEXT_PTR(last) = element;
	} else {
		first = element;
	}
	last = element;
}

template< class type, int nextOffset >
type *arcQueueTemplate<type,nextOffset>::Get( void ) {
	type *element;

	element = first;
	if ( element ) {
		first = QUEUE_NEXT_PTR(first);
		if ( last == element ) {
			last = NULL;
		}
		QUEUE_NEXT_PTR(element) = NULL;
	}
	return element;
}

#endif /* !__QUEUE_H__ */

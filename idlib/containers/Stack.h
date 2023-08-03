#ifndef __STACK_H__
#define __STACK_H__

/*
===============================================================================

	Stack template

===============================================================================
*/

#define idStack( type, next )		arcStackTemplate<type, ( int )&(((type*)nullptr )->next)>

template< class type, int nextOffset >
class arcStackTemplate {
public:
							arcStackTemplate( void );

	void					Add( type *element );
	type *					Get( void );

private:
	type *					top;
	type *					bottom;
};

#define STACK_NEXT_PTR( element ) (*(type**)(((byte*)element)+nextOffset) )

template< class type, int nextOffset >
arcStackTemplate<type,nextOffset>::arcStackTemplate( void ) {
	top = bottom = nullptr;
}

template< class type, int nextOffset >
void arcStackTemplate<type,nextOffset>::Add( type *element ) {
	STACK_NEXT_PTR(element) = top;
	top = element;
	if ( !bottom ) {
		bottom = element;
	}
}

template< class type, int nextOffset >
type *arcStackTemplate<type,nextOffset>::Get( void ) {
	type *element;

	element = top;
	if ( element ) {
		top = STACK_NEXT_PTR(top);
		if ( bottom == element ) {
			bottom = nullptr;
		}
		STACK_NEXT_PTR(element) = nullptr;
	}
	return element;
}

#endif /* !__STACK_H__ */

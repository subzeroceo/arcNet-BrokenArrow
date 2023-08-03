#ifndef __LINKLIST_H__
#define __LINKLIST_H__

/*
==============================================================================

anLinkList

Circular linked list template

==============================================================================
*/

template<class type>
class anLinkList {
public:
						anLinkList();
						~anLinkList();

	bool				IsListEmpty( void ) const;
	bool				InList( void ) const;
	int					Num( void ) const;
	void				Clear( void );

	void				InsertBefore( anLinkList &node );
	void				InsertAfter( anLinkList &node );
	void				AddToEnd( anLinkList &node );
	void				AddToFront( anLinkList &node );

	void				Remove( void );

	type *				Next( void ) const;
	type *				Prev( void ) const;

	type *				Owner( void ) const;
	void				SetOwner( type *object );

	anLinkList *		ListHead( void ) const;
	anLinkList *		NextNode( void ) const;
	anLinkList *		PrevNode( void ) const;

private:
	anLinkList *		head;
	anLinkList *		next;
	anLinkList *		prev;
	type *				owner;
};

/*
================
anLinkList<type>::anLinkList

Node is initialized to be the head of an empty list
================
*/
template<class type>
anLinkList<type>::anLinkList() {
	owner	= nullptr;
	head	= this;
	next	= this;
	prev	= this;
}

/*
================
anLinkList<type>::~anLinkList

Removes the node from the list, or if it's the head of a list, removes
all the nodes from the list.
================
*/
template<class type>
anLinkList<type>::~anLinkList() {
	Clear();
}

/*
================
anLinkList<type>::IsListEmpty

Returns true if the list is empty.
================
*/
template<class type>
bool anLinkList<type>::IsListEmpty( void ) const {
	return head->next == head;
}

/*
================
anLinkList<type>::InList

Returns true if the node is in a list.  If called on the head of a list, will always return false.
================
*/
template<class type>
bool anLinkList<type>::InList( void ) const {
	return head != this;
}

/*
================
anLinkList<type>::Num

Returns the number of nodes in the list.
================
*/
template<class type>
int anLinkList<type>::Num( void ) const {
	anLinkList<type>	*node;
	int					num;

	num = 0;
	for ( node = head->next; node != head; node = node->next ) {
		num++;
	}

	return num;
}

/*
================
anLinkList<type>::Clear

If node is the head of the list, clears the list.  Otherwise it just removes the node from the list.
================
*/
template<class type>
void anLinkList<type>::Clear( void ) {
	if ( head == this ) {
		while( next != this ) {
			next->Remove();
		}
	} else {
		Remove();
	}
}

/*
================
anLinkList<type>::Remove

Removes node from list
================
*/
template<class type>
void anLinkList<type>::Remove( void ) {
	prev->next = next;
	next->prev = prev;

	next = this;
	prev = this;
	head = this;
}

/*
================
anLinkList<type>::InsertBefore

Places the node before the existing node in the list.  If the existing node is the head,
then the new node is placed at the end of the list.
================
*/
template<class type>
void anLinkList<type>::InsertBefore( anLinkList &node ) {
	Remove();

	next		= &node;
	prev		= node.prev;
	node.prev	= this;
	prev->next	= this;
	head		= node.head;
}

/*
================
anLinkList<type>::InsertAfter

Places the node after the existing node in the list.  If the existing node is the head,
then the new node is placed at the beginning of the list.
================
*/
template<class type>
void anLinkList<type>::InsertAfter( anLinkList &node ) {
	Remove();

	prev		= &node;
	next		= node.next;
	node.next	= this;
	next->prev	= this;
	head		= node.head;
}

/*
================
anLinkList<type>::AddToEnd

Adds node at the end of the list
================
*/
template<class type>
void anLinkList<type>::AddToEnd( anLinkList &node ) {
	InsertBefore( *node.head );
}

/*
================
anLinkList<type>::AddToFront

Adds node at the beginning of the list
================
*/
template<class type>
void anLinkList<type>::AddToFront( anLinkList &node ) {
	InsertAfter( *node.head );
}

/*
================
anLinkList<type>::ListHead

Returns the head of the list.  If the node isn't in a list, it returns
a pointer to itself.
================
*/
template<class type>
anLinkList<type> *anLinkList<type>::ListHead( void ) const {
	return head;
}

/*
================
anLinkList<type>::Next

Returns the next object in the list, or nullptr if at the end.
================
*/
template<class type>
type *anLinkList<type>::Next( void ) const {
	if ( !next || ( next == head ) ) {
		return nullptr;
	}
	return next->owner;
}

/*
================
anLinkList<type>::Prev

Returns the previous object in the list, or nullptr if at the beginning.
================
*/
template<class type>
type *anLinkList<type>::Prev( void ) const {
	if ( !prev || ( prev == head ) ) {
		return nullptr;
	}
	return prev->owner;
}

/*
================
anLinkList<type>::NextNode

Returns the next node in the list, or nullptr if at the end.
================
*/
template<class type>
anLinkList<type> *anLinkList<type>::NextNode( void ) const {
	if ( next == head ) {
		return nullptr;
	}
	return next;
}

/*
================
anLinkList<type>::PrevNode

Returns the previous node in the list, or nullptr if at the beginning.
================
*/
template<class type>
anLinkList<type> *anLinkList<type>::PrevNode( void ) const {
	if ( prev == head ) {
		return nullptr;
	}
	return prev;
}

/*
================
anLinkList<type>::Owner

Gets the object that is associated with this node.
================
*/
template<class type>
type *anLinkList<type>::Owner( void ) const {
	return owner;
}

/*
================
anLinkList<type>::SetOwner

Sets the object that this node is associated with.
================
*/
template<class type>
void anLinkList<type>::SetOwner( type *object ) {
	owner = object;
}

#endif /* !__LINKLIST_H__ */

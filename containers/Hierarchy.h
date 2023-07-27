#ifndef __HIERARCHY_H__
#define __HIERARCHY_H__

/*
==============================================================================

	ARCHierarchy

==============================================================================
*/

template< class type >
class ARCHierarchy {
public:

						ARCHierarchy();
						~ARCHierarchy();

	void				SetOwner( type *object );
	type *				Owner( void ) const;
	void				ParentTo( ARCHierarchy &node );
	void				MakeSiblingAfter( ARCHierarchy &node );
	bool				ParentedBy( const ARCHierarchy &node ) const;
	void				RemoveFromParent( void );
	void				RemoveFromHierarchy( void );

	type *				GetParent( void ) const;		// parent of this node
	type *				GetChild( void ) const;			// first child of this node
	type *				GetSibling( void ) const;		// next node with the same parent
	type *				GetPriorSibling( void ) const;	// previous node with the same parent
	type *				GetNext( void ) const;			// goes through all nodes of the hierarchy
	type *				GetNextLeaf( void ) const;		// goes through all leaf nodes of the hierarchy

private:
	ARCHierarchy *		parent;
	ARCHierarchy *		sibling;
	ARCHierarchy *		child;
	type *				owner;

	ARCHierarchy<type>	*GetPriorSiblingNode( void ) const;	// previous node with the same parent
};

/*
================
ARCHierarchy<type>::ARCHierarchy
================
*/
template< class type >
ARCHierarchy<type>::ARCHierarchy() {
	owner	= NULL;
	parent	= NULL;
	sibling	= NULL;
	child	= NULL;
}

/*
================
ARCHierarchy<type>::~ARCHierarchy
================
*/
template< class type >
ARCHierarchy<type>::~ARCHierarchy() {
	RemoveFromHierarchy();
}

/*
================
ARCHierarchy<type>::Owner

Gets the object that is associated with this node.
================
*/
template< class type >
type *ARCHierarchy<type>::Owner( void ) const {
	return owner;
}

/*
================
ARCHierarchy<type>::SetOwner

Sets the object that this node is associated with.
================
*/
template< class type >
void ARCHierarchy<type>::SetOwner( type *object ) {
	owner = object;
}

/*
================
ARCHierarchy<type>::ParentedBy
================
*/
template< class type >
bool ARCHierarchy<type>::ParentedBy( const ARCHierarchy &node ) const {
	if ( parent == &node ) {
		return true;
	} else if ( parent ) {
		return parent->ParentedBy( node );
	}
	return false;
}

/*
================
ARCHierarchy<type>::ParentTo

Makes the given node the parent.
================
*/
template< class type >
void ARCHierarchy<type>::ParentTo( ARCHierarchy &node ) {
	RemoveFromParent();

	parent		= &node;
	sibling		= node.child;
	node.child	= this;
}

/*
================
ARCHierarchy<type>::MakeSiblingAfter

Makes the given node a sibling after the passed in node.
================
*/
template< class type >
void ARCHierarchy<type>::MakeSiblingAfter( ARCHierarchy &node ) {
	RemoveFromParent();
	parent	= node.parent;
	sibling = node.sibling;
	node.sibling = this;
}

/*
================
ARCHierarchy<type>::RemoveFromParent
================
*/
template< class type >
void ARCHierarchy<type>::RemoveFromParent( void ) {
	ARCHierarchy<type> *prev;

	if ( parent ) {
		prev = GetPriorSiblingNode();
		if ( prev ) {
			prev->sibling = sibling;
		} else {
			parent->child = sibling;
		}
	}

	parent = NULL;
	sibling = NULL;
}

/*
================
ARCHierarchy<type>::RemoveFromHierarchy

Removes the node from the hierarchy and adds it's children to the parent.
================
*/
template< class type >
void ARCHierarchy<type>::RemoveFromHierarchy( void ) {
	ARCHierarchy<type> *parentNode;
	ARCHierarchy<type> *node;

	parentNode = parent;
	RemoveFromParent();

	if ( parentNode ) {
		while( child ) {
			node = child;
			node->RemoveFromParent();
			node->ParentTo( *parentNode );
		}
	} else {
		while( child ) {
			child->RemoveFromParent();
		}
	}
}

/*
================
ARCHierarchy<type>::GetParent
================
*/
template< class type >
type *ARCHierarchy<type>::GetParent( void ) const {
	if ( parent ) {
		return parent->owner;
	}
	return NULL;
}

/*
================
ARCHierarchy<type>::GetChild
================
*/
template< class type >
type *ARCHierarchy<type>::GetChild( void ) const {
	if ( child ) {
		return child->owner;
	}
	return NULL;
}

/*
================
ARCHierarchy<type>::GetSibling
================
*/
template< class type >
type *ARCHierarchy<type>::GetSibling( void ) const {
	if ( sibling ) {
		return sibling->owner;
	}
	return NULL;
}

/*
================
ARCHierarchy<type>::GetPriorSiblingNode

Returns NULL if no parent, or if it is the first child.
================
*/
template< class type >
ARCHierarchy<type> *ARCHierarchy<type>::GetPriorSiblingNode( void ) const {
	if ( !parent || ( parent->child == this ) ) {
		return NULL;
	}

	ARCHierarchy<type> *prev;
	ARCHierarchy<type> *node;

	node = parent->child;
	prev = NULL;
	while( ( node != this ) && ( node != NULL ) ) {
		prev = node;
		node = node->sibling;
	}

	if ( node != this ) {
		anLibrary::Error( "ARCHierarchy::GetPriorSibling: could not find node in parent's list of children" );
	}

	return prev;
}

/*
================
ARCHierarchy<type>::GetPriorSibling

Returns NULL if no parent, or if it is the first child.
================
*/
template< class type >
type *ARCHierarchy<type>::GetPriorSibling( void ) const {
	ARCHierarchy<type> *prior;

	prior = GetPriorSiblingNode();
	if ( prior ) {
		return prior->owner;
	}

	return NULL;
}

/*
================
ARCHierarchy<type>::GetNext

Goes through all nodes of the hierarchy.
================
*/
template< class type >
type *ARCHierarchy<type>::GetNext( void ) const {
	const ARCHierarchy<type> *node;

	if ( child ) {
		return child->owner;
	} else {
		node = this;
		while( node && node->sibling == NULL ) {
			node = node->parent;
		}
		if ( node ) {
			return node->sibling->owner;
		} else {
			return NULL;
		}
	}
}

/*
================
ARCHierarchy<type>::GetNextLeaf

Goes through all leaf nodes of the hierarchy.
================
*/
template< class type >
type *ARCHierarchy<type>::GetNextLeaf( void ) const {
	const ARCHierarchy<type> *node;

	if ( child ) {
		node = child;
		while ( node->child ) {
			node = node->child;
		}
		return node->owner;
	} else {
		node = this;
		while( node && node->sibling == NULL ) {
			node = node->parent;
		}
		if ( node ) {
			node = node->sibling;
			while ( node->child ) {
				node = node->child;
			}
			return node->owner;
		} else {
			return NULL;
		}
	}
}

#endif /* !__HIERARCHY_H__ */

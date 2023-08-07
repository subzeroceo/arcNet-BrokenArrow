#ifndef __BTREE_H__
#define __BTREE_H__

/*
===============================================================================

	Balanced Search Tree

===============================================================================
*/

//#define BTREE_CHECK

template< class objType, class keyType >
class anBinaryTreeNode {
public:
	keyType							key;			// key used for sorting
	objType *						object;			// if != nullptr pointer to object stored in leaf node
	anBinaryTreeNode *					parent;			// parent node
	anBinaryTreeNode *					next;			// next sibling
	anBinaryTreeNode *					prev;			// prev sibling
	int								numChildren;	// number of children
	anBinaryTreeNode *					firstChild;		// first child
	anBinaryTreeNode *					lastChild;		// last child
};


template< class objType, class keyType, int maxChildrenPerNode >
class anBinaryTree {
public:
									anBinaryTree( void );
									~anBinaryTree( void );

	void							Init( void );
	void							Shutdown( void );

	anBinaryTreeNode<objType,keyType> *	Add( objType *object, keyType key );						// add an object to the tree
	void							Remove( anBinaryTreeNode<objType,keyType> *node );				// remove an object node from the tree

	objType *						Find( keyType key ) const;									// find an object using the given key
	objType *						FindSmallestLargerEqual( keyType key ) const;				// find an object with the smallest key larger equal the given key
	objType *						FindLargestSmallerEqual( keyType key ) const;				// find an object with the largest key smaller equal the given key

	anBinaryTreeNode<objType,keyType> *	GetRoot( void ) const;										// returns the root node of the tree
	int								GetNodeCount( void ) const;									// returns the total number of nodes in the tree
	anBinaryTreeNode<objType,keyType> *	GetNext( anBinaryTreeNode<objType,keyType> *node ) const;		// goes through all nodes of the tree
	anBinaryTreeNode<objType,keyType> *	GetNextLeaf( anBinaryTreeNode<objType,keyType> *node ) const;	// goes through all leaf nodes of the tree

private:
	anBinaryTreeNode<objType,keyType> *	root;
	anBlockAlloc<anBinaryTreeNode<objType,keyType>,128>	nodeAllocator;

	anBinaryTreeNode<objType,keyType> *	AllocNode( void );
	void							FreeNode( anBinaryTreeNode<objType,keyType> *node );
	void							SplitNode( anBinaryTreeNode<objType,keyType> *node );
	anBinaryTreeNode<objType,keyType> *	MergeNodes( anBinaryTreeNode<objType,keyType> *node1, anBinaryTreeNode<objType,keyType> *node2 );

	void							CheckTree_r( anBinaryTreeNode<objType,keyType> *node, int &numNodes ) const;
	void							CheckTree( void ) const;
};


template< class objType, class keyType, int maxChildrenPerNode >
inline anBinaryTree<objType,keyType,maxChildrenPerNode>::anBinaryTree( void ) {
	assert( maxChildrenPerNode >= 4 );
	root = nullptr;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline anBinaryTree<objType,keyType,maxChildrenPerNode>::~anBinaryTree( void ) {
	Shutdown();
}

template< class objType, class keyType, int maxChildrenPerNode >
inline void anBinaryTree<objType,keyType,maxChildrenPerNode>::Init( void ) {
	root = AllocNode();
}

template< class objType, class keyType, int maxChildrenPerNode >
inline void anBinaryTree<objType,keyType,maxChildrenPerNode>::Shutdown( void ) {
	nodeAllocator.Shutdown();
	root = nullptr;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline anBinaryTreeNode<objType,keyType> *anBinaryTree<objType,keyType,maxChildrenPerNode>::Add( objType *object, keyType key ) {
	anBinaryTreeNode<objType,keyType> *node, *child, *newNode;

	if ( root->numChildren >= maxChildrenPerNode ) {
		newNode = AllocNode();
		newNode->key = root->key;
		newNode->firstChild = root;
		newNode->lastChild = root;
		newNode->numChildren = 1;
		root->parent = newNode;
		SplitNode( root );
		root = newNode;
	}

	newNode = AllocNode();
	newNode->key = key;
	newNode->object = object;

	for ( node = root; node->firstChild != nullptr; node = child ) {
		if ( key > node->key ) {
			node->key = key;
		}

		// find the first child with a key larger equal to the key of the new node
		for ( child = node->firstChild; child->next; child = child->next ) {
			if ( key <= child->key ) {
				break;
			}
		}

		if ( child->object ) {
			if ( key <= child->key ) {
				// insert new node before child
				if ( child->prev ) {
					child->prev->next = newNode;
				} else {
					node->firstChild = newNode;
				}
				newNode->prev = child->prev;
				newNode->next = child;
				child->prev = newNode;
			} else {
				// insert new node after child
				if ( child->next ) {
					child->next->prev = newNode;
				} else {
					node->lastChild = newNode;
				}
				newNode->prev = child;
				newNode->next = child->next;
				child->next = newNode;
			}

			newNode->parent = node;
			node->numChildren++;

#ifdef BTREE_CHECK
			CheckTree();
#endif
			return newNode;
		}

		// make sure the child has room to store another node
		if ( child->numChildren >= maxChildrenPerNode ) {
			SplitNode( child );
			if ( key <= child->prev->key ) {
				child = child->prev;
			}
		}
	}

	// we only end up here if the root node is empty
	newNode->parent = root;
	root->key = key;
	root->firstChild = newNode;
	root->lastChild = newNode;
	root->numChildren++;

#ifdef BTREE_CHECK
	CheckTree();
#endif

	return newNode;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline void anBinaryTree<objType,keyType,maxChildrenPerNode>::Remove( anBinaryTreeNode<objType,keyType> *node ) {
	anBinaryTreeNode<objType,keyType> *parent;
	assert( node->object != nullptr );
	// unlink the node from it's parent
	if ( node->prev ) {
		node->prev->next = node->next;
	} else {
		node->parent->firstChild = node->next;
	}
	if ( node->next ) {
		node->next->prev = node->prev;
	} else {
		node->parent->lastChild = node->prev;
	}
	node->parent->numChildren--;

	// make sure there are no parent nodes with a single child
	for ( parent = node->parent; parent != root && parent->numChildren <= 1; parent = parent->parent ) {

		if ( parent->next ) {
			parent = MergeNodes( parent, parent->next );
		} else if ( parent->prev ) {
			parent = MergeNodes( parent->prev, parent );
		}

		// a parent may not use a key higher than the key of it's last child
		if ( parent->key > parent->lastChild->key ) {
			parent->key = parent->lastChild->key;
		}

		if ( parent->numChildren > maxChildrenPerNode ) {
			SplitNode( parent );
			break;
		}
	}
	for (; parent != nullptr && parent->lastChild != nullptr; parent = parent->parent ) {
		// a parent may not use a key higher than the key of it's last child
		if ( parent->key > parent->lastChild->key ) {
			parent->key = parent->lastChild->key;
		}
	}

	// free the node
	FreeNode( node );

	// remove the root node if it has a single internal node as child
	if ( root->numChildren == 1 && root->firstChild->object == nullptr ) {
		anBinaryTreeNode<objType,keyType> *oldRoot = root;
		root->firstChild->parent = nullptr;
		root = root->firstChild;
		FreeNode( oldRoot );
	}

#ifdef BTREE_CHECK
	CheckTree();
#endif
}

template< class objType, class keyType, int maxChildrenPerNode >
inline objType *anBinaryTree<objType,keyType,maxChildrenPerNode>::Find( keyType key ) const {
	anBinaryTreeNode<objType,keyType> *node;
	for ( node = root->firstChild; node != nullptr; node = node->firstChild ) {
		while( node->next ) {
			if ( node->key >= key ) {
				break;
			}
			node = node->next;
		}
		if ( node->object ) {
			if ( node->key == key ) {
				return node->object;
			} else {
				return nullptr;
			}
		}
	}
	return nullptr;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline objType *anBinaryTree<objType,keyType,maxChildrenPerNode>::FindSmallestLargerEqual( keyType key ) const {
	anBinaryTreeNode<objType,keyType> *node;
	for ( node = root->firstChild; node != nullptr; node = node->firstChild ) {
		while( node->next ) {
			if ( node->key >= key ) {
				break;
			}
			node = node->next;
		}
		if ( node->object ) {
			if ( node->key >= key ) {
				return node->object;
			} else {
				return nullptr;
			}
		}
	}
	return nullptr;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline objType *anBinaryTree<objType,keyType,maxChildrenPerNode>::FindLargestSmallerEqual( keyType key ) const {
	anBinaryTreeNode<objType,keyType> *node;
	for ( node = root->lastChild; node != nullptr; node = node->lastChild ) {
		while( node->prev ) {
			if ( node->key <= key ) {
				break;
			}
			node = node->prev;
		}
		if ( node->object ) {
			if ( node->key <= key ) {
				return node->object;
			} else {
				return nullptr;
			}
		}
	}
	return nullptr;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline anBinaryTreeNode<objType,keyType> *anBinaryTree<objType,keyType,maxChildrenPerNode>::GetRoot( void ) const {
	return root;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline int anBinaryTree<objType,keyType,maxChildrenPerNode>::GetNodeCount( void ) const {
	return nodeAllocator.GetAllocCount();
}

template< class objType, class keyType, int maxChildrenPerNode >
inline anBinaryTreeNode<objType,keyType> *anBinaryTree<objType,keyType,maxChildrenPerNode>::GetNext( anBinaryTreeNode<objType,keyType> *node ) const {
	if ( node->firstChild ) {
		return node->firstChild;
	} else {
		while( node && node->next == nullptr ) {
			node = node->parent;
		}
		return node;
	}
}

template< class objType, class keyType, int maxChildrenPerNode >
inline anBinaryTreeNode<objType,keyType> *anBinaryTree<objType,keyType,maxChildrenPerNode>::GetNextLeaf( anBinaryTreeNode<objType,keyType> *node ) const {
	if ( node->firstChild ) {
		while ( node->firstChild ) {
			node = node->firstChild;
		}
		return node;
	} else {
		while( node && node->next == nullptr ) {
			node = node->parent;
		}
		if ( node ) {
			node = node->next;
			while ( node->firstChild ) {
				node = node->firstChild;
			}
			return node;
		} else {
			return nullptr;
		}
	}
}

template< class objType, class keyType, int maxChildrenPerNode >
inline anBinaryTreeNode<objType,keyType> *anBinaryTree<objType,keyType,maxChildrenPerNode>::AllocNode( void ) {
	anBinaryTreeNode<objType,keyType> *node = nodeAllocator.Alloc();
	node->key = 0;
	node->parent = nullptr;
	node->next = nullptr;
	node->prev = nullptr;
	node->numChildren = 0;
	node->firstChild = nullptr;
	node->lastChild = nullptr;
	node->object = nullptr;
	return node;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline void anBinaryTree<objType,keyType,maxChildrenPerNode>::FreeNode( anBinaryTreeNode<objType,keyType> *node ) {
	nodeAllocator.Free( node );
}

template< class objType, class keyType, int maxChildrenPerNode >
inline void anBinaryTree<objType,keyType,maxChildrenPerNode>::SplitNode( anBinaryTreeNode<objType,keyType> *node ) {
	int i;
	anBinaryTreeNode<objType,keyType> *child, *newNode;

	// allocate a new node
	newNode = AllocNode();
	newNode->parent = node->parent;

	// divide the children over the two nodes
	child = node->firstChild;
	child->parent = newNode;
	for ( i = 3; i < node->numChildren; i += 2 ) {
		child = child->next;
		child->parent = newNode;
	}

	newNode->key = child->key;
	newNode->numChildren = node->numChildren / 2;
	newNode->firstChild = node->firstChild;
	newNode->lastChild = child;

	node->numChildren -= newNode->numChildren;
	node->firstChild = child->next;

	child->next->prev = nullptr;
	child->next = nullptr;

	// add the new child to the parent before the split node
	assert( node->parent->numChildren < maxChildrenPerNode );

	if ( node->prev ) {
		node->prev->next = newNode;
	} else {
		node->parent->firstChild = newNode;
	}
	newNode->prev = node->prev;
	newNode->next = node;
	node->prev = newNode;

	node->parent->numChildren++;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline anBinaryTreeNode<objType,keyType> *anBinaryTree<objType,keyType,maxChildrenPerNode>::MergeNodes( anBinaryTreeNode<objType,keyType> *node1, anBinaryTreeNode<objType,keyType> *node2 ) {
	anBinaryTreeNode<objType,keyType> *child;

	assert( node1->parent == node2->parent );
	assert( node1->next == node2 && node2->prev == node1 );
	assert( node1->object == nullptr && node2->object == nullptr );
	assert( node1->numChildren >= 1 && node2->numChildren >= 1 );

	for ( child = node1->firstChild; child->next; child = child->next ) {
		child->parent = node2;
	}
	child->parent = node2;
	child->next = node2->firstChild;
	node2->firstChild->prev = child;
	node2->firstChild = node1->firstChild;
	node2->numChildren += node1->numChildren;

	// unlink the first node from the parent
	if ( node1->prev ) {
		node1->prev->next = node2;
	} else {
		node1->parent->firstChild = node2;
	}
	node2->prev = node1->prev;
	node2->parent->numChildren--;

	FreeNode( node1 );

	return node2;
}

template< class objType, class keyType, int maxChildrenPerNode >
inline void anBinaryTree<objType,keyType,maxChildrenPerNode>::CheckTree_r( anBinaryTreeNode<objType,keyType> *node, int &numNodes ) const {
	int numChildren;
	anBinaryTreeNode<objType,keyType> *child;

	numNodes++;

	// the root node may have zero children and leaf nodes always have zero children, all other nodes should have at least 2 and at most maxChildrenPerNode children
	assert( ( node == root ) || ( node->object != nullptr && node->numChildren == 0 ) || ( node->numChildren >= 2 && node->numChildren <= maxChildrenPerNode ) );
	// the key of a node may never be larger than the key of it's last child
	assert( ( node->lastChild == nullptr ) || ( node->key <= node->lastChild->key ) );

	numChildren = 0;
	for ( child = node->firstChild; child; child = child->next ) {
		numChildren++;
		// make sure the children are properly linked
		if ( child->prev == nullptr ) {
			assert( node->firstChild == child );
		} else {
			assert( child->prev->next == child );
		}
		if ( child->next == nullptr ) {
			assert( node->lastChild == child );
		} else {
			assert( child->next->prev == child );
		}
		// recurse down the tree
		CheckTree_r( child, numNodes );
	}
	// the number of children should equal the number of linked children
	assert( numChildren == node->numChildren );
}

template< class objType, class keyType, int maxChildrenPerNode >
inline void anBinaryTree<objType,keyType,maxChildrenPerNode>::CheckTree( void ) const {
	int numNodes = 0;
	anBinaryTreeNode<objType,keyType> *node, *lastNode;

	CheckTree_r( root, numNodes );

	// the number of nodes in the tree should equal the number of allocated nodes
	assert( numNodes == nodeAllocator.GetAllocCount() );

	// all the leaf nodes should be ordered
	lastNode = GetNextLeaf( GetRoot() );
	if ( lastNode ) {
		for ( node = GetNextLeaf( lastNode ); node; lastNode = node, node = GetNextLeaf( node ) ) {
			assert( lastNode->key <= node->key );
		}
	}
}

#endif // !__BTREE_H__

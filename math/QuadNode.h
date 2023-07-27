#ifndef __QUADTREE_H__
#define __QUADTREE_H__

/*
==============================================================================

	QuadTree

		Direct Access QuadTree Lookup template

		Children are orientated in the following order:

			+---+---+
			| 2 | 3 |
			+---+---+
			| 0 | 1 |
			+---+---+
		(0,0)

		Neighbors are orientated in the following order:

			 3
			1+2
			 0

==============================================================================
*/

template< class type >
class arcQuadTree {
public:
	typedef struct nodePosition_s {
		int		level;
		int		x;
		int		y;
	} nodePosition_t;

	typedef type dataType;

	class arcQuadNode {
	public:
								arcQuadNode( void ) {
									parent = NULL;
									memset( children, 0, sizeof( children ) );
									memset( neighbors, 0, sizeof( neighbors ) );
									data = NULL;
									bounds.Clear();
								}
								explicit arcQuadNode( const anBounds &bounds ) {
									parent = NULL;
									memset( children, 0, sizeof( children ) );
									memset( neighbors, 0, sizeof( neighbors ) );
									data = NULL;
									this->bounds = bounds;
								}
								explicit arcQuadNode( type *data, const anBounds &bounds ) {
									assert( data != NULL );
									parent = NULL;
									memset( children, 0, sizeof( children ) );
									memset( neighbors, 0, sizeof( neighbors ) );
									this->data = data;
									this->bounds = bounds;
								}
		virtual					~arcQuadNode( void ) {
									assert( data == NULL );
								}

		void					SetData( type *data ) {
									this->data = data;
								}
		type *					GetData( void ) const {
									return data;
								}

		void					SetParent( arcQuadNode &parent ) {
									this->parent = &parent;
								}
		void					SetChild( arcQuadNode &child, const int index ) {
									assert( index >= 0 && index < 4 );
									children[ index ] = &child;
								}
		void					SetNeighbor( arcQuadNode &neighbor, const int index ) {
									assert( index >= 0 && index < 4 );
									neighbors[ index ] = &neighbor;
								}
		void					SetBounds( const anBounds &bounds ) {
									this->bounds = bounds;
								}

		arcQuadNode *			GetParent( void ) const {
									return parent;
								}
		arcQuadNode *			GetChild( const int index ) const {
									assert( index >= 0 && index < 4 );
									return children[ index ];
								}
		arcQuadNode *			GetNeighbor( const int index ) const {
									assert( index >= 0 && index < 4 );
									return neighbors[ index ];
								}
		anBounds &				GetBounds( void ) {
									return bounds;
								}

		void					SetNodePosition( const int nodeLevel, const int nodeX, const int nodeY ) {
									nodePosition.level = nodeLevel;
									nodePosition.x = nodeX;
									nodePosition.y = nodeY;
								}
		const nodePosition_t &	GetNodePosition( void ) const {
									return nodePosition;
								}

		bool					HasChildren( void ) {
									return( children[0] || children[1] || children[2] || children[3] );
								}
		void					ClearChildren( void ) {
									memset( children, 0, sizeof( children ) );
								}

	private:
		arcQuadNode *	parent;
		arcQuadNode *	children[4];
		arcQuadNode *	neighbors[4];
		type *			data;

		anBounds		bounds;

		// keep track of the position in the tree
		nodePosition_t	nodePosition;
	};

						explicit arcQuadTree( const anBounds &bounds, const int depth = 6 );
	virtual				~arcQuadTree( void );

	void				BuildQuadTree( void );
	void				BuildQuadTree( arcQuadNode &node );

	const int			GetDepth( void ) const { return depth; }
	const int			GetUsedDepth( void ) const;

	const arcQuadNode *	GetHeadNode( void ) const { return headNode; }
	arcQuadNode *		GetHeadNode( void ) { return headNode; }

	arcQuadNode *		FindNode( const anVec3 &point );

	arcQuadNode *		GetNode( const anBounds &bounds );
	arcQuadNode *		GetNode( const nodePosition_t &nodePosition );
	arcQuadNode **		GetNodes( const int nodeLevel, int &numNodes ) {
							assert( nodeLevel >=0 && nodeLevel < depth );
							numNodes = anMath::Pow( 2, nodeLevel * 2 );
							return nodes[ nodeLevel ];
						}

	const int			GetNumLeafNodes( void ) const;

	void				CreateChildren( arcQuadNode &parent );
	void				FreeChildren( arcQuadNode &parent ) {
							for ( int i = 0; i < 4; i++ ) {
								if ( parent.GetChild(i) ) {
									FreeNode( *parent.GetChild(i) );
								}
							}

							parent.ClearChildren();
						}

private:
	void				GetUsedDepth_r( arcQuadNode &node, const int currentDepth, int *maxReachedDepth ) const;
	void				GetNumLeafNodes_r( arcQuadNode &node, int *numLeafNodes ) const;
	arcQuadNode *		AllocNode( arcQuadNode **node, int nodeLevel, int x, int y );
	void				FindChildren_r( arcQuadNode &parent, const int nodeLevel );
	void				FindChildren_r( arcQuadNode &parent, const int nodeLevel, const int parentX, const int parentY );
	void				FindNeighbors_r( const int nodeLevel );

	void				FreeNode( arcQuadNode &node ) {
							nodes[ node.GetNodePosition().level ][ ( node.GetNodePosition().y << ( node.GetNodePosition().level ) ) + node.GetNodePosition().x ] = NULL;
							delete &node;
						}

private:
	arcQuadNode *		headNode;
	arcQuadNode ***		nodes;

	int					depth;
	anBounds			bounds;
	anVec2				nodeScale;
};

/*
================
arcQuadTree<type>::arcQuadTree( const int depth )
================
*/
template< class type >
ARC_INLINE arcQuadTree<type>::arcQuadTree( const anBounds &bounds, const int depth ) {
	assert( depth > 0 );

	this->depth = depth;
	this->bounds = bounds;

	// expand by 1 unit so everything fits completely in it
	this->bounds.ExpandSelf( 1.f );

	nodeScale.x = anMath::Pow( 2, depth - 1 ) / ( this->bounds[ 1 ].x - this->bounds[ 0 ].x );
	nodeScale.y = anMath::Pow( 2, depth - 1 ) / ( this->bounds[ 1 ].y - this->bounds[ 0 ].y );

	nodes = new arcQuadNode** [ depth ];

	for ( int i = 0; i < depth; i++ ) {
		int nCells = anMath::Pow( 2, i * 2 );

		nodes[ i ] = new arcQuadNode* [ nCells ];
		memset( nodes[ i ], 0, nCells * sizeof( arcQuadNode* ) );
	}

	// create head node
	headNode = new arcQuadNode;
	headNode->SetBounds( bounds );

	// put in node array
	nodes[ 0 ][ 0 ] = headNode;
}

/*
================
arcQuadTree<type>::~arcQuadTree
================
*/
template< class type >
ARC_INLINE arcQuadTree<type>::~arcQuadTree( void ) {
	if ( nodes ) {
		for ( int i = 0; i < depth; i++ ) {
			int nCells = static_cast< int >( anMath::Pow( 2.f, i * 2.f ) );

			for ( int j = 0; j < nCells; j++ ) {
				if ( nodes[ i ][ j ] ) {
					delete nodes[ i ][ j ];
				}
			}
			delete [] nodes[ i ];
		}
		delete [] nodes;
	}
}

/*
================
arcQuadTree<type>::GetUsedDepth_r
================
*/
template< class type >
void arcQuadTree<type>::GetUsedDepth_r( arcQuadNode &node, const int currentDepth, int *maxReachedDepth ) const {
	if ( currentDepth > *maxReachedDepth ) {
		*maxReachedDepth = currentDepth;
	}

	for ( int i = 0; i < 4; i++ ) {
		arcQuadNode *child = node.GetChild( i );
		if ( child ) {
			if ( currentDepth + 1 < depth ) {
				GetUsedDepth_r( *child, currentDepth + 1, maxReachedDepth );
			}
		}
	}
}

/*
================
arcQuadTree<type>::GetUsedDepth
================
*/
template< class type >
ARC_INLINE const int arcQuadTree<type>::GetUsedDepth( void ) const {
	int maxReachedDepth = 0;
	GetUsedDepth_r( *headNode, 1, &maxReachedDepth );
	return maxReachedDepth + 1;
}

/*
================
arcQuadTree<type>::BuildQuadTree
================
*/
template< class type >
ARC_INLINE void arcQuadTree<type>::BuildQuadTree( void ) {
#if 1
	FindChildren_r( *headNode, 1, 0, 0 );
#else
	FindChildren_r( *headNode, 1 );
#endif
	FindNeighbors_r( 1 );
}

/*
================
arcQuadTree<type>::BuildQuadTree
================
*/
template< class type >
ARC_INLINE void arcQuadTree<type>::BuildQuadTree( typename arcQuadTree<type>::arcQuadNode &node ) {
	// TODO
}

/*
================
arcQuadTree<type>::FindNode
================
*/
template< class type >
ARC_INLINE typename arcQuadTree<type>::arcQuadNode * arcQuadTree<type>::FindNode( const anVec3 &point ) {
	if ( !bounds.ContainsPoint( point ) ) {
		return NULL;
	}

	int x = (int)( ( point.x - bounds[ 0 ].x ) * nodeScale.x );
	int y = (int)( ( point.y - bounds[ 0 ].y ) * nodeScale.y );

	for ( int nodeDepth = depth - 1; nodeDepth >= 0; nodeDepth--, x >>= 1, y >>= 1 ) {
		arcQuadNode	*node = nodes[ nodeDepth ][ ( y << nodeDepth ) + x ];
		if ( node ) {
			return node;
		}
	}

	// should never happen
	return NULL;
}

/*
================
arcQuadTree<type>::GetNode( const anBounds & )
================
*/
template< class type >
ARC_INLINE typename arcQuadTree<type>::arcQuadNode * arcQuadTree<type>::GetNode( const anBounds &bounds ) {
	int x = (int)( ( bounds[ 0 ].x - this->bounds[ 0 ].x ) * nodeScale.x );
	int y = (int)( ( bounds[ 0 ].y - this->bounds[ 0 ].y ) * nodeScale.y );
	int xR = x ^ (int)( ( bounds[ 1 ].x - this->bounds[ 0 ].x ) * nodeScale.x );
	int yR = y ^ (int)( ( bounds[ 1 ].y - this->bounds[ 0 ].y ) * nodeScale.y );

	int nodeDepth = depth;

	// OPTIMIZE: for x86, optimise using BSR ?
	int shifted = 0;

	while ( xR + yR != 0 ) {
		xR >>= 1;
		yR >>= 1;
		nodeDepth--;
		shifted++;
	}

	x >>= shifted;
	y >>= shifted;

	arcQuadNode** node = &nodes[ nodeDepth - 1 ][ ( y << ( nodeDepth - 1 ) ) + x ];

	if ( *node ) {
		return *node;
	} else {
		return AllocNode( node, nodeDepth - 1, x, y );
	}
}

/*
================
arcQuadTree<type>::GetNode( const nodePosition_t &nodePosition  )
================
*/
template< class type >
ARC_INLINE typename arcQuadTree<type>::arcQuadNode * arcQuadTree<type>::GetNode( const nodePosition_t &nodePosition ) {
	arcQuadNode** node = &nodes[ nodePosition.level ][ ( nodePosition.y << ( nodePosition.level ) ) + nodePosition.x ];

	if ( *node ) {
		return *node;
	} else {
		return AllocNode( node, nodePosition.level, nodePosition.x, nodePosition.y );
	}
}

/*
================
arcQuadTree<type>::GetNumLeafNodes_r
================
*/
template< class type >
void arcQuadTree<type>::GetNumLeafNodes_r( arcQuadNode &node, int *numLeafNodes ) const {
	if ( !node.HasChildren() ) {
		(*numLeafNodes)++;
		return;
	}

	for ( int i = 0; i < 4; i++ ) {
		arcQuadNode *child = node.GetChild(i);
		if ( child ) {
			GetNumLeafNodes_r( *child, numLeafNodes );
		}
	}
}

/*
================
arcQuadTree<type>::GetNumLeafNodes
================
*/
template< class type >
const int arcQuadTree<type>::GetNumLeafNodes( void ) const {
	int	numLeafNodes = 0;
	GetNumLeafNodes_r( *headNode, &numLeafNodes );
	return numLeafNodes;
}

/*
================
arcQuadTree<type>::AllocNode
================
*/
template< class type >
ARC_INLINE typename arcQuadTree<type>::arcQuadNode * arcQuadTree<type>::AllocNode( arcQuadNode **node, int nodeLevel, int x, int y ) {
	int levelDimensions = anMath::Pow( 2, nodeLevel );
	anVec2 cellSize( ( headNode->GetBounds()[ 1 ].x - headNode->GetBounds()[ 0 ].x ) / levelDimensions,
	anVec2 pCellsize;

	// create the new node
	anVec2 nodeBounds.Clear();
	anVec2 nodeMins.Set( headNode->GetBounds()[ 0 ].x + x * cellSize.x, headNode->GetBounds()[ 0 ].y + y * cellSize.y );
	anBounds nodeBounds.AddPoint( anVec3( nodeMins.x, nodeMins.y, 0.f ) );
	anBounds nodeBounds.AddPoint( anVec3( nodeMins.x + cellSize.x, nodeMins.y + cellSize.y, 0.f ) );
	*node = new arcQuadNode( nodeBounds );

	(*node)->SetNodePosition( nodeLevel, x, y );

	// find (and create) all its parents
	arcQuadNode** parent;
	arcQuadNode** child = node;
	int pX = x;
	int pY = y;
	int pNodeLevel = nodeLevel;
	do {
		intpX >>= 1;
		pY >>= 1;
		pNodeLevel--;

		parent = &nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];

		if ( !(*parent) ) {
			levelDimensions = anMath::Pow( 2, pNodeLevel );
			pCellsize.Set( ( headNode->GetBounds()[ 1 ].x - headNode->GetBounds()[ 0 ].x ) / levelDimensions,
						   ( headNode->GetBounds()[ 1 ].y - headNode->GetBounds()[ 0 ].y ) / levelDimensions );

			// create the new node
			nodeBounds.Clear();
			nodeMins.Set( headNode->GetBounds()[ 0 ].x + pX * pCellsize.x, headNode->GetBounds()[ 0 ].y + pY * pCellsize.y );
			nodeBounds.AddPoint( anVec3( nodeMins.x, nodeMins.y, 0.f ) );
			nodeBounds.AddPoint( anVec3( nodeMins.x + pCellsize.x, nodeMins.y + pCellsize.y, 0.f ) );
			*parent = new arcQuadNode( nodeBounds );

			(*parent)->SetNodePosition( pNodeLevel, pX, pY );
		}

		(*child)->SetParent( *(*parent) );

		child = parent;
	} while( *parent != headNode && !(*child)->GetParent() );

	// create its siblings
	pX = x & ~1;
	pY = y & ~1;
	for ( x = pX; x < pX + 2; x++ ) {
		for ( y = pY; y < pY + 2; y++ ) {
			arcQuadNode** sibling = &nodes[ nodeLevel ][ ( y << nodeLevel ) + x ];
			if ( sibling == node ) {
				continue;
			}

			// create the new node
			nodeBounds.Clear();
			nodeMins.Set( headNode->GetBounds()[ 0 ].x + x * cellSize.x, headNode->GetBounds()[ 0 ].y + y * cellSize.y );
			nodeBounds.AddPoint( anVec3( nodeMins.x, nodeMins.y, 0.f ) );
			nodeBounds.AddPoint( anVec3( nodeMins.x + cellSize.x, nodeMins.y + cellSize.y, 0.f ) );
			*sibling = new arcQuadNode( nodeBounds );
			(*sibling)->SetParent( *((*node)->GetParent()) );
			(*sibling)->SetNodePosition( nodeLevel, x, y );
		}
	}

	return *node;
}

/*
================
arcQuadTree<type>::FindChildren_r
================
*/
template< class type >
void arcQuadTree<type>::FindChildren_r( arcQuadNode &parent, const int nodeLevel ) {
	int levelDimensions = anMath::Pow( 2, nodeLevel );

	// find all nodes with this node as a parent
	for ( int x = 0; x < levelDimensions; x++ ) {
		for ( int y = 0; y < levelDimensions; y++ ) {
			arcQuadNode *child = nodes[ nodeLevel ][ ( y << nodeLevel ) + x ];
			if ( child && child->GetParent() == &parent ) {
				parent.SetChild( *child, (y % 2) * 2 + (x % 2) );
				if ( nodeLevel + 1 < depth ) {
					FindChildren_r( *child, nodeLevel + 1 );
				}
			}
		}
	}
}

/*
================
arcQuadTree<type>::FindChildren_r
================
*/
template< class type >
void arcQuadTree<type>::FindChildren_r( arcQuadNode &parent, const int nodeLevel, const int parentX, const int parentY ) {
	// find all nodes with this node as a parent
	for ( int x = parentX; x < parentX + 2; x++ ) {
		for ( int y = parentY; y < parentY + 2; y++ ) {
			arcQuadNode *child = nodes[ nodeLevel ][ ( y << nodeLevel ) + x ];
			if ( child ) {
				parent.SetChild( *child, ( ( y - parentY ) << 1 ) + ( x - parentX ) );

				if ( nodeLevel + 1 < depth ) {
					// transform parent coordinates to lower level coordinates
					FindChildren_r( *child, nodeLevel + 1, x << 1, y << 1 );
				}
			}
		}
	}
}

/*
================
arcQuadTree<type>::FindNeighbors_r
================
*/
template< class type >
void arcQuadTree<type>::FindNeighbors_r( const int nodeLevel ) {
	int levelDimensions = anMath::Pow( 2, nodeLevel );

	for ( int x = 0; x < levelDimensions; x++ ) {
		for ( int y = 0; y < levelDimensions; y++ ) {
			node = nodes[ nodeLevel ][ ( y << nodeLevel ) + x ];
			if ( !node ) {
				continue;
			}

			// bottom neighbor (0)
			if ( y > 0 ) {
				int nbX = x;
				int nbY = y - 1;

				arcQuadNode *neighbor = nodes[ nodeLevel ][ ( nbY << nodeLevel ) + nbX ];

				// first see if we have a neighbor on this level
				if ( neighbor ) {
					node->SetNeighbor( *neighbor, 0 );
				} else if ( !(y & 1) ) {
					// try higher levels ( till > 0 as the headnode doesn't have any neighbors )
					for ( int pNodeLevel = nodeLevel - 1, int pX = nbX >> 1, int pY = nbY >> 1; pNodeLevel > 0; pNodeLevel--, pX >>= 1, pY >>= 1 ) {
						neighbor = nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];
						if ( neighbor ) {
							node->SetNeighbor( *neighbor, 0 );
							break;
						}
					}
				}
			}

            // left neighbor (1)
			if ( x > 0 ) {
				int nbX = x - 1;
				int nbY = y;

				arcQuadNode	*neighbor = nodes[ nodeLevel ][ ( nbY << nodeLevel ) + nbX ];

				// first see if we have a neighbor on this level
				if ( neighbor ) {
					node->SetNeighbor( *neighbor, 1 );
				} else if ( !(x & 1) ) {
					// try higher levels ( till > 0 as the headnode doesn't have any neighbors )
					for ( pNodeLevel = nodeLevel - 1, pX = nbX >> 1, pY = nbY >> 1; pNodeLevel > 0; pNodeLevel--, pX >>= 1, pY >>= 1 ) {
						neighbor = nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];
						if ( neighbor ) {
							node->SetNeighbor( *neighbor, 1 );
							break;
						}
					}
				}
			}

			// right neighbor (2)
			if ( x < levelDimensions - 1 ) {
				int nbX = x + 1;
				int nbY = y;

				arcQuadNode	*neighbor = nodes[ nodeLevel ][ ( nbY << nodeLevel ) + nbX ];

				// first see if we have a neighbor on this level
				if ( neighbor ) {
					node->SetNeighbor( *neighbor, 2 );
				} else if ( x & 1 ) {
					// try higher levels ( till > 0 as the headnode doesn't have any neighbors )
					for ( pNodeLevel = nodeLevel - 1, pX = nbX >> 1, pY = nbY >> 1; pNodeLevel > 0; pNodeLevel--, pX >>= 1, pY >>= 1 ) {
						neighbor = nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];
						if ( neighbor ) {
							node->SetNeighbor( *neighbor, 2 );
							break;
						}
					}
				}
			}

			// top neighbor (3)
			if ( y < levelDimensions - 1 ) {
				int nbX = x;
				int nbY = y + 1;

				arcQuadNode	*neighbor = nodes[ nodeLevel ][ ( nbY << nodeLevel ) + nbX ];

				// first see if we have a neighbor on this level
				if ( neighbor ) {
					node->SetNeighbor( *neighbor, 3 );
				} else if ( y & 1 ) {
					// try higher levels ( till > 0 as the headnode doesn't have any neighbors )
					for ( pNodeLevel = nodeLevel - 1, pX = nbX >> 1, pY = nbY >> 1; pNodeLevel > 0; pNodeLevel--, pX >>= 1, pY >>= 1 ) {
						neighbor = nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];
						if ( neighbor ) {
							node->SetNeighbor( *neighbor, 3 );
							break;
						}
					}
				}
			}
		}
	}

	if ( nodeLevel + 1 < depth ) {
		FindNeighbors_r( nodeLevel + 1 );
	}
}

/*
================
arcQuadTree<type>::FindNeighbors_r
================
*/
template< class type >
void arcQuadTree<type>::CreateChildren( arcQuadNode &parent ) {
	int				parentX, parentY;
	nodePosition_t	parentNodePosition = parent.GetNodePosition();

	if ( parentNodePosition.level + 1 >= depth ) {
		return;
	}

	int parentX = parentNodePosition.x << 1;
	int parentY = parentNodePosition.y << 1;

	// create all the nodes children
	for ( int x = parentX; x < parentX + 2; x++ ) {
		for ( int y = parentY; y < parentY + 2; y++ ) {
			arcQuadNode **child = &nodes[ parentNodePosition.level + 1 ][ ( y << (parentNodePosition.level + 1) ) + x ];

			if ( !(*child) ) {
				AllocNode( child, parentNodePosition.level + 1 , x, y );
			}

			parent.SetChild( **child, ( ( y - parentY ) << 1 ) + ( x - parentX ) );
		}
	}
}

#endif /* !__QUADTREE_H__ */

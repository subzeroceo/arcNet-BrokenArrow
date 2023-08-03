#ifndef __BLOCKPOOL_H__
#define __BLOCKPOOL_H__

#ifdef _MEM_SYS_SUPPORT

/*
===============================================================================

	Block based allocator for fixed size objects.

	All objects of the 'type' are properly constructed.
	However, the constructor is not called for re-used objects.

	This is essentially the same as anBlockAlloc found in heap.h. The difference
	is that anBlockQue is aware of which system heap it should allocate into.

===============================================================================
*/

template<class type, int blockSize, byte memoryTag, sys_Heap_ID_t heapID>
class anBlockQue {
public:
							anBlockQue( void );
							~anBlockQue( void );

	void					Shutdown( void );

	type *					Alloc( void );
	void					Free( type *element );

	int						GetTotalCount( void ) const { return total; }
	int						GetAllocCount( void ) const { return active; }
	int						GetFreeCount( void ) const { return total - active; }

	size_t					Allocated( void ) const { return( total * sizeof( type ) ); }

private:
	typedef struct element_s {
		struct element_s *	next;
		type				t;
	} element_t;
	typedef struct block_s {
		element_t			elements[blockSize];
		struct block_s *	next;
	} block_t;

	block_t *				blocks;
	element_t *				free;
	int						total;
	int						active;
};

template<class type, int blockSize, byte memoryTag, sys_Heap_ID_t heapID>
anBlockQue<type,blockSize,memoryTag,heapID>::anBlockQue( void ) {
	blocks = nullptr;
	free = nullptr;
	total = active = 0;
}

template<class type, int blockSize, byte memoryTag, sys_Heap_ID_t heapID>
anBlockQue<type,blockSize,memoryTag,heapID>::~anBlockQue( void ) {
	Shutdown();
}

template<class type, int blockSize, byte memoryTag, sys_Heap_ID_t heapID>
type *anBlockQue<type,blockSize,memoryTag,heapID>::Alloc( void ) {
	if ( !free ) {
		PUSH_SYS_HEAP_ID(heapID);
		block_t *block = new block_t;
		POP_HEAP();
		block->next = blocks;
		blocks = block;
		for ( int i = 0; i < blockSize; i++ ) {
			block->elements[i].next = free;
			free = &block->elements[i];
		}
		total += blockSize;
	}
	active++;
	element_t *element = free;
	free = free->next;
	element->next = nullptr;
	return &element->t;
}

template<class type, int blockSize, byte memoryTag, sys_Heap_ID_t heapID>
void anBlockQue<type,blockSize,memoryTag,heapID>::Free( type *t ) {
	element_t *element = (element_t *)( ( (unsigned char *) t ) - ( (int) &((element_t *)0)->t ) );
	element->next = free;
	free = element;
	active--;
}

template<class type, int blockSize, byte memoryTag, sys_Heap_ID_t heapID>
void anBlockQue<type,blockSize,memoryTag,heapID>::Shutdown( void ) {
	while( blocks ) {
		block_t *block = blocks;
		blocks = blocks->next;
		delete block;
	}
	blocks = nullptr;
	free = nullptr;
	total = active = 0;
}

#endif // _MEM_SYS_SUPPORT
#endif // __BLOCKPOOL_H__

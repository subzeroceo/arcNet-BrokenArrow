#ifndef __HEAP_H__
#define __HEAP_H__

/*
===============================================================================

	Memory Management

	This is a replacement for the compiler heap code ( i.e. "C" malloc() and
	free() calls). On average 2.5-3.0 times faster than MSVC malloc()/free().
	Worst case performance is 1.65 times faster and best case > 70 times.

===============================================================================
*/

#include <cassert>
#include <cstddef>
typedef struct {
	int		num;
	int		minSize;
	int		maxSize;
	int		totalSize;
} memoryStats_t;

void		Mem_Init( void );
void		Mem_Shutdown( void );
void		Mem_EnableLeakTest( const char *name );
void		Mem_ClearFrameStats( void );
void		Mem_GetFrameStats( memoryStats_t &allocs, memoryStats_t &frees );
void		Mem_GetStats( memoryStats_t &stats );
void		Mem_Dump_f( const class anCommandArgs &args );
void		Mem_DumpCompressed_f( const class anCommandArgs &args );
void		Mem_AllocDefragBlock( void );

#ifndef ARC_DEBUG_MEMORY

void *		Mem_Alloc( const int size );
void *		Mem_ClearedAlloc( const int size );
void		Mem_Free( void *ptr );
char *		Mem_CopyString( const char *in );
void *		Mem_Alloc16( const int size );
void		Mem_Free16( void *ptr );

//#ifdef ARC_REDIRECT_NEWDELETE
ARC_INLINE void *operator new( size_t s ) {
	return Mem_Alloc( s );
}
ARC_INLINE void operator delete( void *p ) {
	Mem_Free( p );
}
ARC_INLINE void *operator new[]( size_t s ) {
	return Mem_Alloc( s );
}
ARC_INLINE void operator delete[]( void *p ) {
	Mem_Free( p );
}
//#endif

void *		Mem_Alloc( const int size, const char *fileName, const int lineNumber );
void *		Mem_ClearedAlloc( const int size, const char *fileName, const int lineNumber );
void		Mem_Free( void *ptr, const char *fileName, const int lineNumber );
char *		Mem_CopyString( const char *in, const char *fileName, const int lineNumber );
void *		Mem_Alloc16( const int size, const char *fileName, const int lineNumber );
void		Mem_Free16( void *ptr, const char *fileName, const int lineNumber );

//#ifdef ARC_REDIRECT_NEWDELETE
ARC_INLINE void *operator new( size_t s, int t1, int t2, char *fileName, int lineNumber ) {
	return Mem_Alloc( s, fileName, lineNumber );
}
ARC_INLINE void operator delete( void *p, int t1, int t2, char *fileName, int lineNumber ) {
	Mem_Free( p, fileName, lineNumber );
}
ARC_INLINE void *operator new[]( size_t s, int t1, int t2, char *fileName, int lineNumber ) {
	return Mem_Alloc( s, fileName, lineNumber );
}
ARC_INLINE void operator delete[]( void *p, int t1, int t2, char *fileName, int lineNumber ) {
	Mem_Free( p, fileName, lineNumber );
}
ARC_INLINE void *operator new( size_t s ) {
	return Mem_Alloc( s, "", 0 );
}
ARC_INLINE void operator delete( void *p ) {
	Mem_Free( p, "", 0 );
}
ARC_INLINE void *operator new[]( size_t s ) {
	return Mem_Alloc( s, "", 0 );
}
ARC_INLINE void operator delete[]( void *p ) {
	Mem_Free( p, "", 0 );
}

//#define ID_DEBUG_NEW						new( 0, 0, __FILE__, __LINE__ )
#undef new
#define new									ID_DEBUG_NEW
#endif

#define		Mem_Alloc( size )				Mem_Alloc( size, __FILE__, __LINE__ )
#define		Mem_ClearedAlloc( size )		Mem_ClearedAlloc( size, __FILE__, __LINE__ )
#define		Mem_Free( ptr )					Mem_Free( ptr, __FILE__, __LINE__ )
#define		Mem_CopyString( s )				Mem_CopyString( s, __FILE__, __LINE__ )
#define		Mem_Alloc16( size )				Mem_Alloc16( size, __FILE__, __LINE__ )
#define		Mem_Free16( ptr )				Mem_Free16( ptr, __FILE__, __LINE__ )
#define		Printer_Debug( msg )			Printer_Debug( msg, __FILE__, __LINE__ );
#define		Printer_Debug( msg )			Printer_Debug( msg, __FILE__, __LINE__ );
#endif

/*
===============================================================================

	Block based allocator for fixed size objects.

	All objects of the 'type' are properly constructed.
	However, the constructor is not called for re-used objects.

===============================================================================
*/

template<class type, int blockSize>
class anBlockAlloc {
public:
							anBlockAlloc( void );
							~anBlockAlloc( void );

	void					Shutdown( void );

	type *					Alloc( void );
	void					Free( type *element );

	int						GetTotalCount( void ) const { return total; }
	int						GetAllocCount( void ) const { return active; }
	int						GetFreeCount( void ) const { return total - active; }

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

template<class type, int blockSize>
anBlockAlloc<type,blockSize>::anBlockAlloc( void ) {
	blocks = nullptr;
	free = nullptr;
	total = active = 0;
}

template<class type, int blockSize>
anBlockAlloc<type,blockSize>::~anBlockAlloc( void ) {
	Shutdown();
}

template<class type, int blockSize>
type *anBlockAlloc<type,blockSize>::Alloc( void ) {
	if ( !free ) {
		block_t *block = new block_t;
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

template<class type, int blockSize>
void anBlockAlloc<type,blockSize>::Free( type *t ) {
	element_t *element = (element_t *)( ( (unsigned char *) t ) - ( ( int ) &((element_t *)0 )->t ) );
	element->next = free;
	free = element;
	active--;
}

template<class type, int blockSize>
void anBlockAlloc<type,blockSize>::Shutdown( void ) {
	while( blocks ) {
		block_t *block = blocks;
		blocks = blocks->next;
		delete block;
	}
	blocks = nullptr;
	free = nullptr;
	total = active = 0;
}

/*
==============================================================================

	Dynamic allocator, simple wrapper for normal allocations which can
	be interchanged with anDynamicBlockAlloc.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

template<class type, int baseBlockSize, int minBlockSize>
class anDynamicAlloc {
public:
									anDynamicAlloc( void );
									~anDynamicAlloc( void );

	void							Init( void );
	void							Shutdown( void );
	void							SetFixedBlocks( int numBlocks ) {}
	void							SetLockMemory( bool lock ) {}
	void							FreeEmptyBaseBlocks( void ) {}

	type *							Alloc( const int num );
	type *							Resize( type *ptr, const int num );
	void							Free( type *ptr );
	const char *					CheckMemory( const type *ptr ) const;

	int								GetNumBaseBlocks( void ) const { return 0; }
	int								GetBaseBlockMemory( void ) const { return 0; }
	int								GetNumUsedBlocks( void ) const { return numUsedBlocks; }
	int								GetUsedBlockMemory( void ) const { return usedBlockMemory; }
	int								GetNumFreeBlocks( void ) const { return 0; }
	int								GetFreeBlockMemory( void ) const { return 0; }
	int								GetNumEmptyBaseBlocks( void ) const { return 0; }

private:
	int								numUsedBlocks;			// number of used blocks
	int								usedBlockMemory;		// total memory in used blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear( void );
};

template<class type, int baseBlockSize, int minBlockSize>
anDynamicAlloc<type, baseBlockSize, minBlockSize>::anDynamicAlloc( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
anDynamicAlloc<type, baseBlockSize, minBlockSize>::~anDynamicAlloc( void ) {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicAlloc<type, baseBlockSize, minBlockSize>::Init( void ) {
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicAlloc<type, baseBlockSize, minBlockSize>::Shutdown( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
type *anDynamicAlloc<type, baseBlockSize, minBlockSize>::Alloc( const int num ) {
	numAllocs++;
	if ( num <= 0 ) {
		return nullptr;
	}
	numUsedBlocks++;
	usedBlockMemory += num * sizeof( type );
	return Mem_Alloc16( num * sizeof( type ) );
}

template<class type, int baseBlockSize, int minBlockSize>
type *anDynamicAlloc<type, baseBlockSize, minBlockSize>::Resize( type *ptr, const int num ) {
	numResizes++;

	if ( ptr == nullptr ) {
		return Alloc( num );
	}

	if ( num <= 0 ) {
		Free( ptr );
		return nullptr;
	}

	assert( 0 );
	return ptr;
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicAlloc<type, baseBlockSize, minBlockSize>::Free( type *ptr ) {
	numFrees++;
	if ( ptr == nullptr ) {
		return;
	}
	Mem_Free16( ptr );
}

template<class type, int baseBlockSize, int minBlockSize>
const char *anDynamicAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( const type *ptr ) const {
	return nullptr;
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicAlloc<type, baseBlockSize, minBlockSize>::Clear( void ) {
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;
}


/*
==============================================================================

	Fast dynamic block allocator.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

#include "BT"

//#define DYNAMIC_BLOCK_ALLOC_CHECK

template<class type>
class anDynamicAlloc {
public:
	type *							GetMemory( void ) const { return (type *)( ( (byte *) this ) + sizeof( anDynamicAlloc<type> ) ); }
	int								GetSize( void ) const { return abs( size ); }
	void							SetSize( int s, bool isBaseBlock ) { size = isBaseBlock ? -s : s; }
	bool							IsBaseBlock( void ) const { return ( size < 0 ); }

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								id[3];
	void *							allocator;
#endif

	int								size;					// size in bytes of the block
	anDynamicAlloc<type> *			prev;					// previous memory block
	anDynamicAlloc<type> *			next;					// next memory block
	anBinaryTreeNode<anDynamicAlloc<type>,int> *node;			// node in the B-Tree with free blocks
};

template<class type, int baseBlockSize, int minBlockSize>
class anDynamicBlockAlloc {
public:
									anDynamicBlockAlloc( void );
									~anDynamicBlockAlloc( void );

	void							Init( void );
	void							Shutdown( void );
	void							SetFixedBlocks( int numBlocks );
	void							SetLockMemory( bool lock );
	void							FreeEmptyBaseBlocks( void );

	type *							Alloc( const int num );
	type *							Resize( type *ptr, const int num );
	void							Free( type *ptr );
	const char *					CheckMemory( const type *ptr ) const;

	int								GetNumBaseBlocks( void ) const { return numBaseBlocks; }
	int								GetBaseBlockMemory( void ) const { return baseBlockMemory; }
	int								GetNumUsedBlocks( void ) const { return numUsedBlocks; }
	int								GetUsedBlockMemory( void ) const { return usedBlockMemory; }
	int								GetNumFreeBlocks( void ) const { return numFreeBlocks; }
	int								GetFreeBlockMemory( void ) const { return freeBlockMemory; }
	int								GetNumEmptyBaseBlocks( void ) const;

private:
	anDynamicAlloc<type> *			firstBlock;				// first block in list in order of increasing address
	anDynamicAlloc<type> *			lastBlock;				// last block in list in order of increasing address
	anBinaryTree<anDynamicAlloc<type>,int,4>freeTree;			// B-Tree with free memory blocks
	bool							allowAllocs;			// allow base block allocations
	bool							lockMemory;				// lock memory so it cannot get swapped out

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								blockId[3];
#endif

	int								numBaseBlocks;			// number of base blocks
	int								baseBlockMemory;		// total memory in base blocks
	int								numUsedBlocks;			// number of used blocks
	int								usedBlockMemory;		// total memory in used blocks
	int								numFreeBlocks;			// number of free blocks
	int								freeBlockMemory;		// total memory in free blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear( void );
	anDynamicAlloc<type> *			AllocInternal( const int num );
	anDynamicAlloc<type> *			ResizeInternal( anDynamicAlloc<type> *block, const int num );
	void							FreeInternal( anDynamicAlloc<type> *block );
	void							LinkFreeInternal( anDynamicAlloc<type> *block );
	void							UnlinkFreeInternal( anDynamicAlloc<type> *block );
	void							CheckMemory( void ) const;
};

template<class type, int baseBlockSize, int minBlockSize>
anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::anDynamicBlockAlloc( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::~anDynamicBlockAlloc( void ) {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Init( void ) {
	freeTree.Init();
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Shutdown( void ) {
	anDynamicAlloc<type> *block;

	for ( block = firstBlock; block != nullptr; block = block->next ) {
		if ( block->node == nullptr ) {
			FreeInternal( block );
		}
	}

	for ( block = firstBlock; block != nullptr; block = firstBlock ) {
		firstBlock = block->next;
		assert( block->IsBaseBlock() );
		if ( lockMemory ) {
			anLibrary::sys->UnlockMemory( block, block->GetSize() + ( int )sizeof( anDynamicAlloc<type> ) );
		}
		Mem_Free16( block );
	}

	freeTree.Shutdown();

	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::SetFixedBlocks( int numBlocks ) {
	anDynamicAlloc<type> *block;

	for ( int i = numBaseBlocks; i < numBlocks; i++ ) {
		block = ( anDynamicAlloc<type> * ) Mem_Alloc16( baseBlockSize );
		if ( lockMemory ) {
			anLibrary::sys->LockMemory( block, baseBlockSize );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = (void*)this;
#endif
		block->SetSize( baseBlockSize - ( int )sizeof( anDynamicAlloc<type> ), true );
		block->next = nullptr;
		block->prev = lastBlock;
		if ( lastBlock ) {
			lastBlock->next = block;
		} else {
			firstBlock = block;
		}
		lastBlock = block;
		block->node = nullptr;

		FreeInternal( block );

		numBaseBlocks++;
		baseBlockMemory += baseBlockSize;
	}

	allowAllocs = false;
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::SetLockMemory( bool lock ) {
	lockMemory = lock;
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::FreeEmptyBaseBlocks( void ) {
	anDynamicAlloc<type> *block, *next;

	for ( block = firstBlock; block != nullptr; block = next ) {
		next = block->next;

		if ( block->IsBaseBlock() && block->node != nullptr && ( next == nullptr || next->IsBaseBlock() ) ) {
			UnlinkFreeInternal( block );
			if ( block->prev ) {
				block->prev->next = block->next;
			} else {
				firstBlock = block->next;
			}
			if ( block->next ) {
				block->next->prev = block->prev;
			} else {
				lastBlock = block->prev;
			}
			if ( lockMemory ) {
				anLibrary::sys->UnlockMemory( block, block->GetSize() + ( int )sizeof( anDynamicAlloc<type> ) );
			}
			numBaseBlocks--;
			baseBlockMemory -= block->GetSize() + ( int )sizeof( anDynamicAlloc<type> );
			Mem_Free16( block );
		}
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize>
int anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::GetNumEmptyBaseBlocks( void ) const {
	int numEmptyBaseBlocks;
	anDynamicAlloc<type> *block;

	numEmptyBaseBlocks = 0;
	for ( block = firstBlock; block != nullptr; block = block->next ) {
		if ( block->IsBaseBlock() && block->node != nullptr && ( block->next == nullptr || block->next->IsBaseBlock() ) ) {
			numEmptyBaseBlocks++;
		}
	}
	return numEmptyBaseBlocks;
}

template<class type, int baseBlockSize, int minBlockSize>
type *anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Alloc( const int num ) {
	anDynamicAlloc<type> *block;

	numAllocs++;

	if ( num <= 0 ) {
		return nullptr;
	}

	block = AllocInternal( num );
	if ( block == nullptr ) {
		return nullptr;
	}
	block = ResizeInternal( block, num );
	if ( block == nullptr ) {
		return nullptr;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	numUsedBlocks++;
	usedBlockMemory += block->GetSize();

	return block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize>
type *anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Resize( type *ptr, const int num ) {

	numResizes++;

	if ( ptr == nullptr ) {
		return Alloc( num );
	}

	if ( num <= 0 ) {
		Free( ptr );
		return nullptr;
	}

	anDynamicAlloc<type> *block = ( anDynamicAlloc<type> * ) ( ( (byte *) ptr ) - ( int )sizeof( anDynamicAlloc<type> ) );

	usedBlockMemory -= block->GetSize();

	block = ResizeInternal( block, num );
	if ( block == nullptr ) {
		return nullptr;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	usedBlockMemory += block->GetSize();

	return block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Free( type *ptr ) {
	numFrees++;

	if ( ptr == nullptr ) {
		return;
	}

	anDynamicAlloc<type> *block = ( anDynamicAlloc<type> * ) ( ( (byte *) ptr ) - ( int )sizeof( anDynamicAlloc<type> ) );

	numUsedBlocks--;
	usedBlockMemory -= block->GetSize();

	FreeInternal( block );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize>
const char *anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( const type *ptr ) const {
	anDynamicAlloc<type> *block;

	if ( ptr == nullptr ) {
		return nullptr;
	}

	block = ( anDynamicAlloc<type> * ) ( ( (byte *) ptr ) - ( int )sizeof( anDynamicAlloc<type> ) );

	if ( block->node != nullptr ) {
		return "memory has been freed";
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	if ( block->id[0] != 0x11111111 || block->id[1] != 0x22222222 || block->id[2] != 0x33333333 ) {
		return "memory has invalid id";
	}
	if ( block->allocator != (void*)this ) {
		return "memory was allocated with different allocator";
	}
#endif

	/* base blocks can be larger than baseBlockSize which can cause this code to fail
	anDynamicAlloc<type> *base;
	for ( base = firstBlock; base != nullptr; base = base->next ) {
		if ( base->IsBaseBlock() ) {
			if ( ( ( int )block) >= ( ( int )base) && ( ( int )block) < ( ( int )base) + baseBlockSize ) {
				break;
			}
		}
	}
	if ( base == nullptr ) {
		return "no base block found for memory";
	}
	*/

	return nullptr;
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Clear( void ) {
	firstBlock = lastBlock = nullptr;
	allowAllocs = true;
	lockMemory = false;
	numBaseBlocks = 0;
	baseBlockMemory = 0;
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numFreeBlocks = 0;
	freeBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	blockId[0] = 0x11111111;
	blockId[1] = 0x22222222;
	blockId[2] = 0x33333333;
#endif
}

template<class type, int baseBlockSize, int minBlockSize>
anDynamicAlloc<type> *anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::AllocInternal( const int num ) {
	anDynamicAlloc<type> *block;
	int alignedBytes = ( num * sizeof( type ) + 15 ) & ~15;

	block = freeTree.FindSmallestLargerEqual( alignedBytes );
	if ( block != nullptr ) {
		UnlinkFreeInternal( block );
	} else if ( allowAllocs ) {
		int allocSize = Max( baseBlockSize, alignedBytes + ( int )sizeof( anDynamicAlloc<type> ) );
		block = ( anDynamicAlloc<type> * ) Mem_Alloc16( allocSize );
		if ( lockMemory ) {
			anLibrary::sys->LockMemory( block, baseBlockSize );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = (void*)this;
#endif
		block->SetSize( allocSize - ( int )sizeof( anDynamicAlloc<type> ), true );
		block->next = nullptr;
		block->prev = lastBlock;
		if ( lastBlock ) {
			lastBlock->next = block;
		} else {
			firstBlock = block;
		}
		lastBlock = block;
		block->node = nullptr;

		numBaseBlocks++;
		baseBlockMemory += allocSize;
	}

	return block;
}

template<class type, int baseBlockSize, int minBlockSize>
anDynamicAlloc<type> *anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::ResizeInternal( anDynamicAlloc<type> *block, const int num ) {
	int alignedBytes = ( num * sizeof( type ) + 15 ) & ~15;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == (void*)this );
#endif

	// if the new size is larger
	if ( alignedBytes > block->GetSize() ) {
		anDynamicAlloc<type> *nextBlock = block->next;
		// try to annexate the next block if it's free
		if ( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != nullptr &&
				block->GetSize() + ( int )sizeof( anDynamicAlloc<type> ) + nextBlock->GetSize() >= alignedBytes ) {
			UnlinkFreeInternal( nextBlock );
			block->SetSize( block->GetSize() + ( int )sizeof( anDynamicAlloc<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
			block->next = nextBlock->next;
			if ( nextBlock->next ) {
				nextBlock->next->prev = block;
			} else {
				lastBlock = block;
			}
		} else {
			// allocate a new block and copy
			anDynamicAlloc<type> *oldBlock = block;
			block = AllocInternal( num );
			if ( block == nullptr ) {
				return nullptr;
			}
			memcpy( block->GetMemory(), oldBlock->GetMemory(), oldBlock->GetSize() );
			FreeInternal( oldBlock );
		}
	}

	// if the unused space at the end of this block is large enough to hold a block with at least one element
	if ( block->GetSize() - alignedBytes - ( int )sizeof( anDynamicAlloc<type> ) < Max( minBlockSize, ( int )sizeof( type ) ) ) {
		return block;
	}

	anDynamicAlloc<type> *newBlock;

	newBlock = ( anDynamicAlloc<type> * ) ( ( (byte *) block ) + ( int )sizeof( anDynamicAlloc<type> ) + alignedBytes );
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	memcpy( newBlock->id, blockId, sizeof( newBlock->id ) );
	newBlock->allocator = (void*)this;
#endif
	newBlock->SetSize( block->GetSize() - alignedBytes - ( int )sizeof( anDynamicAlloc<type> ), false );
	newBlock->next = block->next;
	newBlock->prev = block;
	if ( newBlock->next ) {
		newBlock->next->prev = newBlock;
	} else {
		lastBlock = newBlock;
	}
	newBlock->node = nullptr;
	block->next = newBlock;
	block->SetSize( alignedBytes, block->IsBaseBlock() );

	FreeInternal( newBlock );

	return block;
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::FreeInternal( anDynamicAlloc<type> *block ) {
	assert( block->node == nullptr );
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == (void*)this );
#endif

	// try to merge with a next free block
	anDynamicAlloc<type> *nextBlock = block->next;
	if ( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != nullptr ) {
		UnlinkFreeInternal( nextBlock );
		block->SetSize( block->GetSize() + ( int )sizeof( anDynamicAlloc<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
		block->next = nextBlock->next;
		if ( nextBlock->next ) {
			nextBlock->next->prev = block;
		} else {
			lastBlock = block;
		}
	}

	// try to merge with a previous free block
	anDynamicAlloc<type> *prevBlock = block->prev;
	if ( prevBlock && !block->IsBaseBlock() && prevBlock->node != nullptr ) {
		UnlinkFreeInternal( prevBlock );
		prevBlock->SetSize( prevBlock->GetSize() + ( int )sizeof( anDynamicAlloc<type> ) + block->GetSize(), prevBlock->IsBaseBlock() );
		prevBlock->next = block->next;
		if ( block->next ) {
			block->next->prev = prevBlock;
		} else {
			lastBlock = prevBlock;
		}
		LinkFreeInternal( prevBlock );
	} else {
		LinkFreeInternal( block );
	}
}

template<class type, int baseBlockSize, int minBlockSize>
ARC_INLINE void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::LinkFreeInternal( anDynamicAlloc<type> *block ) {
	block->node = freeTree.Add( block, block->GetSize() );
	numFreeBlocks++;
	freeBlockMemory += block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize>
ARC_INLINE void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::UnlinkFreeInternal( anDynamicAlloc<type> *block ) {
	freeTree.Remove( block->node );
	block->node = nullptr;
	numFreeBlocks--;
	freeBlockMemory -= block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize>
void anDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( void ) const {
	anDynamicAlloc<type> *block;

	for ( block = firstBlock; block != nullptr; block = block->next ) {
		// make sure the block is properly linked
		if ( block->prev == nullptr ) {
			assert( firstBlock == block );
		} else {
			assert( block->prev->next == block );
		}
		if ( block->next == nullptr ) {
			assert( lastBlock == block );
		} else {
			assert( block->next->prev == block );
		}
	}
}

#endif

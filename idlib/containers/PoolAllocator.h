#ifndef __LIB_POOL_ALLOCATOR_H__
#define __LIB_POOL_ALLOCATOR_H__

namespace sdDetails {
/*
===============================================================================

Block based allocator for fixed size objects. (based off of anBlockAlloc)
All objects of the 'type' are NOT constructed.

===============================================================================
*/

template<class type, int blockSize>
class anPoolAlloc {
public:
	anPoolAlloc( void );
	~anPoolAlloc( void );

	void						Shutdown( void );

	type *						Alloc( void );
	void						Free( type *element );
	int							Compact( void );

	int							GetTotalCount( void ) const { return total; }
	int							GetAllocCount( void ) const { return active; }
	int							GetFreeCount( void ) const { return total - active; }

private:
	struct element_t {
		element_t *				next;
		char					t[ sizeof( type ) ];
	};
	struct block_t {
		element_t				elements[blockSize];
		block_t *				next;
	};

	block_t *					blocks;
	element_t *					free;
	int							total;
	int							active;
	int							numFree;

private:
	bool						IsFree( const element_t* element );
};

/*
============
anPoolAlloc<type,blockSize>::anPoolAlloc
============
*/
template<class type, int blockSize>
anPoolAlloc<type,blockSize>::anPoolAlloc( void ) {
	blocks = nullptr;
	free = nullptr;
	total = active = numFree = 0;
}

/*
============
anPoolAlloc<type,blockSize>::~anPoolAlloc
============
*/
template<class type, int blockSize>
anPoolAlloc<type,blockSize>::~anPoolAlloc( void ) {
	Shutdown();
}

/*
============
anPoolAlloc<type,blockSize>::IsFree
============
*/
template<class type, int blockSize>
bool anPoolAlloc<type,blockSize>::IsFree( const element_t* element ) {
	element_t* iter = free;
	while( iter != nullptr ) {
		if ( iter == element ) {
			return true;
		}
		iter = iter->next;
	}
	return false;
}

/*
============
anPoolAlloc<type,blockSize>::Alloc
============
*/
template<class type, int blockSize>
type *anPoolAlloc<type,blockSize>::Alloc( void ) {
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
	return reinterpret_cast< type* >( &element->t );
}

/*
============
anPoolAlloc<type,blockSize>::Free
============
*/
template<class type, int blockSize>
void anPoolAlloc<type,blockSize>::Free( type *t ) {
	if ( t == nullptr ) {
		return;
	}
	element_t *element = (element_t *)( ( (unsigned char *) t ) - ( (UINT_PTR) &((element_t *)0)->t ) );
	//element_t *element;
	//element = ( element_t * )( ( (unsigned char *)t ) - offsetof( element_t, t ) );
	element->next = free;
	free = element;
	active--;
	numFree++;
}

/*
============
anPoolAlloc<type,blockSize>::Compact
find blocks that are completely empty and free them

this is relatively slow and should only be called after an event that is known to
free a large chunk of elements

returns the number of bytes freed
============
*/
template<class type, int blockSize>
int anPoolAlloc<type,blockSize>::Compact( void ) {
	bool			usedHeap = false;
	block_t**		blocksToUnlink = nullptr;
	int				blocksToUnlinkNum = 0;

	if ( numFree >= ( 512 * 1024 ) ) {
		usedHeap = true;
		blocksToUnlink = static_cast< block_t** >( Mem_AllocAligned( numFree, ALIGN_16 ) );
	} else {
		blocksToUnlink = static_cast< block_t** >( _alloca16( numFree ) );
	}

	block_t* block = blocks;
	block_t* prev = nullptr;

	while( block != nullptr ) {
		bool allFreed = true;

		for( int i = 0; i < blockSize; i++ ) {
			if ( !IsFree( &block->elements[i] ) ) {
				allFreed = false;
				break;
			}
		}

		// all elements are in the free list, unlink and deallocate this block
		if ( allFreed ) {
			// unlink all elements from the free list
			for( int i = 0; i < blockSize; i++ ) {
				element_t* current = &block->elements[i];
				element_t* prev = nullptr;
				element_t* iter = free;
				while( iter != nullptr ) {
					element_t* next = iter->next;
					if ( iter == current ) {
						if ( prev != nullptr ) {
							prev->next = next;
						} else {
							free = next;
						}
						break;
					}
					prev = iter;
					iter = next;
				}
			}
			blocksToUnlink[ blocksToUnlinkNum++ ] = block;
		} else {
			prev = block;
		}
		block = block->next;
	}

	assert( blocksToUnlinkNum <= numFree );

	for( int i = 0; i < blocksToUnlinkNum; i++ ) {
		block_t* current = blocksToUnlink[i];
		block_t* prev = nullptr;
		block_t* iter = blocks;
		while( iter != nullptr ) {
			block_t* next = iter->next;
			if ( iter == current ) {
				if ( prev != nullptr ) {
					prev->next = next;
				} else {
					blocks = next;
				}
				break;
			}
			prev = iter;
			iter = next;
		}
		delete current;
	}

	if ( usedHeap ) {
		Mem_FreeAligned( blocksToUnlink );
		blocksToUnlink = nullptr;
	}

	numFree -= blocksToUnlinkNum;

	total -= blockSize * blocksToUnlinkNum;
	return blocksToUnlinkNum * blockSize * sizeof( type );
}

/*
============
anPoolAlloc<type,blockSize>::Shutdown
============
*/
template<class type, int blockSize>
void anPoolAlloc<type,blockSize>::Shutdown( void ) {
	while( blocks != nullptr ) {
		block_t *block = blocks;
		blocks = blocks->next;
		delete block;
	}
	blocks = nullptr;
	free = nullptr;
	total = active = 0;
}

}

// derive class T from sdPoolAllocator< T > to allocate all objects of type T from a memory pool
// use new and delete like normal on the objects
#define SD_DISAMBIGUATE_POOL_ALLOCATOR( Allocator )															\
	void* operator new[]( size_t size )					{ return Allocator::operator new[]( size ); }		\
	void operator delete[]( void* ptr, size_t size )	{ Allocator::operator delete[]( ptr, size ); }		\
	void* operator new( size_t size )					{ return Allocator::operator new( size ); }			\
	void operator delete( void* ptr, size_t size )		{ Allocator::operator delete( ptr, size ); }		\
	void* operator new[]( size_t size, size_t t1, int t2, char *fileName, int lineNumber )					{ return Allocator::operator new[]( size, t1, t2, fileName, lineNumber ); }	\
	void operator delete[]( void *ptr, size_t size, int t2, char *fileName, int lineNumber )				{ Allocator::operator delete[]( ptr, size, t2, fileName, lineNumber ); }	\
	void* operator new( size_t size, size_t t1, int t2, char *fileName, int lineNumber )					{ return Allocator::operator new( size, t1, t2, fileName, lineNumber ); }	\
	void operator delete( void *ptr, size_t size, int t2, char *fileName, int lineNumber )					{ Allocator::operator delete( ptr, size, t2, fileName, lineNumber ); }

extern const char sdPoolAllocator_DefaultIdentifier[];
class sdDynamicBlockManagerBase {
public:
	virtual				~sdDynamicBlockManagerBase() {}

	virtual void		Init() = 0;
	virtual	void		Shutdown() = 0;

	virtual void		PrintInfo() = 0;
	virtual void		Accumulate( int& baseBytes, int& freeBytes ) = 0;
	virtual const char *GetName() const = 0;
	virtual int			Compact() = 0;
	virtual void		Purge() = 0;
	virtual bool		IsValid() const = 0;

	static void			MemoryReport( const anCommandArgs& args );

	static void			InitPools();
	static void			ShutdownPools();
	static void			CompactPools();

protected:
	typedef anList< sdDynamicBlockManagerBase* > blockManagerList_t;
	static blockManagerList_t& GetList() {
		static blockManagerList_t list;
		return list;
	}
};

/*
============
sdDynamicBlockManager
============
*/
template< class T, const char *blockName, size_t blockSize >
class sdDynamicBlockManager :
	public sdDynamicBlockManagerBase {
public:
	typedef sdDetails::anPoolAlloc< T, blockSize > allocatorType_t;

					sdDynamicBlockManager() {
						Init();
						GetList().Append( this );
					}
					virtual	~sdDynamicBlockManager() {
					}

	virtual void	Init() {
						assert( allocator == nullptr );
						allocator = new allocatorType_t;
					}
	virtual void	Shutdown() {
						if ( allocator != nullptr ) {
							allocator->Shutdown();
							delete allocator;
							allocator = nullptr;
						}
						GetList().Remove( this );
					}

	const char*		GetName() const { return blockName; }

	T*				Alloc() {
						assert( allocator != nullptr );
						if ( allocator == nullptr ) {
							return nullptr;
						}
						return allocator->Alloc();
					}

	void			Free( T* ptr ) {
						assert( allocator != nullptr );
						if ( allocator == nullptr ) {
							return;
						}
						allocator->Free( ptr );
					}

	virtual int		Compact() {
						if ( allocator == nullptr ) {
							return 0;
						}
						return allocator->Compact();
					}

	virtual void	Purge() {
						if ( allocator != nullptr ) {
							allocator->Shutdown();
						}
					}

	virtual void	PrintInfo() {
						idLib::Printf( "\n%s\n", blockName );
						idLib::Printf( "===================\n", blockName );
						idLib::Printf( "Base Block Memory %i bytes free, %i bytes total\n", allocator->GetFreeCount() * sizeof( T ), allocator->GetTotalCount() * sizeof( T ) );
					}

	virtual void	Accumulate( int& baseBytes, int& freeBytes ) {
						assert( allocator != nullptr );
						baseBytes += allocator->GetTotalCount();
						freeBytes += allocator->GetFreeCount();
					}

	virtual bool	IsValid() const { return allocator != nullptr; }

private:
	allocatorType_t* allocator;
};

class sdLockingPolicy_None {
public:
	void Acquire() {}
	void Release() {}
};

class sdLockingPolicy_Lock {
public:
	void Acquire() { lock.Acquire(); }
	void Release() { lock.Release(); }

private:
	sdLock lock;
};

/*
============
sdPoolAllocator
============
*/
template< class T, const char *name = sdPoolAllocator_DefaultIdentifier, size_t baseBlockSize = 512, class lockingPolicy = sdLockingPolicy_None >
class sdPoolAllocator {

public:
	static const size_t ELEMENTS_PER_PAGE = baseBlockSize;

	// free the entire pool
	static void PurgeAllocator() {
		GetMemoryManager().Purge();
	}

	/*
	============
	operator new
	============
	*/
#ifdef ARC_REDIRECT_NEWDELETE
#undef new
#endif
	void* operator new( size_t size ) {
#ifdef ARC_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
		if ( size != sizeof( T )) {
			assert( 0 );
		}

		lock.Acquire();
		void* retVal = static_cast< void* >( GetMemoryManager().Alloc() );
		lock.Release();
		return retVal;
	}

#ifdef ARC_REDIRECT_NEWDELETE
#undef new
#endif
	void* operator new( size_t size, size_t t1, int t2, char *fileName, int lineNumber ) {
#ifdef ARC_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
		if ( size != sizeof( T )) {
			assert( 0 );
		}

		lock.Acquire();
		void* retVal = static_cast< void* >( GetMemoryManager().Alloc() );
		lock.Release();
		return retVal;
	}

	/*
	============
	operator delete
	============
	*/
	void operator delete( void* ptr, size_t size ) {
		lock.Acquire();
		GetMemoryManager().Free( static_cast< T* >( ptr ) );
		lock.Release();
	}

	/*
	============
	operator delete
	============
	*/
	void operator delete( void *ptr, size_t size, int t2, char *fileName, int lineNumber ) {
		lock.Acquire();
		GetMemoryManager().Free( static_cast< T* >( ptr ) );
		lock.Release();
	}

	/*
	============
	operator new[]
	============
	*/
#ifdef ARC_REDIRECT_NEWDELETE
#undef new
#endif
	void* operator new[]( size_t size ) {
#ifdef ARC_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
		return ::new char[ size ];
	}

	/*
	============
	operator new[]
	============
	*/
#ifdef ARC_REDIRECT_NEWDELETE
#undef new
#endif
	void* operator new[]( size_t size, size_t t1, int t2, char *fileName, int lineNumber ) {
#ifdef ARC_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
		return ::new char[ size ];
	}

	/*
	============
	operator delete[]
	============
	*/
	void operator delete[]( void* ptr, size_t size ) {
		::delete[]( static_cast< char *>( ptr ) );
	}

	/*
	============
	operator delete[]
	============
	*/
	void operator delete[]( void *ptr, size_t size, int t2, char *fileName, int lineNumber ) {
		::delete[]( static_cast< char *>( ptr ) );
	}

private:
	typedef lockingPolicy LockingPolicy;
	static LockingPolicy lock;

	typedef sdDynamicBlockManager< T, name, baseBlockSize > memoryManager_t;
	static memoryManager_t& GetMemoryManager() {
		static memoryManager_t manager;
		if ( !manager.IsValid() ) {
			manager.Init();
		}
		return manager;
	}
};

template< class T, const char *name, size_t baseBlockSize, class lockingPolicy >
typename sdPoolAllocator< T, name, baseBlockSize, lockingPolicy >::LockingPolicy sdPoolAllocator< T, name, baseBlockSize, lockingPolicy >::lock;

#endif // !__LIB_POOL_ALLOCATOR_H__

#ifndef _SYSMEM_H__
#define __SYSMEM_H__

typedef enum  {
	TIDHEAP_ID_DEFAULT,			// heap that exists on application startup
	TIDHEAP_ID_PERMANENT,		// heap for allocations that have permanent (application scope) lifetime
	TIDHEAP_ID_LEVEL,			// heap for allocations that have a level lifetime
	TIDHEAP_ID_MULTIPLE_FRAME,	// heap for run-time allocations that have a lifetime of multiple draw frames
	TIDHEAP_ID_SINGLE_FRAME,	// heap for run-time allocations that have a lifetime of a single draw frame
	TIDHEAP_ID_TEMPORARY,		// heap for objects that have a short lifetime (temporaries generally used for level loading)
	TIDHEAP_ID_IO_TEMP,			// heap for allocations that are temporaries used in I/O operations like level loading or writing out data
	maxHeapCount				// just a count, not a valid type
} sysHeap_ID_t;

static const uint MAX_SYSTEM_HEAPS	= ( uint ) maxHeapCount;

#ifdef _SYSMEM_
extern aRcSecondaryHeapArena *currentHeapArena;

// Functions for getting and setting the system heaps.
void	SetSysHeap( sysHeap_ID_t sysHeapID, aRcSecondaryHeap *heapPtr ); // associates a heap with the given system heap ID value
aRcSecondaryHeap* GetSysHeap( sysHeap_ID_t sysHeapID );		// retrieves the specified system heap
void	GetAllSysHeaps( aRcSecondaryHeap *destSystemHeapArray[MAX_SYSTEM_HEAPS] );	// retrieves all the MAX_SYSTEM_HEAPS heap pointers into the given array
void	idSetAllSysHeaps( aRcSecondaryHeap *srcSystemHeapArray[MAX_SYSTEM_HEAPS] );	// associates all the MAX_SYSTEM_HEAPS heap pointers from the given array with their corresponding id value
bool	PushHeapContainingMemory( const void* mem );		// pushes the heap containg the memory specified to the top of the arena stack, making it current - mwhitlock

void EnterArenaCriticalSection();							// enters the heap arena critical section
void ExitArenaCriticalSection();							// exits the heap arena critical section
void PushSysHeap( sysHeap_ID_t sysHeapID );					// pushes the system heap associated with the given identifier to the top of the arena stack, making it current

// Useful in situations where a heap is pushed, but a return on error or an
// exception could cause the stack to be unwound, bypassing the heap pop
// operation - mwhitlock.
class aRcAutoHeapCtxt {
	bool mPushed;

public:
	aRcAutoHeapCtxt( void ) :
		mPushed( false ) {
		// Should never call this.
		assert( 0 );
	}

	aRcAutoHeapCtxt( sysHeap_ID_t sysHeapID) :
		mPushed( false ) {
		PushSysHeap( sysHeapID );
		mPushed = true;
	}

	aRcAutoHeapCtxt( const void* mem ) :
		mPushed( false ) {
		mPushed = PushHeapContainingMemory( mem );
	}

	~aRcAutoHeapCtxt( void ) {
		if ( mPushed ) {
			currentHeapArena->Pop();
		}
	}
};

#define PUSH_SYS_HEAP_ID( sysHeapID )			PushSysHeap( sysHeapID )
#define PUSH_SYS_HEAP_ID_AUTO( varName, sysHeapI D ) aRcAutoHeapCtxt varName( sysHeapID )

#define PUSH_HEAP_MEM( memPtr )					PushHeapContainingMemory( memPtr )
#define PUSH_HEAP_MEM_AUTO( varName,memPtr )	aRcAutoHeapCtxt varName( memPtr )

//
//	PUSH_HEAP_PTR()
//	Local heaps used mainly by executable (arcLibrary and Main frontend game would use these only
//	if heap was passed in)
//
#define PUSH_HEAP_PTR( heapPtr )				( ( heapPtr )->PushCurrent() )

//
//	PUSH_SYS_HEAP()
//	Pop top of heap stack, regardless of how it was pushed.
//
#define POP_HEAP()								( currentHeapArena->Pop() )

// The following versions enter/exit the heap arena's critical section so that
// critical section protection remains active between a push/pop pair (NOTE that
// the heap and heap arena are always protected by critical sections within a single method call)
#define PUSH_SYS_HEAP_ENTER_CRIT_SECT( sysHeapID ){ idEnnterArenaCriticalSection(); PushSysHeap( sysHeapID ); }
#define PUSH_HEAP_ENTER_CRIT_SECT( heapPtr )	{ idEnnterArenaCriticalSection(); ( heapPtr )->PushCurrent(); }
#define POP_HEAP_EXIT_CRIT_SECT()				{ currentHeapArena->Pop(); rvExitArenaCriticalSection(); }
#else
#define PUSH_SYS_HEAP_ID( sysHeapID )
#define PUSH_SYS_HEAP_ID_AUTO( varName, sysHeapID )
#define PUSH_HEAP_MEM( memPtr )
#define PUSH_HEAP_MEM_AUTO( varName, memPtr )
#define PUSH_HEAP_PTR( heapPtr )
#define POP_HEAP()

#define PUSH_SYS_HEAP_ENTER_CRIT_SECT( sysHeapID )
#define PUSH_HEAP_ENTER_CRIT_SECT( heapPtr )
#define POP_HEAP_EXIT_CRIT_SECT()
#endif
#endif
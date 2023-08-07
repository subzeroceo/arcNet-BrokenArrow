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
void	SetSysHeap( sysHeap_ID_t sysHeapID, anSecondaryHeap *heapPtr ); // associates a heap with the given system heap ID value
anSecondaryHeap* GetSysHeap( sysHeap_ID_t sysHeapID );		// retrieves the specified system heap
void	GetAllSysHeaps( anSecondaryHeap *destSystemHeapArray[MAX_SYSTEM_HEAPS] );	// retrieves all the MAX_SYSTEM_HEAPS heap pointers into the given array
void	idSetAllSysHeaps( anSecondaryHeap *srcSystemHeapArray[MAX_SYSTEM_HEAPS] );	// associates all the MAX_SYSTEM_HEAPS heap pointers from the given array with their corresponding id value
bool	PushHeapContainingMemory( const void* mem );		// pushes the heap containg the memory specified to the top of the arena stack, making it current - mwhitlock

void EnterArenaCriticalSection();							// enters the heap arena critical section
void ExitArenaCriticalSection();							// exits the heap arena critical section
void PushSysHeap( sysHeap_ID_t sysHeapID );					// pushes the system heap associated with the given identifier to the top of the arena stack, making it current

// Useful in situations where a heap is pushed, but a return on error or an
// exception could cause the stack to be unwound, bypassing the heap pop
// operation - mwhitlock.
class anAutoHeapCtxt {
	bool mPushed;

public:
	anAutoHeapCtxt( void ) :
		mPushed( false ) {
		// Should never call this.
		assert( 0 );
	}

	anAutoHeapCtxt( sysHeap_ID_t sysHeapID) :
		mPushed( false ) {
		PushSysHeap( sysHeapID );
		mPushed = true;
	}

	anAutoHeapCtxt( const void* mem ) :
		mPushed( false ) {
		mPushed = PushHeapContainingMemory( mem );
	}

	~anAutoHeapCtxt( void ) {
		if ( mPushed ) {
			currentHeapArena->Pop();
		}
	}
};

#define PushSystemHeapID( sysHeapID )			PushSysHeap( sysHeapID )
#define PushSystemHeapIDAuto( varName, sysHeapI D ) anAutoHeapCtxt varName( sysHeapID )

#define PushHeapMemory( memPtr )					PushHeapContainingMemory( memPtr )
#define PushHeapMemAuto( varName,memPtr )	anAutoHeapCtxt varName( memPtr )

//
//	PushHeapPointer()
//	Local heaps used mainly by executable (anLibrary and Main frontend game would use these only
//	if heap was passed in)
//
#define PushHeapPointer( heapPtr )				( ( heapPtr )->PushCurrent() )

//
//	PUSH_SYS_HEAP()
//	Pop top of heap stack, regardless of how it was pushed.
//
#define PopSystemHeap()								( currentHeapArena->Pop() )

// The following versions enter/exit the heap arena's critical section so that
// critical section protection remains active between a push/pop pair (NOTE that
// the heap and heap arena are always protected by critical sections within a single method call)
#define PUSH_SYS_HEAP_ENTER_CRIT_SECT( sysHeapID ){ idEnnterArenaCriticalSection(); PushSysHeap( sysHeapID ); }
#define PUSH_HEAP_ENTER_CRIT_SECT( heapPtr )	{ idEnnterArenaCriticalSection(); ( heapPtr )->PushCurrent(); }
#define POP_HEAP_EXIT_CRIT_SECT()				{ currentHeapArena->Pop(); rvExitArenaCriticalSection(); }
#else
#define PushSystemHeapID( sysHeapID )
#define PushSystemHeapIDAuto( varName, sysHeapID )
#define PushHeapMemory( memPtr )
#define PushHeapMemAuto( varName, memPtr )
#define PushHeapPointer( heapPtr )
#define PopSystemHeap()

#define PUSH_SYS_HEAP_ENTER_CRIT_SECT( sysHeapID )
#define PUSH_HEAP_ENTER_CRIT_SECT( heapPtr )
#define POP_HEAP_EXIT_CRIT_SECT()
#endif
#endif
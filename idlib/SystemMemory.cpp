#include "/idlib/Lib.h"
#pragma hdrstop

#ifdef _SYSMEM_

static memoryStats_t	mem_total_allocs = { 0, 0x0fffffff, -1, 0 };
static memoryStats_t	mem_frame_allocs;
static memoryStats_t	mem_frame_frees;

aRcSecondaryHeapArena mainHeapArena;
aRcSecondaryHeapArena *currentHeapArena;		// this is the main heap arena that all other heaps use
aRcSecondaryHeap defaultHeap;					// this is the default system heap

static aRcSecondaryHeap *systemHeapArray[MAX_SYSTEM_HEAPS];		// array of pointers to idHeapSecondarys that are common to anLibrary, Game, and executable

#if defined(_WIN32)
static LPVOID sharedMem = nullptr;      // pointer to shared memory
static HANDLE hMapObject = nullptr;	// handle to file mapping
#endif

// Descriptions that go with each tag.  When updating the tag enum in Heap.h please
// update this list as well.
// (also update the list in Heap.cpp)
char *TagNames[] = {
	"none",
	"New operation",
	"default",
	"anBinaryLexer",
	"Parser",
	"blank",
	"blank",
	"blank",
	"Collision Model",
	"CVar",
	"Decl System",
	"File System",
	"Images",
	"Materials",
	"Models",
	"Fonts",
	"Main renderer",
	"Vertex data",
	"Sound",
	"Window",
	"Event loop",
	"Math - Matrices and vectors",
	"blank",
	"Dynamic Blocks",
	"Strings",
	"GUI",
	"blank",
	"blank",
	"blank",
	"blank",
	"blank",
	"Not Used"
};
template<int X>
class TagTableCheck {
private:
	TagTableCheck();
};

template<>
class TagTableCheck<1> {
};

// An error here means you need to synchronize TagNames and Mem_Alloc_Types_t
TagTableCheck<sizeof(TagNames)/sizeof(char*) == MA_MAX> TagTableCheckedHere;
// An error here means there are too many tags.  No more than 32!
TagTableCheck<MA_DO_NOT_USE<32> TagMaxCheckedHere;

#ifndef ENABLE_INTEL_SMP
MemScopedTag* MemScopedTag::mTop = nullptr;
#endif

/*
==================
GetMemAllocStats

Gets some memory allocation stats based on a particular tag.
==================
*/
const char *GetMemAllocStats( int tag, int &num, int &size, int &peak ) {
	assert( tag < MA_MAX );

	currentHeapArena->GetTagStats( tag, num, size, peak );
	return TagNames[tag];
}

/*
==================
idSetSystemHeap

sets the system heap that is associated with the given heap ID value
==================
*/
void idSetSystemHeap( sysHeap_ID_t sysHeapID, aRcSecondaryHeap *heapPtr ) {
	assert( ( uint ) sysHeapID < ( uint ) maxHeapCount );
	if ( nullptr == heapPtr ) {
		// set the system heap back to the default heap
		systemHeapArray[ ( uint ) sysHeapID ] = &defaultHeap;
	} else {
		systemHeapArray[ ( uint ) sysHeapID ] = heapPtr;
	}
}

/*
==================
 idPushSystemHeap

pushes the system heap associated with the given identifier to the top of the arena stack, making it current.
==================
*/
void idPushSystemHeap( sysHeap_ID_t sysHeapID) {
	assert( ( uint ) sysHeapID < ( uint ) maxHeapCount );
	assert( systemHeapArray[ ( uint ) sysHeapID ] != nullptr );
	systemHeapArray[ ( uint ) sysHeapID ]->PushCurrent();
}

/*
==================
 EnterArenaCriticalSection

enters the heap arena critical section.
==================
*/
void EnterArenaCriticalSection() {
	// the following is necessary for memory allocations that take place
	// in static/global object constructors
	if ( nullptr == currentHeapArena ) {
		Mem_Init();
	}
	assert( currentHeapArena != nullptr );
	currentHeapArena->EnterArenaCriticalSection();
}

/*
==================
 ExitArenaCriticalSection

exits the heap arena critical section
==================
*/
void ExitArenaCriticalSection() {
	assert( currentHeapArena != nullptr );
	currentHeapArena->ExitArenaCriticalSection();
}

/*
==================
Mem_Init

initializes the memory system for use
==================
*/
void Mem_Init( void ) {
	if ( nullptr == currentHeapArena ) {
#if defined(_WIN32)
        hMapObject = CreateFileMapping( INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof( systemHeapArray )+sizeof( currentHeapArena ), "teck4share" );		// name of map object
        if ( hMapObject == nullptr ) {
            return;
		}

        // The first process to attach initializes memory.
        bool firstInit = (GetLastError() != ERROR_ALREADY_EXISTS);

        // Get a pointer to the file-mapped shared memory.
        sharedMem = MapViewOfFile( hMapObject, FILE_MAP_WRITE, 0,0,0 0 );
        if ( sharedMem == nullptr ) {
            return;
		}

		if ( !firstInit ) {
			memcpy( &currentHeapArena, sharedMem, sizeof( currentHeapArena ) );
			memcpy( &systemHeapArray, (byte*) sharedMem + sizeof( currentHeapArena ), sizeof( systemHeapArray ) );

			UnmapViewOfFile( sharedMem );
			CloseHandle( hMapObject );

			sharedMem = nullptr;
			hMapObject = nullptr;
			return;
		}
#endif

		currentHeapArena = &mainHeapArena;
		currentHeapArena->Init();
		defaultHeap.Init( *currentHeapArena, 256*1024*1024 );
		defaultHeap.SetDebugID(0 );
		defaultHeap.SetName( "DEFAULT" );
		idSetSystemHeap( TIDHEAP_ID_DEFAULT, &defaultHeap );
		idSetSystemHeap( TIDHEAP_ID_PERMANENT, &defaultHeap );
		idSetSystemHeap( TIDHEAP_ID_LEVEL, &defaultHeap );
		idSetSystemHeap( TIDHEAP_ID_MULTIPLE_FRAME, &defaultHeap );
		idSetSystemHeap( TIDHEAP_ID_SINGLE_FRAME, &defaultHeap );
		idSetSystemHeap( TIDHEAP_ID_TEMPORARY, &defaultHeap );
		idSetSystemHeap( TIDHEAP_ID_IO_TEMP, &defaultHeap );
		PUSH_SYS_HEAP_ID( TIDHEAP_ID_DEFAULT );

#if defined(_WIN32)
		memcpy( sharedMem,  &currentHeapArena, sizeof( currentHeapArena ) );
		memcpy( (byte*) sharedMem + sizeof( currentHeapArena ), &systemHeapArray, sizeof( systemHeapArray ) );
#endif
	}
}

/*
==================
Mem_Shutdown

Shuts down the memory system from all further use
==================
*/
void Mem_Shutdown( void ) {
#if defined(_WIN32) && !defined(_XENON)
	if ( sharedMem != nullptr ) {
		UnmapViewOfFile( sharedMem );
		CloseHandle( hMapObject );
		sharedMem = nullptr;
		hMapObject = nullptr;
	}
#endif

	if ( currentHeapArena == &mainHeapArena ) {
		memset( systemHeapArray, 0, sizeof( systemHeapArray ) ) ;
		defaultHeap.Shutdown();
		currentHeapArena->Shutdown();
	}
}

void Mem_EnableLeakTest( const char *name ) {
}

/*
==================
Mem_UpdateStats
==================
*/
void Mem_UpdateStats( memoryStats_t &stats, int size )  {
	stats.num++;

	if ( size < stats.minSize ) {
		stats.minSize = size;
	}

	if ( size > stats.maxSize ) {
		stats.maxSize = size;
	}

	stats.totalSize += size;
}

/*
==================
Mem_UpdateAllocStats
==================
*/
void Mem_UpdateAllocStats( int size )  {
	Mem_UpdateStats( mem_frame_allocs, size );
	Mem_UpdateStats( mem_total_allocs, size );
}

/*
==================
Mem_UpdateFreeStats
==================
*/
void Mem_UpdateFreeStats( int size ) {
	Mem_UpdateStats( mem_frame_frees, size );
	mem_total_allocs.num--;
	mem_total_allocs.totalSize -= size;
}

/*
==================
Mem_ClearFrameStats
==================
*/
void Mem_ClearFrameStats( void ) {
	mem_frame_allocs.num = mem_frame_frees.num = 0;
	mem_frame_allocs.minSize = mem_frame_frees.minSize = 0x0fffffff;
	mem_frame_allocs.maxSize = mem_frame_frees.maxSize = -1;
	mem_frame_allocs.totalSize = mem_frame_frees.totalSize = 0;
}

/*
==================
Mem_GetFrameStats
==================
*/
void Mem_GetFrameStats( memoryStats_t &allocs, memoryStats_t &frees ) {
	allocs = mem_frame_allocs;
	frees = mem_frame_frees;
}

/*
==================
Mem_GetStats
==================
*/
void Mem_GetStats( memoryStats_t &stats ) {
	stats = mem_total_allocs;
}

void Mem_AllocDefragBlock( void ) {
}

/*
==================
Mem_ShowMemAlloc_f
==================
*/
void Mem_ShowMemAlloc_f( const anCommandArgs &args ) {
	const char *tagName;
	int tag, num, size, peak;
	DWORD totalOutstanding = 0;

	for ( tag = 1; tag < MA_DO_NOT_USE; tag++ ) {
		tagName = GetMemAllocStats( tag, num, size, peak );
		if ( size || peak ) {
			anLibrary::common->Printf( "%-25s peak %9d curr %9d count %9d\n", tagName, peak, size, num );
			totalOutstanding += size;
		}
	}
	anLibrary::common->Printf( "Mem_Alloc Outstanding: %d\n", totalOutstanding );
}

#ifdef ARC_DEBUG_MEMORY

#undef		Mem_Alloc
#undef		Mem_ClearedAlloc
#undef		Com_ClearedReAlloc
#undef		Mem_Free
#undef		Mem_CopyString
#undef		Mem_Alloc16
#undef		Mem_Free16

#define MAX_CALLSTACK_DEPTH		10

// size of this struct must be a multiple of 16 bytes
typedef struct debugMemory_s {
	const char *			fileName;
	short					lineNumber;
	byte					heapId;
	byte					memTag;
	int						frameNumber;
	int						size;
	address_t				callStack[MAX_CALLSTACK_DEPTH];
	struct debugMemory_s *	prev;
	struct debugMemory_s *	next;
} debugMemory_t;

static debugMemory_t *	mem_debugMemory = nullptr;
static char				mem_leakName[256] = "";

/*
==================
Mem_CleanupFileName
==================
*/
const char *Mem_CleanupFileName( const char *fileName ) {
	int i1, i2;
	anString newFileName;
	static char newFileNames[4][MAX_STRING_CHARS];
	static int index;

	newFileName = fileName;
	newFileName.BackSlashesToSlashes();
	i1 = newFileName.Find( "neo", false );

	if ( i1 >= 0 ) {
		i1 = newFileName.Find( "/", false, i1 );
		newFileName = newFileName.Right( newFileName.Length() - ( i1 + 1 ) );
	}

	while( 1 ) {
		i1 = newFileName.Find( "/../" );
		if ( i1 <= 0 ) {
			break;
		}

		i2 = i1 - 1;

		while( i2 > 1 && newFileName[i2-1] != '/' ) {
			i2--;
		}

		newFileName = newFileName.Left( i2 - 1 ) + newFileName.Right( newFileName.Length() - ( i1 + 4 ) );
	}

	index = ( index + 1 ) & 3;

	strncpy( newFileNames[index], newFileName.c_str(), sizeof( newFileNames[index] ) );

	return newFileNames[index];
}

/*
==================
Mem_Dump
==================
*/
void Mem_Dump( const char *fileName ) {
	int i, numBlocks, totalSize;
	char dump[32], *ptr;
	debugMemory_t *b;
	anString module, funcName;
	FILE *f;

	f = fopen( fileName, "wb" );
	if ( !f ) {
		return;
	}

	totalSize = 0;
	for ( numBlocks = 0, b = mem_debugMemory; b; b = b->next, numBlocks++ ) {
		ptr = ((char *) b) + sizeof(debugMemory_t);
		totalSize += b->size;
		for ( i = 0; i < ( sizeof(dump)-1 ) && i < b->size; i++ ) {
			if ( ptr[i] >= 32 && ptr[i] < 127 ) {
				dump[i] = ptr[i];
			} else {
				dump[i] = '_';
			}
		}

		dump[i] = '\0';

		if ( ( b->size >> 10 ) != 0 ) {
			fprintf( f, "size: %6d KB: %s, line: %d [%s], call stack: %s\r\n", ( b->size >> 10 ), Mem_CleanupFileName(b->fileName), b->lineNumber, dump, anLibrary::sys->GetCallStackStr( b->callStack, MAX_CALLSTACK_DEPTH ) );
		} else {
			fprintf( f, "size: %7d B: %s, line: %d [%s], call stack: %s\r\n", b->size, Mem_CleanupFileName(b->fileName), b->lineNumber, dump, anLibrary::sys->GetCallStackStr( b->callStack, MAX_CALLSTACK_DEPTH ) );
		}
	}

	anLibrary::sys->ShutdownSymbols();

	fprintf( f, "%8d total memory blocks allocated\r\n", numBlocks );
	fprintf( f, "%8d KB memory allocated\r\n", ( totalSize >> 10 ) );

	fclose( f );
}

/*
==================
Mem_Dump_f
==================
*/
void Mem_Dump_f( const anCommandArgs &args )  {
	const char *fileName;

	if ( args.Argc() >= 2 ) {
		fileName = args.Argv( 1 );
	} else {
		fileName = "memorydump.txt";
	}

	Mem_Dump( fileName );
}

typedef struct allocInfo_s {
	const char *			fileName;
	short					lineNumber;
	byte					heapId;
	byte					memTag;
	int						size;
	int						numAllocs;
	address_t				callStack[MAX_CALLSTACK_DEPTH];
	struct allocInfo_s *	next;
} allocInfo_t;

typedef enum {
	MEMSORT_SIZE,
	MEMSORT_LOCATION,
	MEMSORT_NUMALLOCS,
	MEMSORT_CALLSTACK,
	MEMSORT_HEAPID,
	MEMSORT_MEMTAG
} memorySortType_t;

/*
==================
Mem_DumpCompressed
==================
*/
void Mem_DumpCompressed( const char *fileName, memorySortType_t memSort, int sortCallStack, int numFrames, bool verbose  ) {
	int numBlocks, totalSize, r, j;
	debugMemory_t *b;
	allocInfo_t *a, *nexta, *allocInfo = nullptr, *sortedAllocInfo = nullptr, *prevSorted, *nextSorted;
	anString module, funcName;

	// build list with memory allocations
	totalSize = 0;
	numBlocks = 0;
	nextSorted = nullptr;

	for ( b = mem_debugMemory; b; b = b->next ) {
		if ( numFrames && b->frameNumber < anLibrary::frameNumber - numFrames ) {
			continue;
		}

		numBlocks++;
		totalSize += b->size;

		// search for an allocation from the same source location
		for ( a = allocInfo; a; a = a->next ) {
			if ( a->lineNumber != b->lineNumber ) {
				continue;
			}
			// removed the call stack info for better consolidation of info and speed of dump
			for ( j = 0; j < MAX_CALLSTACK_DEPTH; j++ ) {
				if ( a->callStack[j] != b->callStack[j] ) {
					break;
				}
			}
			if ( j < MAX_CALLSTACK_DEPTH ) {
				continue;
			}
			if ( anString::Cmp( a->fileName, b->fileName ) != 0 ) {
				continue;
			}

			if ( a->heapId != b->heapId ) {
				continue;
			}

			a->numAllocs++;
			a->size += b->size;
			break;
		}

		// if this is an allocation from a new source location
		if ( !a ) {
			a = (allocInfo_t *) currentHeapArena->Allocate( sizeof( allocInfo_t ) );
			a->fileName = b->fileName;
			a->lineNumber = b->lineNumber;
			a->heapId = b->heapId;
			a->memTag = b->memTag;
			a->size = b->size;
			a->numAllocs = 1;
			for ( j = 0; j < MAX_CALLSTACK_DEPTH; j++ ) {
				a->callStack[j] = b->callStack[j];
			}
			a->next = allocInfo;
			allocInfo = a;
		}
	}

	// sort list
	for ( a = allocInfo; a; a = nexta ) {
		nexta = a->next;
		prevSorted = nullptr;
		switch ( memSort ) {
			// sort on size
			case MEMSORT_SIZE: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->size > nextSorted->size ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on file name and line number
			case MEMSORT_LOCATION: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					r = anString::Cmp( Mem_CleanupFileName( a->fileName ), Mem_CleanupFileName( nextSorted->fileName ) );
					if ( r < 0 || ( r == 0 && a->lineNumber < nextSorted->lineNumber ) ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on the number of allocations
			case MEMSORT_NUMALLOCS: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->numAllocs > nextSorted->numAllocs ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on call stack
			case MEMSORT_CALLSTACK: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->callStack[sortCallStack] < nextSorted->callStack[sortCallStack] ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on heap ID
			case MEMSORT_HEAPID: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->heapId > nextSorted->heapId ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
		}
		if ( !prevSorted ) {
			a->next = sortedAllocInfo;
			sortedAllocInfo = a;
		} else {
			prevSorted->next = a;
			a->next = nextSorted;
		}
	}

	unsigned int notShownSize = 0;
	unsigned int notShownNumAllocs = 0;

	// write list to debug output and console
	char buff[256];
	for ( a = sortedAllocInfo; a; a = nexta ) {
		nexta = a->next;

		if ((a->size >> 10) > 0 ) {
			anString::snPrintf(buff, sizeof(buff), "%6d K",( a->size >> 10) );
		} else {
			anString::snPrintf(buff, sizeof(buff), "%6d B", a->size);
		}

		if ( verbose || ((a->size >> 10) > 0 ) ) {
				anLibrary::common->Printf( "size: %s, allocs: %5d: %s, line: %d, heap: %d, call stack: %s\r\n", buff, a->numAllocs, Mem_CleanupFileName(a->fileName), a->lineNumber, a->heapId, anLibrary::sys->GetCallStackStr( a->callStack, MAX_CALLSTACK_DEPTH ) );
		} else {
			notShownSize+=( unsigned int )a->size;
			notShownNumAllocs+=( unsigned int )a->numAllocs;
		}

		currentHeapArena->Free( a );
	}

	anLibrary::sys->ShutdownSymbols();

	anLibrary::common->Printf( "%8d bytes in %d allocs not shown\r\n", notShownSize, notShownNumAllocs );
	anLibrary::common->Printf( "%8d total memory blocks allocated\r\n", numBlocks );
	anLibrary::common->Printf( "%8d KB memory allocated\r\n", ( totalSize >> 10 ) );

	FILE *f;

// in case you want to flip the above ifdef and write to disc on the xenon
	f = fopen( fileName, "wb" );
	if ( !f ) {
		return;
	}

	// write list to file
	for ( a = sortedAllocInfo; a; a = nexta ) {
		nexta = a->next;
		fprintf( f, "size: %6d KB, allocs: %5d: %s, line: %d, call stack: %s\r\n",
					(a->size >> 10), a->numAllocs, Mem_CleanupFileName(a->fileName),	a->lineNumber, anLibrary::sys->GetCallStackStr( a->callStack, MAX_CALLSTACK_DEPTH ) );
		currentHeapArena->Free( a );
	}

	anLibrary::sys->ShutdownSymbols();

	fprintf( f, "%8d total memory blocks allocated\r\n", numBlocks );
	fprintf( f, "%8d KB memory allocated\r\n", ( totalSize >> 10 ) );

	fclose( f );
}

/*
==================
Mem_DumpCompressed_f
==================
*/
void Mem_DumpCompressed_f( const anCommandArgs &args ) {
	int argNum;
	const char *arg, *fileName;
	memorySortType_t memSort = MEMSORT_LOCATION;
	int sortCallStack = 0, numFrames = 0;
	bool verbose = false;

	// get cmd-line options
	argNum = 1;
	arg = args.Argv( argNum );
	while( arg[0] == '-' ) {
		arg = args.Argv( ++argNum );
		if ( anString::Icmp( arg, "s" ) == 0 ) {
			memSort = MEMSORT_SIZE;
		} else if ( anString::Icmp( arg, "l" ) == 0 ) {
			memSort = MEMSORT_LOCATION;
		} else if ( anString::Icmp( arg, "a" ) == 0 ) {
			memSort = MEMSORT_NUMALLOCS;
		} else if ( anString::Icmp( arg, "cs1" ) == 0 ) {
			memSort = MEMSORT_CALLSTACK;
			sortCallStack = 2;
		} else if ( anString::Icmp( arg, "cs2" ) == 0 ) {
			memSort = MEMSORT_CALLSTACK;
			sortCallStack = 1;
		} else if ( anString::Icmp( arg, "cs3" ) == 0 ) {
			memSort = MEMSORT_CALLSTACK;
			sortCallStack = 0;
		} else if ( anString::Icmp( arg, "h" ) == 0 ) {
			memSort = MEMSORT_HEAPID;
		} else if ( anString::Icmp( arg, "v" ) == 0 ) {
			verbose = true;
		} else if ( arg[0] == 'f' ) {
			numFrames = atoi( arg + 1 );
		} else {
			anLibrary::common->Printf( "memoryDumpCompressed [options] [filename]\n"
						"options:\n"
						"  -s     sort on size\n"
						"  -l     sort on location\n"
						"  -a     sort on the number of allocations\n"
						"  -cs1   sort on first function on call stack\n"
						"  -cs2   sort on second function on call stack\n"
						"  -cs3   sort on third function on call stack\n"
						"  -h     sort on heap ID\n"
						"  -f<X>  only report allocations the last X frames\n"
						"  -v     verbose (list all, even those totalling less than 1K\n"
						"By default the memory allocations are sorted on location.\n"
						"By default a 'memorydump.txt' is written if no file name is specified.\n" );
			return;
		}
		arg = args.Argv( ++argNum );
	}
	if ( argNum >= args.Argc() ) {
		fileName = "memorydump.txt";
	} else {
		fileName = arg;
	}
	Mem_DumpCompressed( fileName, memSort, sortCallStack, numFrames, verbose );
}

/*
==================
Mem_AllocDebugMemory
==================
*/
void *Mem_AllocDebugMemory( const int size, const char *fileName, const int lineNumber, const bool align16, byte tag ) {
	void *p;
	debugMemory_t *m;

	// the following is necessary for memory allocations that take place
	// in static/global object constructors
	if ( nullptr == currentHeapArena ) {
		Mem_Init();
	}

	if ( align16 ) {
		p = currentHeapArena->Allocate16( size + sizeof( debugMemory_t ), tag );
	} else {
		p = currentHeapArena->Allocate( size + sizeof( debugMemory_t ), tag );
	}

	if ( nullptr == p ) {
		return nullptr;
	}

	Mem_UpdateAllocStats( currentHeapArena->Msize( p ) );

	m = (debugMemory_t *) p;
	m->fileName = fileName;
	m->lineNumber = lineNumber;
	m->heapId = currentHeapArena->GetHeap(p)->DebugID();
	m->memTag = tag;
	m->frameNumber = anLibrary::frameNumber;
	m->size = size;
	m->next = mem_debugMemory;
	m->prev = nullptr;
	if ( mem_debugMemory ) {
		mem_debugMemory->prev = m;
	}
	mem_debugMemory = m;

	if ( anLibrary::sys != nullptr ) {
		anLibrary::sys->GetCallStack( m->callStack, MAX_CALLSTACK_DEPTH );
	} else {
		memset( m->callStack, 0, sizeof(m->callStack ) );
	}

	return ( ( (byte *) p ) + sizeof( debugMemory_t ) );
}

/*
==================
Mem_FreeDebugMemory
==================
*/
void Mem_FreeDebugMemory( void *p, const char *fileName, const int lineNumber, const bool align16 ) {
	debugMemory_t *m;

	if ( !p ) {
		return;
	}

	m = (debugMemory_t *) ( ( (byte *) p ) - sizeof( debugMemory_t ) );

	if ( m->size < 0 ) {
		anLibrary::common->FatalError( "memory freed twice, first from %s, now from %s", anLibrary::sys->GetCallStackStr( m->callStack, MAX_CALLSTACK_DEPTH ), anLibrary::sys->GetCallStackCurStr( MAX_CALLSTACK_DEPTH ) );
	}

	Mem_UpdateFreeStats( currentHeapArena->Msize( m ) );

	if ( m->next ) {
		m->next->prev = m->prev;
	}

	if ( m->prev ) {
		m->prev->next = m->next;
	} else {
		mem_debugMemory = m->next;
	}

	m->fileName = fileName;
	m->lineNumber = lineNumber;
	m->frameNumber = anLibrary::frameNumber;
	m->size = -m->size;
	anLibrary::sys->GetCallStack( m->callStack, MAX_CALLSTACK_DEPTH );

	assert( currentHeapArena != nullptr );
	currentHeapArena->Free( m );
}

/*
==================
Mem_Alloc
==================
*/
void *Mem_Alloc( const int size, const char *fileName, const int lineNumber, byte tag ) {
	return Mem_AllocDebugMemory( size, fileName, lineNumber, false, tag );
}

/*
==================
Mem_Free
==================
*/
void Mem_Free( void *ptr, const char *fileName, const int lineNumber ) {
	if ( !ptr ) {
		return;
	}
	Mem_FreeDebugMemory( ptr, fileName, lineNumber, false );
}

/*
==================
Mem_Alloc16
==================
*/
void *Mem_Alloc16( const int size, const char *fileName, const int lineNumber, byte tag ) {
	void *mem = Mem_AllocDebugMemory( size, fileName, lineNumber, true, tag );
	// make sure the memory is 16 byte aligned
	assert( ( ( ( int)mem) & 15) == 0 );
	return mem;
}

/*
==================
Mem_Free16
==================
*/
void Mem_Free16( void *ptr, const char *fileName, const int lineNumber ) {
	if ( !ptr ) {
		return;
	}
	// make sure the memory is 16 byte aligned
	assert( ( ( ( int)ptr) & 15) == 0 );
	Mem_FreeDebugMemory( ptr, fileName, lineNumber, true );
}

/*
==================
Mem_ClearedAlloc
==================
*/
void *Mem_ClearedAlloc( const int size, const char *fileName, const int lineNumber, byte tag ) {
	void *mem = Mem_Alloc( size, fileName, lineNumber, tag );

	if ( mem != nullptr ) {
		SIMDProcessor->Memset( mem, 0, size );
	} else  {
		anLibrary::common->FatalError( "Ran out of memory during a cleared allocation" );
	}
	return mem;
}

/*
==================
Mem_CopyString
==================
*/
char *Mem_CopyString( const char *in, const char *fileName, const int lineNumber ) {
	char	*out;

	out = (char *)Mem_Alloc( strlen(in) + 1, fileName, lineNumber );
	strcpy( out, in );
	return out;
}

/*
==================
Mem_Size
==================
*/
int Mem_Size( void *ptr ) {
	if ( !ptr ) {
		return 0;
	}
	assert( currentHeapArena != nullptr );
	return currentHeapArena->Msize( (byte*)ptr - sizeof( debugMemory_t ) );
}

#else	// #ifdef ARC_DEBUG_MEMORY

void Mem_Dump_f( const class anCommandArgs &args ) {
}

void Mem_DumpCompressed_f( const class anCommandArgs &args )
{

}

/*
==================
Mem_Alloc

Allocate memory from the heap at the top of the current arena stack
==================
*/
void *Mem_Alloc( const int size, byte tag ) {
	void *p;

	// the following is necessary for memory allocations that take place
	// in static/global object constructors
	if ( nullptr == currentHeapArena ) {
		Mem_Init();
	}

	p = currentHeapArena->Allocate( size, tag );
	if ( nullptr == p ) {
		return nullptr;
	}

	Mem_UpdateAllocStats( currentHeapArena->Msize( p ) );

#if defined( _XENON ) && !defined( _FINAL )
	MemTracker::OnAlloc(p, size);
#endif

	return p;
}

// Mem_ClearedAlloc
//
// Allocate memory from the heap at the top of the current arena stack,
// clear that memory to zero before returning it.
void *Mem_ClearedAlloc( const int size, byte tag ) {
	byte *allocation = (byte *) Mem_Alloc( size, tag );

#if defined( _XENON ) && !defined( _FINAL )
	MemTracker::OnAlloc(allocation, size);
#endif

	if ( allocation != nullptr )
	{
		SIMDProcessor->Memset( allocation, 0, size );
	} else {
		anLibrary::common->FatalError( "Ran out of memory during a cleared allocation" );
	}
	return allocation;

}

/*
==================
Mem_Free
==================
*/
void Mem_Free( void *ptr ) {
	if ( !ptr )  {
		return;
	}
	assert( currentHeapArena != nullptr );

#if defined( _XENON ) && !defined( _FINAL )
	MemTracker::OnDelete(ptr);
#endif

	Mem_UpdateFreeStats( currentHeapArena->Msize( ptr ) );

	currentHeapArena->Free( ptr );
}

/*
==================
Mem_CopyString

Allocates memory for a copy of the given string, and copies
that string into the new allocation.
==================
*/
char *Mem_CopyString( const char *in ) {
	char *out;

	out = (char *)Mem_Alloc( strlen(in) + 1, MA_STRING );
	if ( out != nullptr ) {
		strcpy( out, in );
	} else  {
		anLibrary::common->FatalError( "Ran out of memory during string copy allocation" );
	}
	return out;
}

/*
==================
Mem_Alloc16

Allocate memory from the heap at the top of the current arena
stack that is aligned on a 16-byte boundary.
==================
*/
void *Mem_Alloc16( const int size, byte tag ) {
	// the following is necessary for memory allocations that take place
	// in static/global object constructors
	if ( nullptr == currentHeapArena ) {
		Mem_Init();
	}
	void *mem = currentHeapArena->Allocate16( size, tag );

#if defined( _XENON ) && !defined( _FINAL )
	MemTracker::OnAlloc(mem, size);
#endif

	return mem;
}

/*
==================
Mem_Free
==================
*/
void Mem_Free16( void *ptr ) {
	if ( !ptr ) {
		return;
	}
	assert( currentHeapArena != nullptr );
	currentHeapArena->Free( ptr );

#if defined( _XENON ) && !defined( _FINAL )
	MemTracker::OnDelete(ptr);
#endif
}

/*
==================
Mem_Size
==================
*/
int Mem_Size( void *ptr ) {
	if ( !ptr ) {
		return 0;
	}
	assert( currentHeapArena != nullptr );
	return currentHeapArena->Msize( ptr );
}

#endif	// #else #ifdef ARC_DEBUG_MEMORY

/*
==================
GetAllSysHeaps

retrieves the specified system heap
==================
*/
aRcSecondaryHeap* GetSysHeap( sysHeap_ID_t sysHeapID ) {
	assert( ( uint ) sysHeapID < ( uint ) maxHeapCount );
	return systemHeapArray[ ( uint ) sysHeapID ];
}

/*
==================
GetAllSysHeaps

retrieves all the MAX_SYSTEM_HEAPS heap pointers into the given array
==================
*/
void GetAllSysHeaps( aRcSecondaryHeap *destSystemHeapArray[MAX_SYSTEM_HEAPS] ) {
	memcpy( destSystemHeapArray, systemHeapArray, sizeof( systemHeapArray ) );
}

/*
==================
idSetAllSysHeaps

associates all the MAX_SYSTEM_HEAPS heap pointers from the given array with their corresponding id value
==================
*/
void idSetAllSysHeaps( aRcSecondaryHeap *srcSystemHeapArray[MAX_SYSTEM_HEAPS] ) {
	memcpy( systemHeapArray, srcSystemHeapArray, sizeof( systemHeapArray ) ) ;
}

#if 0
/*
==================
idGetHeapContainingMemory

Returns pointer to the heap containg the memory specified. If the memory specified
is not in a heap, returns 0.
==================
*/
aRcSecondaryHeap* idGetHeapContainingMemory( const void* mem ) {
	return ( currentHeapArena!=0 )?currentHeapArena->GetHeap( const_cast<void*>( mem ) ):0;
}
#endif

/*
==================
PushHeapContainingMemory

Pushes the heap containg the memory specified. If the memory specified
is not in a heap, does nothing and returns false, otherwise returns
true on success.
==================
*/
bool PushHeapContainingMemory( const void* mem ) {
	if ( currentHeapArena ) {
		aRcSecondaryHeap* heap=currentHeapArena->GetHeap( const_cast<void*>( mem ) );
		if ( heap ) {
			heap->PushCurrent();
			return true;
		}
	}
	return false;
}


/*

This routine performs a tight packing of a list of rectangles, attempting to minimize the area
of the rectangle that encloses all of them.  Algorithm order is N^2, so it is not apropriate
for lists with many thousands of elements.

Contrast with idBitBlockAllocator, which is used incrementally with either fixed size or
size-doubling target areas.

Typical uses:
packing glyphs into a font image
packing model surfaces into a skin atlas
packing images into swf atlases

If you want a minimum alignment, ensure that all the sizes are multiples of that alignment,
or scale the input sizes down by that alignment and scale the outputPositions back up.

*/

float RectPackingFraction( const anList<anVec2i> &inputSizes, const anVec2i totalSize ) {
	int	totalArea = totalSize.Area();
	if ( totalArea == 0 ) {
		return 0;
	}
	int	inputArea = 0;
	for ( int i = 0; i < inputSizes.Num(); i++ ) {
		inputArea += inputSizes[i].Area();
	}
	return ( float )inputArea / totalArea;
}

class idSortrects : public anSortQuick< int, idSortrects > {
public:
	int SizeMetric( anVec2i v ) const {
		// skinny rects will sort earlier than square ones, because
		// they are more likely to grow the entire region
		return v.x * v.x + v.y * v.y;
	}
	int Compare( const int & a, const int & b ) const {
		return SizeMetric( (*inputSizes)[b] ) - SizeMetric( (*inputSizes)[a] );
	}
	const anList<anVec2i> *inputSizes;
};

void RectAllocator( const anList<anVec2i> &inputSizes, anList<anVec2i> &outputPositions, anVec2i &totalSize ) {
	outputPositions.SetNum( inputSizes.Num() );
	if ( inputSizes.Num() == 0 ) {
		totalSize.Set( 0, 0 );
		return;
	}

	anList<int> sizeRemap;
	sizeRemap.SetNum( inputSizes.Num() );

	for ( int i = 0; i < inputSizes.Num(); i++ ) {
		sizeRemap[i] = i;
	}

	// Sort the rects from largest to smallest (it makes allocating them in the image better)
	idSortrects sortrectsBySize;
	sortrectsBySize.inputSizes = &inputSizes;
	sizeRemap.SortWithTemplate( sortrectsBySize );

	// the largest rect goes to the top-left corner
	outputPositions[sizeRemap[0]].Set( 0, 0 );

	totalSize = inputSizes[sizeRemap[0]];

	// For each image try to fit it at a corner of one of the already fitted images while
	// minimizing the total area.
	// Somewhat better allocation could be had by checking all the combinations of x and y edges
	// in the allocated rectangles, rather than just the corners of each rectangle, but it
	// still does a pretty good job.
	static const int START_MAX = 1<<14;
	for ( int i = 1; i < inputSizes.Num(); i++ ) {
		anVec2i	best( 0, 0 );
		anVec2i	bestMax( START_MAX, START_MAX );
		anVec2i	size = inputSizes[sizeRemap[i]];
		for ( int j = 0; j < i; j++ ) {
			for ( int k = 1;  k < 4; k++ ) {
				anVec2i	test;
				for ( int n = 0; n < 2; n++ ) {
					test[n] = outputPositions[sizeRemap[j]][n] + ( ( k >> n ) & 1 ) * inputSizes[sizeRemap[j]][n];
				}

				anVec2i	newMax;
				for ( int n = 0; n < 2; n++ ) {
					newMax[n] = Max( totalSize[n], test[n] + size[n] );
				}
				// widths must be multiples of 128 pixels / 32 DXT blocks to
				// allow it to be used directly as a GPU texture without re-packing
				// FIXME: make this a parameter
				newMax[0] = (newMax[0]+31) & ~31;

				// don't let an image get larger than 1024 DXT block, or PS3 crashes
				// FIXME: pass maxSize in as a parameter
				if ( newMax[0] > 1024 || newMax[1] > 1024 ) {
					continue;
				}

				// if we have already found a spot that keeps the image smaller, don't bother checking here
				// This calculation biases the rect towards more square shapes instead of
				// allowing it to extend in one dimension for a long time.
				int	newSize = newMax.x * newMax.x + newMax.y * newMax.y;
				int	bestSize = bestMax.x * bestMax.x + bestMax.y * bestMax.y;
				if ( newSize > bestSize ) {
					continue;
				}

				// if the image isn't required to grow, favor the location closest to the origin
				if ( newSize == bestSize && best.x + best.y < test.x + test.y ) {
					continue;
				}

				// see if this spot overlaps any already allocated rect
				int n = 0;
				for (; n < i; n++ ) {
					const anVec2i &check = outputPositions[sizeRemap[n]];
					const anVec2i &checkSize = inputSizes[sizeRemap[n]];
					if ( test.x + size.x > check.x && test.y + size.y > check.y && test.x < check.x + checkSize.x && test.y < check.y + checkSize.y ) {
						break;
					}
				}
				if ( n < i ) {
					// overlapped, can't use
					continue;
				}
				best = test;
				bestMax = newMax;
			}
		}
		if ( bestMax[0] == START_MAX ) {	// FIXME: return an error code
			anLibrary::FatalError( "RectAllocator: couldn't fit everything" );
		}
		outputPositions[sizeRemap[i]] = best;
		totalSize = bestMax;
	}
}


#endif
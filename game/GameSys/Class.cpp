// Copyright (C) 2007 Id Software, Inc.
//
/*

Base class for all C++ objects.  Provides fast run-time type checking and run-time
instancing of objects.

*/

#include "../Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ARC_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../client/ClientEntity.h"

//#include "TypeInfo.h"
#include "../docs/wiki.h"


/***********************************************************************

  idTypeInfo

***********************************************************************/

// this is the head of a singly linked list of all the idTypes
static idTypeInfo				*typelist;
static idHierarchy<idTypeInfo>	classHierarchy;
static int						eventCallbackMemory	= 0;

/*
================
idTypeInfo::anClassType()

Constructor for class.  Should only be called from CLASS_DECLARATION macro.
Handles linking class definition into class hierarchy.  This should only happen
at startup as idTypeInfos are statically defined.  Since static variables can be
initialized in any order, the constructor must handle the case that subclasses
are initialized before superclasses.
================
*/
idTypeInfo::idTypeInfo( const char *classname, const char *superclass, idEventFunc<anClass> *eventCallbacks, anClass *( *_createInstance )( void ),
	void ( anClass::*Spawn )( void ), bool ( *_inhibitSpawn )( const anDict &args ) ) {

	idTypeInfo *type;
	idTypeInfo **insert;

	this->classname			= classname;
	this->superclass		= superclass;
	this->eventCallbacks	= eventCallbacks;
	this->eventMap			= nullptr;
	this->Spawn				= Spawn;
	this->_createInstance	= _createInstance;
	this->_inhibitSpawn		= _inhibitSpawn;
	this->super				= anClass::GetClass( superclass );
	this->freeEventMap		= false;
	typeNum					= 0;
	lastChild				= 0;

	// Check if any subclasses were initialized before their superclass
	for ( type = typelist; type != nullptr; type = type->next ) {
		if ( ( type->super == nullptr ) && !anStr::Cmp( type->superclass, this->classname ) &&
			anStr::Cmp( type->classname, "anClass" ) ) {
			type->super	= this;
		}
	}

	// Insert sorted
	for ( insert = &typelist; *insert; insert = &(*insert)->next ) {
		assert( anStr::Cmp( classname, (*insert)->classname ) );
		if ( anStr::Cmp( classname, (*insert)->classname ) < 0 ) {
			next = *insert;
			*insert = this;
			break;
		}
	}
	if ( !*insert ) {
		*insert = this;
		next = nullptr;
	}
}

/*
================
idTypeInfo::~idTypeInfo
================
*/
idTypeInfo::~idTypeInfo() {
	Shutdown();
}

/*
================
idTypeInfo::Init

Initializes the event callback table for the class.  Creates a
table for fast lookups of event functions.  Should only be called once.
================
*/
void idTypeInfo::Init( void ) {
	idTypeInfo				*c;
	idEventFunc<anClass>	*def;
	int						ev;
	int						i;
	bool					*set;
	int						num;

	if ( eventMap ) {
		// we've already been initialized by a subclass
		return;
	}

	// make sure our superclass is initialized first
	if ( super && !super->eventMap ) {
		super->Init();
	}

	// add to our node hierarchy
	if ( super ) {
		node.ParentTo( super->node );
	} else {
		node.ParentTo( classHierarchy );
	}
	node.SetOwner( this );

	// keep track of the number of children below each class
	for ( c = super; c != nullptr; c = c->super ) {
		c->lastChild++;
	}

	// if we're not adding any new event callbacks, we can just use our superclass's table
	if ( ( !eventCallbacks || !eventCallbacks->event ) && super ) {
		eventMap = super->eventMap;
		return;
	}

	// set a flag so we know to delete the eventMap table
	freeEventMap = true;

	// Allocate our new table.  It has to have as many entries as there
	// are events.  NOTE: could save some space by keeping track of the maximum
	// event that the class responds to and doing range checking.
	num = anEventDef::NumEventCommands();
	eventMap = new eventCallback_t[ num ];
	memset( eventMap, 0, sizeof( eventCallback_t ) * num );
	eventCallbackMemory += sizeof( eventCallback_t ) * num;

	// allocate temporary memory for flags so that the subclass's event callbacks
	// override the superclass's event callback
	set = new bool[ num ];
	memset( set, 0, sizeof( bool ) * num );

	// go through the inheritence order and copies the event callback function into
	// a list indexed by the event number.  This allows fast lookups of
	// event functions.
	for ( c = this; c != nullptr; c = c->super ) {
		def = c->eventCallbacks;
		if ( !def ) {
			continue;
		}

		// go through each entry until we hit the nullptr terminator
		for ( i = 0; def[i].event != nullptr; i++ )	{
			ev = def[i].event->GetEventNum();

			if ( set[ ev ] ) {
				continue;
			}
			set[ ev ] = true;
			eventMap[ ev ] = def[i].function;
		}
	}

	delete[] set;
}

/*
================
idTypeInfo::Shutdown

Should only be called when DLL or EXE is being shutdown.
Although it cleans up any allocated memory, it doesn't bother to remove itself
from the class list since the program is shutting down.
================
*/
void idTypeInfo::Shutdown() {
	// free up the memory used for event lookups
	if ( eventMap && freeEventMap ) {
		delete[] eventMap;
		eventMap = nullptr;
	}
}


/***********************************************************************

  anClass

***********************************************************************/

const anEventDefInternal EV_Remove( "internal_immediateremove", nullptr );
const anEventDef EV_SafeRemove( "remove", '\0', DOC_TEXT( "Schedules the object for removal at the end of the frame." ), 0, nullptr );
const anEventDef EV_IsClass( "isClass", 'd', DOC_TEXT( "Returns whether the object is of the type specified, including being derived from the type." ), 1, "Type handles can be looked up using $event:getTypeHandle$.", "d", "handle", "Handle to the type to check for." );

ABSTRACT_DECLARATION( nullptr, anClass )
	EVENT( EV_Remove,				anClass::Event_Remove )
	EVENT( EV_SafeRemove,			anClass::Event_SafeRemove )
	EVENT( EV_IsClass,				anClass::Event_IsClass )
END_CLASS

// alphabetical order
anList<idTypeInfo *>	anClass::types;
// typenum order
anList<idTypeInfo *>	anClass::typenums;

anList< anClass* >		anClass::spawningObjects; // objects which are currently spawning


bool	anClass::initialized	= false;
int		anClass::typeNumBits	= 0;
size_t	anClass::memused		= 0;
int		anClass::numobjects		= 0;

/*
================
anClass::CallSpawn
================
*/
void anClass::CallSpawn( void ) {
	spawningObjects.Alloc() = this;

	CallSpawnFunc( GetType() );

	spawningObjects.Remove( this );
}

/*
================
anClass::CallSpawnFunc
================
*/
classSpawnFunc_t anClass::CallSpawnFunc( idTypeInfo *cls ) {
	classSpawnFunc_t func;

	if ( cls->super ) {
		func = CallSpawnFunc( cls->super );
		if ( func == cls->Spawn ) {
			// don't call the same function twice in a row.
			// this can happen when subclasses don't have their own spawn function.
			return func;
		}
	}

	( this->*cls->Spawn )();

	return cls->Spawn;
}

/*
================
anClass::FindUninitializedMemory
================
*/
void anClass::FindUninitializedMemory( void ) {
#if 0 //def ARC_DEBUG_UNINITIALIZED_MEMORY
	unsigned long *ptr = ( ( unsigned long * )this ) - 1;
	int size = *ptr;
	assert( ( size & 3 ) == 0 );
	size >>= 2;
	for ( int i = 0; i < size; i++ ) {
		if ( ptr[i] == 0xcdcdcdcd ) {
			const char *varName = GetTypeVariableName( GetClassname(), i << 2 );
			gameLocal.Warning( "type '%s' has uninitialized variable %s (offset %d)", GetClassname(), varName, i << 2 );
		}
	}
#endif
}

/*
================
anClass::Spawn
================
*/
void anClass::Spawn( void ) {
}

/*
================
anClass::~anClass

Destructor for object.  Cancels any events that depend on this object.
================
*/
anClass::~anClass() {
	idEvent::CancelEvents( this );
}

/*
================
anClass::DisplayInfo_f
================
*/
void anClass::DisplayInfo_f( const anCommandArgs &args ) {
	gameLocal.Printf( "Class memory status: %i bytes allocated in %i objects\n", memused, numobjects );
}

/*
================
anClass::ListClasses_f
================
*/
void anClass::ListClasses_f( const anCommandArgs &args ) {
	int			i;
	idTypeInfo *type;

	gameLocal.Printf( "%-24s %-24s %-6s %-6s\n", "Classname", "Superclass", "Type", "Subclasses" );
	gameLocal.Printf( "----------------------------------------------------------------------\n" );

	for ( i = 0; i < types.Num(); i++ ) {
		type = types[i];
		gameLocal.Printf( "%-24s %-24s %6d %6d\n", type->classname, type->superclass, type->typeNum, type->lastChild - type->typeNum );
	}

	gameLocal.Printf( "...%d classes", types.Num() );
}

/*
================
anClass::WikiClassPage_f
================
*/
void anClass::WikiClassPage_f( const anCommandArgs &args ) {
	const char *typeName = args.Argv( 1 );
	if ( *typeName == '\0' ) {
		for ( int i = 0; i < types.Num(); i++ ) {
			sdWikiFormatter wiki;
			wiki.BuildClassInfo( types[i] );
			wiki.WriteToFile( va( "wiki/classes/%s.txt", types[i]->classname ) );
		}
	} else {
		const idTypeInfo* type = anClass::GetClass( typeName );
		if ( type == nullptr ) {
			gameLocal.Warning( "Unknown class '%s'", typeName );
			return;
		}

		sdWikiFormatter wiki;
		wiki.BuildClassInfo( type );
		wiki.CopyToClipBoard();
	}
}

/*
================
anClass::WikiClassTree_f
================
*/
void anClass::WikiClassTree_f( const anCommandArgs &args ) {
	idTypeInfo* type = nullptr;

	const char *baseTypeName = args.Argv( 1 );
	if ( *baseTypeName != '\0' ) {
		type = anClass::GetClass( baseTypeName );
		if ( type == nullptr ) {
			gameLocal.Warning( "Unknown class '%s'", baseTypeName );
			return;
		}
	} else {
		type = &anClass::Type;
	}

	sdWikiFormatter wiki;
	wiki.BuildClassTree( type );
	wiki.CopyToClipBoard();
}

/*
================
anClass::CreateInstance
================
*/
anClass *anClass::CreateInstance( const char *name ) {
	const idTypeInfo	*type;
	anClass				*obj;

	type = anClass::GetClass( name );
	if ( !type ) {
		return nullptr;
	}

	obj = type->CreateInstance();
	return obj;
}

/*
================
anClass::Init

Should be called after all idTypeInfos are initialized, so must be called
manually upon game code initialization.  Tells all the idTypeInfos to initialize
their event callback table for the associated class.  This should only be called
once during the execution of the program or DLL.
================
*/
void anClass::InitClasses( void ) {
	idTypeInfo	*c;
	int			num;

	gameLocal.Printf( "Initializing class hierarchy\n" );

	if ( initialized ) {
		gameLocal.Printf( "...already initialized\n" );
		return;
	}

	// init the event callback tables for all the classes
	for ( c = typelist; c != nullptr; c = c->next ) {
		c->Init();
	}

	// number the types according to the class hierarchy so we can quickly determine if a class
	// is a subclass of another
	num = 0;
	for ( c = classHierarchy.GetNext(); c != nullptr; c = c->node.GetNext(), num++ ) {
        c->typeNum = num;
		c->lastChild += num;
	}

	// number of bits needed to send types over network
	typeNumBits = anMath::BitsForInteger( num );

	// create a list of the types so we can do quick lookups
	// one list in alphabetical order, one in typenum order
	types.SetGranularity( 1 );
	types.SetNum( num );
	typenums.SetGranularity( 1 );
	typenums.SetNum( num );
	num = 0;
	for ( c = typelist; c != nullptr; c = c->next, num++ ) {
		types[ num ] = c;
		typenums[ c->typeNum ] = c;
	}

	initialized = true;

	gameLocal.Printf( "...%i classes, %i bytes for event callbacks\n", types.Num(), eventCallbackMemory );
}

/*
================
anClass::Shutdown
================
*/
void anClass::ShutdownClasses( void ) {
	idTypeInfo	*c;

	for ( c = typelist; c != nullptr; c = c->next ) {
		c->Shutdown();
	}
	types.Clear();
	typenums.Clear();

	initialized = false;
}

/*
================
anClass::new
================
*/
#ifdef ARC_DEBUG_MEMORY
#undef new
#endif

void * anClass::operator new( size_t s ) {
	size_t *p;

	s += sizeof( size_t );
	p = ( size_t *)Mem_Alloc( s );
	*p = s;
	memused += s;
	numobjects++;

#ifdef ARC_DEBUG_UNINITIALIZED_MEMORY
	unsigned long *ptr = (unsigned long *)p;
	size_t size = s;
	assert( ( size & 3 ) == 0 );
	size >>= 3;
	for ( size_t i = 1; i < size; i++ ) {
		ptr[i] = 0xcdcdcdcd;
	}
#endif

	return p + 1;
}

void * anClass::operator new( size_t s, int, int, char *, int ) {
	size_t *p;

	s += sizeof( size_t );
	p = ( size_t *)Mem_Alloc( s );
	*p = s;
	memused += s;
	numobjects++;

#ifdef ARC_DEBUG_UNINITIALIZED_MEMORY
	unsigned long *ptr = (unsigned long *)p;
	size_t size = s;
	assert( ( size & 3 ) == 0 );
	size >>= 3;
	for ( size_t i = 1; i < size; i++ ) {
		ptr[i] = 0xcdcdcdcd;
	}
#endif

	return p + 1;
}

#ifdef ARC_DEBUG_MEMORY
#define new ID_DEBUG_NEW
#endif

/*
================
anClass::delete
================
*/
void anClass::operator delete( void *ptr ) {
	int *p;

	if ( ptr ) {
		p = ( (int *)ptr ) - 1;
		memused -= *p;
		numobjects--;
        Mem_Free( p );
	}
}

void anClass::operator delete( void *ptr, int, int, char *, int ) {
	int *p;

	if ( ptr ) {
		p = ( (int *)ptr ) - 1;
		memused -= *p;
		numobjects--;
        Mem_Free( p );
	}
}

/*
================
anClass::GetClass

Returns the idTypeInfo for the name of the class passed in.  This is a static function
so it must be called as anClass::GetClass( classname )
================
*/
idTypeInfo *anClass::GetClass( const char *name ) {
	idTypeInfo	*c;
	int			order;
	int			mid;
	int			min;
	int			max;

	if ( !initialized ) {
		// anClass::Init hasn't been called yet, so do a slow lookup
		for ( c = typelist; c != nullptr; c = c->next ) {
			if ( !anStr::Cmp( c->classname, name ) ) {
				return c;
			}
		}
	} else {
		// do a binary search through the list of types
		min = 0;
		max = types.Num() - 1;
		while( min <= max ) {
			mid = ( min + max ) / 2;
			c = types[ mid ];
			order = anStr::Cmp( c->classname, name );
			if ( !order ) {
				return c;
			} else if ( order > 0 ) {
				max = mid - 1;
			} else {
				min = mid + 1;
			}
		}
	}

	return nullptr;
}

/*
================
anClass::GetType
================
*/
idTypeInfo *anClass::GetType( const int typeNum ) {
	idTypeInfo *c;

	if ( !initialized ) {
		for ( c = typelist; c != nullptr; c = c->next ) {
			if ( c->typeNum == typeNum ) {
				return c;
			}
		}
	} else if ( ( typeNum >= 0 ) && ( typeNum < types.Num() ) ) {
		return typenums[ typeNum ];
	}

	return nullptr;
}

/*
================
anClass::GetClassname

Returns the text classname of the object.
================
*/
const char *anClass::GetClassname( void ) const {
	idTypeInfo *type;

	type = GetType();
	return type->classname;
}

/*
================
anClass::ArgCompletion_ClassName
================
*/
void anClass::ArgCompletion_ClassName( const anCommandArgs &args, void(*callback)( const char *s ) ) {
	int i;
	for ( i = 0; i < types.Num(); i++ ) {
		callback( va( "%s %s", args.Argv( 0 ), types[i]->classname ) );
	}
}

/*
================
anClass::GetSuperclass

Returns the text classname of the superclass.
================
*/
const char *anClass::GetSuperclass( void ) const {
	idTypeInfo *cls;

	cls = GetType();
	return cls->superclass;
}

/*
================
anClass::CancelEvents
================
*/
void anClass::CancelEvents( const anEventDef *ev ) {
	idEvent::CancelEvents( this, ev );
}

/*
================
anClass::PostEventArgs
================
*/
bool anClass::PostEventArgs( const anEventDef *ev, int time, int numargs, bool guiEvent, ... ) {
	idTypeInfo	*c;
	idEvent		*event;
	va_list		args;

	assert( ev );

	if ( !idEvent::initialized ) {
		return false;
	}

	c = GetType();
	if ( !c->eventMap[ ev->GetEventNum() ] ) {
		// we don't respond to this event, so ignore it
		return false;
	}

	va_start( args, guiEvent );
	event = idEvent::Alloc( ev, numargs, args, guiEvent );
	va_end( args );

	event->Schedule( this, c, time );

	return true;
}

/*
================
anClass::PostGUIEventMS
================
*/
bool anClass::PostGUIEventMS( const anEventDef *ev, int time ) {
	return PostEventArgs( ev, time, 0, true );
}

/*
================
anClass::PostEventMS
================
*/
bool anClass::PostEventMS( const anEventDef *ev, int time ) {
	if ( gameLocal.isClient ) {
		if ( ev == &EV_SafeRemove ) {
			if ( IsType( anEntity::Type ) ) {
				// Clients aren't allowed to do this.
				assert( false );
				gameLocal.Warning( "Client called anClass::PostEventMS with EV_Remove or EV_SafeRemove!" );
				return false;
			}
		}
	}

	return PostEventArgs( ev, time, 0, false );
}

/*
================
anClass::PostEventMS
================
*/
bool anClass::PostEventMS( const anEventDef *ev, int time, idEventArg arg1 ) {
	return PostEventArgs( ev, time, 1, false, &arg1 );
}

/*
================
anClass::PostEventMS
================
*/
bool anClass::PostEventMS( const anEventDef *ev, int time, idEventArg arg1, idEventArg arg2 ) {
	return PostEventArgs( ev, time, 2, false, &arg1, &arg2 );
}

/*
================
anClass::PostEventMS
================
*/
bool anClass::PostEventMS( const anEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3 ) {
	return PostEventArgs( ev, time, 3, false, &arg1, &arg2, &arg3 );
}

/*
================
anClass::PostEventMS
================
*/
bool anClass::PostEventMS( const anEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4 ) {
	return PostEventArgs( ev, time, 4, false, &arg1, &arg2, &arg3, &arg4 );
}

/*
================
anClass::PostEventMS
================
*/
bool anClass::PostEventMS( const anEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5 ) {
	return PostEventArgs( ev, time, 5, false, &arg1, &arg2, &arg3, &arg4, &arg5 );
}

/*
================
anClass::PostEventMS
================
*/
bool anClass::PostEventMS( const anEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6 ) {
	return PostEventArgs( ev, time, 6, false, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6 );
}

/*
================
anClass::PostEventMS
================
*/
bool anClass::PostEventMS( const anEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7 ) {
	return PostEventArgs( ev, time, 7, false, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7 );
}

/*
================
anClass::PostEventMS
================
*/
bool anClass::PostEventMS( const anEventDef *ev, int time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7, idEventArg arg8 ) {
	return PostEventArgs( ev, time, 8, false, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8 );
}

/*
================
anClass::PostEventSec
================
*/
bool anClass::PostEventSec( const anEventDef *ev, float time ) {
	return PostEventArgs( ev, SEC2MS( time ), 0, false );
}

/*
================
anClass::PostEventSec
================
*/
bool anClass::PostEventSec( const anEventDef *ev, float time, idEventArg arg1 ) {
	return PostEventArgs( ev, SEC2MS( time ), 1, false, &arg1 );
}

/*
================
anClass::PostEventSec
================
*/
bool anClass::PostEventSec( const anEventDef *ev, float time, idEventArg arg1, idEventArg arg2 ) {
	return PostEventArgs( ev, SEC2MS( time ), 2, false, &arg1, &arg2 );
}

/*
================
anClass::PostEventSec
================
*/
bool anClass::PostEventSec( const anEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3 ) {
	return PostEventArgs( ev, SEC2MS( time ), 3, false, &arg1, &arg2, &arg3 );
}

/*
================
anClass::PostEventSec
================
*/
bool anClass::PostEventSec( const anEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4 ) {
	return PostEventArgs( ev, SEC2MS( time ), 4, false, &arg1, &arg2, &arg3, &arg4 );
}

/*
================
anClass::PostEventSec
================
*/
bool anClass::PostEventSec( const anEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5 ) {
	return PostEventArgs( ev, SEC2MS( time ), 5, false, &arg1, &arg2, &arg3, &arg4, &arg5 );
}

/*
================
anClass::PostEventSec
================
*/
bool anClass::PostEventSec( const anEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6 ) {
	return PostEventArgs( ev, SEC2MS( time ), 6, false, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6 );
}

/*
================
anClass::PostEventSec
================
*/
bool anClass::PostEventSec( const anEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7 ) {
	return PostEventArgs( ev, SEC2MS( time ), 7, false, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7 );
}

/*
================
anClass::PostEventSec
================
*/
bool anClass::PostEventSec( const anEventDef *ev, float time, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7, idEventArg arg8 ) {
	return PostEventArgs( ev, SEC2MS( time ), 8, false, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8 );
}

/*
================
anClass::ProcessEventArgs
================
*/
bool anClass::ProcessEventArgs( const anEventDef *ev, int numargs, ... ) {
	idTypeInfo	*c;
	int			num;
	UINT_PTR	data[ D_EVENT_MAXARGS ];
	va_list		args;

	assert( ev );
	assert( idEvent::initialized );

	c = GetType();
	num = ev->GetEventNum();
	if ( !c->eventMap[ num ] ) {
		// we don't respond to this event, so ignore it
		return false;
	}

	va_start( args, numargs );
	idEvent::CopyArgs( ev, numargs, args, data );
	va_end( args );

	ProcessEventArgPtr( ev, data );

	return true;
}

/*
================
anClass::ProcessEvent
================
*/
bool anClass::ProcessEvent( const anEventDef *ev ) {
	return ProcessEventArgs( ev, 0 );
}

/*
================
anClass::ProcessEvent
================
*/
bool anClass::ProcessEvent( const anEventDef *ev, idEventArg arg1 ) {
	return ProcessEventArgs( ev, 1, &arg1 );
}

/*
================
anClass::ProcessEvent
================
*/
bool anClass::ProcessEvent( const anEventDef *ev, idEventArg arg1, idEventArg arg2 ) {
	return ProcessEventArgs( ev, 2, &arg1, &arg2 );
}

/*
================
anClass::ProcessEvent
================
*/
bool anClass::ProcessEvent( const anEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3 ) {
	return ProcessEventArgs( ev, 3, &arg1, &arg2, &arg3 );
}

/*
================
anClass::ProcessEvent
================
*/
bool anClass::ProcessEvent( const anEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4 ) {
	return ProcessEventArgs( ev, 4, &arg1, &arg2, &arg3, &arg4 );
}

/*
================
anClass::ProcessEvent
================
*/
bool anClass::ProcessEvent( const anEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5 ) {
	return ProcessEventArgs( ev, 5, &arg1, &arg2, &arg3, &arg4, &arg5 );
}

/*
================
anClass::ProcessEvent
================
*/
bool anClass::ProcessEvent( const anEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6 ) {
	return ProcessEventArgs( ev, 6, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6 );
}

/*
================
anClass::ProcessEvent
================
*/
bool anClass::ProcessEvent( const anEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7 ) {
	return ProcessEventArgs( ev, 7, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7 );
}

/*
================
anClass::ProcessEvent
================
*/
bool anClass::ProcessEvent( const anEventDef *ev, idEventArg arg1, idEventArg arg2, idEventArg arg3, idEventArg arg4, idEventArg arg5, idEventArg arg6, idEventArg arg7, idEventArg arg8 ) {
	return ProcessEventArgs( ev, 8, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8 );
}

#ifdef GAME_FPU_EXCEPTIONS
extern anCVar g_fpuExceptions;
#endif // GAME_FPU_EXCEPTIONS

/*
================
anClass::ProcessEventArgPtr
================
*/
bool anClass::ProcessEventArgPtr( const anEventDef *ev, const UINT_PTR *data ) {
	idTypeInfo	*c;
	int			num;
	eventCallback_t	callback;

	assert( ev );
	assert( idEvent::initialized );

	c = GetType();
	num = ev->GetEventNum();
	if ( !c->eventMap[ num ] ) {
		// we don't respond to this event, so ignore it
		return false;
	}

	callback = c->eventMap[ num ];

#if !CPU_EASYARGS

/*
on ppc architecture, floats are passed in a seperate set of registers
the function prototypes must have matching float declaration

http://developer.apple.com/documentation/DeveloperTools/Conceptual/MachORuntime/2rt_powerpc_abi/chapter_9_section_5.html
*/

	switch ( ev->GetFormatspecIndex() ) {
	case 1 << D_EVENT_MAXARGS :
		( this->*callback )();
		break;

// generated file - see CREATE_EVENT_CODE
#include "Callbacks.cpp"

	default:
		gameLocal.Warning( "Invalid formatspec on event '%s'", ev->GetName() );
		break;
	}

#else

#ifdef GAME_FPU_EXCEPTIONS
	sys->FPU_EnableExceptions( 0 );
#endif // GAME_FPU_EXCEPTIONS

	assert( D_EVENT_MAXARGS == 8 );

	switch ( ev->GetNumArgs() ) {
	case 0 :
		( this->*callback )();
		break;

	case 1 :
		typedef void ( anClass::*eventCallback_1_t )( const UINT_PTR );
		( this->*( eventCallback_1_t )callback )( data[0] );
		break;

	case 2 :
		typedef void ( anClass::*eventCallback_2_t )( const UINT_PTR, const UINT_PTR );
		( this->*( eventCallback_2_t )callback )( data[0], data[1] );
		break;

	case 3 :
		typedef void ( anClass::*eventCallback_3_t )( const UINT_PTR, const UINT_PTR, const UINT_PTR );
		( this->*( eventCallback_3_t )callback )( data[0], data[1], data[2] );
		break;

	case 4 :
		typedef void ( anClass::*eventCallback_4_t )( const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR );
		( this->*( eventCallback_4_t )callback )( data[0], data[1], data[2], data[3] );
		break;

	case 5 :
		typedef void ( anClass::*eventCallback_5_t )( const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR );
		( this->*( eventCallback_5_t )callback )( data[0], data[1], data[2], data[3], data[ 4 ] );
		break;

	case 6 :
		typedef void ( anClass::*eventCallback_6_t )( const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR );
		( this->*( eventCallback_6_t )callback )( data[0], data[1], data[2], data[3], data[ 4 ], data[ 5 ] );
		break;

	case 7 :
		typedef void ( anClass::*eventCallback_7_t )( const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR );
		( this->*( eventCallback_7_t )callback )( data[0], data[1], data[2], data[3], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 8 :
		typedef void ( anClass::*eventCallback_8_t )( const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR, const UINT_PTR );
		( this->*( eventCallback_8_t )callback )( data[0], data[1], data[2], data[3], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	default:
		gameLocal.Warning( "Invalid formatspec on event '%s'", ev->GetName() );
		break;
	}

#ifdef GAME_FPU_EXCEPTIONS
	if ( g_fpuExceptions.GetBool() ) {
		sys->FPU_EnableExceptions( FPU_EXCEPTION_DENORMALIZED_OPERAND );
	} else {
		sys->FPU_EnableExceptions( 0 );
	}
#endif // GAME_FPU_EXCEPTIONS

#endif

	return true;
}

/*
================
anClass::Event_Remove
================
*/
void anClass::Event_Remove( void ) {
	OnEventRemove();
	delete this;
}

/*
================
anClass::Event_SafeRemove
================
*/
void anClass::Event_SafeRemove( void ) {
	if ( gameLocal.isClient && IsType( anEntity::Type ) ) {
		// Clients aren't allowed to do this.
		assert( false );
		gameLocal.Warning( "Client called anClass::Event_SafeRemove!" );
		return;
	}

	// Forces the remove to be done at a safe time
	PostEventMS( &EV_Remove, 0 );
}

/*
================
anClass::Event_IsClass
================
*/
void anClass::Event_IsClass( int typeNumber ) {
	idTypeInfo* type = anClass::GetType( typeNumber );
	if ( !type ) {
		idProgram::ReturnInteger( 0 );
		return;
	}

	idProgram::ReturnInteger( IsType( *type ) );
}

/*
================
idTypeInfo::CreateInstance
================
*/
anClass* idTypeInfo::CreateInstance( void ) const {
	anClass* instance = _createInstance();
	if ( instance ) {
		anLinkList< anClass >* node = instance->GetInstanceNode();
		if ( node ) {
			node->AddToEnd( instances );
		}
	}
	return instance;
}

/*
================
idTypeInfo::InhibitSpawn
================
*/
bool idTypeInfo::InhibitSpawn( const anDict &args ) const {
	return _inhibitSpawn( args );
}

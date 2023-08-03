#ifndef __SYS_LOCAL__
#define __SYS_LOCAL__

/*
==============================================================

	anSystemLocal

==============================================================
*/

class anSystemLocal : public arcSystem {
public:
	virtual void			DebugPrintf( const char *fmt, ... )an_attribute( ( format( printf,2,3 ) ) );
	virtual void			DebugVPrintf( const char *fmt, va_list arg );

	virtual double			GetClockTicks( void );
	virtual double			ClockTicksPerSecond( void );
	virtual cpuid_t			GetProcessorId( void );
	virtual const char *	GetProcessorString( void );
	virtual const char *	FPU_GetState( void );
	virtual bool			FPU_StackIsEmpty( void );
	virtual void			FPU_SetFTZ( bool enable );
	virtual void			FPU_SetDAZ( bool enable );

	virtual void			FPU_EnableExceptions( int exceptions );

	virtual void			GetCallStack( address_t *callStack, const int callStackSize );
	virtual const char *	GetCallStackStr( const address_t *callStack, const int callStackSize );
	virtual const char *	GetCallStackCurStr( int depth );
	virtual void			ShutdownSymbols( void );

	virtual bool			LockMemory( void *ptr, int bytes );
	virtual bool			UnlockMemory( void *ptr, int bytes );

	virtual int				DLL_Load( const char *dllName );
	virtual void *			DLL_GetProcAddress( int dllHandle, const char *procName );
	virtual void			DLL_Unload( int dllHandle );
	virtual void			DLL_GetFileName( const char *baseName, char *dllName, int maxLength );

	virtual sysEvent_t		GenerateMouseButtonEvent( int button, bool down );
	virtual sysEvent_t		GenerateMouseMoveEvent( int deltax, int deltay );

	virtual void			OpenURL( const char *url, bool quit );
	virtual void			StartProcess( const char *exeName, bool quit );
};

#endif
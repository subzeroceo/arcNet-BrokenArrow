#ifndef __TIMER_H__
#define __TIMER_H__

/*
===============================================================================

	Clock tick counter. Should only be used for profiling.

===============================================================================
*/

class anTimer {
public:
					anTimer( void );
					anTimer( double clockTicks );
					~anTimer( void );

	anTimer			operator+( const anTimer &t ) const;
	anTimer			operator-( const anTimer &t ) const;
	anTimer &		operator+=( const anTimer &t );
	anTimer &		operator-=( const anTimer &t );

	void			Start( void );
	void			Stop( void );
	void			Clear( void );
	double			ClockTicks( void ) const;
	double			Milliseconds( void ) const;

private:
	static double	base;
	enum			{
						TS_STARTED,
						TS_STOPPED
					} state;
	double			start;
	double			clockTicks;

	void			InitBaseClockTicks( void ) const;
};

/*
=================
anTimer::anTimer
=================
*/
inline anTimer::anTimer( void ) {
	state = TS_STOPPED;
	clockTicks = 0.0;
}

/*
=================
anTimer::anTimer
=================
*/
inline anTimer::anTimer( double _clockTicks ) {
	state = TS_STOPPED;
	clockTicks = _clockTicks;
}

/*
=================
anTimer::~anTimer
=================
*/
inline anTimer::~anTimer( void ) {
}

/*
=================
anTimer::operator+
=================
*/
inline anTimer anTimer::operator+( const anTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return anTimer( clockTicks + t.clockTicks );
}

/*
=================
anTimer::operator-
=================
*/
inline anTimer anTimer::operator-( const anTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return anTimer( clockTicks - t.clockTicks );
}

/*
=================
anTimer::operator+=
=================
*/
inline anTimer &anTimer::operator+=( const anTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks += t.clockTicks;
	return *this;
}

/*
=================
anTimer::operator-=
=================
*/
inline anTimer &anTimer::operator-=( const anTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks -= t.clockTicks;
	return *this;
}

/*
=================
anTimer::Start
=================
*/
inline void anTimer::Start( void ) {
	assert( state == TS_STOPPED );
	state = TS_STARTED;
	start = anLibrary::sys->GetClockTicks();
}

/*
=================
anTimer::Stop
=================
*/
inline void anTimer::Stop( void ) {
	assert( state == TS_STARTED );
	clockTicks += anLibrary::sys->GetClockTicks() - start;
	if ( base < 0.0 ) {
		InitBaseClockTicks();
	}
	if ( clockTicks > base ) {
		clockTicks -= base;
	}
	state = TS_STOPPED;
}

/*
=================
anTimer::Clear
=================
*/
inline void anTimer::Clear( void ) {
	clockTicks = 0.0;
}

/*
=================
anTimer::ClockTicks
=================
*/
inline double anTimer::ClockTicks( void ) const {
	assert( state == TS_STOPPED );
	return clockTicks;
}

/*
=================
anTimer::Milliseconds
=================
*/
inline double anTimer::Milliseconds( void ) const {
	assert( state == TS_STOPPED );
	return clockTicks / ( anLibrary::sys->ClockTicksPerSecond() * 0.001 );
}


/*
===============================================================================

	Report of multiple named timers.

===============================================================================
*/

class anTimerReport {
public:
						anTimerReport( void );
						~anTimerReport( void );

	void				SetReportName( const char *name );
	int					AddReport( const char *name );
	void				Clear( void );
	void				Reset( void );
	void				PrintReport( void );
	void				AddTime( const char *name, anTimer *time );

private:
	anList<anTimer *>	timers;
	anStringList		names;
	anStr			reportName;
};

/*
================================================================================

idAutoTimer

idAutoTimer is a simple derivation of idTimer that can start a timer upon
instantiation and stop the timer on destruction, outputting the results.

The TIMER_ macros simplify the use of the timers.  To time the execution of an
entire function, just use TIMER_FUNC(); at the top of the function.

Timers are normally active in debug builds.  To disable timers within a file
in a debug build, define DISABLE_AUTO_TIMERS before including AutoTimer.h.

NOTE: classes derived from idAutoTimer must call idAutoTimer::Shutdown in their
destructor to enable auto timer stopping and output.

================================================================================
*/

#if defined( DEBUG ) && !defined( DISABLE_AUTO_TIMERS )
	#define	ENABLE_AUTO_TIMERS
#endif
#if defined( ENABLE_AUTO_TIMERS )
	#if ( _MSC_VER >= 1300 )
		#define	TIMER_FUNC()	idTimerConsole funcTimer( __FUNCTION__, true )
	#else
		#define	TIMER_FUNC()
	#endif
	#define TIMER_START( _name_ ) idTimerConsole timer##_name_( #_name_, true )
	#define TIMER_START_EX( _name_, _string_ ) idTimerConsole timer##_name_( #_name_, _string_, true )
	#define	TIMER_STOP( _name_ ) timer##_name_.Stop();
	#define TIMER_MS( _name_ ) timer##_name_.Milliseconds()
	#define TIMER_TICKS( _name_ ) timer##_name_.ClockTicks()
	#define	TIMER_OUT( _name_ ) timer##_name_.Stop(); timer##_name_.idAutoTimer::Output();
#else
	#define TIMER_FUNC()
	#define TIMER_START( _name_ )
	#define TIMER_START_EX( _name_ )
	#define	TIMER_STOP( _name_ )
	#define	TIMER_MS( _name_ )	0
	#define TIMER_TICKS( _name_ )	0
	#define TIMER_OUT( _name_ )
#endif

/*
================================================================================

idAutoTimer

idTimer that will automatically stop the timer and results when it goes out of
scope.
================================================================================
*/
class idAutoTimer : public idTimer {
public:
	const char *		name;
	const char *		extraData;

public:
	idAutoTimer( const char *name, const char *extraData, bool start = false )
	:	name( name ),
		extraData( extraData ) {
		if ( start ) {
			Clear();
			Start();
		}
	}
	idAutoTimer( const char *name, bool start = false )
	:	name( name ),
		extraData( nullptr ) {
		if ( start ) {
			Clear();
			Start();
		}
	}
	~idAutoTimer( void ) {
		// don't call shutdown here... virtual function table will be hosed already.
		// call it in the derived class's destructor.
	}

	// derived classes should call from destructor to output timer info
	void	Shutdown( void ) {
		// if timer was started and hasn't been stopped, stop on destruction and output
		if ( State() != idTimer::TS_STOPPED ) {
			Stop();
			Output();
		}
		name = nullptr;
		extraData = nullptr;
	}

	void	Output( void ) const {
		assert( State() == idTimer::TS_STOPPED );
		if ( extraData != nullptr ) {
			Output( name, extraData );
		} else {
			Output( name );
		}
	}

	void	OutputMsg( const char *message ) const {
		assert( State() == idTimer::TS_STOPPED );
		if ( extraData != nullptr ) {
			OutputMsg( message, name, extraData );
		} else {
			OutputMsg( message, name );
		}
	}

protected:
	// output a standard message
	virtual	void	Output( const char *name ) const = 0;
	// output a standard message with some extra data
	virtual	void	Output( const char *name, const char *extraData ) const = 0;

	// output a message formatted with %s (name) and %f (time)
	virtual	void	OutputMsg( const char *message, const char *name ) const = 0;

	// output a message formatted with %s (name), %s (extraData) and %f (time)
	virtual void	OutputMsg( const char *message, const char *name, const char *extraData ) const = 0;
};

/*
================================================================================

idTimerConsole

Timer that outputs results to the console
================================================================================
*/
class idTimerConsole : public idAutoTimer {
public:
	idTimerConsole( const char *name, bool start = false )
		:	idAutoTimer( name, start ) {
	}
	idTimerConsole( const char *name, const char *extraData, bool start = false )
		:	idAutoTimer( name, extraData, start ) {
	}

	virtual ~idTimerConsole() {
		idAutoTimer::Shutdown();
	}

protected:
	virtual	void	Output( const char *name ) const {
		common->Printf( "Timer \"%s\" took %.2f ms.\n", name, Milliseconds() );
	}

	virtual	void	Output( const char *name, const char *extraData ) const {
		common->Printf( "Timer \"%s\" ( %s ) took %.2f ms.\n", name, extraData, Milliseconds() );
	}

	virtual	void	OutputMsg( const char *message, const char *name ) const {
		common->Printf( message, name, Milliseconds() );
	}

	virtual void	OutputMsg( const char *message, const char *name, const char *extraData ) const {
		common->Printf( message, name, extraData, Milliseconds() );
	}
};

#endif // !__TIMER_H__

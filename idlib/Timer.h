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
ARC_INLINE anTimer::anTimer( void ) {
	state = TS_STOPPED;
	clockTicks = 0.0;
}

/*
=================
anTimer::anTimer
=================
*/
ARC_INLINE anTimer::anTimer( double _clockTicks ) {
	state = TS_STOPPED;
	clockTicks = _clockTicks;
}

/*
=================
anTimer::~anTimer
=================
*/
ARC_INLINE anTimer::~anTimer( void ) {
}

/*
=================
anTimer::operator+
=================
*/
ARC_INLINE anTimer anTimer::operator+( const anTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return anTimer( clockTicks + t.clockTicks );
}

/*
=================
anTimer::operator-
=================
*/
ARC_INLINE anTimer anTimer::operator-( const anTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return anTimer( clockTicks - t.clockTicks );
}

/*
=================
anTimer::operator+=
=================
*/
ARC_INLINE anTimer &anTimer::operator+=( const anTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks += t.clockTicks;
	return *this;
}

/*
=================
anTimer::operator-=
=================
*/
ARC_INLINE anTimer &anTimer::operator-=( const anTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks -= t.clockTicks;
	return *this;
}

/*
=================
anTimer::Start
=================
*/
ARC_INLINE void anTimer::Start( void ) {
	assert( state == TS_STOPPED );
	state = TS_STARTED;
	start = anLibrary::sys->GetClockTicks();
}

/*
=================
anTimer::Stop
=================
*/
ARC_INLINE void anTimer::Stop( void ) {
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
ARC_INLINE void anTimer::Clear( void ) {
	clockTicks = 0.0;
}

/*
=================
anTimer::ClockTicks
=================
*/
ARC_INLINE double anTimer::ClockTicks( void ) const {
	assert( state == TS_STOPPED );
	return clockTicks;
}

/*
=================
anTimer::Milliseconds
=================
*/
ARC_INLINE double anTimer::Milliseconds( void ) const {
	assert( state == TS_STOPPED );
	return clockTicks / ( anLibrary::sys->ClockTicksPerSecond() * 0.001 );
}


/*
===============================================================================

	Report of multiple named timers.

===============================================================================
*/

class arcTimerReport {
public:
					arcTimerReport( void );
					~arcTimerReport( void );

	void			SetReportName( const char *name );
	int				AddReport( const char *name );
	void			Clear( void );
	void			Reset( void );
	void			PrintReport( void );
	void			AddTime( const char *name, anTimer *time );

private:
	anList<anTimer*>timers;
	anStringList		names;
	anString			reportName;
};

#endif /* !__TIMER_H__ */

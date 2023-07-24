#ifndef __TIMER_H__
#define __TIMER_H__

/*
===============================================================================

	Clock tick counter. Should only be used for profiling.

===============================================================================
*/

class ARCTimer {
public:
					ARCTimer( void );
					ARCTimer( double clockTicks );
					~ARCTimer( void );

	ARCTimer			operator+( const ARCTimer &t ) const;
	ARCTimer			operator-( const ARCTimer &t ) const;
	ARCTimer &		operator+=( const ARCTimer &t );
	ARCTimer &		operator-=( const ARCTimer &t );

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
ARCTimer::ARCTimer
=================
*/
ARC_INLINE ARCTimer::ARCTimer( void ) {
	state = TS_STOPPED;
	clockTicks = 0.0;
}

/*
=================
ARCTimer::ARCTimer
=================
*/
ARC_INLINE ARCTimer::ARCTimer( double _clockTicks ) {
	state = TS_STOPPED;
	clockTicks = _clockTicks;
}

/*
=================
ARCTimer::~ARCTimer
=================
*/
ARC_INLINE ARCTimer::~ARCTimer( void ) {
}

/*
=================
ARCTimer::operator+
=================
*/
ARC_INLINE ARCTimer ARCTimer::operator+( const ARCTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return ARCTimer( clockTicks + t.clockTicks );
}

/*
=================
ARCTimer::operator-
=================
*/
ARC_INLINE ARCTimer ARCTimer::operator-( const ARCTimer &t ) const {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	return ARCTimer( clockTicks - t.clockTicks );
}

/*
=================
ARCTimer::operator+=
=================
*/
ARC_INLINE ARCTimer &ARCTimer::operator+=( const ARCTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks += t.clockTicks;
	return *this;
}

/*
=================
ARCTimer::operator-=
=================
*/
ARC_INLINE ARCTimer &ARCTimer::operator-=( const ARCTimer &t ) {
	assert( state == TS_STOPPED && t.state == TS_STOPPED );
	clockTicks -= t.clockTicks;
	return *this;
}

/*
=================
ARCTimer::Start
=================
*/
ARC_INLINE void ARCTimer::Start( void ) {
	assert( state == TS_STOPPED );
	state = TS_STARTED;
	start = arcLibrary::sys->GetClockTicks();
}

/*
=================
ARCTimer::Stop
=================
*/
ARC_INLINE void ARCTimer::Stop( void ) {
	assert( state == TS_STARTED );
	clockTicks += arcLibrary::sys->GetClockTicks() - start;
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
ARCTimer::Clear
=================
*/
ARC_INLINE void ARCTimer::Clear( void ) {
	clockTicks = 0.0;
}

/*
=================
ARCTimer::ClockTicks
=================
*/
ARC_INLINE double ARCTimer::ClockTicks( void ) const {
	assert( state == TS_STOPPED );
	return clockTicks;
}

/*
=================
ARCTimer::Milliseconds
=================
*/
ARC_INLINE double ARCTimer::Milliseconds( void ) const {
	assert( state == TS_STOPPED );
	return clockTicks / ( arcLibrary::sys->ClockTicksPerSecond() * 0.001 );
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
	void			AddTime( const char *name, ARCTimer *time );

private:
	arcNetList<ARCTimer*>timers;
	arcStringList		names;
	arcNetString			reportName;
};

#endif /* !__TIMER_H__ */

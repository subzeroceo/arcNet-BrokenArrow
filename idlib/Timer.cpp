#include "precompiled.h"
#pragma hdrstop

double ARCTimer::base = -1.0;

/*
=================
ARCTimer::InitBaseClockTicks
=================
*/
void ARCTimer::InitBaseClockTicks( void ) const {
	ARCTimer timer;
	double ct, b;
	int i;

	base = 0.0;
	b = -1.0;
	for ( i = 0; i < 1000; i++ ) {
		timer.Clear();
		timer.Start();
		timer.Stop();
		ct = timer.ClockTicks();
		if ( b < 0.0 || ct < b ) {
			b = ct;
		}
	}
	base = b;
}

/*
=================
arcTimerReport::arcTimerReport
=================
*/
arcTimerReport::arcTimerReport() {
}

/*
=================
arcTimerReport::SetReportName
=================
*/
void arcTimerReport::SetReportName( const char *name ) {
	reportName = ( name ) ? name : "Timer Report";
}

/*
=================
arcTimerReport::~arcTimerReport
=================
*/
arcTimerReport::~arcTimerReport() {
	Clear();
}

/*
=================
arcTimerReport::AddReport
=================
*/
int arcTimerReport::AddReport( const char *name ) {
	if ( name && *name ) {
		names.Append( name );
		return timers.Append( new ARCTimer() );
	}
	return -1;
}

/*
=================
arcTimerReport::Clear
=================
*/
void arcTimerReport::Clear() {
	timers.DeleteContents( true );
	names.Clear();
	reportName.Clear();
}

/*
=================
arcTimerReport::Reset
=================
*/
void arcTimerReport::Reset() {
	assert ( timers.Num() == names.Num() );
	for ( int i = 0; i < timers.Num(); i++ ) {
		timers[i]->Clear();
	}
}

/*
=================
arcTimerReport::AddTime
=================
*/
void arcTimerReport::AddTime( const char *name, ARCTimer *time ) {
	assert ( timers.Num() == names.Num() );
	int i;
	for ( i = 0; i < names.Num(); i++ ) {
		if ( names[i].Icmp( name ) == 0 ) {
			*timers[i] += *time;
			break;
		}
	}
	if ( i == names.Num() ) {
		int index = AddReport( name );
		if ( index >= 0 ) {
			timers[index]->Clear();
			*timers[index] += *time;
		}
	}
}

/*
=================
arcTimerReport::PrintReport
=================
*/
void arcTimerReport::PrintReport() {
	assert( timers.Num() == names.Num() );
	anLibrary::common->Printf( "Timing Report for %s\n", reportName.c_str() );
	anLibrary::common->Printf( "-------------------------------\n" );
	float total = 0.0f;
	for ( int i = 0; i < names.Num(); i++ ) {
		anLibrary::common->Printf( "%s consumed %5.2f seconds\n", names[i].c_str(), timers[i]->Milliseconds() * 0.001f );
		total += timers[i]->Milliseconds();
	}
	anLibrary::common->Printf( "Total time for report %s was %5.2f\n\n", reportName.c_str(), total * 0.001f );
}

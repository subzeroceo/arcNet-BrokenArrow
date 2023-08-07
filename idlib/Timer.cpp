#include "Lib.h"
#pragma hdrstop

double anTimer::base = -1.0;

/*
=================
anTimer::InitBaseClockTicks
=================
*/
void anTimer::InitBaseClockTicks( void ) const {
	anTimer timer;
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
anTimerReport::anTimerReport
=================
*/
anTimerReport::anTimerReport() {
}

/*
=================
anTimerReport::SetReportName
=================
*/
void anTimerReport::SetReportName( const char *name ) {
	reportName = ( name ) ? name : "Timer Report";
}

/*
=================
anTimerReport::~anTimerReport
=================
*/
anTimerReport::~anTimerReport() {
	Clear();
}

/*
=================
anTimerReport::AddReport
=================
*/
int anTimerReport::AddReport( const char *name ) {
	if ( name && *name ) {
		names.Append( name );
		return timers.Append( new anTimer() );
	}
	return -1;
}

/*
=================
anTimerReport::Clear
=================
*/
void anTimerReport::Clear() {
	timers.DeleteContents( true );
	names.Clear();
	reportName.Clear();
}

/*
=================
anTimerReport::Reset
=================
*/
void anTimerReport::Reset() {
	assert ( timers.Num() == names.Num() );
	for ( int i = 0; i < timers.Num(); i++ ) {
		timers[i]->Clear();
	}
}

/*
=================
anTimerReport::AddTime
=================
*/
void anTimerReport::AddTime( const char *name, anTimer *time ) {
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
anTimerReport::PrintReport
=================
*/
void anTimerReport::PrintReport() {
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

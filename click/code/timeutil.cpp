/****************************************************************************
     Copyright (c) 1993 1994 1995
     By Miron Livny, Madison, Wisconsin
     All Rights Reserved.

     UNDER NO CIRCUMSTANCES IS THIS PROGRAM TO BE COPIED OR DISTRIBUTED
     WITHOUT PERMISSION OF MIRON LIVNY

     modified 11/3/95 by Tian Zhang for universal clock ticking 
****************************************************************************/
#include <time.h>
#include <sys/times.h>
#include "timeutil.h"
#include <cstring>

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

extern "C" {
	clock_t times( struct tms * );	// for hpux for example
	char *ctime( const time_t * );  // for sunos for example
}

Timer::
Timer( )
{
	name = 0;
	this->Clear();
	clock_tick = sysconf(_SC_CLK_TCK);
}

Timer::
Timer( const char *nm )
{
	name = new char [ strlen(nm) + 1 ];
	strcpy( name, nm );
	this->Clear();
	clock_tick = sysconf(_SC_CLK_TCK);
}

Timer::
Timer( const Timer& t )
{
	if( t.name ) {
		name = new char [ strlen(t.name) + 1 ];
		strcpy( name, t.name );
	} else {
		name = 0;
	}
	is_running = t.is_running;
	clock_tick = t.clock_tick;
	if( is_running ) {
		system_started = t.system_started;
		user_started = t.user_started;
		wall_clock_started = t.wall_clock_started;
	}
	system_accum = t.system_accum;
	user_accum = t.user_accum;
	wall_clock_accum = t.wall_clock_accum;
}

Timer& Timer::
operator += (const Timer& t )
{
	assert( t.is_running == FALSE );
	assert( is_running == FALSE );

	system_accum += t.system_accum;
	user_accum += t.user_accum;
	wall_clock_accum += t.wall_clock_accum;
	return *this;
}

Timer
Timer:: operator + ( const Timer & t ) const
{
	Timer result = *this;

	result += t;
	return result;
}

Timer& Timer::
operator -= (const Timer& t )
{
	assert( t.is_running == FALSE );
	assert( is_running == FALSE );

	system_accum -= t.system_accum;
	user_accum -= t.user_accum;
	wall_clock_accum -= t.wall_clock_accum;
	return *this;
}

Timer
Timer:: operator - ( const Timer & t ) const
{
	Timer result = *this;

	result -= t;
	return result;
}

Timer::
~Timer()
{
	if( name ) {
		delete [] name;
	}
}

inline void Timer::
Clear()
{
	is_running = FALSE;
	system_accum = 0;
	user_accum = 0;
	wall_clock_accum = 0;
}


void Timer::
Start()
{
	struct tms	buf;

	assert( is_running == FALSE );

	wall_clock_started = times( &buf );
	user_started = buf.tms_utime;
	system_started = buf.tms_stime;
	is_running = TRUE;
}

void Timer::
Stop()
{
	struct tms buf;
	clock_t	wall_clock_stopped;

	assert( is_running == TRUE );

	wall_clock_stopped = times( &buf );
	system_accum += buf.tms_stime - system_started;
	user_accum += buf.tms_utime - user_started;
	wall_clock_accum += wall_clock_stopped - wall_clock_started;
	is_running = FALSE;
}

double Timer::
SystemTime()
{
	return (double)system_accum / clock_tick;
}

double Timer::
UserTime()
{
	return (double)user_accum / clock_tick;
}

double Timer::
WallClockTime()
{
	return (double)wall_clock_accum / clock_tick;
}

void Timer::
Display()
{
	if( name ) { cout << name << ":" ;}
	this->simple_display();
}

ostream&
operator << ( ostream& os, Timer& t )
{
	os << t.name << " Sys: " << t.SystemTime();
	os << " User: " << t.UserTime();
	os << " Total: " << t.SystemTime()+t.UserTime();
	os << " Wall: " << t.WallClockTime() << endl;
	return os;
}

ofstream&
operator << ( ofstream& os, Timer& t )
{
	os << t.name << " Sys: " << t.SystemTime();
	os << " User: " << t.UserTime();
	os << " Total: " << t.SystemTime()+t.UserTime();
	os << " Wall: " << t.WallClockTime() << endl;
	return os;
}

void Timer::
Display( const char *msg )
{
	cout << msg << ":" << endl;
	this->simple_display();
}

void Timer::
simple_display()
{
	cout << this->UserTime() + this->SystemTime() ;
	cout << " ( " << this->UserTime() ;
	cout << "u " << this->SystemTime() << "s )";
}

Counter::
Counter( const char *nm )
{
	name = new char [ strlen(nm) + 1 ];
	strcpy( name, nm );
	val = 0;
}

Counter::
Counter( const Counter& c )
{
	if( c.name ) {
		name = new char [ strlen(c.name) + 1 ];
		strcpy( name, c.name );
	} else {
		name = 0;
	}
	val = c.val;
}

Counter::
~Counter()
{
	if( name ) {
		delete [] name;
	}
}

Counter& Counter::
operator += (const Counter& c )
{
	val += c.val;
	return *this;
}

Counter
Counter:: operator + ( const Counter & t ) const
{
	Counter result = *this;

	result.val += t.val;
	return result;
}

Counter& Counter::
operator -= (const Counter& c )
{
	val -= c.val;
	return *this;
}

Counter
Counter:: operator - ( const Counter & t ) const
{
	Counter result = *this;

	result.val += t.val;
	return result;
}

void Counter::
Display( const char *msg )
{
	cout << msg << ": " << val << endl;
}

void Counter::
Display()
{
	if( name ) {
		cout << name << ":" << val << endl;
	} else {
		cout << "(Unnamed Counter):" << val << endl;
	}
}

void TimeStamp::
Set()
{
	val = time( 0 );
}

void TimeStamp::
Display()
{
	cout << ctime( &val );
}

ostream&
operator << ( ostream& os, TimeStamp& ts )
{
	char	*ptr;

	for( ptr=ctime( &ts.val ); *ptr != '\n'; ptr++ ) {
		os << *ptr;
	}
	return os;
}

ostream&
operator << ( ostream& os, Counter& c )
{
	os << c.val;
	return os;
}


ofstream& Timer::Output(ofstream& of)
{
  of << SystemTime() << "\t" << UserTime() << "\t" << SystemTime()+UserTime() << "\t" << WallClockTime() << endl;
  return of;
}

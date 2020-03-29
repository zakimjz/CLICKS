/****************************************************************************
     Copyright (c) 1993 1994 1995
     By Miron Livny, Madison, Wisconsin
     All Rights Reserved.

     UNDER NO CIRCUMSTANCES IS THIS PROGRAM TO BE COPIED OR DISTRIBUTED
     WITHOUT PERMISSION OF MIRON LIVNY

     modified 11/3/95 by Tian Zhang for universal clock ticking
****************************************************************************/

#ifndef TIMEUTIL_H
#define TIMEUTIL_H

/******************************************************************************

	                    timeutil.h

******************************************************************************/
#include <iostream>
#include <fstream>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>

using namespace std;
/*********************************************************************
Maintain timers for performance monitoring.  Three versions of "time"
are maintained, "system", "user", and "wall clock".  POSIX.1 compliant
routines are used throughout for portability.
*********************************************************************/

class Timer {
friend ostream& operator <<( ostream&, Timer& );
friend ofstream& operator <<( ofstream&, Timer& );
public:
		// Constructors and destructors.  Names are optional, and
		// if provided will be used by the default display routine.
	Timer();
	Timer( const char *nm );
	Timer( const Timer& t );
	~Timer();

		// Stopwatch "buttons"
	void Start();
	void Stop();
	void Clear();

		// Binary arithmetic operators.  Note: Operations on
		// running timers are prohibited - an assertion violation
		// will result.
	Timer & operator += ( const Timer& t );
	Timer & operator -= ( const Timer& t );
	Timer operator + (const Timer & t ) const;
	Timer operator - (const Timer & t ) const;

		// Display with an optional message.  If set, the name will
		// be used in place of a message for the default version.
	void Display();
	void Display( const char *msg );

		// Get individual member values.
	double SystemTime();
	double UserTime();
	double WallClockTime();

  ofstream& Output(ofstream& of);
private:
	void simple_display();
	char		*name;
	int		is_running;
	int 		clock_tick;
	clock_t		system_started;
	clock_t		user_started;
	clock_t		wall_clock_started;
	clock_t		system_accum;
	clock_t		user_accum;
	clock_t		wall_clock_accum;
};

ostream& operator<<(ostream&, Timer& );

/*********************************************************************
Maintain simple counters for performance monitoring.
*********************************************************************/

class Counter {
friend ostream& operator <<( ostream&, Counter& );
public:
		// Constructors and destructors.  Names are optional, and
		// if provided will be used by the default display routine.
	Counter() { val = 0; }
	Counter( const char *nm );
	Counter( const Counter& c );
	~Counter();

		// Basic operations on counters.
	void Clear() { val = 0; }
	void Inc() { val += 1; }

		// Binary arithmetic operators.
	Counter & operator += ( const Counter& c );
	Counter & operator -= ( const Counter& c );
	Counter operator + (const Counter & c ) const;
	Counter operator - (const Counter & c ) const;

		// Display with an optional message.  If set, the name will
		// be used in place of a message for the default version.
	void Display();
	void Display( const char *msg );

		// Get current value.
	long Ticks() { return val; }
private:
	long		val;
	char		*name;
};

ostream& operator<<(ostream&, Counter& );

/*****************************************************************
Maintain a "time stamp", (current date and time).  The time stamp
is initialized by its constructor, and can be reset at any time
using the "Set" function.  The insertion operator "<<" is
overloaded so that you can insert a time stamp directly
into an output stream, i.e.
	cout << some_time_stamp << endl;
******************************************************************/
class TimeStamp {
friend ostream& operator <<( ostream&, TimeStamp& );
public:
	TimeStamp() { val = time(0); }
	void Set();
	void Display();

private:
	time_t	val;
};
ostream& operator<<(ostream&, TimeStamp& );

#endif


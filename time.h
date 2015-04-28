//
// You received this file as part of RRLib
// Robotics Research Library
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//----------------------------------------------------------------------
/*!\file    rrlib/time/time.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-05-23
 *
 * \brief   Contains typedefs for time points and duration - as well as a few functions.
 *
 * \b tTimePoint
 */
//----------------------------------------------------------------------
#ifndef __rrlib__time__time_h__
#define __rrlib__time__time_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <chrono>
#include <mutex>

#include "rrlib/design_patterns/singleton.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace rrlib
{
namespace time
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------
#if __linux__ || __CYGWIN__
#define RRLIB_TIME_PARSING_AVAILABLE
#endif


/*!
 * Clock that "application time" is derived from
 */
typedef std::chrono::high_resolution_clock tBaseClock;

//! Time stamp type
/*!
 * std::chrono::time_point type returned by clocks.
 * It should be used in most parts of applications (e.g. Timestamps, Finroc ports).
 */
typedef tBaseClock::time_point tTimestamp;

//! Duration type
/*!
 * Duration type matching tTimestamp type.
 * It should be used in most parts of applications.
 */
typedef tBaseClock::duration tDuration;

/*!
 * Special time stamp to indicate "no time" or "not set" or "never" (e.g. for a deadline)
 */
extern tTimestamp cNO_TIME;

/*!
 * Possible modes how "application time" is determined
 */
enum class tTimeMode
{
  SYSTEM_TIME,            //!< "application time" is identical to system time
  STRETCHED_SYSTEM_TIME,  //!< "application time" is system time with some sort of time-stretching applied
  CUSTOM_CLOCK            //!< "application time" is set by an external entity ("custom clock")
};

class tCustomClock;

/*!
 * \return Returns current mode regarding how "application time" is determined
 */
tTimeMode GetTimeMode();

/*!
 * Returns "application time".
 * By default this is system time.
 * It can, however, also be simulated time (time stretching when simulating etc.).
 * In order for time stretching to work in whole applications, libraries and application components
 * that do not explicitly require system time should obtain time from this function.
 *
 * Note, that obtaining precise high resolution system time can be a rather expensive system call.
 * (a benchmark that did only this could merely obtain around 150K time stamps per second (a few years ago on a Core Duo Notebook processor)).
 * This can be implemented in a much less expensive way, if less precision is required.
 * Therefore, if high precision is not required, precise should be set to false when frequently calling this method.
 *
 * \param precise If true, the high resolution system clock is used.
 *                If false, the time stamp is less precise (+- 25ms).
 */
tTimestamp Now(bool precise = true);

/*!
 * Sets specified non-linear clock as active time source for "application time".
 * Time mode is set to CUSTOM_CLOCK.
 *
 * \param clock Clock to use as time source. NULL to switch back to STRETCHED_SYSTEM_TIME.
 * \param initial_time Initial time
 */
void SetTimeSource(const tCustomClock* clock, const tTimestamp& initial_time);

/*!
 * Changes time stretching factor for "application time".
 * May be called frequently while application is executing.
 * Time mode is set to STRETCHED_SYSTEM_TIME.
 *
 * "Application time" progresses 'numerator/denominator' times as fast as system time.
 * A factor smaller than one, for instance, means that the application will run in 'slow motion'.
 *
 * \param numerator Time stretching factor numerator (max. 1 million)
 * \param denominator Time stretching factor denominator (max. 1 million)
 */
void SetTimeStretching(unsigned int numerator, unsigned int denominator);

/*!
 * Sometimes (e.g. when calling wait and sleep functions),
 * durations calculated from "application time" need to be converted to system time.
 * (Note that conversion is not really possible if external non-linear clock is used.
 *  In this case, app_duration is merely returned)
 *
 * \param app_duration Duration in "application time"
 * \return Duration in system time
 */
tDuration ToSystemDuration(const tDuration& app_duration);

#ifdef RRLIB_TIME_PARSING_AVAILABLE
/*!
 * Parses timestamp in ISO 8601 string representation
 *
 * \param s Timestamp as ISO 8601 string
 * \return Timestamp
 */
tTimestamp ParseIsoTimestamp(const std::string& s);
#endif


#ifdef RRLIB_TIME_PARSING_AVAILABLE
/*!
 * Parses GPS timestamp in NMEA-0183 GPRMC representation
 *
 * \param nmea_time GPS time according to NMEA-0183 GPRMC representation (HHMMSS{.SSS})
 * \param nmea_date GPS date according to NMEA-0183 GPRMC representation (DDMMYY)
 * \return Timestamp
 */
tTimestamp ParseNmeaTimestamp(const std::string& nmea_time, const std::string& nmea_date);
#endif

/*!
 * Turns Timestamp into string representation following ISO 8601 (or W3C XML Schema 1.0 specification)
 *
 * \param timestamp Timestamp to convert
 * \return ISO 8601 representation of timestamp
 */
std::string ToIsoString(const tTimestamp& timestamp);

#ifdef RRLIB_TIME_PARSING_AVAILABLE
/*!
 * Parses duration in ISO 8601 string representation
 * (throws exception if string cannot be parsed)
 *
 * \param s Duration as ISO 8601 string
 * \return Duration
 */
tDuration ParseIsoDuration(const std::string& s);
#endif

/*!
 * Turns Duration into string representation following ISO 8601 (or W3C XML Schema 1.0 specification)
 *
 * \param duration Duration to convert
 * \return ISO 8601 representation of duration
 */
std::string ToIsoString(const tDuration& duration);

/*!
 * Turns duration into a simple string (number + unit)
 *
 * \param duration Duration to convert
 * \return Simple string representation of duration
 */
std::string ToString(std::chrono::nanoseconds ns);

#if __linux__
/*!
 * Extracts the last full hour from a given timestamp
 *
 * \param timestamp Timestamp to convert
 * \return timestamp of the last full hour
 */
tTimestamp GetLastFullHour(const tTimestamp& timestamp);
#endif

namespace internal
{

/*!
 * Internal mutex that is used throughout rrlib_time library
 */
class tTimeMutexImplementation : public std::mutex {};
typedef rrlib::design_patterns::tSingletonHolder<tTimeMutexImplementation> tTimeMutex;

}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif

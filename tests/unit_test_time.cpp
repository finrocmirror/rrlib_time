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
/*!\file    rrlib/time/tests/unit_test_time.cpp
 *
 * \author  Patrick Fleischmann
 * \author  Max Reichardt
 *
 * \date    2014-04-04
 *
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/util/tUnitTestSuite.h"

#include "rrlib/time/time.h"

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
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

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------
class TestTime : public util::tUnitTestSuite
{
  RRLIB_UNIT_TESTS_BEGIN_SUITE(TestTime);
  RRLIB_UNIT_TESTS_ADD_TEST(Test);
  RRLIB_UNIT_TESTS_END_SUITE;

private:

  virtual void Test()
  {
    std::cout << " (Now is: " << ToIsoString(std::chrono::system_clock::now()) << ") ";

    // ParseIsoTimestamp and ToIsoString tests
    std::string iso_timestamp = "2014-04-04T14:14:14.141414141+02:00";
    tTimestamp iso_timestamp_from_string = ParseIsoTimestamp(iso_timestamp);

//    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("iso_timestamp should be 2014-04-04T14:14:14.141414141+02:00", iso_timestamp, ToIsoString(iso_timestamp_from_string));

    std::string iso_timestamp_short = "2014-04-04T14:14:14.141414+02:00";
    tTimestamp iso_timestamp_short_from_string = ParseIsoTimestamp(iso_timestamp_short);
//    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("iso_timestamp should be 2014-04-04T14:14:14.141414+02:00", iso_timestamp_short, ToIsoString(iso_timestamp_short_from_string));
//
    std::string iso_timestamp_even_shorter = "2014-04-04T14:14:14+02:00";
    tTimestamp iso_timestamp_even_shorter_from_string = ParseIsoTimestamp(iso_timestamp_even_shorter);
//    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("iso_timestamp should be 2014-04-04T14:14:14+02:00", iso_timestamp_even_shorter, ToIsoString(iso_timestamp_even_shorter_from_string));
//
//    std::string iso_timestamp_no_timezone = "2011-11-11T11:11:11";
//    tTimestamp iso_timestamp_no_timezone_from_string = ParseIsoTimestamp(iso_timestamp_no_timezone);
//    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("iso_timestamp_no_timezone should be 2011-11-11T11:11:11", iso_timestamp_no_timezone, ToIsoString(iso_timestamp_no_timezone_from_string));
//
//    const char* char_timestamp = "2012-06-16T15:20:26.12345+03:00";
//    tTimestamp from_char_timestamp = ParseIsoTimestamp(char_timestamp);
//    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("char_timestamp should be 2012-06-16T15:20:26.12345+03:00", char_timestamp, ToIsoString(from_char_timestamp).c_str());


    // now tests
    tTimestamp now = Now();
    std::string now_string = ToIsoString(now);
    tTimestamp now_from_string = ParseIsoTimestamp(now_string);
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("timestamp should be equal to ParseIsoTimestamp(ToIsoString(timestamp))", ToIsoString(now), ToIsoString(now_from_string));

    // duration tests
    tDuration dur = std::chrono::seconds(3235) + std::chrono::milliseconds(25);
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be PT3235.25S", ToIsoString(dur), std::string("PT53M55.025S"));

    dur = std::chrono::seconds(43) + std::chrono::microseconds(123400);
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be PT43.123400S", std::string("PT43.123400S"), ToIsoString(dur));

    dur = std::chrono::seconds(43) + std::chrono::microseconds(123400);

    dur = std::chrono::hours(24 * 400);
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be P1Y35D", std::string("P1Y35D"), ToIsoString(dur));

    std::string s = "P400D";
    dur = ParseIsoDuration(s);
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be P1Y35D", std::string("P1Y35D"), ToIsoString(dur));

    s = "PT43.1234S";
    dur = ParseIsoDuration(s);
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be PT43.1234S", std::string("PT43.123400S") , ToIsoString(dur));

    s = "P1Y2M4DT3H43.22S";
    dur = ParseIsoDuration(s);
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be P1Y63DT3H43.220S", std::string("P1Y63DT3H43.220S") , ToIsoString(dur));

    s = "P1Y244DT3H43.22S";
    dur = ParseIsoDuration(s);
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be P1Y244DT3H43.22S", std::string("P1Y244DT3H43.220S") , ToIsoString(dur));

    s = "P1Y35D";
    dur = ParseIsoDuration(s);
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be P1Y35D", s , ToIsoString(dur));

    dur = iso_timestamp_short_from_string - iso_timestamp_even_shorter_from_string;
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be PT0.141414S", std::string("PT0.141414S") , ToIsoString(dur));

    int time_difference = (std::chrono::duration_cast <std::chrono::microseconds> (dur)).count();
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Time difference between 2014-04-04T14:14:14+02:00 and 2014-04-04T14:14:14.141414+02:00 should be 141414µs", 141414, time_difference);

    // GetLastFullHour Test
    tTimestamp iso_timestamp_last_hour1 = GetLastFullHour(iso_timestamp_from_string);
    dur = iso_timestamp_from_string - iso_timestamp_last_hour1;
    tDuration expected_duration = std::chrono::duration_cast<tDuration>(std::chrono::minutes(14) + std::chrono::seconds(14) + std::chrono::nanoseconds(141414141));
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be PT14M14.141414141S", ToIsoString(expected_duration), ToIsoString(dur));

    tTimestamp iso_timestamp_last_hour2 = GetLastFullHour(iso_timestamp_even_shorter_from_string);
    dur = iso_timestamp_even_shorter_from_string - iso_timestamp_last_hour2;
    RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Duration should be PT14M14S", std::string("PT14M14S"), ToIsoString(dur));

    // ParseNmeaTimestamp Test
    {
      tTimestamp nmea_timestamp = ParseNmeaTimestamp("140512", "170414");
      tTimestamp reference_timestamp = ParseIsoTimestamp("2014-04-17T14:05:12+00:00");
      RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Timestamps should be equal", ToIsoString(reference_timestamp), ToIsoString(nmea_timestamp));
    }
    {
      tTimestamp nmea_timestamp = ParseNmeaTimestamp("140512.123", "170414");
      tTimestamp reference_timestamp = ParseIsoTimestamp("2014-04-17T14:05:12.123+00:00");
      RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Timestamps should be equal", ToIsoString(reference_timestamp), ToIsoString(nmea_timestamp));
    }
    {
      tTimestamp nmea_timestamp = ParseNmeaTimestamp("140512.5", "170414");
      tTimestamp reference_timestamp = ParseIsoTimestamp("2014-04-17T14:05:12.5+00:00");
      RRLIB_UNIT_TESTS_EQUALITY_MESSAGE("Timestamps should be equal", ToIsoString(reference_timestamp), ToIsoString(nmea_timestamp));
    }

  }
};

RRLIB_UNIT_TESTS_REGISTER_SUITE(TestTime);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}

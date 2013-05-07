//
// You received this file as part of RRLib
// Robotics Research Library
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//----------------------------------------------------------------------
/*!\file    rrlib/time/test/IsoStringTest.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-06-17
 *
 */
//----------------------------------------------------------------------
#include <cstdio>
#include "rrlib/time/time.h"

using namespace rrlib::time;

int main(int, char**)
{
  std::string test1 = ToIsoString(std::chrono::system_clock::now());
  printf("Now    is: %s\n", test1.c_str());

  const char* s = "2012-06-16T15:20:26.12345+03:00";
  tTimestamp tp = ParseIsoTimestamp(s);
  test1 = ToIsoString(tp);
  printf("Parsed is: %s\n", test1.c_str());

  s = "2012-06-16T15:20:26+03:00";
  tp = ParseIsoTimestamp(s);
  test1 = ToIsoString(tp);
  printf("Parsed is: %s\n", test1.c_str());

  s = "2012-06-16T15:20:26.14Z";
  tp = ParseIsoTimestamp(s);
  test1 = ToIsoString(tp);
  printf("Parsed is: %s\n", test1.c_str());

  tDuration dur = std::chrono::seconds(3235) + std::chrono::milliseconds(25);
  test1 = ToIsoString(dur);
  printf("Duration is: %s\n", test1.c_str());

  dur = std::chrono::seconds(43) + std::chrono::microseconds(123400);
  test1 = ToIsoString(dur);
  printf("Duration is: %s\n", test1.c_str());

  dur = std::chrono::hours(24 * 400);
  test1 = ToIsoString(dur);
  printf("Duration is: %s\n", test1.c_str());

  s = "P400D";
  dur = ParseIsoDuration(s);
  test1 = ToIsoString(dur);
  printf("Duration is: %s\n", test1.c_str());

  s = "PT43.1234S";
  dur = ParseIsoDuration(s);
  test1 = ToIsoString(dur);
  printf("Duration is: %s\n", test1.c_str());

  s = "P1Y2M4DT3H43.22S";
  dur = ParseIsoDuration(s);
  test1 = ToIsoString(dur);
  printf("Duration is: %s\n", test1.c_str());

  s = "P1Y244DT3H43.22S";
  dur = ParseIsoDuration(s);
  test1 = ToIsoString(dur);
  printf("Duration is: %s\n", test1.c_str());

  s = "P1Y35D";
  dur = ParseIsoDuration(s);
  test1 = ToIsoString(dur);
  printf("Duration is: %s\n", test1.c_str());

  return 0;
}

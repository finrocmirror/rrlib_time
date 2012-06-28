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
/*!\file    rrlib/time/time.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-05-23
 *
 */
//----------------------------------------------------------------------
#include "rrlib/time/time.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <mutex>
#include <atomic>
#include <time.h>
#include <cstring>
#include <sstream>
#include <stdexcept>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/time/tTimeStretchingListener.h"
#include "rrlib/time/tCustomClock.h"
#include "rrlib/time/tAtomicTimestamp.h"

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
tTimestamp cNO_TIME;

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

/*! Current time mode (FIXME: we use int as template parameter, because enum support is not available until gcc 4.7) */
static std::atomic<int> mode((int)tTimeMode::SYSTEM_TIME);

/*! Time stretching parameters: application time = application_start + time_stretching_factor * (system time - application_start - time_diff); */
struct tTimeStretchingParameters
{
  uint time_scaling_numerator, time_scaling_denominator;
  tDuration time_diff;
};

/*! Helper struct for storing parameters in two int64 atomic variables */
static const uint cNUMERATOR_MASK = 0xFFFFF; // 20 bit
static const uint cSTAMP_MASK = 0xFFF; // 12 bit
union tParameterStoreHelper
{
  uint64_t longs[2]; // 12 bit stamp, 20 bit numerator or denominator, 32 bit time_diff part

  int GetStamp()
  {
    uint stamp1 = longs[0] >> 52;
    uint stamp2 = longs[1] >> 52;
    return stamp1 == stamp2 ? (int)stamp1 : -1;
  }
};
static_assert(sizeof(tParameterStoreHelper) == 16, "?!");

/*! Parameter storage - two buffers to avoid live-locks */
static std::atomic<uint64_t> time_stretching_parameters1(1LL << 32);
static std::atomic<uint64_t> time_stretching_parameters2(1LL << 32);
static std::atomic<uint64_t> time_stretching_parameters1_copy(1LL << 32);
static std::atomic<uint64_t> time_stretching_parameters2_copy(1LL << 32);
static const tTimestamp application_start = Now();

/*! Current time - in non-linear clock mode */
static tAtomicTimestamp current_time;

/*! Current time source */
static const tCustomClock* current_clock = NULL;

/*! Load time stretching parameters from two valid atomic variables */
static void LoadParameters(tTimeStretchingParameters& params)
{
  // (Live-)Lock-free loading of parameters
  tParameterStoreHelper helper;
  while (true)
  {
    helper.longs[0] = time_stretching_parameters1.load();
    helper.longs[1] = time_stretching_parameters2.load(std::memory_order_relaxed);
    if (helper.GetStamp() != -1)
    {
      break;
    }
    helper.longs[0] = time_stretching_parameters1_copy.load();
    helper.longs[1] = time_stretching_parameters2_copy.load();
    if (helper.GetStamp() != -1)
    {
      break;
    }
  }

  params.time_scaling_numerator = (helper.longs[0] >> 32) && cNUMERATOR_MASK;
  params.time_scaling_denominator = (helper.longs[1] >> 32) && cNUMERATOR_MASK;
  params.time_diff = tDuration((helper.longs[0] << 32) | (helper.longs[1] & 0xFFFFFFFFLL));
}

/*! Store time stretching parameters to atomic variables */
static void StoreParameters(const tTimeStretchingParameters& params)
{
  static int64_t counter = 0;
  tParameterStoreHelper helper;
  counter = (counter + 1) & cSTAMP_MASK;
  int64_t diff = params.time_diff.count();
  helper.longs[0] = (counter << 52) | (static_cast<int64_t>(params.time_scaling_numerator) << 32) | (diff >> 32);
  helper.longs[1] = (counter << 52) | (static_cast<int64_t>(params.time_scaling_denominator) << 32) | (diff & 0xFFFFFFFFLL);
  time_stretching_parameters1.store(helper.longs[0]);
  time_stretching_parameters2.store(helper.longs[1]);
  time_stretching_parameters1_copy.store(helper.longs[0]);
  time_stretching_parameters2_copy.store(helper.longs[1]);
}


static tTimestamp ToApplicationTime(const tTimestamp& system_time)
{
  switch (GetTimeMode())
  {
  case tTimeMode::SYSTEM_TIME:
    return system_time;
  case tTimeMode::CUSTOM_CLOCK:
    return current_time.Load();
  case tTimeMode::STRETCHED_SYSTEM_TIME:
    tTimeStretchingParameters params;
    LoadParameters(params);
    std::chrono::nanoseconds tmp((system_time - application_start) - params.time_diff);
    auto ticks = tmp.count();
    ticks /= params.time_scaling_denominator; // we have nano-seconds here - so loss of precision is neglible even with denominators of 1 million - with multiplication first, there might be overflows (if our application runs for decades...)
    ticks *= params.time_scaling_numerator;
    return application_start + tDuration(ticks);
  }
  return tTimestamp();
}

tTimestamp Now(bool precise)
{
  // TODO: implement optimized retrieval of low precision time should this become a performance issue
  return ToApplicationTime(tBaseClock::now());
}

tTimeMode GetTimeMode()
{
  return (tTimeMode)mode.load(std::memory_order_relaxed);
}

void SetTimeSource(const tCustomClock* clock, const tTimestamp& initial_time)
{
  try
  {
    std::lock_guard<std::mutex> lock(internal::tTimeMutex::Instance());
    if (clock)
    {
      current_clock = clock;
      current_time.Store(initial_time);
      if (mode.load() != (int)tTimeMode::CUSTOM_CLOCK)
      {
        mode.store((int)tTimeMode::CUSTOM_CLOCK);
        tTimeStretchingListener::NotifyListeners(tTimeMode::CUSTOM_CLOCK);
      }
      tTimeStretchingListener::NotifyListeners(initial_time);
    }
    else
    {
      if (mode.load() != (int)tTimeMode::STRETCHED_SYSTEM_TIME)
      {
        mode.store((int)tTimeMode::STRETCHED_SYSTEM_TIME);
        tTimeStretchingListener::NotifyListeners(tTimeMode::STRETCHED_SYSTEM_TIME);
      }
    }
  }
  catch (std::logic_error &)
  {}
}

void SetTimeStretching(unsigned int numerator, unsigned int denominator)
{
  // check parameters
  if (numerator <= 0 || numerator > 1000000 || denominator <= 0 || denominator > 1000000)
  {
    std::cerr << "Numerator and denominator must lie between 1 and 1000000. Ignoring. Desired numerator: " << numerator << " Desired denominator: " << denominator;
    return;
  }

  try
  {
    // set values
    std::lock_guard<std::mutex> lock(internal::tTimeMutex::Instance());
    assert(denominator != 0);
    tTimeStretchingParameters params;
    LoadParameters(params);
    double new_factor = ((double)numerator) / ((double)denominator);
    double old_factor = ((double)params.time_scaling_numerator) / ((double)params.time_scaling_denominator);
    if (new_factor != old_factor)
    {
      tTimestamp system_time = std::chrono::high_resolution_clock::now();
      tTimestamp app_time = ToApplicationTime(system_time);

      params.time_diff = system_time - app_time;
      params.time_scaling_numerator = numerator;
      params.time_scaling_denominator = denominator;
      StoreParameters(params);

      if (mode.load() != (int)tTimeMode::STRETCHED_SYSTEM_TIME)
      {
        mode.store((int)tTimeMode::STRETCHED_SYSTEM_TIME);
        tTimeStretchingListener::NotifyListeners(tTimeMode::STRETCHED_SYSTEM_TIME);
      }

      tTimeStretchingListener::NotifyListeners(new_factor > old_factor);
    }
  }
  catch (std::logic_error &)
  {}

}

tDuration ToSystemDuration(const tDuration& app_duration)
{
  switch (GetTimeMode())
  {
  case tTimeMode::SYSTEM_TIME:
  case tTimeMode::CUSTOM_CLOCK:
    return app_duration;
  case tTimeMode::STRETCHED_SYSTEM_TIME:
    tTimeStretchingParameters params;
    LoadParameters(params);
    if ((app_duration.count() >> 44) == 0)
    {
      return tDuration((app_duration.count() * params.time_scaling_numerator) / params.time_scaling_denominator);
    }
    return tDuration((app_duration.count() / params.time_scaling_denominator) * params.time_scaling_numerator);
  }
  return tDuration();
}

tTimestamp ParseIsoTimestamp(const std::string& s)
{
  tm t;
  memset(&t, 0, sizeof(t));
  time_t tz = 0;
  char charbuf[s.length() + 1];
  strncpy(charbuf, s.c_str(), s.length() + 1);
  char* c = strptime(charbuf, "%FT%T", &t);
  std::chrono::nanoseconds rest(0);
  if (c && *c == '.')
  {
    char nanos[20];
    memset(nanos, '0', 9);
    nanos[9] = 0;
    for (int i = 0; i < 9; i++)
    {
      c++;
      if (!isdigit(*c))
      {
        break;
      }
      nanos[i] = *c;
    }
    rest = std::chrono::nanoseconds(atoi(nanos));
  }
  char* colon = c ? strchr(c, ':') : NULL;
  if (c && ((*c == '+') || (*c == '-')) && colon)
  {
    // we could use global 'timezone' but this is not thread-safe
    int sign = (*c == '-') ? -1 : 1;
    *colon = 0;
    tz = atoi(c) * -3600;
    colon++;
    if (*colon == '3')
    {
      tz += sign * -1800;
    }
  }
  time_t parsed = timegm(&t) + tz;
  auto ts2 = std::chrono::system_clock::from_time_t(parsed);
  return ts2 + std::chrono::duration_cast<tDuration>(rest);
}

std::string ToIsoString(const tTimestamp& timestamp)
{
  char buf[256], time_zone[12], sub_seconds[20];
  time_t tt = std::chrono::system_clock::to_time_t(timestamp);
  auto timestamp2 = std::chrono::system_clock::from_time_t(tt);
  std::chrono::nanoseconds rest = timestamp - timestamp2;
  if (rest.count() < 0)
  {
    rest += std::chrono::seconds(1);
    tt--;
    assert(rest.count() > 0);
  }
  int  ns = rest.count();

  tm tmp;
  memset(&tmp, 0, sizeof(tmp));
  strftime(buf, 256, "%FT%T", localtime_r(&tt, &tmp));
  strftime(time_zone, 12, "%z", localtime_r(&tt, &tmp));
  if (strlen(time_zone) > 3)
  {
    memmove(&time_zone[4], &time_zone[3], strlen(time_zone) - 2);
    time_zone[3] = ':';
  }

  sub_seconds[0] = 0;
  if (ns != 0)
  {
    if (ns % 1000000 == 0)
    {
      sprintf(sub_seconds, ".%03d", ns / 1000000);
    }
    else if (ns % 1000 == 0)
    {
      sprintf(sub_seconds, ".%06d", ns / 1000);
    }
    else
    {
      sprintf(sub_seconds, ".%09d", ns);
    }
  }
  return std::string(buf) + sub_seconds + time_zone;
}

tDuration ParseIsoDuration(const std::string& s)
{
  tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = 70;
  t.tm_mday = 1;

  if (s.length() == 0 || s[0] != 'P')
  {
    throw std::runtime_error(std::string("Duration string does not start with P: ") + s);
  }
  char cs[s.length() + 1];
  strncpy(cs, s.c_str(), s.length() + 1);
  size_t len = s.length();
  std::chrono::nanoseconds rest(0);

  // start with fractional seconds
  if (cs[len - 1] == 'S')
  {
    char* sec_start = &cs[len - 2];
    for (; isdigit(*sec_start); sec_start--);
    if (*sec_start == '.')
    {
      char nanos[20];
      memset(nanos, '0', 9);
      nanos[9] = 0;
      strncpy(nanos, sec_start + 1, std::min<size_t>(9, &cs[len - 1] - (sec_start + 1)));
      rest = std::chrono::nanoseconds(atoi(nanos));

      // cut off fractional seconds from string
      sec_start[0] = 'S';
      sec_start[1] = 0;
    }
  }

  // parse the rest
  bool time = false;
  char* c = &cs[1];
  while (*c)
  {
    if (isdigit(*c))
    {
      int num = atoi(c);
      for (; c && isdigit(*c); c++);
      if (!c)
      {
        throw std::runtime_error(std::string("Invalid duration string: ") + s);
      }
      switch (*c)
      {
      case 'Y':
        t.tm_year = 70 + num;
        break;
      case 'M':
        if (!time)
        {
          t.tm_mon = num;
        }
        else
        {
          t.tm_min = num;
        }
        break;
      case 'D':
        t.tm_mday = num + 1;
        break;
      case 'H':
        if (!time)
        {
          throw std::runtime_error(std::string("Invalid duration string: ") + s);
        }
        t.tm_hour = num;
        break;
      case 'S':
        if (!time)
        {
          throw std::runtime_error(std::string("Invalid duration string: ") + s);
        }
        t.tm_sec = num;
        break;
      default:
        throw std::runtime_error(std::string("Invalid duration string: ") + s);
      }
    }
    else if (*c == 'T')
    {
      time = true;
    }
    else
    {
      throw std::runtime_error(std::string("Invalid duration string: ") + s);
    }
    c++;
  }

  time_t parsed = timegm(&t);
  return std::chrono::seconds(parsed) + std::chrono::duration_cast<tDuration>(rest);
}

std::string ToIsoString(const tDuration& duration)
{
  std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(duration);
  std::chrono::nanoseconds nanos = duration - sec;
  time_t tt = sec.count();
  int ns = nanos.count();

  tm t;
  memset(&t, 0, sizeof(t));
  gmtime_r(&tt, &t);
  std::ostringstream oss;
  oss << 'P';
  t.tm_year -= 70;
  if (t.tm_year)
  {
    oss << t.tm_year << 'Y';
  }
  // we don't add months because length of months varies significantly
  if (t.tm_yday)
  {
    oss << t.tm_yday << 'D';
  }
  if (t.tm_hour || t.tm_min || t.tm_sec || ns)
  {
    oss << 'T';
    if (t.tm_hour)
    {
      oss << t.tm_hour << 'H';
    }
    if (t.tm_min)
    {
      oss << t.tm_min << 'M';
    }
    if (t.tm_sec || ns)
    {
      oss << t.tm_sec;
      if (ns)
      {
        char sub_seconds[20];
        if (ns % 1000000 == 0)
        {
          sprintf(sub_seconds, ".%03d", ns / 1000000);
        }
        else if (ns % 1000 == 0)
        {
          sprintf(sub_seconds, ".%06d", ns / 1000);
        }
        else
        {
          sprintf(sub_seconds, ".%09d", ns);
        }
        oss << sub_seconds;
      }
      oss << 'S';
    }
  }
  return oss.str();
}

std::string ToString(std::chrono::nanoseconds ns)
{
  if (ns.count() == 0)
  {
    return "0 ms";
  }
  const char* sign = "";
  if (ns.count() < 0)
  {
    sign = "-";
    ns = -ns;
  }

  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
  std::chrono::nanoseconds rest = ns - ms;
  char buf[1000];
  if (rest.count() != 0 || ms.count() != 0)
  {
    if (rest.count() % 1000 == 0)
    {
      sprintf(buf, "%s%lld.%03lld ms", sign, static_cast<long long int>(ms.count()), static_cast<long long int>(rest.count() / 1000));
      return buf;
    }
    sprintf(buf, "%s%lld.%06lld ms", sign, static_cast<long long int>(ms.count()), static_cast<long long int>(rest.count()));
    return buf;
  }
  std::chrono::minutes mins = std::chrono::duration_cast<std::chrono::minutes>(ns);
  rest = ns - mins;
  if (rest.count() != 0)
  {
    sprintf(buf, "%s%lld s", sign, static_cast<long long int>(std::chrono::duration_cast<std::chrono::seconds>(ns).count()));
    return buf;
  }
  std::chrono::hours hours = std::chrono::duration_cast<std::chrono::hours>(ns);
  rest = ns - hours;
  if (rest.count() != 0)
  {
    int c = std::chrono::duration_cast<std::chrono::minutes>(ns).count();
    sprintf(buf, "%s%d minute%s", sign, c, c > 1 ? "s" : "");
    return buf;
  }
  int c = std::chrono::duration_cast<std::chrono::hours>(ns).count();
  sprintf(buf, "%s%d hour%s", sign, c, c > 1 ? "s" : "");
  return buf;
}

void tCustomClock::SetApplicationTime(const rrlib::time::tTimestamp& new_time)
{
  try
  {
    std::lock_guard<std::mutex> lock(internal::tTimeMutex::Instance());
    if (this == current_clock && mode.load() == (int)tTimeMode::CUSTOM_CLOCK)
    {
      current_time.Store(new_time);
      tTimeStretchingListener::NotifyListeners(new_time);
    }
  }
  catch (std::logic_error &)
  {}
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}

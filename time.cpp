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
#include "rrlib/util/patterns/singleton.h"

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

class tTimeMutex : public std::mutex {};
std::mutex* internal::GetMutex()
{
  typedef rrlib::util::tSingletonHolder<tTimeMutex> m;
  return m::Destroyed() ? NULL : &m::Instance();
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
  if (!internal::GetMutex())
  {
    return;
  }
  std::lock_guard<std::mutex> lock(*internal::GetMutex());
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

void SetTimeStretching(unsigned int numerator, unsigned int denominator)
{
  // check parameters
  if (numerator <= 0 || numerator > 1000000 || denominator <= 0 || denominator > 1000000)
  {
    std::cerr << "Numerator and denominator must lie between 1 and 1000000. Ignoring. Desired numerator: " << numerator << " Desired denominator: " << denominator;
    return;
  }
  if (!internal::GetMutex())
  {
    return;
  }

  // set values
  std::lock_guard<std::mutex> lock(*internal::GetMutex());
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

void tCustomClock::SetApplicationTime(const rrlib::time::tTimestamp& new_time)
{
  if (internal::GetMutex())
  {
    std::lock_guard<std::mutex> lock(*internal::GetMutex());
    if (this == current_clock && mode.load() == (int)tTimeMode::CUSTOM_CLOCK)
    {
      current_time.Store(new_time);
      tTimeStretchingListener::NotifyListeners(new_time);
    }
  }
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}

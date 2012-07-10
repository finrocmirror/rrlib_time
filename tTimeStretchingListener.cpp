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
/*!\file    rrlib/time/tTimeStretchingListener.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-06-03
 *
 */
//----------------------------------------------------------------------
#include "rrlib/time/tTimeStretchingListener.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <algorithm>
#include "rrlib/design_patterns/singleton.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

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

typedef rrlib::design_patterns::tSingletonHolder<std::vector<tTimeStretchingListener*>> tListenersSingleton;

template <typename tLambdaFunction>
static void NotifyListenersImpl(tLambdaFunction f)
{
  try
  {
    auto list = tListenersSingleton::Instance();
    for (auto it = list.begin(); it < list.end(); it++)
    {
      f(**it);
    }
  }
  catch (std::logic_error &)
  {}
}

tTimeStretchingListener::tTimeStretchingListener()
{
  try
  {
    std::lock_guard<std::mutex> lock(internal::tTimeMutex::Instance());
    tListenersSingleton::Instance().push_back(this);
  }
  catch (std::logic_error &)
  {}
}

tTimeStretchingListener::~tTimeStretchingListener()
{
  try
  {
    std::lock_guard<std::mutex> lock(internal::tTimeMutex::Instance());
    std::vector<tTimeStretchingListener*> &listeners = tListenersSingleton::Instance();
    listeners.erase(std::remove(listeners.begin(), listeners.end(), this), listeners.end());
  }
  catch (std::logic_error &)
  {}
}

void tTimeStretchingListener::NotifyListeners(const tTimestamp& current_time)
{
  NotifyListenersImpl([&](tTimeStretchingListener & l)
  {
    l.TimeChanged(current_time);
  });
}

void tTimeStretchingListener::NotifyListeners(rrlib::time::tTimeMode new_mode)
{
  NotifyListenersImpl([&](tTimeStretchingListener & l)
  {
    l.TimeModeChanged(new_mode);
  });
}

void tTimeStretchingListener::NotifyListeners(bool app_time_faster)
{
  NotifyListenersImpl([&](tTimeStretchingListener & l)
  {
    l.TimeStretchingFactorChanged(app_time_faster);
  });
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}

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
#include "rrlib/util/patterns/singleton.h"

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

typedef rrlib::util::tSingletonHolder<std::vector<tTimeStretchingListener*>> tListenersSingleton;


tTimeStretchingListener::tTimeStretchingListener()
{
  if (internal::GetMutex() && (!tListenersSingleton::Destroyed()))
  {
    std::lock_guard<std::mutex> lock(*internal::GetMutex());
    tListenersSingleton::Instance().push_back(this);
  }
}

tTimeStretchingListener::~tTimeStretchingListener()
{
  if (internal::GetMutex() && (!tListenersSingleton::Destroyed()))
  {
    std::lock_guard<std::mutex> lock(*internal::GetMutex());
    std::vector<tTimeStretchingListener*>& l = tListenersSingleton::Instance();
    l.erase(std::remove(l.begin(), l.end(), this), l.end());
  }
}

void tTimeStretchingListener::NotifyListeners(const tTimestamp& current_time)
{
  if (!tListenersSingleton::Destroyed())
  {
    auto list = tListenersSingleton::Instance();
    for (auto it = list.begin(); it < list.end(); it++)
    {
      (*it)->TimeChanged(current_time);
    }
  }
}

void tTimeStretchingListener::NotifyListeners(rrlib::time::tTimeMode new_mode)
{
  if (!tListenersSingleton::Destroyed())
  {
    auto list = tListenersSingleton::Instance();
    for (auto it = list.begin(); it < list.end(); it++)
    {
      (*it)->TimeModeChanged(new_mode);
    }
  }
}

void tTimeStretchingListener::NotifyListeners(bool app_time_faster)
{
  if (!tListenersSingleton::Destroyed())
  {
    auto list = tListenersSingleton::Instance();
    for (auto it = list.begin(); it < list.end(); it++)
    {
      (*it)->TimeStretchingFactorChanged(app_time_faster);
    }
  }
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}

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
/*!\file    rrlib/time/tAtomicDuration.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-06-14
 *
 * \brief   Contains tAtomicDuration
 *
 * \b tAtomicDuration
 *
 * Atomic duration (to safely exchange durations among threads)
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__time__tAtomicDuration_h__
#define __rrlib__time__tAtomicDuration_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <atomic>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/time/time.h"

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
// Class declaration
//----------------------------------------------------------------------
//! Atomic duration
/*!
 * Atomic duration (to safely exchange durations among threads)
 */
class tAtomicDuration
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tAtomicDuration(const tDuration& duration = tDuration::zero())
  {
    Store(duration);
  }

  /*!
   * Obtains value from atomic.
   */
  tDuration Load() const
  {
    return tDuration(wrapped.load());
  }

  /*!
   * Stores value to atomic
   */
  void Store(const tDuration& duration)
  {
    wrapped.store(duration.count());
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Wrapped std::atomic */
  std::atomic<int64_t> wrapped;

  // noncopyable (as atomics generally are)
  tAtomicDuration(const tAtomicDuration&) = delete;
  tAtomicDuration& operator=(const tAtomicDuration&) = delete;
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif

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
/*!\file    rrlib/time/tTimeStretchingListener.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-06-03
 *
 * \brief   Contains tTimeStretchingListener
 *
 * \b tTimeStretchingListener
 *
 * Informed when time stretching factor changes
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__time__tTimeStretchingListener_h__
#define __rrlib__time__tTimeStretchingListener_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <iostream>

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
//! Time Stretching Listener
/*!
 * Informed when time stretching factor changes.
 * Is automatically registered for receiving notifications when created.
 */
class tTimeStretchingListener
{
protected:

  tTimeStretchingListener();
  virtual ~tTimeStretchingListener();

  // noncopyable (otherwise callbacks would be difficult)
  tTimeStretchingListener(const tTimeStretchingListener&) = delete;
  tTimeStretchingListener& operator=(const tTimeStretchingListener&) = delete;

  /*!
   * If "application time" is set from an external entity, this method is called whenever
   * time changes.
   *
   * \param current_time Current "application time" from non-linear clock
   */
  virtual void TimeChanged(const tTimestamp& current_time) = 0;

  /*!
   * Called whenever the current time mode changes.
   *
   * \param new_mode New time mode
   */
  virtual void TimeModeChanged(rrlib::time::tTimeMode new_mode) = 0;

  /*!
   * Called whenever the time stretching factor changes
   *
   * \param app_time_faster True if application time flows faster than before
   */
  virtual void TimeStretchingFactorChanged(bool app_time_faster) = 0;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:
  friend void SetTimeStretching(unsigned int numerator, unsigned int denominator);
  friend void SetTimeSource(const tCustomClock* clock, const tTimestamp& initial_time);
  friend class tCustomClock;

  /*!
   * Notifies all listeners of time change
   *
   * \param current_time Current "application time" from non-linear clock
   */
  static void NotifyListeners(const tTimestamp& current_time);

  /*!
   * Notifies all listeners of time mode change
   *
   * \param new_mode New time mode
   */
  static void NotifyListeners(rrlib::time::tTimeMode new_mode);

  /*!
   * Notifies all listeners of time stretching change
   *
   * \param app_time_faster True if application time flows faster than before
   */
  static void NotifyListeners(bool app_time_faster);
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif

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
/*!\file    rrlib/time/tCustomClock.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-06-07
 *
 * \brief   Contains tCustomClock
 *
 * \b tCustomClock
 *
 * Using this class as base class, "application time" can be set from an external entity.
 * The time does not need to be in any (linear) way related to real or system time.
 * Between calls to SetApplicationTime() "application time" will remain the same.
 * Thus, SetApplicationTime() should be called with relatively high frequency.
 */
//----------------------------------------------------------------------
#ifndef __rrlib__time__tCustomClock_h__
#define __rrlib__time__tCustomClock_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

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
//! Custom clock base class.
/*!
 * Using this class as base class, "application time" can be set from an external entity.
 * Time does not have to be in any way related to system time.
 * Between calls to SetApplicationTime() "application time" will remain the same.
 * Thus, SetApplicationTime() should be called with relatively high frequency.
 *
 * In order to set this clock as active time source, SetTimeSource (in time.h) must be called.
 */
class tCustomClock
{
  // noncopyable (otherwise clock could not be identified by pointer)
  tCustomClock(const tCustomClock&) = delete;
  tCustomClock& operator=(const tCustomClock&) = delete;

//----------------------------------------------------------------------
// Protected methods and typedefs
//----------------------------------------------------------------------
protected:

  /*!
   * Sets new "application time".
   *
   * \param new_time New "application time".
   */
  void SetApplicationTime(const rrlib::time::tTimestamp& new_time);
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif

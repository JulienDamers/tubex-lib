/* ============================================================================
 *  tubex-lib - DomainException class
 * ============================================================================
 *  Copyright : Copyright 2017 Simon Rohou
 *  License   : This program is distributed under the terms of
 *              the GNU Lesser General Public License (LGPL).
 *
 *  Author(s) : Simon Rohou
 *  Bug fixes : -
 *  Created   : 2015
 * ---------------------------------------------------------------------------- */

#ifndef DomainException_HEADER
#define DomainException_HEADER

#include <iostream>
#include <exception>
#include <string>
#include <sstream>
#include "tubex_TubeNode.h"
#include "tubex_Exception.h"

namespace tubex
{
  /**
   * \brief Domain error exception.
   *
   * Thrown when an access to a tube is impossible.
   */
  class DomainException : public Exception
  {
    public:

      DomainException(const TubeNode& x, int slice_index);
      DomainException(const TubeNode& x, double t);
      DomainException(const Trajectory& x, double t);
      DomainException(const TubeNode& x, const ibex::Interval& t);
      DomainException(const Trajectory& x, const ibex::Interval& t);
      DomainException(const TubeNode& x1, const TubeNode& x2);
      DomainException(const Trajectory& x1, const Trajectory& x2);
        
      static void check(const TubeNode& x, int slice_index);
      static void check(const TubeNode& x, double t);
      static void check(const Trajectory& x, double t);
      static void check(const TubeNode& x, const ibex::Interval& t);
      static void check(const Trajectory& x, const ibex::Interval& t);
      static void check(const TubeNode& x1, const TubeNode& x2);
      static void check(const Trajectory& x1, const Trajectory& x2);
  };
}

#endif
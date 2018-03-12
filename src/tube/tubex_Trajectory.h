/* ============================================================================
 *  tubex-lib - Trajectory class
 * ============================================================================
 *  Copyright : Copyright 2017 Simon Rohou
 *  License   : This program is distributed under the terms of
 *              the GNU Lesser General Public License (LGPL).
 *
 *  Author(s) : Simon Rohou
 *  Bug fixes : -
 *  Created   : 2018
 * ---------------------------------------------------------------------------- */

#ifndef TRAJECTORY_HEADER
#define TRAJECTORY_HEADER

#include <map>
#include "ibex_Interval.h"

namespace tubex
{
  class Trajectory
  {
    public:

      /**
       * \brief Default constructor
       */
      Trajectory(std::map<double,double> m_map_values);
      const ibex::Interval domain() const;
      double operator[](double t) const;
      void truncateDomain(const ibex::Interval& domain);
      //void shiftDomain(double shift);


    protected:

      /** Class variables **/

        std::map<double,double> m_map_values;
  };
}

#endif
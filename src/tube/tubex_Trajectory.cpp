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

#include "tubex_Trajectory.h"

using namespace std;
using namespace ibex;

namespace tubex
{
  Trajectory::Trajectory(const map<double,double> map_values) : m_map_values(map_values)
  {

  }

  const Interval Trajectory::domain() const
  {
    return Interval(m_map_values.begin()->first, m_map_values.rbegin()->first);
  }

  double Trajectory::operator[](double t) const
  {
    if(!domain().contains(t))
      return std::numeric_limits<double>::quiet_NaN();

    if(domain().ub() == t)
      return m_map_values.at(t);

    typename map<double,double>::const_iterator it_lower, it_upper;
    it_lower = m_map_values.lower_bound(t);

    it_upper = it_lower;
    it_upper++;

    return it_lower->second +
           (t - it_lower->first) * (it_upper->second - it_lower->second) /
           (it_upper->first - it_lower->first);
  }

  void Trajectory::truncateDomain(const Interval& domain)
  {
    map<double,double>::iterator it = m_map_values.begin();
    while(it != m_map_values.end())
    {
      if(!domain.contains(it->first)) it = m_map_values.erase(it);
      else ++it;
    }
  }

  /*void Trajectory::shiftDomain(double shift)
  {

  }*/
}
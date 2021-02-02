/** 
 *  GrahamScan class
 * ----------------------------------------------------------------------------
 *  \date       2019
 *  \author     Simon Rohou
 *  \copyright  Copyright 2021 Codac Team
 *  \license    This program is distributed under the terms of
 *              the GNU Lesser General Public License (LGPL).
 */

#ifndef __CODAC_GRAHAMSCAN_H__
#define __CODAC_GRAHAMSCAN_H__

#include <stack>
#include "codac_Interval.h"
#include "codac_Point.h"
#include "codac_ConvexPolygon.h"

namespace codac
{
  enum class OrientationInterval { CLOCKWISE, COUNTERCLOCKWISE, UNDEFINED } ;

  class GrahamScan
  {
    public:

      // Prints convex hull of a set of n points.
      static const std::vector<ibex::Vector> convex_hull(const std::vector<ibex::Vector>& v_points);


    protected:

      // A utility function to find next to top in a stack
      static const ibex::Vector next_to_top(const std::stack<ibex::Vector>& s);
      
      // A utility function to swap two points
      static void swap(ibex::Vector& p1, ibex::Vector& p2);

      // A utility function to return square of distance between p1 and p2
      static const Interval dist(const IntervalVector& p1, const IntervalVector& p2);

      // To find orientation of ordered triplet (p, q, r).
      static OrientationInterval orientation(const IntervalVector& p0, const IntervalVector& p1, const IntervalVector& p2);

      friend class PointsSorter;
      friend class ConvexPolygon;
  };

  class PointsSorter
  {
    public:

      PointsSorter(const ibex::Vector& p0);
      bool operator()(const ibex::Vector& p1, const ibex::Vector& p2);

    protected:

      ibex::Vector m_p0 = ibex::Vector(2);
  };
}

#endif
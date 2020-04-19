/** 
 *  \file
 *  CtcLinobs class
 * ----------------------------------------------------------------------------
 *  \date       2020
 *  \author     Simon Rohou
 *  \copyright  Copyright 2020 Simon Rohou
 *  \license    This program is distributed under the terms of
 *              the GNU Lesser General Public License (LGPL).
 */

#ifndef __TUBEX_CTCLINOBS_H__
#define __TUBEX_CTCLINOBS_H__

#include "tubex_Ctc.h"
#include "tubex_ConvexPolygon.h"

namespace tubex
{
  /**
   * \class CtcLinobs
   */
  class CtcLinobs : public Ctc
  {
    public:

      /**
       * \brief Creates a contractor object \f$\mathcal{C}_\textrm{linobs}\f$
       */
      // CtcLinobs(const ibex::Matrix& A, const ibex::Vector& b); // not yet available since auto evaluation of e^At not at hand
      CtcLinobs(const ibex::Matrix& A, const ibex::Vector& b, ibex::IntervalMatrix (*exp_At)(const ibex::Matrix& A, const ibex::Interval& t));

      void contract(std::vector<Domain*>& v_domains);
      void contract(TubeVector& x, const Tube& u);

    protected:

      void ctc_fwd_gate(ConvexPolygon& p_k, const ConvexPolygon& p_km1, double dt_km1_k, const ibex::Matrix& A, const ibex::Vector& b, const ibex::Interval& u_km1);
      void ctc_bwd_gate(ConvexPolygon& p_k, const ConvexPolygon& p_kp1, double dt_k_kp1, const ibex::Matrix& A, const ibex::Vector& b, const ibex::Interval& u_k);

      ConvexPolygon polygon_envelope(const ConvexPolygon& p_k, double dt_k_kp1, const ibex::Matrix& A, const ibex::Vector& b, const ibex::Interval& u_k);

    protected:

      const ibex::Matrix& m_A;
      const ibex::Vector& m_b;
      ibex::IntervalMatrix (*m_exp_At)(const ibex::Matrix& A, const ibex::Interval& t);

      const int m_polygon_max_edges = 15;
  };
}

#endif
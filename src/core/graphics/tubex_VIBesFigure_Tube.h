/* ============================================================================
 *  tubex-lib - VIBesFigure_Tube class
 * ============================================================================
 *  Copyright : Copyright 2017 Simon Rohou
 *  License   : This program is distributed under the terms of
 *              the GNU Lesser General Public License (LGPL).
 *
 *  Author(s) : Simon Rohou
 *  Bug fixes : -
 *  Created   : 2015
 * ---------------------------------------------------------------------------- */

#ifndef __TUBEX_VIBESFIGURETUBE_H__
#define __TUBEX_VIBESFIGURETUBE_H__

#include "tubex_VIBesFigure.h"
#include "tubex_Tube.h"
#include "tubex_Slice.h"
#include "tubex_Trajectory.h"

namespace tubex
{
  #define DEFAULT_TUBE_NAME         "[?](·)"
  #define DEFAULT_TRAJ_NAME         "?(·)"
  #define TRAJ_NB_DISPLAYED_POINTS  10000
  
  // HTML color codes:
  #define DEFAULT_TRAJ_COLOR        "#004257"
  #define DEFAULT_FRGRND_COLOR      "#a2a2a2[#a2a2a2]"
  #define DEFAULT_BCKGRND_COLOR     "#d2d2d2[#d2d2d2]"
  #define DEFAULT_SLICES_COLOR      "#828282[#F0F0F0]"
  #define DEFAULT_GATES_COLOR       "#0084AF[#0084AF]"
  #define DEFAULT_POLYGONS_COLOR    "#00536E[#2696BA]"

  class VIBesFigure_Tube : public VIBesFigure
  {
    public:

      VIBesFigure_Tube(const std::string& fig_name, const Tube *tube = NULL, const Trajectory *traj = NULL);
      ~VIBesFigure_Tube();

      void add_tube(const Tube *tube, const std::string& name, const std::string& color_frgrnd = DEFAULT_FRGRND_COLOR, const std::string& color_bckgrnd = DEFAULT_BCKGRND_COLOR);
      void set_tube_name(const Tube *tube, const std::string& name);
      void set_tube_derivative(const Tube *tube, const Tube *derivative);
      void set_tube_color(const Tube *tube, const std::string& color_frgrnd, const std::string& color_bckgrnd);
      void set_tube_color(const Tube *tube, int color_type, const std::string& color);
      void remove_tube(const Tube *tube);

      void add_trajectory(const Trajectory *traj, const std::string& name, const std::string& color = DEFAULT_TRAJ_COLOR);
      void set_trajectory_name(const Trajectory *traj, const std::string& name);
      void set_trajectory_color(const Trajectory *traj, const std::string& color);
      void remove_trajectory(const Trajectory *traj);

      void show();
      void show(bool detail_slices);

      enum TubeColorType { FOREGROUND, BACKGROUND, SLICES, GATES, POLYGONS };

    protected:

      const ibex::IntervalVector draw_tube(const Tube *tube, bool detail_slices = false);
      const Polygon polygon_envelope(const Tube *tube) const;
      void draw_slice(const Slice& slice, const vibes::Params& params);
      void draw_slice(const Slice& slice, const Slice& deriv_slice, const vibes::Params& params_slice, const vibes::Params& params_polygon);
      void draw_gate(const ibex::Interval& gate, double t, const vibes::Params& params);
      const ibex::IntervalVector draw_trajectory(const Trajectory *traj, float points_size = 0.);
      void create_group_color(const Tube *tube, int color_type);
      void create_groups_color(const Tube *tube);

    protected:

      struct FigTubeParams
      {
        std::map<int,std::string> m_colors;
        const Tube *tube_copy = NULL; // to display previous values in background
        const Tube *tube_derivative = NULL; // to display polygons enclosed by slices
        std::string name;
      };

      struct FigTrajParams
      {
        std::string color;
        std::string name;
      };

      std::map<const Tube*,FigTubeParams> m_map_tubes;
      std::map<const Trajectory*,FigTrajParams> m_map_trajs;

      friend class VIBesFigure_TubeVector;
  };
}

#endif
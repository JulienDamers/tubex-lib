/* ============================================================================
 *  tubex-lib - Tube class
 * ============================================================================
 *  Copyright : Copyright 2017 Simon Rohou
 *  License   : This program is distributed under the terms of
 *              the GNU Lesser General Public License (LGPL).
 *
 *  Author(s) : Simon Rohou
 *  Bug fixes : -
 *  Created   : 2015
 * ---------------------------------------------------------------------------- */

#include "tubex_Tube.h"
#include "tubex_Exception.h"
#include "tubex_DomainException.h"
#include "tubex_TubeSerialization.h"
#include "tubex_TrajectorySerialization.h"
#include "tubex_CtcDeriv.h"
#include "tubex_CtcEval.h"

using namespace std;
using namespace ibex;

namespace tubex
{
  // Public methods

    // Definition

    Tube::Tube(const Interval& domain, const Interval& codomain)
    {
      // By default, the tube is defined as one single slice
      m_component = new TubeSlice(domain, codomain);
      m_component->setTubeReference(this);
    }
    
    Tube::Tube(const Interval& domain, double timestep, const Interval& codomain) : Tube(domain, codomain)
    {
      if(timestep < 0.)
        throw Exception("Tube constructor", "invalid timestep");

      else if(timestep > 0. && timestep < domain.diam())
      {
        double lb, ub = domain.lb();
        vector<double> v_bounds; // a vector of slices is created only once
        do
        {
          lb = ub; // we guarantee all slices are adjacent
          ub = lb + timestep;
          if(ub < domain.ub()) v_bounds.push_back(ub);
        } while(ub < domain.ub());

        sample(v_bounds);
      }

      else // timestep == 0. or timestep >= domain.diam()
      {
        // then the tube stays defined as one single slice
      }
    }
    
    Tube::Tube(const Interval& domain, double timestep, const Function& function) : Tube(domain, timestep)
    {
      set(function);
    }

    Tube::Tube(const Tube& x)
    {
      *this = x;
    }

    Tube::Tube(const Tube& x, const Interval& codomain) : Tube(x)
    {
      set(codomain);
    }

    Tube::Tube(const Tube& x, const Function& function) : Tube(x)
    {
      set(function);
    }

    Tube::Tube(const Trajectory& traj, double thickness, double timestep) : Tube(traj.domain(), timestep, Interval::EMPTY_SET)
    {
      *this |= traj;
      inflate(thickness);
    }

    Tube::Tube(const Trajectory& lb, const Trajectory& ub, double timestep) : Tube(lb.domain(), timestep, Interval::EMPTY_SET)
    {
      *this |= lb;
      *this |= ub;
    }

    Tube::Tube(const string& binary_file_name)
    {
      vector<Trajectory> v_trajs;
      deserialize(binary_file_name, v_trajs);
    }
    
    Tube::Tube(const string& binary_file_name, Trajectory& traj)
    {
      vector<Trajectory> v_trajs;
      deserialize(binary_file_name, v_trajs);

      if(v_trajs.size() == 0)
        throw Exception("Tube constructor", "unable to deserialize a Trajectory");

      traj = v_trajs[0];
    }

    Tube::Tube(const string& binary_file_name, vector<Trajectory>& v_trajs)
    {
      deserialize(binary_file_name, v_trajs);
      if(v_trajs.size() == 0)
        throw Exception("Tube constructor", "unable to deserialize some Trajectory");
    }
    
    Tube::~Tube()
    {
      if(m_component == NULL)
        throw Exception("Tube destructor", "uninitialized component");
      delete m_component;
    }

    Tube Tube::primitive(const Interval& initial_value) const
    {
      Tube primitive(*this, Interval::ALL_REALS);
      primitive.set(primitive.domain().lb(), initial_value);
      primitive.ctcFwd(*this);
      return primitive;
    }

    Tube& Tube::operator=(const Tube& x)
    {
      if(typeid(*(x.m_component)) == typeid(TubeSlice))
        m_component = new TubeSlice(*((TubeSlice*)x.m_component));

      else if(typeid(*(x.m_component)) == typeid(TubeNode))
        m_component = new TubeNode(*((TubeNode*)x.m_component));

      else
        throw Exception("Tube constructor", "invalid component");

      m_component->setTubeReference(this);
      return *this;
    }

    const Interval& Tube::domain() const
    {
      return m_component->domain();
    }
  
    // Slices structure

    int Tube::nbSlices() const
    {
      return m_component->nbSlices();
    }

    TubeSlice* Tube::getSlice(int slice_id)
    {
      return m_component->getSlice(slice_id);
    }

    const TubeSlice* Tube::getSlice(int slice_id) const
    {
      return m_component->getSlice(slice_id);
    }

    TubeSlice* Tube::getSlice(double t)
    {
      return m_component->getSlice(t);
    }

    const TubeSlice* Tube::getSlice(double t) const
    {
      return m_component->getSlice(t);
    }

    TubeSlice* Tube::getFirstSlice() const
    {
      return m_component->getFirstSlice();
    }

    TubeSlice* Tube::getLastSlice() const
    {
      return m_component->getLastSlice();
    }

    TubeSlice* Tube::getWiderSlice() const
    {
      double max_domain_width = 0.;
      TubeSlice *wider_slice, *slice = getFirstSlice();

      while(slice != NULL)
      {
        if(slice->domain().diam() > max_domain_width)
        {
          wider_slice = slice;
          max_domain_width = slice->domain().diam();
        }

        slice = slice->nextSlice();
      }

      return wider_slice;
    }

    void Tube::getSlices(vector<const TubeSlice*>& v_slices) const
    {
      m_component->getSlices(v_slices);
    }

    int Tube::input2index(double t) const
    {
      return m_component->input2index(t);
    }

    void Tube::sample(double t, const Interval& gate)
    {
      DomainException::check(*this, t);
      
      TubeSlice *slice_to_be_sampled = getSlice(t);

      if(slice_to_be_sampled->domain().lb() == t || slice_to_be_sampled->domain().ub() == t)
      {
        // No degenerate slice,
        // the method has no effect.
        return;
      }

      else
      {
        TubeComponent *parent = m_component->getParentOf(slice_to_be_sampled);
        TubeComponent *new_component = new TubeNode(*slice_to_be_sampled, t);

        if(parent == NULL) // no parent, the tube has one slice
        {
          delete m_component;
          m_component = new_component;
        }

        else
        {
          TubeComponent *first_component = ((TubeNode*)parent)->m_first_component;
          TubeComponent *second_component = ((TubeNode*)parent)->m_second_component;

          if(slice_to_be_sampled == (TubeSlice*)first_component)
          {
            delete first_component;
            ((TubeNode*)parent)->m_first_component = new_component;
          }
          
          else if(slice_to_be_sampled == (TubeSlice*)second_component)
          {
            delete second_component;
            ((TubeNode*)parent)->m_second_component = new_component;
          }

          else
            throw Exception("Tube::sample", "unhandled case");
        }

        m_component->updateSlicesNumber();
        set(gate, t);
      }
    }

    void Tube::sample(const vector<double>& v_bounds)
    {
      if(v_bounds.empty())
        return;

      vector<double> v_first_bounds, v_last_bounds;

      int mid = v_bounds.size() / 2;
      for(int i = 0 ; i < v_bounds.size() ; i++)
      {
        if(i < mid) v_first_bounds.push_back(v_bounds[i]);
        else if(i <= mid) sample(v_bounds[i]);
        else v_last_bounds.push_back(v_bounds[i]);
      }

      sample(v_first_bounds);
      sample(v_last_bounds);
    }

    TubeComponent* Tube::getTubeComponent()
    {
      return m_component;
    }

    // Access values

    const Interval& Tube::codomain() const
    {
      return m_component->codomain();
    }

    double Tube::volume() const
    {
      TubeSlice *slice = getFirstSlice();
      double volume = 0.;
      while(slice != NULL)
      {
        volume += slice->box().volume();
        slice = slice->nextSlice();
      }
      return volume;
    }

    const Interval Tube::operator[](int slice_id) const
    {
      return (*m_component)[slice_id];
    }

    const Interval Tube::operator[](double t) const
    {
      return (*m_component)[t];
    }

    const Interval Tube::operator[](const Interval& t) const
    {
      return (*m_component)[t];
    }

    Interval Tube::invert(const Interval& y, const Interval& search_domain) const
    {
      return m_component->invert(y, search_domain);
    }

    void Tube::invert(const Interval& y, vector<Interval> &v_t, const Interval& search_domain) const
    {
      if(typeid(*m_component) == typeid(TubeSlice))
        ((TubeSlice*)m_component)->invert(y, v_t, search_domain);

      else if(typeid(*m_component) == typeid(TubeNode))
        ((TubeNode*)m_component)->invert(y, v_t, search_domain);

      else
        throw Exception("Tube::invert", "invalid component");
    }

    const pair<Interval,Interval> Tube::eval(const Interval& t) const
    {
      return m_component->eval(t);
    }

    const Interval Tube::interpol(double t, const Tube& derivative) const
    {
      return interpol(Interval(t), derivative);
      // todo: check a faster implementation for this degenerate case?
    }

    const Interval Tube::interpol(const Interval& t, const Tube& derivative) const
    {
      Interval y;
      CtcDeriv ctc;
      Interval t_ = t;
      ctc.contract(*this, derivative, t_, y);
      return y;
    }

    double Tube::maxThickness()
    {
      int first_id_max_thickness;
      return maxThickness(first_id_max_thickness);
    }

    double Tube::maxThickness(int& first_id_max_thickness)
    {
      int i = 0;
      double max_thickness = 0.;

      TubeSlice *slice = getFirstSlice();
      while(slice != NULL)
      {
        if(slice->codomain().diam() > max_thickness)
        {
          max_thickness = slice->codomain().diam();
          first_id_max_thickness = i;
        }

        slice = slice->nextSlice();
        i++;
      }

      return max_thickness;
    }

    // Tests

    bool Tube::operator==(const Tube& x) const
    {
      if(typeid(*m_component) != typeid(*(x.m_component)))
        return false;

      else if(typeid(*m_component) == typeid(TubeSlice))
        return ((TubeSlice*)m_component)->TubeSlice::operator==(*((TubeSlice*)x.m_component));
      
      else
        return ((TubeNode*)m_component)->TubeNode::operator==(*((TubeNode*)x.m_component));
    }

    bool Tube::operator!=(const Tube& x) const
    {
      if(typeid(*m_component) != typeid(*(x.m_component)))
        return true;

      else if(typeid(*m_component) == typeid(TubeSlice))
        return ((TubeSlice*)m_component)->TubeSlice::operator!=(*((TubeSlice*)x.m_component));

      else
        return ((TubeNode*)m_component)->TubeNode::operator!=(*((TubeNode*)x.m_component));
    }

    bool Tube::isSubset(const Tube& x) const
    {
      StructureException::check(*m_component, *(x.m_component));

      if(typeid(*m_component) == typeid(TubeSlice))
        return ((TubeSlice*)m_component)->TubeSlice::isSubset(*((TubeSlice*)x.m_component));

      else
        return ((TubeNode*)m_component)->TubeNode::isSubset(*((TubeNode*)x.m_component));
    }

    bool Tube::isStrictSubset(const Tube& x) const
    {
      StructureException::check(*m_component, *(x.m_component));

      if(typeid(*m_component) == typeid(TubeSlice))
        return ((TubeSlice*)m_component)->TubeSlice::isStrictSubset(*((TubeSlice*)x.m_component));

      else
        return ((TubeNode*)m_component)->TubeNode::isStrictSubset(*((TubeNode*)x.m_component));
    }

    bool Tube::isEmpty() const
    {
      // todo: compliance with methods of same type: operator==, isSubset, etc.
      return m_component->isEmpty();
    }

    bool Tube::encloses(const Trajectory& x) const
    {
      // todo: compliance with methods of same type: operator==, isSubset, etc.
      return m_component->encloses(x);
    }

    // Setting values

    void Tube::set(const Interval& y)
    {
      m_component->set(y);
    }

    void Tube::set(const Interval& y, int slice_id)
    {
      m_component->getSlice(slice_id)->set(y);
    }

    void Tube::set(const Interval& y, double t)
    {
      sample(t);
      TubeSlice *slice = getSlice(t);

      if(slice->domain().lb() == t)
        slice->setInputGate(y);

      else if(slice->domain().ub() == t)
        slice->setOutputGate(y);

      else
        throw Exception("Tube::set", "inexistent gate");
    }

    void Tube::set(const Interval& y, const Interval& t)
    {
      if(t.is_degenerated())
        set(y, t.lb());

      else
      {
        sample(t.lb());
        sample(t.ub());

        int i = input2index(t.lb());
        TubeSlice *slice = getSlice(i);

        for( ; i <= input2index(t.ub()) && slice != NULL ; i++)
        {
          if((t & slice->domain()).is_degenerated())
            continue;
          slice->set(y);
          slice = slice->nextSlice();
        }
      }
    }

    void Tube::set(const Function& function)
    {
      TubeSlice *slice, *first_slice = getFirstSlice();

      // Setting envelopes
      slice = first_slice;
      while(slice != NULL)
      {
        IntervalVector iv_domain(1, slice->domain());
        slice->setEnvelope(function.eval(iv_domain));
        slice = slice->nextSlice();
      }

      // Setting gates
      slice = first_slice;
      while(slice != NULL)
      {
        IntervalVector iv_domain_input(1, slice->domain().lb());
        slice->setInputGate(function.eval(iv_domain_input));
        IntervalVector iv_domain_output(1, slice->domain().ub());
        slice->setOutputGate(function.eval(iv_domain_output));
        slice = slice->nextSlice();
      }
    }

    void Tube::setEmpty()
    {
      m_component->setEmpty();
    }

    Tube& Tube::inflate(double rad)
    {
      m_component->inflate(rad);
      return *this;
    }

    // Bisection
    
    pair<Tube,Tube> Tube::bisect(double t, float ratio) const
    {
      pair<Tube,Tube> p = make_pair(*this,*this);

      LargestFirst bisector(0., ratio);
      IntervalVector slice_domain(1, (*this)[t]);

      try
      {
        pair<IntervalVector,IntervalVector> p_codomain = bisector.bisect(slice_domain);
        p.first.set(p_codomain.first[0], t);
        p.second.set(p_codomain.second[0], t);
      }

      catch(ibex::NoBisectableVariableException&)
      {
        throw Exception("Tube::bisect", "unable to bisect, degenerated slice (ibex::NoBisectableVariableException)");
      };

      return p;
    }

    // String
    
    ostream& operator<<(ostream& str, const Tube& x)
    {
      str << "Tube " << x.domain() << "↦" << x.codomain()
          << ", " << x.nbSlices()
          << " slice" << (x.nbSlices() > 1 ? "s" : "")
          << flush;
      return str;
    }

    // Integration

    Interval Tube::integral(double t) const
    {
      return integral(Interval(t));
    }

    Interval Tube::integral(const Interval& t) const
    {
      pair<Interval,Interval> partial_ti = partialIntegral(t);
      return Interval(partial_ti.first.lb(), partial_ti.second.ub());
    }

    Interval Tube::integral(const Interval& t1, const Interval& t2) const
    {
      pair<Interval,Interval> integral_t1 = partialIntegral(t1);
      pair<Interval,Interval> integral_t2 = partialIntegral(t2);
      double lb = (integral_t2.first - integral_t1.first).lb();
      double ub = (integral_t2.second - integral_t1.second).ub();
      return Interval(min(lb, ub), max(lb, ub));
    }

    pair<Interval,Interval> Tube::partialIntegral(const Interval& t) const
    {
      checkPartialPrimitive();
      
      int index_lb = input2index(t.lb());
      int index_ub = input2index(t.ub());

      Interval integral_lb = Interval::EMPTY_SET;
      Interval integral_ub = Interval::EMPTY_SET;

      Interval intv_t_lb = getSlice(index_lb)->domain();
      Interval intv_t_ub = getSlice(index_ub)->domain();

      // Part A
      {
        pair<Interval,Interval> partial_primitive_first = getSlice(index_lb)->getPartialPrimitiveValue();
        Interval primitive_lb = Interval(partial_primitive_first.first.lb(), partial_primitive_first.second.ub());

        Interval y_first = (*this)[index_lb];
        Interval ta1 = Interval(intv_t_lb.lb(), t.lb());
        Interval ta2 = Interval(intv_t_lb.lb(), min(t.ub(), intv_t_lb.ub()));
        Interval tb1 = Interval(t.lb(), intv_t_lb.ub());
        Interval tb2 = Interval(min(t.ub(), intv_t_lb.ub()), intv_t_lb.ub());

        if(y_first.lb() < 0)
          integral_lb |= Interval(primitive_lb.lb() - y_first.lb() * tb2.diam(),
                                  primitive_lb.lb() - y_first.lb() * tb1.diam());

        else if(y_first.lb() > 0)
          integral_lb |= Interval(primitive_lb.lb() + y_first.lb() * ta1.diam(),
                                  primitive_lb.lb() + y_first.lb() * ta2.diam());

        if(y_first.ub() < 0)
          integral_ub |= Interval(primitive_lb.ub() + y_first.ub() * ta2.diam(),
                                  primitive_lb.ub() + y_first.ub() * ta1.diam());

        else if(y_first.ub() > 0)
          integral_ub |= Interval(primitive_lb.ub() - y_first.ub() * tb1.diam(),
                                  primitive_lb.ub() - y_first.ub() * tb2.diam());
      }

      // Part B
      if(index_ub - index_lb > 1)
      {
        pair<Interval,Interval> partial_primitive = m_component->getPartialPrimitiveValue(Interval(intv_t_lb.ub(), intv_t_ub.lb()));
        integral_lb |= partial_primitive.first;
        integral_ub |= partial_primitive.second;
      }

      // Part C
      if(index_lb != index_ub)
      {
        pair<Interval,Interval> partial_primitive_second = getSlice(index_ub)->getPartialPrimitiveValue();
        Interval primitive_ub = Interval(partial_primitive_second.first.lb(), partial_primitive_second.second.ub());

        Interval y_second = (*this)[index_ub];
        Interval ta = Interval(intv_t_ub.lb(), t.ub());
        Interval tb1 = intv_t_ub;
        Interval tb2 = Interval(t.ub(), intv_t_ub.ub());

        if(y_second.lb() < 0)
          integral_lb |= Interval(primitive_ub.lb() - y_second.lb() * tb2.diam(),
                                  primitive_ub.lb() - y_second.lb() * tb1.diam());

        else if(y_second.lb() > 0)
          integral_lb |= Interval(primitive_ub.lb(),
                                  primitive_ub.lb() + y_second.lb() * ta.diam());

        if(y_second.ub() < 0)
          integral_ub |= Interval(primitive_ub.ub() + y_second.ub() * ta.diam(),
                                  primitive_ub.ub());

        else if(y_second.ub() > 0)
          integral_ub |= Interval(primitive_ub.ub() - y_second.ub() * tb1.diam(),
                                  primitive_ub.ub() - y_second.ub() * tb2.diam());
      }

      return make_pair(integral_lb, integral_ub);
    }

    pair<Interval,Interval> Tube::partialIntegral(const Interval& t1, const Interval& t2) const
    {
      pair<Interval,Interval> integral_t1 = partialIntegral(t1);
      pair<Interval,Interval> integral_t2 = partialIntegral(t2);
      return make_pair((integral_t2.first - integral_t1.first),
                       (integral_t2.second - integral_t1.second));
    }

    // Contractors

    bool Tube::ctcFwd(const Tube& derivative)
    {
      CtcDeriv ctc;
      return ctc.contractFwd(*this, derivative);
    }

    bool Tube::ctcBwd(const Tube& derivative)
    {
      CtcDeriv ctc;
      return ctc.contractBwd(*this, derivative);
    }

    bool Tube::ctcFwdBwd(const Tube& derivative)
    {
      CtcDeriv ctc;
      return ctc.contract(*this, derivative);
    }

    bool Tube::ctcEval(Interval& t, Interval& z, const Tube& derivative, bool propagate)
    {
      CtcEval ctc;
      return ctc.contract(t, z, *this, derivative, propagate);
    }
      
    // Serialization

    /*
      Tube binary files structure (VERSION 2)
        - minimal storage
        - format: [tube]
                  [int_nb_trajectories]
                  [traj1]
                  [traj2]
                  ...
    */

    void Tube::serialize(const string& binary_file_name, int version_number) const
    {
      vector<Trajectory> v_trajs;
      serialize(binary_file_name, v_trajs, version_number);
    }

    void Tube::serialize(const string& binary_file_name, const Trajectory& traj, int version_number) const
    {
      vector<Trajectory> v_trajs;
      v_trajs.push_back(traj);
      serialize(binary_file_name, v_trajs, version_number);
    }
    
    void Tube::serialize(const string& binary_file_name, const vector<Trajectory>& v_trajs, int version_number) const
    {
      ofstream bin_file(binary_file_name.c_str(), ios::out | ios::binary);

      if(!bin_file.is_open())
        throw Exception("Tube::serialize()", "error while writing file \"" + binary_file_name + "\"");

      serializeTube(bin_file, *this, version_number);

      int nb_trajs = v_trajs.size();
      bin_file.write((const char*)&nb_trajs, sizeof(int));
      for(int i = 0 ; i < v_trajs.size() ; i++)
        serializeTrajectory(bin_file, v_trajs[i], version_number);

      bin_file.close();
    }

  // Protected methods

    // Integration

    void Tube::checkPartialPrimitive() const
    {
      // Warning: this method can only be called from the root (Tube class)
      // (because computation starts from 0)

      if(m_component->m_primitive_update_needed)
      {
        Interval sum_max = Interval(0);

        TubeSlice *slice = getFirstSlice();
        while(slice != NULL)
        {
          double dt = slice->domain().diam();
          Interval slice_codomain = slice->codomain();
          Interval integral_value = sum_max + slice_codomain * Interval(0., dt);
          slice->m_partial_primitive = make_pair(Interval(integral_value.lb(), integral_value.lb() + fabs(slice_codomain.lb() * dt)),
                                                 Interval(integral_value.ub() - fabs(slice_codomain.ub() * dt), integral_value.ub()));
          slice->m_primitive_update_needed = true;
          sum_max += slice_codomain * dt;
          slice = slice->nextSlice();
        }

        m_component->checkPartialPrimitive(); // updating nodes from leafs information
      }
    }

    // Serialization

    void Tube::deserialize(const string& binary_file_name, vector<Trajectory>& v_trajs)
    {
      ifstream bin_file(binary_file_name.c_str(), ios::in | ios::binary);

      if(!bin_file.is_open())
        throw Exception("Tube::deserialize()", "error while opening file \"" + binary_file_name + "\"");

      deserializeTube(bin_file, *this);

      if(!bin_file.eof())
      {
        int nb_trajs;
        bin_file.read((char*)&nb_trajs, sizeof(int));
        for(int i = 0 ; i < nb_trajs ; i++)
        {
          Trajectory traj;
          deserializeTrajectory(bin_file, traj);
          v_trajs.push_back(traj);
        }
      }

      bin_file.close();
    }
}
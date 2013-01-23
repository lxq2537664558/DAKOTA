/*  _______________________________________________________________________

    DAKOTA: Design Analysis Kit for Optimization and Terascale Applications
    Copyright (c) 2010, Sandia National Laboratories.
    This software is distributed under the GNU Lesser General Public License.
    For more information, see the README file in the top Dakota directory.
    _______________________________________________________________________ */

//- Class:        TaylorApproximation
//- Description:  Class for first- or second-order order Taylor series
//-               approximation.
//-               
//- Owner:        Mike Eldred, Sandia National Laboratories
 
#ifndef TAYLOR_APPROXIMATION_H
#define TAYLOR_APPROXIMATION_H

#include "DakotaApproximation.hpp"


namespace Dakota {

/// Derived approximation class for first- or second-order Taylor
/// series (a local approximation).

/** The TaylorApproximation class provides a local approximation based 
    on data from a single point in parameter space.  It uses a zeroth-,
    first- or second-order Taylor series expansion: f(x) = f(x_c) for
    zeroth-order, plus grad(x_c)' (x - x_c) for first- and second-order,
    and plus (x - x_c)' Hess(x_c) (x - x_c) / 2 for second-order. */

class TaylorApproximation: public Approximation
{
public:

  //
  //- Heading: Constructor and destructor
  //

  /// default constructor
  TaylorApproximation();
  /// standard constructor
  TaylorApproximation(ProblemDescDB& problem_db, size_t num_vars);
  /// alternate constructor
  TaylorApproximation(size_t num_vars, short data_order);
  /// destructor
  ~TaylorApproximation();

protected:

  //
  //- Heading: Virtual function redefinitions
  //

  int min_coefficients() const;

  //int num_constraints() const; // use default implementation

  void build();

  Real value(const Variables& vars);

  const RealVector& gradient(const Variables& vars);

  const RealSymMatrix& hessian(const Variables& vars);

private:

  //
  //- Heading: Data
  //

};


inline TaylorApproximation::TaylorApproximation()
{ }


inline TaylorApproximation::
TaylorApproximation(size_t num_vars, short data_order)
{ numVars = num_vars; buildDataOrder = data_order; }


inline TaylorApproximation::~TaylorApproximation()
{ }

} // namespace Dakota

#endif
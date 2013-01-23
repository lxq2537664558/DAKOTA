/*  _______________________________________________________________________

    DAKOTA: Design Analysis Kit for Optimization and Terascale Applications
    Copyright (c) 2010, Sandia National Laboratories.
    This software is distributed under the GNU Lesser General Public License.
    For more information, see the README file in the top Dakota directory.
    _______________________________________________________________________ */

//- Class:        Interface
//- Description:  Class implementation for abstract interface base class
//- Owner:        Michael Eldred

#include "DakotaInterface.hpp"
#include "ProblemDescDB.hpp"
#include "DakotaVariables.hpp"

#include "SysCallApplicInterface.hpp"
#ifndef _MSC_VER
#include "ForkApplicInterface.hpp"
#endif // _MSC_VER

// Direct interfaces
#ifdef DAKOTA_GRID
#include "GridApplicInterface.hpp"
#endif // DAKOTA_GRID
#ifdef DAKOTA_MATLAB
#include "MatlabInterface.hpp"
#endif // DAKOTA_MATLAB
#ifdef DAKOTA_PYTHON
#include "PythonInterface.hpp"
#endif // DAKOTA_PYTHON
#ifdef DAKOTA_SCILAB
#include "ScilabInterface.hpp"
#endif // DAKOTA_SCILAB
#include "TestDriverInterface.hpp"

#include "ApproximationInterface.hpp"

#ifdef HAVE_AMPL
#undef NO // avoid name collision from UTILIB
#include "ampl/asl.h"
#endif // HAVE_AMPL

//#define DEBUG


namespace Dakota {


/** This constructor is the one which must build the base class data for all
    inherited interfaces.  get_interface() instantiates a derived class letter
    and the derived constructor selects this base class constructor in its 
    initialization list (to avoid the recursion of the base class constructor
    calling get_interface() again).  Since this is the letter and the letter 
    IS the representation, interfaceRep is set to NULL (an uninitialized 
    pointer causes problems in ~Interface). */
Interface::Interface(BaseConstructor, const ProblemDescDB& problem_db): 
  interfaceType(problem_db.get_string("interface.type")),
  interfaceId(problem_db.get_string("interface.id")), algebraicMappings(false),
  coreMappings(true), currEvalId(0), fineGrainEvalCounters(false),
  evalIdCntr(0), newEvalIdCntr(0), evalIdRefPt(0), newEvalIdRefPt(0),
  multiProcEvalFlag(false), ieDedMasterFlag(false),
  // See base constructor in DakotaIterator.cpp for full discussion of output
  // verbosity.  Interfaces support the full granularity in verbosity.
  outputLevel(problem_db.get_short("method.output")),
  interfaceRep(NULL), referenceCount(1)
{
#ifdef DEBUG
  outputLevel = DEBUG_OUTPUT;
#endif // DEBUG
  if (outputLevel > NORMAL_OUTPUT) { // detailed evaluation reporting
    fineGrainEvalCounters = true;
    fnLabels = problem_db.get_sa("responses.labels");
    init_evaluation_counters(fnLabels.size());
  }

  // Process the algebraic_mappings file (an AMPL .nl file) to get the number
  // of variables/responses (currently, the tags are converted to index arrays
  // at evaluation time, using the passed vars and response).
  // TO DO: parallel bcast of data or very proc reads file?
  const String& ampl_file_name
    = problem_db.get_string("interface.algebraic_mappings");
  if (!ampl_file_name.empty()) {
#ifdef HAVE_AMPL
    algebraicMappings = true;
    bool hess_flag
      = (problem_db.get_string("responses.hessian_type") == "analytic");
    asl = (hess_flag) ? ASL_alloc(ASL_read_pfgh) : ASL_alloc(ASL_read_fg);
    // allow user input of either stub or stub.nl
    String stub = (ampl_file_name.ends(".nl")) ? 
      String(ampl_file_name, 0, ampl_file_name.size() - 3) : ampl_file_name;
    //std::ifstream ampl_nl(ampl_file_name);
    fint stub_str_len = stub.size();
    FILE* ampl_nl = jac0dim(stub.data(), stub_str_len);
    if (!ampl_nl) {
      Cerr << "\nError: failure opening " << ampl_file_name << std::endl;
      abort_handler(-1);
    }
    int rtn = (hess_flag) ? pfgh_read(ampl_nl, ASL_return_read_err)
                          :   fg_read(ampl_nl, ASL_return_read_err);
    if (rtn) {
      Cerr << "\nError: AMPL processing problem with " << ampl_file_name
	   << std::endl;
      abort_handler(-1);
    }

    // extract input/output tag lists
    String row = stub + ".row", col = stub + ".col";
    std::ifstream ampl_col(col);
    if (!ampl_col) {
      Cerr << "\nError: failure opening " << ampl_col << std::endl;
      abort_handler(-1);
    }
    algebraicVarTags.resize(n_var);
    for (size_t i=0; i<n_var; i++)
      ampl_col >> algebraicVarTags[i];

    std::ifstream ampl_row(row);
    if (!ampl_row) {
      Cerr << "\nError: failure opening " << ampl_row << std::endl;
      abort_handler(-1);
    }
    algebraicFnTags.resize(n_obj+n_con);
    algebraicFnTypes.resize(n_obj+n_con);
    algebraicConstraintWeights.resize(n_con);
    for (size_t i=0; i<n_obj+n_con; i++) {
      String& tag = algebraicFnTags[i];
      ampl_row >> tag;
      algebraicFnTypes[i] = algebraic_function_type(tag);
    }

#ifdef DEBUG
    Cout << ">>>>> algebraicVarTags =\n" << algebraicVarTags
	 << "\n>>>>> algebraicFnTags =\n" << algebraicFnTags
	 << "\n>>>>> algebraicFnTypes =\n" << algebraicFnTypes << std::endl;
#endif

#else
    Cerr << "\nError: algebraic_mappings not supported without the AMPL solver "
	 << "library provided with the Acro package." << std::endl;
    abort_handler(-1);
#endif // HAVE_AMPL
  }

#ifdef REFCOUNT_DEBUG
  Cout << "Interface::Interface(BaseConstructor, ProblemDescDB&) called to "
       << "build base class data for letter object." << std::endl;
#endif
}


Interface::Interface(NoDBBaseConstructor, size_t num_fns, short output_level):
  interfaceId("NO_DB_INTERFACE"), algebraicMappings(false), coreMappings(true),
  currEvalId(0), fineGrainEvalCounters(false), evalIdCntr(0), newEvalIdCntr(0),
  evalIdRefPt(0), newEvalIdRefPt(0), multiProcEvalFlag(false),
  ieDedMasterFlag(false), outputLevel(output_level), interfaceRep(NULL),
  referenceCount(1)
{
#ifdef DEBUG
  outputLevel = DEBUG_OUTPUT;
#endif // DEBUG
  if (outputLevel > NORMAL_OUTPUT) {
    fineGrainEvalCounters = true;
    init_evaluation_counters(num_fns);
  }

#ifdef REFCOUNT_DEBUG
  Cout << "Interface::Interface(NoDBBaseConstructor) called to build base "
       << "class data for letter object." << std::endl;
#endif
}


/** used in Model envelope class instantiations */
Interface::Interface(): interfaceRep(NULL), referenceCount(1)
{ }


/** Used in Model instantiation to build the envelope.  This constructor
    only needs to extract enough data to properly execute get_interface, since
    Interface::Interface(BaseConstructor, problem_db) builds the 
    actual base class data inherited by the derived interfaces. */
Interface::Interface(ProblemDescDB& problem_db): referenceCount(1)
{
#ifdef REFCOUNT_DEBUG
  Cout << "Interface::Interface(ProblemDescDB&) called to instantiate envelope."
       << std::endl;
#endif

  // Set the rep pointer to the appropriate interface type
  interfaceRep = get_interface(problem_db);
  if (!interfaceRep) // bad type or insufficient memory
    abort_handler(-1);
}


/** used only by the envelope constructor to initialize interfaceRep
    to the appropriate derived type. */
Interface* Interface::get_interface(ProblemDescDB& problem_db)
{
#ifdef REFCOUNT_DEBUG
  Cout << "Envelope instantiating letter: Getting interface " << interface_type 
       << std::endl;
#endif

  // In the case where a derived interface type has been selected for managing
  // analysis_drivers, then this determines the letter instantiation and any 
  // algebraic mappings are overlayed by ApplicationInterface.
  const String& interface_type = problem_db.get_string("interface.type");
  const String& algebraic_map_file
    = problem_db.get_string("interface.algebraic_mappings");
  if (interface_type == "system")
    return new SysCallApplicInterface(problem_db);

#ifndef _MSC_VER
  else if (interface_type == "fork")
    return new ForkApplicInterface(problem_db);
#endif // _MSC_VER

  else if (interface_type == "direct")
    return new TestDriverInterface(problem_db);
  // Note: in the case of a plug-in direct interface, this object gets replaced
  // using Interface::assign_rep().  Error checking in DirectApplicInterface::
  // derived_map_ac() should catch if this replacement fails to occur properly.

#ifdef DAKOTA_GRID
  else if (interface_type == "grid")
    return new GridApplicInterface(problem_db);
#endif

  else if (interface_type == "matlab") {
#ifdef DAKOTA_MATLAB
    return new MatlabInterface(problem_db);
#else
    Cerr << "Direct Matlab interface requested, but not enabled in this "
	 << "DAKOTA executable." << std::endl;
      return NULL;
#endif
  }
  else if (interface_type == "python") {
#ifdef DAKOTA_PYTHON
    return new PythonInterface(problem_db);
#else
    Cerr << "Direct Python interface requested, but not enabled in this "
	 << "DAKOTA executable." << std::endl;
    return NULL;
#endif
  }

  else if (interface_type == "scilab") {
#ifdef DAKOTA_SCILAB
    return new ScilabInterface(problem_db);
#else
    Cerr << "Direct Scilab interface requested, but not enabled in this "
	 << "DAKOTA executable." << std::endl;
    return NULL;
#endif
  }

  // Should not be needed since ApproximationInterface is plugged-in from
  // DataFitSurrModel using Interface::assign_rep().
  //else if (interface_type == "approximation")
  //  return new ApproximationInterface(problem_db, num_acv, num_fns);

  // In the case where only algebraic mappings are used, then no derived map
  // functionality is needed and ApplicationInterface is used for the letter.
  else if (!algebraic_map_file.empty()) {
#ifdef DEBUG
    Cout << ">>>>> new ApplicationInterface: " << algebraic_map_file
	 << std::endl;
#endif // DEBUG
    return new ApplicationInterface(problem_db);
  }

  // If the interface type is empty (e.g., from default DataInterface creation
  // in ProblemDescDB::check_input()), then ApplicationInterface is the letter.
  else if (interface_type.empty()) {
    Cerr << "Warning: empty interface type in Interface::get_interface()."
	 << std::endl;
    return new ApplicationInterface(problem_db);
  }

  else {
    Cerr << "Invalid interface: " << interface_type << std::endl;
    return NULL;
  }
}


/** Copy constructor manages sharing of interfaceRep and incrementing
    of referenceCount. */
Interface::Interface(const Interface& interface)
{
  // Increment new (no old to decrement)
  interfaceRep = interface.interfaceRep;
  if (interfaceRep) // Check for an assignment of NULL
    interfaceRep->referenceCount++;

#ifdef REFCOUNT_DEBUG
  Cout << "Interface::Interface(Interface&)" << std::endl;
  if (interfaceRep)
    Cout << "interfaceRep referenceCount = " << interfaceRep->referenceCount
	 << std::endl;
#endif
}


/** Assignment operator decrements referenceCount for old interfaceRep, assigns
    new interfaceRep, and increments referenceCount for new interfaceRep. */
Interface Interface::operator=(const Interface& interface)
{
  if (interfaceRep != interface.interfaceRep) { // normal case: old != new
    // Decrement old
    if (interfaceRep) // Check for NULL
      if ( --interfaceRep->referenceCount == 0 ) 
	delete interfaceRep;
    // Assign and increment new
    interfaceRep = interface.interfaceRep;
    if (interfaceRep) // Check for NULL
      interfaceRep->referenceCount++;
  }
  // else if assigning same rep, then do nothing since referenceCount
  // should already be correct

#ifdef REFCOUNT_DEBUG
  Cout << "Interface::operator=(Interface&)" << std::endl;
  if (interfaceRep)
    Cout << "interfaceRep referenceCount = " << interfaceRep->referenceCount
	 << std::endl;
#endif

  return *this; // calls copy constructor since returned by value
}


/** Destructor decrements referenceCount and only deletes interfaceRep
    if referenceCount is zero. */
Interface::~Interface()
{ 
  // Check for NULL pointer 
  if (interfaceRep) {
    --interfaceRep->referenceCount;
#ifdef REFCOUNT_DEBUG
    Cout << "interfaceRep referenceCount decremented to " 
         << interfaceRep->referenceCount << std::endl;
#endif
    if (interfaceRep->referenceCount == 0) {
#ifdef REFCOUNT_DEBUG
      Cout << "deleting interfaceRep" << std::endl;
#endif
      delete interfaceRep;
    }
  }
}


/** Similar to the assignment operator, the assign_rep() function
    decrements referenceCount for the old interfaceRep and assigns the
    new interfaceRep.  It is different in that it is used for
    publishing derived class letters to existing envelopes, as opposed
    to sharing representations among multiple envelopes (in particular,
    assign_rep is passed a letter object and operator= is passed an 
    envelope object).  Letter assignment supports two models as 
    governed by ref_count_incr:

    \li ref_count_incr = true (default): the incoming letter belongs to 
    another envelope.  In this case, increment the reference count in the 
    normal manner so that deallocation of the letter is handled properly.

    \li ref_count_incr = false: the incoming letter is instantiated on the
    fly and has no envelope.  This case is modeled after get_interface():
    a letter is dynamically allocated using new and passed into assign_rep,
    the letter's reference count is not incremented, and the letter is not
    remotely deleted (its memory management is passed over to the envelope). */
void Interface::assign_rep(Interface* interface_rep, bool ref_count_incr)
{
  if (interfaceRep == interface_rep) {
    // if ref_count_incr = true (rep from another envelope), do nothing as
    // referenceCount should already be correct (see also operator= logic).
    // if ref_count_incr = false (rep from on the fly), then this is an error.
    if (!ref_count_incr) {
      Cerr << "Error: duplicated interface_rep pointer assignment without "
	   << "reference count increment in Interface::assign_rep()."
	   << std::endl;
      abort_handler(-1);
    }
  }
  else { // normal case: old != new
    // Decrement old
    if (interfaceRep) // Check for NULL
      if ( --interfaceRep->referenceCount == 0 ) 
	delete interfaceRep;
    // Assign new
    interfaceRep = interface_rep;
    // Increment new
    if (interfaceRep && ref_count_incr) // Check for NULL & honor ref_count_incr
      interfaceRep->referenceCount++;
  }

#ifdef REFCOUNT_DEBUG
  Cout << "Interface::assign_rep(Interface*)" << std::endl;
  if (interfaceRep)
    Cout << "interfaceRep referenceCount = " << interfaceRep->referenceCount
	 << std::endl;
#endif
}


void Interface::fine_grained_evaluation_counters(size_t num_fns)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->fine_grained_evaluation_counters(num_fns);
  else if (!fineGrainEvalCounters) { // letter (not virtual)
    init_evaluation_counters(num_fns);
    fineGrainEvalCounters = true;
  }
}


void Interface::init_evaluation_counters(size_t num_fns)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->init_evaluation_counters(num_fns);
  else { // letter (not virtual)
    //if (fnLabels.empty()) {
    //  fnLabels.resize(num_fns);
    //  build_labels(fnLabels, "response_fn_"); // generic resp fn labels
    //}
    if (fnValCounter.empty()) {
      fnValCounter.resize(num_fns);     fnValCounter.assign(num_fns, 0);
      fnGradCounter.resize(num_fns);    fnGradCounter.assign(num_fns, 0);
      fnHessCounter.resize(num_fns);    fnHessCounter.assign(num_fns, 0);
      newFnValCounter.resize(num_fns);  newFnValCounter.assign(num_fns, 0);
      newFnGradCounter.resize(num_fns); newFnGradCounter.assign(num_fns, 0);
      newFnHessCounter.resize(num_fns); newFnHessCounter.assign(num_fns, 0);
      fnValRefPt.resize(num_fns);       fnValRefPt.assign(num_fns, 0);
      fnGradRefPt.resize(num_fns);      fnGradRefPt.assign(num_fns, 0);
      fnHessRefPt.resize(num_fns);      fnHessRefPt.assign(num_fns, 0);
      newFnValRefPt.resize(num_fns);    newFnValRefPt.assign(num_fns, 0);
      newFnGradRefPt.resize(num_fns);   newFnGradRefPt.assign(num_fns, 0);
      newFnHessRefPt.resize(num_fns);   newFnHessRefPt.assign(num_fns, 0);
    }
  }
}


void Interface::set_evaluation_reference()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->set_evaluation_reference();
  else { // letter (not virtual)

    evalIdRefPt    = evalIdCntr;
    newEvalIdRefPt = newEvalIdCntr;

    if (fineGrainEvalCounters) {
      size_t i, num_fns = fnValCounter.size();
      for (i=0; i<num_fns; i++) {
	fnValRefPt[i]     =     fnValCounter[i];
	newFnValRefPt[i]  =  newFnValCounter[i];
	fnGradRefPt[i]    =    fnGradCounter[i];
	newFnGradRefPt[i] = newFnGradCounter[i];
	fnHessRefPt[i]    =    fnHessCounter[i];
	newFnHessRefPt[i] = newFnHessCounter[i];
      }
    }
  }
}


void Interface::
print_evaluation_summary(std::ostream& s, bool minimal_header,
			 bool relative_count) const
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->print_evaluation_summary(s, minimal_header, relative_count);
  else { // letter (not virtual)

    // standard evaluation summary
    if (minimal_header) {
      if (interfaceId.empty())
	s << "  Interface evaluations";
      else
	s << "  " << interfaceId << " evaluations";
    }
    else {
      s << "<<<<< Function evaluation summary";
      if (!interfaceId.empty())
	s << " (" << interfaceId << ')';
    }
    int     fn_evals = (relative_count) ? evalIdCntr - evalIdRefPt
                                        : evalIdCntr;
    int new_fn_evals = (relative_count) ? newEvalIdCntr - newEvalIdRefPt
                                        : newEvalIdCntr;
    s << ": " << fn_evals << " total (" << new_fn_evals << " new, "
      << fn_evals - new_fn_evals << " duplicate)\n";

    // detailed evaluation summary
    if (fineGrainEvalCounters) {
      size_t i, num_fns = std::min(fnValCounter.size(), fnLabels.size());
      for (i=0; i<num_fns; i++) {
	int t_v = (relative_count) ?     fnValCounter[i] -     fnValRefPt[i]
	                           :     fnValCounter[i];
	int n_v = (relative_count) ?  newFnValCounter[i] -  newFnValRefPt[i]
	                           :  newFnValCounter[i];
	int t_g = (relative_count) ?    fnGradCounter[i] -    fnGradRefPt[i]
	                           :    fnGradCounter[i];
	int n_g = (relative_count) ? newFnGradCounter[i] - newFnGradRefPt[i]
	                           : newFnGradCounter[i];
	int t_h = (relative_count) ?    fnHessCounter[i] -    fnHessRefPt[i]
	                           :    fnHessCounter[i];
	int n_h = (relative_count) ? newFnHessCounter[i] - newFnHessRefPt[i]
	                           : newFnHessCounter[i];
	s << std::setw(15) << fnLabels[i] << ": "
	  << t_v << " val ("  << n_v << " n, " << t_v - n_v << " d), "
	  << t_g << " grad (" << n_g << " n, " << t_g - n_g << " d), "
	  << t_h << " Hess (" << n_h << " n, " << t_h - n_h << " d)\n";
      }
    }
  }
}


void Interface::map(const Variables& vars, const ActiveSet& set,
		    Response& response, const bool asynch_flag)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->map(vars, set, response, asynch_flag);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual map function.\n"
         << "No default map defined at Interface base class." << std::endl;
    abort_handler(-1);
  }
}


void Interface::
init_algebraic_mappings(const Variables& vars, const Response& response)
{
  size_t i, num_alg_vars = algebraicVarTags.size(),
    num_alg_fns = algebraicFnTags.size();

  algebraicACVIndices.resize(num_alg_vars);
  algebraicACVIds.resize(num_alg_vars);
  StringMultiArrayConstView acv_labels = vars.all_continuous_variable_labels();
  SizetMultiArrayConstView  acv_ids    = vars.all_continuous_variable_ids();
  for (i=0; i<num_alg_vars; ++i) {
    // Note: variable mappings only support continuous variables.
    //       discrete variables are not directly supported by ASL interface.
    size_t acv_index = find_index(acv_labels, algebraicVarTags[i]);
    //size_t adv_index = find_index(adv_labels, algebraicVarTags[i]);
    if (acv_index == _NPOS) { // && adv_index == _NPOS) {
      Cerr << "\nError: AMPL column label does not exist in DAKOTA "
	   << "continuous variable descriptors.\n" << std::endl;
      abort_handler(-1);
    }
    else {
      algebraicACVIndices[i] = acv_index;
      //algebraicADVIndices[i] = adv_index;
      algebraicACVIds[i] = acv_ids[acv_index];
    }
  }

  algebraicFnIndices.resize(num_alg_fns);
  const StringArray& fn_labels = response.function_labels();
  for (size_t i=0; i<num_alg_fns; ++i) {
    size_t fn_index = Pecos::find_index(fn_labels, algebraicFnTags[i]);
    if (fn_index == _NPOS) {
      Cerr << "\nError: AMPL row label does not exist in DAKOTA response "
	   << "descriptors.\n" << std::endl;
      abort_handler(-1);
    }
    else
      algebraicFnIndices[i] = fn_index;
  }
}


void Interface::
asv_mapping(const ActiveSet& total_set, ActiveSet& algebraic_set,
	    ActiveSet& core_set)
{
  const ShortArray& total_asv = total_set.request_vector();
  const SizetArray& total_dvv = total_set.derivative_vector();

  // algebraic_asv/dvv:

  // the algebraic active set is defined over reduced algebraic function
  // and variable spaces, rather than the original spaces.  This simplifies
  // algebraic_mappings() and allows direct copies of data from AMPL.
  size_t i, num_alg_fns = algebraicFnTags.size(),
    num_alg_vars = algebraicVarTags.size();
  ShortArray algebraic_asv(num_alg_fns);
  SizetArray algebraic_dvv(num_alg_vars);
  for (i=0; i<num_alg_fns; i++) // map total_asv to algebraic_asv
    algebraic_asv[i] = total_asv[algebraicFnIndices[i]];

  algebraic_set.request_vector(algebraic_asv);
  algebraic_set.derivative_vector(algebraic_dvv);
  algebraic_set.derivative_start_value(1);

  // core_asv/dvv:

  // for now, core_asv is the same as total_asv, since there is no mechanism
  // yet to determine if the algebraic_mapping portion is the complete
  // definition (for which core_asv requests could be turned off).
  core_set.request_vector(total_asv);
  core_set.derivative_vector(total_dvv);
}


void Interface::
algebraic_mappings(const Variables& vars, const ActiveSet& algebraic_set,
		   Response& algebraic_response)
{
#ifdef HAVE_AMPL
  // make sure cur_ASL is pointing to the ASL of this interface
  // this is important for problems with multiple interfaces
  set_cur_ASL(asl);
  const ShortArray& algebraic_asv = algebraic_set.request_vector();
  const SizetArray& algebraic_dvv = algebraic_set.derivative_vector();
  size_t i, num_alg_fns = algebraic_asv.size(),
    num_alg_vars = algebraic_dvv.size();
  bool grad_flag = false, hess_flag = false;
  for (i=0; i<num_alg_fns; ++i) {
    if (algebraic_asv[i] & 2)
      grad_flag = true;
    if (algebraic_asv[i] & 4)
      hess_flag = true;
  }

  // dak_a_c_vars (DAKOTA space) -> nl_vars (reduced AMPL space)
  const RealVector& dak_a_c_vars = vars.all_continuous_variables();
  //IntVector  dak_a_d_vars = vars.all_discrete_variables();
  Real* nl_vars = new Real [num_alg_vars];
  for (i=0; i<num_alg_vars; i++)
    nl_vars[i] = dak_a_c_vars[algebraicACVIndices[i]];

  // nl_vars -> algebraic_response
  algebraic_response.reset_inactive(); // zero inactive data
  Real fn_val; RealVector fn_grad; RealSymMatrix fn_hess;
  fint err = 0;
  for (i=0; i<num_alg_fns; i++) {
    // nl_vars -> response fns via AMPL
    if (algebraic_asv[i] & 1) {
      if (algebraicFnTypes[i] > 0)
	fn_val = objval(algebraicFnTypes[i]-1, nl_vars, &err);
      else
	fn_val = conival(-1-algebraicFnTypes[i], nl_vars, &err);
      if (err) {
	Cerr << "\nError: AMPL processing failure in objval().\n" << std::endl;
	abort_handler(-1);
      }
      algebraic_response.function_value(fn_val, i);
    }
    // nl_vars -> response grads via AMPL
    if (algebraic_asv[i] & 6) { // need grad for Hessian
      fn_grad = algebraic_response.function_gradient_view(i);
      if (algebraicFnTypes[i] > 0)
	objgrd(algebraicFnTypes[i]-1, nl_vars, fn_grad.values(), &err);
      else
	congrd(-1-algebraicFnTypes[i], nl_vars, fn_grad.values(), &err);
      if (err) {
	Cerr << "\nError: AMPL processing failure in objgrad().\n" << std::endl;
	abort_handler(-1);
      }
    }
    // nl_vars -> response Hessians via AMPL
    if (algebraic_asv[i] & 4) {
      fn_hess = algebraic_response.function_hessian_view(i);
      // the fullhess calls must follow corresp call to objgrad/congrad
      if (algebraicFnTypes[i] > 0)
	fullhes(fn_hess.values(), num_alg_vars, algebraicFnTypes[i]-1,
		NULL, NULL);
      else {
	algebraicConstraintWeights.assign(algebraicConstraintWeights.size(), 0);
	algebraicConstraintWeights[-1-algebraicFnTypes[i]] = 1;
	fullhes(fn_hess.values(), num_alg_vars, num_alg_vars, NULL, 
		&algebraicConstraintWeights[0]); 
      }
    }
  }
  delete [] nl_vars;
  algebraic_response.function_labels(algebraicFnTags);
#ifdef DEBUG
  Cout << ">>>>> algebraic_response.fn_labels\n"
       << algebraic_response.function_labels() << std::endl;
#endif // DEBUG

  if (outputLevel > NORMAL_OUTPUT)
    Cout << "Algebraic mapping applied.\n";
#endif // HAVE_AMPL
}


void Interface::
response_mapping(const Response& algebraic_response,
		 const Response& core_response, Response& total_response)
{
  const ShortArray& total_asv = total_response.active_set_request_vector();
  const SizetArray& total_dvv = total_response.active_set_derivative_vector();
  size_t i, j, k, num_total_fns = total_asv.size(),
    num_total_vars = total_dvv.size();
  bool grad_flag = false, hess_flag = false;
  for (i=0; i<num_total_fns; ++i) {
    if (total_asv[i] & 2)
      grad_flag = true;
    if (total_asv[i] & 4)
      hess_flag = true;
  }

  // core_response contributions to total_response:

  total_response.reset_inactive();
  if (coreMappings) {
    const ShortArray& core_asv = core_response.active_set_request_vector();
    size_t num_core_fns = core_asv.size();
    for (i=0; i<num_core_fns; ++i) {
      if (core_asv[i] & 1)
	total_response.function_value(core_response.function_value(i), i);
      if (core_asv[i] & 2)
	total_response.function_gradient(
	  core_response.function_gradient_view(i), i);
      if (core_asv[i] & 4)
	total_response.function_hessian(core_response.function_hessian(i), i);
    }
  }

  // algebraic_response contributions to total_response:

  const ShortArray& algebraic_asv
    = algebraic_response.active_set_request_vector();
  size_t num_alg_fns = algebraic_asv.size(),
    num_alg_vars = algebraic_response.active_set_derivative_vector().size();
  if (num_alg_fns > num_total_fns) {
    Cerr << "Error: response size mismatch in Interface::response_mapping()."
	 << std::endl;
    abort_handler(-1);
  }
  if ( (grad_flag || hess_flag) && num_alg_vars > num_total_vars) {
    Cerr << "Error: derivative variables size mismatch in Interface::"
         << "response_mapping()." << std::endl;
    abort_handler(-1);
  }
  SizetArray algebraic_dvv_indices;
  if (grad_flag || hess_flag) {
    algebraic_dvv_indices.resize(num_alg_vars);
    using Pecos::find_index;
    for (i=0; i<num_alg_vars; ++i)
      algebraic_dvv_indices[i] = find_index(total_dvv, algebraicACVIds[i]);
      // Note: _NPOS return is handled below
  }
  // augment total_response
  const RealVector& algebraic_fn_vals = algebraic_response.function_values();
  const RealMatrix& algebraic_fn_grads
    = algebraic_response.function_gradients();
  const RealSymMatrixArray& algebraic_fn_hessians
    = algebraic_response.function_hessians();
  RealVector total_fn_vals = total_response.function_values_view();
  for (i=0; i<num_alg_fns; ++i) {
    size_t fn_index = algebraicFnIndices[i];
    if (algebraic_asv[i] & 1)
      total_fn_vals[fn_index] += algebraic_fn_vals[i];
    if (algebraic_asv[i] & 2) {
      const Real* algebraic_fn_grad = algebraic_fn_grads[i];
      RealVector total_fn_grad
	= total_response.function_gradient_view(fn_index);
      for (j=0; j<num_alg_vars; j++) {
	size_t dvv_index = algebraic_dvv_indices[j];
	if (dvv_index != _NPOS)
	  total_fn_grad[dvv_index] += algebraic_fn_grad[j];
      }
    }
    if (algebraic_asv[i] & 4) {
      const RealSymMatrix& algebraic_fn_hess = algebraic_fn_hessians[i];
      RealSymMatrix total_fn_hess
	= total_response.function_hessian_view(fn_index);
      for (j=0; j<num_alg_vars; ++j) {
	size_t dvv_index_j = algebraic_dvv_indices[j];
	if (dvv_index_j != _NPOS) {
	  for (k=0; k<=j; ++k) {
	    size_t dvv_index_k = algebraic_dvv_indices[k];
	    if (dvv_index_k != _NPOS)
	      total_fn_hess(dvv_index_j,dvv_index_k) +=	algebraic_fn_hess(j,k);
	  }
	}
      }
    }
  }

  // output response sets:

  if (outputLevel == DEBUG_OUTPUT) {
    if (coreMappings) Cout << "core_response:\n" << core_response;
    Cout << "algebraic_response:\n" << algebraic_response
	 << "total_response:\n"     << total_response << '\n';
  }
}


int Interface::algebraic_function_type(String functionTag) 
{
#ifdef HAVE_AMPL
  size_t i;
  for (i=0; i<n_obj; i++)
    if (functionTag.contains(obj_name(i)))
      return i+1;
  for (i=0; i<n_con; i++)
    if (functionTag.contains(con_name(i)))
      return -(i+1);

  Cerr << "Error: No function type available for \'" << functionTag << "\' " 
       << "via algebraic_mappings interface." << std::endl;
  abort_handler(-1);
#else
  return 0;
#endif // HAVE_AMPL
}

const IntResponseMap& Interface::synch()
{
  if (!interfaceRep) { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual synch function.\n"
         << "No default synch defined at Interface base class." << std::endl;
    abort_handler(-1);
  }

  return interfaceRep->synch();
}


const IntResponseMap& Interface::synch_nowait()
{
  if (!interfaceRep) { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual synch_nowait "
	 << "function.\nNo default synch_nowait defined at Interface base "
	 << "class." << std::endl;
    abort_handler(-1);
  }

  return interfaceRep->synch_nowait();
}


void Interface::serve_evaluations()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->serve_evaluations();
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual serve_evaluations "
	 << "function.\nNo default serve_evaluations defined at Interface"
	 << " base class." << std::endl;
    abort_handler(-1);
  }
}


void Interface::stop_evaluation_servers()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->stop_evaluation_servers();
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual stop_evaluation_"
	 << "servers fn.\nNo default stop_evaluation_servers defined at "
	 << "Interface base class." << std::endl;
    abort_handler(-1);
  }
}


void Interface::init_communicators(const IntArray& message_lengths,
				   int max_iterator_concurrency)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->init_communicators(message_lengths,max_iterator_concurrency);
  else { // letter lacking redefinition of virtual fn.
    // ApproximationInterfaces: do nothing
  }
}


void Interface::set_communicators(const IntArray& message_lengths,
				  int max_iterator_concurrency)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->set_communicators(message_lengths, max_iterator_concurrency);
  else { // letter lacking redefinition of virtual fn.
    // ApproximationInterfaces: do nothing
  }
}


void Interface::free_communicators()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->free_communicators();
  else { // letter lacking redefinition of virtual fn.
    // ApproximationInterfaces: do nothing
  }
}


void Interface::init_serial()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->init_serial();
  else { // letter lacking redefinition of virtual fn.
    // ApproximationInterfaces: do nothing
  }
}


int Interface::asynch_local_evaluation_concurrency() const
{
  if (interfaceRep) // envelope fwd to letter
    return interfaceRep->asynch_local_evaluation_concurrency();
  else // letter lacking redefinition of virtual fn.
    return 0; // default (redefined only for ApplicationInterfaces)
}


String Interface::interface_synchronization() const
{
  if (interfaceRep) // envelope fwd to letter
    return interfaceRep->interface_synchronization(); // ApplicationInterfaces
  else // letter lacking redefinition of virtual fn.
    return String("synchronous"); // default (ApproximationInterfaces)
}


int Interface::minimum_points(bool constraint_flag) const
{
  if (interfaceRep) // envelope fwd to letter
    return interfaceRep->minimum_points(constraint_flag);
  else // letter lacking redefinition of virtual fn.
    return 0; // default (currently redefined only for ApproximationInterfaces)
}


int Interface::recommended_points(bool constraint_flag) const
{
  if (interfaceRep) // envelope fwd to letter
    return interfaceRep->recommended_points(constraint_flag);
  else // letter lacking redefinition of virtual fn.
    return 0; // default (currently redefined only for ApproximationInterfaces)
}


void Interface::approximation_function_indices(const IntSet& approx_fn_indices)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->approximation_function_indices(approx_fn_indices);
  // else: default implementation is no-op
}


void Interface::
update_approximation(const Variables& vars, const IntResponsePair& response_pr)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->update_approximation(vars, response_pr);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual update_approximation"
         << "(Variables, IntResponsePair) function.\n       This interface "
	 << "does not support approximation updating." << std::endl;
    abort_handler(-1);
  }
}


void Interface::
update_approximation(const RealMatrix& samples, const IntResponseMap& resp_map)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->update_approximation(samples, resp_map);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual update_approximation"
         << "(RealMatrix, IntResponseMap) function.\n       This interface "
	 << "does not support approximation updating." << std::endl;
    abort_handler(-1);
  }
}


void Interface::
update_approximation(const VariablesArray& vars_array,
		     const IntResponseMap& resp_map)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->update_approximation(vars_array, resp_map);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual update_approximation"
         << "(VariablesArray, IntResponseMap) function.\n       This interface "
	 << "does not support approximation updating." << std::endl;
    abort_handler(-1);
  }
}


void Interface::
append_approximation(const Variables& vars, const IntResponsePair& response_pr)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->append_approximation(vars, response_pr);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual append_approximation"
	 << "(Variables, IntResponsePair) function.\n       This interface "
	 << "does not support approximation appending." << std::endl;
    abort_handler(-1);
  }
}


void Interface::
append_approximation(const RealMatrix& samples, const IntResponseMap& resp_map)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->append_approximation(samples, resp_map);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual append_approximation"
         << "(RealMatrix, IntResponseMap) function.\n       This interface "
	 << "does not support approximation appending." << std::endl;
    abort_handler(-1);
  }
}


void Interface::
append_approximation(const VariablesArray& vars_array,
		     const IntResponseMap& resp_map)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->append_approximation(vars_array, resp_map);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual append_approximation"
         << "(VariablesArray, IntResponseMap) function.\n       This interface "
	 << "does not support approximation appending." << std::endl;
    abort_handler(-1);
  }
}


void Interface::
build_approximation(const RealVector&  c_l_bnds, const RealVector&  c_u_bnds,
		    const IntVector&  di_l_bnds, const IntVector&  di_u_bnds,
		    const RealVector& dr_l_bnds, const RealVector& dr_u_bnds)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->build_approximation(c_l_bnds, c_u_bnds, di_l_bnds, di_u_bnds,
				      dr_l_bnds, dr_u_bnds);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual build_approximation"
         << "() function.\n       This interface does not support "
	 << "approximations." << std::endl;
    abort_handler(-1);
  }
}


void Interface::
rebuild_approximation(const BoolDeque& rebuild_deque)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->rebuild_approximation(rebuild_deque);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual rebuild_"
	 << "approximation() function.\n       This interface does not "
	 << "support approximations." << std::endl;
    abort_handler(-1);
  }
}


void Interface::pop_approximation(bool save_surr_data)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->pop_approximation(save_surr_data);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual pop_approximation"
	 << "(bool)\n       function. This interface does not support "
	 << "approximation\n       data removal." << std::endl;
    abort_handler(-1);
  }
}


void Interface::restore_approximation()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->restore_approximation();
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual restore_"
	 << "approximation() function.\n       This interface does not "
	 << "support approximation restoration." << std::endl;
    abort_handler(-1);
  }
}


bool Interface::restore_available()
{
  if (!interfaceRep) { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual restore_"
	 << "available() function.\n       This interface does not "
	 << "support approximation restoration queries." << std::endl;
    abort_handler(-1);
  }
  return interfaceRep->restore_available();
}


void Interface::finalize_approximation()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->finalize_approximation();
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual finalize_"
	 << "approximation() function.\n       This interface does not "
	 << "support approximation finalization." << std::endl;
    abort_handler(-1);
  }
}


void Interface::store_approximation()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->store_approximation();
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual store_"
	 << "approximation() function.\n       This interface does not "
	 << "support approximation storage." << std::endl;
    abort_handler(-1);
  }
}


void Interface::combine_approximation(short corr_type)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->combine_approximation(corr_type);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual combine_"
	 << "approximation() function.\n       This interface does not "
	 << "support approximation combination." << std::endl;
    abort_handler(-1);
  }
}


void Interface::clear_current()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->clear_current();
  else { // letter lacking redefinition of virtual fn.
    // ApplicationInterfaces: do nothing
  }
}


void Interface::clear_all()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->clear_all();
  else { // letter lacking redefinition of virtual fn.
    // ApplicationInterfaces: do nothing
  }
}


void Interface::clear_saved()
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->clear_saved();
  else { // letter lacking redefinition of virtual fn.
    // ApplicationInterfaces: do nothing
  }
}


std::vector<Approximation>& Interface::approximations()
{
  if (!interfaceRep) { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual approximations() "
	 << "function.\n       This interface does not support approximations."
	 << std::endl;
    abort_handler(-1);
  }
  
  // envelope fwd to letter
  return interfaceRep->approximations();
}


const Pecos::SurrogateData& Interface::approximation_data(size_t index)
{
  if (!interfaceRep) { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual approximation_data "
	 << "function.\n       This interface does not support approximations."
	 << std::endl;
    abort_handler(-1);
  }
  
  // envelope fwd to letter
  return interfaceRep->approximation_data(index);
}


const RealVectorArray& Interface::approximation_coefficients()
{
  if (!interfaceRep) { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual approximation_"
	 << "coefficients function.\n       This interface does not support "
         << "approximations." << std::endl;
    abort_handler(-1);
  }
  
  // envelope fwd to letter
  return interfaceRep->approximation_coefficients();
}


void Interface::approximation_coefficients(const RealVectorArray& approx_coeffs)
{
  if (interfaceRep) // envelope fwd to letter
    interfaceRep->approximation_coefficients(approx_coeffs);
  else { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual approximation_"
	 << "coefficients function.\n       This interface does not support "
         << "approximations." << std::endl;
    abort_handler(-1);
  }
}


const RealVector& Interface::approximation_variances(const Variables& vars)
{
  if (!interfaceRep) { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual approximation_"
	 << "variances function.\n       This interface does not support "
         << "approximations." << std::endl;
    abort_handler(-1);
  }
  
  // envelope fwd to letter
  return interfaceRep->approximation_variances(vars);
}


const StringArray& Interface::analysis_drivers() const
{
  if (!interfaceRep) { // letter lacking redefinition of virtual fn.
    Cerr << "Error: Letter lacking redefinition of virtual analysis_drivers "
	 << "function." << std::endl;
    abort_handler(-1);
  }

  // envelope fwd to letter
  return interfaceRep->analysis_drivers();
}


const AnalysisCode* Interface::analysis_code() const
{
  if (interfaceRep) // envelope fwd to letter
    return interfaceRep->analysis_code();
  // default: virtual fn not defined by derived class
  return NULL;
}


bool Interface::evaluation_cache() const
{
  if (interfaceRep)
    return interfaceRep->evaluation_cache();
  else // letter lacking redefinition of virtual fn.
    return false; // default
}

} // namespace Dakota
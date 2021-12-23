/* -------------------------------------------------------------------------
  Process Kernels

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Daniil Svyatsky
*/

#include "PK_Physical.hh"
#include "State.hh"
#include "TreeVector.hh"

namespace Amanzi {

// -----------------------------------------------------------------------------
// Transfer operators -- copies ONLY pointers
// -----------------------------------------------------------------------------
void PK_Physical::State_to_Solution(const Teuchos::RCP<State>& S,
                                    TreeVector& solution) {
  solution.SetData(S->GetPtrW<CompositeVector>(key_, key_));
}


void PK_Physical::Solution_to_State(TreeVector& solution,
                                    const Teuchos::RCP<State>& S) {
  AMANZI_ASSERT(solution.Data() == S->GetPtr<TreeVector>(key_));
  // S->SetData(key_, name_, solution->Data());
  // solution_evaluator_->SetFieldAsChanged();
}


void PK_Physical::Solution_to_State(const TreeVector& solution,
                                    const Teuchos::RCP<State>& S) {
  AMANZI_ASSERT(solution.Data() == S->GetPtr<TreeVector>(key_));
  // S->SetData(key_, name_, solution->Data());
  // solution_evaluator_->SetFieldAsChanged();
}
  

// -----------------------------------------------------------------------------
// Experimental approach -- we must pull out S_next_'s solution_evaluator_ to
// stay current for ChangedSolution()
// -----------------------------------------------------------------------------
void PK_Physical::set_states(const Teuchos::RCP<State>& S,
                             const Teuchos::RCP<State>& S_inter,
                             const Teuchos::RCP<State>& S_next) {
  S_ = S;
  S_inter_ = S_inter;
  S_next_ = S_next;
    
  // Get the evaluator and mark it as changed.
  // Note that this is necessary because we need this to point at the
  // evaluator in S_next_, not the one which we created in S_.
  auto fm = S_next->GetEvaluatorPtr(key_);
#if ENABLE_DBC
  solution_evaluator_ = Teuchos::rcp_dynamic_cast<EvaluatorPrimary<TreeVector> >(fm);
  AMANZI_ASSERT(solution_evaluator_ != Teuchos::null);
#else
  solution_evaluator_ = Teuchos::rcp_static_cast<EvaluatorPrimary<TreeVector> >(fm);
#endif

  solution_evaluator_->SetChanged();
}

} // namespace Amanzi


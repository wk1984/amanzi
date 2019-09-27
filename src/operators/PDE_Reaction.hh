/*
  Operators

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)

  Reaction operator. 
*/

#ifndef AMANZI_OPERATOR_PDE_REACTION_HH_
#define AMANZI_OPERATOR_PDE_REACTION_HH_

#include <string>

#include "Epetra_MultiVector.h"

// Amanzi
#include "BilinearForm.hh"
#include "VectorObjects.hh"

// Amanzi::Operators
#include "PDE_HelperDiscretization.hh"
#include "Schema.hh"

namespace Amanzi {
namespace Operators {

class PDE_Reaction : public PDE_HelperDiscretization {
 public:
  PDE_Reaction(Teuchos::ParameterList& plist, Teuchos::RCP<Operator> global_op) :
      K_(Teuchos::null),
      PDE_HelperDiscretization(global_op),
      static_matrices_initialized_(false) {
    InitReaction_(plist);
  }

  PDE_Reaction(Teuchos::ParameterList& plist, Teuchos::RCP<const AmanziMesh::Mesh> mesh) : 
      K_(Teuchos::null),
      PDE_HelperDiscretization(mesh),
      static_matrices_initialized_(false) {
    InitReaction_(plist);
  }

  // required members 
  // -- setup
  void SetupScalar(const Teuchos::RCP<Epetra_MultiVector>& K) { K_ = K; }
  void SetupPoly(const Teuchos::RCP<std::vector<WhetStone::Polynomial> >& K) { poly_ = K; }
  void Setup(const Teuchos::RCP<const std::vector<WhetStone::SpaceTimePolynomial> >& K, bool reset) {
    poly_st_ = K;
    if (!static_matrices_initialized_ || reset) CreateStaticMatrices_();
  }

  // -- generate a linearized operator 
  using PDE_HelperDiscretization::UpdateMatrices;
  virtual void UpdateMatrices(const Teuchos::Ptr<const CompositeVector>& u,
                              const Teuchos::Ptr<const CompositeVector>& p) override;
  // -- new interface for pre-computed data  
  void UpdateMatrices(double t);

  // -- flux calculation has yet no meaning for this operator
  virtual void UpdateFlux(const Teuchos::Ptr<const CompositeVector>& p,
                          const Teuchos::Ptr<CompositeVector>& u) override {};
  
  // boundary conditions
  virtual void ApplyBCs(bool primary, bool eliminate, bool essential_eqn) override;

 private:
  void InitReaction_(Teuchos::ParameterList& plist);
  void CreateStaticMatrices_();

 protected:
  Teuchos::RCP<const Epetra_MultiVector> K_;
  Teuchos::RCP<const std::vector<WhetStone::Polynomial> > poly_;
  Teuchos::RCP<const std::vector<WhetStone::SpaceTimePolynomial> > poly_st_;

  Teuchos::RCP<WhetStone::BilinearForm> mfd_;

 private:
  Schema global_schema_col_, global_schema_row_;
  Schema local_schema_col_, local_schema_row_;

  bool static_matrices_initialized_;
  std::vector<std::vector<WhetStone::DenseMatrix> > static_matrices_;
};

}  // namespace Operators
}  // namespace Amanzi

#endif


/*
This is the flow component of the Amanzi code.  

Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
Amanzi is released under the three-clause BSD License. 
The terms of use and "as is" disclaimer for this license are 
provided in the top-level COPYRIGHT file.

Authors: Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)

The class provides a different implementation of solvers than in 
the base class. In particular, Lagrange multipliers and cell-based 
pressures are strongly coupled and form the global system.
*/

#ifndef AMANZI_MATRIX_MFD_PLAMBDA_HH_
#define AMANZI_MATRIX_MFD_PLAMBDA_HH_

#include <strings.h>

#include "Ifpack.h" 
#include "Teuchos_RCP.hpp"

#include "Matrix_MFD.hh"


namespace Amanzi {
namespace AmanziFlow {

class Matrix_MFD_PLambda : public Matrix_MFD {
 public:
  Matrix_MFD_PLambda(Teuchos::RCP<Flow_State> FS, const Epetra_Map& map) : Matrix_MFD(FS, map) {};
  ~Matrix_MFD_PLambda() {};

  // override main methods of the base class
  void SymbolicAssembleGlobalMatrices(const Epetra_Map& super_map);
  void AssembleGlobalMatrices();
  void AssembleSchurComplement(std::vector<int>& bc_model, std::vector<bc_tuple>& bc_values);

  int Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;
  int ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

  const char* Label() const { return strdup("Matrix MFD_PLambda"); }

  // access methods
  std::vector<Teuchos::SerialDenseMatrix<int, double> >& Acc_faces() { return Acc_faces_; }

 protected:
  Teuchos::RCP<Epetra_FECrsMatrix> APLambda_;
  std::vector<Teuchos::SerialDenseMatrix<int, double> > Acc_faces_;

 private:
  void operator=(const Matrix_MFD_PLambda& matrix);
};

}  // namespace AmanziFlow
}  // namespace Amanzi

#endif

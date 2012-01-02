/*
This is the flow component of the Amanzi code. 
License: BSD
Authors: Neil Carlson (version 1) 
         Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)
*/

#ifndef __DARCY_PK_HPP__
#define __DARCY_PK_HPP__

#include "Teuchos_RCP.hpp"

#include "Epetra_Vector.h"
#include "AztecOO.h"

#include "Mesh.hh"
#include "Point.hh"
#include "boundary-function.hh"
#include "tensor.hpp"

#include "Flow_PK.hpp"
#include "Flow_State.hpp"
#include "Matrix_MFD.hpp"

namespace Amanzi {
namespace AmanziFlow {

class Darcy_PK : public Flow_PK {
 public:
  Darcy_PK(Teuchos::RCP<Teuchos::ParameterList> dp_list_, Teuchos::RCP<Flow_State> FS_MPC);
  ~Darcy_PK() { delete super_map_, solver, matrix, preconditioner, bc_pressure, bc_head, bc_flux; }

  // main methods
  void Init(Matrix_MFD* matrix_ = NULL, Matrix_MFD* preconditioner_ = NULL);

  int advance(double dT) {}; 
  int advance_to_steady_state();
  void commit_state() {};

  // other main methods
  void process_parameter_list();
  void populate_absolute_permeability_tensor(std::vector<WhetStone::Tensor>& K);
  void addGravityFluxes_MFD(Matrix_MFD* matrix);

  // control methods
  void print_statistics() const;

  // access methods
  Flow_State& get_FS() { return *FS; }
  Epetra_Vector& get_solution_cells() { return *solution_cells; }
  Epetra_Vector& get_solution_faces() { return *solution_faces; }
  AmanziGeometry::Point& get_gravity() { return gravity; }
  double get_rho() { return rho; }
  double get_mu() { return mu; }

 private:
  Teuchos::RCP<Teuchos::ParameterList> dp_list;

  Teuchos::RCP<Flow_State> FS;
  AmanziGeometry::Point gravity;
  double rho, mu;

  Teuchos::RCP<AmanziMesh::Mesh> mesh_;
  Epetra_Map *super_map_;
  int dim;

  Teuchos::RCP<Epetra_Import> cell_importer_;  // parallel communicators
  Teuchos::RCP<Epetra_Import> face_importer_;

  AztecOO* solver;
  Matrix_MFD* matrix;
  Matrix_MFD* preconditioner;

  int num_itrs, max_itrs;  // numbers of linear solver iterations
  double err_tol, residual;  // errors in linear solver

  Teuchos::RCP<Epetra_Vector> solution;  // global solution
  Teuchos::RCP<Epetra_Vector> solution_cells;  // cell-based pressures
  Teuchos::RCP<Epetra_Vector> solution_faces;  // face-base pressures
  Teuchos::RCP<Epetra_Vector> rhs;  // It has same size as solution.
  Teuchos::RCP<Epetra_Vector> rhs_faces;

  BoundaryFunction *bc_pressure;  // Pressure Dirichlet b.c., excluding static head
  BoundaryFunction *bc_head;  // Static pressure head b.c.; also Dirichlet-type
  BoundaryFunction *bc_flux;  // Outward mass flux b.c.
  std::vector<int> bc_markers;  // Used faces marked with boundary conditions
  std::vector<double> bc_values;

  std::vector<WhetStone::Tensor> K;  // tensor of absolute permeability
  Teuchos::RCP<Epetra_IntVector> upwind_cell, downwind_cell;
};

}  // namespace AmanziFlow
}  // namespace Amanzi

#endif


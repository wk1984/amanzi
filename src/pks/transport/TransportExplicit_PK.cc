/*
  Transport PK 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Konstantin Lipnikov (lipnikov@lanl.gov)
           Daniil Svyatsky (dasvyat@lanl.gov)

  Implementation of explicit time integration algorithms.
*/

#include <algorithm>
#include <vector>

#include "Epetra_Vector.h"
#include "Epetra_IntVector.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Import.h"
#include "Teuchos_RCP.hpp"

// Amanzi
#include "BCs.hh"
#include "errors.hh"
#include "Explicit_TI_RK.hh"
#include "Mesh.hh"
#include "PDE_AdvectionUpwind.hh"
#include "PDE_AdvectionUpwindFracturedMatrix.hh"
#include "PDE_AdvectionUpwindDFN.hh"
#include "PDE_Diffusion.hh"
#include "PK_DomainFunctionFactory.hh"
#include "PK_Utils.hh"
#include "WhetStoneDefs.hh"

// amanzi::Transport
#include "MultiscaleTransportPorosityFactory.hh"
#include "TransportExplicit_PK.hh"
#include "TransportBoundaryFunction_Alquimia.hh"
#include "TransportDomainFunction.hh"
#include "TransportSourceFunction_Alquimia.hh"

namespace Amanzi {
namespace Transport {

/* ******************************************************************
* New constructor compatible with new MPC framework.
****************************************************************** */
TransportExplicit_PK::TransportExplicit_PK(Teuchos::ParameterList& pk_tree,
                           const Teuchos::RCP<Teuchos::ParameterList>& glist,
                           const Teuchos::RCP<State>& S,
                           const Teuchos::RCP<TreeVector>& soln) :
    Transport_PK(pk_tree, glist, S, soln)
{
}


/* ******************************************************************
* Simple constructor for unit tests.
****************************************************************** */
TransportExplicit_PK::TransportExplicit_PK(const Teuchos::RCP<Teuchos::ParameterList>& glist,
                                           Teuchos::RCP<State> S, 
                                           const std::string& pk_list_name,
                                           std::vector<std::string>& component_names) :
  Transport_PK(glist, S, pk_list_name, component_names)
{  
}


/* ******************************************************************* 
* Advance each component independently due to different field
* reconstructions. This routine uses custom implementation of the 
* second-order predictor-corrector time integration scheme. 
******************************************************************* */
void TransportExplicit_PK::AdvanceSecondOrderUpwindRK2(double dt_cycle)
{
  dt_ = dt_cycle;  // overwrite the maximum stable transport step
  mass_solutes_source_.assign(num_aqueous + num_gaseous, 0.0);

  // work memory
  const Epetra_Map& cmap_wghost = mesh_->cell_map(true);
  Epetra_Vector f_component(cmap_wghost);

  // distribute old vector of concentrations
  S_->Get<CompositeVector>(tcc_key_).ScatterMasterToGhosted("cell");
  Epetra_MultiVector& tcc_prev = *tcc->ViewComponent("cell", true);
  Epetra_MultiVector& tcc_next = *tcc_tmp->ViewComponent("cell", true);

  Epetra_Vector ws_ratio(Copy, *ws_start, 0);
  for (int c = 0; c < ncells_owned; c++) ws_ratio[c] /= (*ws_end)[0][c];

  // We advect only aqueous components.
  int num_advect = num_aqueous;

  // predictor step
  for (int i = 0; i < num_advect; i++) {
    current_component_ = i;  // needed by BJ 

    double T = t_physics_;
    Epetra_Vector*& component = tcc_prev(i);
    DudtOld(T, *component, f_component);

    for (int c = 0; c < ncells_owned; c++) {
      tcc_next[i][c] = (tcc_prev[i][c] + dt_ * f_component[c]) * ws_ratio[c];
    }
  }

  tcc_tmp->ScatterMasterToGhosted("cell");

  // corrector step
  for (int i = 0; i < num_advect; i++) {
    current_component_ = i;  // needed in BJ for BCs

    double T = t_physics_;
    Epetra_Vector*& component = tcc_next(i);
    DudtOld(T, *component, f_component);

    for (int c = 0; c < ncells_owned; c++) {
      double value = (tcc_prev[i][c] + dt_ * f_component[c]) * ws_ratio[c];
      tcc_next[i][c] = (tcc_next[i][c] + value) / 2;
    }
  }

  // update mass balance
  for (int i = 0; i < num_aqueous + num_gaseous; i++) {
    mass_solutes_exact_[i] += mass_solutes_source_[i] * dt_ / 2;
  }

  if (internal_tests_) {
    VV_CheckGEDproperty(*tcc_tmp->ViewComponent("cell"));
  }
}


/* ******************************************************************* 
* Advance each component independently due to different field
* reconstructions. This routine uses generic explicit time integrator. 
******************************************************************* */
void TransportExplicit_PK::AdvanceSecondOrderUpwindRKn(double dt_cycle)
{
  dt_ = dt_cycle;  // overwrite the maximum stable transport step

  S_->Get<CompositeVector>(tcc_key_).ScatterMasterToGhosted("cell");
  Epetra_MultiVector& tcc_prev = *tcc->ViewComponent("cell", true);
  Epetra_MultiVector& tcc_next = *tcc_tmp->ViewComponent("cell", true);

  // define time integration method
  auto ti_method = Explicit_TI::forward_euler;
  if (temporal_disc_order == 2) {
    ti_method = Explicit_TI::heun_euler;
  } else if (temporal_disc_order == 3) {
    ti_method = Explicit_TI::tvd_3rd_order;
  } else if (temporal_disc_order == 4) {
    ti_method = Explicit_TI::runge_kutta_4th_order;
  }

  // We interpolate ws using dt which becomes local time.
  double T = 0.0; 
  // We advect only aqueous components.
  int ncomponents = num_aqueous;

  for (int i = 0; i < ncomponents; i++) {
    current_component_ = i;  // it is needed in BJ called inside RK:fun

    Epetra_Vector*& component_prev = tcc_prev(i);
    Epetra_Vector*& component_next = tcc_next(i);

    Explicit_TI::RK<Epetra_Vector> TVD_RK(*this, ti_method, *component_prev);
    TVD_RK.TimeStep(T, dt_, *component_prev, *component_next);
  }
}


/* ******************************************************************* 
* MPC will call this function to advance the transport state.
* Efficient subcycling requires to calculate an intermediate state of
* saturation only once, which leads to a leap-frog-type algorithm.
******************************************************************* */
bool TransportExplicit_PK::AdvanceStep(double t_old, double t_new, bool reinit)
{ 
  bool failed = false;
  double dt_MPC = t_new - t_old;

  // We use original tcc and make a copy of it later if needed.
  tcc = S_->GetPtrW<CompositeVector>(tcc_key_, Tags::DEFAULT, passwd_);
  Epetra_MultiVector& tcc_prev = *tcc->ViewComponent("cell");

  // calculate stable time step
  double dt_shift = 0.0, dt_global = dt_MPC;
  double time = S_->intermediate_time();
  if (time >= 0.0) { 
    t_physics_ = time;
    dt_shift = time - S_->initial_time();
    dt_global = S_->final_time() - S_->initial_time();
  }

  StableTimeStep();
  double dt_original = dt_;  // advance routines override dt_
  int interpolate_ws = (dt_ < dt_global) ? 1 : 0;

  // start subcycling
  double dt_sum = 0.0;
  double dt_cycle;
  if (interpolate_ws) {
    dt_cycle = dt_original;
    InterpolateCellVector(*ws_prev, *ws, dt_shift, dt_global, *ws_subcycle_start);
  } else {
    dt_cycle = dt_MPC;
    ws_start = ws_prev;
    ws_end = ws;
  }

  int ncycles = 0, swap = 1;
  while (dt_sum < dt_MPC) {
    // update boundary conditions
    time = t_physics_ + dt_cycle / 2;
    for (int i = 0; i < bcs_.size(); i++) {
      bcs_[i]->Compute(time, time);
      bcs_[i]->ComputeSubmodel(mesh_, tcc);
    }
    
    double dt_try = dt_MPC - dt_sum;
    double tol = 1e-14 * (std::max(dt_try + dt_original, t_new)); 
    bool final_cycle = false;
    if (dt_try >= 2 * dt_original) {
      dt_cycle = dt_original;
    } else if (dt_try > dt_original + tol) { 
      dt_cycle = dt_try / 2; 
    } else {
      dt_cycle = dt_try;
      final_cycle = true;
    }

    t_physics_ += dt_cycle;
    dt_sum += dt_cycle;

    if (interpolate_ws) {
      if (swap) {  // Initial water saturation is in 'start'.
        ws_start = ws_subcycle_start;
        ws_end = ws_subcycle_end;

        double dt_int = dt_sum + dt_shift;
        InterpolateCellVector(*ws_prev, *ws, dt_int, dt_global, *ws_subcycle_end);
      } else {  // Initial water saturation is in 'end'.
        ws_start = ws_subcycle_end;
        ws_end = ws_subcycle_start;

        double dt_int = dt_sum + dt_shift;
        InterpolateCellVector(*ws_prev, *ws, dt_int, dt_global, *ws_subcycle_start);
      }
      swap = 1 - swap;
    }

    if (mesh_->space_dimension() == mesh_->manifold_dimension()) {
      if (spatial_disc_order == 1) {
        AdvanceDonorUpwind(dt_cycle);
      } else if (spatial_disc_order == 2 && genericRK_) {
        AdvanceSecondOrderUpwindRKn(dt_cycle);
      /* DEPRECATED 
      } else if (spatial_disc_order == 2 && temporal_disc_order == 1) {
        AdvanceSecondOrderUpwindRK1(dt_cycle);
      */
      } else if (spatial_disc_order == 2 && temporal_disc_order == 2) {
        AdvanceSecondOrderUpwindRK2(dt_cycle);
      }
    } else {  // transport on intersecting manifolds
      if (spatial_disc_order == 1) {
        AdvanceDonorUpwindNonManifold(dt_cycle);
      } else {
        AdvanceSecondOrderUpwindRKn(dt_cycle);
      }
    }

    // add implicit multiscale model
    if (multiscale_porosity_) {
      double t_int1 = t_old + dt_sum - dt_cycle;
      double t_int2 = t_old + dt_sum;
      AddMultiscalePorosity_(t_old, t_new, t_int1, t_int2);
    }

    if (! final_cycle) {  // rotate concentrations (we need new memory for tcc)
      tcc = Teuchos::RCP<CompositeVector>(new CompositeVector(*tcc_tmp));
    }

    ncycles++;
  }

  // output of selected statistics
  VV_PrintLimiterStatistics();

  dt_ = dt_original;  // restore the original time step (just in case)

  // We define tracer as the species #0 as calculate some statistics.
  Epetra_MultiVector& tcc_next = *tcc_tmp->ViewComponent("cell", false);

  bool flag_diffusion(false);
  for (int i = 0; i < 2; i++) {
    if (diffusion_phase_[i] != Teuchos::null) {
      if (diffusion_phase_[i]->values().size() != 0) flag_diffusion = true;
    }
  }
  if (flag_diffusion) {
    // no molecular diffusion if all tortuosities are zero.
    double tau(0.0);
    for (int i = 0; i < mat_properties_.size(); i++) {
      tau += mat_properties_[i]->tau[0] + mat_properties_[i]->tau[1];
    }
    if (tau == 0.0) flag_diffusion = false;
  }

  if (flag_dispersion_ || flag_diffusion) {
    if (flag_dispersion_) {
      auto darcy_flux = *S_->Get<CompositeVector>(darcy_flux_key_).ViewComponent("face", true);
      CalculateDispersionTensor_(darcy_flux, *transport_phi, *ws);
    }
    if (use_effective_diffusion_) {
      DiffusionSolverEffective(tcc_next, t_old, t_new);
    } else {
      DispersionSolver(tcc_prev, tcc_next, t_old, t_new);
    }
  }

  // optional Henry Law for the case of gas diffusion
  if (henry_law_) {
    MakeAirWaterPartitioning_();
  }

  // statistics output
  nsubcycles = ncycles;
  if (vo_->getVerbLevel() >= Teuchos::VERB_MEDIUM) {
    Teuchos::OSTab tab = vo_->getOSTab();
    *vo_->os() << ncycles << " sub-cycles, dt_stable=" << units_.OutputTime(dt_original) 
               << ", dt_MPC=" << units_.OutputTime(dt_MPC) << std::endl;

    VV_PrintSoluteExtrema(tcc_next, dt_MPC, "");
  }

  return failed;
}


/* ******************************************************************* 
* A simple first-order "donor" upwind method.
******************************************************************* */
void TransportExplicit_PK::AdvanceDonorUpwind(double dt_cycle)
{
  dt_ = dt_cycle;  // overwrite the maximum stable transport step
  mass_solutes_source_.assign(num_aqueous + num_gaseous, 0.0);

  // populating next state of concentrations
  tcc->ScatterMasterToGhosted("cell");
  Epetra_MultiVector& tcc_prev = *tcc->ViewComponent("cell", true);
  Epetra_MultiVector& tcc_next = *tcc_tmp->ViewComponent("cell", true);

  // prepare conservative state in master and slave cells
  double vol_phi_ws, tcc_flux;

  // We advect only aqueous components.
  int num_advect = num_aqueous;

  for (int c = 0; c < ncells_owned; c++) {
    vol_phi_ws = mesh_->cell_volume(c) * (*phi)[0][c] * (*ws_start)[0][c];

    for (int i = 0; i < num_advect; i++)
      tcc_next[i][c] = tcc_prev[i][c] * vol_phi_ws;
  }

  auto darcy_flux = *S_->Get<CompositeVector>(darcy_flux_key_).ViewComponent("face", true);
  const auto& flux_map = S_->Get<CompositeVector>(darcy_flux_key_).Map().Map("face", true);
 
  // advance all components at once
  for (int f = 0; f < nfaces_wghost; f++) {  // loop over master and slave faces
    int g = flux_map->FirstPointInElement(f);

    for ( int j = 0; j < upwind_cells_[f].size(); j++) {
      int c1 = upwind_cells_[f][j];
      int c2 = downwind_cells_[f][j];
                
      double u = fabs(darcy_flux[0][g + j]);

      if (c1 >=0 && c1 < ncells_owned && c2 >= 0 && c2 < ncells_owned) {
        for (int i = 0; i < num_advect; i++) {
          tcc_flux = dt_ * u * tcc_prev[i][c1];
          tcc_next[i][c1] -= tcc_flux;
          tcc_next[i][c2] += tcc_flux;
        }

      } else if (c1 >=0 && c1 < ncells_owned && (c2 >= ncells_owned || c2 < 0)) {
        for (int i = 0; i < num_advect; i++) {
          tcc_flux = dt_ * u * tcc_prev[i][c1];
          tcc_next[i][c1] -= tcc_flux;
        }

      } else if (c1 >= ncells_owned && c2 >= 0 && c2 < ncells_owned) {
        for (int i = 0; i < num_advect; i++) {
          tcc_flux = dt_ * u * tcc_prev[i][c1];
          tcc_next[i][c2] += tcc_flux;
        }
      }
    }
  }

  // loop over exterior boundary sets
  int flag(0);
  tcc_tmp->PutScalarGhosted(0.0);

  for (int m = 0; m < bcs_.size(); m++) {
    std::vector<int>& tcc_index = bcs_[m]->tcc_index();
    int ncomp = tcc_index.size();

    for (auto it = bcs_[m]->begin(); it != bcs_[m]->end(); ++it) {
      int f = it->first;
      if (f >= nfaces_owned) continue;

      std::vector<double>& values = it->second;       

      if (downwind_cells_[f].size() > 0) {
        for (int j = 0; j < downwind_cells_[f].size(); j++) {
          int c2 = downwind_cells_[f][j];
          if (c2 < 0) continue;
          if (c2 >= ncells_owned) flag = 1;

          double u = fabs(downwind_flux_[f][j]);
          for (int i = 0; i < ncomp; i++) {
            int k = tcc_index[i];
            if (k < num_advect) {
              tcc_flux = dt_ * u * values[i];
              tcc_next[k][c2] += tcc_flux;
            }
          }
        }
      }
    }    
  }

  int flag_tmp(flag);
  mesh_->get_comm()->MaxAll(&flag_tmp, &flag, 1);
  if (flag == 1) tcc_tmp->GatherGhostedToMaster();

  // process external sources
  if (srcs_.size() != 0) {
    double time = t_physics_;
    ComputeSources_(time, dt_, tcc_next, tcc_prev, 0, num_advect - 1);
  }

  // recover concentration from new conservative state
  for (int c = 0; c < ncells_owned; c++) {
    vol_phi_ws = mesh_->cell_volume(c) * (*phi)[0][c] * (*ws_end)[0][c];
    for (int i = 0; i < num_advect; i++) tcc_next[i][c] /= vol_phi_ws;
  }

  // update mass balance
  for (int i = 0; i < mass_solutes_exact_.size(); i++) {
    mass_solutes_exact_[i] += mass_solutes_source_[i] * dt_;
  }

  if (internal_tests_) {
    VV_CheckGEDproperty(*tcc_tmp->ViewComponent("cell"));
  }
}


/* ******************************************************************* 
* A simple first-order upwind method on non-manifolds.
******************************************************************* */
void TransportExplicit_PK::AdvanceDonorUpwindNonManifold(double dt_cycle)
{
  dt_ = dt_cycle;  // overwrite the maximum stable transport step
  mass_solutes_source_.assign(num_aqueous + num_gaseous, 0.0);

  // populating next state of concentrations
  tcc->ScatterMasterToGhosted("cell");
  Epetra_MultiVector& tcc_prev = *tcc->ViewComponent("cell", true);
  Epetra_MultiVector& tcc_next = *tcc_tmp->ViewComponent("cell", true);

  // prepare conservative state in master and slave cells
  double u, vol_phi_ws, tcc_flux;

  // We advect only aqueous components.
  int num_advect = num_aqueous;

  for (int c = 0; c < ncells_owned; c++) {
    vol_phi_ws = mesh_->cell_volume(c) * (*phi)[0][c] * (*ws_start)[0][c];

    for (int i = 0; i < num_advect; i++)
      tcc_next[i][c] = tcc_prev[i][c] * vol_phi_ws;
  }

  // advance all components at once
  for (int f = 0; f < nfaces_wghost; f++) {
    // calculate in and out fluxes and solutes at given face
    double flux_in(0.0);
    std::vector<double> tcc_out(num_advect, 0.0);

    for (int n = 0; n < upwind_cells_[f].size(); ++n) {
      int c = upwind_cells_[f][n];
      u = upwind_flux_[f][n];

      for (int i = 0; i < num_advect; i++) {
        tcc_out[i] += u * tcc_prev[i][c];
      }
    }

    for (int n = 0; n < downwind_cells_[f].size(); ++n) {
      flux_in -= downwind_flux_[f][n];
    }
    if (flux_in == 0.0) flux_in = 1e-12;

    // update solutes
    for (int n = 0; n < upwind_cells_[f].size(); ++n) {
      int c = upwind_cells_[f][n];
      u = upwind_flux_[f][n];

      if (c < ncells_owned) {
        for (int i = 0; i < num_advect; i++) {
          tcc_next[i][c] -= dt_ * u * tcc_prev[i][c];
        }
      }
    }

    for (int n = 0; n < downwind_cells_[f].size(); ++n) {
      int c = downwind_cells_[f][n];
      u = downwind_flux_[f][n];

      if (c < ncells_owned) {
        double tmp = u / flux_in;
        for (int i = 0; i < num_advect; i++) {
          tcc_next[i][c] -= dt_ * tmp * tcc_out[i];
        }
      }
    }
  }

  // loop over exterior boundary sets
  for (int m = 0; m < bcs_.size(); m++) {
    std::vector<int>& tcc_index = bcs_[m]->tcc_index();
    int ncomp = tcc_index.size();

    for (auto it = bcs_[m]->begin(); it != bcs_[m]->end(); ++it) {
      int f = it->first;
      std::vector<double>& values = it->second; 

      if (downwind_cells_[f].size() > 0) {
        int c = downwind_cells_[f][0];
        u = downwind_flux_[f][0];

        for (int i = 0; i < ncomp; i++) {
          int k = tcc_index[i];
          if (k < num_advect) {
            tcc_flux = dt_ * u * values[i];
            tcc_next[k][c] -= tcc_flux;
          }
        }
      }
    }
  }

  // process external sources
  if (srcs_.size() != 0) {
    double time = t_physics_;
    ComputeSources_(time, dt_, tcc_next, tcc_prev, 0, num_advect - 1);
  }

  // recover concentration from new conservative state
  for (int c = 0; c < ncells_owned; c++) {
    vol_phi_ws = mesh_->cell_volume(c) * (*phi)[0][c] * (*ws_end)[0][c];
    for (int i = 0; i < num_advect; i++) tcc_next[i][c] /= vol_phi_ws;
  }

  // update mass balance
  for (int i = 0; i < mass_solutes_exact_.size(); i++) {
    mass_solutes_exact_[i] += mass_solutes_source_[i] * dt_;
  }
}

}  // namespace Transport
}  // namespace Amazni

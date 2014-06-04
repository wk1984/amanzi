/*
  This is the flow component of the Amanzi code. 

  Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "UnitTest++.h"

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_ParameterXMLFileReader.hpp"

#include "MeshFactory.hh"
#include "GMVMesh.hh"

#include "State.hh"
#include "Darcy_PK.hh"

/* **************************************************************** */
TEST(FLOW_2D_TRANSIENT_DARCY) {
  using namespace Teuchos;
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::AmanziGeometry;
  using namespace Amanzi::AmanziFlow;

  Epetra_MpiComm comm(MPI_COMM_WORLD);
  int MyPID = comm.MyPID();

  if (MyPID == 0) std::cout << "Test: 2D transient Darcy, 2-layer model" << std::endl;

  /* read parameter list */
  std::string xmlFileName = "test/flow_darcy_2D.xml";
  ParameterXMLFileReader xmlreader(xmlFileName);
  ParameterList plist = xmlreader.getParameters();

  /* create an SIMPLE mesh framework */
  ParameterList region_list = plist.get<Teuchos::ParameterList>("Regions");
  GeometricModelPtr gm = new GeometricModel(2, region_list, &comm);

  FrameworkPreference pref;
  pref.clear();
  pref.push_back(MSTK);

  MeshFactory meshfactory(&comm);
  meshfactory.preference(pref);
  RCP<const Mesh> mesh = meshfactory(0.0, -2.0, 1.0, 0.0, 18, 18, gm);

  /* create a simple state and populate it */
  Amanzi::VerboseObject::hide_line_prefix = true;
  Amanzi::VerboseObject::global_default_level = Teuchos::VERB_EXTREME;

  ParameterList state_list;
  RCP<State> S = rcp(new State(state_list));
  S->RegisterDomainMesh(rcp_const_cast<Mesh>(mesh));

  Darcy_PK* DPK = new Darcy_PK(plist, S);
  S->Setup();
  S->InitializeFields();
  S->InitializeEvaluators();
  DPK->InitializeFields();
  S->CheckAllFieldsInitialized();

  /* modify the default state for the problem at hand */
  std::string passwd("state"); 
  Epetra_MultiVector& K = *S->GetFieldData("permeability", passwd)->ViewComponent("cell", false);
  
  AmanziMesh::Entity_ID_List block;
  mesh->get_set_entities("Material 1", AmanziMesh::CELL, AmanziMesh::OWNED, &block);
  for (int i = 0; i != block.size(); ++i) {
    int c = block[i];
    K[0][c] = 0.1;
    K[1][c] = 2.0;
  }

  mesh->get_set_entities("Material 2", AmanziMesh::CELL, AmanziMesh::OWNED, &block);
  for (int i = 0; i != block.size(); ++i) {
    int c = block[i];
    K[0][c] = 0.5;
    K[1][c] = 0.5;
  }

  *S->GetScalarData("fluid_density", passwd) = 1.0;
  *S->GetScalarData("fluid_viscosity", passwd) = 1.0;
  Epetra_Vector& gravity = *S->GetConstantVectorData("gravity", passwd);
  gravity[1] = -1.0;

  S->GetFieldData("specific_storage", passwd)->PutScalar(2.0);

  /* create the initial pressure function */
  Epetra_MultiVector& p = *S->GetFieldData("pressure", passwd)->ViewComponent("cell", false);

  for (int c = 0; c < p.MyLength(); c++) {
    const Point& xc = mesh->cell_centroid(c);
    p[0][c] = xc[1] * (xc[1] + 2.0);
  }

  /* initialize the Darcy process kernel */
  DPK->InitPK();
  DPK->InitSteadyState(0.0, 1e-8);

  /* transient solution */
  double dT = 0.1;
  for (int n = 0; n < 10; n++) {
    double dT_actual(dT);
    DPK->Advance(dT, dT_actual);
    DPK->CommitState(S);

    if (MyPID == 0 && n > 5) {
      GMV::open_data_file(*mesh, (std::string)"flow.gmv");
      GMV::start_data();
      GMV::write_cell_data(p, 0, "pressure");
      GMV::close_data_file();
    }
  }

  // Testing secondary fields
  DPK->UpdateAuxilliaryData();
  const Epetra_MultiVector& darcy_velocity = *S->GetFieldData("darcy_velocity")->ViewComponent("cell");
  Point p5(darcy_velocity[0][5], darcy_velocity[1][5]);

  // Testing recovery
  std::vector<AmanziGeometry::Point> xyz;
  std::vector<AmanziGeometry::Point> velocity;
  DPK->CalculateDarcyVelocity(xyz, velocity);

  CHECK(L22(p5 - velocity[5]) < 1e-10);
  
  for (int n = 0; n < 10; n++) { 
    // std::cout << n << " xyz=" << xyz[n] << " vel=" << velocity[n] << std::endl;
  } 

  delete DPK;
}


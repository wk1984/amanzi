/*
  This is the flow component of the Amanzi code. 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Neil Carlson (version 1)  (nnc@lanl.gov)
           Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)
*/

#include <string>
#include <vector>

#include "errors.hh"
#include "MultiFunction.hh"

#include "Flow_BC_Factory.hh"
#include "FlowDefs.hh"

namespace Amanzi {
namespace AmanziFlow {

/* ******************************************************************
* Constructor
****************************************************************** */
FlowBCFactory::FlowBCFactory(const Teuchos::RCP<const AmanziMesh::Mesh>& mesh,
                             const Teuchos::RCP<Teuchos::ParameterList>& plist)
   : mesh_(mesh), plist_(plist)
{
  // create verbosity object
  Teuchos::ParameterList vlist;
  // vlist.set("Verbosity Level", "medium");
  vo_ = new VerboseObject("FlowPK::Richards", vlist); 
}


/* ******************************************************************
* Process Dirichet BC (pressure), step 1.
****************************************************************** */
Functions::FlowBoundaryFunction* FlowBCFactory::CreatePressure(std::vector<int>& submodel) const
{
  Functions::FlowBoundaryFunction* bc = new Functions::FlowBoundaryFunction(mesh_);
  try {
    ProcessPressureList(plist_->sublist("pressure"), submodel, bc);
  } catch (Errors::Message& msg) {
    Errors::Message m;
    m << "FlowBCFactory: \"pressure\" sublist error: " << msg.what();
    Exceptions::amanzi_throw(m);
  } catch (Teuchos::Exceptions::InvalidParameterType& msg) {
    Errors::Message m;
    m << "FlowBCFactory: \"pressure\" sublist error: not a sublist: " << msg.what();
    Exceptions::amanzi_throw(m);
  }
  return bc;
}


/* ******************************************************************
* Process Neumann BC (mass flux), step 1.
****************************************************************** */
Functions::FlowBoundaryFunction* FlowBCFactory::CreateMassFlux(std::vector<int>& submodel) const
{
  int ncells = mesh_->num_entities(AmanziMesh::FACE, AmanziMesh::OWNED);

  Functions::FlowBoundaryFunction* bc = new Functions::FlowBoundaryFunction(mesh_);
  try {
    ProcessMassFluxList(plist_->sublist("mass flux"), submodel, bc);
  } catch (Errors::Message& msg) {
    Errors::Message m;
    m << "FlowBCFactory: \"mass flux\" sublist error: " << msg.what();
    Exceptions::amanzi_throw(m);
  } catch (Teuchos::Exceptions::InvalidParameterType& msg) {
    Errors::Message m;
    m << "FlowBCFactory: \"mass flux\" sublist error: not a sublist: " << msg.what();
    Exceptions::amanzi_throw(m);
  }
  return bc;
}


/* ******************************************************************
* Process Dirichet BC (static head), step 1.
****************************************************************** */
Functions::FlowBoundaryFunction* FlowBCFactory::CreateStaticHead(
    double p0, double density, const AmanziGeometry::Point& gravity, 
    std::vector<int>& submodel) const
{
  Functions::FlowBoundaryFunction* bc = new Functions::FlowBoundaryFunction(mesh_);
  bc->set_reference_pressure(p0);  // Set default reference pressure

  try {
    ProcessStaticHeadList(p0, density, gravity, plist_->sublist("static head"), submodel, bc);
  } catch (Errors::Message& msg) {
    Errors::Message m;
    m << "FlowBCFactory: \"static head\" sublist error: " << msg.what();
    Exceptions::amanzi_throw(m);
  } catch (Teuchos::Exceptions::InvalidParameterType& msg) {
    Errors::Message m;
    m << "FlowBCFactory: \"static head\" sublist error: not a sublist: " << msg.what();
    Exceptions::amanzi_throw(m);
  }
  return bc;
}


/* ******************************************************************
* Seepage Face BC, step 1.
****************************************************************** */
Functions::FlowBoundaryFunction* FlowBCFactory::CreateSeepageFace(
    double p0, std::vector<int>& submodel) const
{
  Functions::FlowBoundaryFunction* bc = new Functions::FlowBoundaryFunction(mesh_);
  bc->set_reference_pressure(p0);  // Set default reference pressure

  try {
    ProcessSeepageFaceList(plist_->sublist("seepage face"), submodel, bc);
  } catch (Errors::Message& msg) {
    Errors::Message m;
    m << "FlowBCFactory: \"seepage face\" sublist error: " << msg.what();
    Exceptions::amanzi_throw(m);
  } catch (Teuchos::Exceptions::InvalidParameterType& msg) {
    Errors::Message m;
    m << "FlowBCFactory: \"seepage face\" sublist error: not a sublist: " << msg.what();
    Exceptions::amanzi_throw(m);
  }
  return bc;
}


/* ******************************************************************
* Process Dirichet BC (pressure), step 2.
* Loop over sublists with typical names "BC 0", "BC 1", etc.
****************************************************************** */
void FlowBCFactory::ProcessPressureList(
    Teuchos::ParameterList& list, std::vector<int>& submodel, Functions::FlowBoundaryFunction* bc) const
{
  // Iterate through the BC specification sublists in the list.
  // All are expected to be sublists of identical structure.
  for (Teuchos::ParameterList::ConstIterator i = list.begin(); i != list.end(); ++i) {
    std::string name = i->first;
    if (list.isSublist(name)) {
      Teuchos::ParameterList& spec = list.sublist(name);
      try {
        ProcessPressureSpec(spec, submodel, bc);
      } catch (Errors::Message& msg) {
        Errors::Message m;
        m << "in sublist \"" << spec.name().c_str() << "\": " << msg.what();
        Exceptions::amanzi_throw(m);
      }
    } else {
      Errors::Message m;
      m << "parameter \"" << name.c_str() << "\" is not a sublist";
      Exceptions::amanzi_throw(m);
    }
  }
}


/* ******************************************************************
* Process Dirichet BC (pressure), step 3.
****************************************************************** */
void FlowBCFactory::ProcessPressureSpec(
    Teuchos::ParameterList& list, std::vector<int>& submodel, Functions::FlowBoundaryFunction* bc) const
{
  Errors::Message m;
  std::vector<std::string> regions;

  if (list.isParameter("regions")) {
    if (list.isType<Teuchos::Array<std::string> >("regions")) {
      regions = list.get<Teuchos::Array<std::string> >("regions").toVector();
    } else {
      m << "parameter \"regions\" is not of type \"Array string\"";
      Exceptions::amanzi_throw(m);
    }
  } else {
    m << "parameter \"regions\" is missing";
    Exceptions::amanzi_throw(m);
  }

  Teuchos::ParameterList* f_list;
  if (list.isSublist("boundary pressure")) {
    f_list = &list.sublist("boundary pressure");
  } else {  // "boundary pressure" is not a sublist
    m << "parameter \"boundary pressure\" is not a sublist";
    Exceptions::amanzi_throw(m);
  }

  // Make the boundary pressure function.
  Teuchos::RCP<Amanzi::MultiFunction> f;
  try {
    f = Teuchos::rcp(new Amanzi::MultiFunction(*f_list));
  } catch (Errors::Message& msg) {
    m << "error in sublist \"boundary pressure\": " << msg.what();
    Exceptions::amanzi_throw(m);
  }

  // Add this BC specification to the boundary function.
  bc->Define(regions, f, Amanzi::Functions::BOUNDARY_FUNCTION_ACTION_NONE);
}


/* ******************************************************************
* Process Neumann BC (mass flux), step 2.
* Iterate through the BC specification sublists in the list with 
* typical names "BC 0", "BC 1", etc. All are expected to be sublists 
* of identical structure.
****************************************************************** */
void FlowBCFactory::ProcessMassFluxList(
    Teuchos::ParameterList& list, std::vector<int>& submodel, Functions::FlowBoundaryFunction* bc) const
{
  for (Teuchos::ParameterList::ConstIterator i = list.begin(); i != list.end(); ++i) {
    std::string name = i->first;
    if (list.isSublist(name)) {
      Teuchos::ParameterList& spec = list.sublist(name);
      try {
        ProcessMassFluxSpec(spec, submodel, bc);
      } catch (Errors::Message& msg) {
        Errors::Message m;
        m << "in sublist \"" << spec.name().c_str() << "\": " << msg.what();
        Exceptions::amanzi_throw(m);
      }
    } else {
      Errors::Message m;
      m << "parameter \"" << name.c_str() << "\" is not a sublist";
      Exceptions::amanzi_throw(m);
    }
  }
}


/* ******************************************************************
* Process Neumann BC (mass flux), step 3.
****************************************************************** */
void FlowBCFactory::ProcessMassFluxSpec(
    Teuchos::ParameterList& list, std::vector<int>& submodel, Functions::FlowBoundaryFunction* bc) const
{
  Errors::Message m;
  std::vector<std::string> regions;

  if (list.isParameter("regions")) {
    if (list.isType<Teuchos::Array<std::string> >("regions")) {
      regions = list.get<Teuchos::Array<std::string> >("regions").toVector();
    } else {
      m << "parameter \"regions\" is not of type \"Array string\"";
      Exceptions::amanzi_throw(m);
    }
  } else {
    m << "parameter \"regions\" is missing";
    Exceptions::amanzi_throw(m);
  }

  Teuchos::ParameterList* f_list;
  if (list.isSublist("outward mass flux")) {
    f_list = &list.sublist("outward mass flux");
  } else {
    m << "parameter \"outward mass flux\" is not a sublist";
    Exceptions::amanzi_throw(m);
  }

  // Make the outward mass flux function.
  Teuchos::RCP<Amanzi::MultiFunction> f;
  try {
    f = Teuchos::rcp(new Amanzi::MultiFunction(*f_list));
  } catch (Errors::Message& msg) {
    m << "error in sublist \"outward mass flux\": " << msg.what();
    Exceptions::amanzi_throw(m);
  }

  // Add this BC specification to the boundary function.
  bc->Define(regions, f, Amanzi::Functions::BOUNDARY_FUNCTION_ACTION_NONE);

  // Populate submodel flags.
  if (list.get<bool>("rainfall", false))
      PopulateSubmodelFlag(regions, FLOW_BC_SUBMODEL_RAINFALL, submodel);
}


/* ******************************************************************
* Process Dirichet BC (static head), step 2.
* Iterate through the BC specification sublists with typical names
* "BC 0", "BC 1", etc. All are expected to be sublists of identical
* structure.
****************************************************************** */
void FlowBCFactory::ProcessStaticHeadList(
    double p0, double density, const AmanziGeometry::Point& gravity,
    Teuchos::ParameterList& list, std::vector<int>& submodel, Functions::FlowBoundaryFunction* bc) const
{
  for (Teuchos::ParameterList::ConstIterator i = list.begin(); i != list.end(); ++i) {
    std::string name = i->first;
    if (list.isSublist(name)) {
      Teuchos::ParameterList& spec = list.sublist(name);
      try {
        ProcessStaticHeadSpec(p0, density, gravity, spec, submodel, bc);
      } catch (Errors::Message& msg) {
        Errors::Message m;
        m << "in sublist \"" << spec.name().c_str() << "\": " << msg.what();
        Exceptions::amanzi_throw(m);
      }
    } else {
      Errors::Message m;
      m << "parameter \"" << name.c_str() << "\" is not a sublist";
      Exceptions::amanzi_throw(m);
    }
  }
}


/* ******************************************************************
* Process Dirichet BC (static head), step 3.
****************************************************************** */
void FlowBCFactory::ProcessStaticHeadSpec(
    double p0, double density, const AmanziGeometry::Point& gravity,
    Teuchos::ParameterList& list, std::vector<int>& submodel, Functions::FlowBoundaryFunction* bc) const
{
  Errors::Message m;
  std::vector<std::string> regions;

  if (list.isParameter("regions")) {
    if (list.isType<Teuchos::Array<std::string> >("regions")) {
      regions = list.get<Teuchos::Array<std::string> >("regions").toVector();
    } else {
      m << "parameter \"regions\" is not of type \"Array string\"";
      Exceptions::amanzi_throw(m);
    }
  } else {
    m << "parameter \"regions\" is missing";
    Exceptions::amanzi_throw(m);
  }

  // Get the water table elevation function sublist.
  Teuchos::ParameterList* water_table_list;
  if (list.isSublist("water table elevation")) {
    water_table_list = &list.sublist("water table elevation");
  } else {
    m << "parameter \"water table elevation\" is not a sublist";
    Exceptions::amanzi_throw(m);
  }

  // Form the parameter list to create static head function.
  Teuchos::ParameterList f_list;
  Teuchos::ParameterList& static_head_list = f_list.sublist("function-static-head");
  int dim = gravity.dim();

  static_head_list.set("p0", p0);
  static_head_list.set("density", density);
  static_head_list.set("gravity", -gravity[dim-1]);
  static_head_list.set("space dimension", dim);
  static_head_list.set("water table elevation", *water_table_list);

  Teuchos::RCP<Amanzi::MultiFunction> f;
  try {
    f = Teuchos::rcp(new Amanzi::MultiFunction(f_list));
  } catch (Errors::Message& msg) {
    m << "error in sublist \"water table elevation\": " << msg.what();
    Exceptions::amanzi_throw(m);
  }

  // Populate submodel flags.
  int method = Amanzi::Functions::BOUNDARY_FUNCTION_ACTION_NONE;
  if (list.get<bool>("relative to top", false)) {
    PopulateSubmodelFlag(regions, FLOW_BC_SUBMODEL_HEAD_RELATIVE, submodel);
    method = Amanzi::Functions::BOUNDARY_FUNCTION_ACTION_HEAD_RELATIVE;
  }

  // Add this BC specification to the boundary function.
  bc->Define(regions, f, method);
}


/* ******************************************************************
* Process Seepage Face BC, step 2.
****************************************************************** */
void FlowBCFactory::ProcessSeepageFaceList(
    Teuchos::ParameterList& list, std::vector<int>& submodel, Functions::FlowBoundaryFunction* bc) const
{
  // Iterate through the BC specification sublists in the list.
  // All are expected to be sublists of identical structure.
  for (Teuchos::ParameterList::ConstIterator i = list.begin(); i != list.end(); ++i) {
    std::string name = i->first;
    if (list.isSublist(name)) {
      Teuchos::ParameterList& spec = list.sublist(name);
      try {
        ProcessSeepageFaceSpec(spec, submodel, bc);
      } catch (Errors::Message& msg) {
        Errors::Message m;
        m << "in sublist \"" << spec.name().c_str() << "\": " << msg.what();
        Exceptions::amanzi_throw(m);
      }
    } else {  // WARNING -- parameter is not a sublist
      Teuchos::OSTab tab = vo_->getOSTab();
      *vo_->os() << vo_->color("yellow") << "ignoring Flow BC parameter \"" 
                 << name.c_str() << "\"" << vo_->reset() << std::endl;
    }
  }
}


/* ******************************************************************
* Process Seepage Face BC, step 3.
****************************************************************** */
void FlowBCFactory::ProcessSeepageFaceSpec(
    Teuchos::ParameterList& list, std::vector<int>& submodel, Functions::FlowBoundaryFunction* bc) const
{
  Errors::Message m;
  std::vector<std::string> regions;

  if (list.isParameter("regions")) {
    if (list.isType<Teuchos::Array<std::string> >("regions")) {
      regions = list.get<Teuchos::Array<std::string> >("regions").toVector();
    } else {
      m << "parameter \"regions\" is not of type \"Array string\"";
      Exceptions::amanzi_throw(m);
    }
  } else {  // Parameter "regions" is missing.
    m << "parameter \"regions\" is missing";
    Exceptions::amanzi_throw(m);
  }

  Teuchos::ParameterList* f_list;
  if (list.isSublist("outward mass flux")) {
    f_list = &list.sublist("outward mass flux");
  } else {
    m << "parameter \"outward mass flux\" is not a sublist";
    Exceptions::amanzi_throw(m);
  }

  // Make the seepage face function.
  Teuchos::RCP<Amanzi::MultiFunction> f;
  try {
    f = Teuchos::rcp(new Amanzi::MultiFunction(*f_list));
  } catch (Errors::Message& msg) {
    m << "error in sublist \"outward mass flux\": " << msg.what();
    Exceptions::amanzi_throw(m);
  }

  // Add this BC specification to the boundary function.
  bc->Define(regions, f, Amanzi::Functions::BOUNDARY_FUNCTION_ACTION_NONE);

  // Populate submodel flags.
  if (list.get<bool>("rainfall", false))
      PopulateSubmodelFlag(regions, FLOW_BC_SUBMODEL_RAINFALL, submodel);

  std::string submodel_name = list.get<std::string>("submodel", "PFloTran");
  if (submodel_name == "PFloTran") {
    PopulateSubmodelFlag(regions, FLOW_BC_SUBMODEL_SEEPAGE_PFLOTRAN, submodel);
  } else if (submodel_name == "FACT") {
    PopulateSubmodelFlag(regions, FLOW_BC_SUBMODEL_SEEPAGE_FACT, submodel);
  } else {
    PopulateSubmodelFlag(regions, FLOW_BC_SUBMODEL_SEEPAGE_AMANZI, submodel);
  }
}


/* ******************************************************************
* Populate submodel flags.
****************************************************************** */
void FlowBCFactory::PopulateSubmodelFlag(
    const std::vector<std::string>& regions, int flag, std::vector<int>& submodel) const
{
  int nregions = regions.size();
  for (int n = 0; n < nregions; n++) {
    AmanziMesh::Entity_ID_List faces;
    mesh_->get_set_entities(regions[n], AmanziMesh::FACE, AmanziMesh::OWNED, &faces);

    int nfaces = faces.size();
    for (int m = 0; m < nfaces; m++) {
      int f = faces[m];
      submodel[f] += flag;
    }
  }
}

}  // namespace AmanziFlow
}  // namespace Amanzi

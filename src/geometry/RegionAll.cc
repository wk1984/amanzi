/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
/*
  A region consisting of all entities on a mesh.

  Copyright 2010-2013 held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#include "dbc.hh"
#include "errors.hh"

#include "RegionAll.hh"


namespace Amanzi {
namespace AmanziGeometry {

RegionAll::RegionAll(const std::string& name,
                     const int id,
                     const LifeCycleType lifecycle)
  : Region(name, id, false, RegionType::ALL, 0, 0, lifecycle) {
  // Region dimension is set arbitrarily as 0 since the set of
  // entities in the mesh will determine the dimension
}


// -------------------------------------------------------------
// RegionAll::inside
// -------------------------------------------------------------
bool
RegionAll::inside(const Point& p) const
{
  Errors::Message mesg("In/out check not implemented for region all");
  Exceptions::amanzi_throw(mesg);
  return false;
}

} // namespace AmanziGeometry
} // namespace Amanzi


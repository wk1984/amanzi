#include "Teuchos_ParameterList.hpp"
#include "Teuchos_Array.hpp"

namespace Amanzi {
namespace AmanziInput {

#define AMANZI_OLD_INPUT_VERSION_MAJOR 1
#define AMANZI_OLD_INPUT_VERSION_MINOR 2
#define AMANZI_OLD_INPUT_VERSION_MICRO 1


Teuchos::ParameterList translate (Teuchos::ParameterList* plist, int numproc);

Teuchos::ParameterList get_Time_Macro (const std::string& macro_name, Teuchos::ParameterList* plist );
Teuchos::ParameterList get_Cycle_Macro (const std::string& macro_name, Teuchos::ParameterList* plist );
Teuchos::Array<std::string> get_Variable_Macro ( const std::string& macro_name, Teuchos::ParameterList* plist );
void init_global_info( Teuchos::ParameterList* plist );

Teuchos::ParameterList create_Checkpoint_Data_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_Walkabout_Data_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_Visualization_Data_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_Observation_Data_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList get_Regions_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList get_Mesh_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList translate_Mesh_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList get_Domain_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_MPC_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_Transport_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_TransportSrc_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_Flow_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_WRM_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_FlowSrc_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_SS_FlowBC_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_State_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_Verbosity_List ( const std::string& vlevel );
Teuchos::ParameterList CreateChemistryList ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_Preconditioners_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_DPC_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_BILU_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_HypreAMG_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_Solvers_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_Nonlinear_Solvers_List ( Teuchos::ParameterList* plist );
Teuchos::ParameterList create_TimePeriodControl_List(Teuchos::ParameterList* plist);
Teuchos::ParameterList CreateFlowOperatorList();

void output_boundary_conditions( Teuchos::ParameterList* plist );
void check_AmanziInputVersion(Teuchos::ParameterList* plist);

Teuchos::Array<std::string> translate_forms (Teuchos::Array<std::string> & forms);

static std::string phase_name;
static std::string phase_comp_name;
static Teuchos::Array<std::string> comp_names;
static std::map<std::string, int> comp_names_map;
static Teuchos::Array<std::string> mineral_names_;
static Teuchos::Array<std::string> sorption_site_names_;
static std::string verbosity_level("low");
static int numproc_;
static int spatial_dimension_;  

static bool need_dispersion_;
}
}

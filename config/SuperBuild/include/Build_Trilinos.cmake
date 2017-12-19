#  -*- mode: cmake -*-

#
# Build TPL: Trilinos
#    
# --- Define all the directories and common external project flags
set(trilinos_depend_projects NetCDF Boost SEACAS)
if(ENABLE_HYPRE)
  list(APPEND trilinos_depend_projects HYPRE)
endif()
define_external_project_args(Trilinos
                             TARGET trilinos
                             DEPENDS ${trilinos_depend_projects})

# add version version to the autogenerated tpl_versions.h file
amanzi_tpl_version_write(FILENAME ${TPL_VERSIONS_INCLUDE_FILE}
  PREFIX Trilinos
  VERSION ${Trilinos_VERSION_MAJOR} ${Trilinos_VERSION_MINOR} ${Trilinos_VERSION_PATCH})
  
# --- Define the configuration parameters   

#  - Trilinos Package Configuration

#if(Trilinos_Build_Config_File)
#  message(STATUS "Including Trilinos build configuration file ${Trilinos_Build_Config_File}")
#  if ( NOT EXISTS ${Trilinos_Build_Config_File} )
#    message(FATAL_ERROR "File ${Trilinos_Build_Config_File} does not exist.")
#  endif()
#  include(${Trilinos_Build_Config_File})
#endif()

# List of packages enabled in the Trilinos build
set(Trilinos_PACKAGE_LIST Teuchos Epetra EpetraExt Amesos Belos NOX Ifpack AztecOO)
if ( ENABLE_STK_Mesh )
  list(APPEND Trilinos_PACKAGE_LIST STK)
endif()
if ( ENABLE_MSTK_Mesh )
  list(APPEND Trilinos_PACKAGE_LIST Zoltan)
endif()
if ( ENABLE_Unstructured )
  list(APPEND Trilinos_PACKAGE_LIST ML)
endif()


# Generate the Trilinos Package CMake Arguments
set(Trilinos_CMAKE_PACKAGE_ARGS "-DTrilinos_ENABLE_ALL_OPTIONAL_PACKAGES:BOOL=OFF")
foreach(package ${Trilinos_PACKAGE_LIST})
  list(APPEND Trilinos_CMAKE_PACKAGE_ARGS "-DTrilinos_ENABLE_${package}:STRING=ON")
endforeach()

# Build PyTrilinos if shared
# if (BUILD_SHARED_LIBS)
#   list(APPEND Trilinos_CMAKE_PACKAGE_ARGS "-DTrilinos_ENABLE_PyTrilinos:BOOL=ON")
#endif()

# Trilinos 11.0.3 has some C++ compile errors in it that we can sidestep by 
# defining HAVE_TEUCHOS_ARRAY_BOUNDSCHECK.
list(APPEND Trilinos_CMAKE_PACKAGE_ARGS "-DTeuchos_ENABLE_ABC:BOOL=ON")

# Disable Pamgen ( doesn't compile with gnu++14 standard )
list(APPEND Trilinos_CMAKE_PACKAGE_ARGS "-DTrilinos_ENABLE_Pamgen:STRING=OFF")

#  - Trilinos TPL Configuration

set(Trilinos_CMAKE_TPL_ARGS)

# MPI
list(APPEND Trilinos_CMAKE_TPL_ARGS "-DTPL_ENABLE_MPI:BOOL=ON")

# Pass the following MPI arguments to Trilinos if they are set 
set(MPI_CMAKE_ARGS DIR EXEC EXEC_NUMPROCS_FLAG EXE_MAX_NUMPROCS C_COMPILER)
foreach (var ${MPI_CMAKE_ARGS} )
  set(mpi_var "MPI_${var}")
  if ( ${mpi_var} )
    list(APPEND Trilinos_CMAKE_TPL_ARGS "-D${mpi_var}:STRING=${${mpi_var}}")
  endif()
endforeach() 

# BLAS
if ( BLAS_LIBRARIES )
  list(APPEND Trilinos_CMAKE_TPL_ARGS
              "-DTPL_ENABLE_BLAS:BOOL=TRUE")
  list(APPEND Trilinos_CMAKE_TPL_ARGS
              "-DTPL_BLAS_LIBRARIES:STRING=${BLAS_LIBRARIES}")
  message(STATUS "Trilinos BLAS libraries: ${BLAS_LIBRARIES}")    
else()
  message(WARNING "BLAS libraies not set. Trilinos will perform search.") 
endif()            
 
# LAPACK
if ( LAPACK_LIBRARIES )
  list(APPEND Trilinos_CMAKE_TPL_ARGS
              "-DTPL_LAPACK_LIBRARIES:STRING=${LAPACK_LIBRARIES}")
            message(STATUS "Trilinos LAPACK libraries: ${LAPACK_LIBRARIES}")    
else()
  message(WARNING "LAPACK libraies not set. Trilinos will perform search.") 
endif()

# Boost
list(APPEND Trilinos_CMAKE_TPL_ARGS
            "-DTPL_ENABLE_BoostLib:BOOL=ON" 
            "-DTPL_ENABLE_Boost:BOOL=ON" 
            "-DTPL_ENABLE_GLM:BOOL=OFF" 
            "-DTPL_BoostLib_INCLUDE_DIRS:FILEPATH=${TPL_INSTALL_PREFIX}/include"
            "-DBoostLib_LIBRARY_DIRS:FILEPATH=${TPL_INSTALL_PREFIX}/lib"
            "-DTPL_Boost_INCLUDE_DIRS:FILEPATH=${TPL_INSTALL_PREFIX}/include"
            "-DBoost_LIBRARY_DIRS:FILEPATH=${TPL_INSTALL_PREFIX}/lib")

# NetCDF
list(APPEND Trilinos_CMAKE_TPL_ARGS
            "-DTPL_ENABLE_Netcdf:BOOL=ON"
            "-DTPL_Netcdf_INCLUDE_DIRS:FILEPATH=${NetCDF_INCLUDE_DIRS}"
            "-DTPL_Netcdf_LIBRARIES:STRING=${NetCDF_C_LIBRARIES}")


# HYPRE
if( ENABLE_HYPRE )
  list(APPEND Trilinos_CMAKE_TPL_ARGS
              "-DTPL_ENABLE_HYPRE:BOOL=ON"
              "-DTPL_HYPRE_LIBRARIES:STRING=${HYPRE_LIBRARIES}"
              "-DTPL_HYPRE_INCLUDE_DIRS:FILEPATH=${TPL_INSTALL_PREFIX}/include")
endif()

#  - Addtional Trilinos CMake Arguments
set(Trilinos_CMAKE_EXTRA_ARGS
    "-DTrilinos_VERBOSE_CONFIGURE:BOOL=ON"
    "-DTrilinos_ENABLE_TESTS:BOOL=OFF"
    "-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON"
    "-DNOX_ENABLE_ABSTRACT_IMPLEMENTATION_THYRA:BOOL=OFF"
    "-DNOX_ENABLE_THYRA_EPETRA_ADAPTERS:BOOL=OFF"    
    )

if ( CMAKE_BUILD_TYPE )
  list(APPEND Trilinos_CMAKE_EXTRA_ARGS
              "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}")

  if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    list(APPEND Trilinos_CMAKE_EXTRA_ARGS
              "-DEpetra_ENABLE_FATAL_MESSAGES:BOOL=ON")
  endif()
  #message(DEBUG ": Trilinos_CMAKE_EXTRA_ARGS = ${Trilinos_CMAKE_EXTRA_ARGS}")
endif()

if ( BUILD_SHARED_LIBS )
  list(APPEND Trilinos_CMAKE_EXTRA_ARGS
    "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}")
  #message(DEBUG ": Trilinos_CMAKE_EXTRA_ARGS = ${Trilinos_CMAKE_EXTRA_ARGS}")
endif()


#  - Add CMake configuration file
if(Trilinos_Build_Config_File)
    list(APPEND Trilinos_Config_File_ARGS
        "-C${Trilinos_Build_Config_File}")
    message(STATUS "Will add ${Trilinos_Build_Config_File} to the Trilinos configure")    
    message(DEBUG "Trilinos_CMAKE_EXTRA_ARGS = ${Trilinos_CMAKE_EXTRA_ARGS}")
endif()    


#  - Final Trilinos CMake Arguments 
set(Trilinos_CMAKE_ARGS 
   ${Trilinos_CMAKE_PACKAGE_ARGS}
   ${Trilinos_CMAKE_TPL_ARGS}
   ${Trilinos_CMAKE_EXTRA_ARGS}
   )


#  --- Define the Trilinos patch step
#

# Trilinos patches
set(ENABLE_Trilinos_Patch ON)
if (ENABLE_Trilinos_Patch)
  set(Trilinos_patch_file trilinos-ifpack-hypre.patch trilinos-duplicate-parameters.patch trilinos-ifpack-hypre2.patch)
  configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/trilinos-patch-step.sh.in
                 ${Trilinos_prefix_dir}/trilinos-patch-step.sh
                 @ONLY)
  set(Trilinos_PATCH_COMMAND bash ${Trilinos_prefix_dir}/trilinos-patch-step.sh)
  message(STATUS "Applying trilinos patches")
else()
  set(Trilinos_PATCH_COMMAND)
  message(STATUS "Patch NOT APPLIED for trilinos")
endif()

# Trilinos needs a patch for GNU versions > 4.6
#LPRITCHif ( CMAKE_CXX_COMPILER_VERSION )
#LPRITCH  if ( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" )
#LPRITCH    if ( ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "4.6" )
#LPRITCH      set(ENABLE_Trilinos_Patch OFF)
#LPRITCH    else()
#LPRITCH      message(STATUS "Trilinos requires a patch when using"
#LPRITCH                     " GNU ${CMAKE_CXX_COMPILER_VERSION}")
#LPRITCH      set(ENABLE_Trilinos_Patch ON)
#LPRITCH    endif()
#LPRITCH  endif()
#LPRITCHendif()  
#LPRITCH
#LPRITCHset(Trilinos_PATCH_COMMAND)
#LPRITCHif (ENABLE_Trilinos_Patch)
#LPRITCH    set(Trilinos_patch_file)
#LPRITCH    # Set the patch file name
#LPRITCH    if(CMAKE_CXX_COMPILER_VERSION)
#LPRITCH      if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#LPRITCH        if ( "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.6" )
#LPRITCH          message(FATAL_ERROR "ENABLE_Trilinos_Patch is ON, however no patch file exists"
#LPRITCH                              " for version ${CMAKE_CXX_COMPILER_VERSION}.")
#LPRITCH        elseif( "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.7" )
#LPRITCH          set(Trilinos_patch_file trilinos-${Trilinos_VERSION}-gcc46.patch)
#LPRITCH        elseif ( "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.8" )
#LPRITCH          set(Trilinos_patch_file trilinos-${Trilinos_VERSION}-gcc47.patch)
#LPRITCH        else()
#LPRITCH          message(FATAL_ERROR "ENABLE_Trilinos_Patch is ON, however no patch file exists"
#LPRITCH                             " for version ${CMAKE_CXX_COMPILER_VERSION}.")
#LPRITCH        endif()
#LPRITCH      endif()
#LPRITCH    endif()
#LPRITCH
#LPRITCH    #print_variable(Trilinos_patch_file)
#LPRITCH    if(Trilinos_patch_file)
#LPRITCH       configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/trilinos-patch-step.sh.in
#LPRITCH                      ${Trilinos_prefix_dir}/trilinos-patch-step.sh
#LPRITCH                      @ONLY)
#LPRITCH       set(Trilinos_PATCH_COMMAND sh ${Trilinos_prefix_dir}/trilinos-patch-step.sh)
#LPRITCH    else()
#LPRITCH       message(WARNING "ENABLE_Trilinos_Patch is ON but no patch file found for "
#LPRITCH                       "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} "
#LPRITCH                       "Will not patch Trilinos.")
#LPRITCH    endif()                   
#LPRITCH                      
#LPRITCHendif()  
#print_variable(Trilinos_PATCH_COMMAND)

# --- Define the Trilinos location
set(Trilinos_install_dir ${TPL_INSTALL_PREFIX}/${Trilinos_BUILD_TARGET}-${Trilinos_VERSION})

# --- Add external project build and tie to the Trilinos build target
ExternalProject_Add(${Trilinos_BUILD_TARGET}
                    DEPENDS   ${Trilinos_PACKAGE_DEPENDS}             # Package dependency target
                    TMP_DIR   ${Trilinos_tmp_dir}                     # Temporary files directory
                    STAMP_DIR ${Trilinos_stamp_dir}                   # Timestamp and log directory
                    # -- Download and URL definitions
                    DOWNLOAD_DIR ${TPL_DOWNLOAD_DIR}                  # Download directory
                    URL          ${Trilinos_URL}                      # URL may be a web site OR a local file
                    URL_MD5      ${Trilinos_MD5_SUM}                  # md5sum of the archive file
                    # -- Patch
                    PATCH_COMMAND ${Trilinos_PATCH_COMMAND}
                    # -- Configure
                    SOURCE_DIR    ${Trilinos_source_dir}           # Source directory
                    CMAKE_ARGS          ${Trilinos_Config_File_ARGS}
                    CMAKE_CACHE_ARGS    ${Trilinos_CMAKE_ARGS} 
                                        -DCMAKE_C_FLAGS:STRING=${Amanzi_COMMON_CFLAGS}  # Ensure uniform build
                                        -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                                        -DCMAKE_CXX_FLAGS:STRING=${Amanzi_COMMON_CXXFLAGS}
                                        -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                                        -DCMAKE_Fortran_FLAGS:STRING=${Amanzi_COMMON_FCFLAGS}
                                        -DCMAKE_Fortran_COMPILER:FILEPATH=${CMAKE_Fortran_COMPILER}
                                        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
                                        -DTrilinos_ENABLE_Stratimikos:BOOL=FALSE
                                        -DTrilinos_ENABLE_SEACAS:BOOL=FALSE
                                        -DCMAKE_INSTALL_RPATH:PATH=${Trilinos_install_dir}/lib
                                        -DCMAKE_INSTALL_NAME_DIR:PATH=${Trilinos_install_dir}/lib
                    # -- Build
                    BINARY_DIR        ${Trilinos_build_dir}        # Build directory 
                    BUILD_COMMAND     $(MAKE)                      # $(MAKE) enables parallel builds through make
                    BUILD_IN_SOURCE   ${Trilinos_BUILD_IN_SOURCE}  # Flag for in source builds
                    # -- Install
                    INSTALL_DIR      ${Trilinos_install_dir}        # Install directory
                    # -- Output control
                    ${Trilinos_logging_args}
		    )

# --- Useful variables for packages that depends on Trilinos
global_set(Trilinos_INSTALL_PREFIX  ${Trilinos_install_dir})
global_set(Zoltan_INSTALL_PREFIX "${Trilinos_install_dir}")

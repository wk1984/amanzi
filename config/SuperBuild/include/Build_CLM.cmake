#  -*- mode: cmake -*-

#
# Build TPL:  CLM
#   
# --- Define all the directories and common external project flags
define_external_project_args(CLM
                             TARGET clm
                             BUILD_IN_SOURCE)



# add version to the autogenerated tpl_versions.h file
include(${SuperBuild_SOURCE_DIR}/TPLVersions.cmake)
amanzi_tpl_version_write(FILENAME ${TPL_VERSIONS_INCLUDE_FILE}
                         PREFIX CLM
                         VERSION ${CLM_VERSION_MAJOR} ${CLM_VERSION_MINOR} ${CLM_VERSION_PATCH})

# --- Define the arguments passed to CMake.
set(CLM_CMAKE_ARGS 
      "-DCMAKE_INSTALL_PREFIX:FILEPATH=${TPL_INSTALL_PREFIX}"
      "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
      "-DCMAKE_BUILD_TYPE:=${CMAKE_BUILD_TYPE}"
      "-DCMAKE_Fortran_FLAGS:STRING=-fPIC -w -Wno-unused-variable -ffree-line-length-0 -fbounds-check -g3 -funderscoring")

# --- Define the build command
# Build the build script
set(CLM_sh_build ${CLM_prefix_dir}/clm-build-step.sh)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/clm-build-step.sh.in
               ${CLM_sh_build}
               @ONLY)

# Configure the CMake command file
set(CLM_cmake_build ${CLM_prefix_dir}/clm-build-step.cmake)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/clm-build-step.cmake.in
               ${CLM_cmake_build}
               @ONLY)
set(CLM_CMAKE_COMMAND ${CMAKE_COMMAND} -P ${CLM_cmake_build})

# --- Define the install command
# Build the install script
set(CLM_sh_install ${CLM_prefix_dir}/clm-install-step.sh)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/clm-install-step.sh.in
               ${CLM_sh_install}
               @ONLY)

# Configure the CMake command file
set(CLM_cmake_install ${CLM_prefix_dir}/clm-install-step.cmake)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/clm-install-step.cmake.in
               ${CLM_cmake_install}
               @ONLY)
set(CLM_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${CLM_cmake_install})

# --- Add external project build and tie to the CLM build target
ExternalProject_Add(${CLM_BUILD_TARGET}
                    DEPENDS   ${CLM_PACKAGE_DEPENDS}           # Package dependency target
                    TMP_DIR   ${CLM_tmp_dir}                   # Temporary files directory
                    STAMP_DIR ${CLM_stamp_dir}                 # Timestamp and log directory
                    # -- Download and URL definitions     
                    DOWNLOAD_DIR   ${TPL_DOWNLOAD_DIR}
                    URL            ${CLM_URL}               # URL may be a web site OR a local file
                    URL_MD5        ${CLM_MD5_SUM}           # md5sum of the archive file
                    DOWNLOAD_NAME  ${CLM_SAVEAS_FILE}       # file name to store (if not end of URL)
                    # -- Update (one way to skip this step is use null command)
                    UPDATE_COMMAND ""
                    # -- Patch 
                    PATCH_COMMAND ${CLM_PATCH_COMMAND}
                    # -- Configure
                    SOURCE_DIR    ${CLM_source_dir}         # Source directory
                    CONFIGURE_COMMAND ""
                    CMAKE_ARGS    ${CLM_CMAKE_CACHE_ARGS}   # CMAKE_CACHE_ARGS or CMAKE_ARGS => CMake configure
                                  -DCMAKE_C_FLAGS:STRING=${Amanzi_COMMON_CFLAGS}  # Ensure uniform build
                                  -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                                  -DCMAKE_Fortran_COMPILER:FILEPATH=${CMAKE_Fortran_COMPILER}
                    # -- Build
                    # BINARY_DIR     ${CLM_build_dir}       # Build directory 
                    BUILD_COMMAND    ${CLM_CMAKE_COMMAND}   # $(MAKE) enables parallel builds through make
                    BUILD_IN_SOURCE  1                           # Flag for in source builds
                    # -- Install
                    INSTALL_DIR      ${TPL_INSTALL_PREFIX}       # Install directory
                    INSTALL_COMMAND  ${CLM_INSTALL_COMMAND} 
                    # -- Output control
                    ${CLM_logging_args})

include(BuildLibraryName)
build_library_name(clm CLM_LIB APPEND_PATH ${TPL_INSTALL_PREFIX}/lib)
global_set(CLM_INCLUDE_DIRS ${TPL_INSTALL_PREFIX})
global_set(CLM_DIR ${TPL_INSTALL_PREFIX})

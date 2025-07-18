cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(NukeOIDNDenoise)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  if(WIN32)
    # Was getting LNK4098: defaultlib 'MSCVRT' conflicts with use of other libs
    # It took many hours to figure out the -MT flag
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -DNDEBUG -MT")
  else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3 -DNDEBUG -Wall")
  endif()
endif()

# Windows' math include does not define constants by default.
# Set this definition so it does.
# Also set NOMINMAX so the min and max functions are not overwritten with macros.
IF(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
ENDIF()

# Set install RPATH
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")
# Build with with install RPATH
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
# Add RPATHS outside of the build tree to install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_MACOSX_RPATH TRUE)

# CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# /fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Plugin source code
add_subdirectory(src)

# Nuke home directory
if(WIN32 AND NOT CYGWIN)
  get_filename_component(Nuke_HOME_DIR "c:/Users/$ENV{USERNAME}/.nuke/" ABSOLUTE)
else()
  get_filename_component(Nuke_HOME_DIR "$ENV{HOME}/.nuke/" ABSOLUTE)
endif()

# Set default install prefix
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${Nuke_HOME_DIR}/oidnDenoise" CACHE PATH "Install path prefix" FORCE)
  message("CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
endif()

# Create init.py for $HOME/.nuke
if(EXISTS ${Nuke_HOME_DIR}/init.py)
  set(_pluginAddPath_FOUND "")
  file(READ ${Nuke_HOME_DIR}/init.py _nuke_INIT)
  if(_nuke_INIT)
    string(REGEX REPLACE "nuke.pluginAddPath\\(['\"]?(.+oidnDenoise\\/?)['\"]?\\)" "\\0;\\1;" _mw_PLUGIN_PATH_LIST ${_nuke_INIT})
    list(LENGTH _mw_PLUGIN_PATH_LIST _mw_PLUGIN_PATH_LIST_LENGTH)
    if(${_mw_PLUGIN_PATH_LIST_LENGTH} GREATER 1)
      list(GET _mw_PLUGIN_PATH_LIST 1 _mw_PLUGIN_PATH)
      string(REGEX MATCH ${CMAKE_INSTALL_PREFIX} _mw_PLUGIN_PATH_MATCH ${_mw_PLUGIN_PATH})
      if(_mw_PLUGIN_PATH_MATCH)
        set(_pluginAddPath_FOUND "FOUND")
      endif()
    endif()
  endif()
  if(NOT _pluginAddPath_FOUND)
    set(_nuke_INIT "${_nuke_INIT}nuke.pluginAddPath('${CMAKE_INSTALL_PREFIX}')\n\n")
  endif()
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/init.py ${_nuke_INIT})
else()
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/init.py "import nuke\n\nnuke.pluginAddPath('${CMAKE_INSTALL_PREFIX}')\n\n")
endif()


install(FILES scripts/menu.py DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES images/icon.png DESTINATION ${CMAKE_INSTALL_PREFIX})
# this is only for CI build to pick up and package menu.py with the plugin release
install(FILES scripts/menu.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/init.py DESTINATION ${Nuke_HOME_DIR})

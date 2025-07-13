if(NOT DEFINED OIDN_ROOT)
  if(WIN32)
    set(OIDN_ROOT "C:/oidn")
  elseif(APPLE)
    set(OIDN_ROOT "/opt/homebrew/Cellar/open-image-denoise")
    file(GLOB OIDN_VERSIONED_DIRS "${OIDN_ROOT}/*")
    if(OIDN_VERSIONED_DIRS)
      list(GET OIDN_VERSIONED_DIRS 0 OIDN_ROOT) # Take the first match
    endif()
  else()
    set(OIDN_ROOT "/usr/local/lib64")
  endif()
endif()

message("OIDN_ROOT=${OIDN_ROOT}")
find_path(
  OpenImageDenoise_INCLUDE_DIR
  NAMES OpenImageDenoise/oidn.hpp
  PATHS ${OIDN_ROOT}/include
  NO_DEFAULT_PATH
)

message("OpenImageDenoise_INCLUDE_DIR=${OpenImageDenoise_INCLUDE_DIR}")


if(OpenImageDenoise_INCLUDE_DIR)
  include_directories(${OpenImageDenoise_INCLUDE_DIR})
endif()

find_library(
  OpenImageDenoise_LIBRARY
  NAMES OpenImageDenoise
  PATHS ${OIDN_ROOT}/lib
  NO_DEFAULT_PATH
)

message("OpenImageDenoise_LIBRARY=${OpenImageDenoise_LIBRARY}")


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenImageDenoise
  FOUND_VAR
    OpenImageDenoise_FOUND
  REQUIRED_VARS
    OIDN_ROOT
    OpenImageDenoise_INCLUDE_DIR
    OpenImageDenoise_LIBRARY
)

if(OpenImageDenoise_FOUND AND NOT TARGET OpenImageDenoise::OpenImageDenoise)
    add_library(OpenImageDenoise::OpenImageDenoise UNKNOWN IMPORTED)
    set_target_properties(OpenImageDenoise::OpenImageDenoise PROPERTIES
        IMPORTED_LOCATION "${OpenImageDenoise_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OpenImageDenoise_INCLUDE_DIR}")
endif()
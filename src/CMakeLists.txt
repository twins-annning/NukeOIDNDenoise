# Nuke
find_package(Nuke REQUIRED)

# OpenImageDenoise
find_package(OpenImageDenoise REQUIRED)

add_library(oidnDenoise SHARED 
  oidnDenoise.cpp
)

target_link_libraries(oidnDenoise PUBLIC 
  Nuke::NDK
  OpenImageDenoise::OpenImageDenoise
)

set_target_properties(oidnDenoise PROPERTIES 
  PREFIX ""
  INSTALL_RPATH "$ORIGIN/../lib"
)

install(
  TARGETS
    oidnDenoise
  DESTINATION
    plugins
  COMPONENT
    Nuke
)

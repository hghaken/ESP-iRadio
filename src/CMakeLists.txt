# Only used by the experimental PlatformIO env `idf` (see docs/framework-build.md).
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# BLE Mesh is switched off in sdkconfig.defaults, so the link-time stubs are not needed.
list(FILTER app_sources EXCLUDE REGEX ".*bt_mesh_stubs[.]c$")

idf_component_register(SRCS ${app_sources})

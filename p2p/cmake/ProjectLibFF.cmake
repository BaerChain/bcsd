include(ProjectMPIR)

set(prefix "${CMAKE_SOURCE_DIR}/deps")
set(libff_library "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ff${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(libff_inlcude_dir "${prefix}/include/libff")

#add_dependencies(libff mpir)
# Create snark imported library
add_library(libff::ff STATIC IMPORTED)
file(MAKE_DIRECTORY ${libff_inlcude_dir})
set_property(TARGET libff::ff PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET libff::ff PROPERTY IMPORTED_LOCATION_RELEASE ${libff_library})
set_property(TARGET libff::ff PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${libff_inlcude_dir})
set_property(TARGET libff::ff PROPERTY INTERFACE_LINK_LIBRARIES MPIR::mpir)
add_dependencies(libff::ff libff)

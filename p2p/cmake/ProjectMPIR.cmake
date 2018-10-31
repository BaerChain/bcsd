include(ExternalProject)

set(prefix "${CMAKE_SOURCE_DIR}/deps")
set(MPIR_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mpir${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(MPIR_INCLUDE_DIR "${prefix}/include")

add_library(MPIR::mpir STATIC IMPORTED)
set_property(TARGET MPIR::mpir PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET MPIR::mpir PROPERTY IMPORTED_LOCATION_RELEASE ${MPIR_LIBRARY})
set_property(TARGET MPIR::mpir PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MPIR_INCLUDE_DIR})
add_dependencies(MPIR::mpir mpir)

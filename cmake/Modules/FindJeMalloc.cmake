# Jemalloc library find, cherry picked from cmake project/github
find_path(JEMALLOC_ROOT_DIR NAMES include/jemalloc/jemalloc.h)
find_library(JEMALLOC_LIBRARIES NAMES jemalloc HINTS ${JEMALLOC_ROOT_DIR}/lib)
find_path(JEMALLOC_INCLUDE_DIR NAMES jemalloc/jemalloc.h HINTS ${JEMALLOC_ROOT_DIR}/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JeMalloc DEFAULT_MSG JEMALLOC_LIBRARIES JEMALLOC_INCLUDE_DIR)

mark_as_advanced(JEMALLOC_ROOT_DIR JEMALLOC_LIBRARIES JEMALLOC_INCLUDE_DIR)

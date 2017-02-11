add_definitions(-std=c++11)
include_directories(..)
add_library(dacloudcpp ${LIBTYPE} dacloud.cpp)
target_link_libraries(dacloudcpp dacloud ${JEMALLOC_LIBRARIES})
set_target_properties(dacloudcpp PROPERTIES SOVERSION 1.0)

install(TARGETS dacloudcpp DESTINATION /usr/local/lib)
install(FILES dacloud.hpp DESTINATION /usr/local/include)

subdirs(examples)

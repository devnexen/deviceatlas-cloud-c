cmake_minimum_required(VERSION 2.6)

# mandatory libraries
find_path(HAVE_CONFIG libconfig.h)
find_path(HAVE_CURL curl/curl.h)
find_path(HAVE_JANSSON jansson.h)

find_package(PkgConfig REQUIRED)

if (NOT NOGLIB2)
    pkg_check_modules(GLIB2 glib-2.0)
endif()

# memcached cache provider
find_path(HAVE_MEMCACHED libmemcached/memcached.h)

# CUnit
find_path(HAVE_CUNIT CUnit/CUnit.h)

set(CACHELIBS "")

if (${HAVE_CONFIG} STREQUAL "HAVE_CONFIG-NOTFOUND")
    message(FATAL_ERROR "Requires Libconfig")
endif()

if (${HAVE_CURL} STREQUAL "HAVE_CURL-NOTFOUND")
    message(FATAL_ERROR "Requires LibCURL")
endif()

if (${HAVE_JANSSON} STREQUAL "HAVE_JANSSON-NOTFOUND")
    message(FATAL_ERROR "Requires LibJansson")
endif()

if (NOT ${HAVE_MEMCACHED} STREQUAL "HAVE_MEMCACHED-NOTFOUND")
    add_definitions(-DHAVE_MEMCACHED)
    message(STATUS "Memcached support")
    find_library(MEMCACHED memcached)
    find_library(MEMCACHEDUTIL memcachedutil)
    set(CACHELIBS ${MEMCACHED} ${MEMCACHEDUTIL})
endif()

if (GLIB2_FOUND AND NOT NOGLIB2)
    add_definitions(-DHAVE_GLIB)
    message(STATUS "Memory cache (Glib / HashTable) support")
    include_directories(${GLIB2_INCLUDE_DIRS})
    set(CACHELIBS ${CACHELIBS} ${GLIB2_LIBRARIES})
else()
    message(STATUS "Memory cache (Simple Tail Queue) support")
endif()

find_library(CONFIG config)
find_library(CURL curl)
find_library(JANSSON jansson)
find_library(CRYPTO crypto)

if (NOT LIBTYPE)
    set(LIBTYPE SHARED)	
endif()

message(STATUS "${LIBTYPE} library version")

file(GLOB SRCS *.c)
include_directories(${HAVE_CURL})
add_library(dacloud ${LIBTYPE} ${SRCS})
target_link_libraries(dacloud ${CONFIG} ${CURL} ${JANSSON} ${CRYPTO} ${CACHELIBS})
set_target_properties(dacloud PROPERTIES SOVERSION 1.0)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++11" HAS_CXX11)

subdirs(examples)
if (HAS_CXX11)
    subdirs(cppwrapper)
endif()

if (NOT ${HAVE_CUNIT} STREQUAL "HAVE_CUNIT-NOTFOUND")
    find_library(CUNIT cunit)
    message(STATUS "CUnit support")
    subdirs(tests)
endif()

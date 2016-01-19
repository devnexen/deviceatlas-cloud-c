cmake_minimum_required(VERSION 2.6)

# mandatory libraries
find_path(HAVE_CONFIG libconfig.h)
find_path(HAVE_CURL curl/curl.h)
find_path(HAVE_JANSSON jansson.h)

# memcached cache provider
find_path(HAVE_MEMCACHED libmemcached/memcached.h)

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

subdirs(examples)

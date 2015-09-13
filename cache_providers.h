#ifndef  CACHE_PROVIDERS_H
#define  CACHE_PROVIDERS_H

#include "file_cache_provider.h"

#ifdef   HAVE_MEMCACHED
#include "memcached_cache_provider.h"
#include "redis_cache_provider.h"
#endif

#define  CACHE_SET(cops, name)         	\
    cops->init = name ## _cache_init; 	\
    cops->get = name ## _cache_get;     \
    cops->set = name ## _cache_set;     \
    cops->fini = name ## _cache_fini

#endif

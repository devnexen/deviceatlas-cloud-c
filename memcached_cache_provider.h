#ifdef   HAVE_MEMCACHED
#ifndef  MEMCACHED_CACHE_PROVIDER_H
#define  MEMCACHED_CACHE_PROVIDER_H

#include "dacloud_cache.h"

int memcached_cache_init(struct da_cloud_cache_cfg *);
int memcached_cache_get(struct da_cloud_cache_cfg *, const char *, char **);
int memcached_cache_set(struct da_cloud_cache_cfg *, const char *, const char *);
void memcached_cache_fini(struct da_cloud_cache_cfg *);

#endif
#endif

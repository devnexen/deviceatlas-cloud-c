#ifdef	HAVE_HIREDIS
#ifndef  REDIS_CACHE_PROVIDER_H
#define  REDIS_CACHE_PROVIDER_H

#include "dacloud_cache.h"

int redis_cache_init(struct da_cloud_cache_cfg *);
int redis_cache_get(struct da_cloud_cache_cfg *, const char *, char **);
int redis_cache_set(struct da_cloud_cache_cfg *, const char *, const char *);
void redis_cache_fini(struct da_cloud_cache_cfg *);

#endif
#endif

#ifndef  FILE_CACHE_PROVIDER_H
#define  FILE_CACHE_PROVIDER_H

#include "dacloud_cache.h"

int file_cache_init(struct da_cloud_cache_cfg *);
int file_cache_get(struct da_cloud_cache_cfg *, const char *, char **);
int file_cache_set(struct da_cloud_cache_cfg *, const char *, const char *);
void file_cache_fini(struct da_cloud_cache_cfg *);

#endif


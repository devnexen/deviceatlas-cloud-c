#ifndef  DACLOUD_CACHE_H
#define  DACLOUD_CACHE_H

#include <time.h>

struct da_cloud_cache_cfg {
    void *data;
    void *cache_obj;
    char *cache_cfg_str;
    size_t expiration;
};

struct da_cloud_cache_ops {
    int (*init)(struct da_cloud_cache_cfg *);
    int (*get)(struct da_cloud_cache_cfg *, const char *, char **);
    int (*set)(struct da_cloud_cache_cfg *, const char *, const char *);
    void (*fini)(struct da_cloud_cache_cfg *);
};

void
da_cloud_crypt_key(char *, size_t, char *, size_t);

int mock_cache_init(struct da_cloud_cache_cfg *);
int mock_cache_get(struct da_cloud_cache_cfg *, const char *, char **);
int mock_cache_set(struct da_cloud_cache_cfg *, const char *, const char *);
void mock_cache_fini(struct da_cloud_cache_cfg *);

void cache_set(struct da_cloud_cache_ops *, const char *);

#include "cache_providers.h"

#endif

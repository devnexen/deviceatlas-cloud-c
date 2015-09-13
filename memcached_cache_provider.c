#ifdef   HAVE_MEMCACHED
#include <string.h>

#include <libmemcached/memcached.h>
#include <libmemcached/util.h>

#include "memcached_cache_provider.h"

int
memcached_cache_init(struct da_cloud_cache_cfg *cfg) {
    cfg->cache_obj = NULL;
    cfg->data = memcached_create(NULL);
    if (cfg->data == NULL)
        return (-1);
    memcached_server_st *servers = memcached_servers_parse(cfg->cache_cfg_str);
    memcached_server_push(cfg->data, servers);
    cfg->cache_obj = memcached_pool_create(cfg->data, 1, 5);
    memcached_server_free(servers);
    return (0);
}

int
memcached_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) {
    memcached_return_t ret = MEMCACHED_FAILURE;
    if (cfg->cache_obj != NULL && value != NULL) {
        memcached_st *client = NULL;
        size_t vlen;
        uint32_t flags;
        client = memcached_pool_pop(cfg->cache_obj, 1, &ret);

        if (ret == MEMCACHED_SUCCESS)
            *value = memcached_get(client, key, strlen(key),
                    &vlen, &flags, &ret);
        memcached_pool_push(cfg->cache_obj, client);
    }

    return (ret == MEMCACHED_SUCCESS ? 0 : -1);
}

int
memcached_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) {
    memcached_return_t ret = MEMCACHED_FAILURE;
    if (cfg->cache_obj != NULL) {
        memcached_st *client = NULL;
        memcached_pool_st *pl;
        client = memcached_pool_pop(cfg->cache_obj, 1, &ret);

        if (ret == MEMCACHED_SUCCESS)
            ret = memcached_set(client, key, strlen(key), value, strlen(value),
                    (time_t)(time(NULL) + cfg->expiration), 0);

        memcached_pool_push(cfg->cache_obj, client);
    }

    return (ret == MEMCACHED_SUCCESS ? 0 : -1);
}

void
memcached_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL) {
        memcached_free((memcached_st *)cfg->data);
        memcached_pool_destroy((memcached_pool_st *)cfg->cache_obj);
    }
}

#endif

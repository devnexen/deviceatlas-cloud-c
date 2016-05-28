#include <string.h>
#ifdef   HAVE_MEMCACHED

#include <libmemcached/memcached.h>
#include <libmemcached/util.h>

#include "memcached_cache_provider.h"
#include "dacloud_util.h"

const char *
memcached_cache_id(void) {
    return "memcached";
}

int
memcached_cache_init(struct da_cloud_cache_cfg *cfg) {
    memcached_server_st *servers;
    int num_cores;
    cfg->cache_obj = NULL;
    cfg->data = memcached_create(NULL);
    if (cfg->data == NULL) {
        da_cloud_log(cfg->efp, "could not allocate data structure");
        return (-1);
    }

    servers = memcached_servers_parse(cfg->cache_cfg_str);
    if (servers == NULL) {
        da_cloud_log(cfg->efp, "could not create server instance");
        memcached_free((memcached_st *)cfg->data);
        return (-1);
    } else {
        cfg->cache_dcm = da_cloud_membuf_create(1024 * 4);
        if (cfg->cache_dcm == NULL) {
            da_cloud_log(cfg->efp, "could not allocate mem pool");
            return (-1);
        }
        cfg->cache_root = cfg->cache_dcm;
        num_cores = da_cloud_get_num_cores();
        memcached_server_push(cfg->data, servers);
        cfg->cache_obj = memcached_pool_create(cfg->data, 1, num_cores);
        memcached_server_free(servers);
    }

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

        if (ret == MEMCACHED_SUCCESS) {
            char *svalue = memcached_get(client, key, strlen(key),
                    &vlen, &flags, &ret);
            if (svalue != NULL) {
                *value = da_cloud_membuf_strdup(&cfg->cache_dcm, svalue);
                free(svalue);
                if (*value == NULL)
                    return (-1);
            }
        }
        memcached_pool_push(cfg->cache_obj, client);
    }

    return (ret == MEMCACHED_SUCCESS ? 0 : -1);
}

int
memcached_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) {
    memcached_return_t ret = MEMCACHED_FAILURE;
    if (cfg->cache_obj != NULL) {
        memcached_st *client = NULL;
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

    da_cloud_membuf_free(cfg->cache_root);
}

#endif

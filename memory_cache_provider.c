#ifdef   HAVE_GLIB
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>

#include <glib.h>

#include "memory_cache_provider.h"

#define HASH_THRESHOLD      4096
#define HASH_MIN            128

const char *
memory_cache_id(void) {
    return "memory";
}

int
memory_cache_init(struct da_cloud_cache_cfg *cfg) {
    GHashTable *hash = NULL;
    cfg->cache_obj = NULL;
    guint *threshold;

    threshold = calloc(1, sizeof(*threshold));
    if (threshold == NULL) {
        da_cloud_log(cfg->efp, "could not allocate threshold data");
        return (-1);
    }

    hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    if (hash == NULL) {
        free(threshold);
        return (-1);
    }

    *threshold = HASH_THRESHOLD;
    cfg->cache_obj = hash;
    if (cfg->cache_cfg_str != NULL) {
        long temp = strtol(cfg->cache_cfg_str, 0, 10);
        if (temp >= HASH_MIN && temp < INT_MAX)
            *threshold = (int)temp;
    }

    cfg->data = (void *)threshold;

    return (0);
}

int
memory_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) {
    int ret = -1;
    guint threshold;
    gconstpointer g_key = NULL;
    gpointer g_value = NULL;
    if (cfg->cache_obj != NULL && value != NULL) {
        threshold = *((guint *)cfg->data);
        if (g_hash_table_size(cfg->cache_obj) >= threshold) {
            g_hash_table_remove_all(cfg->cache_obj);
            return (ret);
        }

        g_key = key;
        g_value	= g_hash_table_lookup(cfg->cache_obj, g_key);
        if (g_value != NULL) {
            *value = strdup((char *)g_value);
            ret = 0;
        }
    }

    return (ret);
}

int
memory_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) {
    int ret = -1;
    gpointer g_key = NULL;
    gpointer g_value = NULL;
    if (cfg->cache_obj != NULL) {
        g_key = g_strdup(key);
        g_value = g_strdup(value);
        g_hash_table_insert(cfg->cache_obj, g_key, g_value);
        ret = 0;
    }

    return (ret);
}

void
memory_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL) {
        free(cfg->data);
        g_hash_table_destroy(cfg->cache_obj);
    }
}

#endif

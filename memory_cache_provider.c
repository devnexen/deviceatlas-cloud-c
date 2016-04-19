#ifdef   HAVE_GLIB
#include <string.h>

#include <glib.h>

#include "memory_cache_provider.h"

const char *
memory_cache_id(void) {
    return "memory";
}

int
memory_cache_init(struct da_cloud_cache_cfg *cfg) {
    GHashTable *hash = NULL;
    cfg->cache_obj = NULL;

    hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    if (hash == NULL) {
        return (-1);
    }

    cfg->cache_obj = hash;

    return (0);
}

int
memory_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) {
    int ret = -1;
    gconstpointer g_key = NULL;
    gpointer g_value = NULL;
    if (cfg->cache_obj != NULL && value != NULL) {
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
        g_hash_table_insert((GHashTable *)cfg->cache_obj, g_key, g_value);
        ret = 0;
    }

    return (ret);
}

void
memory_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL)
        g_hash_table_destroy((GHashTable *)cfg->cache_obj);
}

#endif

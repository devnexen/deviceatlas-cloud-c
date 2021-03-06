#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>

#include "memory_cache_provider.h"

#define HASH_THRESHOLD      4096
#define HASH_MIN            128

#ifdef  HAVE_GLIB
#include <glib.h>

const char *
memory_cache_id(void) {
    return "memory";
}

int
memory_cache_init(struct da_cloud_cache_cfg *cfg) {
    GHashTable *hash = NULL;
    cfg->cache_obj = NULL;
    guint *threshold;

    cfg->cache_dcm = da_cloud_membuf_create(1024 * 4);
    if (cfg->cache_dcm == NULL) {
        da_cloud_log(cfg->efp, "could not allocate mem pool");
        return (-1);
    }
    cfg->cache_root = cfg->cache_dcm;
    threshold = da_cloud_membuf_alloc(&cfg->cache_dcm, sizeof(*threshold));
    if (threshold == NULL) {
        da_cloud_log(cfg->efp, "could not allocate threshold data");
        return (-1);
    }

    hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    if (hash == NULL) {
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
    MTX_INIT

    return (0);
}

int
memory_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) {
    int ret = -1;
    guint threshold;
    gconstpointer g_key = NULL;
    gpointer g_value = NULL;
    if (cfg->cache_obj != NULL && value != NULL) {
        pthread_mutexattr_t attr;
        pthread_mutex_t mtx;
        MTX_LOCK
        threshold = *((guint *)cfg->data);
        if (g_hash_table_size(cfg->cache_obj) >= threshold) {
            g_hash_table_remove_all(cfg->cache_obj);
            MTX_UNLOCK
            return (ret);
        }

        g_key = key;
        g_value = g_hash_table_lookup(cfg->cache_obj, g_key);
        if (g_value != NULL) {
            *value = g_allocator.strdup(g_allocator.child_ctx, (const char *)g_value);
            if (*value != NULL)
                ret = 0;
        }
        MTX_UNLOCK
    }

    return (ret);
}

int
memory_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) {
    int ret = -1;
    gpointer g_key = NULL;
    gpointer g_value = NULL;
    if (cfg->cache_obj != NULL) {
        pthread_mutexattr_t attr;
        pthread_mutex_t mtx;
        MTX_LOCK
        g_key = g_strdup(key);
        g_value = g_strdup(value);
        if (!g_hash_table_contains(cfg->cache_obj, (gconstpointer)g_key)) {
            g_hash_table_insert(cfg->cache_obj, g_key, g_value);
        }
        ret = 0;
        MTX_UNLOCK
    }

    return (ret);
}

void
memory_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL) {
        g_hash_table_destroy(cfg->cache_obj);
    }

    da_cloud_membuf_free(cfg->cache_root);
    MTX_DISPOSE
}

#else
#include <sys/queue.h>

struct memory_cache_entry {
    char *key;
    char *value;
    TAILQ_ENTRY(memory_cache_entry) entry;
};

struct memory_cache {
    size_t cnt;
    TAILQ_HEAD(entry, memory_cache_entry) entries;
};

static void
memory_cache_removeall(struct memory_cache *mc) {
    struct memory_cache_entry *mce = TAILQ_FIRST(&mc->entries);
    while (!TAILQ_EMPTY(&mc->entries)) {
        TAILQ_REMOVE(&mc->entries, mce, entry);
        mce = TAILQ_FIRST(&mc->entries);
    }
}

const char *
memory_cache_id(void) {
    return "memory";
}

int
memory_cache_init(struct da_cloud_cache_cfg *cfg) {
    struct memory_cache *mc = NULL;
    long temp;
    int *threshold = NULL;
    cfg->cache_obj = NULL;

    cfg->cache_dcm = da_cloud_membuf_create(1024 * 4);
    if (cfg->cache_dcm == NULL) {
        da_cloud_log(cfg->efp, "could not allocate mem pool");
        return (-1);
    }
    cfg->cache_root = cfg->cache_dcm;
    threshold = da_cloud_membuf_alloc(&cfg->cache_dcm, sizeof(*threshold));
    if (threshold == NULL) {
        da_cloud_log(cfg->efp, "could not allocate threshold");
        return (-1);
    }

    mc = da_cloud_membuf_alloc(&cfg->cache_dcm, sizeof(struct memory_cache));
    if (mc == NULL) {
        da_cloud_log(cfg->efp, "could not allocate tailq");
        return (-1);
    }

    *threshold = HASH_THRESHOLD;
    if (cfg->cache_cfg_str != NULL) {
        temp = strtol(cfg->cache_cfg_str, 0, 10);
        if (temp >= HASH_MIN && temp < INT_MAX)
            *threshold = (int)temp;
    }

    TAILQ_INIT(&mc->entries);
    cfg->data = (void *)threshold;
    cfg->cache_obj = mc;

    return (0);
}

int
memory_cache_get(struct da_cloud_cache_cfg *cfg, const char *key,
        char **value) {
    int ret = -1;
    int threshold;
    struct memory_cache *mc = NULL;
    struct memory_cache_entry *mce;

    if (cfg->cache_obj != NULL && value != NULL) {
        mc = cfg->cache_obj;
        threshold = *((int *)cfg->data);
        if (mc->cnt >= HASH_THRESHOLD) {
            memory_cache_removeall(mc);
            return (ret);
        }

        TAILQ_FOREACH(mce, &mc->entries, entry) {
            if (strcmp(key, mce->key) == 0) {
                *value = g_allocator.strdup(g_allocator.child_ctx, mce->value);
                if (*value == NULL)
                    return (ret);
                ret = 0;
                break;
            }
        }
    }

    return (ret);
}

int
memory_cache_set(struct da_cloud_cache_cfg *cfg, const char *key,
        const char *value) {
    int ret = -1;
    struct memory_cache *mc = NULL;
    struct memory_cache_entry *mce = NULL;

    if (cfg->cache_obj != NULL) {
        mc = cfg->cache_obj;
        mce = da_cloud_membuf_alloc(&cfg->cache_dcm, sizeof(*mce));
        if (mce == NULL)
            return (ret);
        mce->key = da_cloud_membuf_strdup(&cfg->cache_dcm, key);
        if (mce->key == NULL)
            return (ret);
        mce->value = da_cloud_membuf_strdup(&cfg->cache_dcm, value);
        if (mce->value == NULL)
            return (ret);
        TAILQ_INSERT_TAIL(&mc->entries, mce, entry);
        mc->cnt ++;
        ret = 0;
    }

    return (ret);
}

void
memory_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL) {
        struct memory_cache *mc = cfg->cache_obj;
        memory_cache_removeall(mc);
    }

    da_cloud_membuf_free(cfg->cache_root);
}

#endif

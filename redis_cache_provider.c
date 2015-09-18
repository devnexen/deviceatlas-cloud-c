#ifdef   HAVE_HIREDIS
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>

#include <hiredis/hiredis.h>

#include "redis_cache_provider.h"

static int
redis_cache_setunixsock(void **cache_obj, char *cfg_str) {
    char *sockpath = strchr(cfg_str, ' ');
    if (sockpath != NULL) {
        redisContext *ctx;
        while (isspace(*sockpath) && sockpath++);
        ctx = redisConnectUnix(sockpath);
        if (ctx == NULL) {
            fprintf(stderr, "could not set '%s' unix socket path\n", sockpath);
            return (-1);
        }

        *cache_obj = ctx;
        return (0);
    }

    return (-1);
}

static int
redis_cache_settcp(void **cache_obj, char *cfg_str) {
    redisContext *ctx;
    char *strport = strchr(cfg_str, ' ');
    int port = 6379;
    if (strport != NULL && *(strport + 1) != 0) {
        int tmp;
        size_t pos = strport - cfg_str;
        strport ++;
        tmp = strtol(strport, 0, 10);
        if (tmp >= 1024 && tmp <= 65535)
            port = tmp;
        cfg_str[pos] = 0;
    }
    
    ctx = redisConnect(cfg_str, port);
    if (ctx == NULL) {
        fprintf(stderr, "could not set to '%s' host, port '%d'", cfg_str, port);
        return (-1);
    }

    *cache_obj = ctx;
    return (0);
}

int
redis_cache_init(struct da_cloud_cache_cfg *cfg) {
    char *cfg_str = cfg->cache_cfg_str;
    cfg->cache_obj = NULL;
    if (strncasecmp(cfg_str, "unixsocket", 10) == 0)
        return (redis_cache_setunixsock(&cfg->cache_obj, cfg_str));
    else
        return (redis_cache_settcp(&cfg->cache_obj, cfg_str));
}

int
redis_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) {
    if (cfg->cache_obj != NULL && value != NULL) {
        redisReply *ret;
        pthread_mutex_t mtx;
        if (pthread_mutex_init(&mtx, NULL) != 0) {
            fprintf(cfg->efp, "cannot lock the cache\n");
            return (-1);
        }

        pthread_mutex_lock(&mtx);
        ret = redisCommand(cfg->cache_obj, "GET %s", key);
        pthread_mutex_unlock(&mtx);
        pthread_mutex_destroy(&mtx);
        if (ret != NULL) {
            if (ret->str != NULL)
                *value = strdup(ret->str);
            freeReplyObject(ret);
        }
    }

    return (-1);
}

int
redis_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) {
    if (cfg->cache_obj != NULL) {
        redisReply *ret;
        pthread_mutex_t mtx;
        if (pthread_mutex_init(&mtx, NULL) != 0) {
            fprintf(cfg->efp, "cannot lock the cache\n");
            return (-1);
        }

        pthread_mutex_lock(&mtx);
        ret = redisCommand(cfg->cache_obj, "SETEX %s %d %s", key, (int)cfg->expiration, value);
        pthread_mutex_unlock(&mtx);
        pthread_mutex_destroy(&mtx);
        if (ret != NULL) {
            freeReplyObject(ret);
            return (0);
        } else {
            fprintf(cfg->efp, "could not set cache: %s\n", ((redisContext *)cfg->cache_obj)->errstr);
        }
    }

    return (-1);
}

void
redis_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL)
        redisFree(cfg->cache_obj);
}

#endif

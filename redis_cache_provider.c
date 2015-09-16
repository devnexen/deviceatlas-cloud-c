#ifdef   HAVE_HIREDIS
#include <string.h>
#include <stdlib.h>

#include <hiredis/hiredis.h>

#include "redis_cache_provider.h"

static int
redis_cache_setunixsock(void **cache_obj, char *cfg_str) {
    char *sockpath = strchr(cfg_str, ':');
    if (sockpath != NULL && ++ *sockpath != 0) {
        *cache_obj = redisConnectUnix(sockpath);
        return (*cache_obj != NULL ? 0 : -1);
    }

    return (-1);
}

static int
redis_cache_settcp(void **cache_obj, char *cfg_str) {
    char *strport = strchr(cfg_str, ':');
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
    
    *cache_obj = redisConnect(cfg_str, port);
    return (*cache_obj != NULL ? 0 : -1);
}

int
redis_cache_init(struct da_cloud_cache_cfg *cfg) {
    char *cfg_str = cfg->cache_cfg_str;
    cfg->cache_obj = NULL;
    if (strncasecmp(cfg_str, "unix", 4) == 0 && strlen(cfg_str) > 5) {
        return (redis_cache_setunixsock(&cfg->cache_obj, cfg_str));
    } else {
        return (redis_cache_settcp(&cfg->cache_obj, cfg_str));
    }
}

int
redis_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) {
    if (cfg->cache_obj != NULL && value != NULL) {
        redisReply *ret;
        ret = redisCommand(cfg->cache_obj, "GET %s", key);
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
        ret = redisCommand(cfg->cache_obj, "SETEX %s %d %s", key, (int)cfg->expiration, value);
        if (ret != NULL) {
            freeReplyObject(ret);
            return (0);
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

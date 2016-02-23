#include <sys/stat.h>
#include <sys/file.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "file_cache_provider.h"

#define FILE_CACHE_ATTEMPT(mode)                        \
    while ((cache = fopen(fcfg->dir, mode)) == NULL) {  \
        usleep(1000000);                                \
        ++ i;                                           \
        if (i == 3)                                     \
        break;                                          \
    }                                                   \

#define FILE_MTX_DISPOSE                                \
    pthread_mutex_unlock(&mtx);                         \
    pthread_mutex_destroy(&mtx)

struct file_cache_cfg {
    size_t dirlen;
    char dir[PATH_MAX];
};

static void
file_cache_setumask(mode_t *m) {
    *m = umask(0);
    umask(*m);
}

static int
file_cache_mkdir(char *dir, size_t dirlen, const char *key, int creat, mode_t m) {
    dir[dirlen] = 0;
    strcat(dir, "/");
    strncat(dir, key, 1);
    if (creat) {
        if (mkdir(dir, 0777 & ~m) != 0)
            return (-1);
    }
    strcat(dir, "/");
    strcat(dir, key + 1);    

    return (0);
}

const char *
file_cache_id(void) {
    return "file";
}

int
file_cache_init(struct da_cloud_cache_cfg *cfg) {
    struct file_cache_cfg *fcfg;
    struct stat s;
    size_t cache_cfg_strlen = strlen(cfg->cache_cfg_str);
    cfg->cache_obj = NULL;
    if (cache_cfg_strlen == 0) {
        da_cloud_log(cfg->efp, "directory cannot be empty", NULL);
        return (-1);
    }

    if (cache_cfg_strlen >= (PATH_MAX - 67)) {
        da_cloud_log(cfg->efp, "directory '%s' too long", cfg->cache_cfg_str, NULL);
        return (-1);
    }

    memset(&s, 0, sizeof(s));
    if (stat(cfg->cache_cfg_str, &s) != 0) {
        da_cloud_log(cfg->efp, "directory '%s' not found", cfg->cache_cfg_str, NULL);
        return (-1);
    }
    if ((s.st_mode & S_IFMT) != S_IFDIR) {
        da_cloud_log(cfg->efp, "directory '%s' invalid", cfg->cache_cfg_str, NULL);
        return (-1);
    } else if (!(s.st_mode & S_IWUSR) && !(s.st_mode & S_IWGRP) && !(s.st_mode & S_IWOTH)) {
        da_cloud_log(cfg->efp, "directory '%s' permission denied", cfg->cache_cfg_str, NULL);
        return (-1);
    }

    fcfg = malloc(sizeof(*fcfg));
    if (fcfg == NULL) {
        da_cloud_log(cfg->efp, "could not allocate data structure", NULL);
        return (-1);
    }

    fcfg->dirlen = strlen(cfg->cache_cfg_str);
    strncpy(fcfg->dir, cfg->cache_cfg_str, fcfg->dirlen);
    fcfg->dir[fcfg->dirlen] = '\0';
    cfg->cache_obj = fcfg;

    return (0);
}

int
file_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) {
    if (cfg->cache_obj != NULL && value != NULL) {
        FILE *cache = NULL;
        pthread_mutex_t mtx;
        mode_t m;
        struct stat s;
        size_t valuelen, i = 0;
        int cachefd = -1;
        struct file_cache_cfg *fcfg = cfg->cache_obj;
        if (pthread_mutex_init(&mtx, NULL) != 0) {
            da_cloud_log(cfg->efp, "could not lock", NULL);
            return (-1);
        }

        pthread_mutex_lock(&mtx);

        file_cache_setumask(&m);
        if (file_cache_mkdir(fcfg->dir, fcfg->dirlen, key, 0, m) == -1) {
            FILE_MTX_DISPOSE;
            return (-1);
        }

        FILE_CACHE_ATTEMPT("r")

        if (cache == NULL) {
            FILE_MTX_DISPOSE;
            return (-1);
        }

        cachefd = fileno(cache);
        memset(&s, 0, sizeof(s));
        if (fstat(cachefd, &s) != 0) {
            fclose(cache);
            FILE_MTX_DISPOSE;
            return (-1);
        }

        if ((time_t)(s.st_mtime + cfg->expiration) <= time(NULL)) {
            fclose(cache);
            FILE_MTX_DISPOSE;
            if (unlink(fcfg->dir) == - 1)
                da_cloud_log(cfg->efp, "could not delete '%s' file", fcfg->dir, NULL);
            return (-1);
        }

        if (flock(cachefd, LOCK_SH)  == -1) {
            fclose(cache);
            FILE_MTX_DISPOSE;
            da_cloud_log(cfg->efp, "could not lock the cache", NULL);
            return (-1);
        }

        fseek(cache, 0, SEEK_END);
        valuelen = ftell(cache);
        rewind(cache);

        *value = malloc(sizeof(char) * valuelen + 1);
        fgets(*value, valuelen + 1, cache);
        flock(cachefd, LOCK_UN);
        fclose(cache);
        FILE_MTX_DISPOSE;

        return (0);
    }

    return (-1);
}

int
file_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) {
    if (cfg->cache_obj != NULL) {
        FILE *cache = NULL;
        pthread_mutex_t mtx;
        size_t i = 0;
        mode_t m;
        int cachefd = -1;
        struct file_cache_cfg *fcfg = cfg->cache_obj;
        if (pthread_mutex_init(&mtx, NULL) != 0) {
            da_cloud_log(cfg->efp, "could not lock", NULL);
            return (-1);
        }

        pthread_mutex_lock(&mtx);
        file_cache_setumask(&m);
        if (file_cache_mkdir(fcfg->dir, fcfg->dirlen, key, 1, m) == -1) {
            FILE_MTX_DISPOSE;
            return (0);
        }

        FILE_CACHE_ATTEMPT("w")

        if (cache == NULL) {
            FILE_MTX_DISPOSE;
            return (-1);
        }

        cachefd = fileno(cache);
        if (flock(cachefd, LOCK_EX) == -1) {
            FILE_MTX_DISPOSE;
            da_cloud_log(cfg->efp, "could not lock the cache", NULL);
            fclose(cache);
            return (-1);
        }

        fwrite(value, 1, strlen(value), cache);
        flock(cachefd, LOCK_UN);
        fclose(cache);
        FILE_MTX_DISPOSE;

        return (0);
    }

    return (-1);
}

void
file_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL)
        free(cfg->cache_obj);
}

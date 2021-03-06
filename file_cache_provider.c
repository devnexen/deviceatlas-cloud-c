#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "file_cache_provider.h"

#define FILE_CACHE_ATTEMPT(mode)                        \
    while ((cache = fopen(dir, mode)) == NULL) {        \
        ++ i;                                           \
        if (i == 10)                                    \
        break;                                          \
    }                                                   \

struct file_cache_cfg {
    char dir[PATH_MAX];
};

static void
file_cache_setumask(mode_t *m) {
    *m = umask(0);
    umask(*m);
}

static int
file_cache_mkdir(struct file_cache_cfg *fcfg, char *dir,
	const char *key, int creat, mode_t m) {
    strcpy(dir, fcfg->dir);
    strcat(dir, "/");
    strncat(dir, key, 1);
    if (creat) {
        int errnos = errno;
        if (mkdir(dir, 0777 & ~m) != 0) {
            if (errno != EEXIST) {
                errno = errnos;
                return (-1);
            }
            errno = errnos;
        }
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
    if (!S_ISDIR(s.st_mode)) {
        da_cloud_log(cfg->efp, "directory '%s' invalid", cfg->cache_cfg_str, NULL);
        return (-1);
    } else if (!(s.st_mode & S_IWUSR) && !(s.st_mode & S_IWGRP) && !(s.st_mode & S_IWOTH)) {
        da_cloud_log(cfg->efp, "directory '%s' permission denied", cfg->cache_cfg_str, NULL);
        return (-1);
    }

    cfg->cache_dcm = da_cloud_membuf_create(1024 * 4);
    if (cfg->cache_dcm == NULL) {
        da_cloud_log(cfg->efp, "could not allocate mem pool", NULL);
        return (-1);
    }
    cfg->cache_root = cfg->cache_dcm;
    fcfg = da_cloud_membuf_alloc(&cfg->cache_dcm, sizeof(*fcfg));
    if (fcfg == NULL) {
        da_cloud_log(cfg->efp, "could not allocate data structure", NULL);
        return (-1);
    }

    strcpy(fcfg->dir, cfg->cache_cfg_str);
    cfg->cache_obj = fcfg;
    MTX_INIT

    return (0);
}

int
file_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) {
    if (cfg->cache_obj != NULL && value != NULL) {
        FILE *cache = NULL;
        pthread_mutexattr_t attr;
        pthread_mutex_t mtx;
        mode_t m;
        struct stat s;
        size_t valuelen, i = 0;
        int cachefd = -1;
        char dir[PATH_MAX] = { 0 };
        struct file_cache_cfg *fcfg = cfg->cache_obj;
        MTX_LOCK

        file_cache_setumask(&m);
        if (file_cache_mkdir(fcfg, dir, key, 0, m) == -1) {
            MTX_UNLOCK
            return (-1);
        }

#ifndef FILE_CACHE_MMAP
        FILE_CACHE_ATTEMPT("r")

        if (cache == NULL) {
            MTX_UNLOCK
            return (-1);
        }

        cachefd = fileno(cache);
        memset(&s, 0, sizeof(s));
        if (fstat(cachefd, &s) != 0) {
            fclose(cache);
            MTX_UNLOCK
            return (-1);
        }

        if ((time_t)(s.st_mtime + cfg->expiration) <= time(NULL)) {
            fclose(cache);
            MTX_UNLOCK
            if (unlink(dir) == - 1)
                da_cloud_log(cfg->efp, "could not delete '%s' file", dir, NULL);
            return (-1);
        }

        if (flock(cachefd, LOCK_SH)  == -1) {
            fclose(cache);
            MTX_UNLOCK
            da_cloud_log(cfg->efp, "could not lock the cache", NULL);
            return (-1);
        }

        fseek(cache, 0, SEEK_END);
        valuelen = ftell(cache);
        rewind(cache);

        *value = g_allocator.alloc(g_allocator.child_ctx,
			sizeof(char) * valuelen + 1);
        if (*value == NULL) {
            fclose(cache);
            MTX_UNLOCK
            da_cloud_log(cfg->efp, "could not allocate memory for value", NULL);
            return (-1);
        }
        fgets(*value, valuelen + 1, cache);
        flock(cachefd, LOCK_UN);
        fclose(cache);
#else
        cachefd = open(dir, O_RDONLY);
        if (cachefd == -1) {
            MTX_UNLOCK
            return (-1);
        }

        if (flock(cachefd, LOCK_SH)  == -1) {
            close(cachefd);
            MTX_UNLOCK
            da_cloud_log(cfg->efp, "could not lock the cache", NULL);
            return (-1);
        }

        valuelen = (size_t)lseek(cachefd, 0, SEEK_END);
        lseek(cachefd, 0, SEEK_SET);

        char *region = mmap(NULL, valuelen + 1, PROT_READ, MAP_SHARED, cachefd, 0);
        if (region == ((void *)-1)) {
            close(cachefd);
            MTX_UNLOCK
            da_cloud_log(cfg->efp, "could not read the cache data '%s'", strerror(errno));
            return (-1);
        }

        *value = g_allocator.strdup(g_allocator.child_ctx, region);
        if (*value == NULL) {
            close(cachefd);
            MTX_UNLOCK
            da_cloud_log(cfg->efp, "could not allocate memory for value", NULL);
            return (-1);
        }

        munmap(region, valuelen);
        close(cachefd);
#endif
        MTX_UNLOCK

        return (0);
    }

    return (-1);
}

int
file_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) {
    if (cfg->cache_obj != NULL) {
        FILE *cache = NULL;
        pthread_mutexattr_t attr;
        pthread_mutex_t mtx;
        size_t i = 0, valuelen;
        mode_t m;
        int cachefd = -1;
        char dir[PATH_MAX] = { 0 };
        struct file_cache_cfg *fcfg = cfg->cache_obj;
        MTX_LOCK

        file_cache_setumask(&m);
        if (file_cache_mkdir(fcfg, dir, key, 1, m) == -1) {
            MTX_UNLOCK
            return (0);
        }

#ifndef FILE_CACHE_MMAP
        FILE_CACHE_ATTEMPT("w")

        if (cache == NULL) {
            MTX_UNLOCK
            return (-1);
        }

        cachefd = fileno(cache);
        if (flock(cachefd, LOCK_EX) == -1) {
            fclose(cache);
            MTX_UNLOCK
            da_cloud_log(cfg->efp, "could not lock the cache", NULL);
            return (-1);
        }

        valuelen = strlen(value);
        fwrite(value, 1, valuelen, cache);
        flock(cachefd, LOCK_UN);
        fclose(cache);
#else
        cachefd = open(dir, O_RDWR | O_CREAT, (mode_t)0600);
        if (cachefd == -1) {
            MTX_UNLOCK
            da_cloud_log(cfg->efp, "could not create the cache file", NULL);
            return (-1);
        }

        if (flock(cachefd, LOCK_EX) == -1) {
            close(cachefd);
            MTX_UNLOCK
            da_cloud_log(cfg->efp, "could not lock the cache", NULL);
            return (-1);
        }

        valuelen = strlen(value);
        ftruncate(cachefd, valuelen);
        lseek(cachefd, 0, SEEK_SET);
        char *region = mmap(NULL, valuelen, PROT_WRITE, MAP_SHARED, cachefd, 0);
        if (region == ((void *)-1)) {
            close(cachefd);
            MTX_UNLOCK
            da_cloud_log(cfg->efp, "could not create the cache data '%s'", strerror(errno));
            return (-1);
        }

        memcpy(region, value, valuelen);
        munmap(region, valuelen);
        close(cachefd);
#endif
        MTX_UNLOCK

        return (0);
    }

    return (-1);
}

void
file_cache_fini(struct da_cloud_cache_cfg *cfg) {
    da_cloud_membuf_free(cfg->cache_root);
    MTX_DISPOSE
}

#include <sys/stat.h>
#include <sys/file.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "file_cache_provider.h"

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
file_cache_mkdir(char *dir, size_t dirlen, const char *key, mode_t m) {
    struct stat s;
    memset(&s, 0, sizeof(s));
    dir[dirlen] = 0;
    strcat(dir, "/");
    strncat(dir, key, 1);
    if (stat(dir, &s) != 0) {
       if (mkdir(dir, 0777 & ~m) != 0)
           return (-1);
    }
    strcat(dir, "/");
    strcat(dir, key + 1);    

    return (0);
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

    strcpy(fcfg->dir, cfg->cache_cfg_str);
    fcfg->dirlen = strlen(fcfg->dir);
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
         if (file_cache_mkdir(fcfg->dir, fcfg->dirlen, key, m) == -1) {
             pthread_mutex_unlock(&mtx);
             pthread_mutex_destroy(&mtx);
             da_cloud_log(cfg->efp, "could not create dir '%s'", fcfg->dir);
             return (-1);
         }
         pthread_mutex_unlock(&mtx);
         pthread_mutex_lock(&mtx);
         while ((cache = fopen(fcfg->dir, "r")) == NULL) {
             sleep(1);
             ++ i;
             if (i == 3)
                 break;
         }

         if (cache == NULL) {
             pthread_mutex_unlock(&mtx);
             pthread_mutex_destroy(&mtx);
             return (-1);
         }

         memset(&s, 0, sizeof(s));
         cachefd = fileno(cache);
         if (stat(fcfg->dir, &s) != 0) {
             fclose(cache);
             pthread_mutex_unlock(&mtx);
             pthread_mutex_destroy(&mtx);
             return (-1);
         }

         if ((time_t)(s.st_mtime + cfg->expiration) <= time(NULL)) {
             fclose(cache);
             pthread_mutex_unlock(&mtx);
             pthread_mutex_destroy(&mtx);
             if (unlink(fcfg->dir) == - 1)
                 da_cloud_log(cfg->efp, "could not delete '%s' file", fcfg->dir, NULL);
             return (-1);
         }

         if (flock(cachefd, LOCK_SH)  == -1) {
             fclose(cache);
             pthread_mutex_unlock(&mtx);
             pthread_mutex_destroy(&mtx);
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
         pthread_mutex_unlock(&mtx);
         pthread_mutex_destroy(&mtx);

         return (0);
    }

    return (-1);
}

int
file_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) {
    if (cfg->cache_obj != NULL) {
         FILE *cache = NULL;
         struct stat s;
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
         if (file_cache_mkdir(fcfg->dir, fcfg->dirlen, key, m) == -1) {
             pthread_mutex_unlock(&mtx);
             pthread_mutex_destroy(&mtx);
             da_cloud_log(cfg->efp, "could not create dir '%s'", fcfg->dir);
             return (-1);
         }
         pthread_mutex_unlock(&mtx);
         pthread_mutex_lock(&mtx);
         memset(&s, 0, sizeof(s));
         if (stat(fcfg->dir, &s) == 0) {
             pthread_mutex_unlock(&mtx);
             pthread_mutex_destroy(&mtx);
             return (0);
         }

         while ((cache = fopen(fcfg->dir, "w")) == NULL) {
             sleep(1);
             ++ i;
             if (i == 3)
                 break;
         }

         if (cache == NULL) {
             pthread_mutex_unlock(&mtx);
             pthread_mutex_destroy(&mtx);
             da_cloud_log(cfg->efp, "could not open cache for writing", NULL);
             return (-1);
         }

         cachefd = fileno(cache);
         if (flock(cachefd, LOCK_EX) == -1) {
             pthread_mutex_unlock(&mtx);
             pthread_mutex_destroy(&mtx);
             da_cloud_log(cfg->efp, "could not lock the cache", NULL);
             fclose(cache);
             return (-1);
         }

         fwrite(value, 1, strlen(value), cache);
         flock(cachefd, LOCK_UN);
         fclose(cache);
         pthread_mutex_unlock(&mtx);
         pthread_mutex_destroy(&mtx);

         return (0);
    }

    return (-1);
}

void
file_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL)
        free(cfg->cache_obj);
}

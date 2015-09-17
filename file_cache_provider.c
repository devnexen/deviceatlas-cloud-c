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

static
void file_cache_mkdir(char *dir, size_t dirlen, const char *key, mode_t m) {
    struct stat s;
    memset(&s, 0, sizeof(s));
    dir[dirlen] = 0;
    strcat(dir, "/");
    strncat(dir, key, 1);
    if (stat(dir, &s) != 0)
        mkdir(dir, 0777 & ~m);
    strcat(dir, "/");
    strcat(dir, key + 1);    
}

int
file_cache_init(struct da_cloud_cache_cfg *cfg) {
    struct stat s;
    size_t cache_cfg_strlen = strlen(cfg->cache_cfg_str);
    cfg->cache_obj = NULL;
    if (cache_cfg_strlen == 0) {
        fprintf(stderr, "directory cannot be empty\n");
        return (-1);
    }
    if (cache_cfg_strlen >= (PATH_MAX - 67)) {
        fprintf(stderr, "directory '%s' too long\n", cfg->cache_cfg_str);
        return (-1);
    }
    memset(&s, 0, sizeof(s));
    stat(cfg->cache_cfg_str, &s);
    if ((s.st_mode & S_IFMT) != S_IFDIR || !(s.st_mode & S_IWOTH)) {
        fprintf(stderr, "directory '%s' invalid\n", cfg->cache_cfg_str);
        return (-1);
    }
    struct file_cache_cfg *fcfg = malloc(sizeof(*fcfg));
    if (fcfg == NULL) {
        fprintf(stderr, "could not allocated data structure\n");
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
         size_t valuelen;
         int cachefd = -1;
         struct file_cache_cfg *fcfg = cfg->cache_obj;
         if (pthread_mutex_init(&mtx, NULL) != 0) {
             fprintf(stderr, "could not lock\n");
             return (-1);
         }
         
         pthread_mutex_lock(&mtx);
         file_cache_setumask(&m);
         file_cache_mkdir(fcfg->dir, fcfg->dirlen, key, m);
         cache = fopen(fcfg->dir, "r");
         if (cache == NULL) {
             pthread_mutex_unlock(&mtx);
             return (-1);
         }
         memset(&s, 0, sizeof(s));
         cachefd = fileno(cache);
         if (stat(fcfg->dir, &s) != 0) {
             fclose(cache);
             pthread_mutex_unlock(&mtx);
             fprintf(stderr, "cannot stat '%s' file\n", fcfg->dir);
             return (-1);
         }
         if ((time_t)(s.st_mtime + cfg->expiration) <= time(NULL)) {
             fclose(cache);
             pthread_mutex_unlock(&mtx);
             if (unlink(fcfg->dir) == - 1)
                 fprintf(stderr, "could not delete '%s' file\n", fcfg->dir);
             return (-1);
         }
         if (flock(cachefd, LOCK_SH)  == -1) {
             fclose(cache);
             pthread_mutex_unlock(&mtx);
             fprintf(stderr, "could not lock the cache\n");
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
         mode_t m;
         int cachefd = -1;
         struct file_cache_cfg *fcfg = cfg->cache_obj;
         if (pthread_mutex_init(&mtx, NULL) != 0) {
             fprintf(stderr, "could not lock\n");
             return (-1);
         }
         
         pthread_mutex_lock(&mtx);
         file_cache_setumask(&m);
         file_cache_mkdir(fcfg->dir, fcfg->dirlen, key, m);
         memset(&s, 0, sizeof(s));
         if (stat(fcfg->dir, &s) == 0)
             return (0);
         cache = fopen(fcfg->dir, "w");
         if (cache == NULL) {
             pthread_mutex_unlock(&mtx);
             fprintf(stderr, "could not open cache for writing\n");
             return (-1);
         }
         cachefd = fileno(cache);
         if (flock(cachefd, LOCK_EX) == -1) {
             pthread_mutex_unlock(&mtx);
             fprintf(stderr, "could not lock the cache\n");
             fclose(cache);
             return (-1);
         }
         fwrite(value, 1, strlen(value), cache);
         flock(cachefd, LOCK_UN);
         fclose(cache);
         pthread_mutex_unlock(&mtx);
         return (0);
    }

    return (-1);
}

void
file_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL)
        free(cfg->cache_obj);
}

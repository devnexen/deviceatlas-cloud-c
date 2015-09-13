#include <sys/stat.h>
#include <sys/file.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "file_cache_provider.h"

struct file_cache_cfg {
    size_t dirlen;
    char dir[PATH_MAX];
};

static
void file_cache_mkdir(char *dir, size_t dirlen, const char *key) {
    struct stat s;
    memset(&s, 0, sizeof(s));
    memset(dir + dirlen, 0, sizeof(dir) - dirlen);
    strcat(dir, "/");
    strncat(dir, key, 1);
    if (stat(dir, &s) != 0)
        mkdir(dir, 0777);
    strcat(dir, "/");
    strcat(dir, key + 1);    
}

int
file_cache_init(struct da_cloud_cache_cfg *cfg) {
    struct stat s;
    cfg->cache_obj = NULL;
    if (strlen(cfg->cache_cfg_str) >= (PATH_MAX - 74)) {
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
         struct stat s;
         size_t valuelen;
         int cachefd = -1;
         struct file_cache_cfg *fcfg = cfg->cache_obj;
         file_cache_mkdir(fcfg->dir, fcfg->dirlen, key);
         cache = fopen(fcfg->dir, "r");
         if (cache == NULL)
             return (-1);
         memset(&s, 0, sizeof(s));
         stat(fcfg->dir, &s);
         if ((time_t)(s.st_mtime + cfg->expiration) <= time(NULL)) {
             fclose(cache);
             if (unlink(fcfg->dir) == - 1)
                 fprintf(stderr, "could not delete '%s' file\n", fcfg->dir);
             return (-1);
         }
         cachefd = fileno(cache);
         if (flock(cachefd, LOCK_SH)  == -1) {
             fprintf(stderr, "could not lock the cache\n");
             fclose(cache);
             return (-1);
         }
         fseek(cache, 0, SEEK_END);
         valuelen = ftell(cache);
         rewind(cache);

         *value = malloc(sizeof(char) * valuelen + 1);
         fgets(*value, valuelen + 1, cache);
         flock(cachefd, LOCK_UN);
         fclose(cache);
         return (0);
    }

    return (-1);
}

int
file_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) {
    if (cfg->cache_obj != NULL) {
         FILE *cache = NULL;
         int cachefd = -1;
         struct file_cache_cfg *fcfg = cfg->cache_obj;
         file_cache_mkdir(fcfg->dir, fcfg->dirlen, key);
         cache = fopen(fcfg->dir, "w");
         if (cache == NULL)
             return (-1);
         cachefd = fileno(cache);
         if (flock(cachefd, LOCK_EX) == -1) {
             fprintf(stderr, "could not lock the cache\n");
             fclose(cache);
             return (-1);
         }
         fwrite(value, 1, strlen(value), cache);
         flock(cachefd, LOCK_UN);
         fclose(cache);
         return (0);
    }
    return (-1);
}

void
file_cache_fini(struct da_cloud_cache_cfg *cfg) {
    if (cfg->cache_obj != NULL)
        free(cfg->cache_obj);
}

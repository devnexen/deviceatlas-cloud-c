/**
 *
 *  Copyright (C) 2015  David Carlier <devnexen@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3.0 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.
 */

#ifndef  DACLOUD_CACHE_H
#define  DACLOUD_CACHE_H

#include <time.h>
#include <stdio.h>

#define XXH_IMPLEMENTATION
#define XXH_INLINE_ALL
#include <import/xxhash.h>

struct da_cloud_config;
struct da_cloud_mem;

struct da_cloud_cache_cfg {
    void *data;
    void *cache_obj;
    struct da_cloud_membuf *cache_root;
    struct da_cloud_membuf *cache_dcm;
    FILE *efp;
    char *cache_cfg_str;
    size_t expiration;
    pthread_mutexattr_t attr;
    pthread_mutex_t mtx;
};

struct da_cloud_cache_ops {
    const char * (*id)(void);
    int (*init)(struct da_cloud_cache_cfg *);
    int (*get)(struct da_cloud_cache_cfg *, const char *, char **);
    int (*set)(struct da_cloud_cache_cfg *, const char *, const char *);
    void (*fini)(struct da_cloud_cache_cfg *);
};

void
da_cloud_crypt_key(char *, size_t, char *, size_t) __attribute__((deprecated));

const char *mock_cache_id(void);
int mock_cache_init(struct da_cloud_cache_cfg *);
int mock_cache_get(struct da_cloud_cache_cfg *, const char *, char **);
int mock_cache_set(struct da_cloud_cache_cfg *, const char *, const char *);
void mock_cache_fini(struct da_cloud_cache_cfg *);

void cache_set(struct da_cloud_cache_ops *, const char *);

void da_cloud_log(FILE *, const char *, ...);

#define MTX_INIT                                                \
   if (pthread_mutexattr_init(&cfg->attr) != 0) {                    \
       da_cloud_log(cfg->efp, "could not lock", NULL);          \
       return (-1);                                             \
   }                                                            \
                                                                \
   pthread_mutexattr_settype(&cfg->attr, PTHREAD_MUTEX_NORMAL); \
                                                                \
   if (pthread_mutex_init(&cfg->mtx, &cfg->attr) != 0) {	    \
       da_cloud_log(cfg->efp, "could not lock", NULL);          \
       return (-1);                                             \
   }

#define MTX_LOCK											    \
	pthread_mutex_lock(&cfg->mtx);

#define MTX_UNLOCK                                              \
    pthread_mutex_unlock(&mtx);

#define MTX_DISPOSE                                             \
    pthread_mutexattr_destroy(&cfg->attr);                      \
    pthread_mutex_destroy(&cfg->mtx);

#include "cache_providers.h"

#endif

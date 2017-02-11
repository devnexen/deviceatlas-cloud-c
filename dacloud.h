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

#ifndef  DACLOUD_H
#define  DACLOUD_H

#include <sys/queue.h>
#include <curl/curl.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "dacloud_cache.h"
#include "dacloud_mem.h"


#define DACLOUD_CACHEKEY_SIZE       65
/**
 * HTTP headers list struct
 * fed by the consumer and used
 * internally by CURL
 */
struct da_cloud_header {
    char *orig_key;
    char *key;
    char *value;
    SLIST_ENTRY(da_cloud_header) entries;
};

struct da_cloud_header_head {
    /* keeping track of the cache key */
    char *cachekey;
    struct da_cloud_membuf *root;
    struct da_cloud_membuf *dcm;
    SLIST_HEAD(da_cloud_header_list, da_cloud_header) list;
};

#define da_list_init(h)        SLIST_INIT(h)
#define da_list_foreach(p, h)  SLIST_FOREACH(p, h, entries)

int da_cloud_header_init(struct da_cloud_header_head *);
int da_cloud_header_add(struct da_cloud_header_head *, const char *, const char *);
int da_cloud_clientside_add(struct da_cloud_header_head *, const char *);
int da_cloud_useragent_add(struct da_cloud_header_head *, const char *);
int da_cloud_language_add(struct da_cloud_header_head *, const char *);
void da_cloud_header_free(struct da_cloud_header_head *);

enum da_cloud_property_type {
    DA_CLOUD_LONG,
    DA_CLOUD_BOOL,
    DA_CLOUD_STRING,
    DA_CLOUD_UNKNOWN = 16
};

/**
 * DeviceAtlas property list struct
 * fed internally via the Cloud Service
 * response data
 */
struct da_cloud_property {
    char *name;
    union {
        long l;
        char *s;
    } value;
    enum da_cloud_property_type type;
    SLIST_ENTRY(da_cloud_property) entries;
} __attribute__((packed));

struct da_cloud_property_head {
    /* can be 'cloud', 'cache' or 'none' */
    char cachesource[8];
    struct da_cloud_membuf *root;
    struct da_cloud_membuf *dcm;
    SLIST_HEAD(da_cloud_property_list, da_cloud_property) list;
};

/**
 * Server list struct
 * For internal server endpoint rankings
 * Once the library is initialized via da_cloud_init()
 * the list of servers will be ordered per response speed
 */
struct da_cloud_server {
    double response_time;
    char *host;
    unsigned short port;
};

struct da_cloud_server_head {
    struct da_cloud_server **servers;
    size_t nb;
    unsigned int dservers: 1;
};

struct da_cloud_config {
    struct da_cloud_cache_cfg cache_cfg;
    struct da_cloud_server_head *shead;
    FILE *efp;
    char *licence_key;
    unsigned short manual_ranking: 1;
    struct da_cloud_cache_ops cops;
};

/* Handy function to print a header in the related stream */
void da_cloud_print_header(FILE *, struct da_cloud_header *);
/* Handy function to print a server in the related stream */
void da_cloud_print_server(FILE *, struct da_cloud_server *);
/* Handy function to print a property in the related stream */
void da_cloud_print_property(FILE *, struct da_cloud_property *);

/**
 *  da_cloud_init() must be called before entering in MT's context
 *  da_cloud_fini() must be called when the library usage is ended
 */
int da_cloud_init(struct da_cloud_config *, const char *);
int da_cloud_detect(struct da_cloud_config *, struct da_cloud_header_head *, struct da_cloud_property_head *);
int da_cloud_property(struct da_cloud_property_head *, const char *, struct da_cloud_property **);
int da_cloud_property_count(struct da_cloud_property_head *, size_t *);
void da_cloud_properties_free(struct da_cloud_property_head *);
void da_cloud_fini(struct da_cloud_config *);
/* Handy function to get the cache identifier */
const char *da_cloud_cache_id(struct da_cloud_config *);

#ifdef __cplusplus
}
#endif
#endif

#ifndef  DACLOUD_H
#define  DACLOUD_H

#include <sys/queue.h>
#include <curl/curl.h>
#include <stdio.h>

#include "dacloud_cache.h"

#ifdef __cplusplus
extern "C" {
#endif

struct da_cloud_header {
    char *key;
    char *value;
    SLIST_ENTRY(da_cloud_header) entries;
};

struct da_cloud_header_head {
    SLIST_HEAD(da_cloud_header_list, da_cloud_header) list;
};

int da_cloud_header_init(struct da_cloud_header_head *);
int da_cloud_header_add(struct da_cloud_header_head *, const char *, const char *);
void da_cloud_header_free(struct da_cloud_header_head *);

enum da_cloud_property_type {
    DA_CLOUD_LONG,
    DA_CLOUD_BOOL,
    DA_CLOUD_STRING
};

struct da_cloud_property {
    char *name;
    union {
        long l;
        char *s;
    } value;
    enum da_cloud_property_type type;
    SLIST_ENTRY(da_cloud_property) entries;
};

struct da_cloud_property_head {
    char cachesource[16];
    SLIST_HEAD(da_cloud_property_list, da_cloud_property) list;
};

struct da_cloud_server {
    double response_time;
    char *host;
    unsigned short port;
};

struct da_cloud_server_head {
    struct da_cloud_server **servers;
    size_t nb;
};

struct da_cloud_config {
    struct da_cloud_cache_cfg cache_cfg;
    struct da_cloud_cache_ops cops;
    struct da_cloud_server_head *shead;
    char *licence_key;
};

void da_cloud_print_server(FILE *, struct da_cloud_server *);

int da_cloud_init(struct da_cloud_config *, const char *);
int da_cloud_detect(struct da_cloud_config *, struct da_cloud_header_head *, struct da_cloud_property_head *);
void da_cloud_properties_free(struct da_cloud_property_head *);
void da_cloud_fini(struct da_cloud_config *);

#endif
#endif

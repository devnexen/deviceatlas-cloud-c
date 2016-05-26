#ifndef DACLOUD_MEM_H
#define DACLOUD_MEM_H
#include <string.h>

struct da_cloud_mem {
    size_t a;
    unsigned char *n;
    unsigned char *e;
};

struct da_cloud_mem *
da_cloud_mem_create(size_t);

void *
da_cloud_mem_alloc(struct da_cloud_mem *, size_t);

char *
da_cloud_mem_strdup(struct da_cloud_mem *, const char *);

void
da_cloud_mem_free(struct da_cloud_mem *);

#endif

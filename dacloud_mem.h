#ifndef DACLOUD_MEM_H
#define DACLOUD_MEM_H
#include <string.h>
#include <stdio.h>

struct da_cloud_allocator {
    void *(*alloc)(void *, size_t);
    void *(*realloc)(void *, void *, size_t);
    void (*free)(void *, void *);
    char *(*strdup)(void *, const char *);
    void *main_ctx;
    void *child_ctx;
};

void *
default_alloc(void *, size_t);
void *
default_realloc(void *, void *, size_t);
void
default_free(void *, void *);
char *
default_strdup(void *, const char *);

static struct da_cloud_allocator g_allocator = {
	default_alloc,
	default_realloc,
	default_free,
	default_strdup,
	NULL,
	NULL
};

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

/**
 * should be called before the main instance initialization
 * as the allocators are used internally for every part
 */
void
da_cloud_setallocator(struct da_cloud_allocator *);

struct da_cloud_membuf {
    struct da_cloud_mem *p;
    struct da_cloud_membuf *n;
};

struct da_cloud_membuf *
da_cloud_membuf_create(size_t);

void *
da_cloud_membuf_alloc(struct da_cloud_membuf **, size_t);

char *
da_cloud_membuf_strdup(struct da_cloud_membuf **, const char *);

void
da_cloud_membuf_free(struct da_cloud_membuf *);

#endif

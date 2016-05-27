#include "dacloud_mem.h"

#include <stdlib.h>
#include <limits.h>

struct da_cloud_mem *
da_cloud_mem_create(size_t needed)
{
    struct da_cloud_mem *dcm = malloc(sizeof(*dcm) + needed);
    if (dcm == NULL)
        return (NULL);

    dcm->n = (unsigned char *)&dcm[1];
    dcm->e = dcm->n + needed;
    dcm->a = needed;

    return (dcm);
}

void *
da_cloud_mem_alloc(struct da_cloud_mem *dcm, size_t needed)
{
    void *region = NULL;
    if (dcm != NULL) {
        if (needed < (dcm->e - dcm->n)) {
            size_t n = (needed + 3) & 0xfffffffc;
            region = dcm->n;
            dcm->n += n;
        }
    }

    return (region);
}

char *
da_cloud_mem_strdup(struct da_cloud_mem *dcm, const char *src)
{
    char *p = da_cloud_mem_alloc(dcm, strlen(src) + 1);
    if (p == NULL)
        return (NULL);

    strcpy(p, src);
    return (p);
}

void
da_cloud_mem_free(struct da_cloud_mem *dcm)
{
    if (dcm)
        free(dcm);
    dcm = NULL;
}

struct da_cloud_membuf *
da_cloud_membuf_create(size_t needed)
{
    struct da_cloud_membuf *dcm = malloc(sizeof(*dcm));
    if (dcm == NULL)
        return (NULL);

    dcm->p = da_cloud_mem_create(needed);
    if (dcm->p == NULL) {
        free(dcm);
        dcm = NULL;
    }

    dcm->n = NULL;

    return (dcm);
}

void *
da_cloud_membuf_alloc(struct da_cloud_membuf **dcm, size_t needed)
{
    if (dcm == NULL || *dcm == NULL)
        return (NULL);

    void *region = da_cloud_mem_alloc((*dcm)->p, needed);
    if (region == NULL) {
        size_t newsize = (*dcm)->p->a + ((*dcm)->p->a / 2 + needed);
        if (newsize >= LONG_MAX)
            return (NULL);
        struct da_cloud_membuf *n = da_cloud_membuf_create(newsize);
        if (n == NULL)
            return (NULL);

        (*dcm)->n = n;
        *dcm = (*dcm)->n;

        return (da_cloud_membuf_alloc(dcm, needed));
    }

    return (region);
}

char *
da_cloud_membuf_strdup(struct da_cloud_membuf **dcm, const char *src)
{
    if (dcm == NULL)
        return (NULL);

    char *p = da_cloud_membuf_alloc(dcm, strlen(src) + 1);
    if (p == NULL)
        return (NULL);

    strcpy(p, src);

    return (p);
}

void
da_cloud_membuf_free(struct da_cloud_membuf *dcm)
{
    struct da_cloud_membuf *h = dcm;
    while (h) {
        struct da_cloud_membuf *n = h->n;
        da_cloud_mem_free(h->p);
        free(h);
        h = n;
    }
}

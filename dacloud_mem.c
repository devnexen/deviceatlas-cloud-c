#include "dacloud_mem.h"

#include <stdlib.h>

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
}

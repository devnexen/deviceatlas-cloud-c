#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <talloc.h>

#include "dacloud.h"
#include "dacloud_util.h"

static const char *pgname;

static void *
__alloc(void *ctx, size_t size)
{
    return (talloc_size(ctx, size));
}

static void *
__realloc(void *ctx, void *orig, size_t size)
{
    void *ptr;

    if ((ptr = talloc_realloc_size(ctx, orig, size)) == NULL) {
        talloc_free(orig);
	orig = NULL;
	return (NULL);
    }

    return (ptr);
}

static void
__free(void *ctx, void *ptr)
{
    (void)ctx;
    talloc_free(ptr);
}

static char *
__strdup(void *ctx, const char *src)
{
    return (talloc_strdup(ctx, src));
}

static void
__debug_report(void)
{
    talloc_report_full(g_allocator.main_ctx, stderr);
}

static void
usage(void)
{
    printf("%s <config path> <user-agent>\n", pgname);
    exit(-1);
}

static void
print_headers(struct da_cloud_header_head hhead) {
    struct da_cloud_header *h;
    printf("\nHeaders used:\n");
    da_list_foreach(h, &hhead.list) {
        da_cloud_print_header(stderr, h);
    }
    printf("\n");
}

static void
print_properties(struct da_cloud_property_head phead) {
    struct da_cloud_property *p;
    size_t i;
	da_cloud_property_count(&phead, &i);
    da_list_foreach(p, &phead.list) {
        da_cloud_print_property(stderr, p);
    }
    printf("source : %s\n", phead.cachesource);
    printf("properties : %d\n", (int)i);
}

int
main(int argc, char *argv[]) {
    struct da_cloud_config config;
    struct da_cloud_allocator tac;
    TALLOC_CTX *p_ctx, *c_ctx;
    const char *configpath, *useragent;
    pgname = argv[0];
    if (argc < 3)
        usage();
    configpath = argv[1];
    useragent = argv[2];

    talloc_enable_leak_report_full();
    p_ctx = talloc_new(NULL);
    c_ctx = talloc_new(p_ctx);

    tac.alloc = __alloc;
    tac.realloc = __realloc;
    tac.free = __free;
    tac.strdup = __strdup;
    tac.main_ctx = p_ctx;
    tac.child_ctx = c_ctx;

    atexit(__debug_report);

    da_cloud_setallocator(&tac);
    memset(&config, 0, sizeof(config));
    if (da_cloud_init(&config, configpath) == 0) {
        struct da_cloud_header_head hhead;
        struct da_cloud_property_head phead;
        da_cloud_servers_ranking(stderr, &config);
	memset(&hhead, 0, sizeof(hhead));
	da_cloud_header_init(&hhead);
	da_cloud_useragent_add(&hhead, useragent);
	da_cloud_detect(&config, &hhead, &phead);
	print_headers(hhead);
	print_properties(phead);
	da_cloud_properties_free(&phead);
	da_cloud_header_free(&hhead);
        da_cloud_fini(&config);
    }

    talloc_free(p_ctx);

    return (0);
}

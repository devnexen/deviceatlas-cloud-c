#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "dacloud.h"

static void
print_headers(struct da_cloud_header_head hhead) {
    struct da_cloud_header *h;
    printf("\nHeaders used:\n");
    SLIST_FOREACH(h, &hhead.list, entries) {
        printf("%s => %s\n", h->key, h->value);
    }
    printf("\n");
}

static void
print_properties(struct da_cloud_property_head phead) {
    struct da_cloud_property *p;
    size_t i = 0;
    SLIST_FOREACH(p, &phead.list, entries) {
        da_cloud_print_property(stderr, p);
        i ++;
    }
    printf("source : %s\n", strlen(phead.cachesource) > 0 ? phead.cachesource : "none");
    printf("properties : %d\n", (int)i);
}

int
main(int argc, char *argv[]) {
    if (argc < 2)
        return (-1);
    const char *configpath = argv[1];
    struct da_cloud_config config;
    memset(&config, 0, sizeof(config));
    if (da_cloud_init(&config, configpath) == 0) {
        struct da_cloud_header_head hhead;
        struct da_cloud_property_head phead;
        char buf[1024];
        size_t i = 0;
        for (i = 0; i < config.shead->nb; i ++)
            da_cloud_print_server(stderr, config.shead->servers[i]);
        while ((fgets(buf, sizeof(buf), stdin))) {
            char *p = strdup(buf);
            char *b = strstr(p, "\r\n");
            if (b != NULL)
                *(p + (b - p)) = 0;
            memset(&hhead, 0, sizeof(hhead));
            da_cloud_header_init(&hhead);
            da_cloud_header_add(&hhead, "user-agent", p);
            da_cloud_detect(&config, &hhead, &phead);
            print_headers(hhead);
            print_properties(phead);
            da_cloud_properties_free(&phead);
            da_cloud_header_free(&hhead);
            free(p);
        }
    }
    da_cloud_fini(&config);
    return (0);
}

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "dacloud.h"

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
    const char *configpath;
    if (argc < 2)
        return (-1);
    configpath = argv[1];
    memset(&config, 0, sizeof(config));
    if (da_cloud_init(&config, configpath) == 0) {
        struct da_cloud_header_head hhead;
        struct da_cloud_property_head phead;
        char buf[1024];
        size_t i = 0;
        printf("default servers => %s\n", config.shead->dservers ? "yes" : "no");
        printf("manual ranking => %s\n", config.manual_ranking ? "yes" : "no");
        for (i = 0; i < config.shead->nb; i ++)
            da_cloud_print_server(stderr, config.shead->servers[i]);
        while ((fgets(buf, sizeof(buf), stdin))) {
            char *b;
            char *p = buf;
            while ((isspace(*p) || *p == '"') && p++);
            b = strpbrk(p, "\"\r\n");
            if (b != NULL)
                *(p + (b - p)) = 0;
            if (strlen(p) == 0)
                break;
            memset(&hhead, 0, sizeof(hhead));
            da_cloud_header_init(&hhead);
            da_cloud_useragent_add(&hhead, p);
            da_cloud_detect(&config, &hhead, &phead);
            print_headers(hhead);
            print_properties(phead);
            da_cloud_properties_free(&phead);
            da_cloud_header_free(&hhead);
        }
        da_cloud_fini(&config);
    }
    return (0);
}

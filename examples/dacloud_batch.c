#include <string.h>
#include <stdio.h>
#include "dacloud.h"

int
main(int argc, char *argv[]) {
    if (argc < 2)
        return (-1);
    char *configpath = argv[1];
    struct da_cloud_config config;
    memset(&config, 0, sizeof(config));
    size_t i = 0;
    int ret;
    if ((ret = da_cloud_init(&config, configpath)) == 0) {
        struct da_cloud_header_head hhead;
        struct da_cloud_property_head phead;
        struct da_cloud_property *p;
        memset(&hhead, 0, sizeof(hhead));
        da_cloud_header_init(&hhead);
        da_cloud_header_add(&hhead, "user-agent", "iPhone");
        for (i = 0; i < config.shead->nb; i ++)
            da_cloud_print_server(stderr, config.shead->servers[i]);
        da_cloud_detect(&config, &hhead, &phead);
        SLIST_FOREACH(p, &phead.list, entries) {
            printf("%s : ", p->name);
            switch (p->type) {
                case DA_CLOUD_STRING:
                    printf("%s", p->value.s);
                    break;
                case DA_CLOUD_LONG:
                case DA_CLOUD_BOOL:
                    printf("%ld", p->value.l);
                    break;
            }
            printf("\n");
        }
        printf("source : %s\n", phead.cachesource);
        da_cloud_properties_free(&phead);
        da_cloud_header_free(&hhead);
    }
    da_cloud_fini(&config);
    return (0);
}

#include <pthread.h>
#include <string.h>

#include "dacloud.h"

struct da_cloud_req {
    struct da_cloud_header_head head;
    struct da_cloud_config cfg;
};

void *
da_cloud_process_req(void *arg) {
    struct da_cloud_property_head phead;
    struct da_cloud_req *req = arg;
    da_cloud_detect(&req->cfg, &req->head, &phead);
    da_cloud_properties_free(&phead);
    return (NULL);
}

int
main(int argc, char *argv[]) {
    if (argc < 2)
        return (-1);
    pthread_t pt[4];
    const char *configpath = argv[1];
    struct da_cloud_config config;
    memset(&config, 0, sizeof(config));
    if (da_cloud_init(&config, configpath) == 0) {
        struct da_cloud_header_head hhead;
        struct da_cloud_req req;
        memset(&req, 0, sizeof(req));
        size_t i = 0;
        memset(&hhead, 0, sizeof(hhead));
        da_cloud_header_init(&hhead);
        da_cloud_header_add(&hhead, "user-agent", "Dalvik/1.2.0 (Linux; U; Android 2.2.1; GT-S5830L Build/FROYO)");
        req.cfg = config;
        req.head = hhead;
        for (i = 0; i < sizeof(pt); i ++)
            pthread_create(&pt[i], NULL, da_cloud_process_req, (void *) &req);
        for (i = 0; i < sizeof(pt); i ++)
            pthread_join(pt[i], NULL);
        da_cloud_header_free(&hhead);
    }

    da_cloud_fini(&config);
    return (0);
}

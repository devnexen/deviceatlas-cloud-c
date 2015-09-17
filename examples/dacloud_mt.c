#include <pthread.h>
#include <string.h>

#include "dacloud.h"

#define	THREADS		16

struct da_cloud_req {
    struct da_cloud_header_head head;
    struct da_cloud_config cfg;
    int tid;
};

void *
da_cloud_process_req(void *arg) {
    struct da_cloud_property_head phead;
    struct da_cloud_req *req = arg;
    printf("thread %d starts\n", req->tid);
    da_cloud_detect(&req->cfg, &req->head, &phead);
    printf("thread %d ends from %s\n", req->tid, phead.cachesource);
    da_cloud_properties_free(&phead);
    return (NULL);
}

int
main(int argc, char *argv[]) {
    pthread_t pt[THREADS];
    struct da_cloud_req req[THREADS];
    struct da_cloud_config config;
    const char *configpath;
    if (argc < 2)
        return (-1);
    configpath = argv[1];
    memset(&config, 0, sizeof(config));
    if (da_cloud_init(&config, configpath) == 0) {
        struct da_cloud_header_head hhead;
        size_t i = 0;
        memset(&hhead, 0, sizeof(hhead));
        da_cloud_header_init(&hhead);
        da_cloud_header_add(&hhead, "user-agent", "Dalvik/1.2.0 (Linux; U; Android 2.2.1; GT-S5830L Build/FROYO)");
        for (i = 0; i < THREADS; i ++) {
            req[i].head = hhead;
            req[i].cfg = config;
            req[i].tid = i;
            pthread_create(&pt[i], NULL, da_cloud_process_req, (void *) &req[i]);
        }

        for (i = 0; i < THREADS; i ++)
            pthread_join(pt[i], NULL);

        da_cloud_header_free(&hhead);
    }

    da_cloud_fini(&config);
    return (0);
}

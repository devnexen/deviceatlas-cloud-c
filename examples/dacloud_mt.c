#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "dacloud.h"

#define	THREADS		128

struct da_cloud_req {
    struct da_cloud_config cfg;
    int iterations;
    int tid;
};

void *
da_cloud_process_req(void *arg) {
    struct da_cloud_header_head hhead;
    struct da_cloud_property_head phead;
	struct da_cloud_property *p;
    struct da_cloud_req *req = arg;
    int i = 0;
    memset(&hhead, 0, sizeof(hhead));
    da_cloud_header_init(&hhead);
    da_cloud_useragent_add(&hhead, "Dalvik/1.2.0 (Linux; U; Android 2.2.1; GT-S5830L Build/FROYO)");
    for (i = 0; i < req->iterations; i ++) {
        printf("thread %d (iteration %d) starts\n", req->tid, (i + 1));
        da_cloud_detect(&req->cfg, &hhead, &phead);
        if (da_cloud_property(&phead, "id", &p) == 0)
            printf("thread %d (iteration %d): id is %ld\n", req->tid, (i + 1),
                    p->value.l);
        printf("thread %d (iteration %d) ends from %s\n", req->tid, (i + 1), phead.cachesource);
    }
    da_cloud_properties_free(&phead);
    da_cloud_header_free(&hhead);
    return (NULL);
}

int
main(int argc, char *argv[]) {
    pthread_t pt[THREADS];
    struct da_cloud_req req[THREADS];
    struct da_cloud_config config;
    const char *configpath;
    int iterations = 1;
    if (argc < 2)
        return (-1);
    configpath = argv[1];
    if (argc > 2) {
        iterations = strtol(argv[2], 0, 10);
        if (iterations < 1)
            iterations = 1;
    }
    memset(&config, 0, sizeof(config));
    if (da_cloud_init(&config, configpath) == 0) {
        size_t i = 0;
        for (i = 0; i < THREADS; i ++) {
            memset(&req[i], 0, sizeof(req[i]));
            memcpy(&req[i].cfg, &config, sizeof(req[i].cfg));
            req[i].tid = i;
            req[i].iterations = iterations;
            pthread_create(&pt[i], NULL, da_cloud_process_req, (void *) &req[i]);
        }

        for (i = 0; i < THREADS; i ++)
            pthread_join(pt[i], NULL);
    }

    da_cloud_fini(&config);
    return (0);
}

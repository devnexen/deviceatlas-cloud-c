#include "dacloud.h"
#include <string.h>
#include <stdlib.h>

#define	DEFAULT_ITERATIONS	1000

int
main(int argc, char *argv[]) {
    if (argc < 2)
        return (-1);
    int iterations = DEFAULT_ITERATIONS;
    if (argc >= 3) {
        int tmp = strtol(argv[2], 0, 10);
        if (tmp > 0)
            iterations = tmp;
    }
    const char *configpath = argv[1];
    struct da_cloud_config config;
    memset(&config, 0, sizeof(config));
    if (da_cloud_init(&config, configpath) == 0) {
        struct da_cloud_header_head hhead;
        size_t i = 0;
        memset(&hhead, 0, sizeof(hhead));
        da_cloud_header_init(&hhead);
        da_cloud_header_add(&hhead, "user-agent", "iPhone");
        struct timeval start, end;
        double timetotal = 0.0;
        if (gettimeofday(&start, NULL) == 0) {
            for (i = 0; i < iterations; i ++) {
                struct da_cloud_property_head phead;
                da_cloud_detect(&config, &hhead, &phead);
                da_cloud_properties_free(&phead);
            }
            gettimeofday(&end, NULL);
            timetotal = (double)((end.tv_sec + ((double)(end.tv_usec / 1000000))) -
                    ((start.tv_sec + ((double)(start.tv_usec / 1000000)))));
        }
        da_cloud_header_free(&hhead);
        da_cloud_fini(&config);

        printf("Time with %d iterations (in sec): %4.4f\n", iterations, timetotal);
    }

    return (0);
}

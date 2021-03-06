#include "dacloud.h"
#include <string.h>
#include <stdlib.h>

#define	DEFAULT_ITERATIONS	1000

static const char *pgname;

static void
usage(void)
{
    printf("%s <config path> [<nb of iterations>]\n", pgname);
    exit(-1);
}

int
main(int argc, char *argv[]) {
    struct da_cloud_config config;
    const char *configpath, *iterationstr;
    int iterations = DEFAULT_ITERATIONS;
    pgname = argv[0];
    if (argc < 2)
        usage();
    if (argc >= 3) {
        iterationstr = argv[2];
        long tmp = strtol(iterationstr, 0, 10);
        if (tmp > 0 && tmp < INT_MAX)
            iterations = (int)tmp;
    }
    configpath = argv[1];
    memset(&config, 0, sizeof(config));
    if (da_cloud_init(&config, configpath) == 0) {
        struct da_cloud_header_head hhead;
        struct timeval start, end;
        double timetotal = 0.0;
        memset(&hhead, 0, sizeof(hhead));
        da_cloud_header_init(&hhead);
        da_cloud_useragent_add(&hhead, "iPhone");
        if (gettimeofday(&start, NULL) == 0) {
            int i = 0;
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

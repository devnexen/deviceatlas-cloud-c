#include <sys/param.h>
#include "dacloud_util.h"
#include "dacloud.h"

#if defined(__linux__) || defined(__sun) || defined(BSD)
#include <unistd.h>

int
da_cloud_get_num_cores(void)
{
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    return (num_cores);
}
#else
int
da_cloud_get_num_cores(void)
{
    return (1);
}
#endif

void
da_cloud_servers_ranking(FILE *fp, struct da_cloud_config *config)
{
    size_t i;
    for (i = 0; i < config->shead->nb; i ++)
        da_cloud_print_server(fp, config->shead->servers[i]);
}

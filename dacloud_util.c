#include "dacloud_util.h"
#if defined(__linux__) || defined(__sun) || defined(BSD)
#include <unistd.h>

int
dacloud_get_num_cores(void)
{
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    return (num_cores);
}
#else
int
dacloud_get_num_cores(void)
{
    return (1);
}
#endif


#include <stdio.h>

struct da_cloud_config;

int da_cloud_get_num_cores(void);
void da_cloud_servers_ranking(FILE *fp, struct da_cloud_config);

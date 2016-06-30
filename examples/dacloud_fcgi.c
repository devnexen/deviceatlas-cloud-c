#include <stdlib.h>
#include <string.h>
#include <fcgi_stdio.h>

#include "dacloud.h"

int
main(int argc, char *argv[]) {
    struct da_cloud_config cfg;
    char *configpath = getenv("DACLOUD_CONFIG");
    (void)argc;

    if (configpath == NULL || configpath[0] == '\0') {
        fprintf(stderr, "%s: DACLOUD_CONFIG environment variable needs to be defined\n", argv[0]);
        exit(-1);
    }

    if (da_cloud_init(&cfg, configpath) == 0) {
        while (FCGI_Accept() >= 0) {
            char *user_agent = getenv("HTTP_USER_AGENT");
            if (user_agent == NULL || user_agent[0] == '\0')
                continue;
            struct da_cloud_header_head headers;
            struct da_cloud_property_head properties;
            da_cloud_header_init(&headers);
            da_cloud_useragent_add(&headers, user_agent);

            printf("<html>\n");
            printf("<head></head>\n");
            printf("<body>\n");
            printf("<h2>User-Agent: %s</h2>\n", user_agent);
            printf("<h3>Cache type: %s</h3>\n", da_cloud_cache_id(&cfg));
            if (da_cloud_detect(&cfg, &headers, &properties) == 0) {
                struct da_cloud_property *p;
                da_list_foreach(p, &properties.list) {
                    printf("<p>");
                    da_cloud_print_property(stdin, p);
                    printf("</p>\n");
                }

                printf("<p>Cache source:%s</p>\n", properties.cachesource);
                printf("<p>Cache key:%s</p>\n", headers.cachekey);
                da_cloud_properties_free(&properties);
            }

            printf("</body>\n");
            printf("</html>\n");

            da_cloud_header_free(&headers);
        }
        da_cloud_fini(&cfg);
    }
    return (0);
}

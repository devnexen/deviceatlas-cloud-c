#include <iostream>
#include <cstring>
#include <cstdlib>
#include <locale>
#include <stdexcept>
#include <err.h>
#include "dacloud.h"

using namespace std;

static da_cloud_config config;
static int ret;

__attribute__((constructor))
void __init(void)
{
    ret = -1;
    const char *configpath = getenv("CONFIGPATH");
    if (configpath == NULL)
        errx(1, "no config path");
    ret = da_cloud_init(&config, configpath);
}

__attribute__((destructor))
void __fini(void)
{
    if (ret == 0)
        da_cloud_fini(&config);
}

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t datalen) {
    const auto useragent = std::string(reinterpret_cast<const char *>(data), datalen);
    if (ret == 0) {
        da_cloud_header_head head;
        da_cloud_property_head phead;
        da_cloud_header_init(&head);
        try {
            locale lc("");
            da_cloud_language_add(&head, lc.name().c_str());
        } catch (runtime_error &e) {
            cerr << e.what() << endl;
            da_cloud_header_free(&head);
            da_cloud_fini(&config);
            return (-1);
        }

        da_cloud_useragent_add(&head, useragent.c_str());
        if (da_cloud_detect(&config, &head, &phead) == 0) {
            struct da_cloud_property *p;
            da_list_foreach(p, &phead.list) {
                da_cloud_property_type type = p->type;
                cout << p->name << ": ";
                switch (type) {
                case DA_CLOUD_LONG:
                    cout << p->value.l;
                    break;
                case DA_CLOUD_BOOL:
                    cout << (p->value.l > 0 ? "true" : "false");
                    break;
                case DA_CLOUD_STRING:
                case DA_CLOUD_UNKNOWN:
                    cout << p->value.s;
                    break;
                }
                cout << endl;
            }
            cout << "cache set: " << da_cloud_cache_id(&config) << endl;
            cout << "cache source: " << phead.cachesource << endl;
            cout << "cache key: " << head.cachekey << endl;
            cout << "user-agent: " << useragent << endl;
            da_cloud_properties_free(&phead);
        }
        da_cloud_header_free(&head);
    }
    return (0);
}

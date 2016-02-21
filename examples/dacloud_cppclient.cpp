#include <iostream>
#include <cstring>
#include <cstdlib>
#include "dacloud.h"

using namespace std;

int
main(int argc, char *argv[]) {
    if (argc < 3)
        return (-1);
    string configpath = argv[1];
    string useragent = argv[2];
    da_cloud_config config;
    ::memset(&config, 0, sizeof(config));
    if (da_cloud_init(&config, configpath.c_str()) == 0) {
        da_cloud_header_head head;
        da_cloud_property_head phead;
        da_cloud_header_init(&head);
        da_cloud_header_add(&head, "user-agent", useragent.c_str());
        if (argc > 3) {
            string clientside = argv[3];
            da_cloud_clientside_add(&head, clientside.c_str());
        }
        if (da_cloud_detect(&config, &head, &phead) == 0) {
            struct da_cloud_property *p;
            SLIST_FOREACH(p, &phead.list, entries) {
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
            cout << "source: " << phead.cachesource << endl;
            cout << "cache set: " << da_cloud_cache_id(&config) << endl;
            da_cloud_properties_free(&phead);
        }
        da_cloud_header_free(&head);
    }

    da_cloud_fini(&config);
    return (0);
}

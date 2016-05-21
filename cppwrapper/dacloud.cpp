#include "dacloud.hpp"

#include <cstring>

Da::DaCloudHeaders::DaCloudHeaders()
{
    int ret = da_cloud_header_init(&hhead);
    SetSet((ret == 0));
}

Da::DaCloudHeaders::~DaCloudHeaders()
{
    if (IsSet())
        da_cloud_header_free(&hhead);
}

int
Da::DaCloudHeaders::Add(const std::string &key, const std::string &value)
{
    if (IsSet())
        return (da_cloud_header_add(&hhead, key.c_str(), value.c_str())); 
    return (-1);
}

int
Da::DaCloudHeaders::AddClientSide(const std::string &value)
{
    if (IsSet())
        return (da_cloud_clientside_add(&hhead, value.c_str())); 
    return (-1);
}

int
Da::DaCloudHeaders::AddUserAgent(const std::string &value)
{
    if (IsSet())
        return (da_cloud_useragent_add(&hhead, value.c_str())); 
    return (-1);
}

Da::DaCloud::DaCloud(std::string &configpath)
{
    int ret = da_cloud_init(&cfg, configpath.c_str());
    if (ret == 0)
        cache_id = std::string(da_cloud_cache_id(&cfg));
    SetSet((ret == 0));
}

Da::DaCloud::~DaCloud()
{
    if (IsSet())
        da_cloud_fini(&cfg);
}

Da::DaCloudDetect::DaCloudDetect(Da::DaCloud &dc, Da::DaCloudHeaders &header, struct da_cloud_property_head &_phead)
{
    ::memset(&phead.list, 0, sizeof(phead.list));
    if (dc.IsSet() && header.IsSet()) {
        struct da_cloud_header_head hhead = header.RealObject();
        int ret = da_cloud_detect(&dc.Cfg(), &hhead, &phead);
        if (ret == 0) {
            cache_source = std::string(phead.cachesource);
            cache_key = std::string(hhead.cachekey);
        }
    }
    _phead = phead;
}

Da::DaCloudDetect::DaCloudDetect(Da::DaCloud &dc, Da::DaCloudHeaders &header, Da::DaCloudProperties &props)
{
    ::memset(&phead.list, 0, sizeof(phead.list));
    if (dc.IsSet() && header.IsSet()) {
        struct da_cloud_header_head hhead = header.RealObject();
        props.clear();
        int ret = da_cloud_detect(&dc.Cfg(), &hhead, &phead);
        if (ret == 0) {
            struct da_cloud_property *p;
            da_list_foreach(p, &phead.list) {
                auto prop = std::make_shared<struct da_cloud_property>(*p);
                auto pname = std::string(prop->name);
                auto pr = std::make_pair(pname, prop);
                props.insert(pr);
            }
        
            cache_source = std::string(phead.cachesource);
            cache_key = std::string(hhead.cachekey);
        }
    }
}

Da::DaCloudDetect::~DaCloudDetect()
{
    da_cloud_properties_free(&phead);
}

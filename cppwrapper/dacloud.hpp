/**
 *
 *  Copyright (C) 2016  David Carlier <devnexen@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3.0 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.
 */
#ifndef DACLOUD_HPP
#define DACLOUD_HPP
#include "dacloud.h"

#include <map>
#include <memory>
#include <string>

namespace Da {

typedef std::map<std::string, std::shared_ptr<struct da_cloud_property>> DaCloudProperties;

class DaBase {
protected:
    bool set = 0;
public:
    bool IsSet() const { return set; }
    void SetSet(bool _set) { set = _set; }
};

class DaCloudHeaders : public DaBase {
private:
    struct da_cloud_header_head hhead;
public:
    DaCloudHeaders();
    DaCloudHeaders(const DaCloudHeaders &) = delete;
    ~DaCloudHeaders();
    int Add(const std::string &, const std::string &);
    int AddClientSide(const std::string &);
    int AddUserAgent(const std::string &);
    struct da_cloud_header_head &RealObject(void) { return hhead; }
};

class DaCloud : public DaBase {
private:
    struct da_cloud_config cfg;
    std::string cache_id = std::string("none");
public:
    DaCloud(std::string &);
    DaCloud() = delete;
    DaCloud(const DaCloud &) = delete;
    ~DaCloud();
    struct da_cloud_config &Cfg(void) { return cfg; }
    const std::string &CacheId(void) const { return cache_id; } 
};

class DaCloudDetect {
private:
    struct da_cloud_property_head phead;;
    std::string cache_key = std::string("none");
    std::string cache_source = std::string("none");
public:
    DaCloudDetect() = delete;
    DaCloudDetect(DaCloud &, DaCloudHeaders &, struct da_cloud_property_head &);
    DaCloudDetect(DaCloud &, DaCloudHeaders &, DaCloudProperties &);
    ~DaCloudDetect();
    const std::string &CacheKey(void) const { return cache_key; }
    const std::string &CacheSource(void) const { return cache_source; }
};
}

#endif

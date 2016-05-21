#include "dacloud.hpp"

#include <iostream>
#include <sstream>
#include <thread>

using namespace Da;

void
threadop(size_t i, std::shared_ptr<DaCloud> dc, std::shared_ptr<DaCloudHeaders> dh, 
    DaCloudProperties dp)
{
    auto dd = std::make_shared<DaCloudDetect>(*dc, *dh, dp);
    std::cout << "thread " << i << ", model: " << dp["model"]->value.s << std::endl;
}

int
main(int argc, char *argv[])
{
    if (argc < 2)
        exit(-1);
    std::string configpath(argv[1]);
    const std::string uakey = "user-agent";
    const std::string uavalue = "iPhone";
    auto dc = std::make_shared<DaCloud>(configpath); 
    auto dh = std::make_shared<DaCloudHeaders>();
    DaCloudProperties dp = DaCloudProperties();
    
    dh->Add(uakey, uavalue);
    auto dd = std::make_shared<DaCloudDetect>(*dc, *dh, dp);
    for (auto p: dp) {
        std::cout << p.first << ": ";
        switch (p.second->type) {
        case DA_CLOUD_LONG:
        case DA_CLOUD_BOOL:
            std::cout << p.second->value.l << std::endl;;
            break;
        default:
            std::cout << p.second->value.s << std::endl;
            break;
        }
    }
    std::cout << dp.size() << " properties" << std::endl;   
    std::cout << "cache key : " << dd->CacheKey() << std::endl;
    std::cout << "cache source : " << dd->CacheSource() << std::endl;

    std::thread td[2] { std::thread(threadop, 0, dc, dh, dp),
        std::thread(threadop, 1, dc, dh, dp) };

    for (size_t i = 0; i < 2; i ++)
        td[i].join();
}

#include "dacloud.hpp"

#include <iostream>
#include <sstream>
#include <thread>

using namespace Da;
using namespace std;


static std::string pgname;

static void
usage()
{
    std::cout << pgname << " <config path> <user-agent>" << std::endl;
    ::exit(-1);
}

void
threadop(size_t i, std::shared_ptr<DaCloud> dc, std::shared_ptr<DaCloudHeaders> dh, 
    DaCloudProperties dp)
{
    auto dd = std::make_shared<DaCloudDetect>(*dc, *dh, dp);
    fprintf(stderr, "thread %zd, model: %s\n", i, dp.find("model") != dp.end() ? dp["model"]->value.s : "/");
}

int
main(int argc, char *argv[])
{
    pgname = std::string(argv[0]);
    if (argc < 3)
	usage();
    std::string configpath(argv[1]);
    const std::string uakey = "user-agent";
    const std::string uavalue = argv[2];
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

    std::thread td[10];
    for (size_t i = 0; i < sizeof(td) / sizeof(td[0]); i ++)
        td[i] = std::thread(threadop, i, dc, dh, dp);

    for (size_t i = 0; i < sizeof(td) / sizeof(td[0]); i ++)
        td[i].join();

    return (0);
}

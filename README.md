DeviceAtlas Cloud client C Api
==============================

It is a library which allows to use the DeviceAtlas Cloud service inside C/C++ code.
More informations can be found in this page to get a licence key :

https://deviceatlas.com/resources/getting-started-cloud

A local cache handler can be used to avoid using Cloud service hits with a same request.
At the moment a simple disk, memcached and redis cache solutions are supported.
3 samples configuration files are provided.
A new cache handler can be written via the cache_provider interface as long as the library used is multi thread safe.

1/ Operating systems

* Tested on Linux, FreeBSD, OpenBSD and MacOS. 
* Should work on NetBSD and Solaris based systems.

2/ Dependencies

a/ Mandatories
* GCC (tested with 4.2.1 version and above) or clang (tested with 3.4.1 version and above).
* CMake (tested with 2.8 version and above).
* LibCURl.
* LibConfig.

b/ Optional
* LibMemcached (works with old 0.4.x versions as well).
* LibHiRedis.

DeviceAtlas Cloud client C Api
==============================

[![Build Status](https://travis-ci.org/devnexen/deviceatlas-cloud-c.svg?branch=master)](https://travis-ci.org/devnexen/deviceatlas-cloud-c)

<a href="https://scan.coverity.com/projects/device-atlas-cloud-c">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/6557/badge.svg"/>
</a>

It is a library which allows to use the DeviceAtlas Cloud service inside C/C++ code.
More informations can be found in this page to get a licence key :

https://deviceatlas.com/resources/getting-started-cloud

A local cache handler can be used to avoid using Cloud service hits with a same request.

- At the moment a simple disk, a in-memory and memcached cache solutions are supported.
- A new cache handler can be written via the cache's provider interface as long as the library used is multi thread safe.
- 2 samples configuration files for each cache support are provided.
- 3 examples which represents sort of use cases, batch, C++ example, multi thread and a simple one for pure speed benchmarking.
- A simple C++ wrapper and an example (requires a C++11 compiler compliant, gcc >= 4.8 or clang).
- A simple Python 3 wrapper (requires Python 3 C headers installed).
- A basic travis build configuration.
- A basic test suite.

1/ Operating systems
====================

* Tested on Linux, FreeBSD, OpenBSD, NetBSD and MacOS. 
* Should work on Solaris based systems.

2/ Dependencies
===============

a/ Mandatories

* GCC (tested with 4.2.1 version and above) or clang (tested with 3.4.1 version and above).
* CMake (tested with 2.8 version and above).
* LibCURl.
* LibConfig.

b/ Optional

* LibMemcached (works with old 0.4.x versions as well).
* Glib 2.

3/ Examples
===========

- batch       : examples/dacloud_batch <configuration file path> < <file of user agents> (N.B: a sample batch file can be found inside examples folder)
- C++         : examples/dacloud_cppclient <configuration file path> <user-agent>
- MultiThread : examples/dacloud_mt <configuration file path>
- Benchmark   : examples/dacloud_simplebench <configuration file path> [ number of iterations, per default 1000 ]

4/ Wrappers
===========

- The C++ wrapper is provided with one example with both mono and multithread usage
- examples/example <configuration file path>
- The Python wrapper needs to be compiled as follow
- CFLAGS=... LDFLAGS=... python setup.py build
- A basic example is provided inside examples subfolder

[![Build Status](https://travis-ci.org/devnexen/deviceatlas-cloud-c.svg?branch=master)](https://travis-ci.org/devnexen/deviceatlas-cloud-c)

<a href="https://scan.coverity.com/projects/device-atlas-cloud-c">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/6557/badge.svg"/>
</a>

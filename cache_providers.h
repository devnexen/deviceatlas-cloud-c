/**
 *
 *  Copyright (C) 2015  David Carlier <devnexen@gmail.com>
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

#ifndef  CACHE_PROVIDERS_H
#define  CACHE_PROVIDERS_H

#include "file_cache_provider.h"

#ifdef   HAVE_MEMCACHED
#include "memcached_cache_provider.h"
#endif

#define  CACHE_SET(cops, name)         	\
    cops->init = name ## _cache_init; 	\
    cops->get = name ## _cache_get;     \
    cops->set = name ## _cache_set;     \
    cops->fini = name ## _cache_fini

#endif

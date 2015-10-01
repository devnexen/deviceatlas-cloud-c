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

#include "dacloud_cache.h"

int file_cache_init(struct da_cloud_cache_cfg *);
int file_cache_get(struct da_cloud_cache_cfg *, const char *, char **);
int file_cache_set(struct da_cloud_cache_cfg *, const char *, const char *);
void file_cache_fini(struct da_cloud_cache_cfg *);

#endif


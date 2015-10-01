/**
 *
 *  Copyright (C) 2015  David Carlier <devnexen@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef  FILE_CACHE_PROVIDER_H
#define  FILE_CACHE_PROVIDER_H

#include "dacloud_cache.h"

int file_cache_init(struct da_cloud_cache_cfg *);
int file_cache_get(struct da_cloud_cache_cfg *, const char *, char **);
int file_cache_set(struct da_cloud_cache_cfg *, const char *, const char *);
void file_cache_fini(struct da_cloud_cache_cfg *);

#endif


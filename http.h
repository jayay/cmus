/*
 * Copyright 2008-2020 Various Authors
 * Copyright 2004-2005 Timo Hirvonen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CMUS_HTTP_H
#define CMUS_HTTP_H

#include "config/curl.h"

#ifndef CONFIG_CURL
#include "http_legacy.h"
typedef struct http_get http_priv;
#else 
#include <curl/curl.h>
typedef CURL http_priv;
#endif

int http_init(http_priv **p);
int http_set_url(http_priv *r, const char *uri);
int http_set_basic_auth(http_priv *r, char *user, char *pass);
int http_set_proxy(http_priv *r, char *host, char *user, char *pass);
void http_set_write_cb(http_priv *r, int (char *));
int http_perform(http_priv *r);
void http_free(http_priv *r);

#endif
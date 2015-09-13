#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <libconfig.h>
#include <jansson.h>

#include "dacloud.h"

#ifndef	__DBL_EPSILON
#define	__DBL_EPSILON	DBL_EPSILON
#endif

#define	API_VERSION		"1.4.1"
#define	CACHE_EXPIRATION	2952000

static int _da_cloud_servers_fireup(struct da_cloud_server_head *);

void
da_cloud_print_server(FILE *fp, struct da_cloud_server *s) {
    if (fp != NULL)
        fprintf(fp, "host(%s), port(%d), rank(%4.4f)\n", s->host,
                s->port, s->response_time);
}

int
da_cloud_header_init(struct da_cloud_header_head *head)
{
    SLIST_INIT(&head->list);
    return (0);
}

int
da_cloud_header_add(struct da_cloud_header_head *head,
        const char *key, const char *value) {
#define  CLOUD_HEADER_PREFIX     "X-DA-"
    struct da_cloud_header *dh = malloc(sizeof(*dh));
    if (dh == NULL)
        return (-1);
    size_t keylen = strlen(key) + sizeof(CLOUD_HEADER_PREFIX);
    dh->key = malloc(sizeof(char) * keylen + sizeof(CLOUD_HEADER_PREFIX) + 1);
    strncpy(dh->key, CLOUD_HEADER_PREFIX, sizeof(CLOUD_HEADER_PREFIX));
    strncat(dh->key, key, keylen);
    *(dh->key + keylen) = 0;
    dh->value = strdup(value);
    SLIST_INSERT_HEAD(&head->list, dh, entries);
    return (0);
}

void
da_cloud_header_free(struct da_cloud_header_head *head) {
    struct da_cloud_header *dh = SLIST_FIRST(&head->list);
    while (!SLIST_EMPTY(&head->list)) {
        SLIST_REMOVE_HEAD(&head->list, entries);
        free(dh->value);
        free(dh->key);
        free(dh);
        dh = SLIST_FIRST(&head->list);
    }
}

void
da_cloud_properties_free(struct da_cloud_property_head *phead) {
    struct da_cloud_property *p = SLIST_FIRST(&phead->list);
    while (!SLIST_EMPTY(&phead->list)) {
        SLIST_REMOVE_HEAD(&phead->list, entries);
        if (p->type == DA_CLOUD_STRING)
            free(p->value.s);
        free(p->name);
        free(p);
        p = SLIST_FIRST(&phead->list);
    }
}

int
da_cloud_init(struct da_cloud_config *config, const char *confpath) {
    config->shead = calloc(1, sizeof(*config->shead));
    if (config->shead == NULL) {
        fprintf(stderr, "servers list allocation failed\n");
        return (-1);
    }

    memset(&config->cache_cfg, 0, sizeof(config->cache_cfg));

    config->cache_cfg.expiration = CACHE_EXPIRATION;
    config->cops.init = mock_cache_init;
    config->cops.get = mock_cache_get;
    config->cops.set = mock_cache_set;
    config->cops.fini = mock_cache_fini;

    config_t cfg;
    config_setting_t *servers, *server;
    const char *licence_key, *cache_name, *cache_string;
    size_t nservers = 0;
    config_init(&cfg);
    if (config_read_file(&cfg, confpath) != CONFIG_TRUE) {
        fprintf(stderr, "%s: invalid config file\n", confpath);
        return (-1);
    }
    if (config_lookup_string(&cfg, "user.licence_key", &licence_key) != CONFIG_TRUE) {
        fprintf(stderr, "%s: could not find user.licence_key value\n", confpath);
    } else {
        config_lookup_string(&cfg, "user.cache.type", &cache_name);
        config_lookup_string(&cfg, "user.cache.config", &cache_string);
    }
    if ((servers = config_lookup(&cfg, "servers")) == NULL) {
        fprintf(stderr, "%s: could not find servers config\n", confpath);
        config_destroy(&cfg);
        return (-1);
    }

    config->licence_key = strdup(licence_key);

    if (cache_name != NULL && cache_string != NULL) {
        config->cache_cfg.cache_cfg_str = strdup(cache_string);
        cache_set(&config->cops, cache_name);
        if (config->cops.init(&config->cache_cfg) == -1) {
            free(config->cache_cfg.cache_cfg_str);
            fprintf(stderr, "could not set %s cache\n", cache_name);
            config_destroy(&cfg);
            return (-1);
        }
	free(config->cache_cfg.cache_cfg_str);
    }

    while ((server = config_setting_get_elem(servers, nservers)) != NULL) {
        struct da_cloud_server *s;
        const char *host;
        int tmp;
        unsigned short port = 80;
        if (config_setting_lookup_string(server, "host", &host) != CONFIG_TRUE) {
            fprintf(stderr, "%s: did not find host setting (%d)\n", confpath, (int)nservers);
            continue;
        }
        if (config_setting_lookup_int(server, "port", &tmp) == CONFIG_TRUE) {
            if (tmp >= 80 && tmp <= 65535)
                port = (unsigned short)tmp;
        }
        config->shead->servers = realloc(config->shead->servers,
                (++nservers * sizeof(*config->shead->servers)));
        s = malloc(sizeof(*s));
        s->host = strdup(host);
        s->port = port;
        config->shead->servers[nservers - 1] = s;
        config->shead->nb = nservers;
    }

    config_destroy(&cfg);
    if (nservers == 0)
        return (-1);
    curl_global_init(CURL_GLOBAL_NOTHING);
    return (_da_cloud_servers_fireup(config->shead));
}

static int
_servers_cmp(const void *sa, const void *sb) {
    const struct da_cloud_server *const*a = sa;
    const struct da_cloud_server *const*b = sb;

    const double diff = (*a)->response_time - (*b)->response_time;
    if (diff > __DBL_EPSILON)
        return (1);
    else if (diff < __DBL_EPSILON)
        return (-1);
    else
        return (0);
}

struct data_reader {
    char *buf;
    size_t buflen;
};

static void
data_reader_free(struct data_reader *dr) {
    if (dr != NULL) {
        if (dr->buf != NULL)
            free(dr->buf);
        free(dr);
        dr = NULL;
    }
}

static size_t
_write_mock(char *p, size_t size, size_t nb, void *arg) {
    return (size * nb);
}

static int
_da_cloud_servers_fireup(struct da_cloud_server_head *shead) {
    struct da_cloud_server *s;
    size_t i = 0 ;
    int _ret = -1;
    for (i = 0; i < shead->nb; i ++) {
        CURL *c = curl_easy_init();
        if (c == NULL)
            return (-1);
        s = *(shead->servers + i);
        long response_code;
        s->response_time = (double)-1;
        curl_easy_setopt(c, CURLOPT_URL, s->host);
        curl_easy_setopt(c, CURLOPT_PORT, (long)s->port);
        curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, _write_mock);
        curl_easy_setopt(c, CURLOPT_WRITEDATA, NULL);
        CURLcode ret = curl_easy_perform(c);
        if (ret == CURLE_OK) {
            curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code == 200) {
                curl_easy_getinfo(c, CURLINFO_TOTAL_TIME, &s->response_time);
                if (_ret == -1)
                    _ret = 0;
            }
        }
        curl_easy_cleanup(c);
    }

    qsort(shead->servers, shead->nb, sizeof(*shead->servers), _servers_cmp);
    return (_ret);
}

static size_t
_write_servers_response(char *data, size_t size, size_t nb, void *arg) {
    struct data_reader *dr = (struct data_reader *)arg;
    size_t total = (size * nb);
    if (dr->buflen == 0) {
        dr->buf = malloc(sizeof(char) * total + 1);
        memcpy(dr->buf, data, total);
    } else {
        char *p = strndup(dr->buf, dr->buflen);
        dr->buf = realloc(dr->buf, sizeof(char) * (dr->buflen + total + 1));
        memcpy(dr->buf, p, dr->buflen);
        memcpy(dr->buf + dr->buflen, data, total);
        free(p);
    }

    dr->buflen += total;
    return (total);
}

int
da_cloud_detect(struct da_cloud_config *config, struct da_cloud_header_head *head,
        struct da_cloud_property_head *phead) {
#define  DETECT_URL_FORMAT "%s:%d/v1/detect/properties?licenceKey=%s"
#define  DETECT_HDR_FORMAT "%s: %s"
    if (phead == NULL) {
        fprintf(stderr, "properties cannot be null\n");
        return (-1);
    }

    CURL *c = NULL;
    struct da_cloud_server *s;
    struct da_cloud_header *h;
    struct curl_slist *hd = NULL;
    json_error_t err;
    json_t *response = NULL;
    char *cacheval = NULL;
    char cachekeybuf[1024] = { 0 }, cachekey[65] = { 0 };
    size_t i;
    int _ret = -1;
    SLIST_INIT(&phead->list);
    *phead->cachesource = 0;

    SLIST_FOREACH(h, &head->list, entries) {
        size_t keylen = strlen(h->key);
        size_t valuelen = strlen(h->value);
        size_t cachekeybuflen = strlen(cachekeybuf);
        if ((sizeof(cachekeybuf) - cachekeybuflen) > (keylen + valuelen)) {
            if (cachekeybuflen == 0)
                strncpy(cachekeybuf, h->key, keylen);
            else
                strncat(cachekeybuf, h->key, keylen);
            strncat(cachekeybuf, h->value, valuelen);
        }
    }

    da_cloud_crypt_key(cachekeybuf, sizeof(cachekeybuf), cachekey, sizeof(cachekey));
    config->cops.get(&config->cache_cfg, cachekey, &cacheval);
    if (cacheval != NULL) {
        strcpy(phead->cachesource, "cache");
        goto jsoninit;
    }

    c = curl_easy_init();
    if (c == NULL)
        return (-1);

    SLIST_FOREACH(h, &head->list, entries) {
        char buf[256];
        snprintf(buf, sizeof(buf), DETECT_HDR_FORMAT, h->key, h->value);
        hd = curl_slist_append(hd, buf);
    }

    hd = curl_slist_append(hd, "Accept: application/json");
    hd = curl_slist_append(hd, "User-Agent: c/"API_VERSION);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hd);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, _write_servers_response);
    for (i = 0; i < config->shead->nb; i ++) {
        struct data_reader *dr = calloc(1, sizeof(*dr));
        char urlbuf[256];
        CURLcode ret;
        s = config->shead->servers[i];
        snprintf(urlbuf, sizeof(urlbuf), DETECT_URL_FORMAT, s->host,
                s->port, config->licence_key);
        curl_easy_setopt(c, CURLOPT_URL, urlbuf);
        curl_easy_setopt(c, CURLOPT_WRITEDATA, dr);
        ret = curl_easy_perform(c);

        if (ret == CURLE_OK) {
            *(dr->buf + dr->buflen) = 0;
            if (*dr->buf != '{') {
                fprintf(stderr, "invalid json: %s\n", dr->buf);
                data_reader_free(dr);
		curl_slist_free_all(hd);
		curl_easy_cleanup(c);
                goto end;
            }

            if (config->cops.set(&config->cache_cfg, cachekey, dr->buf) == -1)
                fprintf(stderr, "could not cache %s\n", cachekey);

            cacheval = strdup(dr->buf);
            strcpy(phead->cachesource, "cloud");
            data_reader_free(dr);
	    curl_slist_free_all(hd);
	    curl_easy_cleanup(c);
            _ret = 0;
            goto jsoninit;
        }

        data_reader_free(dr);
    }

    curl_slist_free_all(hd);
    curl_easy_cleanup(c);

 jsoninit:
    response = json_loads(cacheval, JSON_PRESERVE_ORDER, &err);
    if (strlen(err.text) > 0) {
        fprintf(stderr, "response: %s\n", err.text);
        fprintf(stderr, "got:\n%s\n", cacheval);
    } else {
        json_t *properties = json_object_get(response, "properties");
        void *it = json_object_iter(properties);
        while (it) {
            struct da_cloud_property *p = malloc(sizeof(*p));
            const char *key = json_object_iter_key(it);
            json_t *value = json_object_iter_value(it);
            int type = json_typeof(value);
            p->name = strdup(key);

            switch (type) {
                case JSON_STRING:
                default:
                    p->type = DA_CLOUD_STRING;
                    p->value.s = strdup(json_string_value(value));
                    break;
                case JSON_TRUE:
                    p->type = DA_CLOUD_BOOL;
                    p->value.l = 1;
                    break;
                case JSON_FALSE:
                    p->type = DA_CLOUD_BOOL;
                    p->value.l = 0;
                    break;
                case JSON_INTEGER:
                    p->type = DA_CLOUD_LONG;
                    p->value.l = json_integer_value(value);
                    break;
            }

            SLIST_INSERT_HEAD(&phead->list, p, entries);
            it = json_object_iter_next(properties, it);
        }

        json_decref(response);
    }

    free(cacheval);

end:
    return (_ret);
}

void
da_cloud_fini(struct da_cloud_config *config) {
    if (config->shead != NULL) {
        struct da_cloud_server *s;
        size_t i;
        for (i = 0; i < config->shead->nb; i ++) {
            s = *(config->shead->servers + i);
            free(s->host);
            free(s);
        }

        free(config->shead->servers);
        free(config->shead);
    }

    config->cops.fini(&config->cache_cfg);
    free(config->licence_key);
    curl_global_cleanup();
}

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

void
da_cloud_print_property(FILE *fp, struct da_cloud_property *p) {
    if (fp != NULL) {
        enum da_cloud_property_type type = p->type;
        fprintf(fp, "%s: ", p->name);
        switch (type) {
        case DA_CLOUD_LONG:
            fprintf(fp, "%d", (int)p->value.l);
            break;
        case DA_CLOUD_BOOL:
            fprintf(fp, "%s", (p->value.l > 0 ? "true" : "false"));
            break;
        case DA_CLOUD_STRING:
        case DA_CLOUD_UNKNOWN:
            fprintf(fp, "%s", p->value.s);
            break;
        default:
            break;
        }

        fprintf(fp, "\n");
    }
}

void
da_cloud_log(FILE *fp, const char *fmt, ...) {
    if (fp != NULL) {
        va_list args;
        va_start(args, fmt);
        time_t now = time(NULL);
        char strnow[26];
        if (ctime_r(&now, strnow) != NULL) {
            strnow[sizeof(strnow) - 2] = 0;
            fprintf(fp, "[%s]: ", strnow);
            vfprintf(fp, fmt, args);
            fprintf(fp, "\n");
        }

        va_end(args);
    }
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
    size_t keylen;
    struct da_cloud_header *dh = malloc(sizeof(*dh));
    if (dh == NULL)
        return (-1);
    keylen = strlen(key) + sizeof(CLOUD_HEADER_PREFIX);
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
        if (p->type == DA_CLOUD_STRING || p->type == DA_CLOUD_UNKNOWN)
            free(p->value.s);
        free(p->name);
        free(p);
        p = SLIST_FIRST(&phead->list);
    }

    memset(phead->cachesource, 0, sizeof(phead->cachesource));
}

int
da_cloud_init(struct da_cloud_config *config, const char *confpath) {
    config_setting_t *servers, *server;
    const char *licence_key, *cache_name, *cache_string, *error_path;
    config_t cfg;
    size_t nservers;
    config->efp = stderr;
    config->shead = calloc(1, sizeof(*config->shead));
    if (config->shead == NULL) {
        da_cloud_log(config->efp, "servers list allocation failed", NULL);
        return (-1);
    }

    memset(&config->cache_cfg, 0, sizeof(config->cache_cfg));

    config->cache_cfg.expiration = CACHE_EXPIRATION;
    config->cache_cfg.efp = config->efp;
    config->cache_cfg.efp = config->efp;
    config->cops.init = mock_cache_init;
    config->cops.get = mock_cache_get;
    config->cops.set = mock_cache_set;
    config->cops.fini = mock_cache_fini;

    cache_name = NULL;
    cache_string = NULL;
    error_path = NULL;
    nservers = 0;
    config_init(&cfg);
    if (config_read_file(&cfg, confpath) != CONFIG_TRUE) {
        da_cloud_log(config->efp, "%s: invalid config file", confpath, NULL);
        return (-1);
    }
    if (config_lookup_string(&cfg, "user.error_path", &error_path) == CONFIG_TRUE) {
        if (strcmp(error_path, "stdin") == 0 || strcmp(error_path, "2") == 0) {
            config->efp = stdin;
        } else {
            config->efp = fopen(error_path, "a");
            if (config->efp == NULL) {
                config->efp = stderr;
                fprintf(stderr, "cannot open error path '%s'\n", error_path);
            }
        }
    }

    config->cache_cfg.efp = config->efp;
    if (config_lookup_string(&cfg, "user.licence_key", &licence_key) != CONFIG_TRUE) {
        da_cloud_log(config->efp, "%s: could not find user.licence_key value", confpath, NULL);
    } else {
        config_lookup_string(&cfg, "user.cache.type", &cache_name);
        config_lookup_string(&cfg, "user.cache.config", &cache_string);
    }
    if ((servers = config_lookup(&cfg, "servers")) == NULL) {
        da_cloud_log(config->efp, "%s: could not find servers config", confpath, NULL);
        config_destroy(&cfg);
        return (-1);
    }

    config->licence_key = strdup(licence_key);

    if (cache_name != NULL && cache_string != NULL) {
        config->cache_cfg.cache_cfg_str = strdup(cache_string);
        cache_set(&config->cops, cache_name);
        if (config->cops.init(&config->cache_cfg) == -1) {
            free(config->cache_cfg.cache_cfg_str);
            config_destroy(&cfg);
            da_cloud_log(config->efp, "could not set %s cache", cache_name);
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
            da_cloud_log(config->efp, "%s: did not find host setting (%d)", confpath, (int)nservers, NULL);
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
        long response_code;
        CURLcode ret;
        CURL *c = curl_easy_init();
        if (c == NULL)
            return (-1);
        s = *(shead->servers + i);
        s->response_time = (double)-1;
        curl_easy_setopt(c, CURLOPT_URL, s->host);
        curl_easy_setopt(c, CURLOPT_PORT, (long)s->port);
        curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, _write_mock);
        curl_easy_setopt(c, CURLOPT_WRITEDATA, NULL);
        ret = curl_easy_perform(c);
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

    if (_ret == 0)
        qsort(shead->servers, shead->nb, sizeof(*shead->servers), _servers_cmp);
    else
        fprintf(stderr, "no servers available\n");
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
    struct da_cloud_server *s;
    struct da_cloud_header *h;
    struct curl_slist *hd;
    char *cacheval;
    json_t *response;
    json_error_t err;
    CURL *c;
    size_t i;
    int _ret;
    char cachekeybuf[1024] = { 0 }, cachekey[65] = { 0 };
    if (phead == NULL) {
        da_cloud_log(config->efp, "properties cannot be null", NULL);
        return (-1);
    }

    hd = NULL;
    c = NULL;
    response = NULL;
    cacheval = NULL;
    _ret = -1;
    strcpy(phead->cachesource, "none");
    SLIST_INIT(&phead->list);

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
        _ret = 0;
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
                da_cloud_log(config->efp, "error:\n%s", dr->buf, NULL);
                data_reader_free(dr);
                curl_slist_free_all(hd);
                curl_easy_cleanup(c);
                goto end;
            }

            if (config->cops.set(&config->cache_cfg, cachekey, dr->buf) == -1)
                da_cloud_log(config->efp, "could not cache %s", cachekey, NULL);

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
    if (strlen(cacheval) == 0)
        goto fcache;
    response = json_loads(cacheval, JSON_PRESERVE_ORDER, &err);
    if (strlen(err.text) > 0) {
        da_cloud_log(config->efp, "response: %s", err.text, NULL);
        da_cloud_log(config->efp, "got:\n%s", cacheval, NULL);
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
                default:
                    p->type = DA_CLOUD_UNKNOWN;
                    p->value.s = strdup("(unknown)");
                    break;
            }

            SLIST_INSERT_HEAD(&phead->list, p, entries);
            it = json_object_iter_next(properties, it);
        }

        json_decref(response);
    }

fcache:
    free(cacheval);

end:
    return (_ret);
}

void
da_cloud_fini(struct da_cloud_config *config) {
    if (config->shead != NULL) {
        size_t i;
        for (i = 0; i < config->shead->nb; i ++) {
            struct da_cloud_server *s = *(config->shead->servers + i);
            free(s->host);
            free(s);
        }

        free(config->shead->servers);
        free(config->shead);
    }

    config->cops.fini(&config->cache_cfg);
    if (config->efp != stdin && config->efp != stderr)
        fclose(config->efp);
    free(config->licence_key);
    curl_global_cleanup();
}

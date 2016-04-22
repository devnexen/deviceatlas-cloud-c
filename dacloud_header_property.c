#include <stdlib.h>
#include <string.h>

#include "dacloud.h"

void
da_cloud_print_header(FILE *fp, struct da_cloud_header *h) {
    if (fp != NULL)
        fprintf(fp, "%s => %s\n", h->orig_key, h->value);
}

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

int
da_cloud_header_init(struct da_cloud_header_head *head) {
    da_list_init(&head->list);
    head->cachekey = calloc(1, DACLOUD_CACHEKEY_SIZE * sizeof(char));
    if (head->cachekey == NULL)
        return (-1);

    return (0);
}

int
da_cloud_header_add(struct da_cloud_header_head *head,
        const char *key, const char *value) {
#define  CLOUD_HEADER_PREFIX     "X-DA-"
    size_t keylen;
    char cachekeybuf[1024] = { 0 };
    struct da_cloud_header *h;
    struct da_cloud_header *ddh, *dh = malloc(sizeof(*dh));
    if (dh == NULL)
        return (-1);
    keylen = strlen(key) + sizeof(CLOUD_HEADER_PREFIX);
    dh->key = malloc(sizeof(char) * keylen + sizeof(CLOUD_HEADER_PREFIX) + 1);
    strncpy(dh->key, CLOUD_HEADER_PREFIX, sizeof(CLOUD_HEADER_PREFIX));
    strncat(dh->key, key, keylen);
    *(dh->key + keylen) = 0;
    dh->orig_key = dh->key + (sizeof(CLOUD_HEADER_PREFIX) - 1);
    dh->value = strdup(value);
    da_list_foreach(ddh, &head->list) {
        if (strcmp(ddh->key, dh->key) == 0) {
            free(dh);
            return (-1);
        }
    }
    SLIST_INSERT_HEAD(&head->list, dh, entries);

    da_list_foreach(h, &head->list) {
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
    da_cloud_crypt_key(cachekeybuf, sizeof(cachekeybuf), head->cachekey, DACLOUD_CACHEKEY_SIZE);

    return (0);
}

int
da_cloud_clientside_add(struct da_cloud_header_head *head,
    const char *value) {
#define CLOUD_CLIENTSIDE_NAME   "Client-Properties"
    return da_cloud_header_add(head, CLOUD_CLIENTSIDE_NAME, value);
}

int
da_cloud_useragent_add(struct da_cloud_header_head *head,
    const char *value) {
#define USERAGENT_NAME          "User-Agent"
    return da_cloud_header_add(head, USERAGENT_NAME, value);
}

int
da_cloud_language_add(struct da_cloud_header_head *head,
    const char *value) {
#define LANGUAGE_NAME           "Accept-Language"
    return da_cloud_header_add(head, LANGUAGE_NAME, value);
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

    free(head->cachekey);
    head->cachekey = NULL;
}

int
da_cloud_property(struct da_cloud_property_head *phead, const char *pname,
    struct da_cloud_property **prop) {
	if (prop == NULL)
		return (-1);
	da_list_foreach(*prop, &phead->list) {
		if (strcasecmp((*prop)->name, pname) == 0)
			return (0);
	}

	*prop = NULL;

	return (-1);
}

int
da_cloud_property_count(struct da_cloud_property_head *phead, size_t *count) {
	struct da_cloud_property *p;
	size_t ct;
	if (count == NULL)
		return (-1);
	*count = ct = 0;
	da_list_foreach(p, &phead->list) {
		ct ++;
	}

	*count = ct;

	return (0);
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

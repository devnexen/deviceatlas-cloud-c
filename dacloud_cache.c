#include <sys/types.h>
#include <openssl/sha.h>
#include <string.h>
#include <err.h>

#include "dacloud_cache.h"

// the sha256's api is itself obsolete since openssl 3.0
void
da_cloud_crypt_key(char *source, size_t sourcelen, char *result, size_t resultlen) {
    SHA256_CTX ctx;
    u_int8_t digest[SHA256_DIGEST_LENGTH];
    u_int8_t *p;
    static u_int8_t a[] = "0123456789abcdef";
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, source, sourcelen);
    SHA256_Final(digest, &ctx);
    p = digest;

    resultlen /= 2;
    while (resultlen --) {
        *result ++ = a[*p >> 4];
        *result ++ = a[*p ++ & 0xf];
    }

    *result = 0;
}

const char *
mock_cache_id(void) { return "mock"; }

int
mock_cache_init(struct da_cloud_cache_cfg *cfg) { (void)cfg; return (0); }

int
mock_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) { (void)cfg; (void)key; (void)value; return (0);  }

int
mock_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) { (void)cfg; (void)key; (void)value; return (0); }

void
mock_cache_fini(struct da_cloud_cache_cfg *cfg) { (void)cfg; }

void
cache_set(struct da_cloud_cache_ops *cops, const char *cache_name) {
    if (strcasecmp(cache_name, "file") == 0) {
        CACHE_SET(cops, file);
#ifdef	HAVE_MEMCACHED
    } else if (strcasecmp(cache_name, "memcached") == 0) {
        CACHE_SET(cops, memcached);
#endif
    } else if (strcasecmp(cache_name, "memory") == 0) {
        CACHE_SET(cops, memory);
    } else if (strcasecmp(cache_name, "mock") == 0) {
        CACHE_SET(cops, mock);
    } else {
        errx(1, "%s cache invalid", cache_name);
    }
}

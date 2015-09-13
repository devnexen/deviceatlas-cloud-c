#include <sys/types.h>
#include <openssl/sha.h>

#include "dacloud_cache.h"

void
da_cloud_crypt_key(char *source, size_t sourcelen, char *result, size_t resultlen) {
    SHA256_CTX ctx;
    size_t i;
    u_int8_t digest[SHA256_DIGEST_LENGTH];
    static u_int8_t a[] = "0123456789abcdef";
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, source, sourcelen);
    SHA256_Final(digest, &ctx);
    u_int8_t *p = digest;

    resultlen /= 2;
    while (resultlen --) {
        *result ++ = a[*p >> 4];
        *result ++ = a[*p ++ & 0xf];
    }

    *result = 0;
}

int
mock_cache_init(struct da_cloud_cache_cfg *cfg) { (void)cfg; return (0); }

int
mock_cache_get(struct da_cloud_cache_cfg *cfg, const char *key, char **value) { (void)cfg; (void)key; (void)value;  }

int
mock_cache_set(struct da_cloud_cache_cfg *cfg, const char *key, const char *value) { (void)cfg; (void)key; (void)value; }

void
mock_cache_fini(struct da_cloud_cache_cfg *cfg) { (void)cfg; }

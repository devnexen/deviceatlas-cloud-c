#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <dacloud.h>
#include <stdlib.h>

static struct da_cloud_config config;

void
da_cloud_properties_test(void) {
	struct da_cloud_header_head headers;
	struct da_cloud_property_head properties; 
	struct da_cloud_property *p;
	const char *property;
	size_t count;

	property = "isRobot";

	da_cloud_header_init(&headers);
	da_cloud_header_add(&headers, "user-agent", "Googlebot-Image/1.0");
	CU_ASSERT_FATAL(da_cloud_detect(&config, &headers, &properties) == 0);
	CU_ASSERT(da_cloud_property_count(&properties, &count) == 0);
	CU_ASSERT(count == 9);
	CU_ASSERT_FATAL(da_cloud_property(&properties, property, &p) == 0);
	CU_ASSERT_STRING_EQUAL(p->name, property);
	CU_ASSERT(p->value.l == 1);
	da_cloud_properties_free(&properties);
	da_cloud_header_free(&headers);
}

void
da_cloud_simple_lookup(void) {
	struct da_cloud_header_head headers;
	struct da_cloud_property_head properties; 
	da_cloud_header_init(&headers);
	da_cloud_header_add(&headers, "user-agent", "iPhone");
	CU_ASSERT_FATAL(da_cloud_detect(&config, &headers, &properties) == 0);
	da_cloud_properties_free(&properties);
	da_cloud_header_free(&headers);
}

void
da_cloud_headers_test(void) {
	struct da_cloud_header_head headers;
	CU_ASSERT(da_cloud_header_init(&headers) == 0);
	CU_ASSERT(da_cloud_header_add(&headers, "user-agent", "iPhone") == 0);
	CU_ASSERT(da_cloud_header_add(&headers, "accept-language", "en-US") == 0);
	da_cloud_header_free(&headers);
}

int
main(int argc, char *argv[]) {
	CU_ErrorCode cu;
	CU_pSuite suite;
	const char *configpath;
	if (argc < 2) {
		fprintf(stderr, "please gives a configuration file\n");
		exit(-1);
	}

	cu = CU_initialize_registry();
	configpath = argv[1];

	if (da_cloud_init(&config, configpath) == 0) {
		suite = CU_add_suite("da_cloud", NULL, NULL);
		CU_add_test(suite, "da_cloud_headers_test", da_cloud_headers_test);
		CU_add_test(suite, "da_cloud_simple_lookup", da_cloud_simple_lookup);
		CU_add_test(suite, "da_cloud_properties_test", da_cloud_properties_test);
		CU_automated_run_tests();
	}
	da_cloud_fini(&config);
	CU_cleanup_registry();

	return (0);
}

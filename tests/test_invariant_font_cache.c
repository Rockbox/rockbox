#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../firmware/font_cache.c"

START_TEST(test_font_cache_bounds_integrity)
{
    // Invariant: memmove operations must never write beyond allocated _index buffer bounds
    
    typedef struct {
        uint16_t glyph_count;
        uint16_t cache_size;
        const char *description;
    } test_payload;
    
    test_payload payloads[] = {
        {0xFFFF, 16, "max_glyph_overflow"},
        {256, 8, "boundary_cache_small"},
        {10, 32, "valid_normal"}
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        font_cache_t *fcache = malloc(sizeof(font_cache_t));
        ck_assert_ptr_nonnull(fcache);
        
        fcache->_index = calloc(payloads[i].cache_size, sizeof(font_cache_entry_t));
        ck_assert_ptr_nonnull(fcache->_index);
        
        uint8_t *guard_before = malloc(16);
        uint8_t *guard_after = malloc(16);
        memset(guard_before, 0xAA, 16);
        memset(guard_after, 0xBB, 16);
        
        size_t index_size = payloads[i].cache_size * sizeof(font_cache_entry_t);
        uint8_t *monitored_region = malloc(16 + index_size + 16);
        memcpy(monitored_region, guard_before, 16);
        memcpy(monitored_region + 16 + index_size, guard_after, 16);
        
        free(fcache->_index);
        fcache->_index = (font_cache_entry_t *)(monitored_region + 16);
        fcache->size = payloads[i].cache_size;
        
        for (uint16_t glyph = 0; glyph < payloads[i].glyph_count && glyph < 100; glyph++) {
            font_cache_get(fcache, glyph);
        }
        
        ck_assert_msg(memcmp(monitored_region, guard_before, 16) == 0,
                      "Buffer underflow detected for %s", payloads[i].description);
        ck_assert_msg(memcmp(monitored_region + 16 + index_size, guard_after, 16) == 0,
                      "Buffer overflow detected for %s", payloads[i].description);
        
        free(monitored_region);
        free(guard_before);
        free(guard_after);
        free(fcache);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_font_cache_bounds_integrity);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
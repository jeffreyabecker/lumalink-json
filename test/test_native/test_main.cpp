#include <unity.h>

#include "test_catalog.h"

void setUp(void) {}

void tearDown(void) {}

int main() {
    UNITY_BEGIN();
    run_phase1_harness_tests();
    run_phase2_scalar_tests();
    run_phase3_composite_tests();
    return UNITY_END();
}

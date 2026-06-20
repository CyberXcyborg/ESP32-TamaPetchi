// Native test bridge — compiled only for native test builds
// Provides the minimal source presence PlatformIO needs to build the native environment.
// This file is excluded from ESP32 builds via build_src_filter.
#if defined(UNIT_TEST) || defined(__linux__) || defined(__APPLE__)
// Intentionally minimal — test files in test/ provide all test code.
// The build_src_filter includes this file so PlatformIO finds at least one source in src/.
void native_test_bridge_placeholder() {}
#endif

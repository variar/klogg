# Add test cpp file
add_executable(klogg_tests
    linepositionarray_test.cpp
    patternmatcher_test.cpp
    tests_main.cpp
)

target_link_libraries(klogg_tests klogg_ui klogg_utils klogg_logging catch2_wrapper Qt${QT_VERSION_MAJOR}::Test)
set_target_properties(klogg_tests PROPERTIES AUTOMOC ON)

add_test(
    NAME klogg_tests
    COMMAND klogg_tests -platform offscreen
)
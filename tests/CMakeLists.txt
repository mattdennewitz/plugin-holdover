CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    GIT_TAG v3.5.2)

add_executable(HoldoverTests
    test_main.cpp
    test_scales.cpp
    test_parameters.cpp
    test_filter.cpp
    test_eq.cpp
    test_compressor.cpp
    test_drive.cpp
    test_inputstage.cpp
    test_matrix.cpp
    test_ceiling.cpp
    test_output.cpp
    test_channelstrip.cpp
    test_presets.cpp)

target_link_libraries(HoldoverTests PRIVATE HoldoverDSP Catch2::Catch2WithMain)
target_compile_features(HoldoverTests PRIVATE cxx_std_20)

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(HoldoverTests)

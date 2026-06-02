cmake_minimum_required(VERSION 3.22)
project(Holdover VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build for the host architecture (no universal binary). Deployment target only.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)

include(cmake/CPM.cmake)
CPMAddPackage(
    NAME JUCE
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG 8.0.4
    OPTIONS "JUCE_BUILD_EXTRAS OFF" "JUCE_BUILD_EXAMPLES OFF")

enable_testing()

add_subdirectory(Source)   # defines HoldoverDSP + Holdover (added in later tasks)
add_subdirectory(tests)

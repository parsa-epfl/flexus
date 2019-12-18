cmake_minimum_required(VERSION 3.10.2)

# Check if top level or invoked from the parent
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Top level, invoked only here
    # TODO
else()
    # Called from parent (e.g., during a flexus build)
    set(REL_PATH ${CMAKE_SOURCE_DIR}/build)
    set(STAT_MANAGER_PATH ${FLEXUS_ROOT}/stat-manager)
endif()

# Get sources from Flexus core
set(CORE_STATS_SRCS
    ${FLEXUS_ROOT}/core/stats/stats.cpp
    ${FLEXUS_ROOT}/core/stats/stats_calc.cpp
    ${FLEXUS_ROOT}/core/stats/measurement.cpp
    )
file(GLOB_RECURSE CORE_DEBUG_SRCS ${FLEXUS_ROOT}/core/debug/*.cpp)

# Add a static library for the core stats srcs and core dbg srcs
add_library(stats_flexcore STATIC ${CORE_STATS_SRCS}
    ${CORE_DEBUG_SRCS})
target_compile_options(stats_flexcore PRIVATE -c ${GCC_FLAGS})
target_link_libraries(stats_flexcore ${GCC_LDFLAGS} "-L${BOOST_LIBRARYDIR}" boost_system boost_regex boost_serialization boost_iostreams z)

# Add 3 executable targets:
# stat manager, stat sample, stat collapse
add_executable(stat-manager ${STAT_MANAGER_PATH}/stat-manager.cpp)
target_link_libraries(stat-manager pthread stats_flexcore)
add_executable(stat-sample ${STAT_MANAGER_PATH}/stat-sample.cpp)
target_link_libraries(stat-sample pthread stats_flexcore)
add_executable(stat-collapse ${STAT_MANAGER_PATH}/stat-collapse.cpp)
target_link_libraries(stat-collapse pthread stats_flexcore)

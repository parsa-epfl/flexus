set(FLEXUS_ROOT ${CMAKE_SOURCE_DIR})

# add option -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# add option --std=c++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT TARGET_PLATFORM)
  message( STATUS "TARGET_PLATFORM was not specified; using default TARGET_PLATFORM=arm" )
  set(TARGET_PLATFORM arm)
endif()

if(NOT SELECTED_DEBUG)
  message( STATUS "SELECTED_DEBUG was not specified; using default SELECTED_DEBUG=vverb" )
  set(SELECTED_DEBUG vverb)
endif()

if(NOT SIMULATOR)
  message( STATUS "SIMULATOR was not specified; using default SIMULATOR=KnottyKraken" )
  set(SIMULATOR KnottyKraken)
endif()

if(NOT BOOST_INCLUDEDIR)
  message( STATUS "BOOST_INCLUDEDIR was not specified; using default BOOST_INCLUDEDIR=/usr/local/include" )
  set(BOOST_INCLUDEDIR /usr/local/include)
endif()

if(CMAKE_C_COMPILER)
  message( STATUS "CMAKE_C_COMPILER was not specified; using default CMAKE_C_COMPILER=/usr/bin/gcc" )
  set(CMAKE_C_COMPILER /usr/bin/gcc)
endif()

if(CMAKE_CXX_COMPILER)
  message( STATUS "CMAKE_CXX_COMPILER was not specified; using default CMAKE_CXX_COMPILER=/usr/bin/g++" )
  set(CMAKE_CXX_COMPILER /usr/bin/g++)
endif()

if(NOT BOOST_LIBRARYDIR)
  message( STATUS "BOOST_LIBRARYDIR was not specified; using default BOOST_LIBRARYDIR=/usr/local/lib" )
  set(BOOST_LIBRARYDIR /usr/local/lib)
endif()

if(NOT BUILD_DEBUG)
    message( STATUS "Building in default optimized configuration" )
    if(NOT GCC_FLAGS)
        message( STATUS "GCC_FLAGS was not specified; using default" )
        set(GCC_FLAGS -O3 -funroll-loops -fno-omit-frame-pointer -g3 -Wall -Werror -fmessage-length=160 -x c++)
    endif()
else()
    message( STATUS "Building in slow debugging configuration (ASAN enabled)" )
    set(GCC_FLAGS -O0 -g3 -fsanitize=address -Wall -Werror -fmessage-length=160 -x c++) 
    set(GCC_LDFLAGS -fsanitize=address)
endif()

add_definitions(
  -DTARGET_PLATFORM=${TARGET_PLATFORM}
  -DSELECTED_DEBUG=${SELECTED_DEBUG}
  -DSIMULATOR=${SIMULATOR}
  -DBOOST_MPL_LIMIT_VECTOR_SIZE=50
  -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
  -DCONFIG_QEMU
)

cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

if(NOT (CMAKE_MAJOR_VERSION LESS 2))
    if(POLICY CMP0048)
        #policy for VERSION in cmake 3.0
        cmake_policy(SET CMP0048 OLD)
    endif()
endif()

# -----------------------------------------------------------------------------
# Make RelWithDebInfo the default build type if otherwise not set
# -----------------------------------------------------------------------------
IF(DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
  CACHE STRING "Choose the type of build: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
  "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

PROJECT(sha1-sat)

if (CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        "-std=c++0x  -pedantic -Wall -Wextra -Wunused -Wsign-compare -Wtype-limits -Wuninitialized -Wno-deprecated -g -fno-omit-frame-pointer -O4 -mtune=native")
    SET(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wunused -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Wno-deprecated -O0 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "-std=c++0x -Wall -Wno-deprecated -O2 -g -DNDEBUG -mtune=native") #-fprofile-generate / -fprofile-use
endif(CMAKE_COMPILER_IS_GNUCC)

find_package( Boost 1.46 REQUIRED COMPONENTS program_options)

add_executable(sha1-gen
    main.cc
)

target_link_libraries(sha1-gen
    ${Boost_LIBRARIES}
)

add_executable(sha1-verify
    verify-preimage.cc
)


cmake_minimum_required(VERSION 2.6)

PROJECT(bcmswconfig C)

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

ADD_DEFINITIONS(-Wall -Werror -std=gnu99 -g3)

find_package(LIBUCI REQUIRED)
find_package(LIBCEXT REQUIRED)

INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_STRLCPY)

INCLUDE_DIRECTORIES(${LIBUCI_INCLUDE_DIRS} ${LIBCEXT_INCLUDE_DIR})

SET(SOURCES src/bcmswconfig.c)

ADD_EXECUTABLE(bcmswconfig ${SOURCES})
TARGET_LINK_LIBRARIES(bcmswconfig ${LIBUCI_LIBRARIES} ${LIBCEXT_LIBRARIES})

INSTALL(TARGETS bcmswconfig RUNTIME DESTINATION bin)


SET(SOURCES src/bcmvopiconfig.c)

ADD_EXECUTABLE(bcmvopiconfig ${SOURCES})
TARGET_LINK_LIBRARIES(bcmvopiconfig ${LIBUCI_LIBRARIES})

INSTALL(TARGETS bcmvopiconfig RUNTIME DESTINATION bin)


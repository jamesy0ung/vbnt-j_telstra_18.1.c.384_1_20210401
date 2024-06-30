# The source code form of this Open Source Project components is
# subject to the terms of the Clear BSD license.
# You can redistribute it and/or modify it under the terms of the
# Clear BSD License (http://directory.fsf.org/wiki/License:ClearBSD)
# See COPYING file/LICENSE file for more details.
# This software project does also include third party Open Source
# Software: See COPYING file/LICENSE file for more details.

cmake_minimum_required(VERSION 2.8)
project(preload-tch C)

add_definitions(-std=gnu99 -D_GNU_SOURCE -Wall -Werror)
set(PRELOAD_SRC
  preload.c mwan.c qos.c
)

add_library(preload-tch SHARED ${PRELOAD_SRC})
target_link_libraries(preload-tch ${CMAKE_DL_LIBS})

install(TARGETS preload-tch LIBRARY DESTINATION lib)

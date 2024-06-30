#
# Copyright (c) 2017 Technicolor
# All Rights Reserved
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

cmake_minimum_required(VERSION 2.8)
project(load_balancer C)

add_definitions(-std=gnu99 -D_GNU_SOURCE -Wall -Werror)
add_executable(load_balancer main.c)
target_link_libraries(load_balancer netfilter_queue ubus ubox uci pthread blobmsg_json)

set(CMAKE_INSTALL_PREFIX /usr)
install(TARGETS load_balancer RUNTIME DESTINATION bin)

cmake_minimum_required(VERSION 2.6)

PROJECT(urlfilterd-tch C)
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
OPTION(PCAP_DEBUG "PCAP debugging" OFF)

ADD_DEFINITIONS(-O2 -Wall -Werror -Wno-unused --std=gnu99 -Wmissing-declarations -D_GNU_SOURCE)

find_package(Threads REQUIRED)
find_package(UBUS REQUIRED)
find_package(UCI REQUIRED)
find_package(URCU REQUIRED)
find_package(NFQUEUE REQUIRED)

SET(SOURCES
	src/main.c
	src/config.c
	src/ubus.c
	src/nfq.c )
SET(HDRS
	include/urlfilterd.h
	include/nfq.h
	include/config.h
	include/ubus.h )


INCLUDE_DIRECTORIES(include ${UBUS_INCLUDE_DIRS} ${UCI_INCLUDE_DIRS} ${URCU_INCLUDE_DIRS} ${NFQUEUE_INCLUDE_DIRS})

ADD_DEFINITIONS(${URCU_DEFINITIONS})

IF(PCAP_DEBUG)
  find_package(PCAP)
  ADD_DEFINITIONS(-DPCAP_DEBUG=1)
  INCLUDE_DIRECTORIES(${PCAP_INCLUDE_DIRS})
  SET(EXTRA_LIBS ${PCAP_LIBRARIES})
ENDIF(PCAP_DEBUG)

ADD_EXECUTABLE(urlfilterd ${SOURCES} ${HDRS})

target_link_libraries(urlfilterd
	${UBUS_LIBRARIES} ${UCI_LIBRARIES} ${URCU_LIBRARIES} ${NFQUEUE_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${EXTRA_LIBS})

INSTALL(TARGETS urlfilterd
	RUNTIME DESTINATION sbin
)

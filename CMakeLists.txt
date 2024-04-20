PROJECT(tun2socks)

CMAKE_MINIMUM_REQUIRED(VERSION 3.16)

# ADD_DEFINITIONS(-DSIMPLE_MESSAGE)
ADD_DEFINITIONS(-DDETAIL_MESSAGE)
# ADD_DEFINITIONS(-DZLOG_MESSAGE)
ADD_DEFINITIONS(-w)

# 1. 是否显示tun读取的数据
ADD_DEFINITIONS(-D__HEX_DUMP__)
# 2. 是否设置测试IP和网关
ADD_DEFINITIONS(-D__TEST_TUN2SOCKS__)
# 3. 是否显示LWIP接收的内容
ADD_DEFINITIONS(-D__SHOW_LWIP_RECV_MSG__)


SET(CMAKE_C_FLAGS_DEBUG "-O0 -Wall -g -ggdb")
SET(CMAKE_C_FLAGS_RELEASE "-O3 -Wall")

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")

SET(CMAKE_BUILD_TYPE Debug)
SET(EXECUTABLE_OUTPUT_PATH bin)
SET(LIBRARY_OUTPUT_PATH lib)

INCLUDE_DIRECTORIES(base/)
INCLUDE_DIRECTORIES(pkg/include)
INCLUDE_DIRECTORIES(lwip/include)

# LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/pkg/lib)


###############################################################################
#############################   library   #####################################
###############################################################################

AUX_SOURCE_DIRECTORY(base BASE)
ADD_LIBRARY(base ${BASE})

SET(LWIPCORE_SRCS lwip/init.c lwip/def.c lwip/dns.c lwip/inet_chksum.c lwip/ip.c lwip/mem.c lwip/memp.c lwip/netif.c lwip/pbuf.c lwip/raw.c lwip/stats.c lwip/sys.c lwip/altcp.c lwip/altcp_alloc.c lwip/altcp_tcp.c lwip/tcp.c lwip/tcp_in.c lwip/tcp_out.c lwip/timeouts.c lwip/udp.c lwip/arch/sys_arch.c)

SET(LWIPCORE4_SRCS lwip/ipv4/autoip.c lwip/ipv4/dhcp.c lwip/ipv4/etharp.c lwip/ipv4/icmp.c lwip/ipv4/igmp.c lwip/ipv4/ip4_frag.c lwip/ipv4/ip4.c lwip/ipv4/ip4_addr.c)

SET(LWIPCORE6_SRCS lwip/ipv6/dhcp6.c lwip/ipv6/ethip6.c lwip/ipv6/icmp6.c lwip/ipv6/inet6.c lwip/ipv6/ip6.c lwip/ipv6/ip6_addr.c lwip/ipv6/ip6_frag.c lwip/ipv6/mld6.c lwip/ipv6/nd6.c)

SET(LWIPNOAPPS_SRCS ${LWIPCORE_SRCS} ${LWIPCORE4_SRCS} ${LWIPCORE6_SRCS})

add_library(lwipcore ${LWIPNOAPPS_SRCS})




###############################################################################
###########################   application  ####################################
###############################################################################

SET(T2S app/tun2sokcs/main.cpp app/tun2sokcs/tun2socks.cpp app/tun2sokcs/lwip_stack.cpp app/tun2sokcs/lwip_proxy.cpp app/tun2sokcs/lwip_echo.cpp app/tun2sokcs/lwip_timer.cpp app/tun2sokcs/remote_tcp_server.cpp app/tun2sokcs/conn_session.cpp app/tun2sokcs/pipe2socks.cpp)
ADD_EXECUTABLE(tun2socks ${T2S})
TARGET_LINK_LIBRARIES(tun2socks base uv zlog lwipcore)

SET(SNIFFER app/sniffer/main.cpp app/sniffer/sniffer.cpp)
ADD_EXECUTABLE(sniffer ${SNIFFER})
TARGET_LINK_LIBRARIES(sniffer zlog pcap base)

SET(XTUN app/xtun/main.cpp)
ADD_EXECUTABLE(xtun ${XTUN})
TARGET_LINK_LIBRARIES(xtun base)

SET(ASYNC app/uv/async.cpp)
ADD_EXECUTABLE(async ${ASYNC})
TARGET_LINK_LIBRARIES(async uv)

SET(IOWATCHER app/uv/iowatcher.cpp)
ADD_EXECUTABLE(iowatcher ${IOWATCHER})
TARGET_LINK_LIBRARIES(iowatcher uv)










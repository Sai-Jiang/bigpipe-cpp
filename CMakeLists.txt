cmake_minimum_required(VERSION 3.12)
project(bigpipe_cpp)
set(CMAKE_CXX_STANDARD 14)

set(Folly_Dir /usr/local/Cellar/folly/2018.09.10.01)
set(Glog_Dir /usr/local/Cellar/glog/0.3.5_3)
set(Gflags_Dir /usr/local/Cellar/gflags/2.2.1)

include_directories(${Folly_Dir}/include ${Glog_Dir}/include ${Gflags_Dir}/include)
link_directories(${Folly_Dir}/lib ${Glog_Dir}/lib ${Gflags_Dir}/lib)
link_libraries(folly glog gflags)

add_executable(bigpipe_cpp main.cpp client/rate_limit.h client/iclient.hpp proto/reqmsg.hpp config/config.hpp acceptor/acceptor.cpp acceptor/acceptor.hpp kafka/producer.hpp proto/reqbody.hpp proto/acl.hpp proto/callresp.hpp client/async_client.hpp client/circuit_breaker.hpp client/healthStats.hpp kafka/consumer.hpp base/BlockingQueue.hpp base/BoundedBlockingQueue.hpp base/SignalHandler.hpp)
target_link_libraries(bigpipe_cpp folly glog gflags served boost_system)
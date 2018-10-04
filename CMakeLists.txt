cmake_minimum_required(VERSION 3.12)
project(bigpipe_cpp)
set(CMAKE_CXX_STANDARD 14)

set(Folly_Dir /usr/local/Cellar/folly/2018.09.10.01)
set(Glog_Dir /usr/local/Cellar/glog/0.3.5_3)
set(Gflags_Dir /usr/local/Cellar/gflags/2.2.1)

include_directories(${Folly_Dir}/include ${Glog_Dir}/include ${Gflags_Dir}/include)
link_directories(${Folly_Dir}/lib ${Glog_Dir}/lib ${Gflags_Dir}/lib)
link_libraries(folly glog gflags)

add_executable(bigpipe_cpp main.cpp client/rate_limit.h client/client.h proto/reqmsg.hpp config/config.h server/server.cpp server/server.hpp kafka/producer.hpp kafka/producer.cpp kafka/consumer.h kafka/consumer.c server/acceptor.hpp proto/reqbody.hpp proto/acl.hpp proto/callstatus.hpp proto/callresponse.hpp)
target_link_libraries(bigpipe_cpp folly glog gflags served boost_system)
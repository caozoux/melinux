cmake_minimum_required(VERSION 3.16)
project(MyGrpcClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED CONFIG)
find_package(gRPC REQUIRED CONFIG)

set(SOURCES
    client.cc
    greeter.pb.cc
    greeter.grpc.pb.cc
)

add_executable(my_client ${SOURCES})

target_link_libraries(my_client PRIVATE
  gRPC::grpc++
  gRPC::grpc
  protobuf::libprotobuf
)

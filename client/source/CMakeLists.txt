cmake_minimum_required(VERSION 3.9)
project(WADGNSS_Client)

set(CMAKE_CXX_STANDARD 14)

add_executable(WADGNSS_Client client.cpp client.h ubxcfg.h)

# Copyright (c) 2021 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.15)

add_library(ApiInterface INTERFACE)

target_sources(ApiInterface INTERFACE
        include/Api/EncodedEvent.h
        include/Api/LockFreeApiEventProducer.h
        include/Api/Orbit.h)

target_include_directories(ApiInterface INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include)

add_executable(ApiInterfaceTests)

target_compile_options(ApiInterfaceTests PRIVATE ${STRICT_COMPILE_FLAGS})

target_sources(ApiInterfaceTests PRIVATE
        EncodedEventTest.cpp)

target_link_libraries(ApiInterfaceTests PRIVATE
        ApiInterface
        GTest::Main)

register_test(ApiInterfaceTests)

if(NOT WIN32)
project(Api)

add_library(Api SHARED)

set_target_properties(Api PROPERTIES OUTPUT_NAME "orbit")

target_compile_options(Api PRIVATE ${STRICT_COMPILE_FLAGS})

target_compile_features(Api PUBLIC cxx_std_17)

target_sources(Api PRIVATE
        Orbit.cpp)

target_link_libraries(Api PUBLIC
        ApiInterface
        CaptureEventProducer
        GrpcProtos
        OrbitBase
        ProducerSideChannel)

strip_symbols(Api)

endif()

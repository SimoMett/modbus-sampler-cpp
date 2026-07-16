cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O3")

IF (WIN32)
set(CMAKE_CXX_FLAGS "/utf-8")
ENDIF()

project(modbus_csv_sampler_daemon)

include_directories(src/ include/)

set(SOURCE_CPP src/workers/ConsumerWorker.cpp src/workers/ModbusWorker.cpp src/workers/CsvWorker.cpp include/simomett/common.cpp)

add_executable(modbus_csv_sampler_daemon src/modbus_csv_sampler_daemon.cpp ${SOURCE_CPP})

target_compile_features(modbus_csv_sampler_daemon PRIVATE cxx_std_20)
target_link_libraries(modbus_csv_sampler_daemon "~/modbus-sampler-cpp/libmodbus.so.0.5.0")
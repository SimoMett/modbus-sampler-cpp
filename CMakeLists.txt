cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_FLAGS "/utf-8")

project(modbus_csv_sampler_daemon)

include_directories(src/ include/)

set(SOURCE_CPP src/workers/ConsumerWorker.cpp src/workers/ModbusWorker.cpp src/workers/CsvWorker.cpp)

add_executable(modbus_csv_sampler_daemon main.cpp ${SOURCE_CPP})

target_compile_features(modbus_csv_sampler_daemon PRIVATE cxx_std_17)
target_link_libraries(modbus_csv_sampler_daemon ../modbus)
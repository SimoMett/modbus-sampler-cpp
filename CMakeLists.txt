cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O3")

IF (WIN32)
set(CMAKE_CXX_FLAGS "/utf-8")
ENDIF()

project(modbus_samplers)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(SDL2 REQUIRED)

include_directories(src/ include/ include/imgui/ ${SDL2_INCLUDE_DIRS})

set(COMMON_SOURCE_CPP src/workers/ConsumerWorker.cpp src/workers/ModbusWorker.cpp include/simomett/common.cpp)

add_executable(modbus_csv_sampler_daemon src/modbus_csv_sampler_daemon.cpp ${SOURCE_CPP})


add_executable(modbus_csv_sampler_daemon src/modbus_csv_sampler_daemon.cpp ${COMMON_SOURCE_CPP} src/workers/CsvWorker.cpp)
target_compile_features(modbus_csv_sampler_daemon PRIVATE cxx_std_20)
target_link_libraries(modbus_csv_sampler_daemon ${MODBUS_LIB})

set(IMGUI_SOURCE_CPP include/imgui/imgui.cpp include/imgui/imgui_tables.cpp include/imgui/imgui_widgets.cpp include/imgui/imgui_draw.cpp include/implot/implot.cpp include/implot/implot_items.cpp include/imgui/backends/imgui_impl_sdl2.cpp include/imgui/backends/imgui_impl_opengl2.cpp)
add_executable(modbus_graph_plotter src/modbus_graph_plotter.cpp ${COMMON_SOURCE_CPP} src/workers/GuiWorker.cpp ${IMGUI_SOURCE_CPP} src/workers/SamplesRingQueue.cpp)
target_compile_features(modbus_graph_plotter PRIVATE cxx_std_20)
target_link_libraries(modbus_graph_plotter ${MODBUS_LIB} GL ${SDL2_LIBRARIES})

add_executable(modbus_pg_sampler_daemon src/modbus_pg_sampler_daemon.cpp ${COMMON_SOURCE_CPP} src/workers/PostgreWorker.cpp)
target_compile_features(modbus_pg_sampler_daemon PRIVATE cxx_std_20)
target_link_libraries(modbus_pg_sampler_daemon ${MODBUS_LIB} pqxx)

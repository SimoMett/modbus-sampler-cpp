cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

project(modbus_samplers)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(SDL2 REQUIRED)

include_directories(src/ include/ include/imgui/)

if(WIN32)
set(MODBUS_LIB "${CMAKE_CURRENT_SOURCE_DIR}/libmodbus.dll")
else()
set(MODBUS_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libmodbus.so")
endif()

set(COMMON_SOURCE_CPP
    src/workers/ConsumerWorker.cpp
    src/workers/ModbusWorker.cpp
    include/simomett/common.cpp
)

set(IMGUI_SOURCE_CPP
    include/imgui/imgui.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_draw.cpp
    include/implot/implot.cpp
    include/implot/implot_items.cpp
    include/imgui/backends/imgui_impl_sdl2.cpp
    include/imgui/backends/imgui_impl_opengl2.cpp
)

add_executable(modbus_csv_sampler_daemon
    src/modbus_csv_sampler_daemon.cpp
    ${COMMON_SOURCE_CPP}
    src/workers/CsvWorker.cpp
)
target_compile_features(modbus_csv_sampler_daemon PRIVATE cxx_std_20)
target_link_options(modbus_csv_sampler_daemon PRIVATE -static-libgcc -static-libstdc++)
if(WIN32)
    target_link_libraries(modbus_csv_sampler_daemon ${MODBUS_LIB} "libwinpthread.a")
else()
    target_link_libraries(modbus_csv_sampler_daemon ${MODBUS_LIB})
endif()

add_executable(modbus_graph_plotter
    src/modbus_graph_plotter.cpp
    ${COMMON_SOURCE_CPP}
    src/workers/GuiWorker.cpp
    ${IMGUI_SOURCE_CPP}
    src/workers/SamplesRingQueue.cpp
)
target_compile_features(modbus_graph_plotter PRIVATE cxx_std_20)
target_link_options(modbus_graph_plotter PRIVATE -static-libgcc -static-libstdc++)
if(WIN32)
    target_link_libraries(modbus_graph_plotter ${MODBUS_LIB} opengl32 ${SDL2_LIBRARIES} "libwinpthread.a")
else()
    target_link_libraries(modbus_graph_plotter ${MODBUS_LIB} GL ${SDL2_LIBRARIES})
endif()

add_executable(modbus_pg_sampler_daemon
    src/modbus_pg_sampler_daemon.cpp
    ${COMMON_SOURCE_CPP}
    src/workers/PostgreWorker.cpp
)
target_compile_features(modbus_pg_sampler_daemon PRIVATE cxx_std_20)
target_link_options(modbus_pg_sampler_daemon PRIVATE -static-libgcc -static-libstdc++)
if(WIN32)
    target_link_libraries(modbus_pg_sampler_daemon ${MODBUS_LIB} pqxx "libwinpthread.a")
else()
    target_link_libraries(modbus_pg_sampler_daemon ${MODBUS_LIB} pqxx)
endif()
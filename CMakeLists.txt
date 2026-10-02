cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

project(modbus_samplers)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s")

find_package(SDL2 REQUIRED)

include_directories(src/ include/ include/imgui/)

if(WIN32)
    link_libraries(winpthread)
endif()

include(FetchContent)
FetchContent_Declare(modbuslib
    GIT_REPOSITORY https://github.com/serhmarch/ModbusLib.git
    GIT_TAG v0.5.0
)

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
    src/modbus_sampler_daemon.cpp
    ${COMMON_SOURCE_CPP}
    src/workers/CsvWorker.cpp
)
target_compile_features(modbus_csv_sampler_daemon PRIVATE cxx_std_20)
target_compile_definitions(modbus_csv_sampler_daemon PRIVATE CSV_WORKER_ENABLED)
target_link_options(modbus_csv_sampler_daemon PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(modbus_csv_sampler_daemon modbus)

add_executable(modbus_graph_plotter
    src/modbus_sampler_daemon.cpp
    ${COMMON_SOURCE_CPP}
    src/workers/ImGuiWorker.cpp
    ${IMGUI_SOURCE_CPP}
    src/workers/SamplesRingQueue.cpp
)
target_compile_features(modbus_graph_plotter PRIVATE cxx_std_20)
target_compile_definitions(modbus_graph_plotter PRIVATE IMGUI_WORKER_ENABLED)
target_link_options(modbus_graph_plotter PRIVATE -static-libgcc -static-libstdc++)
if(WIN32)
    target_link_libraries(modbus_graph_plotter modbus opengl32 ${SDL2_LIBRARIES})
else()
    target_link_libraries(modbus_graph_plotter modbus GL ${SDL2_LIBRARIES})
endif()

add_executable(modbus_pg_sampler_daemon
    src/modbus_sampler_daemon.cpp
    ${COMMON_SOURCE_CPP}
    src/workers/PostgreWorker.cpp
)
target_compile_features(modbus_pg_sampler_daemon PRIVATE cxx_std_20)
target_compile_definitions(modbus_pg_sampler_daemon PRIVATE PG_WORKER_ENABLED)
target_link_options(modbus_pg_sampler_daemon PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(modbus_pg_sampler_daemon modbus pqxx)

####
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)

FetchContent_Declare(lunasvg
    GIT_REPOSITORY https://github.com/sammycage/lunasvg.git
    GIT_TAG v3.5.0
)

FetchContent_Declare(pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.16
)

FetchContent_MakeAvailable(modbuslib SFML lunasvg pugixml)

add_executable(modbus_plant_ui
    src/modbus_sampler_daemon.cpp
    ${COMMON_SOURCE_CPP}
    src/workers/SFMLWorker.cpp
)
target_compile_features(modbus_plant_ui PRIVATE cxx_std_20)
target_compile_definitions(modbus_plant_ui PRIVATE SFML_WORKER_ENABLED)
target_link_options(modbus_plant_ui PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(modbus_plant_ui modbus pugixml plutovg lunasvg SFML::Graphics)

add_executable(modbus_alarm_logger
    src/modbus_sampler_daemon.cpp
    ${COMMON_SOURCE_CPP}
    src/workers/AlarmLogger.cpp
)
target_compile_features(modbus_alarm_logger PRIVATE cxx_std_20)
target_compile_definitions(modbus_alarm_logger PRIVATE ALARM_LOGGER_ENABLED)
target_link_options(modbus_alarm_logger PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(modbus_alarm_logger modbus)
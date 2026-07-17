#include <iostream>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif

#include "imgui/imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl2.h"

#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "workers/GuiWorker.h"
#include "simomett/common.h"
#include "workers/ModbusWorker.h"

const std::string program_name = "Modbus graph plotter";
const std::string program_version = "0.1";

// Main code
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char ** argv)
#endif
{
    // Setup SDL
#ifdef _WIN32
    ::SetProcessDPIAware();
#endif
    
    argparse::ArgumentParser parser(program_name, program_version);
    parser.add_argument("--config").default_value("modbus_sampler_daemon_config.json").help("use different configuration file");
    parser.add_argument("--log").default_value("modbus_sampler_daemon.log").help("put logs in a different file");
    parser.add_argument("tags_json").required();

    bool failed_to_parse_args = false;
    try
    {
        #ifdef _WIN32
        parser.parse_args(__argc, __argv);
        #else
        parser.parse_args(argc, argv);
        #endif
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << '\n';
        failed_to_parse_args = true;
    }

    if (failed_to_parse_args)
    {
        std::cout << parser;
        return 0;
    }

    // logger setup
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    std::ostringstream pattern;
    pattern << "[" << program_name << "] [%^%l%$] %v";
    console_sink->set_pattern(pattern.str());

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(parser.get<std::string>("--log"), true);
    file_sink->set_level(spdlog::level::trace);

    std::shared_ptr<spdlog::logger> logger(new spdlog::logger(program_name, {file_sink, console_sink}));
    logger->set_level(spdlog::level::info);

    try
    {
        logger->info("Initializing..");
        json config_json = simomett::json_from_file(parser.get<std::string>("--config"));
        json tags_json = simomett::json_from_file(parser.get<std::string>("tags_json"));

        // workers setup
        std::vector<std::shared_ptr<ConsumerWorker>> consumerWorkers;
        consumerWorkers.push_back(std::make_shared<GuiWorker>(logger, program_name, config_json["gui"], tags_json));
        
        for (auto worker : consumerWorkers)
            worker->start();

        Modbus::TcpSettings modbus_settings;
        modbus_settings.host = config_json["modbus_connection"]["host"].get<std::string>().c_str();
        modbus_settings.port = config_json["modbus_connection"]["port"].get<uint16_t>();
        modbus_settings.timeout = 3000;

        ModbusWorker modbus_worker(logger, &modbus_settings, tags_json, consumerWorkers);
        modbus_worker.start();

        modbus_worker.join();

        //Termination
        for (auto worker : consumerWorkers)
            worker->stop();

        for (auto worker : consumerWorkers)
            worker->join();

        consumerWorkers.clear();
        logger->info("Exiting");
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << "\n";
        logger->info(e.what());
    }

    return 0;
}

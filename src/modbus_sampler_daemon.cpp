#include <sstream>
#include <csignal>
#include "argparse/argparse.hpp"
#include "modbus/ModbusGlobal.h"
#include "nlohmann/json.hpp"
#include "workers/ModbusWorker.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "simomett/common.h"

#ifdef CSV_WORKER_ENABLED
#include "workers/CsvWorker.h"
#endif
#ifdef PG_WORKER_ENABLED
#include "workers/PostgreWorker.h"
#endif
#ifdef IMGUI_WORKER_ENABLED
#include "workers/ImGuiWorker.h"
#endif
#ifdef SFML_WORKER_ENABLED
#include "workers/SFMLWorker.h"
#endif
#ifdef ALARM_LOGGER_ENABLED
#include "workers/AlarmLogger.h"
#endif

const std::string program_name = "ModbusSamplerDaemon";

std::function<void(int)> shutdown_handler;
void signal_handler(int signal)
{
    shutdown_handler(signal);
}

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
    argparse::ArgumentParser parser(program_name, "0.0a");
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

        #ifdef CSV_WORKER_ENABLED
        if(config_json["csv"].is_null())
            throw std::runtime_error("Missing 'csv' field in config json");
        consumerWorkers.push_back(std::make_shared<CsvWorker>(logger, config_json["csv"]["output_directory"].get<std::string>(), config_json["csv"]["storing_interval"].get<float>(), tags_json));
        #endif

        #ifdef PG_WORKER_ENABLED
        if(config_json["postgres"].is_null())
            throw std::runtime_error("Missing 'postgres' field in config json");
        PostgreWorkerConfig pg_config = {
            config_json["postgres"]["host"].get<std::string>(),
            config_json["postgres"]["port"].get<uint16_t>(),
            config_json["postgres"]["dbname"].get<std::string>(),
            config_json["postgres"]["user"].get<std::string>(),
            config_json["postgres"]["password"].get<std::string>()
        };
        consumerWorkers.push_back(std::make_shared<PostgreWorker>(logger, pg_config, config_json["postgres"]["storing_interval"].get<float>(), tags_json));
        #endif

        #ifdef IMGUI_WORKER_ENABLED
        if(config_json["imgui"].is_null())
            throw std::runtime_error("Missing 'imgui' field in config json");
        consumerWorkers.push_back(std::make_shared<ImGuiWorker>(logger, program_name, config_json["imgui"], tags_json));
        #endif

        #ifdef SFML_WORKER_ENABLED
        if(config_json["sfmlgui"].is_null())
            throw std::runtime_error("Missing 'sfmlgui' field in config json");
        consumerWorkers.push_back(std::make_shared<SFMLWorker>(logger, program_name, config_json["sfmlgui"], tags_json));
        #endif

        #ifdef ALARM_LOGGER_ENABLED
        if(config_json["alarmlogger"].is_null())
            throw std::runtime_error("Missing 'alarmlogger' field in config json");
        consumerWorkers.push_back(std::make_shared<AlarmLogger>(logger, config_json["alarmlogger"], tags_json));
        #endif

        // connections workers setup
        if(config_json["modbus_connection"].is_null())
            throw std::runtime_error("Missing 'modbus_connection' field in config json");

        Modbus::NetSettings modbus_settings;
        modbus_settings.host = config_json["modbus_connection"]["host"].get<std::string>().c_str();
        modbus_settings.port = config_json["modbus_connection"]["port"].get<uint16_t>();
        modbus_settings.timeout = 3000;

        ModbusWorker modbus_worker(logger, &modbus_settings, tags_json, consumerWorkers);

        // Start everything - consumers first
        for (auto worker : consumerWorkers)
            worker->start();

        modbus_worker.start();

        // CTRL+C
        std::signal(SIGINT, signal_handler);
        shutdown_handler = [&](int signal)
        {
            logger->info("CTR+C received");
            modbus_worker.stop();
        };
        //

        modbus_worker.join();

        //Termination

        // Uninstall signal handler
        std::signal(SIGINT, SIG_DFL);

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
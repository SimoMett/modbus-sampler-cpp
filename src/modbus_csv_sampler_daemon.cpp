#include <fstream>
#include <sstream>
#include <csignal>
#include "argparse/argparse.hpp"
#include "nlohmann/json.hpp"
#include "modbus/ModbusClient.h"
#include "workers/ModbusWorker.h"
#include "workers/CsvWorker.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "simomett/common.h"

const std::string program_name = "ModbusSamplerDaemon";
const std::string program_version = "0.1";

std::function<void(int)> shutdown_handler;
void signal_handler(int signal)
{
    shutdown_handler(signal);
}

int main(int argc, char **argv)
{
    argparse::ArgumentParser parser(program_name, program_version);
    parser.add_argument("--config").default_value("modbus_sampler_daemon_config.json").help("use different configuration file");
    parser.add_argument("--log").default_value("modbus_sampler_daemon.log").help("put logs in a different file");
    parser.add_argument("tags_json").required();

    bool failed_to_parse_args = false;
    try
    {
        parser.parse_args(argc, argv);
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

    std::shared_ptr<spdlog::logger> logger(new spdlog::logger("multi_sink", {file_sink, console_sink}));
    logger->set_level(spdlog::level::info);

    try
    {
        logger->info("Initializing..");
        json config_json = simomett::json_from_file(parser.get<std::string>("--config"));
        json tags_json = simomett::json_from_file(parser.get<std::string>("tags_json"));

        // workers setup
        std::vector<std::shared_ptr<ConsumerWorker>> consumerWorkers;
        consumerWorkers.push_back(std::make_shared<CsvWorker>(std::shared_ptr<spdlog::logger>(logger), config_json["csv"]["output_directory"].get<std::string>(), config_json["csv"]["storing_interval"].get<float>(), tags_json));
        
        for (auto worker : consumerWorkers)
            worker->start();

        Modbus::TcpSettings modbus_settings;
        modbus_settings.host = config_json["modbus_connection"]["host"].get<std::string>().c_str();
        modbus_settings.port = config_json["modbus_connection"]["port"].get<uint16_t>();
        modbus_settings.timeout = 3000;

        ModbusWorker modbus_worker(logger, &modbus_settings, tags_json, consumerWorkers);
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
        std::cout << "exc\n";
        logger->info(e.what());
    }
    return 0;
}
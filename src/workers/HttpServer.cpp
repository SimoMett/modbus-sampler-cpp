#include "HttpServer.h"

const std::string HttpServer::WORKER_VERSION = "Http server build: 1";

HttpServer::HttpServer(std::shared_ptr<spdlog::logger> logger, json http_server_conf, json tags): logger(logger), http_port(8380)
{
    http_server.Get("/", [](const httplib::Request &, httplib::Response &res) {
    res.set_content("Hello World!", "text/plain");
    });
}

void HttpServer::start()
{
    run_thread = std::make_unique<std::thread>(&HttpServer::run, this);
}

void HttpServer::join()
{
    this->run_thread->join();
}

void HttpServer::stop()
{
    http_server.stop();
}

bool HttpServer::running()
{
    return this->is_running;
}

void HttpServer::push_words(std::vector<AddressValue<uint16_t>>, std::chrono::system_clock::time_point)
{
}

void HttpServer::push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point)
{
}

void HttpServer::push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point)
{
}

void HttpServer::push_coils(std::vector<AddressValue<bool>>, std::chrono::system_clock::time_point)
{
}

void HttpServer::push_bits(std::vector<BitAddressValue>, std::chrono::system_clock::time_point)
{
}

void HttpServer::run()
{
    is_running = true;    
    try
    {        
        // init here
        this->logger->info(std::format("HttpServer listening on port http://127.0.0.1:{}", http_port));
        http_server.listen("127.0.0.1", http_port);

        /*while (!this->should_close)
        {
            // cylic stuff here
        }*/

        this->logger->info("HttpServer stopped");
    }
    catch(std::exception & e)
    {
        this->logger->error(e.what());
        this->logger->error("HttpServer stopped due to error");
    }
    this->is_running = false;
}

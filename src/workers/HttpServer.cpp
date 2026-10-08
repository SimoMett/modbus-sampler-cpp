#include "HttpServer.h"

const std::string HttpServer::WORKER_VERSION = "Http server build: 1";

HttpServer::HttpServer(std::shared_ptr<spdlog::logger> logger, json http_server_conf, json tags): logger(logger), http_port(8380)
{
    http_server.Get("/", [this](const httplib::Request &req, httplib::Response &res) { this->http_return_status(req, res); });
    http_server.Get("/tag", [this](const httplib::Request &req, httplib::Response &res) { this->http_handle_tags_request(req, res); });
    http_server.Get("/address", [this](const httplib::Request &req, httplib::Response &res) { this->http_handle_addr_request(req, res); });
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
        this->logger->info(std::format("Http server listening on port http://127.0.0.1:{}", http_port));
        http_server.listen("127.0.0.1", http_port);

        /*while (!this->should_close)
        {
            // cylic stuff here
        }*/

        this->logger->info("Http server stopped");
    }
    catch(std::exception & e)
    {
        this->logger->error(e.what());
        this->logger->error("Http server stopped due to error");
    }
    this->is_running = false;
}

void HttpServer::http_return_status(const httplib::Request &, httplib::Response &res)
{
    res.set_content("Healthy!", "text/plain");
}

void HttpServer::http_handle_tags_request(const httplib::Request &req, httplib::Response &res)
{
    res.set_content("Tags", "text/plain");
    logger->info(std::format("Request by {}", req.remote_addr));
}

void HttpServer::http_handle_addr_request(const httplib::Request &, httplib::Response &res)
{
    res.set_content("Address", "text/plain");
}

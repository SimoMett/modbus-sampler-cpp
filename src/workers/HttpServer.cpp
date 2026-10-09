#include "HttpServer.h"
#include "simomett/common.h"

const std::string HttpServer::WORKER_VERSION = "Http server build: 1";

using simomett::MbValueType;

HttpServer::HttpServer(std::shared_ptr<spdlog::logger> logger, json http_server_conf, json tags): logger(logger), use_tls(false)
{
    for(const char * field : {"host", "port", "useTls"})
    {
        if(http_server_conf[field].is_null())
            throw std::runtime_error(std::format("Missing '{}' field in httpserver config", field));
    }
    http_host = http_server_conf["host"].get<std::string>();
    http_port = http_server_conf["port"].get<unsigned short>();

    http_server.Get("/", [this](const httplib::Request &req, httplib::Response &res) { this->http_return_status(req, res); });
    http_server.Get("/tags/:tagname", [this](const httplib::Request &req, httplib::Response &res) { this->http_handle_tags_request(req, res); });

    //Parse tag names
    std::unordered_map<std::string, addr_t> *names_maps[4];
    names_maps[MbValueType::WORD_TYPE] = &this->words_names;
    names_maps[MbValueType::DWORD_TYPE] = &this->dwords_names;
    names_maps[MbValueType::REAL_TYPE] = &this->floats_names;
    names_maps[MbValueType::COIL_TYPE] = &this->coils_names;

    std::string json_str[4];
    json_str[MbValueType::WORD_TYPE] = "words";
    json_str[MbValueType::DWORD_TYPE] = "dwords";
    json_str[MbValueType::REAL_TYPE] = "floats";
    json_str[MbValueType::COIL_TYPE] = "coils";

    for (auto v : {MbValueType::WORD_TYPE, MbValueType::DWORD_TYPE, MbValueType::REAL_TYPE, MbValueType::COIL_TYPE})
    {
        if (!tags[json_str[v]].is_null())
        {
            for (json s : tags[json_str[v]])
            {
                std::string formatted_tag_name = ConsumerWorker::format_name(s["tag"].get<std::string>());
                names_maps[v]->insert({formatted_tag_name, s["address"].get<addr_t>()});
            }
        }
    }

    for(auto e : this->words_names)
        this->words_values.insert(std::pair<addr_t, uint16_t>(e.second, 0));
    for(auto e : this->floats_names)
        this->float_values.insert(std::pair<addr_t, float>(e.second, 0));
    for(auto e : this->dwords_names)
        this->dwords_values.insert(std::pair<addr_t, uint32_t>(e.second, 0));
    for(auto e : this->coils_names)
        this->coils_values.insert(std::pair<addr_t, bool>(e.second, 0));
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

void HttpServer::push_words(std::vector<AddressValue<uint16_t>> samples, std::chrono::system_clock::time_point)
{
    for (auto &sample : samples)
        this->words_values[sample.address] = sample.val;
}

void HttpServer::push_floats(std::vector<AddressValue<float>> samples, std::chrono::system_clock::time_point)
{
    for (auto &sample : samples)
        this->float_values[sample.address] = sample.val;
}

void HttpServer::push_dwords(std::vector<AddressValue<uint32_t>> samples, std::chrono::system_clock::time_point)
{
    for (auto &sample : samples)
        this->dwords_values[sample.address] = sample.val;
}

void HttpServer::push_coils(std::vector<AddressValue<bool>> samples, std::chrono::system_clock::time_point)
{
    for (auto &sample : samples)
        this->coils_values[sample.address] = sample.val;
}

void HttpServer::push_bits(std::vector<BitAddressValue> samples, std::chrono::system_clock::time_point)
{
    //TODO
    static bool warn_issued = false;
    if(!warn_issued)
    {
        logger->warn("'HttpServer::push_bits' not implemented yet"); 
        warn_issued = true;
    }
}

void HttpServer::run()
{
    is_running = true;    
    try
    {
        this->logger->info(std::format("Http server listening on {}://{}:{}", use_tls? "https":"http", http_host, http_port));
        http_server.listen(http_host, http_port);

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
    std::string tag_name = req.path_params.at("tagname");
    logger->debug(std::format("Client {} requested tag '{}'", req.remote_addr, tag_name));

    json response = {
        {"tagname", tag_name},
        {"value", "invalid"}
    };

    unsigned int diag_tag_name_counts = 0;

    if(this->words_names.contains(tag_name))
    {
        addr_t addr = this->words_names[tag_name];
        response = {
            {"tagname", tag_name},
            {"value", this->words_values[addr]}
        };
        diag_tag_name_counts++;
    }

    if(this->floats_names.contains(tag_name))
    {
        addr_t addr = this->floats_names[tag_name];
        response = {
            {"tagname", tag_name},
            {"value", this->float_values[addr]}
        };
        diag_tag_name_counts++;
    }

    if(this->dwords_names.contains(tag_name))
    {
        addr_t addr = this->dwords_names[tag_name];
        response = {
            {"tagname", tag_name},
            {"value", this->dwords_values[addr]}
        };
        diag_tag_name_counts++;
    }

    if(this->coils_names.contains(tag_name))
    {
        addr_t addr = this->coils_names[tag_name];
        response = {
            {"tagname", tag_name},
            {"value", this->coils_values[addr]}
        };
        diag_tag_name_counts++;
    }
    
    res.set_content(response.dump(), "application/json");

    if(diag_tag_name_counts > 1)
        logger->warn(std::format("Tag '{}' appears twice in tags json", tag_name));
}

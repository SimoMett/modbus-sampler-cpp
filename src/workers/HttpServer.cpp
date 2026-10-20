#include "HttpServer.h"
#include "simomett/common.h"

const std::string HttpServer::WORKER_VERSION = "Http server build: 1";

using simomett::MbValueType;

HttpServer::HttpServer(std::shared_ptr<spdlog::logger> logger, json http_server_conf, json tags): logger(logger), tags(tags)
{
    for(const char * field : {"host", "port", "useTls"})
    {
        if(http_server_conf[field].is_null())
            throw std::runtime_error(std::format("Missing '{}' field in httpserver config", field));
    }
    use_tls = http_server_conf["useTls"].get<bool>();
    if(use_tls)
    {
        logger->warn("TLS not yet supported by this Http server");
        use_tls = false;
    }
    http_host = http_server_conf["host"].get<std::string>();
    http_port = http_server_conf["port"].get<unsigned short>();

    http_server.Get("/", [this](const httplib::Request &req, httplib::Response &res) { this->http_return_status(req, res); });
    http_server.Get("/tags/:tagname", [this](const httplib::Request &req, httplib::Response &res) { this->http_handle_tags_request(req, res); });

    for(auto e : this->tags.get_words_addresses())
        this->words_values.insert(std::pair<addr_t, uint16_t>(e, 0));
    for(auto e : this->tags.get_floats_addresses())
        this->float_values.insert(std::pair<addr_t, float>(e, 0));
    for(auto e : this->tags.get_dwords_addresses())
        this->dwords_values.insert(std::pair<addr_t, uint32_t>(e, 0));
    for(auto e : this->tags.get_coils_addresses())
        this->coils_values.insert(std::pair<addr_t, bool>(e, 0));
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
    for(auto &sample : samples)
    {
        this->bits_values[sample.address] &= 0xffff - (1 << sample.bit);
        this->bits_values[sample.address] |= (sample.val << sample.bit);
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
    static const json response = {
        {"status", "healthy"}
    };
    res.set_content(response.dump(), "application/json");
}

bool HttpServer::check_auth(const httplib::Request &req, httplib::Response &res)
{
    std::string client_creds = req.get_header_value("Authorization");

    if(client_creds.empty() /* && bla bla bla */)
    {
        res.status = httplib::Unauthorized_401;
        return false;
    }

    return false;
}

void HttpServer::http_handle_tags_request(const httplib::Request &req, httplib::Response &res)
{
    if(!check_auth(req, res))
        return;

    std::string tag_name = req.path_params.at("tagname");
    logger->debug(std::format("Client {} requested tag '{}'", req.remote_addr, tag_name));

    json response = {
        {"tagname", tag_name},
        {"value", "invalid"}
    };

    unsigned int diag_tag_name_counts = 0;

    if(std::find(this->tags.get_words_names().begin(), this->tags.get_words_names().end(), tag_name) != this->tags.get_words_names().end())
    {
        addr_t addr = this->tags.get_word_addr(tag_name);
        response = {
            {"tagname", tag_name},
            {"value", this->words_values[addr]}
        };
        diag_tag_name_counts++;
    }

    if(std::find(this->tags.get_floats_names().begin(), this->tags.get_floats_names().end(), tag_name) != this->tags.get_floats_names().end())
    {
        addr_t addr = this->tags.get_float_addr(tag_name);
        response = {
            {"tagname", tag_name},
            {"value", this->float_values[addr]}
        };
        diag_tag_name_counts++;
    }

    if(std::find(this->tags.get_dwords_names().begin(), this->tags.get_dwords_names().end(), tag_name) != this->tags.get_dwords_names().end())
    {
        addr_t addr = this->tags.get_dword_addr(tag_name);
        response = {
            {"tagname", tag_name},
            {"value", this->dwords_values[addr]}
        };
        diag_tag_name_counts++;
    }

    if(std::find(this->tags.get_coils_names().begin(), this->tags.get_coils_names().end(), tag_name) != this->tags.get_coils_names().end())
    {
        addr_t addr = this->tags.get_coil_addr(tag_name);
        response = {
            {"tagname", tag_name},
            {"value", this->coils_values[addr]}
        };
        diag_tag_name_counts++;
    }

    if(std::find(this->tags.get_bits_names().begin(), this->tags.get_bits_names().end(), tag_name) != this->tags.get_bits_names().end())
    {
        BitAddress addr = this->tags.get_bit_addr(tag_name);
        response = {
            {"tagname", tag_name},
            {"value", (int)((this->bits_values[addr.address] & (1 << addr.bit)) != 0)}
        };
        diag_tag_name_counts++;
    }
    
    res.set_content(response.dump(), "application/json");

    if(diag_tag_name_counts > 1)
        logger->warn(std::format("Tag '{}' appears twice in tags json", tag_name));
}

#include "cpp-httplib/httplib.h"
#include "ConsumerWorker.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

using nlohmann::json;

class HttpServer: public ConsumerWorker
{
public:
    static const std::string WORKER_VERSION;

    HttpServer(std::shared_ptr<spdlog::logger> logger, json http_server_conf, json tags);
    ~HttpServer(){};

    void start();
    void join();
    void stop();
    bool running();
    void push_words(std::vector<AddressValue<uint16_t>>, std::chrono::system_clock::time_point);
    void push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point);
    void push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point);
    void push_coils(std::vector<AddressValue<bool>>, std::chrono::system_clock::time_point);
    void push_bits(std::vector<BitAddressValue>, std::chrono::system_clock::time_point);

protected:
    const std::shared_ptr<spdlog::logger> logger;
    bool should_close;
    bool is_running;
    std::unique_ptr<std::thread> run_thread;

    void run();
    void dump_samples(){};

private:
    std::string http_host;
    unsigned short http_port;
    bool use_tls;
    httplib::Server http_server;

    std::unordered_map<std::string, addr_t> words_names;
    std::map<addr_t, uint16_t> words_values;

    std::unordered_map<std::string, addr_t> floats_names;
    std::map<addr_t, float> float_values;

    std::unordered_map<std::string, addr_t> dwords_names;
    std::map<addr_t, uint32_t> dwords_values;

    std::unordered_map<std::string, addr_t> coils_names;
    std::map<addr_t, bool> coils_values;

    std::unordered_map<std::string, BitAddress> bits_names;
    std::map<addr_t, uint16_t> bits_values;

    void http_return_status(const httplib::Request &, httplib::Response &res);
    void http_handle_tags_request(const httplib::Request &, httplib::Response &res);
};
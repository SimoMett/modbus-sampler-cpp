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
    unsigned short http_port;
    httplib::Server http_server;
};
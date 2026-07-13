#include <memory>
#include <thread>

#include "ConsumerWorker.h"
#include "spdlog/logger.h"
#include "nlohmann/json.hpp"
#include <pqxx/pqxx>

using nlohmann::json;

struct PostgreWorkerConfig
{
    std::string host;
    uint16_t port;
    std::string dbname;
    std::string user;
    std::string password;
};

class PostgreWorker: public ConsumerWorker
{
public:
    PostgreWorker(std::shared_ptr<spdlog::logger> logger, std::string host, uint16_t port, std::string dbname, std::string user, std::string password, float dump_time_s, json tags);
    PostgreWorker(std::shared_ptr<spdlog::logger> logger, PostgreWorkerConfig config, float dump_time_s, json tags);
    ~PostgreWorker();

    void start();
    void join();
    void stop();
    bool running();
    void push_words(std::vector<AddressValue<uint16_t>>, std::chrono::system_clock::time_point);
    void push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point);
    void push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point);
    void push_coils(std::vector<AddressValue<bool>>, std::chrono::system_clock::time_point);
private:
    pqxx::connection pgconn;

    std::unique_ptr<std::thread> run_thread;
    bool should_close;
    bool is_running;
    const std::shared_ptr<spdlog::logger> logger;
    const std::chrono::milliseconds dump_time_ms;
    std::unordered_map<uint32_t, std::string> words_names;
    std::unordered_map<uint32_t, std::string> dwords_names;
    std::unordered_map<uint32_t, std::string> floats_names;
    
    /*template <typename T>
    concept RegisterValue = std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, float>;
    void push_generic(std::vector<AddressValue<T>>, std::chrono::system_clock::time_point);*/

    unsigned int current_queue;
    std::unordered_map<std::string, std::vector<std::string>> samples_queues[2];

    void dump_samples();
    void run();

    //ExecStatusType postgre_exec(const std::string & command);
};
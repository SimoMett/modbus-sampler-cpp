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
    static const std::string WORKER_VERSION;
    
    PostgreWorker(std::shared_ptr<spdlog::logger> logger, PostgreWorkerConfig & config, float dump_time_s, json tags);
    ~PostgreWorker() override;

    void start() override;
    void join() override;
    void stop() override;
    bool running() override;
    void push_words(std::vector<AddressValue<uint16_t>>, std::chrono::system_clock::time_point) override;
    void push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point) override;
    void push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point) override;
    void push_coils(std::vector<AddressValue<bool>>, std::chrono::system_clock::time_point) override;
    void push_bits(std::vector<BitAddressValue>, std::chrono::system_clock::time_point) override;

private:
    pqxx::connection * pgconn;

    std::unique_ptr<std::thread> run_thread;
    bool should_close;
    bool is_running;
    const std::shared_ptr<spdlog::logger> logger;
    const std::chrono::milliseconds dump_time_ms;
    std::unordered_map<addr_t, std::string> words_names;
    std::unordered_map<addr_t, std::string> dwords_names;
    std::unordered_map<addr_t, std::string> floats_names;
    
    template <RegisterValue T>
    void push_generic(std::vector<AddressValue<T>>, std::chrono::system_clock::time_point, std::unordered_map<uint32_t, std::string> &);

    unsigned int current_queue;
    std::unordered_map<std::string, std::vector<std::string>> samples_queues[2];

    void dump_samples() override;
    void run();

    //ExecStatusType postgre_exec(const std::string & command);
};
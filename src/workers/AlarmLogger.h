#include "ConsumerWorker.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

using nlohmann::json;

class AlarmLogger: public ConsumerWorker
{
public:
    static const std::string WORKER_VERSION;
    
    AlarmLogger(std::shared_ptr<spdlog::logger> logger, json alarm_logger_config, json tags);
    ~AlarmLogger() override {};

    void start() override {};
    void join() override {};
    void stop() override {};
    bool running() override { return true; };

    void push_words(std::vector<AddressValue<uint16_t>>, std::chrono::system_clock::time_point) override {};
    void push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point) override {};
    void push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point) override {};
    void push_coils(std::vector<AddressValue<bool>> samples, std::chrono::system_clock::time_point) override;
    void push_bits(std::vector<BitAddressValue> samples, std::chrono::system_clock::time_point) override;

private:
    const std::shared_ptr<spdlog::logger> logger;
    std::string logger_name;
    std::unordered_map<addr_t, std::array<std::string, 16>> bits_names;
    std::unordered_map<std::string, bool> current_alarms_status;
    std::unordered_map<std::string, bool> default_alarms_status;

    void dump_samples() override {};
};
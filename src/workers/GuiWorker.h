#include <deque>
#include <SDL2/SDL.h>

#include "ConsumerWorker.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "SamplesRingQueue.h"

using nlohmann::json;

class GuiWorker: public ConsumerWorker
{
public:
    static const std::string WORKER_VERSION;

    GuiWorker(std::shared_ptr<spdlog::logger> logger, std::string window_name, json gui_config, json tags);
    ~GuiWorker() override;

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
    const std::string window_name;
    std::unique_ptr<std::thread> run_thread;
    bool should_close;
    bool is_running;
    const std::shared_ptr<spdlog::logger> logger;
    std::unordered_map<addr_t, std::string> words_names;
    std::unordered_map<addr_t, std::string> floats_names;
    std::unordered_map<addr_t, std::string> dwords_names;
    std::unordered_map<addr_t, std::string> coils_names;
    unsigned int current_queue;
    std::unordered_map<std::string, SamplesRingQueue> samples_queues;
    const unsigned short refresh_rate_ms;
    const unsigned short deque_max_len;
    const bool light_theme;

    void run();
    void dump_samples() override;
};
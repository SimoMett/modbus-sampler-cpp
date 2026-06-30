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

    GuiWorker(std::shared_ptr<spdlog::logger> logger, std::string window_name, json gui_config, json tags);
    ~GuiWorker();

    void start();
    void join();
    void stop();
    bool running();
    void push_words(std::vector<AddressValue<uint16_t>>, std::chrono::system_clock::time_point);
    void push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point);
    void push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point);

private:
    const std::string window_name;
    std::unique_ptr<std::thread> run_thread;
    bool should_close;
    bool is_running;
    const std::shared_ptr<spdlog::logger> logger;
    std::unordered_map<uint32_t, std::string> words_names;
    std::unordered_map<uint32_t, std::string> floats_names;
    std::unordered_map<uint32_t, std::string> dwords_names;
    unsigned int current_queue;
    std::unordered_map<std::string, SamplesRingQueue> samples_queues;
    const unsigned short refresh_rate_ms;
    const unsigned short deque_max_len;
    const bool light_theme;

    void run();
    void dump_samples();

    static std::string format_time(const std::chrono::system_clock::time_point & tp);
};
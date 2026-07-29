#include <memory>
#include <vector>
#include <chrono>
#include <unordered_map>
#include "ConsumerWorker.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

#pragma once

using json = nlohmann::json;

class CsvWorker: public ConsumerWorker
{
public:
    static const std::string WORKER_VERSION;
    
    CsvWorker(std::shared_ptr<spdlog::logger> logger, std::string output_dir, float dump_time_s, json tags);
    ~CsvWorker() override;

    void start() override;
    void join() override;
    void stop() override;
    bool running() override;

    void push_words(std::vector<AddressValue<uint16_t>> samples, std::chrono::system_clock::time_point) override;
    void push_floats(std::vector<AddressValue<float>> samples, std::chrono::system_clock::time_point) override;
    void push_dwords(std::vector<AddressValue<uint32_t>> samples, std::chrono::system_clock::time_point) override;
    void push_coils(std::vector<AddressValue<bool>> samples, std::chrono::system_clock::time_point) override;
    void push_bits(std::vector<BitAddressValue> samples, std::chrono::system_clock::time_point) override;

private:
    std::unique_ptr<std::thread> run_thread;
    bool should_close;
    bool is_running;
    const std::shared_ptr<spdlog::logger> logger;
    const std::filesystem::path output_dir;
    const std::chrono::milliseconds dump_time_ms;
    std::unordered_map<addr_t, std::string> words_names;
    std::unordered_map<addr_t, std::string> floats_names;
    std::unordered_map<addr_t, std::string> dwords_names;
    std::unordered_map<addr_t, std::string> coils_names;
    std::unordered_map<addr_t, std::array<std::string, 16>> bits_names;
    unsigned int current_queue;
    std::unordered_map<std::string, std::vector<std::string>> samples_queues[2];

    void dump_samples() override;
    void run();
};
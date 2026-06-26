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
    CsvWorker(std::shared_ptr<spdlog::logger> logger, std::string output_dir, float dump_time_s, json tags);
    ~CsvWorker();

    void start();
    void join();
    void stop();
    bool running();

    void push_words(std::vector<AddressValue<uint16_t>> samples, std::chrono::system_clock::time_point);
    void push_floats(std::vector<AddressValue<float>> samples, std::chrono::system_clock::time_point);
    void push_dwords(std::vector<AddressValue<uint32_t>> samples, std::chrono::system_clock::time_point);

private:
    std::unique_ptr<std::thread> run_thread;
    bool should_close;
    bool is_running;
    const std::shared_ptr<spdlog::logger> logger;
    const std::filesystem::path output_dir;
    const std::chrono::milliseconds dump_time_ms;
    std::unordered_map<uint32_t, std::string> words_names;
    std::unordered_map<uint32_t, std::string> floats_names;
    std::unordered_map<uint32_t, std::string> dwords_names;
    unsigned int current_queue;
    std::unordered_map<std::string, std::vector<std::string>> samples_queues[2];

    void dump_samples();
    void run();

    static std::string format_name(const std::string & name);
    static std::string format_time(const std::chrono::system_clock::time_point &);
};
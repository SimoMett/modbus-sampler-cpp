#include <fstream>
#include <sstream>
#include <iostream>
#include "CsvWorker.h"
#include "simomett/common.h"

using simomett::MbValueType;

CsvWorker::CsvWorker(std::shared_ptr<spdlog::logger> logger, std::string output_dir, float dump_time_s, json tags) : logger(logger), output_dir(output_dir), dump_time_ms(static_cast<int>(dump_time_s * 1000)), is_running(false), current_queue(0)
{
    std::unordered_map<uint32_t, std::string> *maps[4];
    maps[MbValueType::WORD_TYPE] = &this->words_names;
    maps[MbValueType::DWORD_TYPE] = &this->dwords_names;
    maps[MbValueType::REAL_TYPE] = &this->floats_names;
    maps[MbValueType::COIL_TYPE] = &this->coils_names;

    std::string json_str[3];
    json_str[MbValueType::WORD_TYPE] = "words";
    json_str[MbValueType::DWORD_TYPE] = "dwords";
    json_str[MbValueType::REAL_TYPE] = "floats";
    json_str[MbValueType::COIL_TYPE] = "coils";

    for (auto v : {MbValueType::WORD_TYPE, MbValueType::DWORD_TYPE, MbValueType::REAL_TYPE, MbValueType::COIL_TYPE})
    {
        if (!tags[json_str[v]].is_null())
        {
            for (json s : tags[v])
            {
                maps[v]->insert({s["address"].get<uint32_t>(),
                                s["tag"].get<std::string>()});
            }
        }
    }
}

CsvWorker::~CsvWorker()
{
    this->words_names.clear();
    this->dwords_names.clear();
    this->floats_names.clear();
    this->coils_names.clear();
}

void CsvWorker::start()
{
    this->run_thread = std::make_unique<std::thread>(&CsvWorker::run, this);
}

void CsvWorker::join()
{
    this->run_thread->join();
}

bool CsvWorker::running()
{
    return this->is_running;
}

void CsvWorker::dump_samples()
{
    // std::cout << "dump_samples" << std::endl;
    // std::cout << this->samples_queues[this->current_queue].size() << std::endl;
    if (this->samples_queues[this->current_queue].size())
    {
        int old_queue = this->current_queue;

        // Critical part - mutex needed ?
        this->current_queue = (this->current_queue + 1) % 2;
        //

        // dump to csv
        auto &q = this->samples_queues[old_queue];
        std::vector<std::string> keys;
        for (auto &kv : q)
            keys.push_back(kv.first);

        // std::cout << keys.size() << "\n";
        for (std::string filename : keys)
        {
            // std::cout << filename << "\n";
            std::ofstream output(this->output_dir / (filename + ".csv"), std::ios_base::app);
            if (!output.is_open())
                throw std::runtime_error(
                    std::string("Couldn't open file ") + (this->output_dir / (filename + ".csv")).string() + ". Does the output folder exists?");

            for (auto &csv_line : q[filename])
                output << csv_line << "\n";

            q[filename].clear();
        }
    }
}

void CsvWorker::run()
{
    this->logger->info("CSV worker started");
    this->is_running = true;
    this->should_close = false;

    try
    {
        while (!this->should_close)
        {
            std::this_thread::sleep_for(this->dump_time_ms);
            this->dump_samples();
        }
        this->dump_samples();
        this->logger->info("CSV worker stopped");
    }
    catch (const std::exception &e)
    {
        this->logger->error(e.what());
        this->logger->error("CSV worker stopped due to error");
    }
    this->is_running = false;
}

void CsvWorker::stop()
{
    this->should_close = true;
}

void CsvWorker::push_words(std::vector<AddressValue<uint16_t>> samples, std::chrono::system_clock::time_point instant)
{
    for (AddressValue<uint16_t> &sample : samples)
    {
        if (this->words_names.find(sample.address) == this->words_names.end())
        {
            // std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string tag_name = ConsumerWorker::format_name(this->words_names[sample.address]); // also 'filename'
        // std::cout << tag_name << "\n";

        std::ostringstream csv_line;
        csv_line << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "," << sample.val;

        this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
    }
}

void CsvWorker::push_floats(std::vector<AddressValue<float>> samples, std::chrono::system_clock::time_point instant)
{
    for (AddressValue<float> &sample : samples)
    {
        if (this->floats_names.find(sample.address) == this->floats_names.end())
        {
            // std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string tag_name = ConsumerWorker::format_name(this->floats_names[sample.address]); // also 'filename'
        // std::cout << tag_name << "\n";

        std::ostringstream csv_line;
        csv_line << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "," << sample.val;

        this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
    }
}

void CsvWorker::push_dwords(std::vector<AddressValue<uint32_t>> samples, std::chrono::system_clock::time_point instant)
{
    for (AddressValue<uint32_t> &sample : samples)
    {
        if (this->dwords_names.find(sample.address) == this->dwords_names.end())
        {
            // std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string tag_name = ConsumerWorker::format_name(this->dwords_names[sample.address]); // also 'filename'

        std::ostringstream csv_line;
        csv_line << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "," << sample.val;

        this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
    }
}

void CsvWorker::push_coils(std::vector<AddressValue<bool>> samples, std::chrono::system_clock::time_point instant)
{
    for (AddressValue<bool> &sample : samples)
    {
        if (this->coils_names.find(sample.address) == this->coils_names.end())
            continue;

        std::string tag_name = ConsumerWorker::format_name(this->coils_names[sample.address]);

        std::ostringstream csv_line;
        csv_line << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "," << sample.val;

        this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
    }
}

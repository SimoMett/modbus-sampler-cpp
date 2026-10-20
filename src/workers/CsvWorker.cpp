#include <fstream>
#include <sstream>
#include <iostream>
#include "CsvWorker.h"
#include "simomett/common.h"

using simomett::MbValueType;

const std::string CsvWorker::WORKER_VERSION = "CsvWorker build: 1";

CsvWorker::CsvWorker(std::shared_ptr<spdlog::logger> logger, std::string output_dir, float dump_time_s, json _tags) : logger(logger), output_dir(output_dir), dump_time_ms(static_cast<int>(dump_time_s * 1000)), current_queue(0), tags(_tags)
{

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
        for (const auto &kv : q)
            keys.push_back(kv.first);

        // std::cout << keys.size() << "\n";
        for (const std::string & filename : keys)
        {
            // std::cout << filename << "\n";
            std::ofstream output(this->output_dir / (filename + ".csv"), std::ios_base::app);
            if (!output.is_open())
                throw std::runtime_error(
                    std::string("Couldn't open file ") + (this->output_dir / (filename + ".csv")).string() + ". Does the output folder exists?");

            for (const auto &csv_line : q[filename])
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
        if (std::find(this->tags.get_words_addresses().begin(), this->tags.get_words_addresses().end(), sample.address) != this->tags.get_words_addresses().end())
        {
            // std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string tag_name = ConsumerWorker::format_name(this->tags.get_word_name(sample.address)); // also 'filename'
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
        if (std::find(this->tags.get_floats_addresses().begin(), this->tags.get_floats_addresses().end(), sample.address) != this->tags.get_floats_addresses().end())
        {
            // std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string tag_name = ConsumerWorker::format_name(this->tags.get_float_name(sample.address)); // also 'filename'
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
        if (std::find(this->tags.get_dwords_addresses().begin(), this->tags.get_dwords_addresses().end(), sample.address) != this->tags.get_dwords_addresses().end())
        {
            // std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string tag_name = ConsumerWorker::format_name(this->tags.get_dword_name(sample.address)); // also 'filename'

        std::ostringstream csv_line;
        csv_line << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "," << sample.val;

        this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
    }
}

void CsvWorker::push_coils(std::vector<AddressValue<bool>> samples, std::chrono::system_clock::time_point instant)
{
    for (AddressValue<bool> &sample : samples)
    {
        if (std::find(this->tags.get_coils_addresses().begin(), this->tags.get_coils_addresses().end(), sample.address) != this->tags.get_coils_addresses().end())
            continue;

        std::string tag_name = ConsumerWorker::format_name(this->tags.get_coil_name(sample.address));

        std::ostringstream csv_line;
        csv_line << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "," << sample.val;

        this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
    }
}

void CsvWorker::push_bits(std::vector<BitAddressValue> samples, std::chrono::system_clock::time_point instant)
{
    //FIXME: never used. Need testing.
    for (BitAddressValue &sample : samples)
    {
        if (std::find(this->tags.get_bits_addresses().begin(), this->tags.get_bits_addresses().end(), sample.address) != this->tags.get_bits_addresses().end())
            continue;

        for(int i = 0; i < 16; i++)
        {
            const std::string & bn = this->tags.get_bit_name(sample.address, i);
            if(!bn.empty())
            {
                std::string tag_name = ConsumerWorker::format_name(bn);

                std::ostringstream csv_line;
                csv_line << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "," << sample.val;

                this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
            }
        }        
    }
}

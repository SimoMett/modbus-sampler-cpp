#include <fstream>
#include <sstream>
#include <iostream>
#include "CsvWorker.h"

CsvWorker::CsvWorker(std::shared_ptr<spdlog::logger> logger, std::string output_dir, float dump_time_s, json tags) :
logger(logger), output_dir(output_dir), dump_time_ms(static_cast<int>(dump_time_s * 1000)), is_running(false), current_queue(0)
{
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::string>*> map = {
        std::make_pair<std::string, std::unordered_map<uint32_t, std::string>*>("words", &this->words_names),
        std::make_pair<std::string, std::unordered_map<uint32_t, std::string>*>("dwords", &this->dwords_names),
        std::make_pair<std::string, std::unordered_map<uint32_t, std::string>*>("floats", &this->floats_names),
    };

    for(auto & v : {"words", "dwords", "floats"})
    {
        if (!tags[v].is_null())
        {
            for (json s : tags[v])
            {
                map[v]->insert({s["address"].get<uint32_t>(),
                                        s["tag"].get<std::string>()});
            }
        }
    }

    /*if (!tags["words"].is_null())
    {
        for (json s : tags["words"])
        {
            this->words_names.insert({s["address"].get<uint32_t>(),
                                      s["tag"].get<std::string>()});
        }
    }

    if (!tags["dwords"].is_null())
    {
        for (json s : tags["dwords"])
        {
            this->dwords_names.insert({s["address"].get<uint32_t>(),
                                      s["tag"].get<std::string>()});
        }
    }

    if (!tags["floats"].is_null())
    {
        for (json s : tags["floats"])
        {
            this->dwords_names.insert({s["address"].get<uint32_t>(),
                                      s["tag"].get<std::string>()});
        }
    }*/
}

CsvWorker::~CsvWorker()
{
    this->words_names.clear();
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
            //std::cout << filename << "\n";
            std::ofstream output(this->output_dir / (filename + ".csv"), std::ios_base::app);
            if (!output.is_open())
                throw std::runtime_error(
                    std::string("Couldn't open file ") + (this->output_dir / (filename + ".csv")).string() + ". Does the output folder exists?");

            for (auto & csv_line : q[filename])
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
    for (AddressValue<uint16_t> & sample : samples)
    {
        if (this->words_names.find(sample.address) == this->words_names.end())
        {
            //std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string tag_name = ConsumerWorker::format_name(this->words_names[sample.address]); // also 'filename'
        // std::cout << tag_name << "\n";

        std::ostringstream csv_line;
        csv_line << CsvWorker::format_time(instant) << "," << sample.val;

        this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
    }
}

void CsvWorker::push_floats(std::vector<AddressValue<float>> samples, std::chrono::system_clock::time_point instant)
{
    for (AddressValue<float> & sample : samples)
    {
        if (this->floats_names.find(sample.address) == this->floats_names.end())
        {
            //std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string tag_name = ConsumerWorker::format_name(this->floats_names[sample.address]); // also 'filename'
        // std::cout << tag_name << "\n";

        std::ostringstream csv_line;
        csv_line << CsvWorker::format_time(instant) << "," << sample.val;

        this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
    }
}

void CsvWorker::push_dwords(std::vector<AddressValue<uint32_t>> samples, std::chrono::system_clock::time_point instant)
{
    for (AddressValue<uint32_t> & sample : samples)
    {
        if (this->dwords_names.find(sample.address) == this->dwords_names.end())
        {
            //std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string tag_name = ConsumerWorker::format_name(this->dwords_names[sample.address]); // also 'filename'

        std::ostringstream csv_line;
        csv_line << CsvWorker::format_time(instant) << "," << sample.val;

        this->samples_queues[this->current_queue][tag_name].push_back(csv_line.str());
    }
}

std::string CsvWorker::format_time(const std::chrono::system_clock::time_point & tp)
{
    using namespace std::chrono;

    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;

    // convert to std::time_t in order to convert to std::tm (broken time)
    auto timer = system_clock::to_time_t(tp);

    // convert to broken time
    std::tm bt = *std::localtime(&timer);

    std::ostringstream oss;

    oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S"); // HH:MM:SS
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}
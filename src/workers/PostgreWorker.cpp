#include <sstream>
#include <iostream>

#include "PostgreWorker.h"
#include "simomett/common.h"

PostgreWorker::PostgreWorker(std::shared_ptr<spdlog::logger> logger, std::string host, uint16_t port, std::string dbname, std::string user, std::string password, float dump_time_s, json tags): logger(logger), dump_time_ms(static_cast<int>(dump_time_s * 1000))
{
    std::stringstream connection_str;
    connection_str << "host=localhost port=5432 dbname=mydatabase user=myuser password=mypassword";
    connection_str << "host=" << host <<" port=" << port << "dbname=" << dbname << " user=" << user << " password=" << password;
    this->pgconn = PQconnectdb(connection_str.str().c_str());

    if(PQstatus(pgconn) != CONNECTION_OK)
        throw std::runtime_error("Cannot initialize Postgre connection");
    
}

PostgreWorker::~PostgreWorker()
{
    PQfinish(pgconn);
}

void PostgreWorker::start()
{
}

void PostgreWorker::join()
{
}

void PostgreWorker::stop()
{
    should_close = true;
}

bool PostgreWorker::running()
{
    return is_running;
}

void PostgreWorker::push_words(std::vector<AddressValue<uint16_t>> samples, std::chrono::system_clock::time_point instant)
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

        //std::ostringstream query_line;
        //query_line << "INSERT INTO "<< tag_name << " (timeandday, value) VALUES (timestamp '" << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "', " << sample.val << ");";
        std::cout << "INSERT INTO "<< tag_name << " (timeandday, value) VALUES (timestamp '" << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "', " << sample.val << ");";

        //this->samples_queues[this->current_queue][tag_name].push_back(query_line.str());
    }
}

void PostgreWorker::push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point)
{
}

void PostgreWorker::push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point)
{
}

void PostgreWorker::push_coils(std::vector<AddressValue<bool>>, std::chrono::system_clock::time_point)
{
}

void PostgreWorker::dump_samples()
{
    if (this->samples_queues[this->current_queue].size())
    {
        int old_queue = this->current_queue;

        // Critical part - mutex needed ?
        this->current_queue = (this->current_queue + 1) % 2;
        //

        // dump to pg
        auto &q = this->samples_queues[old_queue];
        std::vector<std::string> keys;
        for (auto &kv : q)
            keys.push_back(kv.first);

        // std::cout << keys.size() << "\n";
        /*for (std::string filename : keys)
        {
            // std::cout << filename << "\n";
            std::ofstream output(this->output_dir / (filename + ".csv"), std::ios_base::app);
            if (!output.is_open())
                throw std::runtime_error(
                    std::string("Couldn't open file ") + (this->output_dir / (filename + ".csv")).string() + ". Does the output folder exists?");

            for (auto &csv_line : q[filename])
                output << csv_line << "\n";

            q[filename].clear();
        }*/
    }
}

void PostgreWorker::run()
{
    this->logger->info("Postgre worker started");
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
        this->logger->info("Postgre worker stopped");
    }
    catch (const std::exception &e)
    {
        this->logger->error(e.what());
        this->logger->error("Postgre worker stopped due to error");
    }
    this->is_running = false;
}

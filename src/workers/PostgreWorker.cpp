#include <sstream>
#include <iostream>

#include "PostgreWorker.h"
#include "simomett/common.h"

PostgreWorker::PostgreWorker(std::shared_ptr<spdlog::logger> logger, std::string host, uint16_t port, std::string dbname, std::string user, std::string password, float dump_time_s, json tags): logger(logger), dump_time_ms(static_cast<int>(dump_time_s * 1000))
{
    std::stringstream connection_str;
    connection_str << "host=" << host <<" port=" << port << "dbname=" << dbname << " user=" << user << " password=" << password;

    this->pgconn = pqxx::connection(connection_str.str());
}

PostgreWorker::~PostgreWorker()
{
    
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

//Nuovo metodo
/*template <RegisterValue T>
void PostgreWorker::push_generic(std::vector<AddressValue<T>> samples, std::chrono::system_clock::time_point instant)
{
    std::unordered_map<uint32_t, std::string> * tags_names;
    if (std::is_same_v<T, uint32_t>)
        tags_names = &this->dwords_names;
    else if (std::is_same_v<T, float>)
        tags_names = &this->floats_names;
    else
        tags_names = &this->words_names;

    for (AddressValue<T> &sample : samples)
    {
        if (tags_names->find(sample.address) == tags_names->end())
        {
            // std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string table_name = ConsumerWorker::format_name(tags_names[sample.address]); //also 'tag_name'
        // std::cout << tag_name << "\n";

        std::stringstream query_line;
        query_line << "INSERT INTO "<< table_name << " (timeandday, value) VALUES (timestamp '" << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "', " << sample.val << ");";
        std::cout << query_line.str() << "\n";

        //TODO: provare con una lista semplice invece di una unordered_map
        this->samples_queues[this->current_queue][table_name].push_back(query_line.str());
    }
}*/

void PostgreWorker::push_words(std::vector<AddressValue<uint16_t>> samples, std::chrono::system_clock::time_point instant)
{
    for (AddressValue<uint16_t> &sample : samples)
    {
        if (this->words_names.find(sample.address) == this->words_names.end())
        {
            // std::cout << "address " << sample.address << " not found \n";
            continue;
        }
        std::string table_name = ConsumerWorker::format_name(this->words_names[sample.address]); //also 'tag_name'
        // std::cout << tag_name << "\n";

        std::stringstream query_line;
        query_line << "INSERT INTO "<< table_name << " (timeandday, value) VALUES (timestamp '" << simomett::format_time(instant, "%Y-%m-%d %H:%M:%S") << "', " << sample.val << ");";
        //std::cout << query_line.str() << "\n";

        //TODO: provare con una lista semplice invece di una unordered_map
        this->samples_queues[this->current_queue][table_name].push_back(query_line.str());
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
        auto &samples_queue = this->samples_queues[old_queue];

        pqxx::work tx{pgconn};
        tx.exec("BEGIN");
        for (auto &kv : samples_queue)
        {
            const std::string & table_name = kv.first;
            const std::vector<std::string> & queries = kv.second;

            for(auto & query : queries)
            {
                std::cout << query << "\n";
                //tx.exec(query.c_str());
                //TODO: provare anche metodo PQprepare + PQexecPrepared
            }
        }
        tx.exec("COMMIT");
    }
}

void PostgreWorker::run()
{
    this->logger->info("Postgre worker started");
    this->is_running = true;
    this->should_close = false;

    try
    {
        std::chrono::system_clock::time_point start;
        while (!this->should_close)
        {
            start = std::chrono::system_clock::now();

            this->dump_samples();

            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
            if(elapsed_ms > this->dump_time_ms)
            {
                std::stringstream msg;
                msg << "Samples dump took too much time (" << elapsed_ms << " ms)";
                logger->warn(msg.str());
            }
            else
                std::this_thread::sleep_for(this->dump_time_ms - elapsed_ms);
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

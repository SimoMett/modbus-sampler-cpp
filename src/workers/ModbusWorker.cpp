#include <algorithm>
#include <deque>
#include <iostream>
#include <chrono>
#include <concepts>
#include "ModbusWorker.h"

void ModbusWorker::parse_tags(json tags)
{
    if (!tags["words"].is_null())
    {
        std::vector<uint32_t> addresses;
        for (json w : tags["words"])
            addresses.push_back(w["address"].get<uint32_t>());

        this->words = segmentize(addresses, 1);
    }

    if (!tags["floats"].is_null())
    {
        std::vector<uint32_t> addresses;
        for (json w : tags["floats"])
            addresses.push_back(w["address"]);

        this->floats = segmentize(addresses, 2);
    }

    if (!tags["dwords"].is_null())
    {
        std::vector<uint32_t> addresses;
        for (json w : tags["dwords"])
            addresses.push_back(w["address"]);

        this->dwords = segmentize(addresses, 2);
    }
}

ModbusWorker::ModbusWorker(std::shared_ptr<spdlog::logger> logger, Modbus::TcpSettings *modbus_settings, json tags, std::vector<std::shared_ptr<ConsumerWorker>> workers) :
logger(logger), workers(workers), one_indexed(tags["one_indexed"].get<bool>()), reg_order(tags["register_order"].get<std::string>() == "R1R0" ? RegisterOrder::R1R0 : RegisterOrder::R0R1), scantime_ms(tags["scantime_ms"].get<unsigned int>()),
    port(createClientPort(Modbus::TCP, modbus_settings, true)), client(1, this->port.get())
{
    this->should_close = false;

    this->parse_tags(tags);
}

ModbusWorker::~ModbusWorker()
{
    this->should_close = true;
}

void ModbusWorker::start()
{
    this->run_thread = std::thread(&ModbusWorker::run, this);
}

void ModbusWorker::join()
{
    this->run_thread.join();
}

void ModbusWorker::run()
{
    const int max_retries = 3;
    int retries = max_retries;

    this->logger->info("Modbus worker started");

    try
    {
        while (!this->should_close)
        {
            std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

            // logic here
            for (const Segment &s : this->bits)
            {
                // TODO
                logger->warn("'Bits' not implemented yet");
            }

            for (const Segment &s : this->coils)
            {
                // TODO
                logger->warn("'Coils' not implemented yet");
            }

            for (const Segment &s : this->words)
            {
                //Metodo nuovo
                std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                
                std::vector<AddressValue<uint16_t>> samples = fetch_holding_registers<uint16_t>(s);

                for (const auto &worker : this->workers)
                {
                    if (worker->running())
                        worker->push_words(samples, now);
                }

                // fetch_and_push_words(s);
            }

            for (const Segment &s : this->floats)
            {
                //Metodo vecchio
                fetch_and_push_floats(s);
            }

            for (const Segment &s : this->dwords)
            {
                fetch_and_push_dwords(s);
            }

            workers.erase(std::remove_if(workers.begin(), workers.end(), [](const std::shared_ptr<ConsumerWorker> &w)
                                         { return !w->running(); }),
                          workers.end());
            if (workers.size() == 0)
            {
                logger->info("No more consumer workers left");
                this->should_close = true;
                break;
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(this->scantime_ms) - (std::chrono::system_clock::now() - start));
        }
    }
    catch (const std::exception &e)
    {
        logger->error(e.what());
        this->should_close = true;
    }

    logger->info("Modbus worker stopped");
}

void ModbusWorker::fetch_and_push_words(const Segment &s)
{
    // std::cout << "words: " << this->words.size() << "\n";
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    auto results = std::make_unique<uint16_t[]>(s.end - s.start + 1);

    Modbus::StatusCode code = this->client.readHoldingRegisters(s.start - (this->one_indexed ? 400001 : 400000), s.end - s.start + 1, results.get());
    if (code != Modbus::Status_Good)
    {
        // throw the correct message
        using Modbus::StatusCode;
        switch (code)
        {
        case StatusCode::Status_Bad | StatusCode::Status_BadTcpConnect:
            throw std::runtime_error("Unable to create a TCP connection");
            break;
        default:
        {
            std::stringstream msg;
            msg << "Failed to read holding registers. Returned code 0x" << std::hex << code << ". See https://github.com/serhmarch/ModbusLib/blob/main/src/ModbusGlobal.h";
            throw std::runtime_error(msg.str());
        }
        }
    }

    std::vector<AddressValue<uint16_t>> samples;
    // std::cout << "Segment " << s.start << ", "<< s.end << "\n";

    for (uint32_t i = s.start; i < s.end; i++)
    {
        // std::cout << "[i] " << i <<": " << results[i-s.start] << "\n";
        samples.push_back(AddressValue<uint16_t>{i, results[i - s.start]});
    }

    // std::cout << samples.size() << "\n";
    for (const auto &worker : this->workers)
    {
        if (worker->running())
            worker->push_words(samples, now);
    }
}

void ModbusWorker::fetch_and_push_floats(const Segment &s)
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    auto results = std::make_unique<uint16_t[]>(s.end - s.start + 2);

    Modbus::StatusCode code = this->client.readHoldingRegisters(s.start - (this->one_indexed ? 400001 : 400000), s.end - s.start + 2, results.get());
    if (code != Modbus::Status_Good)
    {
        // throw the correct message
        using Modbus::StatusCode;
        switch (code)
        {
        case StatusCode::Status_Bad | StatusCode::Status_BadTcpConnect:
            throw std::runtime_error("Unable to create a TCP connection");
            break;
        default:
        {
            std::stringstream msg;
            msg << "Failed to read holding registers. Returned code 0x" << std::hex << code << ". See https://github.com/serhmarch/ModbusLib/blob/main/src/ModbusGlobal.h";
            throw std::runtime_error(msg.str());
        }
        }
    }

    std::vector<AddressValue<float>> samples;

    for (uint32_t i = s.start; i < s.end; i += 2)
    {
        uint16_t rr[2] = {results[i - s.start], results[i + 1 - s.start]};
        float v = regs_to_float(rr, this->reg_order);
        samples.push_back(AddressValue<float>{i, v});
    }

    for (const auto &worker : this->workers)
    {
        if (worker->running())
            worker->push_floats(samples, now);
    }
}

void ModbusWorker::fetch_and_push_dwords(const Segment &s)
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    auto results = std::make_unique<uint16_t[]>(s.end - s.start + 2);

    Modbus::StatusCode code = this->client.readHoldingRegisters(s.start - (this->one_indexed ? 400001 : 400000), s.end - s.start + 2, results.get());
    if (code != Modbus::Status_Good)
    {
        // throw the correct message
        using Modbus::StatusCode;
        switch (code)
        {
        case StatusCode::Status_Bad | StatusCode::Status_BadTcpConnect:
            throw std::runtime_error("Unable to create a TCP connection");
            break;
        default:
        {
            std::stringstream msg;
            msg << "Failed to read holding registers. Returned code 0x" << std::hex << code << ". See https://github.com/serhmarch/ModbusLib/blob/main/src/ModbusGlobal.h";
            throw std::runtime_error(msg.str());
        }
        }
    }

    std::vector<AddressValue<uint32_t>> samples;

    for (uint32_t i = s.start; i < s.end; i += 2)
    {
        uint16_t rr[2] = {results[i - s.start], results[i + 1 - s.start]};
        uint32_t v = regs_to_uint32(rr, this->reg_order);
        samples.push_back(AddressValue<uint32_t>{i, v});
    }

    for (const auto &worker : this->workers)
    {
        if (worker->running())
            worker->push_dwords(samples, now);
    }
}

template <RegisterValue T>
std::vector<AddressValue<T>> ModbusWorker::fetch_holding_registers(const Segment &s)
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    auto results = std::make_unique<uint16_t[]>(s.end - s.start + std::is_same_v<T, uint16_t> ? 1 : 2);

    Modbus::StatusCode code = this->client.readHoldingRegisters(s.start - (this->one_indexed ? 400001 : 400000), s.end - s.start + std::is_same_v<T, uint16_t> ? 1 : 2, results.get());
    if (code != Modbus::Status_Good)
    {
        // throw the correct message
        using Modbus::StatusCode;
        switch (code)
        {
        case StatusCode::Status_Bad | StatusCode::Status_BadTcpConnect:
            throw std::runtime_error("Unable to create a TCP connection");
            break;
        default:
        {
            std::stringstream msg;
            msg << "Failed to read holding registers. Returned code 0x" << std::hex << code << ". See https://github.com/serhmarch/ModbusLib/blob/main/src/ModbusGlobal.h";
            throw std::runtime_error(msg.str());
        }
        }
    }

    std::vector<AddressValue<T>> samples;

    for (uint32_t i = s.start; i < s.end; i++)
    {
        if (std::is_same_v<T, uint16_t>)
        {
            samples.push_back(AddressValue<uint16_t>{i, results[i - s.start]});
        }
        else
        {
            uint16_t rr[2] = {results[i - s.start], results[i + 1 - s.start]};
            T v;
            if (std::is_same_v<T, float>)
                v = regs_to_float(rr, this->reg_order);
            else
                v = regs_to_uint32(rr, this->reg_order);
            samples.push_back(AddressValue<T>{i, v});
        }
    }

    return samples;
}

void ModbusWorker::fetch_and_push_coils(const Segment &s)
{
    // std::cout << "words: " << this->words.size() << "\n";
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    auto results = std::make_unique<bool[]>(s.end - s.start + 1);

    Modbus::StatusCode code = this->client.readCoilsAsBoolArray(s.start - (this->one_indexed ? 1 : 0), s.end - s.start + 1, results.get());
    if (code != Modbus::Status_Good)
    {
        // throw the correct message
        using Modbus::StatusCode;
        switch (code)
        {
        case StatusCode::Status_Bad | StatusCode::Status_BadTcpConnect:
            throw std::runtime_error("Unable to create a TCP connection");
            break;
        default:
        {
            std::stringstream msg;
            msg << "Failed to read coils. Returned code 0x" << std::hex << code << ". See https://github.com/serhmarch/ModbusLib/blob/main/src/ModbusGlobal.h";
            throw std::runtime_error(msg.str());
        }
        }
    }

    std::vector<AddressValue<bool>> samples;
    // std::cout << "Segment " << s.start << ", "<< s.end << "\n";

    for (uint32_t i = s.start; i < s.end; i++)
    {
        // std::cout << "[i] " << i <<": " << results[i-s.start] << "\n";
        samples.push_back(AddressValue<bool>{i, results[i - s.start]});
    }

    // std::cout << samples.size() << "\n";
    for (const auto &worker : this->workers)
    {
        if (worker->running())
            worker->push_coils(samples, now);
    }
}

void ModbusWorker::stop()
{
    this->should_close = true;
}

std::vector<Segment> ModbusWorker::segmentize(std::vector<uint32_t> addresses, uint16_t distance)
{
    // Condense addresses to reduce 'read_holding_registers' calls
    std::sort(addresses.begin(), addresses.end());
    auto tmp = std::vector<Segment>({Segment{addresses[0], addresses[0] + 1}});
    for (int i = 1; i < addresses.size(); i++)
    {
        if ((addresses[i] - addresses[i - 1]) <= distance)
        {
            tmp.back().end = addresses[i];
        }
        else
        {
            Segment s{addresses[i], addresses[i] + 1};
            tmp.push_back(s);
        }
    }

    return tmp;
}

uint32_t ModbusWorker::regs_to_uint32(uint16_t regs[2], RegisterOrder reg_order)
{
    uint32_t x = 0;
    if (reg_order == R1R0)
        x = (regs[1] << 16) | regs[0];
    else
        x = (regs[0] << 16) | regs[1];

    return x;
}

inline float ModbusWorker::regs_to_float(uint16_t regs[2], RegisterOrder reg_order)
{
    uint32_t x = regs_to_uint32(regs, reg_order);
    return *(float *)(&x);
}
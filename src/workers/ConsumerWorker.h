#include <vector>
#include <chrono>

#pragma once

template <typename T>
struct AddressValue
{
    uint32_t address;
    T val;
};

class ConsumerWorker
{
public:
    ConsumerWorker();
    virtual ~ConsumerWorker() = 0;
    virtual void start() = 0;
    virtual void join() = 0;
    virtual void stop() = 0;
    virtual bool running() = 0;
    virtual void push_words(std::vector<AddressValue<uint16_t>>, std::chrono::system_clock::time_point)=0;
    virtual void push_floats(std::vector<AddressValue<float>>, std::chrono::system_clock::time_point)=0;
    virtual void push_dwords(std::vector<AddressValue<uint32_t>>, std::chrono::system_clock::time_point)=0;
protected:
    virtual void dump_samples() = 0;
    static std::string format_name(const std::string & name);
};
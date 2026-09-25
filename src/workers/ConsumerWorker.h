#include <vector>
#include <chrono>

#pragma once

typedef uint32_t addr_t;

template <typename T>
struct AddressValue
{
    addr_t address;
    T val;
};

struct BitAddressValue
{
    addr_t address;
    uint8_t bit;
    bool val;
};

template <typename T>
concept RegisterValue = std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, float>;

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
    virtual void push_coils(std::vector<AddressValue<bool>>, std::chrono::system_clock::time_point)=0;
    virtual void push_bits(std::vector<BitAddressValue>, std::chrono::system_clock::time_point)=0;
protected:
    virtual void dump_samples() = 0;
    static std::string format_name(const std::string & name);
};
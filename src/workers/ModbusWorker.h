#include <vector>
#include <thread>
#include <memory>

#include "spdlog/spdlog.h"
#include "modbus/ModbusClient.h"
#include "modbus/ModbusClientPort.h"
#include "nlohmann/json.hpp"
#include "ConsumerWorker.h"

using json = nlohmann::json;

template <typename T>
concept RegisterValue = std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, float>;

struct Segment
{
    uint32_t start;
    uint32_t end;
};

enum RegisterOrder
{
    R1R0,
    R0R1
};

class ModbusWorker
{
public:
    ModbusWorker(std::shared_ptr<spdlog::logger> logger, Modbus::TcpSettings *modbus_settings, json tags, std::vector<std::shared_ptr<ConsumerWorker>>);
    ~ModbusWorker();

    void start();
    void join();
    void stop();

private:
    std::thread run_thread;
    bool should_close;
    const std::shared_ptr<spdlog::logger> logger;
    std::vector<std::shared_ptr<ConsumerWorker>> workers;
    const bool one_indexed;
    const RegisterOrder reg_order;
    const unsigned int scantime_ms;

    //support vars
    std::unique_ptr<ModbusClientPort> port;
    ModbusClient client;
    
    //Addresses segments for Modbus polling
    std::vector<Segment> bits;
    std::vector<Segment> coils;
    std::vector<Segment> words;
    std::vector<Segment> floats;
    std::vector<Segment> dwords;

    void parse_tags(json tags);

    void run();

    void fetch_and_push_words(const Segment &s);
    void fetch_and_push_floats(const Segment &s);
    void fetch_and_push_dwords(const Segment &s);
    void fetch_and_push_coils(const Segment &s);

    template <RegisterValue T>
    std::vector<AddressValue<T>> fetch_holding_registers(const Segment &s);

    static std::vector<Segment> segmentize(std::vector<uint32_t> addresses, uint16_t distance);
    static uint32_t regs_to_uint32(uint16_t regs[2], RegisterOrder reg_order);
    static float regs_to_float(uint16_t regs[2], RegisterOrder reg_order);
};
#include <iostream>
#include "AlarmLogger.h"

AlarmLogger::AlarmLogger(std::shared_ptr<spdlog::logger> logger, json alarm_logger_config, json tags): logger(logger), logger_name("AlarmLogger")
{
    if(alarm_logger_config["tagValues"].is_null())
        throw std::runtime_error("Missing 'tagValues' field in config json for AlarmLogger");

    if(!alarm_logger_config["name"].is_null() && alarm_logger_config["name"].is_string())
        logger_name = alarm_logger_config["name"].get<std::string>();

    // dedicated logic for parsing bits addresses and bitmasks.
    if (!tags["bits"].is_null())
    {
        for(auto j : tags["bits"])
        {
            std::array<std::string, 16> rr;
            for(int i=0; i<16; i++)
                rr[i] = j["tags"][i].is_string() ? j["tags"][i].get<std::string>() : "";

            bits_names.insert({j["address"].get<addr_t>(), rr});
        }
    }

    for(auto j : alarm_logger_config["tagValues"].items())
    {
        this->current_alarms_status.insert({j.key(), j.value().get<unsigned int>() != 0});
        this->default_alarms_status.insert({j.key(), j.value().get<unsigned int>() != 0});
    }
}

void AlarmLogger::push_coils(std::vector<AddressValue<bool>>, std::chrono::system_clock::time_point)
{
    //TODO
}

void AlarmLogger::push_bits(std::vector<BitAddressValue> samples, std::chrono::system_clock::time_point)
{
    for(BitAddressValue & sample : samples)
    {
        std::string & tag_name = this->bits_names.at(sample.address)[sample.bit];
        if(this->current_alarms_status.at(tag_name) != sample.val)
        {
            this->current_alarms_status[tag_name] = sample.val;

            if(this->current_alarms_status[tag_name] != this->default_alarms_status[tag_name])
                logger->error(std::format("[{}] \033[91m{}\033[0m", logger_name, tag_name));
            else
                logger->info(std::format("[{}] Restored '{}'", logger_name, tag_name));
        }
    }
}

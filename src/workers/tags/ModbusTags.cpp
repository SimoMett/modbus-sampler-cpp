#include <iostream>
#include <format>
#include "ModbusTags.h"

using simomett::MbValueType;

ModbusTags::ModbusTags(json &tags)
{
    std::string json_str[4];
    json_str[MbValueType::WORD_TYPE] = "words";
    json_str[MbValueType::DWORD_TYPE] = "dwords";
    json_str[MbValueType::REAL_TYPE] = "floats";
    json_str[MbValueType::COIL_TYPE] = "coils";

    std::unordered_map<addr_t, std::string> *maps[4];
    maps[MbValueType::WORD_TYPE] = &this->words_names;
    maps[MbValueType::DWORD_TYPE] = &this->dwords_names;
    maps[MbValueType::REAL_TYPE] = &this->floats_names;
    maps[MbValueType::COIL_TYPE] = &this->coils_names;

    for (auto v : {MbValueType::WORD_TYPE, MbValueType::DWORD_TYPE, MbValueType::REAL_TYPE, MbValueType::COIL_TYPE})
    {
        if (tags[json_str[v]].is_array())
        {
            for (json s : tags[json_str[v]])
            {
                maps[v]->insert({
                    s["address"].get<addr_t>(),
                    s["tag"].get<std::string>()
                });
            }
        }
    }

    // dedicated logic for parsing bits addresses and bitmasks.
    if (tags["bits"].is_array())
    {
        for(auto j : tags["bits"])
        {
            std::array<std::string, 16> rr;
            for(int i=0; i<16; i++)
                rr[i] = j["tags"][i].is_string() ? j["tags"][i].get<std::string>() : "";

            bits_names.insert({
                j["address"].get<addr_t>(),
                rr
            });
        }
    }
}

bool ModbusTags::contain_address_in_words(addr_t address)
{
    return this->words_names.find(address) == this->words_names.end();
}

bool ModbusTags::contain_address_in_floats(addr_t address)
{
    return this->floats_names.find(address) == this->words_names.end();
}

bool ModbusTags::contain_address_in_dwords(addr_t address)
{
    return this->dwords_names.find(address) == this->words_names.end();
}

bool ModbusTags::contain_address_in_coils(addr_t address)
{
    return this->coils_names.find(address) == this->words_names.end();
}

bool ModbusTags::contain_address_in_bits(addr_t address)
{
    return this->bits_names.find(address) == this->bits_names.end();
}

const std::string & ModbusTags::get_word_name(addr_t address)
{
    return this->words_names[address];
}

const std::string & ModbusTags::get_float_name(addr_t address)
{
    return this->floats_names[address];
}

const std::string & ModbusTags::get_dword_name(addr_t address)
{
    return this->dwords_names[address];
}

const std::string & ModbusTags::get_coil_name(addr_t address)
{
    return this->coils_names[address];
}

const std::string & ModbusTags::get_bit_name(addr_t address, unsigned int bit)
{
    return this->bits_names.at(address)[bit];
}

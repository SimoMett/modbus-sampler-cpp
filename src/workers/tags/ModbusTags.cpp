#include <iostream>
#include <format>
#include <regex>
#include "ModbusTags.h"

using simomett::MbValueType;

ModbusTags::ModbusTags(json &tags, bool format_tags)
{
    std::string json_str[4];
    json_str[MbValueType::WORD_TYPE] = "words";
    json_str[MbValueType::DWORD_TYPE] = "dwords";
    json_str[MbValueType::REAL_TYPE] = "floats";
    json_str[MbValueType::COIL_TYPE] = "coils";

    std::unordered_map<addr_t, std::string> *maps[4];
    maps[MbValueType::WORD_TYPE] = &this->mapped_words_names;
    maps[MbValueType::DWORD_TYPE] = &this->mapped_dwords_names;
    maps[MbValueType::REAL_TYPE] = &this->mapped_floats_names;
    maps[MbValueType::COIL_TYPE] = &this->mapped_coils_names;

    std::unordered_map<std::string, addr_t> *rev_maps[4];
    rev_maps[MbValueType::WORD_TYPE] = &this->mapped_names_words;
    rev_maps[MbValueType::DWORD_TYPE] = &this->mapped_names_dwords;
    rev_maps[MbValueType::REAL_TYPE] = &this->mapped_names_floats;
    rev_maps[MbValueType::COIL_TYPE] = &this->mapped_names_coils;

    for (auto v : {MbValueType::WORD_TYPE, MbValueType::DWORD_TYPE, MbValueType::REAL_TYPE, MbValueType::COIL_TYPE})
    {
        if (tags[json_str[v]].is_array())
        {
            for (json s : tags[json_str[v]])
            {
                maps[v]->insert({
                    s["address"].get<addr_t>(),
                    format_tags? format_name(s["tag"].get<std::string>()) : s["tag"].get<std::string>()
                });

                rev_maps[v]->insert({
                    format_tags? format_name(s["tag"].get<std::string>()) : s["tag"].get<std::string>(),
                    s["address"].get<addr_t>()
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
            for(uint8_t i=0; i<16; i++)
            {
                if(j["tags"][i].is_string())
                {
                    rr[i] = format_tags? format_name(j["tags"][i].get<std::string>()) : j["tags"][i].get<std::string>();
                    mapped_names_bits.insert({
                        rr[i],
                        {j["address"].get<addr_t>(), i}
                    });
                }
                else
                    rr[i] = "";
            }

            mapped_bits_names.insert({
                j["address"].get<addr_t>(),
                rr
            });           
        }
    }

   words_addresses = std::vector<addr_t>(this->mapped_words_names.size());
    for(auto e : this->mapped_words_names)
        words_addresses.push_back(e.first);
   floats_addresses = std::vector<addr_t>(this->mapped_floats_names.size());
    for(auto e : this->mapped_floats_names)
        floats_addresses.push_back(e.first);
   dwords_addresses = std::vector<addr_t>(this->mapped_dwords_names.size());
    for(auto e : this->mapped_dwords_names)
        dwords_addresses.push_back(e.first);
   coils_addresses = std::vector<addr_t>(this->mapped_coils_names.size());
    for(auto e : this->mapped_coils_names)
        coils_addresses.push_back(e.first);
   bits_addresses = std::vector<addr_t>(this->mapped_bits_names.size());
    for(auto e : this->mapped_bits_names)
        bits_addresses.push_back(e.first);

   words_names = std::vector<std::string>(this->mapped_names_words.size());
    for(auto e : this->mapped_names_words)
        words_names.push_back(e.first);
   floats_names = std::vector<std::string>(this->mapped_names_floats.size());
    for(auto e : this->mapped_names_floats)
        floats_names.push_back(e.first);
   dwords_names = std::vector<std::string>(this->mapped_names_dwords.size());
    for(auto e : this->mapped_names_dwords)
        dwords_names.push_back(e.first);
   coils_names = std::vector<std::string>(this->mapped_names_coils.size());
    for(auto e : this->mapped_names_coils)
        coils_names.push_back(e.first);
   bits_names = std::vector<std::string>(this->mapped_names_bits.size());
    for(auto e : this->mapped_names_bits)
        bits_names.push_back(e.first);
}

const std::string & ModbusTags::get_word_name(addr_t address)
{
    return this->mapped_words_names[address];
}

const std::string & ModbusTags::get_float_name(addr_t address)
{
    return this->mapped_floats_names[address];
}

const std::string & ModbusTags::get_dword_name(addr_t address)
{
    return this->mapped_dwords_names[address];
}

const std::string & ModbusTags::get_coil_name(addr_t address)
{
    return this->mapped_coils_names[address];
}

const std::string & ModbusTags::get_bit_name(addr_t address, unsigned int bit)
{
    return this->mapped_bits_names.at(address)[bit];
}

const addr_t &ModbusTags::get_word_addr(const std::string & name)
{
    return this->mapped_names_words.at(name);
}

const addr_t &ModbusTags::get_float_addr(const std::string & name)
{
    return this->mapped_names_floats.at(name);
}

const addr_t &ModbusTags::get_dword_addr(const std::string & name)
{
    return this->mapped_names_dwords.at(name);
}

const addr_t &ModbusTags::get_coil_addr(const std::string & name)
{
    return this->mapped_names_coils.at(name);
}

const BitAddress &ModbusTags::get_bit_addr(const std::string & name)
{
    return this->mapped_names_bits.at(name);
}

std::string ModbusTags::format_name(const std::string &name)
{
    // AI generated
    std::string sss = std::regex_replace(name, std::regex("[^a-zA-Z0-9]"), "_");

    sss = std::regex_replace(sss, std::regex("_+"), "_");

    if (!sss.empty() && sss.front() == '_')
        sss.erase(0, 1);

    if (!sss.empty() && sss.back() == '_')
        sss.pop_back();

    return sss;
}

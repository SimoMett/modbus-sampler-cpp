#include <vector>
#include "nlohmann/json.hpp"
#include "simomett/common.h"

#pragma once

using nlohmann::json;

typedef uint32_t addr_t;

template <typename T>
struct AddressValue
{
    addr_t address;
    T val;
};

struct BitAddress
{
    addr_t address;
    uint8_t bit;
};

struct BitAddressValue
{
    addr_t address;
    uint8_t bit;
    bool val;
};

class ModbusTags
{
public:
    ModbusTags(json &tags, bool format_tags = false);

    const std::string & get_word_name(addr_t);
    const std::string & get_float_name(addr_t);
    const std::string & get_dword_name(addr_t);
    const std::string & get_coil_name(addr_t);
    const std::string & get_bit_name(addr_t, unsigned int bit);

    const addr_t & get_word_addr(const std::string &);
    const addr_t & get_float_addr(const std::string &);
    const addr_t & get_dword_addr(const std::string &);
    const addr_t & get_coil_addr(const std::string &);
    const BitAddress & get_bit_addr(const std::string &);

    const std::vector<addr_t> & get_words_addresses() {return this->words_addresses;};
    const std::vector<addr_t> & get_floats_addresses() {return this->floats_addresses;};
    const std::vector<addr_t> & get_dwords_addresses() {return this->dwords_addresses;};
    const std::vector<addr_t> & get_coils_addresses() {return this->coils_addresses;}
    const std::vector<addr_t> & get_bits_addresses() {return this->bits_addresses;};

    const std::vector<std::string> & get_words_names() {return this->words_names;};
    const std::vector<std::string> & get_floats_names() {return this->floats_names;};
    const std::vector<std::string> & get_dwords_names() {return this->dwords_names;};
    const std::vector<std::string> & get_coils_names() {return this->coils_names;};
    const std::vector<std::string> & get_bits_names() {return this->bits_names;};

    static std::string format_name(const std::string & name);

private:
    std::unordered_map<addr_t, std::string> mapped_words_names;
    std::unordered_map<addr_t, std::string> mapped_floats_names;
    std::unordered_map<addr_t, std::string> mapped_dwords_names;
    std::unordered_map<addr_t, std::string> mapped_coils_names;
    std::unordered_map<addr_t, std::array<std::string, 16>> mapped_bits_names;//ugly

    std::unordered_map<std::string, addr_t> mapped_names_words;
    std::unordered_map<std::string, addr_t> mapped_names_floats;
    std::unordered_map<std::string, addr_t> mapped_names_dwords;
    std::unordered_map<std::string, addr_t> mapped_names_coils;
    std::unordered_map<std::string, BitAddress> mapped_names_bits;

    std::vector<addr_t> words_addresses;
    std::vector<addr_t> floats_addresses;
    std::vector<addr_t> dwords_addresses;
    std::vector<addr_t> coils_addresses;
    std::vector<addr_t> bits_addresses;

    std::vector<std::string> words_names;
    std::vector<std::string> floats_names;
    std::vector<std::string> dwords_names;
    std::vector<std::string> coils_names;
    std::vector<std::string> bits_names;
};
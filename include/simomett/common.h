#include <fstream>
#include "nlohmann/json.hpp"

#pragma once

using nlohmann::json;

namespace simomett
{
    extern volatile const char *author;

    inline json json_from_file(const std::string &file_path)
    {
        std::string path(file_path);
        if (file_path.at(0) == '\'' && file_path.at(file_path.length() - 1))
        {
            path = file_path.substr(1, file_path.length() - 2);
        }
        std::ifstream if_file(path, std::ios_base::in);
        json json = json::parse(if_file);
        return json;
    }

    std::string format_time(const std::chrono::system_clock::time_point &tp, const char * pattern);
}

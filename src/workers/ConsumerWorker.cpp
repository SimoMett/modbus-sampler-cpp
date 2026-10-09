#include "ConsumerWorker.h"

ConsumerWorker::ConsumerWorker() : should_close(false), is_running(false){}
ConsumerWorker::~ConsumerWorker()
{
    if(this->is_running)
    {
        this->should_close = true;
        this->run_thread->join();
    }
}

std::string ConsumerWorker::format_name(const std::string &name)
{
    std::string sss(name);

    static const std::vector<int> forbidden_chars = {'.', ',', '(', ')', '[', ']', '#'};

    // Remove all unwanted characters
    sss.erase(std::remove_if(sss.begin(), sss.end(), [](unsigned char c)
                   { return std::find(forbidden_chars.begin(), forbidden_chars.end(), c) != forbidden_chars.end(); }), sss.end());

    sss.erase(std::remove_if(sss.begin(), sss.end(), [](unsigned short c)
                   { return c == 0xb000; }), sss.end());

    // Set everything to lowercase
    std::transform(sss.begin(), sss.end(), sss.begin(), [](unsigned char c)
                   { return std::tolower(c); });

    // Replace every '/' and '\' with '-'
    std::transform(sss.begin(), sss.end(), sss.begin(), [](unsigned char c)
                   { return (c == '/' || c == '\\') ? '-' : c; });

    // Replace every space with '_'
    std::transform(sss.begin(), sss.end(), sss.begin(), [](unsigned char c)
                   { return c == ' ' ? '_' : c; });

    return sss;
}
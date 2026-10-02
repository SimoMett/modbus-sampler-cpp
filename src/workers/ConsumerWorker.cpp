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

    // Remove all dots
    sss.erase(std::remove_if(sss.begin(), sss.end(), [](unsigned char c)
                   { return c == '.'; }), sss.end());

    // Set everything to lowercase
    std::transform(sss.begin(), sss.end(), sss.begin(), [](unsigned char c)
                   { return std::tolower(c); });

    // Replace every '/' with '-'
    std::transform(sss.begin(), sss.end(), sss.begin(), [](unsigned char c)
                   { return c == '/' ? '-' : c; });

    // Replace every space with '_'
    std::transform(sss.begin(), sss.end(), sss.begin(), [](unsigned char c)
                   { return c == ' ' ? '_' : c; });

    return sss;
}
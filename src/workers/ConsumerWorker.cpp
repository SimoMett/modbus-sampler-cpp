#include "ConsumerWorker.h"

ConsumerWorker::ConsumerWorker(){}
ConsumerWorker::~ConsumerWorker(){}

std::string ConsumerWorker::format_name(const std::string &name)
{
    // return str(tag_name).lower().replace(".","").replace("/","-").replace(" ","_")
    std::string sss(name);
    // std::cout << sss << std::endl;
    sss.erase(std::remove_if(sss.begin(), sss.end(), [](unsigned char c)
                   { return c == '.'; }), sss.end());
    std::transform(sss.begin(), sss.end(), sss.begin(), [](unsigned char c)
                   { return std::tolower(c); });
    std::transform(sss.begin(), sss.end(), sss.begin(), [](unsigned char c)
                   { return c == '/' ? '-' : c; });
    std::transform(sss.begin(), sss.end(), sss.begin(), [](unsigned char c)
                   { return c == ' ' ? '_' : c; });

    return sss;
}
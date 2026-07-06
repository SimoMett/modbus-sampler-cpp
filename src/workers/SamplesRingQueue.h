#include <chrono>
#include <deque>
#include "simomett/common.h"

union MbValue
{
    uint16_t word;
    uint32_t dword;
    float real;
};

using simomett::MbValueType;

struct Sample
{
    MbValue val;
    std::chrono::system_clock::time_point time;
};

class SamplesRingQueue
{
public:
    SamplesRingQueue(unsigned short max_len, MbValueType type);

    const MbValueType type;

    void append(Sample sample);
    std::size_t size() const;
    Sample operator [](unsigned short index) const;
    const std::vector<double> & x_data() const;
    const std::vector<double> & y_data() const;
    const std::vector<const char *> data_labels() const;
    void refresh();
    
private:
    const unsigned short max_len;
    std::deque<Sample> queue;
    std::vector<double> data_x;
    std::vector<double> data_y;
    std::vector<std::string> labels;
};
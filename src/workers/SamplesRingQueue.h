#include <chrono>
#include <deque>

union MbValue
{
    uint16_t word;
    uint32_t dword;
    float real;
};

enum MbValueType
{
    WORD_TYPE,
    DWORD_TYPE,
    REAL_TYPE
};

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
    const std::vector<float> & x_data() const;
    const std::vector<float> & y_data() const;
    const std::vector<const char *> data_labels() const;
    void refresh();
    
private:
    const unsigned short max_len;
    std::deque<Sample> queue;
    std::vector<float> data_x;
    std::vector<float> data_y;
    std::vector<std::string> labels;
};
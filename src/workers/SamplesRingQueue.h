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

    void append(Sample sample);
    std::size_t size() const;
    Sample operator [](unsigned short index) const;

    const MbValueType type;
private:
    const unsigned short max_len;
    std::deque<Sample> queue;
};
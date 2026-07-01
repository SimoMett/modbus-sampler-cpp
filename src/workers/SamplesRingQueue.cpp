#include <iostream>
#include "SamplesRingQueue.h"
#include "simomett/common.h"

SamplesRingQueue::SamplesRingQueue(unsigned short max_len, MbValueType type) : max_len(max_len), type(type) {}

void SamplesRingQueue::append(Sample sample)
{
    queue.push_back(sample);
    if (queue.size() == max_len + 1)
        queue.pop_front();
}

std::size_t SamplesRingQueue::size() const
{
    return queue.size();
}

Sample SamplesRingQueue::operator[](unsigned short index) const
{
    // occhio
    return Sample(queue[index]);
}

const std::vector<float> & SamplesRingQueue::x_data() const
{
    return data_x;
}

const std::vector<float> & SamplesRingQueue::y_data() const
{
    return data_y;
}

const std::vector<const char *> SamplesRingQueue::data_labels() const
{
    std::vector<const char*> v;
    for(int i=0; i < labels.size(); i++)
        v.push_back(labels[i].c_str());

    return v;
}

void SamplesRingQueue::refresh()
{
    data_x.clear();
    data_y.clear();
    labels.clear();
    for (int i = 0; i < queue.size(); i++)
    {
        auto sample = queue[i];
        auto time = sample.time;
        switch (type)
        {
        case MbValueType::WORD_TYPE:
            data_y.push_back((float)(sample.val.word));
            break;
        case MbValueType::DWORD_TYPE:
            data_y.push_back((float)(sample.val.dword));
            break;
        case MbValueType::REAL_TYPE:
            data_y.push_back(sample.val.real);
            break;
        }

        labels.push_back(simomett::format_time(time, "%H:%M:%S"));
        data_x.push_back((float)i);
    }
}
